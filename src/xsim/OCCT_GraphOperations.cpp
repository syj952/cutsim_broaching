#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <iostream>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Wire.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include "OCCT_GraphOperations.h"
#include <gp_Pnt.hxx>
#include <GeomConvert.hxx>
#include <Standard_Type.hxx>
#include <Geom_Circle.hxx>
#include <vector>
#include <gp_Dir.hxx>
#include "Msg.h"
#include <BRepAdaptor_CompCurve.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include "BRepClass_FaceClassifier.hxx"
#include "BRepExtrema_DistShapeShape.hxx"
#include <algorithm>
#include "ComplainUtf8.h"
#include "OCCT_ShapeList.h"
#include "BRepTools.hxx"
#include <BRep_Tool.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>

//#include "GCPnts_ProjectionOnSurface.hxx"
using namespace std;
// 在复合曲线上均匀采样点
void OCCT_GraphOperations::SampleWireUniformly(std::vector<gp_Pnt>& pointVec,std::vector<gp_Dir>& dirVec, TopoDS_Wire& wire, int numPoints)
{
    //std::vector<gp_Pnt> points;
    if (numPoints < 2)
    {
        return; // 至少需要2个点
    }

    // 验证Wire的方向一致性
    if (!IsWireConsistentlyOriented(wire)) {
        // 如果方向不一致，先修复方向
        wire = FixWireOrientation(wire);
		Msg::ShowWarning(
        "The input composite curve direction is inconsistent, and the direction problem is automatically corrected.");
    }

    Standard_Real totalLength = ComputeTotalLength(wire);
    Standard_Real step = totalLength / (numPoints - 1); // 计算步长
	gp_Pnt point;
	gp_Dir dir;
    Standard_Real currentPosition = 0.0;
	GetPointAtLength(wire, 0.0, point, dir);
    pointVec.push_back(point); // 起点
	dirVec.push_back(dir);

    for (int i = 1; i < numPoints - 1; ++i)
    {
        currentPosition += step;
		GetPointAtLength(wire, currentPosition, point, dir);
        pointVec.push_back(point);
		dirVec.push_back(dir);
    }

	GetPointAtLength(wire, totalLength, point, dir);
    pointVec.push_back(point); // 终点
	dirVec.push_back(dir);

    return;
}



void OCCT_GraphOperations::SampleWireUniformly(std::vector<std::pair<gp_Pnt, gp_Vec>>& results, TopoDS_Wire& wire, int numPoints)
{
    // 将Wire适配为复合曲线
    BRepAdaptor_CompCurve compCurve(wire);

    // 计算总长度并设置均匀采样
    GCPnts_UniformAbscissa sampler;
    sampler.Initialize(compCurve, numPoints, compCurve.FirstParameter(), compCurve.LastParameter());
    

    // 获取采样点参数和位置
    for (int i = 1; i <= sampler.NbPoints(); ++i) {
        double param = sampler.Parameter(i);
        gp_Pnt point;
        gp_Vec tangent;

        // 计算点位置和切向
        compCurve.D1(param, point, tangent);

        // 归一化切向量
        if (tangent.Magnitude() > 1e-7) {
            tangent.Normalize();
        }

        results.emplace_back(point, tangent);
    }

    return;
};

// 计算复合曲线总长度
Standard_Real OCCT_GraphOperations::ComputeTotalLength(const TopoDS_Wire& wire)
{
    Standard_Real totalLength = 0.0;

    // 遍历Wire中的所有Edge
    for (TopExp_Explorer exp(wire, TopAbs_EDGE); exp.More(); exp.Next())
    {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        totalLength += ComputeEdgeLength(edge);
    }

    return totalLength;
}

// 根据长度在复合曲线上获取点
void OCCT_GraphOperations::GetPointAtLength(const TopoDS_Wire& wire, Standard_Real length,gp_Pnt& Pnt,gp_Dir& Dir)
{
    Standard_Real accumulatedLength = 0.0;

    for (TopExp_Explorer exp(wire, TopAbs_EDGE); exp.More(); exp.Next())
    {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        Standard_Real edgeLength = ComputeEdgeLength(edge);

        if (accumulatedLength + edgeLength >= length)
        {
            // 该点在当前边上
            Standard_Real positionOnEdge = length - accumulatedLength;
			gp_Pnt pnt;
			gp_Vec vec;
            GetPointOnEdge(edge, positionOnEdge,pnt,vec);
			Pnt = pnt;
			Dir = gp_Dir(vec);
            return;
        }

        accumulatedLength += edgeLength;
    }

    // 如果长度超过总长度，返回最后一点
    TopoDS_Edge lastEdge;
    for (TopExp_Explorer exp(wire, TopAbs_EDGE); exp.More(); exp.Next())
    {
        lastEdge = TopoDS::Edge(exp.Current());
    }

    BRepAdaptor_Curve curveAdaptor(lastEdge);
    GeomAdaptor_Curve geomAdaptor = curveAdaptor.Curve();
    //gp_Pnt pnt;
    gp_Vec vec;
	geomAdaptor.D1(geomAdaptor.LastParameter(), Pnt, vec);
	Dir = gp_Dir(vec);
}


