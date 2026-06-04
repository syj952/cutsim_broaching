#include "mfem_mesh_writer.hpp"

#include <boost/foreach.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace cutsim {

MfemMeshWriter::MfemMeshWriter(Octree& octree)
    : octree(octree),
      root(octree.root),
      node_cache(octree.node_cache),
      xyzconstraints(octree.xyzconstraints)
{
}

void MfemMeshWriter::exportMesh(const std::string& meshFile, std::vector<GLVertex*>& normalvertices)
{
    clearVertexStates();
    std::cout << "start numbering..." << std::endl;

    std::vector<GLVertex*> hanging_vertices;
    get_hangingVertex(hanging_vertices, normalvertices);
    std::cout << "hanging vertex: " << hanging_vertices.size() << std::endl;
    std::cout << "normal vertex: " << normalvertices.size() << std::endl;

    get_hanging_vertex_parent();

    std::vector<std::vector<int>> boundaryFaces;
    std::vector<Octnode*> boundarynode;
    boundary(boundaryFaces, normalvertices, boundarynode);
    std::cout << "boundary node: " << boundarynode.size() << std::endl;

    export_mesh_to_file(normalvertices, boundaryFaces, hanging_vertices, meshFile);
}

void MfemMeshWriter::boundary(std::vector<std::vector<int>>& boundaryFaces, std::vector<GLVertex*>& normalVertexList, std::vector<Octnode*>& boundarynode)
    {
        std::vector<Octnode*> leaf_nodes;
        get_leaf_nodes2(root, leaf_nodes);

        BOOST_FOREACH(Octnode * node, leaf_nodes)
        {
            int index = node->idx;
            int deep = node->depth;
            int idxx = node->indexs->x;
            int idxy = node->indexs->y;
            int idxz = node->indexs->z;
            bool hasboundaryface = false;
            for (int i = 0; i < 6; i++)
            {
                switch (i)
                {
                case 0:
                {
                    std::vector<std::vector<int>> parents_data1 = find_parent_nodes(deep, idxx, idxy - 1, idxz);
                    bool is_boundary = false;
                    for (int j = 0; j < parents_data1.size(); j++) {
                        if (find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->isLeaf() && find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->is_outside()) {
                            is_boundary = true; // 周围存在比他大的outside节点，是边界
                            break;
                        }
                    }
                    if (is_boundary) {
                        std::vector<int> vertex;
                        hasboundaryface = true;
                        // 添加顶点0的ID
                        if (node->vertex[0]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[0]->id);
                        else
                            vertex.push_back(node->vertex[0]->id + normalVertexList.size() - 1);

                        // 添加顶点1的ID
                        if (node->vertex[1]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[1]->id);
                        else
                            vertex.push_back(node->vertex[1]->id + normalVertexList.size() - 1);

                        // 添加顶点4的ID
                        if (node->vertex[5]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[5]->id);
                        else
                            vertex.push_back(node->vertex[5]->id + normalVertexList.size() - 1);

                        // 添加顶点5的ID
                        if (node->vertex[4]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[4]->id);
                        else
                            vertex.push_back(node->vertex[4]->id + normalVertexList.size() - 1);

                        boundaryFaces.push_back(vertex);
                    }

                    break;
                }


                case 1:
                {
                    std::vector<std::vector<int>> parents_data1 = find_parent_nodes(deep, idxx + 1, idxy, idxz);
                    bool is_boundary = false;
                    for (int j = 0; j < parents_data1.size(); j++) {
                        if (find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->isLeaf() && find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->is_outside()) {
                            is_boundary = true; // 存在父节点，不是边界
                            break;
                        }
                    }
                    if (is_boundary) {
                        std::vector<int> vertex;
                        hasboundaryface = true;
                        // 添加顶点0的ID
                        if (node->vertex[0]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[0]->id);
                        else
                            vertex.push_back(node->vertex[0]->id + normalVertexList.size() - 1);

                        // 添加顶点3的ID
                        if (node->vertex[3]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[3]->id);
                        else
                            vertex.push_back(node->vertex[3]->id + normalVertexList.size() - 1);

                        // 添加顶点7的ID
                        if (node->vertex[7]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[7]->id);
                        else
                            vertex.push_back(node->vertex[7]->id + normalVertexList.size() - 1);

                        // 添加顶点4的ID
                        if (node->vertex[4]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[4]->id);
                        else
                            vertex.push_back(node->vertex[4]->id + normalVertexList.size() - 1);
                        //std::cout<<"我是右边边界"<<node->indexs->x<<std::endl;
                        boundaryFaces.push_back(vertex);
                    }

                    break;
                }
                case 2:
                {
                    std::vector<std::vector<int>> parents_data1 = find_parent_nodes(deep, idxx, idxy + 1, idxz);
                    bool is_boundary = false;
                    for (int j = 0; j < parents_data1.size(); j++) {
                        if (find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->isLeaf() && find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->is_outside()) {
                            is_boundary = true; // 存在父节点，不是边界
                            break;
                        }
                    }
                    if (is_boundary) {
                        std::vector<int> vertex;
                        hasboundaryface = true;
                        // 添加顶点2的ID
                        if (node->vertex[2]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[2]->id);
                        else
                            vertex.push_back(node->vertex[2]->id + normalVertexList.size() - 1);

                        // 添加顶点3的ID
                        if (node->vertex[3]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[3]->id);
                        else
                            vertex.push_back(node->vertex[3]->id + normalVertexList.size() - 1);

                        // 添加顶点7的ID
                        if (node->vertex[7]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[7]->id);
                        else
                            vertex.push_back(node->vertex[7]->id + normalVertexList.size() - 1);

                        // 添加顶点6的ID
                        if (node->vertex[6]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[6]->id);
                        else
                            vertex.push_back(node->vertex[6]->id + normalVertexList.size() - 1);

                        boundaryFaces.push_back(vertex);
                    }

                    break;
                }
                case 3:
                {
                    std::vector<std::vector<int>> parents_data1 = find_parent_nodes(deep, idxx - 1, idxy, idxz);
                    bool is_boundary = false;
                    for (int j = 0; j < parents_data1.size(); j++) {
                        if (find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->isLeaf() && find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->is_outside()) {
                            is_boundary = true; // 存在父节点，不是边界
                            break;
                        }
                    }
                    if (is_boundary) {
                        std::vector<int> vertex;
                        hasboundaryface = true;
                        // 添加顶点1的ID
                        if (node->vertex[1]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[1]->id);
                        else
                            vertex.push_back(node->vertex[1]->id + normalVertexList.size() - 1);

                        // 添加顶点2的ID
                        if (node->vertex[2]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[2]->id);
                        else
                            vertex.push_back(node->vertex[2]->id + normalVertexList.size() - 1);

                        // 添加顶点6的ID
                        if (node->vertex[6]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[6]->id);
                        else
                            vertex.push_back(node->vertex[6]->id + normalVertexList.size() - 1);

                        // 添加顶点5的ID
                        if (node->vertex[5]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[5]->id);
                        else
                            vertex.push_back(node->vertex[5]->id + normalVertexList.size() - 1);
                        //std::cout<<"我是左边边界"<<node->indexs->x<<std::endl;
                        boundaryFaces.push_back(vertex);
                    }

                    break;
                }
                case 4:
                {
                    std::vector<std::vector<int>> parents_data1 = find_parent_nodes(deep, idxx, idxy, idxz + 1);
                    bool is_boundary = false;
                    for (int j = 0; j < parents_data1.size(); j++) {
                        if (find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->isLeaf() && find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->is_outside()) {
                            is_boundary = true; // 存在父节点，不是边界
                            break;
                        }
                    }
                    if (is_boundary) {
                        std::vector<int> vertex;
                        hasboundaryface = true;
                        // 添加顶点4的ID
                        if (node->vertex[4]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[4]->id);
                        else
                            vertex.push_back(node->vertex[4]->id + normalVertexList.size() - 1);

                        // 添加顶点5的ID
                        if (node->vertex[5]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[5]->id);
                        else
                            vertex.push_back(node->vertex[5]->id + normalVertexList.size() - 1);

                        // 添加顶点6的ID
                        if (node->vertex[6]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[6]->id);
                        else
                            vertex.push_back(node->vertex[6]->id + normalVertexList.size() - 1);

                        // 添加顶点7的ID
                        if (node->vertex[7]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[7]->id);
                        else
                            vertex.push_back(node->vertex[7]->id + normalVertexList.size() - 1);
                        //std::cout << "我是上边边界" << node->indexs->z << std::endl;
                        boundaryFaces.push_back(vertex);

                    }

                    break;
                }
                case 5:
                {
                    std::vector<std::vector<int>> parents_data1 = find_parent_nodes(deep, idxx, idxy, idxz - 1);
                    bool is_boundary = false;
                    for (int j = 0; j < parents_data1.size(); j++) {
                        if (find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->isLeaf() && find_target_node(root, parents_data1[j][0], parents_data1[j][1], parents_data1[j][2], parents_data1[j][3])->is_outside()) {
                            is_boundary = true; // 存在父节点，不是边界
                            break;
                        }
                    }
                    if (is_boundary) {
                        std::vector<int> vertex;
                        hasboundaryface = true;
                        // 添加顶点0的ID
                        if (node->vertex[0]->id <= 0) // 正在常顶点
                            vertex.push_back(-node->vertex[0]->id);
                        else
                            vertex.push_back(node->vertex[0]->id + normalVertexList.size() - 1);

                        // 添加顶点1的ID
                        if (node->vertex[1]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[1]->id);
                        else
                            vertex.push_back(node->vertex[1]->id + normalVertexList.size() - 1);

                        // 添加顶点2的ID
                        if (node->vertex[2]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[2]->id);
                        else
                            vertex.push_back(node->vertex[2]->id + normalVertexList.size() - 1);

                        // 添加顶点3的ID
                        if (node->vertex[3]->id <= 0) // 正常顶点
                            vertex.push_back(-node->vertex[3]->id);
                        else
                            vertex.push_back(node->vertex[3]->id + normalVertexList.size() - 1);

                        boundaryFaces.push_back(vertex);
                        //std::cout<<"我是底面"<<node->indexs->z<<std::endl;
                    }

                    break;
                }



                break;
                }
            }
            if (hasboundaryface)
            {
                boundarynode.push_back(node);
            }
        }
    }
