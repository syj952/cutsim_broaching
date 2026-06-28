
// 文件名: src/cutsim/cutsim/cuda_functions.cpp
#include "cuda_functions.hpp"
#include "octree.hpp"
#include "octnode.hpp"
#include "volume.hpp"
#include <cstdio>
#include <cuda_runtime.h>
#include <vector>
#include <stack>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <unordered_map>
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

// 定义 CutterSegment 结构体
struct CutterSegment {
    int type;  // 改为int类型，确保大小一致
    double radius1, radius2, length;
    GLVertex_xyz center_xyz, angle_xyz;
    double z_start, z_end;

    // 添加默认构造函数
    CutterSegment() : type(0), radius1(0.0), radius2(0.0), length(0.0), center_xyz(), z_start(0.0), z_end(0.0) {}

    // 添加带参数的构造函数
    CutterSegment(int t, double r1, double r2, double len, double zs, double ze)
        : type(t), radius1(r1), radius2(r2), length(len), center_xyz(), z_start(zs), z_end(ze) {
    }
};

struct BladePoint {
    GLVertex_xyz v1, v2, v3, v4;
};

struct broaching_BladeParams {
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

struct milling_BladeParams {
    float center_x, center_y, center_z;
    float dx, dy, dz;
    float cube_resolution;
    double* point_r_blade;  // 设备指针
    int point_r_count;      // 点的数量
    GLVertex_xyz* blade_points;  // 改为GLVertex数组指针
    int blade_points_count;  // 点的数量
    int device_id;
    // 这里可以根据需要添加更多参数
};


// STL 体积参数结构体
struct StlParams {
    // STL 三角形面数据
    GLVertex_xyz* facets_v1;
    GLVertex_xyz* facets_v2;
    GLVertex_xyz* facets_v3;
    GLVertex_xyz* facets_normal;
    // 预计算的边向量
    GLVertex_xyz* V21;
    GLVertex_xyz* V21invV21dotV21;
    GLVertex_xyz* V32;
    GLVertex_xyz* V32invV32dotV32;
    GLVertex_xyz* V13;
    GLVertex_xyz* V13invV13dotV13;
    int facet_count;
    // 邻居索引（为了简化，我们使用所有面而不是邻居索引）
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
    float inv_cube_size;
    float r, g, b;
};


extern "C" void broaching_cuda_diff_volume_blade(CudaNodeData * host_nodes, int numNodes, broaching_BladeParams host_blade,
    float** host_z_array, float** host_distence2edge, int** host_node_ids, int* host_record_count);
extern "C" void milling_cuda_diff_volume_blade(CudaNodeData * host_nodes, int numNodes, milling_BladeParams host_blade,
    float** host_z_array, float** host_distence2edge, int** host_node_ids, int* host_record_count);
extern "C" void digitaltwin_milling_cuda_diff_volume_blade(
    CudaNodeData * host_nodes,
    int numNodes,
    CutterSegment * segments,           // 改为指针
    int numSegments                   // 添加数量参数
);
extern "C" void cuda_diff_volume(CudaNodeData * host_nodes, int numNodes, VolumeParams host_volume);
extern "C" void cuda_sum_stl(CudaNodeData * host_nodes, int numNodes, StlParams host_stl);


#pragma pack(pop)


namespace cutsim {

    static double selected_deformation_value(int color_var, double q_total, double u_x, double u_y, double u_z)
    {
        switch (color_var) {
        case 1:
            return u_x;
        case 2:
            return u_y;
        case 3:
            return u_z;
        default:
            return q_total;
        }
    }

    template <typename VolumeT>
    static void update_dynamic_deform_color_range(VolumeT* vol, const std::vector<double>& values, const std::vector<char>& active)
    {
        if (!vol) {
            return;
        }

        double min_val = std::numeric_limits<double>::max();
        double max_val = -std::numeric_limits<double>::max();
        bool found = false;

        for (size_t i = 0; i < values.size(); ++i) {
            if (!active[i] || !std::isfinite(values[i])) {
                continue;
            }

            min_val = std::min(min_val, values[i]);
            max_val = std::max(max_val, values[i]);
            found = true;
        }

        if (!found) {
            return;
        }

        if (vol->deform_color_var == 0) {
            min_val = std::min(0.0, min_val);
        }

        if (max_val <= min_val && vol->deform_color_var == 0 && max_val == 0.0) {
            min_val = 0.0;
            max_val = 1e-9;
        }
        else if (max_val <= min_val) {
            const double scale = std::max(1.0, std::max(std::abs(min_val), std::abs(max_val)));
            const double padding = scale * 1e-6;
            min_val -= padding;
            max_val += padding;
        }

        vol->deform_color_min = min_val;
        vol->deform_color_max = max_val;
    }

    static void update_parent_states_from_leaves(const std::vector<Octnode*>& leaf_nodes) {
        std::vector<Octnode*> parents;
        std::unordered_set<Octnode*> seen;

        for (Octnode* leaf : leaf_nodes) {
            for (Octnode* node = leaf ? leaf->parent : nullptr; node != nullptr; node = node->parent) {
                if (seen.insert(node).second) {
                    parents.push_back(node);
                }
            }
        }

        std::sort(parents.begin(), parents.end(), [](const Octnode* a, const Octnode* b) {
            return a->depth > b->depth;
            });

        for (Octnode* node : parents) {
            if (node->childcount != 8) {
                continue;
            }

            Octnode::NodeState new_state = Octnode::UNDECIDED;
            if (node->all_child_state(Octnode::INSIDE)) {
                new_state = Octnode::INSIDE;
            }
            else if (node->all_child_state(Octnode::OUTSIDE)) {
                new_state = Octnode::OUTSIDE;
            }

            if (node->state != new_state) {
                if (new_state == Octnode::UNDECIDED && node->state != Octnode::UNDECIDED) {
                    node->prev_state = node->state;
                }
                node->state = new_state;
                node->setInvalid();
            }

            if (new_state != Octnode::UNDECIDED) {
                node->delete_children();
            }
        }
    }


    // 构造函数，初始化CUDA状态
    static int shared_vertex_lookup_id(const GLVertex* vertex, int normalvertices_size)
    {
        if (!vertex || vertex->id == -9999999) {
            return -1;
        }

        return vertex->id <= 0 ? -vertex->id : vertex->id + normalvertices_size - 1;
    }

    size_t cuda_functions::check_leaf_shared_vertex_f_consistency(
        const std::vector<Octnode*>& leaf_nodes,
        int normalvertices_size,
        double tolerance,
        size_t max_report_count) const
    {
        struct VertexFRange {
            double min_f = std::numeric_limits<double>::max();
            double max_f = -std::numeric_limits<double>::max();
            const Octnode* min_node = nullptr;
            const Octnode* max_node = nullptr;
            int min_corner = -1;
            int max_corner = -1;
            GLVertex min_position;
            GLVertex max_position;
            size_t count = 0;
        };

        std::unordered_map<int, VertexFRange> f_by_vertex_id;
        f_by_vertex_id.reserve(leaf_nodes.size() * 8);

        size_t leaf_count = 0;
        size_t corner_count = 0;
        for (Octnode* node : leaf_nodes) {
            if (!node || !node->isLeaf()) {
                continue;
            }

            ++leaf_count;
            for (int j = 0; j < 8; ++j) {
                const int vertex_id = shared_vertex_lookup_id(node->vertex[j], normalvertices_size);
                if (vertex_id < 0) {
                    continue;
                }

                const double f = node->f[j];
                VertexFRange& range = f_by_vertex_id[vertex_id];
                if (f < range.min_f) {
                    range.min_f = f;
                    range.min_node = node;
                    range.min_corner = j;
                    range.min_position = *node->vertex[j];
                }
                if (f > range.max_f) {
                    range.max_f = f;
                    range.max_node = node;
                    range.max_corner = j;
                    range.max_position = *node->vertex[j];
                }
                ++range.count;
                ++corner_count;
            }
        }

        size_t inconsistent_count = 0;
        size_t reported_count = 0;
        for (const auto& pair : f_by_vertex_id) {
            const int vertex_id = pair.first;
            const VertexFRange& range = pair.second;
            if (range.count < 2) {
                continue;
            }

            const double diff = range.max_f - range.min_f;
            if (diff <= tolerance) {
                continue;
            }

            ++inconsistent_count;
            if (reported_count < max_report_count) {
                qDebug() << "[shared-f-check] mismatch"
                    << "vertex_id" << vertex_id
                    << "count" << static_cast<qulonglong>(range.count)
                    << "diff" << diff
                    << "min_f" << range.min_f
                    << "max_f" << range.max_f
                    << "min_node" << static_cast<const void*>(range.min_node)
                    << "min_depth" << (range.min_node ? range.min_node->depth : 0)
                    << "min_corner" << range.min_corner
                    << "min_xyz" << range.min_position.x << range.min_position.y << range.min_position.z
                    << "max_node" << static_cast<const void*>(range.max_node)
                    << "max_depth" << (range.max_node ? range.max_node->depth : 0)
                    << "max_corner" << range.max_corner
                    << "max_xyz" << range.max_position.x << range.max_position.y << range.max_position.z;
                ++reported_count;
            }
        }

        qDebug() << "[shared-f-check] leaf_count" << static_cast<qulonglong>(leaf_count)
            << "corner_count" << static_cast<qulonglong>(corner_count)
            << "shared_vertex_count" << static_cast<qulonglong>(f_by_vertex_id.size())
            << "inconsistent_vertex_count" << static_cast<qulonglong>(inconsistent_count)
            << "tolerance" << tolerance;

        return inconsistent_count;
    }