// 在指定边上根据长度比例获取点
void OCCT_GraphOperations::GetPointOnEdge(const TopoDS_Edge& edge, Standard_Real length, gp_Pnt& pnt, gp_Vec& vec)
{
    BRepAdaptor_Curve curveAdaptor(edge);
    GeomAdaptor_Curve geomAdaptor = curveAdaptor.Curve();

    Standard_Real firstParam = geomAdaptor.FirstParameter();
    Standard_Real lastParam = geomAdaptor.LastParameter();

    // 计算指定长度对应的参数
    GCPnts_AbscissaPoint abscissa(geomAdaptor, length, firstParam);
    if (abscissa.IsDone())
    {
		geomAdaptor.D1(abscissa.Parameter(), pnt, vec);
        return;
    }

    // 如果失败，返回终点
    geomAdaptor.D1(lastParam, pnt, vec);
    return;
}

// 计算单条边的长度
Standard_Real OCCT_GraphOperations::ComputeEdgeLength(const TopoDS_Edge& edge)
{
    BRepAdaptor_Curve curveAdaptor(edge);
    GeomAdaptor_Curve geomAdaptor = curveAdaptor.Curve();

    GCPnts_AbscissaPoint abscissa;
    Standard_Real firstParam = geomAdaptor.FirstParameter();
    Standard_Real lastParam = geomAdaptor.LastParameter();

    // 计算曲线总长度
    return abscissa.Length(geomAdaptor, firstParam, lastParam);
}
#include <ShapeFix_Wire.hxx>
#include <BRepTools_WireExplorer.hxx>
namespace
{
    constexpr Standard_Real kDisorderEdgeConnectTolerance = 1.0e-4;

    bool getWireEndPoints(const TopoDS_Wire& wire, gp_Pnt& firstPoint, gp_Pnt& lastPoint)
    {
        TopoDS_Vertex firstVertex;
        TopoDS_Vertex lastVertex;
        TopExp::Vertices(wire, firstVertex, lastVertex);
        if (firstVertex.IsNull() || lastVertex.IsNull()) {
            return false;
        }

        firstPoint = BRep_Tool::Pnt(firstVertex);
        lastPoint = BRep_Tool::Pnt(lastVertex);
        return true;
    }

    bool wireEndPointsAreClose(const TopoDS_Wire& lhs, const TopoDS_Wire& rhs, Standard_Real tolerance)
    {
        gp_Pnt lhsFirst;
        gp_Pnt lhsLast;
        gp_Pnt rhsFirst;
        gp_Pnt rhsLast;
        if (!getWireEndPoints(lhs, lhsFirst, lhsLast) || !getWireEndPoints(rhs, rhsFirst, rhsLast)) {
            return false;
        }

        return lhsFirst.Distance(rhsFirst) <= tolerance ||
            lhsFirst.Distance(rhsLast) <= tolerance ||
            lhsLast.Distance(rhsFirst) <= tolerance ||
            lhsLast.Distance(rhsLast) <= tolerance;
    }

    void collectWireEdges(const TopoDS_Wire& wire, std::vector<TopoDS_Edge>& edges)
    {
        for (TopExp_Explorer explorer(wire, TopAbs_EDGE); explorer.More(); explorer.Next()) {
            const TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
            if (!edge.IsNull()) {
                edges.push_back(edge);
            }
        }
    }

    std::vector<TopoDS_Wire> connectEdgesToWiresLenient(const std::vector<TopoDS_Edge>& edgeList)
    {
        std::vector<TopoDS_Wire> result;
        if (edgeList.empty()) {
            return result;
        }

        Handle(TopTools_HSequenceOfShape) edges = new TopTools_HSequenceOfShape();
        for (const TopoDS_Edge& edge : edgeList) {
            if (!edge.IsNull()) {
                edges->Append(edge);
            }
        }

        Handle(TopTools_HSequenceOfShape) wires;
        ShapeAnalysis_FreeBounds::ConnectEdgesToWires(
            edges,
            kDisorderEdgeConnectTolerance,
            Standard_False,
            wires);

        if (!wires.IsNull()) {
            for (Standard_Integer i = 1; i <= wires->Length(); ++i) {
                result.push_back(TopoDS::Wire(wires->Value(i)));
            }
        }

        return result;
    }

