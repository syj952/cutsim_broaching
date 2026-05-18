
// 文件名: src/cutsim/cuda_functions.cpp
#include "cuda_functions.hpp"
#include "octree.hpp"
#include "octnode.hpp"
#include "volume.hpp"
#include <cstdio>
#include <cuda_runtime.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <stack>
//#include "cuda_diff_volume.hpp"

// 确保结构体内存对齐一致
#pragma pack(push, 8)

struct CudaNodeData {
    float x[8];  // 节点顶点的 x 坐标
    float y[8];  // 节点顶点的 y 坐标
    float z[8];  // 节点顶点的 z 坐标
    float f[8];  // 每个顶点的距离值
    int node_id[8];    // 新增全局节点ID
};

// 定义 VolumeParams 结构体
struct VolumeParams {
    // 修改枚举定义，使用int类型确保大小一致
    int type;  // 使用整数代替枚举，避免类型不匹配问题

    // 定义常量，与C++代码保持一致
    static const int SPHERE_VOLUME = 0;
    static const int CYLINDER_VOLUME = 1;
    static const int RECTANGLE_VOLUME = 2;

    // 为每个参数结构体添加类型声明
    struct SphereParams {
        float x, y, z;
        float radius;
        float r, g, b;
    };

    struct CylinderParams {
        float x, y, z;
        float radius;
        float length;
        float angle_x, angle_y, angle_z;
        float r, g, b;
        double holderradius;
    };
    struct RectangleParams {
        float center_x, center_y, center_z;
        float length_x, length_y, length_z;
        float r, g, b;
    };

    union {
        SphereParams sphere;
        CylinderParams cylinder;
        RectangleParams rectangle;
    } params;
};

struct GLVertex_xyz {
    float x, y, z;
    // 可以添加其他需要的成员
};

struct BladePoint {
    GLVertex_xyz v1, v2, v3, v4;
};

struct BladeParams {
    float center_x, center_y, center_z;
    float dx, dy, dz;
    float cube_resolution_1;
    double* point_r_blade;  // 设备指针
    int point_r_count;      // 点的数量
    GLVertex_xyz* blade_points;  // 当前切削刃的点数组指针
    int blade_points_count;  // 当前切削刃的点数量
    int blade_id;           // 当前处理的切削刃ID
    int device_id;

    // 前刀面平面信息
    int plane_count;        // 平面数量
    GLVertex_xyz* plane_normals;  // 平面法向量
    GLVertex_xyz* plane_points;   // 平面上的点
    // 这里可以根据需要添加更多参数
};


extern "C" void cuda_diff_volume_blade(CudaNodeData * host_nodes, int numNodes, BladeParams host_blade,
    float** host_z_array, float** host_distence2edge, int** host_node_ids, int* host_record_count);
extern "C" void cuda_diff_volume(CudaNodeData * host_nodes, int numNodes, VolumeParams host_volume);


#pragma pack(pop)


namespace cutsim {


    // 构造函数，初始化CUDA状态
    cuda_functions::cuda_functions() : deviceCount(0) {
        cudaError_t cudaStatus = cudaGetDeviceCount(&deviceCount);
        if (cudaStatus != cudaSuccess || deviceCount == 0) {
            qDebug() << "没有找到CUDA设备或CUDA初始化失败: " << cudaGetErrorString(cudaStatus);
        }
        else {
            //qDebug() << "找到" << deviceCount << "个CUDA设备";
            cudaStatus = cudaSetDevice(0);
            if (cudaStatus != cudaSuccess) {
                qDebug() << "设置CUDA设备失败: " << cudaGetErrorString(cudaStatus);
            }
            else {
                // 获取设备属性
                cudaDeviceProp deviceProp;
                cudaGetDeviceProperties(&deviceProp, 0);
                //qDebug() << "使用设备:" << deviceProp.name;
                //qDebug() << "计算能力:" << deviceProp.major << "." << deviceProp.minor;
            }
        }
    }

    void cuda_functions::clean_outside_nodes(Octnode* current, AptCutterVolume* vol) {
        if (!current) {
            return; // 如果节点为空，直接返回
        }

        // 获取包围盒的8个顶点
        const auto& bb = vol->bb_points;
        float3 min_coord = {
            std::min({std::get<0>(bb).x, std::get<1>(bb).x, std::get<2>(bb).x, std::get<3>(bb).x,
                      std::get<4>(bb).x, std::get<5>(bb).x, std::get<6>(bb).x, std::get<7>(bb).x}),
            std::min({std::get<0>(bb).y, std::get<1>(bb).y, std::get<2>(bb).y, std::get<3>(bb).y,
                      std::get<4>(bb).y, std::get<5>(bb).y, std::get<6>(bb).y, std::get<7>(bb).y}),
            std::min({std::get<0>(bb).z, std::get<1>(bb).z, std::get<2>(bb).z, std::get<3>(bb).z,
                      std::get<4>(bb).z, std::get<5>(bb).z, std::get<6>(bb).z, std::get<7>(bb).z})
        };
        float3 max_coord = {
            std::max({std::get<0>(bb).x, std::get<1>(bb).x, std::get<2>(bb).x, std::get<3>(bb).x,
                      std::get<4>(bb).x, std::get<5>(bb).x, std::get<6>(bb).x, std::get<7>(bb).x}),
            std::max({std::get<0>(bb).y, std::get<1>(bb).y, std::get<2>(bb).y, std::get<3>(bb).y,
                      std::get<4>(bb).y, std::get<5>(bb).y, std::get<6>(bb).y, std::get<7>(bb).y}),
            std::max({std::get<0>(bb).z, std::get<1>(bb).z, std::get<2>(bb).z, std::get<3>(bb).z,
                      std::get<4>(bb).z, std::get<5>(bb).z, std::get<6>(bb).z, std::get<7>(bb).z})
        };

        GLVertex* v = current->center;
        float3 p{ v->x, v->y, v->z };

        // 检查点是否在包围盒内
        bool inside = (p.x >= min_coord.x - current->scale && p.x <= max_coord.x + current->scale &&
            p.y >= min_coord.y - current->scale && p.y <= max_coord.y + current->scale &&
            p.z >= min_coord.z - current->scale && p.z <= max_coord.z + current->scale);

        if (!inside) {
            return;
        }


        // 如果当前节点是叶子节点，直接返回
        if (current->isLeaf()) {
            return;
        }

        // 检查是否有子节点
        if (current->childcount == 8) {
            // 先递归处理所有子节点
            bool all_outside = true;
            for (int n = 0; n < 8; ++n) {
                if (current->child[n]) {
                    clean_outside_nodes(current->child[n], vol);
                    // 检查子节点是否是OUTSIDE
                    if (!current->child[n]->is_outside()) {
                        all_outside = false;
                    }
                }
            }

            // 新增：如果所有子节点都是OUTSIDE，清理当前节点
            if (all_outside) {
                current->state = Octnode::OUTSIDE;  // 标记父节点为OUTSIDE
                current->delete_children();         // 删除所有子节点
            }
        }
    }


