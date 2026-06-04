#pragma once

#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <iostream>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Wire.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Tool.hxx>
#include<vector>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include "Msg.h"
#include <GC_MakePlane.hxx>
#include <TopoDS_Solid.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include "OCCT_ShapeList.h"
//@brief OCCT图形操作类，提供相应的opencascade图形操作的功能，封装成静态方法供外部调用
class OCCT_GraphOperations
{
private:
    OCCT_GraphOperations(void) {};
    ~OCCT_GraphOperations(void) {};
public:
    //@brief 在复合曲线上均匀采样点，在每个edge上均匀采样，返回点和切向量分别存储
    static void SampleWireUniformly(std::vector<gp_Pnt>& pointVec, std::vector<gp_Dir>& dirVec, TopoDS_Wire& wire, int numPoints); // 方法1：在每个edge上均匀采样，返回点和切向量分别存储
    //@brief 在复合曲线上均匀采样点，在整个复合曲线上均匀采样，返回点和切向量对的列表
    static void SampleWireUniformly(std::vector<std::pair<gp_Pnt, gp_Vec>>& results, TopoDS_Wire& wire, int numPoints);// 方法2：在整个复合曲线上均匀采样，返回点和切向量对的列表
    //@brief 计算复合曲线总长度
    static Standard_Real ComputeTotalLength(const TopoDS_Wire& wire);

    //@brief 根据长度在复合曲线上获取点
    static void GetPointAtLength(const TopoDS_Wire& wire, Standard_Real length, gp_Pnt& pnt, gp_Dir& dir);

    //@brief 在指定边上根据长度比例获取点
    static void GetPointOnEdge(const TopoDS_Edge& edge, Standard_Real length, gp_Pnt& pnt, gp_Vec& vec);

    //@brief 计算单条边的长度
    static Standard_Real ComputeEdgeLength(const TopoDS_Edge& edge);

    //@brief 连接多个边并返回复合曲线
    static TopoDS_Wire ConnectEdges(const std::vector<TopoDS_Edge>& edges);

    static TopoDS_Wire OCCT_GraphOperations::ConnectDisorderEdges(const std::vector<TopoDS_Edge>& edgeList);
    static std::vector<TopoDS_Wire> ConnectDisorderEdgesToWires(const std::vector<TopoDS_Edge>& edgeList);

    // @brief 检查Wire是否具有一致的方向
    static bool IsWireConsistentlyOriented(const TopoDS_Wire& wire);

    // @brief 修复复合曲线的方向性问题
    static TopoDS_Wire FixWireOrientation(const TopoDS_Wire& wire);

    //@brief将曲线拟合成圆弧  (useless)
    static bool FitBSplineToArc(
        const BRepAdaptor_Curve& adaptorCurve,
        gp_Pnt& center,
        double& radius,
        gp_Pnt& startPoint,
        gp_Pnt& endPoint
    );

    //@brief判断是否为圆弧并输出圆心、半径、起点、终点 
    static bool IsCirAcr(const TopoDS_Edge& edge, gp_Pnt& center, double& radius, gp_Pnt& startPoint, gp_Pnt& endPoint);

    //@brief 计算向量与Edge之间最小夹角
    static double ComputeMinAngle(const gp_Vec& theVec, const TopoDS_Edge& theEdge, double tolerance = 1e-6);

    static double ComputeAngle(const gp_Vec& v1, const gp_Vec& v2);

	//@brief 根据节点坐标和单元信息创建OCC网格形状
	//@param nodeCoords 节点坐标数组，格式为 [x1, y1, z1, x2, y2, z2, ...]
	//@param elemTypes 单元类型数组，每个元素对应一个单元的类型标识符
	//@param elemNodeTags 单元节点标签数组，每个元素为一个单元的节点索引列表
    static TopoDS_Shape CreatOCCMeshShape(const std::vector<double>& nodeCoords,
        const std::vector<int>& elemTypes,
        const std::vector<std::vector<std::size_t>>& elemNodeTags);

    // @brief 打印网格信息 
    // @param NodeNum 节点数量
    // @param elemTypes 单元类型数组
    // @param elemTags 单元节点索引数组
    static void PrintMeshInfo(int NodeNum, std::vector<int> elemTypes, std::vector<std::vector<std::size_t>> elemTags);

    // @brief 根据四个顶点创建四面体实体
	static TopoDS_Solid CreateTetrahedronSolid(const gp_Pnt& p0, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3);

    // @brief 根据八个顶点创建六面体实体
    static TopoDS_Solid CreateHexahedronSolid(const std::vector<TopoDS_Vertex>& points);

    //@brief 根据施加的边界条件形状列表，找到关联的节点坐标和节点ID
    static void AssociateNodes(OCCT_ShapeList shapes, std::vector<double>& nodeCoords, std::vector<int>& nodeIds, std::vector<gp_Pnt>& pnts);

    static bool IsPointOnFace(const TopoDS_Face& theFace,
        const gp_Pnt& thePnt,
        const Standard_Real theTol = Precision::Confusion());
};