    std::vector<TopoDS_Wire> mergeNearbyWireFragments(const std::vector<TopoDS_Wire>& wires)
    {
        if (wires.size() < 2) {
            return wires;
        }

        std::vector<int> parent(wires.size());
        for (int i = 0; i < static_cast<int>(parent.size()); ++i) {
            parent[i] = i;
        }

        auto findRoot = [&parent](int value) {
            while (parent[value] != value) {
                parent[value] = parent[parent[value]];
                value = parent[value];
            }
            return value;
        };

        auto unite = [&parent, &findRoot](int lhs, int rhs) {
            const int lhsRoot = findRoot(lhs);
            const int rhsRoot = findRoot(rhs);
            if (lhsRoot != rhsRoot) {
                parent[rhsRoot] = lhsRoot;
            }
        };

        for (int i = 0; i < static_cast<int>(wires.size()); ++i) {
            for (int j = i + 1; j < static_cast<int>(wires.size()); ++j) {
                if (wireEndPointsAreClose(wires[i], wires[j], kDisorderEdgeConnectTolerance)) {
                    unite(i, j);
                }
            }
        }

        std::vector<std::vector<int>> groups(wires.size());
        for (int i = 0; i < static_cast<int>(wires.size()); ++i) {
            groups[findRoot(i)].push_back(i);
        }

        std::vector<TopoDS_Wire> mergedWires;
        for (const std::vector<int>& group : groups) {
            if (group.empty()) {
                continue;
            }

            if (group.size() == 1) {
                mergedWires.push_back(wires[group.front()]);
                continue;
            }

            std::vector<TopoDS_Edge> groupEdges;
            for (int wireIndex : group) {
                collectWireEdges(wires[wireIndex], groupEdges);
            }

            std::vector<TopoDS_Wire> connectedGroup = connectEdgesToWiresLenient(groupEdges);
            if (connectedGroup.empty()) {
                for (int wireIndex : group) {
                    mergedWires.push_back(wires[wireIndex]);
                }
                continue;
            }

            mergedWires.insert(mergedWires.end(), connectedGroup.begin(), connectedGroup.end());
        }

        return mergedWires;
    }
}
// 连接多个边并返回复合曲线
TopoDS_Wire OCCT_GraphOperations::ConnectEdges(const std::vector<TopoDS_Edge>& edges)
{
    if (edges.empty()) {
        return TopoDS_Wire();
    }

    // 1. 先创建初始Wire
    BRepBuilderAPI_MakeWire wireMaker;
    for (const auto& edge : edges) {
        wireMaker.Add(edge);
    }

    if (!wireMaker.IsDone()) {
        // 处理连接失败的情况
        return TopoDS_Wire();
    }

    TopoDS_Wire wire = wireMaker.Wire();

    // 2. 使用ShapeFix_Wire修复和统一方向
    Handle(ShapeFix_Wire) sfw = new ShapeFix_Wire;
    sfw->Load(wire);
    sfw->SetPrecision(1e-5); // 设置合适的精度
    sfw->FixReorder();      // 修复边的顺序
    sfw->FixConnected();    // 修复连接
    sfw->FixDegenerated();  // 修复退化边
    sfw->FixSelfIntersection(); // 修复自相交

    // 执行修复
    if (sfw->Perform()) {
        wire = sfw->Wire();
    }

    return wire;
}
TopoDS_Wire OCCT_GraphOperations::ConnectDisorderEdges(const std::vector<TopoDS_Edge>& edgeList)
{
    const std::vector<TopoDS_Wire> wires = ConnectDisorderEdgesToWires(edgeList);
    if (!wires.empty()) {
        return wires.front();
    }
    return TopoDS_Wire();
}

std::vector<TopoDS_Wire> OCCT_GraphOperations::ConnectDisorderEdgesToWires(const std::vector<TopoDS_Edge>& edgeList)
{
    std::vector<TopoDS_Wire> result = connectEdgesToWiresLenient(edgeList);
    return mergeNearbyWireFragments(result);
}
bool OCCT_GraphOperations::IsWireConsistentlyOriented(const TopoDS_Wire& wire)
{
    if (wire.IsNull()) return false;

    BRepTools_WireExplorer explorer;
    explorer.Init(wire);

    if (!explorer.More()) return false;

    // 获取第一个顶点作为参考
    TopoDS_Vertex firstVertex = explorer.CurrentVertex();
    TopoDS_Vertex currentVertex = firstVertex;

    int edgeCount = 0;
    for (; explorer.More(); explorer.Next(), edgeCount++) {
        TopoDS_Vertex nextVertex = explorer.CurrentVertex();

        // 检查相邻边是否正确连接
        if (currentVertex.IsNull() || nextVertex.IsNull()) {
            return false;
        }

        // 检查顶点是否重合（在容差范围内）
        if (!currentVertex.IsSame(nextVertex)) {
            gp_Pnt p1 = BRep_Tool::Pnt(currentVertex);
            gp_Pnt p2 = BRep_Tool::Pnt(nextVertex);

            if (p1.Distance(p2) > Precision::Confusion()) {
                return false; // 顶点不连接
            }
        }

        currentVertex = nextVertex;
    }

    return (edgeCount > 0); // 至少有一条边
}

TopoDS_Wire OCCT_GraphOperations::FixWireOrientation(const TopoDS_Wire& wire)
{
    if (wire.IsNull()) return wire;

    // 使用ShapeAnalysis_Wire分析Wire
    Handle(ShapeAnalysis_Wire) analyzer = new ShapeAnalysis_Wire;
    analyzer->Load(wire);

    // 检查并修复方向
    if (analyzer->CheckOrder()) {
        // Wire方向基本正确，直接返回
        return wire;
    }

    // 需要重新构建Wire以确保方向一致
    BRepTools_WireExplorer explorer;
    explorer.Init(wire);

    BRepBuilderAPI_MakeWire newWireMaker;

    // 按照正确顺序添加边
    for (; explorer.More(); explorer.Next()) {
        newWireMaker.Add(explorer.Current());
    }

    if (newWireMaker.IsDone()) {
        return newWireMaker.Wire();
    }

    return wire; // 修复失败，返回原Wire
}