void MfemMeshWriter::get_hangingVertex(std::vector< GLVertex*>& hangingVertexList, std::vector< GLVertex*>& normalVertexList)//针对整个树进行悬挂节点判断
    {
        node_cache.clear();
        std::vector<Octnode*> leaf_node_list;  // 存储叶子节点的列表
        get_leaf_nodes2(root, leaf_node_list);  // 递归获取叶子节点
        Octnode* root = this->root;
        int hangingVertexId = 1;  // 悬挂节点从1开始的正数
        int normalVertexId = 0;
        BOOST_FOREACH(Octnode * node, leaf_node_list)//遍历所有叶子节点
        {
            int index = node->idx;
            int deep = node->depth;
            int idxx = node->indexs->x;
            int idxy = node->indexs->y;
            int idxz = node->indexs->z;
            for (int i = 0; i < 8; i++)
            {
                if (node->vertexnotsaved[i])
                { //Vertex[i] = node.center + Direction[i] * node.scale;//修改
                    switch (index)
                    {//0-7
                    case 0:

                        switch (i)
                        {
                        case 0://0号octnode的0号顶点
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == root->scale && node->vertex[i]->y == -root->scale && node->vertex[i]->z == -root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[0]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }
                                else

                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    {
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[0]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                }


                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;

                        case 1://0号octnode的1号顶点，这是一个新生点，去重+判断悬挂

                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                node->vertex[1]->setId(hangingVertexId++);
                                hangingVertexList.push_back(node->vertex[1]);//悬挂节点存储
                                remove_duplicate_vertex(node, root, i);//去重+id复制
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);

                            }
                            else
                            {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                node->vertex[1]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                remove_duplicate_vertex(node, root, i);
                                node->vertexnotsaved[i] = 0;
                                normalVertexList.push_back(node->vertex[1]);//非悬挂节点存储
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);

                            }

                            break;

                        case 2://这是一个面悬挂节点
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[2]);
                                node->vertex[2]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);

                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[2]);
                                node->vertex[2]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }

                            break;

                        case 3://边悬挂节点
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);

                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }

                            break;

                        case 4:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }

                            break;

                        case 5://面
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }

                            break;

                        case 6://中心点
                            normalVertexList.push_back(node->vertex[i]);
                            node->vertex[i]->setId(normalVertexId--);
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);

                            break;

                        case 7://面
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }

                            break;
                        }
                        break;


                    case 1:
                        switch (i)
                        {
                        case 0: // 边悬挂
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                remove_duplicate_vertex(node, root, i);    // 然后进行去重
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                remove_duplicate_vertex(node, root, i);   // 然后进行去重
                                node->vertexnotsaved[i] = 0;
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;


                        case 1://d角点
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == -root->scale && node->vertex[i]->y == -root->scale && node->vertex[i]->z == -root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }

                                else
                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    {
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);//去重+id复制
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                }

                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;

                        case 2: // 边
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 3: // 面
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 4: // 面
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 5: // 边
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 6:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 7:
                            node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                            normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);  // 然后进行去重
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            break;
                        }
                        break;

                    case 2:
                        switch (i)
                        {
                        case 0: // 面
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 1: // 边
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 2:
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == -root->scale && node->vertex[i]->y == root->scale && node->vertex[i]->z == -root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }
                                else
                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    {
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);//去重+id复制
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                }

                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;

                        case 3: // 边
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 4:
                            node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                            normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);  // 然后进行去重
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            break;

                        case 5:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 6:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i)) {
                                node->vertex[i]->setId(hangingVertexId++);  // 先设置 ID
                                hangingVertexList.push_back(node->vertex[i]);  // 存储悬挂节点
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else {
                                node->vertex[i]->setId(normalVertexId--);  // 先设置 ID
                                normalVertexList.push_back(node->vertex[i]);  // 存储正常节点
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);  // 然后进行去重
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 7:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        }
                        break;


                    case 3:
                        switch (i)
                        {
                        case 0:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 1:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 2:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 3:
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == root->scale && node->vertex[i]->y == root->scale && node->vertex[i]->z == -root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }
                                else
                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    {
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);//去重+id复制
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                }

                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;

                        case 4:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 5:

                            normalVertexList.push_back(node->vertex[i]);
                            node->vertex[i]->setId(normalVertexId--);
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            break;
                        case 6:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 7:

                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;
                        }
                        break;


                    case 4:
                        switch (i)
                        {

                        case 0:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 1:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 2:

                            normalVertexList.push_back(node->vertex[i]);
                            node->vertex[i]->setId(normalVertexId--);
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            break;

                        case 3:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 4:
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == root->scale && node->vertex[i]->y == -root->scale && node->vertex[i]->z == root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }
                                else
                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    {
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);//去重+id复制
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                }

                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;

                        case 5:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 6:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 7:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        }
                        break;
                    case 5:
                        switch (i)
                        {
                        case 0:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;
                        case 1:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 2:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;
                        case 3:

                            normalVertexList.push_back(node->vertex[i]);
                            node->vertex[i]->setId(normalVertexId--);
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            break;
                        case 4:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;
                        case 5:
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == -root->scale && node->vertex[i]->y == -root->scale && node->vertex[i]->z == root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }
                                else
                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    {
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);//去重+id复制
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                }

                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;


                        case 6:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 7:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;


                        }
                        break;
                    case 6:
                        switch (i)
                        {
                        case 0:

                            normalVertexList.push_back(node->vertex[i]);
                            node->vertex[i]->setId(normalVertexId--);
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            break;
                        case 1:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 2:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 3:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 4:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 5:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 6:
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == -root->scale && node->vertex[i]->y == root->scale && node->vertex[i]->z == root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }
                                else
                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    {
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);//去重+id复制
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }

                                }
                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;

                        case 7:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;
                        }
                        break;

                    case 7:
                        switch (i)
                        {
                        case 0:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 1:

                            normalVertexList.push_back(node->vertex[i]);
                            node->vertex[i]->setId(normalVertexId--);
                            node->vertexnotsaved[i] = 0;
                            remove_duplicate_vertex(node, root, i);
                            transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            break;

                        case 2:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 3:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 4:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 5:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 6:
                            if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                            {
                                hangingVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(hangingVertexId++);
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            else
                            {

                                normalVertexList.push_back(node->vertex[i]);
                                node->vertex[i]->setId(normalVertexId--);
                                node->vertexnotsaved[i] = 0;
                                remove_duplicate_vertex(node, root, i);
                                transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                            }
                            break;

                        case 7:
                            if (node->depth > 1)
                            {
                                if (node->vertex[i]->x == root->scale && node->vertex[i]->y == root->scale && node->vertex[i]->z == root->scale)//判断是不是8个根顶点
                                {
                                    node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                    remove_duplicate_vertex(node, root, i);
                                    node->vertexnotsaved[i] = 0;
                                    normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                    transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                }
                                else
                                {
                                    if (check_hanging_vertex(root, deep, idxx, idxy, idxz, index, i))
                                    { //std::cout<<"悬挂了"<<std::endl;
                                        node->vertex[i]->setId(hangingVertexId++);
                                        hangingVertexList.push_back(node->vertex[i]);//悬挂节点存储
                                        remove_duplicate_vertex(node, root, i);//去重+id复制
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                    else
                                    {
                                        // std::cout<<"一般了"<<std::endl;//顶点去重问题，思路：只需要考虑同级顶点的重复，8个角点全部node->vertexnotsaved[i] = 0;
                                        node->vertex[i]->setId(normalVertexId--); // 先赋值为当前值，然后normalVertexId减少1
                                        remove_duplicate_vertex(node, root, i);
                                        node->vertexnotsaved[i] = 0;
                                        normalVertexList.push_back(node->vertex[i]);//非悬挂节点存储
                                        transfer_vertex_id_upward(node, root, node->vertex[i]->id, i);
                                    }
                                }
                            }
                            //如果是deep=1的角点直接不做任何操作
                            break;
                        }
                        break;

                    }

                }
            }
        }
    }




