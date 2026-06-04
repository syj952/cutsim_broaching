/*
 *  Copyright 2010-2011 Anders Wallin (anders.e.e.wallin "at" gmail.com)
 *  Copyright 2015      Kazuyasu Hamada (k-hamada@gifu-u.ac.jp)
 *
 *  This file is part of Cutsim / OpenCAMlib.
 *
 *  OpenCAMlib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCAMlib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenCAMlib.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cutsim.hpp"
#include "volume.hpp"
#include "octnode.hpp"
namespace cutsim {

    Cutsim::Cutsim(double octree_size, unsigned int octree_max_depth, GLVertex* octree_center, GLData* gld, GLWidget* wid) : g(gld), widget(wid) {
        tree = new Octree(octree_size, octree_max_depth, octree_center, g);
        std::cout << "Cutsim() ctor: tree before init: " << tree->str() << "\n";
        tree->init(6);
        tree->debug = false;
        wid->setTree(tree);
        std::cout << "Cutsim() ctor: tree after init: " << tree->str() << "\n";
#ifndef WIRE_FRAME
        iso_algo = new MarchingCubes(g, tree);
#else
        iso_algo = new CubeWireFrame(g, tree);
#endif
    }

    Cutsim::~Cutsim() {
        delete iso_algo;
        delete tree;
        delete g;
    }

    void Cutsim::updateGL() {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        iso_algo->updateGL();
        g->swap();
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp updateGL()    :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    //表面可视化
    void Cutsim::renderNodesGL() {
        //g->clear(); // 直接清空GLData缓冲区

        // 添加安全类型转换
        MarchingCubes* mc = dynamic_cast<MarchingCubes*>(iso_algo);
        if (!mc) {
            qDebug() << "Error: iso_algo is not MarchingCubes type";
            return;
        }

        for (Octnode* node : nodes_to_process) {
            if (node->isLeaf() && node->is_undecided()) {
                // 调用新增的公共方法
                mc->renderNode(node);
            }
        }

        step_vertexMap.clear();
        // 新增数据转移代码
        for (auto& [node, vertices] : mc->vertexMap) {
            // 使用insert_or_assign实现覆盖写入
            all_vertexMap.insert_or_assign(node, vertices);
            step_vertexMap.insert_or_assign(node, vertices);
        }

        mc->vertexMap.clear();

        g->swap();
        //    widget->update();
    }

    //void Cutsim::sum_volume( const Volume* volume ) {
    void Cutsim::sum_volume(Volume* volume) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->sum(volume);
        //tree->check_node( volume );
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp sum_volume()  :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    /// added hust
    void Cutsim::sum_volume_cuda(const Volume* volume, double max_depth) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->cuda_sum(volume, max_depth);
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp sum_volume_cuda()  :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::diff_volume_cuda(const Volume* volume, double max_depth) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->cuda_diff(volume, max_depth);
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp diff_volume_cuda()  :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::sum_stl_volume_cuda(const StlVolume* volume, double max_depth) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->cuda_sum_stl(volume, max_depth);
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp cuda_sum_stl()  :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::broaching_diff_volume_blade_cuda(broaching_AptCutterVolume* volume) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->broaching_cuda_blade_diff(volume);
        nodes_to_process.clear();
        nodes_to_process = tree->nodes_to_process;
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp diff_volume_blade_cuda() :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::milling_diff_volume_blade_cuda(milling_AptCutterVolume* volume) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->milling_cuda_blade_diff(volume);
        nodes_to_process.clear();
        nodes_to_process = tree->nodes_to_process;
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp diff_volume_blade_cuda() :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::digitaltwin_milling_diff_volume_blade_cuda(digitaltwin_AptCutterVolume* volume) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->digitaltwin_milling_cuda_blade_diff(volume);
        nodes_to_process.clear();
        nodes_to_process = tree->nodes_to_process;
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp diff_volume_blade_cuda() :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::diff_volume(const Volume* volume) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->diff(volume);
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp diff_volume() :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::intersect_volume(const Volume* volume) {
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
        tree->intersect(volume);
        stop = std::chrono::system_clock::now();
        qDebug() << "cutsim.cpp intersect_volume() :" << std::chrono::duration<double>(stop - start).count() << "sec.";
    }

    void Cutsim::treeTransfer(GLVertex parallel, int flip_axis, bool ignore_parts) {
        tree->treeTransfer(parallel, flip_axis, ignore_parts);
    }

    void Cutsim::clearSim(double octree_size, unsigned int octree_max_depth, GLVertex* octree_center) {
        tree->clearTree(octree_size, octree_max_depth, octree_center);
        tree->init(2u);
    }
} // end namespace