#include <Geom_TrimmedCurve.hxx>
#include "Msg.h"
#include <GC_MakeCircle.hxx>
bool OCCT_GraphOperations::FitBSplineToArc(
    const BRepAdaptor_Curve& adaptorCurve,
    gp_Pnt& center,
    double& radius,
    gp_Pnt& startPoint,
    gp_Pnt& endPoint){
    // 1. 提取底层的 B 样条曲线
    Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(adaptorCurve.Curve().Curve());
    if (bspline.IsNull()) return false;

    // 2. 尝试拟合为圆弧 (Geom_Circle)
    Handle(Geom_Curve) resultCurve = GeomConvert::CurveToBSplineCurve(bspline);
    if (resultCurve.IsNull() || resultCurve->DynamicType() == STANDARD_TYPE(Geom_Circle)) {     
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(resultCurve);
        center = circle->Position().Location();
        radius = circle->Radius();
        double uStart = adaptorCurve.FirstParameter();
        double uEnd = adaptorCurve.LastParameter();
        startPoint = circle->Value(uStart);
        endPoint = circle->Value(uEnd);
        return true;
    }
    else if(resultCurve->DynamicType() == STANDARD_TYPE(Geom_TrimmedCurve)) {
        // 在拟合后添加圆弧段检查
        Handle(Geom_TrimmedCurve) trimmed = Handle(Geom_TrimmedCurve)::DownCast(resultCurve);
        // 检查底层是否为圆
        if (trimmed->BasisCurve()->DynamicType() == STANDARD_TYPE(Geom_Circle)) {
            Handle(Geom_Circle) baseCircle = Handle(Geom_Circle)::DownCast(trimmed->BasisCurve());
            center = baseCircle->Position().Location();
            radius = baseCircle->Radius();
            // 直接获取修剪曲线的端点
            startPoint = trimmed->StartPoint();
            endPoint = trimmed->EndPoint();
            return true;
        }
    }
    else if(resultCurve->DynamicType() == STANDARD_TYPE(Geom_BSplineCurve))
    {
		Msg::ShowInfo("拟合结果为 B 样条曲线，无法转换为圆弧。");
        return false;
    }
}

bool OCCT_GraphOperations::IsCirAcr(const TopoDS_Edge& edge, gp_Pnt& center, double& radius, gp_Pnt& startPoint, gp_Pnt& endPoint)
{
	Standard_Real first, last;
	Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) return false;

    // 检查是否是修剪曲线
    Handle(Geom_TrimmedCurve) trimmedCurve = Handle(Geom_TrimmedCurve)::DownCast(curve);
    if (!trimmedCurve.IsNull())
    {
        curve = trimmedCurve->BasisCurve(); // 获取修剪曲线的基础曲线
		Handle(Geom_Circle) baseCircle = Handle(Geom_Circle)::DownCast(curve);
        center = baseCircle->Position().Location();
        radius = baseCircle->Radius();
        // 直接获取修剪曲线的端点
        startPoint = trimmedCurve->StartPoint();
        endPoint = trimmedCurve->EndPoint();
        return true;
    }

    // 检查是否是圆 ,是的话直接返回
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(curve);
    if (!circle.IsNull())
    {
        const gp_Circ& circ = circle->Circ();
        center = circ.Location();
        radius = circ.Radius();
        return true;
    }

	Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(curve);
	if (bspline.IsNull()) return false;
    else {                                                          
        double u0 = curve->FirstParameter();
        double u1 = curve->LastParameter();
        double u_mid = (u0 + u1) * 0.5;

        gp_Pnt P0 = curve->Value(u0);
        gp_Pnt P1 = curve->Value(u_mid);
        gp_Pnt P2 = curve->Value(u1);

        GC_MakeCircle makeCirc(P0, P1, P2);
        if (!makeCirc.IsDone()) return false;

        Handle(Geom_Circle) fittedCircle = makeCirc.Value();
        gp_Circ circ = fittedCircle->Circ();
        center = circ.Location();
        radius = circ.Radius();
        startPoint = P0;
        endPoint = P2;
        // 检查拟合的圆是否与测试点接近
        //gp_Pnt circlePoint = fittedCircle->Value(u_test);

		int SamplingNum = 10;// 采样点数量
        for (double i = 1; i < SamplingNum; i++)
        {
			double u_test = u0 + (u1 - u0) * (i / SamplingNum);
            gp_Pnt testPoint = curve->Value(u_test);
            if (testPoint.Distance(center) - radius > 1e-2) return false;
        }

        return true; // 如果不是圆弧或无法拟合，返回 false
    }

}

#include <BRep_Tool.hxx>
double OCCT_GraphOperations::ComputeMinAngle(const gp_Vec& theVec, const TopoDS_Edge& theEdge, double tolerance)
{
    Handle(Geom_Curve) curve;
    Standard_Real first, last;
    curve = BRep_Tool::Curve(theEdge, first, last);  // 提取几何曲线及参数范围[1](@ref)// 判断曲线类型并计算最小夹
    gp_Pnt p;
    gp_Vec tangent;
    curve->D1(first, p, tangent);

    return ComputeAngle(tangent,theVec);
}