void MfemMeshWriter::get_hanging_vertex_parent()
    {
        std::vector<Octnode*> leaf_node_list;  // 存储叶子节点的列表
        get_leaf_nodes2(root, leaf_node_list);  // 递归获取叶子节点
        Octnode* root = this->root;

        BOOST_FOREACH(Octnode * node, leaf_node_list) // 遍历所有叶子节点
        {
            int index = node->idx;
            int deep = node->depth;
            int idxx = node->indexs->x;
            int idxy = node->indexs->y;
            int idxz = node->indexs->z;

            for (int i = 0; i < 8; i++)
            {
                if (node->vertexnotsaved[i])
                {
                    switch (index)
                    {
                    case 0:
                        switch (i)
                        {
                        case 0:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);


                        }
                        break;
                        case 1:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[1]->parentVertices.push_back(parentVertex1); // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[1]->parentVertices.push_back(parentVertex2); // 也将另一个父顶点添加
                        }
                        break;
                        case 2:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 3:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 4:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 5:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 6:
                            //std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        case 7:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        }
                        break;

                    case 1:
                        switch (i)
                        {
                        case 0:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 1:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);
                        }
                        break;
                        case 2:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 3:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 4:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 5:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 6:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 7:
                            //std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        }
                        break;

                    case 2:
                        switch (i)
                        {
                        case 0:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 1:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 2:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);
                        }
                        break;
                        case 3:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 4:
                            // std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        case 5:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 6:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 7:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        }
                        break;

                    case 3:
                        switch (i)
                        {
                        case 0:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 1:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 2:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 3:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);
                        }
                        break;
                        case 4:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 5:
                            //std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        case 6:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 7:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        }
                        break;

                    case 4:
                        switch (i)
                        {
                        case 0:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 1:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 2:
                            // std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        case 3:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 4:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);
                        }
                        break;
                        case 5:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 6:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        case 7:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 也将另一个父顶点添加
                        }
                        break;
                        }
                        break;

                    case 5:
                        switch (i)
                        {
                        case 0:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);
                        }
                        break;
                        case 1:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);
                        }
                        break;
                        case 2:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);
                        }
                        break;
                        case 3:
                            //std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        case 4:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);
                        }
                        break;
                        case 5:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);
                        }
                        break;
                        case 6:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);
                        }
                        break;
                        case 7:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);
                        }
                        break;
                        }
                        break;

                    case 6:
                        switch (i)
                        {
                        case 0:
                            //std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        case 1:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[1]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 6, i=1 的处理逻辑
                        }
                        break;
                        case 2:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[2]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 6, i=2 的处理逻辑
                        }
                        break;
                        case 3:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 6, i=3 的处理逻辑
                        }
                        break;
                        case 4:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 6, i=4 的处理逻辑
                        }
                        break;
                        case 5:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[5]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2);  // 这里添加 case 6, i=5 的处理逻辑
                        }
                        break;
                        case 6:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);
                        }
                        break;
                        case 7:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 6, i=7 的处理逻辑
                        }
                        break;
                        }
                        break;

                    case 7:
                        switch (i)
                        {
                        case 0:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[0]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 7, i=0 的处理逻辑
                        }
                        break;
                        case 1:
                            // std::cout<<"遍历到找体心悬挂点的case"<<std::endl;
                            break;
                        case 2:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 7, i=2 的处理逻辑
                        }
                        break;
                        case 3:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[3]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 7, i=3 的处理逻辑
                        }
                        break;
                        case 4:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 7, i=4 的处理逻辑
                        }
                        break;
                        case 5:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[4]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 7, i=5 的处理逻辑
                        }
                        break;
                        case 6:
                        {
                            int parentVertex1 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[7]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex1);  // 将父顶点添加到当前顶点
                            int parentVertex2 = find_target_node(root, deep - 1, idxx / 2, idxy / 2, idxz / 2)->vertex[6]->id;
                            node->vertex[i]->parentVertices.push_back(parentVertex2); // 这里添加 case 7, i=6 的处理逻辑
                        }
                        break;
                        case 7:
                        {
                            HangingVertexInfo info = find_hanging_vertex_parent_node(node, i);
                            VertexPositionInfo info1 = determine_hanging_vertex_position(info);
                            node->vertex[i]->parentVertices.push_back(info1.vertex1_id);
                            node->vertex[i]->parentVertices.push_back(info1.vertex2_id);
                        }
                        break;
                        }
                        break;
                    }
                }
            }
        }
    }
    ///查找顶点


