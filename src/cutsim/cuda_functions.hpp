// 文件名: src/cutsim/cuda_functions.hpp
#ifndef CUDA_DIFF_VOLUME_HPP
#define CUDA_DIFF_VOLUME_HPP

#include "octree.hpp"
#include "volume.hpp"
#include <vector>
#include <src/cutsim/mfem_analysis1.hpp>


namespace cutsim {

    // 前向声明
    class Octnode;
    class Volume;

    // 定义cuda_functions类
    class cuda_functions {
    public:
        // 构造函数，初始化CUDA状态
        cuda_functions();

        // 析构函数，清理CUDA资源
        ~cuda_functions();

        //表面可视化
        std::vector<Octnode*> nodes_to_process;

        double distanceToSegment(const GLVertex& p, const GLVertex& a, const GLVertex& b) const {
            double ax = a.x, az = a.z;
            double bx = b.x, bz = b.z;
            double px = p.x, pz = p.z;

            double dx = bx - ax;
            double dz = bz - az;

            // 线段退化为点
            if (dx == 0 && dz == 0) {
                return std::hypot(px - ax, pz - az);
            }

            // 计算投影参数t = (AP·AB)/(AB·AB)
            double t = ((px - ax) * dx + (pz - az) * dz) / (dx * dx + dz * dz);

            // 只有当投影点在线段内部(0≤t≤1)时才计算垂距
            if (t >= 0.0 && t <= 1.0) {
                // 投影点坐标
                double proj_x = ax + t * dx;
                double proj_z = az + t * dz;

                // 计算点到投影点的距离（垂距）
                return std::hypot(px - proj_x, pz - proj_z);
            }
            else {
                // 投影点不在线段上，返回一个非常大的数
                return std::numeric_limits<double>::max();
            }
        }

        // 更直观的彩色映射：蓝色 -> 青色 -> 绿色 -> 黄色 -> 红色
        void getDeformationColor(double q_total, double min_val, double max_val,
            float& r, float& g, float& b) {
            if (max_val == min_val) {
                r = g = b = 0.5f; // 中性灰色
                return;
            }

            // 归一化到 [0, 1]
            double t = (q_total - min_val) / (max_val - min_val);
            t = std::max(0.0, std::min(1.0, t));

            // 分段线性插值，创建彩虹色映射
            if (t < 0.25) {
                // 蓝色到青色：蓝色(0,0,1) -> 青色(0,1,1)
                r = 0.0f;
                g = static_cast<float>(t / 0.25);
                b = 1.0f;
            }
            else if (t < 0.5) {
                // 青色到绿色：青色(0,1,1) -> 绿色(0,1,0)
                r = 0.0f;
                g = 1.0f;
                b = static_cast<float>(1.0 - (t - 0.25) / 0.25);
            }
            else if (t < 0.75) {
                // 绿色到黄色：绿色(0,1,0) -> 黄色(1,1,0)
                r = static_cast<float>((t - 0.5) / 0.25);
                g = 1.0f;
                b = 0.0f;
            }
            else {
                // 黄色到红色：黄色(1,1,0) -> 红色(1,0,0)
                r = 1.0f;
                g = static_cast<float>(1.0 - (t - 0.75) / 0.25);
                b = 0.0f;
            }
        }


        // 计算体积差异并更新节点状态
        void diff_volume(Octnode* current, const CylCutterVolume* vol, unsigned int max_depth);

        void sum_volume(Octnode* current, const Volume* vol, unsigned int max_depth);
        ///added by syj
        void diff_volume_blade(Octnode* current, AptCutterVolume* vol, unsigned int max_depth, Octree* octree);
        void diff_volume_blade_multi_gpu(Octnode* current, AptCutterVolume* vol, unsigned int max_depth);
    private:
        void clean_outside_nodes(Octnode* current, AptCutterVolume* vol);
        // 获取叶节点
        void get_leaf_nodes_diff(Octnode* current, std::vector<Octnode*>& nodes_to_process, const Volume* vol, unsigned int max_depth);
        struct float2 {
            float x;
            float y;
        };
        float pointToSegmentDist(float2 p, float2 a, float2 b);
        bool pointInTriangle(float2 a, float2 b, float2 c, float2 p);
        void get_leaf_nodes_diff_blade(Octnode* current, std::vector<Octnode*>& nodes_to_process, AptCutterVolume* vol, unsigned int max_depth, std::chrono::duration<double>& elapsed, int blade_id);
        void get_leaf_nodes_diff_blade_second(Octnode* current, std::vector<Octnode*>& nodes_to_process, AptCutterVolume* vol, unsigned int max_depth, std::chrono::duration<double>& elapsed);
        void get_leaf_nodes_sum(Octnode* current, std::vector<Octnode*>& nodes_to_process, const Volume* vol, unsigned int max_depth);

        // 设备数量
        int deviceCount;
        std::chrono::duration<double> total_elapsed; // 新增总耗时记录

        // 其他私有成员变量和方法（如果有的话）
    };

} // end namespace cutsim
#endif // CUDA_DIFF_VOLUME_HPP