double OCCT_GraphOperations::ComputeAngle(const gp_Vec& v1, const gp_Vec& v2)
{   double dot = v1.Dot(v2);
    double mag1 = v1.Magnitude(), mag2 = v2.Magnitude();
    if (mag1 == 0 || mag2 == 0) return 0.0;  // 处理零向量
    
    double cosAngle = dot / (mag1 * mag2); 
    double angle = std::acos(cosAngle);
    return angle > M_PI_2 ? M_PI - angle : angle;  // 返回锐角
}

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <map>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
TopoDS_Shape OCCT_GraphOperations::CreatOCCMeshShape(const std::vector<double>& nodeCoords, const std::vector<int>& elemTypes, const std::vector<std::vector<std::size_t>>& elemNodeTags)
{
        TopoDS_Compound result;
        BRep_Builder builder;
        builder.MakeCompound(result);

        // 创建节点映射表：Gmsh节点标签 -> OCC顶点
        std::map<std::size_t, TopoDS_Vertex> vertexMap;

        // 1. 创建所有顶点
        for (std::size_t i = 0; i < nodeCoords.size() / 3; i++) {
            double x = nodeCoords[3 * i];
            double y = nodeCoords[3 * i + 1];
            double z = nodeCoords[3 * i + 2];

            gp_Pnt point(x, y, z);
            TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);
            vertexMap[i + 1] = vertex; // Gmsh节点标签从1开始
        }

        // 2. 处理不同类型的网格单元
        for (std::size_t i = 0; i < elemTypes.size(); i++) {
            int elementType = elemTypes[i];
            const std::vector<std::size_t>& elementNodes = elemNodeTags[i];

            std::size_t nodesPerElement = 0;
            switch (elementType) {
            case 1: // 2-node line
                nodesPerElement = 2;
                break;
            case 2: // 3-node triangle
                nodesPerElement = 3;
                break;
            case 3: // 4-node quadrilateral
                nodesPerElement = 4;
                break;
            case 4: // 4-node tetrahedron
                nodesPerElement = 4;
                break;
            case 5: // 8-node hexahedron
                nodesPerElement = 8;
                break;
            case 15: // 1-node point
                nodesPerElement = 1;
                break;
            default:
                std::cout << "警告: 未知单元类型 " << elementType << ", 跳过处理" << std::endl;
                continue;
            }

            std::size_t numElements = elementNodes.size() / nodesPerElement;
            cout << "单元类型编号: " << elementType << " , 单元数量: " << numElements << endl;
            for (std::size_t j = 0; j < numElements; j++) {
                try {
                    // 提取当前单元的节点
                    std::vector<TopoDS_Vertex> currentVertices;
                    for (std::size_t k = 0; k < nodesPerElement; k++) {
                        std::size_t nodeIndex = elementNodes[j * nodesPerElement + k];
                        if (vertexMap.find(nodeIndex) != vertexMap.end()) {
                            currentVertices.push_back(vertexMap[nodeIndex]);
                        }
                    }

                    if (currentVertices.size() != nodesPerElement) {
                        continue;
                    }

                    // 根据单元类型创建相应的几何形状
                    switch (elementType) {
                    case 1: { // 线单元 - 创建边
                        if (currentVertices.size() >= 2) {
                            TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(currentVertices[0], currentVertices[1]);
                            builder.Add(result, edge);
                        }
                        break;
                    }
                    case 2: { // 三角形单元 - 创建面
                        if (currentVertices.size() >= 3) {
                            BRepBuilderAPI_MakePolygon polygon;
                            polygon.Add(currentVertices[0]);
                            polygon.Add(currentVertices[1]);
                            polygon.Add(currentVertices[2]);
                            polygon.Close();// 闭合多边形

                            if (polygon.IsDone()) {
                                TopoDS_Wire wire = polygon.Wire();
                                TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);
                                if (!face.IsNull()) {
                                    builder.Add(result, face);
                                }
                                else cout << "三角形面创建失败" << endl;
                            }
                        }
                        break;
                    }
                    case 3: { // 四边形单元 - 创建面
                        if (currentVertices.size() >= 4) {
                            try {
                                // 获取四个顶点的坐标
                                gp_Pnt P1 = BRep_Tool::Pnt(currentVertices[0]);
                                gp_Pnt P2 = BRep_Tool::Pnt(currentVertices[1]);
                                gp_Pnt P3 = BRep_Tool::Pnt(currentVertices[2]);
                                gp_Pnt P4 = BRep_Tool::Pnt(currentVertices[3]);

                                // 方法1：使用四点创建几何平面
                                GC_MakePlane planeMaker(P1, P2, P3);
                                if (planeMaker.IsDone()) {
                                    Handle(Geom_Plane) geometricPlane = planeMaker.Value();

                                    BRepBuilderAPI_MakePolygon polygon;
                                    polygon.Add(currentVertices[0]);
                                    polygon.Add(currentVertices[1]);
                                    polygon.Add(currentVertices[2]);
                                    polygon.Add(currentVertices[3]);
                                    polygon.Close();

                                    if (polygon.IsDone()) {
                                        TopoDS_Wire wire = polygon.Wire();

                                        // 使用明确的几何平面创建面
                                        TopoDS_Face face = BRepBuilderAPI_MakeFace(geometricPlane, wire);

                                        if (!face.IsNull()) {
                                            builder.Add(result, face);
                                        }
                                        else {
                                            // 如果失败，尝试不使用线环直接创建平面面
                                            face = BRepBuilderAPI_MakeFace(geometricPlane, Precision::Confusion());
                                            if (!face.IsNull()) {
                                                builder.Add(result, face);
                                            }
                                            else {
                                                builder.Add(result, wire); // 降级为线框显示
                                            }
                                        }
                                    }
                                }
                            }
                            catch (Standard_Failure&) {
                                // 创建失败时的备选方案
                                BRepBuilderAPI_MakePolygon poly;
                                poly.Add(currentVertices[0]);
                                poly.Add(currentVertices[1]);
                                poly.Add(currentVertices[2]);
                                poly.Add(currentVertices[3]);
                                poly.Close();

                                if (poly.IsDone()) {
                                    builder.Add(result, poly.Wire());
                                }
                            }
                        }
                        break;
                    }
                    case 4: { // 四面体单元 
                        TopoDS_Solid tetrahedron = CreateTetrahedronSolid(BRep_Tool::Pnt(currentVertices[0]), BRep_Tool::Pnt(currentVertices[1]),
                            BRep_Tool::Pnt(currentVertices[2]), BRep_Tool::Pnt(currentVertices[3]));
                        if(!tetrahedron.IsNull()) builder.Add(result, tetrahedron);
                        else {
                            Msg::ShowWarning("创建四面体实体失败，降级为显示面。");
                            std::vector<std::vector<int>> tetraFaces = {
                               {0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}
                            };
                            for (const auto& faceIndices : tetraFaces) {
                                BRepBuilderAPI_MakePolygon polygon;
                                for (int idx : faceIndices) {
                                    polygon.Add(currentVertices[idx]);
                                }
                                polygon.Close();

                                if (polygon.IsDone()) {
                                    TopoDS_Wire wire = polygon.Wire();
                                    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);
                                    if (!face.IsNull()) {
                                        builder.Add(result, face);
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case 5: { // 六面体单元 - 创建四边形面组成的复合体
                        // 六面体面的节点顺序（需要根据Gmsh的实际顺序调整）
                        if (currentVertices.size() == 8) {
                            // 创建六面体实体
                            TopoDS_Solid hexahedron = CreateHexahedronSolid(currentVertices);
                            if (!hexahedron.IsNull()) {
                                builder.Add(result, hexahedron);
                            }
                            else {
                                Msg::ShowWarning("创建六面体实体失败，降级为显示面。");
                                // 六面体面的节点顺序（需要根据Gmsh的实际顺序调整）
                                std::vector<std::vector<int>> hexFaces = {
                                    {0, 1, 2, 3}, // 底面
                                    {4, 7, 6, 5}, // 顶面
                                    {0, 4, 5, 1}, // 前面
                                    {1, 5, 6, 2}, // 右面
                                    {2, 6, 7, 3}, // 后面
                                    {3, 7, 4, 0}  // 左面
                                };
                                for (const auto& faceIndices : hexFaces) {
                                    BRepBuilderAPI_MakePolygon polygon;
                                    for (int idx : faceIndices) {
                                        if (idx < currentVertices.size()) {
                                            polygon.Add(currentVertices[idx]);
                                        }
                                    }
                                    polygon.Close();
                                    if (polygon.IsDone()) {
                                        TopoDS_Wire wire = polygon.Wire();
                                        TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);
                                        if (!face.IsNull()) {
                                            builder.Add(result, face);
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case 15: { // 点单元
                        builder.Add(result, currentVertices[0]);
                        break;
                    }
                    }
                }
                catch (const Standard_Failure& e) {
                    std::cerr << "创建单元时出错: " << e.GetMessageString() << std::endl;
                    continue;
                }
            }
        }

        std::cout << "成功创建OCC网格形状，包含 " << vertexMap.size() << " 个顶点" << std::endl;
        return result;
}

void OCCT_GraphOperations::PrintMeshInfo(int NodeNum, std::vector<int> elemTypes, std::vector<std::vector<std::size_t>> elemTags)
{
    Standard_Character Buffer[1024] = { 0 };

    Sprintf(Buffer, "节点数量：%d", NodeNum);
    Msg::ShowInfo(Buffer);

    Sprintf(Buffer, "单元类型数量：%zu", elemTypes.size());
    Msg::ShowInfo(Buffer);

    // 打印总单元数量
    std::size_t totalElements = 0;
    for (const auto& elemTagList : elemTags) {
        totalElements += elemTagList.size();
    }
    Sprintf(Buffer, "总单元数量：%zu", totalElements);
    Msg::ShowInfo(Buffer);

    // 打印每种单元类型的详细信息
    for (std::size_t i = 0; i < elemTypes.size(); ++i) {

        switch (elemTypes[i]) {
        case 15: Sprintf(Buffer, "1-node point.num: %zu", elemTags[i].size()); break;
        case 1: Sprintf(Buffer, "2-node line.num: %zu", elemTags[i].size()); break;
        case 2: Sprintf(Buffer, "3-node triangle.num: %zu", elemTags[i].size()); break;
        case 3: Sprintf(Buffer, "4-node quadrangle.num: %zu", elemTags[i].size()); break;
        case 4: Sprintf(Buffer, "4-node tetrahedron.num: %zu", elemTags[i].size()); break;
        case 5: Sprintf(Buffer, "8-node hexahedron.num: %zu", elemTags[i].size()); break;
        case 6: Sprintf(Buffer, "6-node prism.num: %zu", elemTags[i].size()); break;
        case 7: Sprintf(Buffer, "5-node pyramid.num: %zu", elemTags[i].size()); break;
            //....常用单元类型如上 其他不常见单元类型可继续添加
        default: Sprintf(Buffer, "未知单元类型 %d", elemTypes[i]); break;

        }
        Msg::ShowInfo(Buffer);
    };
}


TopoDS_Solid OCCT_GraphOperations::CreateTetrahedronSolid(const gp_Pnt& p0, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3)
{
    try {
        // 方法1: 使用BRepPrimAPI创建四面体
        // 计算四面体的中心点
        gp_Pnt center((p0.X() + p1.X() + p2.X() + p3.X()) / 4.0,
            (p0.Y() + p1.Y() + p2.Y() + p3.Y()) / 4.0,
            (p0.Z() + p1.Z() + p2.Z() + p3.Z()) / 4.0);

        // 创建四个三角形面
        BRepBuilderAPI_MakePolygon poly1, poly2, poly3, poly4;

        // 面1: p0-p1-p2
        poly1.Add(p0);
        poly1.Add(p1);
        poly1.Add(p2);
        poly1.Close();

        // 面2: p0-p3-p1
        poly2.Add(p0);
        poly2.Add(p3);
        poly2.Add(p1);
        poly2.Close();

        // 面3: p0-p2-p3
        poly3.Add(p0);
        poly3.Add(p2);
        poly3.Add(p3);
        poly3.Close();

        // 面4: p1-p3-p2
        poly4.Add(p1);
        poly4.Add(p3);
        poly4.Add(p2);
        poly4.Close();

        if (poly1.IsDone() && poly2.IsDone() && poly3.IsDone() && poly4.IsDone()) {
            TopoDS_Face face1 = BRepBuilderAPI_MakeFace(poly1.Wire());
            TopoDS_Face face2 = BRepBuilderAPI_MakeFace(poly2.Wire());
            TopoDS_Face face3 = BRepBuilderAPI_MakeFace(poly3.Wire());
            TopoDS_Face face4 = BRepBuilderAPI_MakeFace(poly4.Wire());

            // 创建壳
            BRepBuilderAPI_Sewing sewer;
            sewer.Add(face1);
            sewer.Add(face2);
            sewer.Add(face3);
            sewer.Add(face4);
            sewer.Perform();

                TopoDS_Shell shell = TopoDS::Shell(sewer.SewedShape());

                // 创建实体
                BRepBuilderAPI_MakeSolid solidMaker;
                solidMaker.Add(shell);

                if (solidMaker.IsDone()) {
                    return solidMaker.Solid();
                }
        }
    }
    catch (const Standard_Failure& e) {
        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "创建四面体实体失败:", e.GetMessageString());
		Msg::ShowError(Buffer);
        return TopoDS_Solid();
    }
}
TopoDS_Solid OCCT_GraphOperations::CreateHexahedronSolid(const std::vector<TopoDS_Vertex>& points)
{
    if (points.size() != 8) {
        return TopoDS_Solid();
    }
    try {
        // 方法1: 通过创建六个面然后缝合创建实体
        std::vector<TopoDS_Face> faces;

        // 定义六面体的六个面（按逆时针顺序）
        std::vector<std::vector<int>> faceIndices = {
            {0, 1, 2, 3}, // 底面
            {4, 7, 6, 5}, // 顶面
            {0, 4, 5, 1}, // 前面
            {1, 5, 6, 2}, // 右面
            {2, 6, 7, 3}, // 后面
            {3, 7, 4, 0}  // 左面
        };

        for (const auto& indices : faceIndices) {
            BRepBuilderAPI_MakePolygon poly;
            for (int idx : indices) {
                poly.Add(points[idx]);
            }
            poly.Close();

            if (poly.IsDone()) {
                TopoDS_Face face = BRepBuilderAPI_MakeFace(poly.Wire());
                if (!face.IsNull()) {
                    faces.push_back(face);
                }
            }
        }
        if (faces.size() == 6) {
            // 缝合所有面创建壳
            BRepBuilderAPI_Sewing sewer;
            for (const auto& face : faces) {
                sewer.Add(face);
            }
            sewer.Perform();

            TopoDS_Shell shell = TopoDS::Shell(sewer.SewedShape());

            // 创建实体
            BRepBuilderAPI_MakeSolid solidMaker;
            solidMaker.Add(shell);

            if (solidMaker.IsDone()) {
                return solidMaker.Solid();
            }
            
        }

    }
    catch (const Standard_Failure& e) {
        Standard_Character Buffer[1024]{};
        Sprintf(Buffer, "创建六面体失败：%s", e.GetMessageString());
        Msg::ShowError(Buffer);
        return TopoDS_Solid();
    }
}

void OCCT_GraphOperations::AssociateNodes(OCCT_ShapeList shapes, std::vector<double>& nodeCoords, std::vector<int>& nodeIds, std::vector<gp_Pnt>& pnts)
{
    // 清空输出结果
    nodeIds.clear();

    // 检查节点坐标数组大小是否为3的倍数
    if (nodeCoords.size() % 3 != 0) {
        // 可以抛出异常或记录错误日志
        Msg::ShowError("错误: nodeCoords大小必须是3的倍数");
        return;
    }

    int numNodes = nodeCoords.size() / 3;
    double tolerance = 1e-5; // 容差，用于判断点是否在形状上

    // 遍历所有节点
    for (int nodeId = 1; nodeId <= numNodes; ++nodeId) {
        // 提取节点坐标 (索引从0开始，节点编号从1开始)
        int coordIndex = (nodeId - 1) * 3;
        double x = nodeCoords[coordIndex];
        double y = nodeCoords[coordIndex + 1];
        double z = nodeCoords[coordIndex + 2];
        gp_Pnt nodePoint(x, y, z);

        bool isOnShape = false;

        // 遍历所有边界条件形状
        for (OCCT_ShapeList::Iterator it(shapes); it.More(); it.Next()) {
            const TopoDS_Shape& shape = it.Value();

            switch (shape.ShapeType()) {
            case TopAbs_FACE: {
                // 检查点是否在面上
                TopoDS_Face face = TopoDS::Face(shape);
                isOnShape = IsPointOnFace(face, nodePoint, tolerance);
                break;
            }
            case TopAbs_EDGE: {
                // 检查点是否在边上
                TopoDS_Edge edge = TopoDS::Edge(shape);

                // 方法1: 使用距离判断
                BRepExtrema_DistShapeShape distCalc;
                TopoDS_Vertex nodeVertex = BRepBuilderAPI_MakeVertex(nodePoint);
                distCalc.LoadS1(edge);
                distCalc.LoadS2(nodeVertex);

                if (distCalc.Perform() && distCalc.Value() < tolerance) {
                    isOnShape = true;
                }
                break;
            }
            case TopAbs_VERTEX: {
                // 检查点是否与顶点重合
                TopoDS_Vertex vertex = TopoDS::Vertex(shape);
                gp_Pnt vertexPoint = BRep_Tool::Pnt(vertex);

                if (nodePoint.Distance(vertexPoint) < tolerance) {
                    isOnShape = true;
                }
                break;
            }
            case TopAbs_COMPOUND: {
                // 如果是复合形状，递归检查其中的子形状
                TopExp_Explorer explorer;

                // 检查面
                explorer.Init(shape, TopAbs_FACE);
                while (explorer.More() && !isOnShape) {
                    TopoDS_Face face = TopoDS::Face(explorer.Current());
                    BRepClass_FaceClassifier classifier;
                    classifier.Perform(face, nodePoint, tolerance);

                    if (classifier.State() == TopAbs_IN || classifier.State() == TopAbs_ON) {
                        isOnShape = true;
                    }
                    explorer.Next();
                }

                // 检查边
                if (!isOnShape) {
                    explorer.Init(shape, TopAbs_EDGE);
                    while (explorer.More() && !isOnShape) {
                        TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
                        BRepExtrema_DistShapeShape distCalc;
                        TopoDS_Vertex nodeVertex = BRepBuilderAPI_MakeVertex(nodePoint);
                        distCalc.LoadS1(edge);
                        distCalc.LoadS2(nodeVertex);

                        if (distCalc.Perform() && distCalc.Value() < tolerance) {
                            isOnShape = true;
                        }
                        explorer.Next();
                    }
                }

                // 检查顶点
                if (!isOnShape) {
                    explorer.Init(shape, TopAbs_VERTEX);
                    while (explorer.More() && !isOnShape) {
                        TopoDS_Vertex vertex = TopoDS::Vertex(explorer.Current());
                        gp_Pnt vertexPoint = BRep_Tool::Pnt(vertex);
                        if (nodePoint.Distance(vertexPoint) < tolerance) {
                            isOnShape = true;
                        }
                        explorer.Next();
                    }
                }
                break;
            }
            default:
                // 忽略其他类型的形状
                break;
            }

            // 如果已经找到匹配的形状，提前退出循环
            if (isOnShape) {
                break;
            }
        }

        // 如果节点在任意一个形状上，添加到结果列表
        if (isOnShape) {
            nodeIds.push_back(nodeId);
			pnts.push_back(nodePoint);
        }
    }

    // 对结果进行排序（可选）
    std::sort(nodeIds.begin(), nodeIds.end());

    // 去除重复项（理论上不应该有，但为了安全）
   // nodeIds.erase(std::unique(nodeIds.begin(), nodeIds.end()), nodeIds.end());
}
bool OCCT_GraphOperations::IsPointOnFace(const TopoDS_Face& theFace, const gp_Pnt& thePnt, const Standard_Real theTol)
{
    //if (theFace.IsNull()) return false;

    //// 初始化面分类器，执行点的拓扑状态判断
    //BRepClass_FaceClassifier aClassifier;
    //aClassifier.Perform(theFace, thePnt, theTol);

    //// TopAbs_State枚举：IN(内部)、ON(边界)、OUT(外部)
    //TopAbs_State aState = aClassifier.State();
    //return (aState == TopAbs_IN || aState == TopAbs_ON);


// 创建一个顶点
    BRepBuilderAPI_MakeVertex builder(thePnt);
    TopoDS_Vertex vertex = builder.Vertex();

    // 计算点到面的最小距离
    BRepExtrema_DistShapeShape distShape(vertex, theFace);
    distShape.Perform();

    if (!distShape.IsDone()) {
        return false;
    }

    // 如果最小距离小于容差，认为点在面上
    return distShape.Value() < theTol;
}
;
////////////////////////////////////