void MfemMeshWriter::export_mesh_to_file(std::vector<GLVertex*>& normalvertices, std::vector<std::vector<int>>& boundaryFaces, std::vector<GLVertex*>& hanging_vertices, const std::string& filename)

    {
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "无法打开文件 " << filename << std::endl;
            return;
        }

        // tital
        out << "MFEM NC mesh v1.0\n";
        out << "dimension\n";
        out << "3\n";


        // elements
        std::vector<Octnode*> leaf_node_list;
        get_leaf_nodes2(root, leaf_node_list);  // 使用 this 而不是 tree

        out << "elements\n" << leaf_node_list.size() << "\n";

        for (Octnode* node : leaf_node_list) {
            out << "0 1 5 0 ";
            int order[] = { 1, 0, 3, 2, 5, 4, 7, 6 };
            for (int i = 0; i < 8; ++i) {
                int idx = order[i];
                if (node->vertex[idx]->id <= 0)
                {
                    //node->vertex[idx]->id=-node->vertex[idx]->id;
                    out << -node->vertex[idx]->id;
                }
                else
                {
                    //node->vertex[idx]->id=node->vertex[idx]->id + normalvertices.size()-1;
                    out << node->vertex[idx]->id + normalvertices.size() - 1;
                }
                if (i < 7) out << " ";
                if (node->vertex[idx]->id == -9999999)
                {
                    std::cout << "有问题node" << " " << node->depth << " " << node->indexs->x << " " << node->indexs->y << " " << node->indexs->z << " " << idx << std::endl;
                }
            }
            out << "\n";
        }

        //  boundary2
         // 预处理：将forceBoundaryFaceVertices转换为哈希集合以快速查找
        auto start_time = std::chrono::high_resolution_clock::now();
        // 预处理：建立顶点到节点的映射，避免重复遍历
        std::unordered_map<int, std::vector<Octnode*>> vertexToNodes;
        double maxz = -1e9;
        for (Octnode* node : leaf_node_list) {
            //if (node->indexs != nullptr && (node->indexs->x == 80 || node->indexs->z == 176)) 
            if (node->indexs != nullptr && (node->center->x >= xyzconstraints[0][0] && node->center->x <= xyzconstraints[0][1]
                && node->center->y >= xyzconstraints[1][0] && node->center->y <= xyzconstraints[1][1]
                && node->center->z >= xyzconstraints[2][0] && node->center->z <= xyzconstraints[2][1]))
            {
                for (int i = 0; i < 8; i++) {
                    int nodeVertexId = node->vertex[i]->id;
                    node->vertex[i]->x;
                    int mappedId = (nodeVertexId <= 0) ? -nodeVertexId : nodeVertexId + normalvertices.size() - 1;
                    vertexToNodes[mappedId].push_back(node);
                }
            }
            if (node->center->z > maxz)maxz = node->center->z;
        }
        // boundary
        out << "boundary\n" << boundaryFaces.size() << "\n";
        for (const auto& face : boundaryFaces) {
            bool isTargetBoundaryFace = false; // 标记是否属于目标约束节点

            // 使用顶点到节点的映射快速找到候选节点
            std::unordered_set<Octnode*> candidateNodes;
            // 找到所有面顶点可能所属的节点
            for (int faceVertexIdx : face) {
                auto it = vertexToNodes.find(faceVertexIdx);
                if (it != vertexToNodes.end()) {
                    for (Octnode* node : it->second) {
                        candidateNodes.insert(node);
                    }
                }
            }

            // 只检查候选节点
            for (Octnode* node : candidateNodes) {
                // 检查这个面的所有顶点是否都属于当前节点
                bool allVerticesInNode = true;
                for (int faceVertexIdx : face) {
                    bool vertexFound = false;
                    for (int i = 0; i < 8; i++) {
                        int nodeVertexId = node->vertex[i]->id;
                        int mappedId = (nodeVertexId <= 0) ? -nodeVertexId : nodeVertexId + normalvertices.size() - 1;
                        if (faceVertexIdx == mappedId) {
                            vertexFound = true;
                            break;
                        }
                    }
                    if (!vertexFound) {
                        allVerticesInNode = false;
                        break;
                    }
                }

                // 如果这个面的4个顶点都在同一个 candidateNode 里，说明它就是我们要找的约束面
                if (allVerticesInNode) {
                    isTargetBoundaryFace = true;
                    break; // 找到了就可以直接跳出节点循环
                }
            }

            // 根据标记输出边界属性
            if (isTargetBoundaryFace) {
                out << "1 3 ";
            }
            else {
                out << "2 3 ";
            }

            // 输出面顶点ID
            for (int idx : face) {
                out << idx << " ";
            }
            out << "\n";
        }
        // 结束计时并输出结果
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        std::cout << "边界面处理耗时: " << duration.count() << " 微秒 ("
            << duration.count() / 1000.0 << " 毫秒)" << std::endl;
        //        ///边界判断
        //        // boundary
        //           out << "boundary\n" << boundaryFaces.size() << "\n";
        //           for (const auto& face : boundaryFaces) {
        //               bool faceProcessed = false;

        //               // 遍历所有叶节点，查找面所属的节点
        //               for (Octnode* node : leaf_node_list) {
        //                   // 检查这个面的所有顶点是否都属于当前节点
        //                   bool allVerticesInNode = true;
        //                   for (int faceVertexIdx : face) {
        //                       bool vertexFound = false;
        //                       for (int i = 0; i < 8; i++) {
        //                           int nodeVertexId = node->vertex[i]->id;
        //                           // 检查顶点ID是否匹配（考虑正负ID）
        //                           if ((nodeVertexId <= 0 && faceVertexIdx == -nodeVertexId) ||
        //                               (nodeVertexId > 0 && faceVertexIdx == nodeVertexId + normalvertices.size() - 1)) {
        //                               vertexFound = true;
        //                               break;
        //                           }
        //                       }
        //                       if (!vertexFound) {
        //                           allVerticesInNode = false;
        //                           break;
        //                       }
        //                   }

        //                   // 如果所有顶点都在这个节点中，检查index->z的值
        //                   if (allVerticesInNode && node->indexs != nullptr) {
        //                       // 检查是否是右面 (底面顶点索引为0,1,2,3)
        //                       bool isBottomFace = true;
        //                       std::vector<int> bottomFaceIndices = {0, 3, 7, 4};
        //                       for (int faceVertexIdx : face) {
        //                           bool foundInBottomFace = false;
        //                           for (int bottomIdx : bottomFaceIndices) {
        //                               int nodeVertexId = node->vertex[bottomIdx]->id;
        //                               if ((nodeVertexId <= 0 && faceVertexIdx == -nodeVertexId) ||
        //                                   (nodeVertexId > 0 && faceVertexIdx == nodeVertexId + normalvertices.size() - 1)) {
        //                                   foundInBottomFace = true;
        //                                   break;
        //                               }
        //                           }
        //                           if (!foundInBottomFace) {
        //                               isBottomFace = false;
        //                               break;
        //                           }
        //                       }

        //                       // 检查是否是左面 (顶面顶点索引为4,5,6,7)
        //                       bool isTopFace = true;
        //                       std::vector<int> topFaceIndices = {1, 2, 6, 5};
        //                       for (int faceVertexIdx : face) {
        //                           bool foundInTopFace = false;
        //                           for (int topIdx : topFaceIndices) {
        //                               int nodeVertexId = node->vertex[topIdx]->id;
        //                               if ((nodeVertexId <= 0 && faceVertexIdx == -nodeVertexId) ||
        //                                   (nodeVertexId > 0 && faceVertexIdx == nodeVertexId + normalvertices.size() - 1)) {
        //                                   foundInTopFace = true;
        //                                   break;
        //                               }
        //                           }
        //                           if (!foundInTopFace) {
        //                               isTopFace = false;
        //                               break;
        //                           }
        //                       }

        //                       // 根据条件输出不同的标识符
        //                       if (node->indexs->x == 170 && isBottomFace) {//右面
        //                           out << "1 3 ";
        //                       } else if (node->indexs->x == 133 && isTopFace) {//左面
        //                           out << "2 3 ";
        //                       } else {
        //                           out << "3 3 ";
        //                       }

        //                       // 输出面的顶点ID
        //                       for (int idx : face) {
        //                           out << idx << " ";
        //                       }
        //                       out << "\n";

        //                       faceProcessed = true;
        //                       break;
        //                   }
        //               }

        //               // 如果没有找到匹配的节点，使用默认输出
        //               if (!faceProcessed) {
        //                   out << "3 3 ";
        //                   for (int idx : face) {
        //                       out << idx << " ";
        //                   }
        //                   out << "\n";
        //               }
        //           }

        // vertex_parents
        out << "vertex_parents\n" << hanging_vertices.size() << "\n";
        for (GLVertex* vertex : hanging_vertices) {
            out << vertex->id + normalvertices.size() - 1 << " ";
            for (int j = 0; j < 2; ++j) {
                if (vertex->parentVertices[j] <= 0)//正常顶点是0，-1，-2。。。
                    out << -vertex->parentVertices[j];
                else
                    out << vertex->parentVertices[j] + normalvertices.size() - 1;//悬挂顶点是1.2.3
                if (j == 0) out << " ";
            }
            out << "\n";
        }

        // coordinates
        out << "coordinates\n";
        out << normalvertices.size() << "\n";
        out << "3\n";
        for (GLVertex* vertex : normalvertices) {
            out << vertex->x << " " << vertex->y << " " << vertex->z << "\n";
        }

        // end
        out << "mfem_mesh_end\n";
        out.close();
        std::cout << "输出完成: " << filename << std::endl;
    }
    /// 清除所有节点顶点的状态，恢复到网格导出前的状态


void MfemMeshWriter::clearVertexStates() {
        clearVertexStates(root);
        std::cout << "已清除所有节点顶点状态" << std::endl;
    }

    /// 递归清除节点顶点状态的实现

void MfemMeshWriter::clearVertexStates(Octnode* current) {
        if (!current) return;

        // 清除当前节点的8个顶点状态
        for (int n = 0; n < 8; ++n) {
            if (current->vertex[n]) {
                // 重置顶点ID为初始值
                current->vertex[n]->id = -9999999;
                // 清空父顶点列表
                current->vertex[n]->parentVertices.clear();
            }
            // 重置顶点遍历状态
            current->vertexnotsaved[n] = 1;
        }

        // 递归处理子节点
        for (int n = 0; n < 8; ++n) {
            if (current->child[n]) {
                clearVertexStates(current->child[n]);
            }
        }
    }


} // namespace cutsim
