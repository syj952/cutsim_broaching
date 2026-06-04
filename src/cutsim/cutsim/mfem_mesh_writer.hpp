#ifndef MFEM_MESH_WRITER_HPP
#define MFEM_MESH_WRITER_HPP

#include <array>
#include <map>
#include <string>
#include <vector>

#include "octree.hpp"

namespace cutsim {

class MfemMeshWriter {
public:
    explicit MfemMeshWriter(Octree& octree);

    void exportMesh(const std::string& meshFile, std::vector<GLVertex*>& normalvertices);

private:
    using HangingVertexInfo = Octree::HangingVertexInfo;
    using VertexPositionInfo = Octree::VertexPositionInfo;
    using NodeCacheKey = Octree::NodeCacheKey;
    using NodeCacheValue = Octree::NodeCacheValue;

    Octree& octree;
    Octnode* root;
    std::map<NodeCacheKey, NodeCacheValue>& node_cache;
    std::array<std::array<double, 2>, 3>& xyzconstraints;

    void boundary(std::vector<std::vector<int>>& boundaryFaces,
                  std::vector<GLVertex*>& normalVertexList,
                  std::vector<Octnode*>& boundarynode);
    void get_hangingVertex(std::vector<GLVertex*>& hangingVertexList,
                           std::vector<GLVertex*>& normalVertexList);
    void get_hanging_vertex_parent();
    void export_mesh_to_file(std::vector<GLVertex*>& normalvertices,
                             std::vector<std::vector<int>>& boundaryFaces,
                             std::vector<GLVertex*>& hanging_vertices,
                             const std::string& filename);
    void clearVertexStates();
    void clearVertexStates(Octnode* current);

    void get_leaf_nodes2(Octnode* current, std::vector<Octnode*>& nodelist) const
    {
        octree.get_leaf_nodes2(current, nodelist);
    }

    std::vector<std::vector<int>> find_parent_nodes(int deep, int idxx, int idxy, int idxz)
    {
        return octree.find_parent_nodes(deep, idxx, idxy, idxz);
    }

    Octnode* find_target_node(Octnode* searchRoot, int deep, int idxx, int idxy, int idxz)
    {
        return octree.find_target_node(searchRoot, deep, idxx, idxy, idxz);
    }

    bool check_hanging_vertex(Octnode* searchRoot, int deep, int idxx, int idxy, int idxz, int index, int i)
    {
        return octree.check_hanging_vertex(searchRoot, deep, idxx, idxy, idxz, index, i);
    }

    void remove_duplicate_vertex(Octnode* node, Octnode* searchRoot, int vertex_idx)
    {
        octree.remove_duplicate_vertex(node, searchRoot, vertex_idx);
    }

    void transfer_vertex_id_upward(Octnode* node, Octnode* searchRoot, int vertex_id, int i)
    {
        octree.transfer_vertex_id_upward(node, searchRoot, vertex_id, i);
    }

    HangingVertexInfo find_hanging_vertex_parent_node(Octnode* node, int vertex_idx)
    {
        return octree.find_hanging_vertex_parent_node(node, vertex_idx);
    }

    VertexPositionInfo determine_hanging_vertex_position(const HangingVertexInfo& info)
    {
        return octree.determine_hanging_vertex_position(info);
    }
};

} // namespace cutsim

#endif