    // 析构函数，清理CUDA资源
    cuda_functions::~cuda_functions() {}

    void cuda_functions::get_leaf_nodes_diff(Octnode* current, std::vector<Octnode*>& nodes_to_process, const Volume* vol, unsigned int max_depth) {
        // 如果节点已经在外部或没有与体积重叠，则直接返回
        if (current->is_outside() || !vol->bb.overlaps(current->bb)) {
            return;
        }


        // 首先判断是否有子节点
        if (current->childcount == 8) {
            // 检查子节点是否是outside
            for (int n = 0; n < 8; ++n) {
                if (!current->child[n]->is_outside()) {
                    get_leaf_nodes_diff(current->child[n], nodes_to_process, vol, max_depth);
                }
            }
            // 如果所有子节点都是outside，则不添加到nodes_to_process
        }
        else {
            // 没有子节点
            // 如果不是undecided状态且不是outside，判断是否达到最大深度
            if (current->depth < (max_depth - 1)) {
                if (!current->is_undecided()) { current->force_setUndecided(); }
                // 未达到最大深度，设置为undecided并加入处理列表
                // 如果是undecided状态，不加入到处理列表
                current->subdivide();
                for (int m = 0; m < 8; ++m) {
                    get_leaf_nodes_diff(current->child[m], nodes_to_process, vol, max_depth);
                }
            }
            else
                // 如果已达到最大深度，则加入处理列表
            {
                nodes_to_process.push_back(current);
            }
        }
    }

    // 判断点是否在三角形内
    bool cuda_functions::pointInTriangle(float2 a, float2 b, float2 c, float2 p) {
        auto cross = [](float2 a, float2 b, float2 c) {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        };
        float c1 = cross(a, b, p);
        float c2 = cross(b, c, p);
        float c3 = cross(c, a, p);
        return (c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0);
    }

    // 计算点到线段距离
    float cuda_functions::pointToSegmentDist(float2 p, float2 a, float2 b) {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        if (dx == 0 && dy == 0) return hypotf(p.x - a.x, p.y - a.y);
        float t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / (dx * dx + dy * dy);
        t = fmaxf(0.0f, fminf(1.0f, t));
        float proj_x = a.x + t * dx;
        float proj_y = a.y + t * dy;
        return hypotf(p.x - proj_x, p.y - proj_y);
    }