    static constexpr size_t STL_SUM_NODE_BATCH_SIZE = 65536;

    static void fill_cuda_node_data(CudaNodeData& dst, const Octnode* node) {
        for (int j = 0; j < 8; ++j) {
            dst.x[j] = static_cast<float>(node->vertex[j]->x);
            dst.y[j] = static_cast<float>(node->vertex[j]->y);
            dst.z[j] = static_cast<float>(node->vertex[j]->z);
            dst.f[j] = static_cast<float>(node->f[j]);
            dst.node_id[j] = 0;
        }
    }

    static void apply_cuda_stl_node_data(Octnode* node, const CudaNodeData& src, const StlVolume* vol) {
        bool updated = false;
        for (int j = 0; j < 8; ++j) {
            const double new_f = static_cast<double>(src.f[j]);
            if (new_f > node->f[j]) {
                node->f[j] = new_f;
                updated = true;
            }
        }

        if (updated) {
            node->color = vol->color;
            node->setInvalid();
        }
    }

    static Octnode::NodeState complete_stl_node_state(const Octnode* node) {
        bool inside = true;
        bool outside = true;
        const double limit = node->scale * 4.0;

        for (int j = 0; j < 8; ++j) {
            if (node->f[j] <= limit) {
                inside = false;
            }
            if (-limit <= node->f[j]) {
                outside = false;
            }
        }

        if (inside) {
            return Octnode::INSIDE;
        }
        if (outside) {
            return Octnode::OUTSIDE;
        }
        return Octnode::UNDECIDED;
    }

    static void force_node_state(Octnode* node, Octnode::NodeState state) {
        if (state == Octnode::INSIDE) {
            if (!node->is_inside()) {
                node->force_setInside();
                node->setInvalid();
            }
        }
        else if (state == Octnode::OUTSIDE) {
            if (!node->is_outside()) {
                node->force_setOutside();
                node->setInvalid();
            }
        }
        else if (!node->is_undecided()) {
            node->force_setUndecided();
            node->setInvalid();
        }
    }

    static void process_stl_node_batch(
        const std::vector<Octnode*>& batch,
        std::vector<CudaNodeData>& host_nodes,
        const StlVolume* vol,
        StlParams& stl_params) {
        if (batch.empty()) {
            return;
        }

        host_nodes.resize(batch.size());
        for (size_t i = 0; i < batch.size(); ++i) {
            fill_cuda_node_data(host_nodes[i], batch[i]);
        }

        cuda_sum_stl(host_nodes.data(), static_cast<int>(host_nodes.size()), stl_params);

        for (size_t i = 0; i < batch.size(); ++i) {
            apply_cuda_stl_node_data(batch[i], host_nodes[i], vol);
        }
    }

    static Octnode::NodeState compress_stl_subtree_states(Octnode* node) {
        if (node == nullptr || node->childcount != 8) {
            return node ? node->state : Octnode::UNDECIDED;
        }

        for (int i = 0; i < 8; ++i) {
            compress_stl_subtree_states(node->child[i]);
        }

        if (node->all_child_state(Octnode::INSIDE)) {
            force_node_state(node, Octnode::INSIDE);
            node->delete_children();
        }
        else if (node->all_child_state(Octnode::OUTSIDE)) {
            force_node_state(node, Octnode::OUTSIDE);
            node->delete_children();
        }
        else {
            force_node_state(node, Octnode::UNDECIDED);
        }

        return node->state;
    }

    static Octnode::NodeState update_stl_parent_states_without_pruning(Octnode* node) {
        if (node == nullptr || node->childcount != 8) {
            return node ? node->state : Octnode::UNDECIDED;
        }

        for (int i = 0; i < 8; ++i) {
            update_stl_parent_states_without_pruning(node->child[i]);
        }

        if (node->all_child_state(Octnode::INSIDE)) {
            force_node_state(node, Octnode::INSIDE);
        }
        else if (node->all_child_state(Octnode::OUTSIDE)) {
            force_node_state(node, Octnode::OUTSIDE);
        }
        else {
            force_node_state(node, Octnode::UNDECIDED);
        }

        return node->state;
    }

    static size_t sum_stl_octree_batched(Octnode* root, const StlVolume* vol, unsigned int max_depth, StlParams& stl_params) {
        if (root == nullptr || max_depth == 0) {
            return 0;
        }

        const unsigned int target_depth = max_depth - 1;
        size_t processed_count = 0;

        std::vector<Octnode*> frontier;
        std::vector<Octnode*> next_frontier;
        std::vector<Octnode*> batch;
        std::vector<CudaNodeData> host_nodes;

        frontier.reserve(1024);
        next_frontier.reserve(1024);
        batch.reserve(STL_SUM_NODE_BATCH_SIZE);
        host_nodes.reserve(STL_SUM_NODE_BATCH_SIZE);
        frontier.push_back(root);

        while (!frontier.empty()) {
            next_frontier.clear();
            batch.clear();

            for (Octnode* node : frontier) {
                if (node == nullptr || node->depth > target_depth || node->is_inside() || !vol->bb.overlaps(node->bb)) {
                    continue;
                }

                if (node->depth == target_depth) {
                    if (node->is_undecided() && !node->color.compareColor(vol->color)) {
                        continue;
                    }
                    batch.push_back(node);
                    if (batch.size() == STL_SUM_NODE_BATCH_SIZE) {
                        process_stl_node_batch(batch, host_nodes, vol, stl_params);
                        for (Octnode* processed_node : batch) {
                            processed_node->set_state();
                        }
                        processed_count += batch.size();
                        batch.clear();
                    }
                    continue;
                }

                if (node->childcount == 8) {
                    for (int i = 0; i < 8; ++i) {
                        if (!node->child[i]->is_inside()) {
                            next_frontier.push_back(node->child[i]);
                        }
                    }
                    continue;
                }

                if (!node->is_undecided()) {
                    force_node_state(node, Octnode::UNDECIDED);
                }
                node->subdivide();
                for (int i = 0; i < 8; ++i) {
                    next_frontier.push_back(node->child[i]);
                }
            }

            if (!batch.empty()) {
                process_stl_node_batch(batch, host_nodes, vol, stl_params);
                for (Octnode* processed_node : batch) {
                    processed_node->set_state();
                }
                processed_count += batch.size();
            }

            frontier.swap(next_frontier);
        }

        update_stl_parent_states_without_pruning(root);
        return processed_count;
    }

    struct BroachingAabb {
        double min_x;
        double min_y;
        double min_z;
        double max_x;
        double max_y;
        double max_z;
    };

    struct BroachingLeafSearchContext {
        const broaching_AptCutterVolume* vol;
        BroachingAabb aabb;
    };

    static thread_local const BroachingLeafSearchContext* active_broaching_leaf_search = nullptr;

    static inline void expand_broaching_aabb(BroachingAabb& aabb, const GLVertex& p) {
        const double x = static_cast<double>(p.x);
        const double y = static_cast<double>(p.y);
        const double z = static_cast<double>(p.z);
        if (x < aabb.min_x) aabb.min_x = x;
        if (y < aabb.min_y) aabb.min_y = y;
        if (z < aabb.min_z) aabb.min_z = z;
        if (x > aabb.max_x) aabb.max_x = x;
        if (y > aabb.max_y) aabb.max_y = y;
        if (z > aabb.max_z) aabb.max_z = z;
    }

    static BroachingAabb make_broaching_aabb(const broaching_AptCutterVolume* vol) {
        const auto& bb = vol->bb_points;
        const GLVertex& p0 = std::get<0>(bb);
        BroachingAabb aabb{ p0.x, p0.y, p0.z, p0.x, p0.y, p0.z };
        expand_broaching_aabb(aabb, std::get<1>(bb));
        expand_broaching_aabb(aabb, std::get<2>(bb));
        expand_broaching_aabb(aabb, std::get<3>(bb));
        expand_broaching_aabb(aabb, std::get<4>(bb));
        expand_broaching_aabb(aabb, std::get<5>(bb));
        expand_broaching_aabb(aabb, std::get<6>(bb));
        expand_broaching_aabb(aabb, std::get<7>(bb));

        const double padding = std::max(4.0 * vol->cube_resolution_1, 1e-6);
        aabb.min_x -= padding;
        aabb.min_y -= padding;
        aabb.min_z -= padding;
        aabb.max_x += padding;
        aabb.max_y += padding;
        aabb.max_z += padding;

        return aabb;
    }

    class BroachingLeafSearchScope {
    public:
        explicit BroachingLeafSearchScope(const broaching_AptCutterVolume* vol)
            : previous_(active_broaching_leaf_search),
            owns_context_(previous_ == nullptr || previous_->vol != vol) {
            if (owns_context_) {
                context_.vol = vol;
                context_.aabb = make_broaching_aabb(vol);
                active_broaching_leaf_search = &context_;
            }
        }

        ~BroachingLeafSearchScope() {
            if (owns_context_) {
                active_broaching_leaf_search = previous_;
            }
        }

    private:
        BroachingLeafSearchContext context_{};
        const BroachingLeafSearchContext* previous_;
        bool owns_context_;
    };

    struct DigitalTwinLeafSearchContext {
        const digitaltwin_AptCutterVolume* vol;
        double min_x;
        double min_y;
        double min_z;
        double max_x;
        double max_y;
        double max_z;
    };

    static thread_local const DigitalTwinLeafSearchContext* active_digitaltwin_leaf_search = nullptr;

    class DigitalTwinLeafSearchScope {
    public:
        explicit DigitalTwinLeafSearchScope(const digitaltwin_AptCutterVolume* vol)
            : previous_(active_digitaltwin_leaf_search),
            owns_context_(previous_ == nullptr || previous_->vol != vol) {
            if (owns_context_) {
                context_.vol = vol;
                context_.min_x = static_cast<double>(vol->bb.minpt.x);
                context_.min_y = static_cast<double>(vol->bb.minpt.y);
                context_.min_z = static_cast<double>(vol->bb.minpt.z);
                context_.max_x = static_cast<double>(vol->bb.maxpt.x);
                context_.max_y = static_cast<double>(vol->bb.maxpt.y);
                context_.max_z = static_cast<double>(vol->bb.maxpt.z);
                active_digitaltwin_leaf_search = &context_;
            }
        }

        ~DigitalTwinLeafSearchScope() {
            if (owns_context_) {
                active_digitaltwin_leaf_search = previous_;
            }
        }

    private:
        DigitalTwinLeafSearchContext context_{};
        const DigitalTwinLeafSearchContext* previous_;
        bool owns_context_;
    };

    static inline bool digitaltwin_node_overlaps_volume(const Octnode* node, const DigitalTwinLeafSearchContext& context) {
#ifdef MULTI_AXIS
        return context.vol->bb.overlaps(node->bb);
#else
        return !(context.max_x < node->bb.minpt.x || context.min_x > node->bb.maxpt.x ||
            context.max_y < node->bb.minpt.y || context.min_y > node->bb.maxpt.y ||
            context.max_z < node->bb.minpt.z || context.min_z > node->bb.maxpt.z);
#endif
    }

    static inline int vibration_node_id(const GLVertex* vertex, int normalvertices_size) {
        return vertex->id <= 0 ? -vertex->id : vertex->id + normalvertices_size - 1;
    }

    static inline double vibration_value_or_zero(const std::vector<double>& values, int node_id) {
        if (node_id < 0 || node_id >= static_cast<int>(values.size())) {
            return 0.0;
        }
        return values[node_id];
    }

    static inline double interpolate_corner_values(const double c[8], double tx, double ty, double tz) {
        const double omt_x = 1.0 - tx;
        const double omt_y = 1.0 - ty;
        const double omt_z = 1.0 - tz;
        return c[0] * omt_x * omt_y * omt_z +
            c[1] * tx * omt_y * omt_z +
            c[2] * omt_x * ty * omt_z +
            c[3] * tx * ty * omt_z +
            c[4] * omt_x * omt_y * tz +
            c[5] * tx * omt_y * tz +
            c[6] * omt_x * ty * tz +
            c[7] * tx * ty * tz;
    }

    static void append_digitaltwin_child_vibration(Octnode* current, digitaltwin_AptCutterVolume* vol, bool map_to_unit_interval) {
        static const int parent_corner_order[8] = { 3, 2, 1, 0, 7, 6, 5, 4 };
        int parent_node_ids[8];
        for (int i = 0; i < 8; ++i) {
            parent_node_ids[i] = vibration_node_id(current->vertex[parent_corner_order[i]], vol->normalvertices_size);
        }

        double interp_by_vertex[8];
        const GLVertex* directions = Octnode::getDirection();
        const size_t modal_count = vol->temp_vibration_vectors.size();

        for (size_t modal = 0; modal < modal_count; ++modal) {
            for (int axis = 0; axis < 3; ++axis) {
                auto& values = vol->temp_vibration_vectors[modal][axis];
                double c[8];
                for (int i = 0; i < 8; ++i) {
                    c[i] = vibration_value_or_zero(values, parent_node_ids[i]);
                }

                for (int k = 0; k < 8; ++k) {
                    double tx = directions[k].x;
                    double ty = directions[k].y;
                    double tz = directions[k].z;
                    if (map_to_unit_interval) {
                        tx = (tx + 1.0) * 0.5;
                        ty = (ty + 1.0) * 0.5;
                        tz = (tz + 1.0) * 0.5;
                    }
                    else {
                        tx *= 0.5;
                        ty *= 0.5;
                        tz *= 0.5;
                    }
                    interp_by_vertex[k] = interpolate_corner_values(c, tx, ty, tz);
                }

                values.reserve(values.size() + 64);
                for (int child_index = 0; child_index < 8; ++child_index) {
                    Octnode* child = current->child[child_index];
                    for (int k = 0; k < 8; ++k) {
                        values.push_back(interp_by_vertex[k]);
                        child->vertex[k]->id = -static_cast<int>(values.size() - 1);
                    }
                }
            }
        }
    }

    cuda_functions::cuda_functions() : deviceCount(0) {
        cudaError_t cudaStatus = cudaGetDeviceCount(&deviceCount);
        if (cudaStatus != cudaSuccess || deviceCount == 0) {
            qDebug() << "没有找到CUDA设备或CUDA初始化失败";
        }
        else {
            //qDebug() << "找到" << deviceCount << "个CUDA设备";
            cudaSetDevice(0);
            // 获取设备属性
            cudaDeviceProp deviceProp;
            cudaGetDeviceProperties(&deviceProp, 0);
            //qDebug() << "使用设备:" << deviceProp.name;
            //qDebug() << "计算能力:" << deviceProp.major << "." << deviceProp.minor;
        }
    }