    void cuda_functions::get_leaf_nodes_diff_blade(Octnode* current, std::vector<Octnode*>& nodes_to_process, AptCutterVolume* vol, unsigned int max_depth, std::chrono::duration<double>& elapsed, int blade_id) {
        // 如果节点已经在外部或没有与体积重叠，则直接返回
        if (current->depth > (vol->max_depth_1 - 1) || current->is_outside()) {
            return;
        }

        // 获取包围盒的8个顶点
        const auto& bb = vol->bb_points;
        float3 min_coord = {
            std::min({std::get<0>(bb).x, std::get<1>(bb).x, std::get<2>(bb).x, std::get<3>(bb).x,
                      std::get<4>(bb).x, std::get<5>(bb).x, std::get<6>(bb).x, std::get<7>(bb).x}),
            std::min({std::get<0>(bb).y, std::get<1>(bb).y, std::get<2>(bb).y, std::get<3>(bb).y,
                      std::get<4>(bb).y, std::get<5>(bb).y, std::get<6>(bb).y, std::get<7>(bb).y}),
            std::min({std::get<0>(bb).z, std::get<1>(bb).z, std::get<2>(bb).z, std::get<3>(bb).z,
                      std::get<4>(bb).z, std::get<5>(bb).z, std::get<6>(bb).z, std::get<7>(bb).z})
        };
        float3 max_coord = {
            std::max({std::get<0>(bb).x, std::get<1>(bb).x, std::get<2>(bb).x, std::get<3>(bb).x,
                      std::get<4>(bb).x, std::get<5>(bb).x, std::get<6>(bb).x, std::get<7>(bb).x}),
            std::max({std::get<0>(bb).y, std::get<1>(bb).y, std::get<2>(bb).y, std::get<3>(bb).y,
                      std::get<4>(bb).y, std::get<5>(bb).y, std::get<6>(bb).y, std::get<7>(bb).y}),
            std::max({std::get<0>(bb).z, std::get<1>(bb).z, std::get<2>(bb).z, std::get<3>(bb).z,
                      std::get<4>(bb).z, std::get<5>(bb).z, std::get<6>(bb).z, std::get<7>(bb).z})
        };

        GLVertex* v = current->center;
        float3 p{ v->x, v->y, v->z };

        // 检查点是否在包围盒内
        bool inside = (p.x >= min_coord.x - current->scale && p.x <= max_coord.x + current->scale &&
            p.y >= min_coord.y - current->scale && p.y <= max_coord.y + current->scale &&
            p.z >= min_coord.z - current->scale && p.z <= max_coord.z + current->scale);

        if (!inside) {
            return;
        }

        // 首先判断是否有子节点
        if (current->childcount == 8 && current->depth < (vol->max_depth_1 - 1)) {
            // 检查子节点是否是outside
            for (int n = 0; n < 8; ++n) {
                if (current->depth > MFEM_DEPTH-1) {
                    // 三线性插值计算子顶点振动幅值
                    for (int k = 0; k < 8; ++k) {
                        // 获取子顶点在父立方体中的相对位置 (范围[-1,1])
                        const GLVertex* dir = Octnode::getDirection();
                        double tx = (dir[k].x + 1) * 0.5;  // 修改坐标映射 [-1,1] => [0,1]
                        double ty = (dir[k].y + 1) * 0.5;
                        double tz = (dir[k].z + 1) * 0.5;

                        // 遍历所有模态和方向
                        for (int modal = 0; modal < vol->temp_vibration_vectors.size(); modal++) {
                            for (int dir = 0; dir < 3; dir++) {
                                // 三线性插值公式
                                int node_id;
                                if (current->vertex[3]->id <= 0)
                                    node_id = -current->vertex[3]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[3]->id + vol->normalvertices_size - 1;
                                double c000;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c000 = 0; }
                                else    c000 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[2]->id <= 0)
                                    node_id = -current->vertex[2]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[2]->id + vol->normalvertices_size - 1;
                                double c100;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c100 = 0; }
                                else    c100 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[1]->id <= 0)
                                    node_id = -current->vertex[1]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[1]->id + vol->normalvertices_size - 1;
                                double c010;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c010 = 0; }
                                else    c010 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[0]->id <= 0)
                                    node_id = -current->vertex[0]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[0]->id + vol->normalvertices_size - 1;
                                double c110;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c110 = 0; }
                                else    c110 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[7]->id <= 0)
                                    node_id = -current->vertex[7]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[7]->id + vol->normalvertices_size - 1;
                                double c001;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c001 = 0; }
                                else    c001 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[6]->id <= 0)
                                    node_id = -current->vertex[6]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[6]->id + vol->normalvertices_size - 1;
                                double c101;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c101 = 0; }
                                else    c101 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[5]->id <= 0)
                                    node_id = -current->vertex[5]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[5]->id + vol->normalvertices_size - 1;
                                double c011;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c011 = 0; }
                                else    c011 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[4]->id <= 0)
                                    node_id = -current->vertex[4]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[4]->id + vol->normalvertices_size - 1;
                                double c111;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c111 = 0; }
                                else    c111 = vol->temp_vibration_vectors[modal][dir][node_id];
                                //printf("%f,%f,%f,%f,%f,%f,%f,%f\n",c000,c100,c010,c110,c001,c101,c011,c111);

                                // 计算插值结果
                                double interp =
                                    c000 * (1 - tx) * (1 - ty) * (1 - tz) +
                                    c100 * tx * (1 - ty) * (1 - tz) +
                                    c010 * (1 - tx) * ty * (1 - tz) +
                                    c110 * tx * ty * (1 - tz) +
                                    c001 * (1 - tx) * (1 - ty) * tz +
                                    c101 * tx * (1 - ty) * tz +
                                    c011 * (1 - tx) * ty * tz +
                                    c111 * tx * ty * tz;
                                //printf("%f\n",interp);

                                // 更新子节点振动幅值并插入到vectors
                                vol->temp_vibration_vectors[modal][dir].push_back(interp);
                                current->child[n]->vertex[k]->id = -(vol->temp_vibration_vectors[modal][dir].size() - 1);
                            }
                        }
                    }
                }

                if (!current->child[n]->is_outside()) {
                    get_leaf_nodes_diff_blade(current->child[n], nodes_to_process, vol, max_depth, elapsed, blade_id);
                }
            }
            // 如果所有子节点都是outside，则不添加到nodes_to_process
        }
        else {
            // 没有子节点
            // 如果不是undecided状态且不是outside，判断是否达到最大深度
            if (current->depth < (vol->max_depth_1 - 1)) {
                if (!current->is_undecided()) { current->force_setUndecided(); }
                // 未达到最大深度，设置为undecided并加入处理列表
                // 如果是undecided状态，不加入到处理列表
                current->subdivide();

                for (int m = 0; m < 8; ++m) {
                    // 三线性插值计算子顶点振动幅值
                    for (int k = 0; k < 8; ++k) {
                        // 获取子顶点在父立方体中的相对位置 (范围[-1,1])
                        const GLVertex* dir = Octnode::getDirection();
                        double tx = dir[k].x * 0.5;
                        double ty = dir[k].y * 0.5;
                        double tz = dir[k].z * 0.5;

                        // 遍历所有模态和方向
                        for (int modal = 0; modal < vol->temp_vibration_vectors.size(); modal++) {
                            for (int dir = 0; dir < 3; dir++) {
                                // 三线性插值公式
                                int node_id;
                                if (current->vertex[3]->id <= 0)
                                    node_id = -current->vertex[3]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[3]->id + vol->normalvertices_size - 1;
                                double c000;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c000 = 0; }
                                else    c000 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[2]->id <= 0)
                                    node_id = -current->vertex[2]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[2]->id + vol->normalvertices_size - 1;
                                double c100;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c100 = 0; }
                                else    c100 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[1]->id <= 0)
                                    node_id = -current->vertex[1]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[1]->id + vol->normalvertices_size - 1;
                                double c010;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c010 = 0; }
                                else    c010 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[0]->id <= 0)
                                    node_id = -current->vertex[0]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[0]->id + vol->normalvertices_size - 1;
                                double c110;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c110 = 0; }
                                else    c110 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[7]->id <= 0)
                                    node_id = -current->vertex[7]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[7]->id + vol->normalvertices_size - 1;
                                double c001;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c001 = 0; }
                                else    c001 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[6]->id <= 0)
                                    node_id = -current->vertex[6]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[6]->id + vol->normalvertices_size - 1;
                                double c101;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c101 = 0; }
                                else    c101 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[5]->id <= 0)
                                    node_id = -current->vertex[5]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[5]->id + vol->normalvertices_size - 1;
                                double c011;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c011 = 0; }
                                else    c011 = vol->temp_vibration_vectors[modal][dir][node_id];

                                if (current->vertex[4]->id <= 0)
                                    node_id = -current->vertex[4]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[4]->id + vol->normalvertices_size - 1;
                                double c111;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) { c111 = 0; }
                                else    c111 = vol->temp_vibration_vectors[modal][dir][node_id];

                                //printf("%f,%f,%f,%f,%f,%f,%f,%f\n",c000,c100,c010,c110,c001,c101,c011,c111);

                                // 计算插值结果
                                double interp =
                                    c000 * (1 - tx) * (1 - ty) * (1 - tz) +
                                    c100 * tx * (1 - ty) * (1 - tz) +
                                    c010 * (1 - tx) * ty * (1 - tz) +
                                    c110 * tx * ty * (1 - tz) +
                                    c001 * (1 - tx) * (1 - ty) * tz +
                                    c101 * tx * (1 - ty) * tz +
                                    c011 * (1 - tx) * ty * tz +
                                    c111 * tx * ty * tz;

                                //printf("%f\n",interp);

                                // 更新子节点振动幅值并插入到vectors
                                vol->temp_vibration_vectors[modal][dir].push_back(interp);
                                current->child[m]->vertex[k]->id = -(vol->temp_vibration_vectors[modal][dir].size() - 1);
                            }
                        }
                    }
                    get_leaf_nodes_diff_blade(current->child[m], nodes_to_process, vol, max_depth, elapsed, blade_id);
                }
            }
            else
                // 如果已达到最大深度，则加入处理列表
            {
                nodes_to_process.push_back(current);
            }

        }
    }

    void cuda_functions::get_leaf_nodes_sum(Octnode* current, std::vector<Octnode*>& nodes_to_process, const Volume* vol, unsigned int max_depth) {
        //qDebug() << "vol->type():"<<vol->type;
        // 如果节点已经在外部或没有与体积重叠，则直接返回
        if (current->is_inside() || !vol->bb.overlaps(current->bb)) {
            return;
        }

        if ((current->depth == (max_depth - 1)) && current->is_undecided() && !current->color.compareColor(vol->color)) { // return;
            return;
        }
        //qDebug() << "get_leaf_nodes_sum():";
        // 首先判断是否有子节点
        if (current->childcount == 8) {
            // 检查子节点是否是inside
            for (int n = 0; n < 8; ++n) {
                if (!current->child[n]->is_inside()) {
                    get_leaf_nodes_sum(current->child[n], nodes_to_process, vol, max_depth);
                }
            }
            // 如果所有子节点都是inside，则不添加到nodes_to_process
        }
        else {
            // 没有子节点
            // 如果不是undecided状态且不是inside，判断是否达到最大深度
            if (current->depth < (max_depth - 1)) {
                if (!current->is_undecided()) { current->force_setUndecided(); }
                // 未达到最大深度，设置为undecided并加入处理列表
                // 如果是undecided状态，不加入到处理列表
                current->subdivide();
                for (int m = 0; m < 8; ++m) {
                    get_leaf_nodes_sum(current->child[m], nodes_to_process, vol, max_depth);
                }
            }
            else
                //qDebug() << "current->scale:" <<current->scale;
                // 如果已达到最大深度，则加入处理列表
            {
                nodes_to_process.push_back(current);
            }
        }


    }

    void cuda_functions::sum_volume(Octnode* current, const Volume* vol, unsigned int max_depth) { // 添加max_depth参数


        // 准备要处理的节点列表
        std::vector<Octnode*> nodes_to_process;

        //std::chrono::system_clock::time_point start, stop;
        //start = std::chrono::system_clock::now();
        qDebug() << "sum_volume():";
        // 收集所有需要处理的叶节点
        get_leaf_nodes_sum(current, nodes_to_process, vol, max_depth);

        //stop = std::chrono::system_clock::now();
        //qDebug() << "get_leaf_nodes():" << std::chrono::duration<double>(stop - start).count() << "sec.";
        //qDebug() << std::chrono::duration<double>(stop - start).count();


        // 如果没有需要处理的节点，直接返回
        if (nodes_to_process.empty()) {
            qDebug() << "列表为空,计算结束";
            return;
        }

        // 分配节点数据数组，用于传输到GPU
        size_t node_count = nodes_to_process.size();
        // 在文件顶部添加类型别名
        // 修改类型别名为直接使用命名空间中的VolumeType
        typedef cutsim::VolumeType VolumeType;

        // 修改数组声明部分（约92行）
        CudaNodeData* host_nodes = new CudaNodeData[node_count];  // 改为动态数组

        // 修改参数填充循环（约94-103行）
        for (size_t i = 0; i < node_count; i++) {
            Octnode* node = nodes_to_process[i];
            for (int j = 0; j < 8; j++) {
                host_nodes[i].x[j] = static_cast<float>(node->vertex[j]->x);
                host_nodes[i].y[j] = static_cast<float>(node->vertex[j]->y);
                host_nodes[i].z[j] = static_cast<float>(node->vertex[j]->z);
                host_nodes[i].f[j] = static_cast<float>(node->f[j]);
            }
        }

        // 修改类型转换部分（约113行和126行）
        // 在参数填充循环之后添加变量声明（约109行）
        VolumeParams volume_params;  // 声明结构体实例

        // 修改类型转换部分（原110行）
        volume_params.type = vol->type;

        bool supported_volume = true;

        // 根据体积类型填充参数
        switch (vol->type) {

        case CYLINDER_VOLUME: {
            const CylCutterVolume* cv = static_cast<const CylCutterVolume*>(vol);  // 安全转换
            volume_params.type = VolumeParams::CYLINDER_VOLUME;

            // 使用公有访问方法获取私有成员
            GLVertex center = cv->center;
            GLVertex angle = cv->angle;          // 需要在volume.hpp中添加对应方法


            volume_params.params.cylinder.x = center.x;
            volume_params.params.cylinder.y = center.y;
            volume_params.params.cylinder.z = center.z;
            volume_params.params.cylinder.radius = cv->radius;
            volume_params.params.cylinder.length = cv->length;

            // 修改角度参数赋值
            volume_params.params.cylinder.angle_x = angle.x;
            volume_params.params.cylinder.angle_y = angle.y;
            volume_params.params.cylinder.angle_z = angle.z;

            volume_params.params.cylinder.r = cv->color.r;
            volume_params.params.cylinder.g = cv->color.g;
            volume_params.params.cylinder.b = cv->color.b;

            volume_params.params.cylinder.holderradius = cv->holderradius;
            break;
        }

        case RECTANGLE_VOLUME: {  // 新增矩形体积分支
            const RectVolume* rv = static_cast<const RectVolume*>(vol);
            volume_params.type = VolumeParams::RECTANGLE_VOLUME;

            // 获取矩形中心坐标
            GLVertex center = rv->getCenter();
            volume_params.params.rectangle.center_x = center.x;
            volume_params.params.rectangle.center_y = center.y;
            volume_params.params.rectangle.center_z = center.z;

            // 设置矩形尺寸
            volume_params.params.rectangle.length_x = rv->getLengthX();
            volume_params.params.rectangle.length_y = rv->getLengthY();
            volume_params.params.rectangle.length_z = rv->getLengthZ();

            // 设置颜色参数
            volume_params.params.rectangle.r = rv->color.r;
            volume_params.params.rectangle.g = rv->color.g;
            volume_params.params.rectangle.b = rv->color.b;
            break;
        }
        }


        //qDebug() << "调用cuda_diff_volume";

        // 添加CUDA错误检查
        cudaError_t cudaStatus = cudaGetLastError();
        if (cudaStatus != cudaSuccess) {
            qDebug() << "CUDA初始化错误:" << cudaGetErrorString(cudaStatus);
            delete[] host_nodes;
            return;
        }

        try {
            // 调用CUDA实现
            //qDebug() << "开始调用CUDA函数...";

            //start = std::chrono::system_clock::now();

            cuda_diff_volume(host_nodes, node_count, volume_params);

            //stop = std::chrono::system_clock::now();
            //qDebug() << "cuda_diff_volume() :" << std::chrono::duration<double>(stop - start).count() << "sec.";
            //qDebug() << std::chrono::duration<double>(stop - start).count();
            //qDebug() << "CUDA函数调用成功";

            // 检查CUDA执行错误
            cudaStatus = cudaGetLastError();
            if (cudaStatus != cudaSuccess) {
                qDebug() << "CUDA执行错误:" << cudaGetErrorString(cudaStatus);
                throw std::runtime_error("CUDA执行错误");
            }

            // 同步设备
            cudaStatus = cudaDeviceSynchronize();
            if (cudaStatus != cudaSuccess) {
                qDebug() << "CUDA同步错误:" << cudaGetErrorString(cudaStatus);
                throw std::runtime_error("CUDA同步错误");
            }
        }
        catch (const std::exception& e) {
            qDebug() << "CUDA函数调用异常:" << e.what();
            // 释放内存并回退到CPU实现
            delete[] host_nodes;
            for (size_t i = 0; i < node_count; i++) {
                nodes_to_process[i]->diff(vol);
                nodes_to_process[i]->set_state();
            }
            return;
        }

        // 更新节点数据
        for (size_t i = 0; i < node_count; i++) {
            Octnode* node = nodes_to_process[i];
            //        qDebug() << "node_count:" <<i;
            //            if(i==0){
            //                qDebug() << "node更新:" <<node->f[0];
            //            }
            if (node->f[0] == host_nodes[i].f[0]) {
                qDebug() << "error：dist未更新:" << (node->f[0] == host_nodes[i].f[0]);

            }

            bool updated = false; // 标记是否有更新
            for (int j = 0; j < 8; j++) {
                if (static_cast<double>(host_nodes[i].f[j]) > node->f[j]) {
                    node->f[j] = static_cast<double>(host_nodes[i].f[j]);
                    updated = true; // 标记有更新
                }
            }

            // 如果有更新，则更新节点颜色
            if (updated) {
                //qDebug() << "颜色更新";
                node->color = vol->color; // 更新颜色为体积的颜色
            }

            node->set_state(); // 更新节点状态


        }

        // 释放内存A
        delete[] host_nodes;

    }


    // ... 其他代码 ...
    void cuda_functions::diff_volume_blade(Octnode* current, AptCutterVolume* vol, unsigned int max_depth, Octree* octree) {
        // 收集叶节点

        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed(0);

        nodes_to_process.clear();

        // 默认处理第0个切削刃，实际使用时应该根据需要指定blade_id
        int blade_id = vol->blade_num;
        get_leaf_nodes_diff_blade(current, nodes_to_process, vol, max_depth, elapsed, blade_id);

        //MeshIDExport(octree);

        stop = std::chrono::system_clock::now();
        qDebug() << "get_leaf_nodes_diff_blade():" << std::chrono::duration<double>(stop - start).count() << "sec.";
        //       qDebug() << "get_leaf_nodes_diff_blade_second():" << elapsed.count() << "sec.";

        start = std::chrono::system_clock::now();

        size_t node_count = nodes_to_process.size();
        if (node_count == 0) return;

        CudaNodeData* host_nodes = new CudaNodeData[node_count];
        for (size_t i = 0; i < node_count; i++) {
            Octnode* node = nodes_to_process[i];
            for (int j = 0; j < 8; j++) {
                host_nodes[i].x[j] = static_cast<float>(node->vertex[j]->x);
                host_nodes[i].y[j] = static_cast<float>(node->vertex[j]->y);
                host_nodes[i].z[j] = static_cast<float>(node->vertex[j]->z);
                host_nodes[i].f[j] = static_cast<float>(node->f[j]);

                int node_id;
                if (node->vertex[j]->id <= 0)
                    node_id = -node->vertex[j]->id; // 正常节点编号
                else
                    node_id = node->vertex[j]->id + vol->normalvertices_size - 1; // 悬挂节点编号
                host_nodes[i].node_id[j] = node_id;
            }
        }

        BladeParams blade;
        blade.center_x = static_cast<float>(vol->center.x);
        blade.center_y = static_cast<float>(vol->center.y);
        blade.center_z = static_cast<float>(vol->center.z);
        blade.dx = static_cast<float>(vol->dx);  // 新增dx赋值
        blade.dy = static_cast<float>(vol->dy);  // 新增dy赋值
        blade.dz = static_cast<float>(vol->dz);  // 新增dz赋值
        blade.cube_resolution_1 = static_cast<float>(vol->cube_resolution_1);
        blade.device_id = 0;  // 设置设备ID

        // 处理blade_points数据
        blade.blade_id = blade_id;

        if (!vol->blade_points.empty() && blade_id < vol->blade_points.size()) {
            const auto& current_blade_points = vol->blade_points[blade_id];
            // 分配主机内存
            blade.blade_points = new GLVertex_xyz[current_blade_points.size()];
            // 拷贝数据
            for (size_t i = 0; i < current_blade_points.size(); ++i) {
                blade.blade_points[i].x = static_cast<float>(current_blade_points[i].x);
                blade.blade_points[i].y = static_cast<float>(current_blade_points[i].y);
                blade.blade_points[i].z = static_cast<float>(current_blade_points[i].z);
            }

            blade.blade_points_count = static_cast<int>(current_blade_points.size());
        }
        else {
            blade.blade_points = nullptr;
            blade.blade_points_count = 0;
        }

        // 处理前刀面平面信息
        blade.plane_count = static_cast<int>(vol->planes.size());
        if (!vol->planes.empty()) {
            // 分配内存
            blade.plane_normals = new GLVertex_xyz[vol->planes.size()];
            blade.plane_points = new GLVertex_xyz[vol->planes.size()];

            // 拷贝数据
            for (size_t i = 0; i < vol->planes.size(); i++) {
                // 平面法向量
                blade.plane_normals[i].x = static_cast<float>(vol->planes[i].normal.x);
                blade.plane_normals[i].y = static_cast<float>(vol->planes[i].normal.y);
                blade.plane_normals[i].z = static_cast<float>(vol->planes[i].normal.z);

                // 平面上的点（加上刀具中心偏移）
                blade.plane_points[i].x = static_cast<float>(vol->planes[i].point.x + vol->center.x);
                blade.plane_points[i].y = static_cast<float>(vol->planes[i].point.y + vol->center.y);
                blade.plane_points[i].z = static_cast<float>(vol->planes[i].point.z + vol->center.z);
            }
        }
        else {
            blade.plane_normals = nullptr;
            blade.plane_points = nullptr;
        }

        float* host_z_array = nullptr;
        float* host_distence2edge = nullptr;
        int* host_node_ids = nullptr;  // 新增
        int host_record_count = 0;

        // 添加CUDA错误检查
        cudaError_t cudaStatus = cudaGetLastError();
        if (cudaStatus != cudaSuccess) {
            qDebug() << "CUDA初始化错误:" << cudaGetErrorString(cudaStatus);
            delete[] host_nodes;
            return;
        }

        // 配置BladeParams
        blade.device_id = 0;  // 设置设备ID

        stop = std::chrono::system_clock::now();
        qDebug() << "设备内存操作():" << std::chrono::duration<double>(stop - start).count() << "sec.";


        try {
            cudaSetDevice(0);
            cudaDeviceSynchronize();
            cudaError_t preLaunchErr = cudaGetLastError();
            if (preLaunchErr != cudaSuccess) {
                qDebug() << "Pre-launch error:" << cudaGetErrorString(preLaunchErr);
                return;
            }

            start = std::chrono::system_clock::now();

            cuda_diff_volume_blade(host_nodes, node_count, blade,
                &host_z_array, &host_distence2edge, &host_node_ids, &host_record_count);

            stop = std::chrono::system_clock::now();
            qDebug() << "cuda计算时间():" << std::chrono::duration<double>(stop - start).count() << "sec.";

            // 检查CUDA执行错误
            cudaStatus = cudaGetLastError();
            if (cudaStatus != cudaSuccess) {
                qDebug() << "CUDA执行错误:" << cudaGetErrorString(cudaStatus);
                throw std::runtime_error("CUDA执行错误");
            }

            // 同步设备
            cudaStatus = cudaDeviceSynchronize();
            if (cudaStatus != cudaSuccess) {
                qDebug() << "CUDA同步错误:" << cudaGetErrorString(cudaStatus);
                throw std::runtime_error("CUDA同步错误");
            }
        }
        catch (const std::exception& e) {
            qDebug() << "CUDA函数调用异常:" << e.what();
            // 释放内存并回退到CPU实现
            delete[] host_nodes;
            for (size_t i = 0; i < node_count; i++) {
                nodes_to_process[i]->diff(vol);
                nodes_to_process[i]->set_state();
            }
            return;
        }

        start = std::chrono::system_clock::now();

        // 修改后的数据结构
        struct ZData {
            double min_d2edge = FLT_MAX;
            int min_node_id = -1;
            bool valid = false;
        };

        // 找到最大的inside_index，用于预分配vector大小
        int max_inside_index = 0;
        for (int i = 0; i < host_record_count; ++i) {
            int inside_index = host_z_array[i];
            if (inside_index > max_inside_index) {
                max_inside_index = inside_index;
            }
        }

        // 使用vector代替unordered_map，提高访问速度和缓存友好性
        std::vector<ZData> z2sum_count(max_inside_index + 1);

        for (int i = 0; i < host_record_count; ++i) {

            int node_id = host_node_ids[i];
            int inside_index = host_z_array[i];
            double d2edge = host_distence2edge[i];

            // 确保vector大小足够
            if (inside_index >= static_cast<int>(z2sum_count.size())) {
                z2sum_count.resize(inside_index + 1);
            }

            // 更新统计信息
            ZData& entry = z2sum_count[inside_index];
            entry.valid = true;

            // 跟踪最小d2edge
            if (d2edge < entry.min_d2edge) {
                entry.min_d2edge = d2edge;
                entry.min_node_id = node_id;
            }
        }

        for (int z_key = 0; z_key < static_cast<int>(z2sum_count.size()); ++z_key) {
            const ZData& z_data = z2sum_count[z_key];

            // 跳过无效数据
            if (!z_data.valid) continue;

            //        qDebug()<<"blade_id:"<<z_key<<" ";
            //        qDebug()<<"min_node_id:"<<z_data.min_node_id;
            //        qDebug()<<"avg_dmin:"<<avg_dmin<<"\n";

            if (blade_id >= vol->blade_points.size() || z_key >= vol->blade_points[blade_id].size()) continue;
            double blade_points_bottom_x = vol->blade_points[blade_id][z_key].x;
            double blade_points_bottom_y = vol->blade_points[blade_id][z_key].y;
            double blade_points_bottom_z = vol->blade_points[blade_id][z_key].z;
            if ((z_key + 1) >= vol->blade_points[blade_id].size()) continue;
            double blade_points_up_x = vol->blade_points[blade_id][z_key + 1].x;
            double blade_points_up_y = vol->blade_points[blade_id][z_key + 1].y;
            double blade_points_up_z = vol->blade_points[blade_id][z_key + 1].z;

            GLVertex ideal_blade_points(blade_points_bottom_x, blade_points_bottom_y, blade_points_bottom_z);

            // 获取z_key+1对应的z_data（新增检查）
            if (z_key + 1 >= static_cast<int>(z2sum_count.size()) || !z2sum_count[z_key + 1].valid) continue;  // 若不存在或无效则跳过
            const ZData& up_z_data = z2sum_count[z_key + 1];

            // 优化振动计算：预检查振动数据
            auto vibration_q_iter = vol->vibration_q.find(vol->new_angle);
            bool has_vibration = (vibration_q_iter != vol->vibration_q.end()) && !vol->temp_vibration_vectors.empty();

            if (has_vibration) {
                const auto& vibration_q_data = vibration_q_iter->second;
                int num_modals = vol->temp_vibration_vectors.size();

                // 底部点使用当前z_key的min_node_id
                for (int modal = 0; modal < num_modals; modal++) {
                    double factor = vibration_q_data[modal];
                    blade_points_bottom_x -= vol->temp_vibration_vectors[modal][0][z_data.min_node_id] * factor;
                    //blade_points_bottom_y -= vol->temp_vibration_vectors[modal][1][z_data.min_node_id] * factor;
                    blade_points_bottom_z -= vol->temp_vibration_vectors[modal][2][z_data.min_node_id] * factor;
                }

                // 顶部点使用z_key+1的min_node_id
                for (int modal = 0; modal < num_modals; modal++) {
                    double factor = vibration_q_data[modal];
                    blade_points_up_x -= vol->temp_vibration_vectors[modal][0][up_z_data.min_node_id] * factor;
                    //blade_points_up_y -= vol->temp_vibration_vectors[modal][1][up_z_data.min_node_id] * factor;
                    blade_points_up_z -= vol->temp_vibration_vectors[modal][2][up_z_data.min_node_id] * factor;
                }
            }

            GLVertex real_blade_points(blade_points_bottom_x, blade_points_bottom_y, blade_points_bottom_z);
            GLVertex real_blade_points_up(blade_points_up_x, blade_points_up_y, blade_points_up_z);

            // 计算ideal_blade_points和real_blade_points的欧几里得距离
            double euclidean_distance = (real_blade_points - ideal_blade_points).norm();

            // 确保real_blade_points_map的内部vector有足够的空间
            auto& angle_map = vol->real_blade_points_map[vol->new_angle];
            auto& blade_map = angle_map[blade_id];
            // 确保blade_map的大小足够容纳z_key和z_key+1
            if (blade_map.size() <= static_cast<size_t>(z_key + 1)) {
                blade_map.resize(z_key + 2);
            }

            // 获取引用，减少多次索引访问
            auto& z_key_points = blade_map[z_key];
            auto& z_key_plus_1_points = blade_map[z_key + 1];

            z_key_points.push_back(real_blade_points);
            z_key_plus_1_points.push_back(real_blade_points_up);

            double cuth_distence = 0.0;
            if (blade_id < 1) continue;
            // 计算向量(blade_num_dist_dx, blade_num_dist_dy, blade_num_dist_dz)在向量(vol->dx,vol->dy,vol->dz)上的投影长度
            double blade_num_dist_dx = vol->blade_points[blade_id][z_key].x - vol->blade_points[blade_id - 1][z_key].x;
            double blade_num_dist_dy = vol->blade_points[blade_id][z_key].y - vol->blade_points[blade_id - 1][z_key].y;
            double blade_num_dist_dz = vol->blade_points[blade_id][z_key].z - vol->blade_points[blade_id - 1][z_key].z;

            // 计算向量B(vol->dx,vol->dy,vol->dz)的模长
            double vector_b_length = sqrt(vol->dx * vol->dx + vol->dy * vol->dy + vol->dz * vol->dz);
            // 避免除零错误
            if (vector_b_length < 1e-10) continue;

            // 计算点积: A·B
            double dot_product = blade_num_dist_dx * vol->dx +
                blade_num_dist_dy * vol->dy +
                blade_num_dist_dz * vol->dz;
            // 计算投影长度: (A·B) / |B|
            double projection_length = std::abs(dot_product / vector_b_length);
            double pre_angle = std::round((vol->new_angle - projection_length) * 1000) / 1000.0;
            // 搜索pre_angle正负vol->step范围内的所有角度
            double angle_start = pre_angle - 2 * vol->step;
            double angle_end = pre_angle + 2 * vol->step;

            // 清除real_blade_points_map中第一个key小于vol->new_angle-3*M_PI之前的数据
            double clear_threshold = vol->new_angle - 1.5 * projection_length;
            auto& real_blade_map = const_cast<AptCutterVolume*>(vol)->real_blade_points_map;

            // 找到第一个不小于阈值的元素迭代器
            auto clear_end = real_blade_map.lower_bound(clear_threshold);
            // 清除从begin()到clear_end的所有元素
            if (clear_end != real_blade_map.begin()) {
                real_blade_map.erase(real_blade_map.begin(), clear_end);
            }

            // 当前刀片底部点(P0)和顶部点(P1)构成的线段
            GLVertex P0(blade_points_bottom_x, blade_points_bottom_y, blade_points_bottom_z);
            GLVertex P1(blade_points_up_x, blade_points_up_y, blade_points_up_z);
            double min_distance = std::numeric_limits<double>::max();

            // 优化：使用范围查询快速定位角度范围，避免遍历整个map
            auto& real_blade_points_map_ref = vol->real_blade_points_map;
            auto angle_start_iter = real_blade_points_map_ref.lower_bound(angle_start);
            auto angle_end_iter = real_blade_points_map_ref.upper_bound(angle_end);

            // 遍历角度范围内的所有角度
            for (auto angle_iter = angle_start_iter; angle_iter != angle_end_iter; ++angle_iter) {
                // 检查是否有对应的blade_id
                const auto& blade_map = angle_iter->second;
                auto blade_id_iter = blade_map.find(blade_id - 1);

                if (blade_id_iter != blade_map.end()) {
                    const auto& prev_blade_map = blade_id_iter->second;

                    // 遍历当前角度下所有z_key对应的刀刃点
                    for (const auto& current_z_points : prev_blade_map) {
                        // 提前退出条件：如果已找到足够小的距离
                        if (min_distance <= 1e-6) break;

                        for (const GLVertex& q : current_z_points) {
                            // 计算当前点到线段P0-P1的最短距离
                            double dist = distanceToSegment(q, P0, P1);
                            if (dist < min_distance) {
                                min_distance = dist;
                            }
                        }
                    }
                }
            }

            if (min_distance != std::numeric_limits<double>::max()) {
                cuth_distence = min_distance;
            }
            //printf("%f ",z_key);
            //printf("%f\n",cuth_distence);
            const_cast<AptCutterVolume*>(vol)->add_cut_h_map(
                vol->new_angle,          // 外层键：当前刀具角度
                blade_id,
                z_key,          // 内层键：z坐标值
                euclidean_distance  // 直线度
            );
            const_cast<AptCutterVolume*>(vol)->addcut_h(
                z_key,
                cuth_distence,
                z_data.min_d2edge,  // 新增最小距离
                z_data.min_node_id  // 新增对应节点ID
            );
        }

        free(host_z_array);
        free(host_distence2edge);
        free(host_node_ids);

        // 更新Octnode

        // 预计算振动参数，避免在循环中重复查找
        bool has_vibration_data = !vol->temp_vibration_vectors.empty();
        double current_tool_angle = vol->tool_angle;
        auto vibration_q_iter = vol->vibration_q.find(current_tool_angle);
        bool has_vibration_q = (vibration_q_iter != vol->vibration_q.end());

        auto calculate_color_value = [host_nodes, has_vibration_data, has_vibration_q, vibration_q_iter, vol](size_t i) {
            double q_total = 0.0;
            double u_x = 0.0;
            double u_y = 0.0;
            double u_z = 0.0;
            const int* node_ids = host_nodes[i].node_id;

            for (int j = 0; j < 8; j++) {
                double q_x = 0.0;
                double q_y = 0.0;
                double q_z = 0.0;
                int node_id = node_ids[j];

                if (has_vibration_data && has_vibration_q && node_id >= 0)
                {
                    const auto& vibration_q = vibration_q_iter->second;

                    size_t modal_count = std::min(
                        vol->temp_vibration_vectors.size(),
                        vibration_q.size()
                    );

                    for (size_t modal = 0; modal < modal_count; modal++)
                    {
                        const auto& modal_vectors = vol->temp_vibration_vectors[modal];

                        if (modal_vectors.size() < 3)
                        {
                            continue;
                        }

                        if (node_id >= static_cast<int>(modal_vectors[0].size()) ||
                            node_id >= static_cast<int>(modal_vectors[1].size()) ||
                            node_id >= static_cast<int>(modal_vectors[2].size()))
                        {
                            qDebug() << "node_id out of range:"
                                << "node_id =" << node_id
                                << "modal =" << modal
                                << "size_x =" << modal_vectors[0].size()
                                << "size_y =" << modal_vectors[1].size()
                                << "size_z =" << modal_vectors[2].size();
                            continue;
                        }

                        double modal_factor = vibration_q[modal];

                        q_x += modal_vectors[0][node_id] * modal_factor;
                        q_y += modal_vectors[1][node_id] * modal_factor;
                        q_z += modal_vectors[2][node_id] * modal_factor;
                    }
                }

                q_total += std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z);
                u_x += q_x;
                u_y += q_y;
                u_z += q_z;
            }

            q_total /= 8.0;
            u_x /= 8.0;
            u_y /= 8.0;
            u_z /= 8.0;

            switch (vol->deform_color_var) {
            case 0:
                return q_total;
            case 1:
                return u_x;
            case 2:
                return u_y;
            case 3:
                return u_z;
            default:
                return q_total;
            }
        };

        double dynamic_color_min = std::numeric_limits<double>::max();
        double dynamic_color_max = std::numeric_limits<double>::lowest();

        for (size_t i = 0; i < node_count; i++) {
            double color_value = calculate_color_value(i);
            dynamic_color_min = std::min(dynamic_color_min, color_value);
            dynamic_color_max = std::max(dynamic_color_max, color_value);
        }

        if (dynamic_color_min == std::numeric_limits<double>::max() ||
            dynamic_color_max == std::numeric_limits<double>::lowest() ||
            dynamic_color_min == dynamic_color_max) {
            dynamic_color_min = vol->deform_color_min;
            dynamic_color_max = vol->deform_color_max;
        }
        else {
            vol->deform_color_min = dynamic_color_min;
            vol->deform_color_max = dynamic_color_max;
        }

        // 优化：使用QtConcurrent并行处理节点更新
        // 捕获this指针以访问类的成员变量和成员函数
        auto update_node = [this, host_nodes, has_vibration_data, has_vibration_q, vibration_q_iter, vol, dynamic_color_min, dynamic_color_max](size_t i) {
            Octnode* node = nodes_to_process[i];
            bool updated = false;
            double q_total = 0.0;
            double u_x = 0.0;
            double u_y = 0.0;
            double u_z = 0.0;
            // 快速更新距离值
            double* node_f = node->f;
            const float* host_f = host_nodes[i].f;
            int* node_ids = host_nodes[i].node_id;

            for (int j = 0; j < 8; j++) {
                double new_f = static_cast<double>(-host_f[j]);
                if (new_f < node_f[j]) {
                    node_f[j] = new_f;
                    updated = true;
                }
                double q_x = 0.0, q_y = 0.0, q_z = 0.0;
                int node_id = node_ids[j];

                if (has_vibration_data && has_vibration_q && node_id >= 0)
                {
                    const auto& vibration_q = vibration_q_iter->second;

                    size_t modal_count = std::min(
                        vol->temp_vibration_vectors.size(),
                        vibration_q.size()
                    );

                    for (size_t modal = 0; modal < modal_count; modal++)
                    {
                        const auto& modal_vectors = vol->temp_vibration_vectors[modal];

                        if (modal_vectors.size() < 3)
                        {
                            continue;
                        }

                        if (node_id >= static_cast<int>(modal_vectors[0].size()) ||
                            node_id >= static_cast<int>(modal_vectors[1].size()) ||
                            node_id >= static_cast<int>(modal_vectors[2].size()))
                        {
                            qDebug() << "node_id 越界:"
                                << "node_id =" << node_id
                                << "modal =" << modal
                                << "size_x =" << modal_vectors[0].size()
                                << "size_y =" << modal_vectors[1].size()
                                << "size_z =" << modal_vectors[2].size();
                            continue;
                        }

                        double modal_factor = vibration_q[modal];

                        q_x += modal_vectors[0][node_id] * modal_factor;
                        q_y += modal_vectors[1][node_id] * modal_factor;
                        q_z += modal_vectors[2][node_id] * modal_factor;
                    }
                }

                q_total += std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z);
                u_x += q_x;
                u_y += q_y;
                u_z += q_z;
            }
            q_total /= 8.0;
            u_x = u_x / 8.0;
            u_y = u_y / 8.0;
            u_z = u_z / 8.0;
            float r, g, b;
            switch (vol->deform_color_var) {
            case 0:
                getDeformationColor(q_total, dynamic_color_min, dynamic_color_max, r, g, b);
                break;
            case 1:
                getDeformationColor(u_x, dynamic_color_min, dynamic_color_max, r, g, b);
                break;
            case 2:
                getDeformationColor(u_y, dynamic_color_min, dynamic_color_max, r, g, b);
                break;
            case 3:
                getDeformationColor(u_z, dynamic_color_min, dynamic_color_max, r, g, b);
                break;
            default:
                getDeformationColor(q_total, dynamic_color_min, dynamic_color_max, r, g, b);
            }
            node->color = { r, g, b };
            // 更新节点状态
            node->set_state();
        };

        // 创建正确的索引向量：包含0到node_count-1的索引
        QVector<size_t> indices(node_count);
        for (size_t i = 0; i < node_count; i++) {
            indices[i] = i;
        }

        // 使用QtConcurrent并行执行节点更新
        QtConcurrent::blockingMap(indices, update_node);
        clean_outside_nodes(current, vol);  // 清理当前节点及其子节点中的无效OUTSIDE节点

        delete[] host_nodes;
        delete[] blade.blade_points;
        delete[] blade.plane_normals;
        delete[] blade.plane_points;


        stop = std::chrono::system_clock::now();
        qDebug() << "结果处理：():" << std::chrono::duration<double>(stop - start).count() << "sec.";


    }



} // end namespace cutsim