    void cuda_functions::broaching_clean_outside_nodes(Octnode* current, broaching_AptCutterVolume* vol) {
        if (!current) {
            return; // 如果节点为空，直接返回
        }

        // 获取包围盒的8个顶点
        BroachingLeafSearchScope leaf_search_scope(vol);
        const BroachingAabb& broaching_aabb = active_broaching_leaf_search->aabb;
        float3 min_coord = {
            static_cast<float>(broaching_aabb.min_x),
            static_cast<float>(broaching_aabb.min_y),
            static_cast<float>(broaching_aabb.min_z)
        };
        float3 max_coord = {
            static_cast<float>(broaching_aabb.max_x),
            static_cast<float>(broaching_aabb.max_y),
            static_cast<float>(broaching_aabb.max_z)
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
                    broaching_clean_outside_nodes(current->child[n], vol);
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

    void cuda_functions::milling_clean_outside_nodes(Octnode* current, milling_AptCutterVolume* vol) {
        if (!current) {
            return; // 如果节点为空，直接返回
        }

        GLVertex* v = current->center;
        float3 p{ v->x, v->y, v->z };

        bool inside = false;
        for (const auto& bbox : vol->blade_bboxes) {
            // 直接使用预计算的包围盒
            float3 quad_min = bbox.min;
            float3 quad_max = bbox.max;

            if (p.x >= quad_min.x - current->scale && p.x <= quad_max.x + current->scale &&
                p.y >= quad_min.y - current->scale && p.y <= quad_max.y + current->scale &&
                p.z >= quad_min.z - current->scale && p.z <= quad_max.z + current->scale)
            {
                inside = true;
                break;
            }
        }

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
                    milling_clean_outside_nodes(current->child[n], vol);
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

    void cuda_functions::digitaltwin_milling_clean_outside_nodes(Octnode* current, digitaltwin_AptCutterVolume* vol) {
        if (!current) {
            return; // 如果节点为空，直接返回
        }

        if (!vol->bb.overlaps(current->bb)) {
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
                    digitaltwin_milling_clean_outside_nodes(current->child[n], vol);
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

    void cuda_functions::broaching_get_leaf_nodes_diff_blade(Octnode* current, std::vector<Octnode*>& nodes_to_process, broaching_AptCutterVolume* vol, unsigned int max_depth, std::chrono::duration<double>& elapsed, int blade_id) {
        // 如果节点已经在外部或没有与体积重叠，则直接返回
        if (current->depth > (vol->max_depth_1 - 1) || current->is_outside()) {
            return;
        }

        // 获取包围盒的8个顶点
        BroachingLeafSearchScope leaf_search_scope(vol);
        const BroachingAabb& broaching_aabb = active_broaching_leaf_search->aabb;
        float3 min_coord = {
            static_cast<float>(broaching_aabb.min_x),
            static_cast<float>(broaching_aabb.min_y),
            static_cast<float>(broaching_aabb.min_z)
        };
        float3 max_coord = {
            static_cast<float>(broaching_aabb.max_x),
            static_cast<float>(broaching_aabb.max_y),
            static_cast<float>(broaching_aabb.max_z)
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
                if (current->depth > MFEM_DEPTH - 1) {
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
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                  //  fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                  //      node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c000 = 0.0;
                                }
                                else {
                                    c000 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }

                                if (current->vertex[2]->id <= 0)
                                    node_id = -current->vertex[2]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[2]->id + vol->normalvertices_size - 1;
                                double c100;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                   // fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                   //     node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c100 = 0.0;
                                }
                                else {
                                    c100 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }

                                if (current->vertex[1]->id <= 0)
                                    node_id = -current->vertex[1]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[1]->id + vol->normalvertices_size - 1;
                                double c010;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                    //fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                     //   node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c010 = 0.0;
                                }
                                else {
                                    c010 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }

                                if (current->vertex[0]->id <= 0)
                                    node_id = -current->vertex[0]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[0]->id + vol->normalvertices_size - 1;
                                double c110;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                    //fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                     //   node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c110 = 0.0;
                                }
                                else {
                                    c110 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }

                                if (current->vertex[7]->id <= 0)
                                    node_id = -current->vertex[7]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[7]->id + vol->normalvertices_size - 1;
                                double c001;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                    //fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                    //    node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c001 = 0.0;
                                }
                                else {
                                    c001 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }

                                if (current->vertex[6]->id <= 0)
                                    node_id = -current->vertex[6]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[6]->id + vol->normalvertices_size - 1;
                                double c101;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                   // fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                   //     node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c101 = 0.0;
                                }
                                else {
                                    c101 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }

                                if (current->vertex[5]->id <= 0)
                                    node_id = -current->vertex[5]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[5]->id + vol->normalvertices_size - 1;
                                double c011;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                  //  fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                  //      node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c011 = 0.0;
                                }
                                else {
                                    c011 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }

                                if (current->vertex[4]->id <= 0)
                                    node_id = -current->vertex[4]->id; // 正常节点编号
                                else
                                    node_id = current->vertex[4]->id + vol->normalvertices_size - 1;
                                double c111;
                                if (node_id < 0 || node_id >= static_cast<int>(vol->temp_vibration_vectors[modal][dir].size())) {
                                  //  fprintf(stderr, "Error: node_id %d out of bounds for temp_vibration_vectors[%d][%d] with size %zu\n",
                                  //      node_id, modal, dir, vol->temp_vibration_vectors[modal][dir].size());
                                    c111 = 0.0;
                                }
                                else {
                                    c111 = vol->temp_vibration_vectors[modal][dir][node_id];
                                }
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
                    broaching_get_leaf_nodes_diff_blade(current->child[n], nodes_to_process, vol, max_depth, elapsed, blade_id);
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
                    broaching_get_leaf_nodes_diff_blade(current->child[m], nodes_to_process, vol, max_depth, elapsed, blade_id);
                }
            }
            else
                // 如果已达到最大深度，则加入处理列表
            {
                nodes_to_process.push_back(current);
            }

        }
    }

    void cuda_functions::milling_get_leaf_nodes_diff_blade(Octnode* current, std::vector<Octnode*>& nodes_to_process, milling_AptCutterVolume* vol, unsigned int max_depth, std::chrono::duration<double>& elapsed) {
        // 如果节点已经在外部或没有与体积重叠，则直接返回
        if (current->depth > (vol->max_depth_1 - 1) || current->is_outside()) {
            return;
        }

        //    auto start1 = std::chrono::system_clock::now();

        GLVertex* v = current->center;
        float3 p{ v->x, v->y, v->z };

        bool inside = false;
        for (const auto& bbox : vol->blade_bboxes) {
            // 直接使用预计算的包围盒
            float3 quad_min = bbox.min;
            float3 quad_max = bbox.max;

            if (p.x >= quad_min.x - current->scale && p.x <= quad_max.x + current->scale &&
                p.y >= quad_min.y - current->scale && p.y <= quad_max.y + current->scale &&
                p.z >= quad_min.z - current->scale && p.z <= quad_max.z + current->scale)
            {
                inside = true;
                break;
            }
        }

        if (!inside) {
            return;
        }

        //    auto end1 = std::chrono::system_clock::now();
        //   elapsed += (end1 - start1);
        // 首先判断是否有子节点
        if (current->childcount == 8 && current->depth < (vol->max_depth_1 - 1)) {
            // 检查子节点是否是outside
            for (int n = 0; n < 8; ++n) {
                if (current->depth > MFEM_DEPTH - 1) {
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
                    milling_get_leaf_nodes_diff_blade(current->child[n], nodes_to_process, vol, max_depth, elapsed);
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
                    milling_get_leaf_nodes_diff_blade(current->child[m], nodes_to_process, vol, max_depth, elapsed);
                }
            }
            else
                // 如果已达到最大深度，则加入处理列表
            {
                nodes_to_process.push_back(current);
            }

        }
    }

    void cuda_functions::digitaltwin_get_leaf_nodes_diff(Octnode* current, std::vector<Octnode*>& nodes_to_process, digitaltwin_AptCutterVolume* vol, unsigned int max_depth) {
        //qDebug() << "vol->type():"<<vol->type;
        // 如果节点已经在外部或没有与体积重叠，则直接返回
        DigitalTwinLeafSearchScope leaf_search_scope(vol);
        const DigitalTwinLeafSearchContext& leaf_search_context = *active_digitaltwin_leaf_search;
        const unsigned int target_depth = static_cast<unsigned int>(vol->max_depth_1 - 1);
        if (current->depth > target_depth || current->is_outside() || !digitaltwin_node_overlaps_volume(current, leaf_search_context)) {
            return;
        }

        if (current->childcount == 8 && current->depth < target_depth) {
            if (current->depth > MFEM_DEPTH - 1) {
                append_digitaltwin_child_vibration(current, vol, true);
            }
            for (int n = 0; n < 8; ++n) {
                if (!current->child[n]->is_outside()) {
                    digitaltwin_get_leaf_nodes_diff(current->child[n], nodes_to_process, vol, max_depth);
                }
            }
            return;
        }

        if (current->depth < target_depth) {
            if (!current->is_undecided()) {
                current->force_setUndecided();
            }
            current->subdivide();
            append_digitaltwin_child_vibration(current, vol, false);
            for (int m = 0; m < 8; ++m) {
                digitaltwin_get_leaf_nodes_diff(current->child[m], nodes_to_process, vol, max_depth);
            }
            return;
        }

        nodes_to_process.push_back(current);
        return;

        //qDebug() << "get_leaf_nodes_diff():";
        // 首先判断是否有子节点
        if (current->childcount == 8 && current->depth < target_depth) {
            const bool interpolate_existing_children = current->depth > MFEM_DEPTH - 1;
            if (interpolate_existing_children) {
                append_digitaltwin_child_vibration(current, vol, true);
                for (int n = 0; n < 8; ++n) {
                    if (!current->child[n]->is_outside()) {
                        digitaltwin_get_leaf_nodes_diff(current->child[n], nodes_to_process, vol, max_depth);
                    }
                }
                return;
            }
            // 检查子节点是否是inside
            for (int n = 0; n < 8; ++n) {

                if (current->depth > MFEM_DEPTH - 1) {
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
                    digitaltwin_get_leaf_nodes_diff(current->child[n], nodes_to_process, vol, max_depth);
                }
            }
            // 如果所有子节点都是inside，则不添加到nodes_to_process
        }
        else {
            // 没有子节点
            // 如果不是undecided状态且不是inside，判断是否达到最大深度
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

                    digitaltwin_get_leaf_nodes_diff(current->child[m], nodes_to_process, vol, max_depth);
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

    void cuda_functions::get_leaf_nodes_diff(Octnode* current, std::vector<Octnode*>& nodes_to_process, const Volume* vol, unsigned int max_depth) {
        //qDebug() << "vol->type():"<<vol->type;
        // 如果节点已经在外部或没有与体积重叠，则直接返回
        if (current->depth > (max_depth - 1) || current->is_outside() || !vol->bb.overlaps(current->bb)) {
            return;
        }
        //qDebug() << "get_leaf_nodes_diff():";
        // 首先判断是否有子节点
        if (current->childcount == 8 && current->depth < (max_depth - 1)) {
            // 检查子节点是否是inside
            for (int n = 0; n < 8; ++n) {
                if (!current->child[n]->is_outside()) {
                    get_leaf_nodes_diff(current->child[n], nodes_to_process, vol, max_depth);
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
                    get_leaf_nodes_diff(current->child[m], nodes_to_process, vol, max_depth);
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

    void cuda_functions::sum_volume_stl(Octnode* current, const StlVolume* vol, unsigned int max_depth) {
        qDebug() << "sum_volume_stl():";

        if (current == nullptr || vol == nullptr || max_depth == 0) {
            qDebug() << "sum_volume_stl(): invalid input";
            return;
        }

        if (current->is_inside() || !vol->bb.overlaps(current->bb)) {
            qDebug() << "sum_volume_stl(): nothing to process";
            return;
        }

        int facet_count = static_cast<int>(vol->facets.size());
        if (facet_count <= 0) {
            qDebug() << "sum_volume_stl(): empty STL facets";
            return;
        }

#if 0
        // 准备要处理的节点列表
        std::vector<Octnode*> nodes_to_process;

        qDebug() << "sum_volume_stl():";
        // 收集所有需要处理的叶节点
        get_leaf_nodes_sum(current, nodes_to_process, vol, max_depth);

        // 如果没有需要处理的节点，直接返回
        if (nodes_to_process.empty()) {
            qDebug() << "列表为空,计算结束";
            return;
        }

        // 分配节点数据数组
        size_t node_count = nodes_to_process.size();
        CudaNodeData* host_nodes = new CudaNodeData[node_count];

        // 填充节点数据
        for (size_t i = 0; i < node_count; i++) {
            Octnode* node = nodes_to_process[i];
            for (int j = 0; j < 8; j++) {
                host_nodes[i].x[j] = static_cast<float>(node->vertex[j]->x);
                host_nodes[i].y[j] = static_cast<float>(node->vertex[j]->y);
                host_nodes[i].z[j] = static_cast<float>(node->vertex[j]->z);
                host_nodes[i].f[j] = static_cast<float>(node->f[j]);
            }
        }

        // 准备 STL 数据
        int facet_count = vol->facets.size();
#endif
        StlParams stl_params;

        // 分配设备内存并复制数据
        GLVertex_xyz* d_facets_v1 = nullptr;
        GLVertex_xyz* d_facets_v2 = nullptr;
        GLVertex_xyz* d_facets_v3 = nullptr;
        GLVertex_xyz* d_facets_normal = nullptr;
        GLVertex_xyz* d_V21 = nullptr;
        GLVertex_xyz* d_V21invV21dotV21 = nullptr;
        GLVertex_xyz* d_V32 = nullptr;
        GLVertex_xyz* d_V32invV32dotV32 = nullptr;
        GLVertex_xyz* d_V13 = nullptr;
        GLVertex_xyz* d_V13invV13dotV13 = nullptr;

        // 获取 STL 数据的引用
        const std::vector<GLVertex>& V21_ref = vol->getV21();
        const std::vector<GLVertex>& V21inv_ref = vol->getV21invV21dotV21();
        const std::vector<GLVertex>& V32_ref = vol->getV32();
        const std::vector<GLVertex>& V32inv_ref = vol->getV32invV32dotV32();
        const std::vector<GLVertex>& V13_ref = vol->getV13();
        const std::vector<GLVertex>& V13inv_ref = vol->getV13invV13dotV13();
        const GLVertex& minpt_ref = vol->getMinPt();
        const GLVertex& maxpt_ref = vol->getMaxPt();
        double invcubesize_ref = vol->getInvCubeSize();

        // 复制数据到主机缓冲区
        GLVertex_xyz* h_facets_v1 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_facets_v2 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_facets_v3 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_facets_normal = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_V21 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_V21invV21dotV21 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_V32 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_V32invV32dotV32 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_V13 = new GLVertex_xyz[facet_count];
        GLVertex_xyz* h_V13invV13dotV13 = new GLVertex_xyz[facet_count];

        // 填充数据
        for (int i = 0; i < facet_count; i++) {
            h_facets_v1[i].x = static_cast<float>(vol->facets[i]->v1.x);
            h_facets_v1[i].y = static_cast<float>(vol->facets[i]->v1.y);
            h_facets_v1[i].z = static_cast<float>(vol->facets[i]->v1.z);
            h_facets_v2[i].x = static_cast<float>(vol->facets[i]->v2.x);
            h_facets_v2[i].y = static_cast<float>(vol->facets[i]->v2.y);
            h_facets_v2[i].z = static_cast<float>(vol->facets[i]->v2.z);
            h_facets_v3[i].x = static_cast<float>(vol->facets[i]->v3.x);
            h_facets_v3[i].y = static_cast<float>(vol->facets[i]->v3.y);
            h_facets_v3[i].z = static_cast<float>(vol->facets[i]->v3.z);
            h_facets_normal[i].x = static_cast<float>(vol->facets[i]->normal.x);
            h_facets_normal[i].y = static_cast<float>(vol->facets[i]->normal.y);
            h_facets_normal[i].z = static_cast<float>(vol->facets[i]->normal.z);
            if (i < static_cast<int>(V21_ref.size())) {
                h_V21[i].x = static_cast<float>(V21_ref[i].x);
                h_V21[i].y = static_cast<float>(V21_ref[i].y);
                h_V21[i].z = static_cast<float>(V21_ref[i].z);
            }
            if (i < static_cast<int>(V21inv_ref.size())) {
                h_V21invV21dotV21[i].x = static_cast<float>(V21inv_ref[i].x);
                h_V21invV21dotV21[i].y = static_cast<float>(V21inv_ref[i].y);
                h_V21invV21dotV21[i].z = static_cast<float>(V21inv_ref[i].z);
            }
            if (i < static_cast<int>(V32_ref.size())) {
                h_V32[i].x = static_cast<float>(V32_ref[i].x);
                h_V32[i].y = static_cast<float>(V32_ref[i].y);
                h_V32[i].z = static_cast<float>(V32_ref[i].z);
            }
            if (i < static_cast<int>(V32inv_ref.size())) {
                h_V32invV32dotV32[i].x = static_cast<float>(V32inv_ref[i].x);
                h_V32invV32dotV32[i].y = static_cast<float>(V32inv_ref[i].y);
                h_V32invV32dotV32[i].z = static_cast<float>(V32inv_ref[i].z);
            }
            if (i < static_cast<int>(V13_ref.size())) {
                h_V13[i].x = static_cast<float>(V13_ref[i].x);
                h_V13[i].y = static_cast<float>(V13_ref[i].y);
                h_V13[i].z = static_cast<float>(V13_ref[i].z);
            }
            if (i < static_cast<int>(V13inv_ref.size())) {
                h_V13invV13dotV13[i].x = static_cast<float>(V13inv_ref[i].x);
                h_V13invV13dotV13[i].y = static_cast<float>(V13inv_ref[i].y);
                h_V13invV13dotV13[i].z = static_cast<float>(V13inv_ref[i].z);
            }
        }

        // 分配设备内存并复制
        cudaMalloc(&d_facets_v1, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_facets_v2, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_facets_v3, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_facets_normal, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_V21, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_V21invV21dotV21, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_V32, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_V32invV32dotV32, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_V13, facet_count * sizeof(GLVertex_xyz));
        cudaMalloc(&d_V13invV13dotV13, facet_count * sizeof(GLVertex_xyz));

        cudaMemcpy(d_facets_v1, h_facets_v1, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_facets_v2, h_facets_v2, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_facets_v3, h_facets_v3, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_facets_normal, h_facets_normal, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V21, h_V21, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V21invV21dotV21, h_V21invV21dotV21, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V32, h_V32, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V32invV32dotV32, h_V32invV32dotV32, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V13, h_V13, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);
        cudaMemcpy(d_V13invV13dotV13, h_V13invV13dotV13, facet_count * sizeof(GLVertex_xyz), cudaMemcpyHostToDevice);

        stl_params.facets_v1 = d_facets_v1;
        stl_params.facets_v2 = d_facets_v2;
        stl_params.facets_v3 = d_facets_v3;
        stl_params.facets_normal = d_facets_normal;
        stl_params.V21 = d_V21;
        stl_params.V21invV21dotV21 = d_V21invV21dotV21;
        stl_params.V32 = d_V32;
        stl_params.V32invV32dotV32 = d_V32invV32dotV32;
        stl_params.V13 = d_V13;
        stl_params.V13invV13dotV13 = d_V13invV13dotV13;
        stl_params.facet_count = facet_count;
        stl_params.min_x = static_cast<float>(minpt_ref.x);
        stl_params.min_y = static_cast<float>(minpt_ref.y);
        stl_params.min_z = static_cast<float>(minpt_ref.z);
        stl_params.max_x = static_cast<float>(maxpt_ref.x);
        stl_params.max_y = static_cast<float>(maxpt_ref.y);
        stl_params.max_z = static_cast<float>(maxpt_ref.z);
        stl_params.inv_cube_size = static_cast<float>(invcubesize_ref);
        stl_params.r = vol->color.r;
        stl_params.g = vol->color.g;
        stl_params.b = vol->color.b;

        // 释放主机内存
        delete[] h_facets_v1;
        delete[] h_facets_v2;
        delete[] h_facets_v3;
        delete[] h_facets_normal;
        delete[] h_V21;
        delete[] h_V21invV21dotV21;
        delete[] h_V32;
        delete[] h_V32invV32dotV32;
        delete[] h_V13;
        delete[] h_V13invV13dotV13;

        // 调用 CUDA 函数
        const size_t processed_count = sum_stl_octree_batched(current, vol, max_depth, stl_params);
        qDebug() << "sum_volume_stl processed nodes:" << static_cast<unsigned long long>(processed_count);
#if 0
        cuda_sum_stl(host_nodes, node_count, stl_params);

        // 更新节点数据
        for (size_t i = 0; i < node_count; i++) {
            Octnode* node = nodes_to_process[i];
            bool updated = false;
            for (int j = 0; j < 8; j++) {
                if (static_cast<double>(host_nodes[i].f[j]) > node->f[j]) {
                    node->f[j] = static_cast<double>(host_nodes[i].f[j]);
                    updated = true;
                }
            }
            if (updated) {
                node->color = vol->color;
            }
            node->set_state();
        }

        // 释放设备内存
        update_parent_states_from_leaves(nodes_to_process);

#endif
        cudaFree(d_facets_v1);
        cudaFree(d_facets_v2);
        cudaFree(d_facets_v3);
        cudaFree(d_facets_normal);
        cudaFree(d_V21);
        cudaFree(d_V21invV21dotV21);
        cudaFree(d_V32);
        cudaFree(d_V32invV32dotV32);
        cudaFree(d_V13);
        cudaFree(d_V13invV13dotV13);

        // 释放主机内存
    }

    void cuda_functions::diff_volume(Octnode* current, const Volume* vol, unsigned int max_depth) { // 添加max_depth参数


        // 准备要处理的节点列表
        std::vector<Octnode*> nodes_to_process;

        //std::chrono::system_clock::time_point start, stop;
        //start = std::chrono::system_clock::now();
        qDebug() << "sum_volume():";
        // 收集所有需要处理的叶节点
        get_leaf_nodes_diff(current, nodes_to_process, vol, max_depth);

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
            if (node->f[0] == host_nodes[i].f[0]) {
                qDebug() << "error：dist未更新:" << (node->f[0] == host_nodes[i].f[0]);

            }

            bool updated = false; // 标记是否有更新
            for (int j = 0; j < 8; j++) {
                if (static_cast<double>(-host_nodes[i].f[j]) < node->f[j]) {
                    node->f[j] = static_cast<double>(-host_nodes[i].f[j]);
                    updated = true;
                }
            }

            // 如果有更新，则更新节点颜色
            if (updated) {
                node->color = vol->color; // 更新颜色为体积的颜色
            }

            node->set_state(); // 更新节点状态


        }

        // 释放内存A
        delete[] host_nodes;

    }


    // ... 其他代码 ...
    void cuda_functions::broaching_diff_volume_blade(Octnode* current, broaching_AptCutterVolume* vol, unsigned int max_depth, Octree* octree) {
        // 收集叶节点

        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed(0);

        nodes_to_process.clear();
        if (nodes_to_process.capacity() < 512) {
            nodes_to_process.reserve(512);
        }

        // 默认处理第0个切削刃，实际使用时应该根据需要指定blade_id
        int blade_id = vol->blade_num;
        broaching_get_leaf_nodes_diff_blade(current, nodes_to_process, vol, max_depth, elapsed, blade_id);

        //MeshIDExport(octree);

        stop = std::chrono::system_clock::now();
        qDebug() << "broaching_get_leaf_nodes_diff_blade():" << std::chrono::duration<double>(stop - start).count() << "sec.";
        //       qDebug() << "get_leaf_nodes_diff_blade_second():" << elapsed.count() << "sec.";

        start = std::chrono::system_clock::now();

        size_t node_count = nodes_to_process.size();
        if (node_count == 0) return;

        CudaNodeData* host_nodes = new CudaNodeData[node_count];
        std::unordered_map<int, GLVertex> host_node_points;
        host_node_points.reserve(node_count * 8);
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
                host_node_points[node_id] = GLVertex(host_nodes[i].x[j], host_nodes[i].y[j], host_nodes[i].z[j]);
            }
        }

        broaching_BladeParams blade;
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

            broaching_cuda_diff_volume_blade(host_nodes, node_count, blade,
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
            GLVertex min_node_point;
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
                const auto point_it = host_node_points.find(node_id);
                if (point_it != host_node_points.end()) {
                    entry.min_node_point = point_it->second;
                }
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

            const_cast<broaching_AptCutterVolume*>(vol)->addMachiningContactEvent(
                blade_id,
                z_key,
                real_blade_points,
                real_blade_points_up,
                z_data.min_node_point,
                vol->cube_resolution_1
            );
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
            auto& real_blade_map = const_cast<broaching_AptCutterVolume*>(vol)->real_blade_points_map;

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
            if (!std::isfinite(cuth_distence) || cuth_distence < 0.0) {
                cuth_distence = 0.0;
            }
            double max_cuth_distence = 0.2;
            if (blade_id < static_cast<int>(vol->blade_cut_h.size()) &&
                z_key < static_cast<int>(vol->blade_cut_h[blade_id].size())) {
                const double file_cut_h = std::abs(vol->blade_cut_h[blade_id][z_key]);
                if (file_cut_h > 1e-9) {
                    max_cuth_distence = std::max(max_cuth_distence, 5.0 * file_cut_h);
                }
            }
            if (cuth_distence > max_cuth_distence) {
                cuth_distence = max_cuth_distence;
            }
            //printf("%f ",z_key);
            //printf("%f\n",cuth_distence);
            const_cast<broaching_AptCutterVolume*>(vol)->add_cut_h_map(
                vol->new_angle,          // 外层键：当前刀具角度
                blade_id,
                z_key,          // 内层键：z坐标值
                euclidean_distance  // 直线度
            );
            const_cast<broaching_AptCutterVolume*>(vol)->addcut_h(
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
        std::vector<double> color_values(node_count, 0.0);
        std::vector<char> color_active(node_count, 0);
        constexpr int invalid_vertex_node_id = 9999999;
        std::unordered_map<int, double> min_f_by_node_id;
        min_f_by_node_id.reserve(node_count * 8);

        for (size_t i = 0; i < node_count; ++i) {
            for (int j = 0; j < 8; ++j) {
                const int node_id = host_nodes[i].node_id[j];
                if (node_id == invalid_vertex_node_id) {
                    continue;
                }
                const double new_f = static_cast<double>(-host_nodes[i].f[j]);
                auto it = min_f_by_node_id.find(node_id);
                if (it == min_f_by_node_id.end() || new_f < it->second) {
                    min_f_by_node_id[node_id] = new_f;
                }
            }
        }

        // 优化：使用QtConcurrent并行处理节点更新
        // 捕获this指针以访问类的成员变量和成员函数
        auto update_node = [this, host_nodes, has_vibration_data, has_vibration_q, vibration_q_iter, vol, invalid_vertex_node_id, &color_values, &color_active, &min_f_by_node_id](size_t i) {
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
                int node_id = node_ids[j];
                auto f_it = (node_id == invalid_vertex_node_id)
                    ? min_f_by_node_id.end()
                    : min_f_by_node_id.find(node_id);
                double new_f = (f_it != min_f_by_node_id.end())
                    ? f_it->second
                    : static_cast<double>(-host_f[j]);
                if (new_f < node_f[j]) {
                    node_f[j] = new_f;
                    updated = true;
                }
                double q_x = 0.0, q_y = 0.0, q_z = 0.0;

                // 预计算模态数量
                size_t modal_count = (has_vibration_data && has_vibration_q) ? vol->temp_vibration_vectors.size() : 0;
                for (size_t modal = 0; modal < modal_count; modal++) {
                    const auto& vibration_q = vibration_q_iter->second;
                    double modal_factor = vibration_q[modal];

                    // 避免重复索引查找
                    const auto& modal_vectors = vol->temp_vibration_vectors[modal];
                    q_x += static_cast<float>(modal_vectors[0][node_id] * modal_factor);
                    q_y += static_cast<float>(modal_vectors[1][node_id] * modal_factor);
                    q_z += static_cast<float>(modal_vectors[2][node_id] * modal_factor);
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
            color_values[i] = selected_deformation_value(vol->deform_color_var, q_total, u_x, u_y, u_z);
            color_active[i] = 1;
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
        const double shared_f_tolerance = std::max(1e-9, vol->cube_resolution_1 * 1e-6);
        //check_leaf_shared_vertex_f_consistency(nodes_to_process, vol->normalvertices_size, shared_f_tolerance, 20);
        update_dynamic_deform_color_range(vol, color_values, color_active);
        for (size_t i = 0; i < node_count; ++i) {
            if (!color_active[i]) {
                continue;
            }

            float r, g, b;
            getDeformationColor(color_values[i], vol->deform_color_min, vol->deform_color_max, r, g, b);
            nodes_to_process[i]->color = { r, g, b };
        }
        broaching_clean_outside_nodes(current, vol);  // 清理当前节点及其子节点中的无效OUTSIDE节点

        delete[] host_nodes;
        delete[] blade.blade_points;
        delete[] blade.plane_normals;
        delete[] blade.plane_points;


        stop = std::chrono::system_clock::now();
        qDebug() << "结果处理：():" << std::chrono::duration<double>(stop - start).count() << "sec.";


    }

    void cuda_functions::milling_diff_volume_blade(Octnode* current, milling_AptCutterVolume* vol, unsigned int max_depth, Octree* octree) {
        // 收集叶节点

        std::chrono::system_clock::time_point start, stop;
        //start = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed(0);

        nodes_to_process.clear();
        milling_get_leaf_nodes_diff_blade(current, nodes_to_process, vol, max_depth, elapsed);

        //stop = std::chrono::system_clock::now();
        //qDebug() << "milling_get_leaf_nodes_diff_blade():" << std::chrono::duration<double>(stop - start).count() << "sec.";
        //    qDebug() << "get_leaf_nodes_diff_blade_bbox():" << elapsed.count() << "sec.";

        //start = std::chrono::system_clock::now();

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
                //            for (int modal = 0; modal < vol->temp_vibration_vectors.size(); modal++) {

                //                host_nodes[i].x[j] -= static_cast<float>(
                //                            vol->temp_vibration_vectors[modal][0][node_id] *
                //                        vol->vibration_q[modal]
                //                        );
                //                host_nodes[i].y[j] -= static_cast<float>(
                //                            vol->temp_vibration_vectors[modal][1][node_id] *
                //                        vol->vibration_q[modal]
                //                        );
                //                host_nodes[i].z[j] -= static_cast<float>(
                //                            vol->temp_vibration_vectors[modal][2][node_id] *
                //                        vol->vibration_q[modal]
                //                        );
                //            }
            }
        }

        milling_BladeParams blade;
        blade.center_x = static_cast<float>(vol->center.x);
        blade.center_y = static_cast<float>(vol->center.y);
        blade.center_z = static_cast<float>(vol->center.z);
        blade.dx = static_cast<float>(vol->dx);  // 新增dx赋值
        blade.dy = static_cast<float>(vol->dy);  // 新增dy赋值
        blade.dz = static_cast<float>(vol->dz);  // 新增dz赋值
        blade.cube_resolution = static_cast<float>(vol->cube_resolution_1);
        blade.device_id = 0;  // 设置设备ID

        // 处理blade_points数据
        if (!vol->blade_points.empty()) {
            // 分配主机内存
            blade.blade_points = new GLVertex_xyz[vol->blade_points.size()];
            // 拷贝数据
            for (size_t i = 0; i < vol->blade_points.size(); ++i) {
                blade.blade_points[i].x = vol->blade_points[i].x;
                blade.blade_points[i].y = vol->blade_points[i].y;
                blade.blade_points[i].z = vol->blade_points[i].z;
            }

            blade.blade_points_count = static_cast<int>(vol->blade_points.size());
        }
        else {
            blade.blade_points = nullptr;
            blade.blade_points_count = 0;
        }
        // 新增：显式检查blade_points_count合法性
        if (blade.blade_points_count < 0) {
            qDebug() << "Invalid blade_points_count: " << blade.blade_points_count;
            delete[] blade.blade_points;
            delete[] host_nodes;
            return;
        }

        float* host_z_array = nullptr;
        float* host_distence2edge = nullptr;
        int* host_node_ids = nullptr;  // 新增
        int host_record_count = 0;

        // 添加CUDA错误检查
        cudaError_t cudaStatus = cudaGetLastError();
        if (cudaStatus != cudaSuccess) {
            qDebug() << "CUDA初始化错误:" << cudaGetErrorString(cudaStatus);
            delete[] blade.blade_points;
            delete[] host_nodes;
            return;
        }

        // 配置BladeParams
        blade.device_id = 0;  // 设置设备ID

        // milling_cuda_diff_volume_blade copies blade_points to device internally.



        //stop = std::chrono::system_clock::now();
        //qDebug() << "设备内存操作():" << std::chrono::duration<double>(stop - start).count() << "sec.";


        try {
            cudaSetDevice(0);
            cudaDeviceSynchronize();
            cudaError_t preLaunchErr = cudaGetLastError();
            if (preLaunchErr != cudaSuccess) {
                qDebug() << "Pre-launch error:" << cudaGetErrorString(preLaunchErr);
                // 释放主机内存
                delete[] blade.blade_points;
                delete[] host_nodes;
                return;
            }

            //start = std::chrono::system_clock::now();

            milling_cuda_diff_volume_blade(host_nodes, node_count, blade,
                &host_z_array, &host_distence2edge, &host_node_ids, &host_record_count);

            //stop = std::chrono::system_clock::now();
            //qDebug() << "cuda计算时间():" << std::chrono::duration<double>(stop - start).count() << "sec.";

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
            free(host_z_array);
            free(host_distence2edge);
            free(host_node_ids);
            delete[] blade.blade_points;
            delete[] host_nodes;
            for (size_t i = 0; i < node_count; i++) {
                nodes_to_process[i]->diff(vol);
                nodes_to_process[i]->set_state();
            }
            return;
        }

        //start = std::chrono::system_clock::now();

        // 修改后的数据结构
        struct ZData {
            double min_d2edge = FLT_MAX;
            int min_node_id = -1;
        };
        std::unordered_map<float, ZData> z2sum_count;

        for (int i = 0; i < host_record_count; ++i) {

            int node_id = host_node_ids[i];
            int inside_index = host_z_array[i];
            double d2edge = host_distence2edge[i];

            // 更新统计信息
            auto& entry = z2sum_count[inside_index];

            // 跟踪最小d2edge
            if (d2edge < entry.min_d2edge) {
                entry.min_d2edge = d2edge;
                entry.min_node_id = node_id;
            }
        }

        for (const auto& [z_key, z_data] : z2sum_count) {

            double blade_points_bottom_x = vol->blade_points[z_key * 4 + 1].x;
            double blade_points_bottom_y = vol->blade_points[z_key * 4 + 1].y;
            double blade_points_bottom_z = vol->blade_points[z_key * 4 + 1].z;
            double blade_points_up_x = vol->blade_points[z_key * 4 + 2].x;
            double blade_points_up_y = vol->blade_points[z_key * 4 + 2].y;
            double blade_points_up_z = vol->blade_points[z_key * 4 + 2].z;
            // 获取z_key+1对应的z_data（新增检查）
            auto up_z_data_it = z2sum_count.find(z_key + 1);
            if (up_z_data_it == z2sum_count.end()) continue;  // 若不存在则跳过
            const auto& up_z_data = up_z_data_it->second;

            // 底部点使用当前z_key的min_node_id（保持原逻辑）
            for (int modal = 0; modal < vol->temp_vibration_vectors.size(); modal++) {
                blade_points_bottom_x -= vol->temp_vibration_vectors[modal][0][z_data.min_node_id] *
                    vol->next_vibration_q[modal];  // 修正为+=
                blade_points_bottom_y -= vol->temp_vibration_vectors[modal][1][z_data.min_node_id] *
                    vol->next_vibration_q[modal];  // 修正为+=
                blade_points_bottom_z -= vol->temp_vibration_vectors[modal][2][z_data.min_node_id] *
                    vol->next_vibration_q[modal];  // 修正为+=
            }

            // 顶部点使用z_key+1的min_node_id（修改核心）
            for (int modal = 0; modal < vol->temp_vibration_vectors.size(); modal++) {
                blade_points_up_x -= vol->temp_vibration_vectors[modal][0][up_z_data.min_node_id] *
                    vol->next_vibration_q[modal];
                blade_points_up_y -= vol->temp_vibration_vectors[modal][1][up_z_data.min_node_id] *
                    vol->next_vibration_q[modal];
                blade_points_up_z -= vol->temp_vibration_vectors[modal][2][up_z_data.min_node_id] *
                    vol->next_vibration_q[modal];
            }


            double cuth_distence = 0.0;

            // 清除real_blade_points_map中第一个key小于vol->tool_angle-3*M_PI之前的数据
            double clear_threshold = vol->tool_angle - 3 * M_PI;
            auto& real_blade_map = const_cast<milling_AptCutterVolume*>(vol)->real_blade_points_map;

            // 找到第一个不小于阈值的元素迭代器
            auto clear_end = real_blade_map.lower_bound(clear_threshold);
            // 清除从begin()到clear_end的所有元素
            if (clear_end != real_blade_map.begin()) {
                real_blade_map.erase(real_blade_map.begin(), clear_end);
            }

            // 定义目标角度区间 [tool_angle - 2π, tool_angle - 1.5π]
            const double start_angle = vol->tool_angle - 2 * M_PI / vol->blade_sum - 0.5 * M_PI;
            const double end_angle = vol->tool_angle - 2 * M_PI / vol->blade_sum + 0.5 * M_PI;
            const auto& angle_map = vol->real_blade_points_map;
            // 收集区间内的所有角度键
            std::vector<double> candidate_angles;
            if (!angle_map.empty()) {
                // 使用lower_bound和upper_bound获取区间迭代器范围
                auto it_start = angle_map.lower_bound(start_angle);
                auto it_end = angle_map.upper_bound(end_angle);

                // 遍历区间内的所有元素
                for (auto it = it_start; it != it_end; ++it) {
                    candidate_angles.push_back(it->first);
                }
            }

            // 计算区间内所有角度的最小距离 - 优化版本
            double min_distance = std::numeric_limits<double>::max();
            GLVertex P_min;

            if (!candidate_angles.empty()) {  // 存在区间内的角度
                // 预先创建当前刀片线段的端点，避免重复创建
                GLVertex P0(blade_points_bottom_x, blade_points_bottom_y, blade_points_bottom_z);
                GLVertex P1(blade_points_up_x, blade_points_up_y, blade_points_up_z);

                // 遍历候选角度
                for (double pre_angle : candidate_angles) {
                    auto prev_angle_map_it = angle_map.find(pre_angle);
                    if (prev_angle_map_it == angle_map.end()) continue;

                    const auto& prev_angle_map = prev_angle_map_it->second;

                    // 只精确查找当前z_key
                    auto z_entry_it = prev_angle_map.find(z_key);
                    if (z_entry_it != prev_angle_map.end()) {
                        const std::vector<GLVertex>& blade_points = z_entry_it->second;

                        // 遍历所有刀片点，计算最小距离
                        for (const GLVertex& q : blade_points) {
                            // 判断向量方向：vol->center到P0和P0到q是否同向
                            GLVertex v1 = P0 - vol->center; // 从刀具中心到P0
                            GLVertex v2 = q - P0; // 从P0到q
                            double dot_product = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;

                            // 如果点积为负，向量反向，设置min_distance=0.0
                            if (dot_product < 0) {
                                min_distance = 0.0;
                                P_min = q;
                                break; // 找到反向点后直接退出循环
                            }

                            // 向量同向，计算距离
                            double dist = distanceToSegment(q, P0, P1);
                            if (dist < min_distance) {
                                min_distance = dist;
                                P_min = q; // 直接赋值，避免逐个成员复制
                            }
                        }
                    }
                }
            }
            else {  // 区间内无有效角度
                min_distance = 0.0;
            }

            if (min_distance != std::numeric_limits<double>::max()) {
                cuth_distence = min_distance;
            }

            GLVertex real_blade_points(blade_points_bottom_x, blade_points_bottom_y, blade_points_bottom_z);
            GLVertex real_blade_points_up(blade_points_up_x, blade_points_up_y, blade_points_up_z);

            // 替换而不是push_back：清空现有数据后添加新数据
            auto& z_vector = vol->real_blade_points_map[vol->tool_angle][z_key];
            z_vector.clear();
            z_vector.push_back(real_blade_points);

            //        //在底部点和上部点之间插入10个等间距的点
            //        for (int i = 1; i <= 4; i++) {
            //            double t = static_cast<double>(i) / 5.0;  // 生成11个等间距的分段
            //            double interp_x = blade_points_bottom_x + t * (blade_points_up_x - blade_points_bottom_x);
            //            double interp_y = blade_points_bottom_y + t * (blade_points_up_x - blade_points_bottom_y);
            //            double interp_z = blade_points_bottom_z + t * (blade_points_up_z - blade_points_bottom_z);

            //            GLVertex interp_point(interp_x, interp_y, interp_z);
            //            vol->real_blade_points_map[vol->tool_angle][z_key].push_back(interp_point);
            //        }
            z_vector.push_back(real_blade_points_up);

            // Disabled to avoid retaining surface trace points for every simulation step.
            /*
            // const_cast<milling_AptCutterVolume*>(vol)->add_surface_map(
                z_key,          // 内层键：z坐标值
            //     real_blade_points
            // );
            */
            const_cast<milling_AptCutterVolume*>(vol)->addcut_h(
                z_key,
                cuth_distence,
                z_data.min_d2edge,  // 新增最小距离
                z_data.min_node_id,  // 新增对应节点ID
                P_min,
                up_z_data.min_node_id,
                real_blade_points,
                real_blade_points_up
            );
        }

        free(host_z_array);
        free(host_distence2edge);
        free(host_node_ids);

        start = std::chrono::system_clock::now();

        // 缓存振动向量和模态参数，减少内存访问
        const auto& temp_vibration_vectors = vol->temp_vibration_vectors;
        const auto& next_vibration_q = vol->next_vibration_q;
        size_t num_modes = temp_vibration_vectors.size();
        double q_total = 0.0;
        double u_x = 0.0;
        double u_y = 0.0;
        double u_z = 0.0;
        bool color_updated = false;
        double q_x = 0.0, q_y = 0.0, q_z = 0.0;
        int node_id;
        std::vector<double> color_values(node_count, 0.0);
        std::vector<char> color_active(node_count, 0);

        // 创建索引范围，避免在并行处理中查找索引
        // Update Octnode sequentially.
        for (size_t i = 0; i < node_count; ++i) {
            Octnode* node = nodes_to_process[i];

            q_total = 0.0;
            u_x = 0.0;
            u_y = 0.0;
            u_z = 0.0;
            color_updated = false;

            for (int j = 0; j < 8; j++) {
                node->updated[j] = false;
                if (static_cast<double>(-host_nodes[i].f[j]) < node->f[j]) {
                    node->f[j] = static_cast<double>(-host_nodes[i].f[j]);
                    node->updated[j] = true;
                    color_updated = true;
                }
            }

            if (color_updated) {

                for (int j = 0; j < 8; j++) {

                    node_id = host_nodes[i].node_id[j];
                    q_x = 0.0, q_y = 0.0, q_z = 0.0;

                    for (size_t modal = 0; modal < num_modes; modal++) {
                        double q_mode = next_vibration_q[modal];
                        q_x += static_cast<float>(temp_vibration_vectors[modal][0][node_id] * q_mode);
                        q_y += static_cast<float>(temp_vibration_vectors[modal][1][node_id] * q_mode);
                        q_z += static_cast<float>(temp_vibration_vectors[modal][2][node_id] * q_mode);
                    }

                    //q_total += std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z);
                    q_total += q_x;
                    u_x += q_x;
                    u_y += q_y;
                    u_z += q_z;

                }
            }

            if (color_updated) {
                q_total = std::abs(q_total) / 8.0;
                u_x = u_x / 8.0;
                u_y = u_y / 8.0;
                u_z = u_z / 8.0;
                color_values[i] = selected_deformation_value(vol->deform_color_var, q_total, u_x, u_y, u_z);
                color_active[i] = 1;
            }

            node->set_state();

        }

        milling_clean_outside_nodes(current, vol);  // 清理当前节点及其子节点中的无效OUTSIDE节点

        // 释放主机内存
        update_dynamic_deform_color_range(vol, color_values, color_active);
        for (size_t i = 0; i < node_count; ++i) {
            if (!color_active[i]) {
                continue;
            }

            float r, g, b;
            getDeformationColor(color_values[i], vol->deform_color_min, vol->deform_color_max, r, g, b);
            nodes_to_process[i]->color = { r, g, b };
        }

        delete[] blade.blade_points;
        delete[] host_nodes;

        stop = std::chrono::system_clock::now();
        qDebug() << " 结果处理():" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void cuda_functions::digitaltwin_milling_diff_volume_blade(Octnode* current, digitaltwin_AptCutterVolume* vol, unsigned int max_depth, Octree* octree) { // 添加max_depth参数


        // 使用类的nodes_to_process成员变量
        nodes_to_process.clear();
        if (nodes_to_process.capacity() < 512) {
            nodes_to_process.reserve(512);
        }

        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        qDebug() << "digitaltwin_milling_diff_volume_blade():";
        // 收集所有需要处理的叶节点
        digitaltwin_get_leaf_nodes_diff(current, nodes_to_process, vol, max_depth);

        stop = std::chrono::system_clock::now();
        qDebug() << "get_leaf_nodes():" << std::chrono::duration<double>(stop - start).count() << "sec.";
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
                int node_id;
                if (node->vertex[j]->id <= 0)
                    node_id = -node->vertex[j]->id; // 正常节点编号
                else
                    node_id = node->vertex[j]->id + vol->normalvertices_size - 1; // 悬挂节点编号
                host_nodes[i].node_id[j] = node_id;
                //for (int modal = 0; modal < vol->temp_vibration_vectors.size(); modal++) {

                //    host_nodes[i].x[j] -= static_cast<float>(
                //        vol->temp_vibration_vectors[modal][0][node_id] *
                //        vol->vibration_q[modal]
                //        );
                //    host_nodes[i].y[j] -= static_cast<float>(
                //        vol->temp_vibration_vectors[modal][1][node_id] *
                //        vol->vibration_q[modal]
                //        );
                //    host_nodes[i].z[j] -= static_cast<float>(
                //        vol->temp_vibration_vectors[modal][2][node_id] *
                //        vol->vibration_q[modal]
                //        );
                //}
            }
        }


        // 修改类型转换部分（约113行和126行）
        // 在参数填充循环之后添加变量声明（约109行）
        std::vector<CutterSegment> segments;
        segments.reserve(vol->segments.size());
        for (const auto& seg : vol->segments) {
            CutterSegment cseg;
            cseg.type = seg.type;
            cseg.radius1 = seg.radius1;
            cseg.radius2 = seg.radius2;
            cseg.length = seg.length;
            cseg.center_xyz.x = static_cast<float>(vol->center.x);
            cseg.center_xyz.y = static_cast<float>(vol->center.y);
            cseg.center_xyz.z = static_cast<float>(vol->center.z);
            cseg.angle_xyz.x = static_cast<float>(vol->angle.x);
            cseg.angle_xyz.y = static_cast<float>(vol->angle.y);
            cseg.angle_xyz.z = static_cast<float>(vol->angle.z);
            cseg.z_start = seg.z_start;
            cseg.z_end = seg.z_end;
            segments.push_back(cseg);
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

            start = std::chrono::system_clock::now();

            // 调用CUDA函数，传递指针和数量
            digitaltwin_milling_cuda_diff_volume_blade(
                host_nodes,
                node_count,
                segments.data(),          // 传递数据指针
                static_cast<int>(segments.size())          // 传递元素数量
            );

            stop = std::chrono::system_clock::now();
            qDebug() << "cuda_diff_volume() :" << std::chrono::duration<double>(stop - start).count() << "sec.";
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

        start = std::chrono::system_clock::now();

        // 缓存振动向量和模态参数，减少内存访问
        const auto& temp_vibration_vectors = vol->temp_vibration_vectors;
        const auto& next_vibration_q = vol->next_vibration_q;
        size_t num_modes = temp_vibration_vectors.size();
        double q_total = 0.0;
        double u_x = 0.0;
        double u_y = 0.0;
        double u_z = 0.0;
        bool color_updated = false;
        double q_x = 0.0, q_y = 0.0, q_z = 0.0;
        int node_id;
        std::vector<double> color_values(node_count, 0.0);
        std::vector<char> color_active(node_count, 0);

        // 创建索引范围，避免在并行处理中查找索引
        // Update Octnode sequentially.
        for (size_t i = 0; i < node_count; ++i) {
            Octnode* node = nodes_to_process[i];

            q_total = 0.0;
            u_x = 0.0;
            u_y = 0.0;
            u_z = 0.0;
            color_updated = false;

            for (int j = 0; j < 8; j++) {
                node->updated[j] = false;
                if (static_cast<double>(-host_nodes[i].f[j]) < node->f[j]) {
                    node->f[j] = static_cast<double>(-host_nodes[i].f[j]);
                    node->updated[j] = true;
                    color_updated = true;
                }

                if (color_updated) {

                    node_id = host_nodes[i].node_id[j];
                    q_x = 0.0, q_y = 0.0, q_z = 0.0;

                    for (size_t modal = 0; modal < num_modes; modal++) {
                        double q_mode = next_vibration_q[modal];
                        q_x += static_cast<float>(temp_vibration_vectors[modal][0][node_id] * q_mode);
                        q_y += static_cast<float>(temp_vibration_vectors[modal][1][node_id] * q_mode);
                        q_z += static_cast<float>(temp_vibration_vectors[modal][2][node_id] * q_mode);
                    }

                    q_total += std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z);
                    //q_total += q_x;
                    u_x += q_x;
                    u_y += q_y;
                    u_z += q_z;

                }

            }

            if (color_updated) {
                q_total = std::abs(q_total) / 8.0;
                u_x = u_x / 8.0;
                u_y = u_y / 8.0;
                u_z = u_z / 8.0;
                color_values[i] = selected_deformation_value(vol->deform_color_var, q_total, u_x, u_y, u_z);
                color_active[i] = 1;
            }

            node->set_state();

        }


        stop = std::chrono::system_clock::now();
        qDebug() << "node update :" << std::chrono::duration<double>(stop - start).count() << "sec.";

        update_dynamic_deform_color_range(vol, color_values, color_active);
        for (size_t i = 0; i < node_count; ++i) {
            if (!color_active[i]) {
                continue;
            }

            float r, g, b;
            getDeformationColor(color_values[i], vol->deform_color_min, vol->deform_color_max, r, g, b);
            nodes_to_process[i]->color = { r, g, b };
        }

        start = std::chrono::system_clock::now();

        digitaltwin_milling_clean_outside_nodes(current, vol);  // 清理当前节点及其子节点中的无效OUTSIDE节点

        stop = std::chrono::system_clock::now();
        qDebug() << "clean_outside_nodes :" << std::chrono::duration<double>(stop - start).count() << "sec.";

        // 释放内存A
        delete[] host_nodes;

    }


} // end namespace cutsim
