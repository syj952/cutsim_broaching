#include <QApplication>
#include "ComplainUtf8.h"
#include "mdichild.h"
#include "OCCT_ShapeList.h"
#include "QShapeImportUI.h"
#include "QShapeExportUI.h"
#include <AIS_Shape.hxx>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "Msg.h"
#include "ProjectTree.h"
#include "PropertyView.h"
#include "BoundaryConditionDialog.h"
#include <QFileInfo>
#include <QMessageBox>
#include "AngleDialog.h"
#include <QPushButton>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <AIS_Triangulation.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <QtConcurrent/QtConcurrentRun>
#include <AIS_Trihedron.hxx>
#include <Geom_Axis2Placement.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <gp_Lin.hxx>
#include <Prs3d_PointAspect.hxx>
#include <algorithm>  // 包含 std::reverse
#include <limits>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <Precision.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <SelectMgr_IndexedMapOfOwner.hxx>
#include <cmath>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <Quantity_Color.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>

using namespace std;

namespace
{
    void trimPointComputedFields(std::vector<double>& pointData)
    {
        if (pointData.size() > 6) {
            pointData.resize(6);
        }
    }

    std::array<double, 12> buildConditionPointRow(const std::vector<double>& pointData)
    {
        return {
            pointData[0], pointData[1], pointData[2],
            pointData[6], pointData[7], pointData[8],
            pointData[9], pointData[10], pointData[11],
            pointData[12], pointData[13], pointData[14]
        };
    }

    void appendDiscretePoint(
        const gp_Pnt& point,
        const gp_Vec& dir,
        std::vector<std::array<double, 3>>& flatPoints,
        std::vector<std::vector<double>>& flatPointData,
        std::vector<std::vector<double>>& groupedPointData)
    {
        const std::array<double, 3> xyz = { point.X(), point.Y(), point.Z() };
        const std::vector<double> xyzDir = { point.X(), point.Y(), point.Z(), dir.X(), dir.Y(), dir.Z() };
        flatPoints.push_back(xyz);
        flatPointData.push_back(xyzDir);
        groupedPointData.push_back(xyzDir);
    }

    void rebuildFlatPointDataFromGroups(
        const std::vector<std::vector<std::vector<double>>>& groupedPointData,
        std::vector<std::vector<double>>& flatPointData)
    {
        flatPointData.clear();
        for (const auto& edgeGroup : groupedPointData) {
            flatPointData.insert(flatPointData.end(), edgeGroup.begin(), edgeGroup.end());
        }
    }

    bool exportCuttingEdgePointsToFile(
        const std::vector<std::array<double, 12>>& edgePoints,
        const QString& folderPath,
        int fileIndex,
        const QString& fileBaseName)
    {
        if (edgePoints.empty()) {
            return true;
        }

        QDir exportDir(folderPath);
        if (!exportDir.exists() && !exportDir.mkpath(".")) {
            Msg::ShowError("无法创建切削刃导出文件夹。");
            return false;
        }

        const QString fileName = QString("%1_%2.txt").arg(fileBaseName).arg(fileIndex - 1);
        const QString filePath = exportDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            Msg::ShowError("无法打开切削刃导出文件，请检查路径或权限。");
            return false;
        }

        QTextStream outFile(&file);
        std::vector<std::array<double, 12>> sortedEdgePoints = edgePoints;
        std::sort(sortedEdgePoints.begin(), sortedEdgePoints.end(),
            [](const std::array<double, 12>& lhs, const std::array<double, 12>& rhs) {
                return lhs[0] < rhs[0];
            });

        for (const auto& point : sortedEdgePoints) {
            outFile << point[0] << "\t" << point[1] << "\t" << point[2] << "\t"
                << point[3] << "\t" << point[4] << "\t" << point[5] << "\t"
                << point[6] << "\t" << point[7] << "\t" << point[8] << "\t"
                << point[9] << "\t" << point[10] << "\t" << point[11] << "\n";
        }
        outFile.flush();
        file.close();
        return true;
    }

    bool exportCuttingEdgeFileList(
        const QString& folderPath,
        int fileCount,
        const QString& fileBaseName)
    {
        if (fileCount <= 0) {
            return true;
        }

        QDir exportDir(folderPath);
        if (!exportDir.exists() && !exportDir.mkpath(".")) {
            Msg::ShowError("无法创建切削刃清单文件夹。");
            return false;
        }

        const QString fileListPath = exportDir.filePath(fileBaseName + ".txt");
        QFile file(fileListPath);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            Msg::ShowError("无法打开切削刃清单文件，请检查路径或权限。");
            return false;
        }

        QTextStream outFile(&file);
        for (int index = 0; index < fileCount; ++index) {
            outFile << "data/" << fileBaseName << "_" << index << ".txt\n";
        }
        outFile.flush();
        file.close();
        return true;
    }

    bool findNearestFaceByPointProjection(
        const std::vector<TopoDS_Face>& faces,
        const gp_Pnt& point,
        double tolerance,
        TopoDS_Face& matchedFace)
    {
        double bestDistance = std::numeric_limits<double>::max();
        TopoDS_Face bestFace;

        for (const TopoDS_Face& face : faces) {
            Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
            if (surface.IsNull()) {
                continue;
            }

            try {
                GeomAPI_ProjectPointOnSurf projector(point, surface);
                if (!projector.IsDone() || projector.NbPoints() < 1) {
                    continue;
                }

                const double distance = projector.LowerDistance();
                gp_Pnt projectedPoint = projector.NearestPoint();
                BRepClass_FaceClassifier classifier;
                classifier.Perform(face, projectedPoint, tolerance);
                if (classifier.State() != TopAbs_IN && classifier.State() != TopAbs_ON) {
                    continue;
                }

                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestFace = face;
                }
            }
            catch (Standard_Failure&) {
                continue;
            }
        }

        if (!bestFace.IsNull() && bestDistance <= tolerance) {
            matchedFace = bestFace;
            return true;
        }

        return false;
    }

    bool buildNormalPlaneSectionEdge(
        const gp_Pln& normalPlane,
        const TopoDS_Face& toolFace,
        TopoDS_Edge& sectionEdge)
    {
        const double sectionSizes[] = { 0.1, 1.0, 5.0, 20.0, 100.0 };
        for (double size : sectionSizes) {
            try {
                TopoDS_Face normalFace = BRepBuilderAPI_MakeFace(normalPlane, -size, size, -size, size).Face();
                BRepAlgoAPI_Section sectionMaker(normalFace, toolFace, Standard_False);
                sectionMaker.ComputePCurveOn1(Standard_True);
                sectionMaker.Approximation(Standard_True);
                sectionMaker.Build();

                if (!sectionMaker.IsDone()) {
                    continue;
                }

                TopExp_Explorer edgeExplorer(sectionMaker.Shape(), TopAbs_EDGE);
                if (edgeExplorer.More()) {
                    sectionEdge = TopoDS::Edge(edgeExplorer.Current());
                    return true;
                }
            }
            catch (Standard_Failure&) {
                continue;
            }
        }

        Handle(Geom_Surface) toolSurface = BRep_Tool::Surface(toolFace);
        Handle(Geom_Plane) toolPlane = Handle(Geom_Plane)::DownCast(toolSurface);
        if (!toolPlane.IsNull()) {
            gp_Vec directionVec = gp_Vec(normalPlane.Axis().Direction()).Crossed(gp_Vec(toolPlane->Axis().Direction()));
            if (directionVec.Magnitude() > Precision::Confusion()) {
                gp_Lin sectionLine(normalPlane.Location(), gp_Dir(directionVec));
                sectionEdge = BRepBuilderAPI_MakeEdge(sectionLine, -100.0, 100.0).Edge();
                return !sectionEdge.IsNull();
            }
        }

        return false;
    }
}

void MdiChild::ImportModel()
{

    // 弹出选择对话框
    QMessageBox msgBox;
    msgBox.setWindowTitle("选择导入类型");
    msgBox.setText("请选择导入类型：");
    QPushButton* partButton = msgBox.addButton("导入工件", QMessageBox::ActionRole);
    QPushButton* planeButton = msgBox.addButton("导入刀具", QMessageBox::ActionRole);
    int ret = msgBox.exec();
    QShapeImportUI UI;
    int partorcutter = 0;
    if (msgBox.clickedButton() == partButton) partorcutter = 0;
    if (msgBox.clickedButton() == planeButton) partorcutter = 1;
    const OCCT_ShapeList& Shapes = OCCT_ShapeImport(&UI).ImportModel(partorcutter); \
        if (Shapes.IsEmpty()) {
            Msg::ShowError("导入模型失败！");
            return;
        }

    ModelData model;
    for (TopoDS_Shape shape : Shapes) {
        h_MyViewer->AisObjColor(Quantity_NOC_BLUE);//设置已选中的图形颜色为蓝色
        Handle(AIS_Shape) hAisShape = new AIS_Shape(shape);
        hAisShape->SetDisplayMode(AIS_Shaded);
        h_MyViewer->Display(hAisShape); //显示导入的模型
        model.shape = hAisShape;
    }

    for (OCCT_Utf8String Path : UI.Paths) {
        QString path = QString::fromUtf8(Path.ToCString());
        QFileInfo fileInfo(path);
        model.name = fileInfo.fileName();
        model.path = path;
    }

    if (msgBox.clickedButton() == partButton) {
        Msg::ShowInfo("导入工件");
        model.type = "工件"; // 标记为工件        
        p_TreeWidget->addPartToTree(model);

    }
    else if (msgBox.clickedButton() == planeButton) {
        Msg::ShowInfo("导入刀具");
        model.type = "切割平面"; // 标记为切割平面
        p_TreeWidget->addPlaneToTree(model);
    }
    else {
        Msg::ShowInfo("取消导入");
        // 处理取消或其他按钮的情况
        return;
    }

}

void MdiChild::ImportPart()
{

}

void MdiChild::ImportPlane()
{
}

void MdiChild::ExportModel()
{
    OCCT_ShapeList Shapes;
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();//获取交互上下文
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())//遍历上下文中的图形对象
        Shapes.Append(myContext->SelectedShape());//获取选中的图形对象
    if (Shapes.Size())
    {
        QShapeExportUI UI;
        OCCT_ShapeExport(&UI).ExportModel(Shapes);
    }
}

void MdiChild::SelNatural()
{
    h_MyViewer->getAisContext()->Deactivate();  // 取消当前的选择模式
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_SHAPE));
}

void MdiChild::SelSolid()
{
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_SOLID));
}

void MdiChild::SelFace()
{
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));
}

void MdiChild::SelWire()
{
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_WIRE));
}

void MdiChild::SelEdge()
{
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
}

void MdiChild::SelVertex()
{
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
}

#include <BRepPrimAPI_MakeBox.hxx>
#include <QtWidgetsDepends>
#include <AIS_InteractiveObject.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <qinputdialog.h>
#include <BRepBuilderAPI_Transform.hxx>
#include "TranslationDialog.h"
#include "OCCT_GraphOperations.h"

void MdiChild::Test1()
{
    //测试代码1
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    {
        //BRepPrimAPI_MakeBox Makebox(100,100,100);
        TopoDS_Shape Shape = BRepPrimAPI_MakeBox(100, 100, 100).Solid();
        //double deflection = 0.5; 
        //BRepMesh_IncrementalMesh mesh(Shape, deflection);
        //mesh.Perform();
        Handle(AIS_Shape) hShape = new AIS_Shape(Shape);
        myContext->Display(hShape, Standard_False);
    }
    myContext->DisplayAll(Standard_True);
    Msg::ShowInfo("测试1——创建一个BOX并且显示！");
}

void MdiChild::Test2() {
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    {
        //创建一个四面体
        TopoDS_Shape Shape = OCCT_GraphOperations::CreateTetrahedronSolid(gp_Pnt(0, -100, 0), gp_Pnt(100, 0, 0), gp_Pnt(0, 100, 0), gp_Pnt(0, 0, 100));

        Handle(AIS_Shape) hShape = new AIS_Shape(Shape);
        hShape->SetDisplayMode(AIS_Shaded);
        //// 设置材质属性
        //Graphic3d_MaterialAspect material;
        //material.SetMaterialType(Graphic3d_MATERIAL_ASPECT);
        ////material->SetAmbient(0.2);
        ////material->SetDiffuse(0.8);
        ////material->SetSpecular(0.1);
        //material.SetTransparency(0.0);
        //hShape->SetMaterial(material);    // 应用材质

        h_MyViewer->Display(hShape);
        myContext->SetDisplayMode(hShape, AIS_Shaded, Standard_False);
    }
    myContext->DisplayAll(Standard_True);
    Msg::ShowInfo("测试1——创建一个BOX并且显示！");
}

void MdiChild::Calcu() {

    QString filePath = QFileDialog::getOpenFileName(nullptr, "选择输入文件", "", "kpp Files (*.kpp);;All Files (*)");
    Msg::ShowInfo("选择的文件路径：");
    QString fortranExe = "E:\CNC-XFEM\DistoPart-main\PhiPsi_11.28\x64\Debug\PhiPsi_Book.exe";

    QStringList arguments;

    arguments << "-i" << filePath;

    QProcess process;
    QString program = "E:\CNC-XFEM\DistoPart-main\PhiPsi_11.28\x64\Debug\PhiPsi_Book.exe";

    process.start(program, arguments);
    // 等待外部程序启动完成
    if (process.waitForStarted()) {
        Msg::ShowInfo("程序启动成功!");
    }
    else {
        Msg::ShowInfo("程序启动失败!");
    }
}

///useless
void MdiChild::GetSelectedShapes() {
    //Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    //if (!myContext) {
    //    Msg::ShowError("交互上下文未初始化！");
    //    return;
    //}
    //// 检查是否有选中的对象
    //if (myContext->HasSelectedShape()) {
    //    while (myContext->MoreSelected()){
    //        // 获取当前选中的交互对象
    //        Handle(AIS_InteractiveObject) selectedObject = myContext->SelectedInteractive();
    //        if (selectedObject.IsNull()) continue;
    //        // 获取选中的 AIS 对象
    //        Handle(AIS_Shape) selectedAISShape = Handle(AIS_Shape)::DownCast(selectedObject);
    //        if (selectedAISShape.IsNull()) continue;
    //        // 获取实际的几何形状
    //        const TopoDS_Shape& shape = selectedAISShape->Shape();
    //        // 输出形状类型
    //        TopAbs_ShapeEnum shapeType = shape.ShapeType();
    //        QString shapeTypeName;
    //        switch (shapeType) {
    //        case TopAbs_VERTEX:
    //            shapeTypeName = "顶点";
    //            break;
    //        case TopAbs_EDGE:
    //            shapeTypeName = "边";
    //            break;
    //        case TopAbs_WIRE:
    //            shapeTypeName = "线框";
    //            break;
    //        case TopAbs_FACE:
    //            shapeTypeName = "面";
    //            break;
    //        case TopAbs_SOLID:
    //            shapeTypeName = "实体";
    //            break;
    //        case TopAbs_COMPOUND:
    //            shapeTypeName = "复合体";
    //            break;
    //        default:
    //            shapeTypeName = "未知类型";
    //            break;
    //        }
    //        
    //        Standard_Character Buffer[1024] = { 0 };
    //        Sprintf(Buffer, "选中了：", shapeTypeName);
    //        Msg::ShowInfo(OCCT_Utf8String(Buffer));
    //    } 
    //}
    //else {
    //    Msg::ShowInfo("没有选中任何对象！");
    //}
}

void MdiChild::MoveModel()
{

    // 检查是否有选中的模型
    if (!h_MyViewer->getAisContext()->HasSelectedShape()) {
        Msg::ShowWarning("请先选择一个模型！");
        return;
    }

    // 获取选中的模型
    Handle(AIS_Shape) selectedShape = Handle(AIS_Shape)::DownCast(h_MyViewer->getAisContext()->SelectedInteractive());
    if (selectedShape.IsNull()) {
        Msg::ShowError("选中的对象不是模型！");
        return;
    }

    // 创建自定义对话框
    TranslationDialog dialog(this, 1);
    if (dialog.exec() != QDialog::Accepted) return;

    // 获取用户输入的平移距离
    double dx = dialog.getX();
    double dy = dialog.getY();
    double dz = dialog.getZ();
    bool isAbsolute = dialog.isAbsolutePosition();

    // 创建平移变换
    gp_Trsf translation;

    if (isAbsolute) {
        // 绝对位置移动：移动到指定坐标
        translation.SetTranslation(gp_Vec(dx, dy, dz));
    }
    else {
        // 相对位置移动：相对当前位置移动
        // 获取当前变换
        gp_Trsf currentTrsf = selectedShape->LocalTransformation();
        gp_Pnt currentLocation = gp_Pnt(0, 0, 0).Transformed(currentTrsf);

        // 计算需要移动的距离（目标位置 - 当前位置）
        double moveX = dx + currentLocation.X();
        double moveY = dy + currentLocation.Y();
        double moveZ = dz + currentLocation.Z();

        translation.SetTranslation(gp_Vec(moveX, moveY, moveZ));
    }

    // 应用变换
    selectedShape->SetLocalTransformation(translation);
    h_MyViewer->getAisContext()->Update(selectedShape, Standard_True);

    // 如果移动的是模型，同时移动其关联的离散点
    NCollection_List<Handle(AIS_InteractiveObject)> allObjects = h_MyViewer->GetAisObj();
    for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator it(allObjects); it.More(); it.Next()) {
        Handle(AIS_InteractiveObject) obj = it.Value();
        if (m_discretePointToModelMap.contains(obj) && m_discretePointToModelMap[obj] == selectedShape) {
            // 找到关联的离散点，应用相同的变换
            Handle(AIS_Shape) pointShape = Handle(AIS_Shape)::DownCast(obj);
            if (!pointShape.IsNull()) {
                pointShape->SetLocalTransformation(translation);
                h_MyViewer->getAisContext()->Update(pointShape, Standard_True);
            }
        }
    }

    Msg::ShowInfo("模型已移动！");

}

void MdiChild::MoveToolModel(bool isAbsolute, double dx, double dy, double dz)
{
    // 获取刀具模型
    if (p_TreeWidget->Planeitem->childCount() == 0) return;
    for (int i = 0; i < p_TreeWidget->Planeitem->childCount(); i++) {
        QString tt = p_TreeWidget->Planeitem->child(i)->text(0);
        Handle(AIS_Shape) selectedShape = p_TreeWidget->modelMap.value(tt).shape;
        if (selectedShape.IsNull()) {
            Msg::ShowError("选中的对象不是模型！");
            return;
        }

        // 创建平移变换
        gp_Trsf translation;

        if (isAbsolute) {
            // 绝对位置移动：移动到指定坐标
            translation.SetTranslation(gp_Vec(dx, dy, dz));
        }
        else {
            // 相对位置移动：相对当前位置移动
            // 获取当前变换
            gp_Trsf currentTrsf = selectedShape->LocalTransformation();
            gp_Pnt currentLocation = gp_Pnt(0, 0, 0).Transformed(currentTrsf);

            // 计算需要移动的距离（目标位置 - 当前位置）
            double moveX = dx + currentLocation.X();
            double moveY = dy + currentLocation.Y();
            double moveZ = dz + currentLocation.Z();

            translation.SetTranslation(gp_Vec(moveX, moveY, moveZ));
        }

        // 应用变换
        selectedShape->SetLocalTransformation(translation);
        h_MyViewer->getAisContext()->Update(selectedShape, Standard_True);

        // 如果移动的是模型，同时移动其关联的离散点
        NCollection_List<Handle(AIS_InteractiveObject)> allObjects = h_MyViewer->GetAisObj();
        for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator it(allObjects); it.More(); it.Next()) {
            Handle(AIS_InteractiveObject) obj = it.Value();
            if (m_discretePointToModelMap.contains(obj) && m_discretePointToModelMap[obj] == selectedShape) {
                // 找到关联的离散点，应用相同的变换
                Handle(AIS_Shape) pointShape = Handle(AIS_Shape)::DownCast(obj);
                if (!pointShape.IsNull()) {
                    pointShape->SetLocalTransformation(translation);
                    h_MyViewer->getAisContext()->Update(pointShape, Standard_True);
                }
            }
        }
    }
    //Msg::ShowInfo("刀具模型已移动！");
}

void MdiChild::RotateModel()
{
    // 检查是否有选中的模型
    if (!h_MyViewer->getAisContext()->HasSelectedShape()) {
        Msg::ShowWarning("请先选择一个模型！");
        return;
    }

    // 获取选中的模型
    Handle(AIS_Shape) selectedShape = Handle(AIS_Shape)::DownCast(h_MyViewer->getAisContext()->SelectedInteractive());
    if (selectedShape.IsNull()) {
        Msg::ShowError("选中的对象不是模型！");
        return;
    }

    TranslationDialog dialog(this, 2);
    if (dialog.exec() != QDialog::Accepted) return;

    // 获取旋转参数
    double angle = dialog.getAngle();
    double ax = dialog.getX();
    double ay = dialog.getY();
    double az = dialog.getZ();

    // 创建旋转变换
    gp_Trsf rotation;
    rotation.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(ax, ay, az)), angle * M_PI / 180.0);//过原点的旋转轴，以逆时针来旋转

    // 应用变换
    selectedShape->SetLocalTransformation(rotation);
    h_MyViewer->getAisContext()->Update(selectedShape, Standard_True);

    Msg::ShowInfo("模型已旋转！");

}

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
void MdiChild::ScaleModel()
{
    // 检查是否有选中的模型
    if (!h_MyViewer->getAisContext()->HasSelectedShape()) {
        Msg::ShowWarning("请先选择一个模型！");
        return;
    }

    // 获取选中的模型
    Handle(AIS_Shape) selectedShape = Handle(AIS_Shape)::DownCast(
        h_MyViewer->getAisContext()->SelectedInteractive()
    );
    if (selectedShape.IsNull()) {
        Msg::ShowError("选中的对象不是模型！");
        return;
    }

    // 获取用户输入的缩放参数
    TranslationDialog dialog(this, 3);
    if (dialog.exec() != QDialog::Accepted) return;
    double scaleFactor = dialog.getScale();
    double x = dialog.getX();
    double y = dialog.getY();
    double z = dialog.getZ();
    gp_Pnt center(x, y, z);
    if (scaleFactor <= 0 || scaleFactor == 1.0) {
        Msg::ShowWarning("无效的缩放因子！");
        return;
    }
    TopoDS_Shape originalShape = selectedShape->Shape();

    //// 获取模型中心点作为缩放基准
    //Bnd_Box bbox;
    //BRepBndLib::Add(originalShape, bbox);
    //double x1, y1, z1, x2, y2, z2;
    //bbox.Get(x1, y1, z1, x2, y2, z2);
    //gp_Pnt center((x1 + x2) / 2, (y1 + y2) / 2, (z1 + z2) / 2);

    gp_Trsf scaling;
    scaling.SetScale(center, scaleFactor);

    // 应用缩放变换到几何数据
    BRepBuilderAPI_Transform transform(
        originalShape,
        scaling
    );
    if (!transform.IsDone()) {
        Msg::ShowError("缩放变换失败！");
        return;
    }

    TopoDS_Shape scaledShape = transform.Shape();
    if (scaledShape.IsNull()) {
        Msg::ShowError("缩放后的形状为空！");
        return;
    }
    h_MyViewer->getAisContext()->Remove(selectedShape, Standard_False);
    //selectedShape->SetLocalTransformation(scaling);
    // 更新模型几何和显示
    selectedShape->ResetTransformation();
    selectedShape->SetShape(scaledShape);
    h_MyViewer->getAisContext()->Display(selectedShape, Standard_True);
    h_MyViewer->getAisContext()->Update(selectedShape, Standard_True);
    h_MyViewer->Redraw();

    Msg::ShowInfo("模型已缩放！");
}

void MdiChild::FaceOffset()
{
    // 0. 设置选择模式为面
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));

    // 1. 获取交互上下文
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (!myContext) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }

    // 2. 检查是否有选中的对象
    if (!myContext->HasSelectedShape()) {
        Msg::ShowInfo("请先选择一个面！");
        // 创建事件循环，等待用户选择面
        QEventLoop loop;
        QObject::connect(h_MyViewer->getSignals(), &MyViewerSignals::selectionChangedSignal, &loop, &QEventLoop::quit);
        loop.exec(); // 等待用户选择面
    }

    // 3. 遍历选中的对象，提取第一个面
    TopoDS_Face selectedFace;
    bool hasFace = false;
    while (!hasFace) {
        for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
            // 获取选中的形状
            TopoDS_Shape shape = myContext->SelectedShape();
            Msg::ShowInfo("开始循环");
            // 如果是面，直接使用
            if (shape.ShapeType() == TopAbs_FACE) {
                selectedFace = TopoDS::Face(shape);
                hasFace = true;
                break;
            }
            // 如果是其他类型（如实体），尝试提取其中的面
            else {

                Msg::ShowInfo("请选中一个面！");
                break;

            }
        }
    }

    // 4. 设置偏置参数（偏移距离）
    bool ok;

    double offsetDistance = QInputDialog::getDouble(this, "输入偏移距离", "距离 (mm):", 5.0, -100.0, 100.0, 2, &ok);
    if (!ok) return;

    // 5. 执行偏置操作
    try {
        BRepOffsetAPI_MakeOffsetShape offsetMaker;
        offsetMaker.PerformBySimple(selectedFace, offsetDistance);

        if (!offsetMaker.IsDone()) {
            Msg::ShowError("偏置操作失败！");
            return;
        }

        // 6. 获取偏置后的形状并显示
        TopoDS_Shape offsetShape = offsetMaker.Shape();
        // h_MyViewer->AisObjColor(Quantity_NOC_RED);
        h_MyViewer->Display(offsetShape);
        Handle(AIS_Shape) offsetShape_Ais = new AIS_Shape(offsetShape);

        // 7. 输入到工程树中
        ModelData model;
        model.shape = offsetShape_Ais;
        model.name = "偏置面";

        p_TreeWidget->addModelToTree(model); //将模型添加到工程树
        Msg::ShowInfo("面偏置成功！");
    }
    catch (Standard_Failure& e) {
        Msg::ShowError("偏置操作异常：");
        Msg::ShowError(e.GetMessageString());
    }

}

void MdiChild::FaceExtend()
{  // 0. 设置选择模式为面
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));

    // 1. 获取交互上下文
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (!myContext) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }
    // 2. 检查是否有选中的对象
    if (!myContext->HasSelectedShape()) {
        Msg::ShowInfo("请先选择一个面！");
        // 创建事件循环，等待用户选择面
        QEventLoop loop;
        QObject::connect(h_MyViewer->getSignals(), &MyViewerSignals::selectionChangedSignal, &loop, &QEventLoop::quit);
        loop.exec(); // 等待用户选择面
    }
    // 3. 遍历选中的对象，提取第一个面
    TopoDS_Face selectedFace;
    bool hasFace = false;
    while (!hasFace) {
        for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
            // 获取选中的形状
            TopoDS_Shape shape = myContext->SelectedShape();
            Msg::ShowInfo("开始循环");
            // 如果是面，直接使用
            if (shape.ShapeType() == TopAbs_FACE) {
                selectedFace = TopoDS::Face(shape);
                hasFace = true;
                break;
            }
            else {
                Msg::ShowInfo("请选中一个面！");
                break;
            }
        }
    }
    // 4. 获取当前表面的参数范围
    Handle(Geom_Surface) surface = BRep_Tool::Surface(selectedFace);
    Standard_Real u1 = 0, u2 = 0, v1 = 0, v2 = 0;
    surface->Bounds(u1, u2, v1, v2);

    // 4. 设置偏置参数（偏移距离）
    TranslationDialog dialog(this, 4);
    if (dialog.exec() != QDialog::Accepted) return;

    double u1Ext = dialog.getX();
    double v1Ext = dialog.getY();
    double u2Ext = dialog.getZ();
    double v2Ext = dialog.getAngle();
    Standard_Character Buffer[1024] = { 0 };
    Sprintf(Buffer, "扩展参数范围: u1Ext=%.12f, u2Ext=%12f, v1Ext=%12f, v2Ext=%12f", u1Ext, u2Ext, v1Ext, v2Ext);
    Msg::ShowInfo(Buffer);

    u1 += u1Ext;
    u2 += u2Ext;
    v1 += v1Ext;
    v2 += v2Ext;

    Sprintf(Buffer, "扩展参数范围: u1=%f, u2=%f, v1=%f, v2=%f", u1, u2, v1, v2);
    Msg::ShowInfo(Buffer);

    // 5. 执行延伸操作
    try {
        TopoDS_Face extendedFace = BRepBuilderAPI_MakeFace(surface, u1, u2, v1, v2, Precision::Confusion());

        if (extendedFace.IsNull()) {
            Msg::ShowError("延伸操作失败！");
            return;
        }
        // 6. 获取偏置后的形状并显示
        h_MyViewer->Display(extendedFace);
        Handle(AIS_Shape) extendedFace_Ais = new AIS_Shape(extendedFace);
        // 7. 输入到工程树中
        ModelData model;
        model.shape = extendedFace_Ais;
        model.name = "扩展面";

        p_TreeWidget->addModelToTree(model); //将模型添加到工程树
        Msg::ShowInfo("面扩展成功！");
    }
    catch (Standard_Failure& e) {
        Msg::ShowError("延伸操作异常：");
        Msg::ShowError(e.GetMessageString());
    }
}

#include <BRepAdaptor_Curve.hxx>
#include <vector>
#include <array>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include "OCCT_GraphOperations.h"
#include <TopoDS_Wire.hxx>
#include "AngleDialog.h"
#include <3rdparty/opencascade-7.7.0/inc/StlAPI_Reader.hxx>
//added syj
namespace {
    constexpr double kToolFaceAngleToleranceDeg = 1.0;
    constexpr double kToolFaceAutoMatchAngleToleranceDeg = 3.0;
    constexpr double kToolFaceAutoMatchNormalDotTolerance = 0.95;
    constexpr double kToolFaceAutoMatchAreaLowerRatio = 0.3;
    constexpr double kToolFaceAutoMatchAreaUpperRatio = 3.0;
    constexpr double kEdgeLengthRatioTolerance = 0.25;
    constexpr double kEdgeParallelDotTolerance = 0.98;

    double clampCosValue(double value)
    {
        if (value < -1.0) return -1.0;
        if (value > 1.0) return 1.0;
        return value;
    }

    Handle(AIS_Shape) g_autoMatchedFacePreview;
    Handle(AIS_Shape) g_autoMatchedEdgePreview;

    /**
     * @brief 从当前 OCC 选择上下文中收集边对象。
     * @param context 当前激活的交互上下文，包含用户已选择的对象。
     * @param edgeVec 输出边列表，用于接收提取出的边。
     *
     * 对于直接选中的边，原样加入输出列表；对于复合体或实体，
     * 则遍历其内部全部边并加入输出列表。
     */
    void collectEdgesFromSelection(const Handle_AIS_InteractiveContext& context, std::vector<TopoDS_Edge>& edgeVec)
    {
        for (context->InitSelected(); context->MoreSelected(); context->NextSelected()) {
            const TopoDS_Shape shape = context->SelectedShape();
            if (shape.IsNull()) {
                continue;
            }

            if (shape.ShapeType() == TopAbs_EDGE) {
                edgeVec.push_back(TopoDS::Edge(shape));
                continue;
            }

            if (shape.ShapeType() == TopAbs_COMPOUND || shape.ShapeType() == TopAbs_SOLID) {
                for (TopExp_Explorer edgeExplorer(shape, TopAbs_EDGE); edgeExplorer.More(); edgeExplorer.Next()) {
                    edgeVec.push_back(TopoDS::Edge(edgeExplorer.Current()));
                }
            }
        }
    }

    /**
     * @brief 判断某条边是否已经被收集过。
     * @param edgeVec 已收集的边列表。
     * @param candidateEdge 待检测的候选边。
     * @return 如果 @p candidateEdge 已经存在于 @p edgeVec 中，则返回 true。
     */
    bool edgeAlreadyCollected(const std::vector<TopoDS_Edge>& edgeVec, const TopoDS_Edge& candidateEdge)
    {
        for (const TopoDS_Edge& existingEdge : edgeVec) {
            if (existingEdge.IsSame(candidateEdge)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 判断某个面是否属于给定面集合。
     * @param candidateFace 待检测的面。
     * @param faces 目标面集合。
     * @return 如果 @p candidateFace 与 @p faces 中某个面拓扑相同，则返回 true。
     */
    bool faceInCollection(const TopoDS_Face& candidateFace, const std::vector<TopoDS_Face>& faces)
    {
        for (const TopoDS_Face& face : faces) {
            if (face.IsSame(candidateFace)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 将一组面加入到复合体中。
     * @param faces 需要加入的面集合。
     * @param builder 用于向复合体追加图元的 OCC 构造器。
     * @param compound 接收这些面的目标复合体。
     */
    void addFacesToCompound(const std::vector<TopoDS_Face>& faces, BRep_Builder& builder, TopoDS_Compound& compound)
    {
        for (const TopoDS_Face& face : faces) {
            if (face.IsNull()) {
                continue;
            }
            builder.Add(compound, face);
        }
    }

    /**
     * @brief 收集单个面的有效边界边。
     * @param face 目标面。
     * @param edgeVec 输出边列表。
     */
    void collectValidFaceEdges(const TopoDS_Face& face, std::vector<TopoDS_Edge>& edgeVec)
    {
        for (TopExp_Explorer edgeExplorer(face, TopAbs_EDGE); edgeExplorer.More(); edgeExplorer.Next()) {
            const TopoDS_Edge edge = TopoDS::Edge(edgeExplorer.Current());
            if (edge.IsNull()) {
                continue;
            }

            if (BRep_Tool::Degenerated(edge)) {
                continue;
            }

            if (OCCT_GraphOperations::ComputeEdgeLength(edge) <= 1e-7) {
                continue;
            }

            if (!edgeAlreadyCollected(edgeVec, edge)) {
                edgeVec.push_back(edge);
            }
        }
    }

    /**
     * @brief 计算边中点处的切向信息。
     * @param edge 目标边。
     * @param midPoint 输出中点坐标。
     * @param tangent 输出中点切向。
     * @return 若成功提取中点和切向，则返回 true。
     */
    bool getEdgeMidPointAndTangent(const TopoDS_Edge& edge, gp_Pnt& midPoint, gp_Dir& tangent)
    {
        try {
            BRepAdaptor_Curve adaptorCurve(edge);
            const Standard_Real firstParam = adaptorCurve.FirstParameter();
            const Standard_Real lastParam = adaptorCurve.LastParameter();
            const Standard_Real midParam = 0.5 * (firstParam + lastParam);

            gp_Vec tangentVec;
            adaptorCurve.D1(midParam, midPoint, tangentVec);
            if (tangentVec.Magnitude() <= Precision::Confusion()) {
                return false;
            }

            tangent = gp_Dir(tangentVec);
            return true;
        }
        catch (Standard_Failure&) {
            return false;
        }
    }

    /**
     * @brief 计算两条边之间的最短距离。
     * @param edgeA 第一条边。
     * @param edgeB 第二条边。
     * @return 两条边的最短距离；若计算失败则返回一个很大的值。
     */
    double computeEdgeDistance(const TopoDS_Edge& edgeA, const TopoDS_Edge& edgeB)
    {
        try {
            BRepExtrema_DistShapeShape distTool(edgeA, edgeB);
            distTool.Perform();
            if (!distTool.IsDone()) {
                return 1.0e9;
            }
            return distTool.Value();
        }
        catch (Standard_Failure&) {
            return 1.0e9;
        }
    }

    /**
     * @brief 为一对前后刀面寻找距离最近且几何特征相近的边。
     * @param rakeFace 前刀面。
     * @param clearanceFace 后刀面。
     * @param matchedEdge 输出匹配到的候选切削刃。
     * @return 若成功找到近邻边对，则返回 true。
     *
     * 当两个面不存在直接共享边时，退化为在边界边中寻找长度相近、
     * 方向平行且距离很近的一对边，并任选前刀面上的那条边作为输出。
     */
    bool collectNearbyParallelEdgeFromFacePair(
        const TopoDS_Face& rakeFace,
        const TopoDS_Face& clearanceFace,
        TopoDS_Edge& matchedEdge)
    {
        std::vector<TopoDS_Edge> rakeEdges;
        std::vector<TopoDS_Edge> clearanceEdges;
        collectValidFaceEdges(rakeFace, rakeEdges);
        collectValidFaceEdges(clearanceFace, clearanceEdges);

        double bestDistance = 1.0e9;
        TopoDS_Edge bestEdge;

        for (const TopoDS_Edge& rakeEdge : rakeEdges) {
            const double rakeLength = OCCT_GraphOperations::ComputeEdgeLength(rakeEdge);
            gp_Pnt rakeMidPoint;
            gp_Dir rakeTangent;
            if (!getEdgeMidPointAndTangent(rakeEdge, rakeMidPoint, rakeTangent)) {
                continue;
            }

            for (const TopoDS_Edge& clearanceEdge : clearanceEdges) {
                const double clearanceLength = OCCT_GraphOperations::ComputeEdgeLength(clearanceEdge);
                const double maxLength = std::max(rakeLength, clearanceLength);
                if (maxLength <= Precision::Confusion()) {
                    continue;
                }

                const double relativeLengthDiff = std::fabs(rakeLength - clearanceLength) / maxLength;
                if (relativeLengthDiff > kEdgeLengthRatioTolerance) {
                    continue;
                }

                gp_Pnt clearanceMidPoint;
                gp_Dir clearanceTangent;
                if (!getEdgeMidPointAndTangent(clearanceEdge, clearanceMidPoint, clearanceTangent)) {
                    continue;
                }

                const double tangentDot =
                    std::fabs(rakeTangent.X() * clearanceTangent.X() +
                        rakeTangent.Y() * clearanceTangent.Y() +
                        rakeTangent.Z() * clearanceTangent.Z());
                if (tangentDot < kEdgeParallelDotTolerance) {
                    continue;
                }

                const double edgeDistance = computeEdgeDistance(rakeEdge, clearanceEdge);
                const double distanceTolerance = std::max(0.5, maxLength * 0.05);
                if (edgeDistance > distanceTolerance) {
                    continue;
                }

                if (edgeDistance < bestDistance) {
                    bestDistance = edgeDistance;
                    bestEdge = rakeEdge;
                }
            }
        }

        if (bestEdge.IsNull()) {
            return false;
        }

        matchedEdge = bestEdge;
        return true;
    }

    /**
     * @brief 从前刀面和后刀面集合中提取同时邻接二者的切削刃。
     * @param rakeFaces 候选前刀面集合。
     * @param clearanceFaces 候选后刀面集合。
     * @param edgeVec 输出边列表，用于接收识别出的切削刃。
     *
     * 该函数会先把前刀面和后刀面合并到一个临时复合体中，
     * 再建立“边-邻接面”映射，只保留同时邻接至少一个前刀面
     * 和至少一个后刀面的边，作为候选切削刃输出。
     */
    void collectSharedEdgesFromFaces(
        const std::vector<TopoDS_Face>& rakeFaces,
        const std::vector<TopoDS_Face>& clearanceFaces,
        std::vector<TopoDS_Edge>& edgeVec)
    {
        if (rakeFaces.empty() || clearanceFaces.empty()) {
            return;
        }

        BRep_Builder builder;
        TopoDS_Compound faceCompound;
        builder.MakeCompound(faceCompound);
        addFacesToCompound(rakeFaces, builder, faceCompound);
        addFacesToCompound(clearanceFaces, builder, faceCompound);

        TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
        TopExp::MapShapesAndAncestors(faceCompound, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

        for (int index = 1; index <= edgeFaceMap.Extent(); ++index) {
            const TopoDS_Edge edge = TopoDS::Edge(edgeFaceMap.FindKey(index));
            if (edge.IsNull()) {
                continue;
            }

            if (BRep_Tool::Degenerated(edge)) {
                continue;
            }

            if (OCCT_GraphOperations::ComputeEdgeLength(edge) <= 1e-7) {
                continue;
            }

            bool touchesRakeFace = false;
            bool touchesClearanceFace = false;
            const TopTools_ListOfShape& adjacentFaces = edgeFaceMap.FindFromIndex(index);
            for (TopTools_ListIteratorOfListOfShape it(adjacentFaces); it.More(); it.Next()) {
                const TopoDS_Face adjacentFace = TopoDS::Face(it.Value());
                if (!touchesRakeFace && faceInCollection(adjacentFace, rakeFaces)) {
                    touchesRakeFace = true;
                }
                if (!touchesClearanceFace && faceInCollection(adjacentFace, clearanceFaces)) {
                    touchesClearanceFace = true;
                }
                if (touchesRakeFace && touchesClearanceFace) {
                    break;
                }
            }

            if (!touchesRakeFace || !touchesClearanceFace) {
                continue;
            }

            if (!edgeAlreadyCollected(edgeVec, edge)) {
                edgeVec.push_back(edge);
            }
        }

        if (!edgeVec.empty()) {
            return;
        }

        for (const TopoDS_Face& rakeFace : rakeFaces) {
            for (const TopoDS_Face& clearanceFace : clearanceFaces) {
                TopoDS_Edge nearbyEdge;
                if (!collectNearbyParallelEdgeFromFacePair(rakeFace, clearanceFace, nearbyEdge)) {
                    continue;
                }

                if (!edgeAlreadyCollected(edgeVec, nearbyEdge)) {
                    edgeVec.push_back(nearbyEdge);
                }
            }
        }
    }

    /**
     * @brief 输出单个面的边摘要信息，便于调试切削刃识别。
     * @param face 待分析的面。
     * @param faceLabel 当前面的标签名称。
     */
    void logFaceEdgeSummary(const TopoDS_Face& face, const char* faceLabel)
    {
        int edgeCount = 0;
        for (TopExp_Explorer edgeExplorer(face, TopAbs_EDGE); edgeExplorer.More(); edgeExplorer.Next()) {
            ++edgeCount;
            const TopoDS_Edge edge = TopoDS::Edge(edgeExplorer.Current());
            const double edgeLength = edge.IsNull() ? 0.0 : OCCT_GraphOperations::ComputeEdgeLength(edge);
            Standard_Character buffer[1024] = { 0 };
            Sprintf(buffer, "%s 第%d条边长度：%.6f", faceLabel, edgeCount, edgeLength);
            Msg::ShowInfo(buffer);
        }

        Standard_Character summary[1024] = { 0 };
        Sprintf(summary, "%s 边数量：%d", faceLabel, edgeCount);
        Msg::ShowInfo(summary);
    }

    /**
     * @brief 输出两个面组合后的边邻接调试信息。
     * @param rakeFace 测试用前刀面。
     * @param clearanceFace 测试用后刀面。
     */
    void logFacePairAdjacencyDebug(const TopoDS_Face& rakeFace, const TopoDS_Face& clearanceFace)
    {
        BRep_Builder builder;
        TopoDS_Compound faceCompound;
        builder.MakeCompound(faceCompound);
        builder.Add(faceCompound, rakeFace);
        builder.Add(faceCompound, clearanceFace);

        TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
        TopExp::MapShapesAndAncestors(faceCompound, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

        Standard_Character header[1024] = { 0 };
        Sprintf(header, "两面组合后参与映射的边数量：%d", edgeFaceMap.Extent());
        Msg::ShowInfo(header);

        for (int index = 1; index <= edgeFaceMap.Extent(); ++index) {
            const TopoDS_Edge edge = TopoDS::Edge(edgeFaceMap.FindKey(index));
            if (edge.IsNull()) {
                continue;
            }

            const double edgeLength = OCCT_GraphOperations::ComputeEdgeLength(edge);
            bool touchesRakeFace = false;
            bool touchesClearanceFace = false;
            int adjacentFaceCount = 0;
            const TopTools_ListOfShape& adjacentFaces = edgeFaceMap.FindFromIndex(index);
            for (TopTools_ListIteratorOfListOfShape it(adjacentFaces); it.More(); it.Next()) {
                ++adjacentFaceCount;
                const TopoDS_Face adjacentFace = TopoDS::Face(it.Value());
                if (adjacentFace.IsSame(rakeFace)) {
                    touchesRakeFace = true;
                }
                if (adjacentFace.IsSame(clearanceFace)) {
                    touchesClearanceFace = true;
                }
            }

            Standard_Character buffer[1024] = { 0 };
            Sprintf(buffer,
                "映射边[%d] 长度=%.6f 邻接面数=%d 邻接前刀面=%d 邻接后刀面=%d",
                index,
                edgeLength,
                adjacentFaceCount,
                touchesRakeFace ? 1 : 0,
                touchesClearanceFace ? 1 : 0);
            Msg::ShowInfo(buffer);
        }
    }
}
//added syj
// 
//@brief 边离散 选择几条边，对每条边单独进行均匀离散
void MdiChild::EdgeDiscrete() {
    // 0. 设置选择模式为边
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
    // 1. 获取交互上下文
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (!myContext) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }
    // 2. 检查是否有选中的对象（修复：使用NbSelected()替代HasSelectedShape()，支持框选）
    int Selectcount = myContext->NbSelected();//获取选中对象的数量
    if (Selectcount == 0) {
        Msg::ShowInfo("请先选择一个边！");
        // 创建事件循环，等待用户选择边
        QEventLoop loop;
        QObject::connect(h_MyViewer->getSignals(), &MyViewerSignals::selectionChangedSignal, &loop, &QEventLoop::quit);
        loop.exec(); // 等待用户选择边
        Selectcount = myContext->NbSelected(); // 重新获取选中数量
        if (Selectcount == 0) {
            Msg::ShowWarning("未选中任何对象！");
            return;
        }
    }
    // 3. 遍历选中的对象，提取所有边（修复：支持框选多个对象）
    TopoDS_Edge selectEdge;
    Handle(AIS_Shape) sourceModel;
    bool hasEdge = false;
    // 修复：直接遍历，不需要while循环
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
        // 获取选中的形状
        TopoDS_Shape shape = myContext->SelectedShape();
        if (shape.IsNull()) {
            continue;
        }
        // 如果是边，直接使用
        if (shape.ShapeType() == TopAbs_EDGE) {
            selectEdge = TopoDS::Edge(shape);
            hasEdge = true;
            break; // 只取第一个边用于单边离散
        }
        // 如果选中的是复合形状或其他类型，尝试从中提取边
        else if (shape.ShapeType() == TopAbs_COMPOUND || shape.ShapeType() == TopAbs_SOLID) {
            TopExp_Explorer edgeExplorer(shape, TopAbs_EDGE);
            if (edgeExplorer.More()) {
                selectEdge = TopoDS::Edge(edgeExplorer.Current());
                hasEdge = true;
                break; // 0. 设置选择模式为边
                h_MyViewer->getAisContext()->Deactivate();
                h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
                // 1. 获取交互上下文
                Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
                if (!myContext) {
                    Msg::ShowError("交互上下文未初始化！");
                    return;
                }
                // 2. 检查是否有选中的对象（修复：使用NbSelected()替代HasSelectedShape()，支持框选）
                int Selectcount = myContext->NbSelected();//获取选中对象的数量
                if (Selectcount == 0) {
                    Msg::ShowInfo("请先选择一个边！");
                    // 创建事件循环，等待用户选择边
                    QEventLoop loop;
                    QObject::connect(h_MyViewer->getSignals(), &MyViewerSignals::selectionChangedSignal, &loop, &QEventLoop::quit);
                    loop.exec(); // 等待用户选择边
                    Selectcount = myContext->NbSelected(); // 重新获取选中数量
                    if (Selectcount == 0) {
                        Msg::ShowWarning("未选中任何对象！");
                        return;
                    }
                }
                // 3. 遍历选中的对象，提取所有边（修复：支持框选多个对象）
                TopoDS_Edge selectEdge;
                bool hasEdge = false;
                // 修复：直接遍历，不需要while循环
                for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
                    // 获取选中的形状
                    TopoDS_Shape shape = myContext->SelectedShape();
                    if (shape.IsNull()) {
                        continue;
                    }
                    // 如果是边，直接使用
                    if (shape.ShapeType() == TopAbs_EDGE) {
                        selectEdge = TopoDS::Edge(shape);
                        hasEdge = true;
                        break; // 只取第一个边用于单边离散
                    }
                    // 如果选中的是复合形状或其他类型，尝试从中提取边
                    else if (shape.ShapeType() == TopAbs_COMPOUND || shape.ShapeType() == TopAbs_SOLID) {
                        TopExp_Explorer edgeExplorer(shape, TopAbs_EDGE);
                        if (edgeExplorer.More()) {
                            selectEdge = TopoDS::Edge(edgeExplorer.Current());
                            hasEdge = true;
                            break;
                        }
                    }
                }

                if (!hasEdge) {
                    Msg::ShowWarning("未找到有效的边！请确保选中了边对象。");
                    return;
                }

                // 重新获取选中数量（可能在等待选择后变化）
                Selectcount = myContext->NbSelected();
            }
        }
    }
    if (!hasEdge) {
        Msg::ShowWarning("未找到有效的边！请确保选中了边对象。");
        return;
    }

    // 重新获取选中数量（可能在等待选择后变化）
    Selectcount = myContext->NbSelected();
    // 4. 设置离散数量
    bool ok;
    int numPoints = QInputDialog::getInt(this, "输入离散点数量", "个", 100, 1, 10000, 10, &ok); //获取离散点数量
    if (!ok) return;
    vector<array<double, 12>> pointsVec; //存储离散点的坐标
    // 5. 执行离散操作
    if (Selectcount == 1) {//如果只选中一个边
        try {
            BRepAdaptor_Curve adaptorCurve(selectEdge);
            Standard_Real startParam = adaptorCurve.FirstParameter();
            Standard_Real endParam = adaptorCurve.LastParameter();

            Standard_Real uStep = (endParam - startParam) / (numPoints - 1);
            for (Standard_Integer i = 0; i < numPoints; i++)
            {
                Standard_Real u = startParam + (i)*uStep;
                gp_Pnt point;
                gp_Vec dir;
                adaptorCurve.D1(u, point, dir); //获得点
                std::array<double, 3> xyz = { point.X(),point.Y(),point.Z() };
                std::array<double, 12> xyzabc = { point.X(),point.Y(),point.Z(),0,0,0,0,0 };
                Standard_Character Buffer[1024] = { 0 };
                Sprintf(Buffer, "离散点坐标为: X=%f,Y=%f,Z=%f", point.X(), point.Y(), point.Z());
                Msg::ShowInfo(Buffer);
                p_TreeWidget->pointsVec.push_back(xyz);
                vector<double> XYZDir = { point.X(),point.Y(),point.Z(),dir.X(),dir.Y(),dir.Z() };
                AngleDialog::pointsAndVec.push_back(XYZDir);
                pointsVec.push_back(xyzabc);
                TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);

                //PointToDir.SetX(point.X() + dir.X());                           /
                //PointToDir.SetY(point.Y() + dir.Y());
                //PointToDir.SetZ(point.Z() + dir.Z());
                //TopoDS_Vertex dirVertex = BRepBuilderAPI_MakeVertex(PointToDir);
                //h_MyViewer->AisObjColor(Quantity_NOC_RED);
                //h_MyViewer->Display(dirVertex); //显示方向向量
                Handle(AIS_Shape) pointAIS = new AIS_Shape(vertex);
                h_MyViewer->Display(pointAIS);

                // 记录离散点到原模型的映射关系
                if (!sourceModel.IsNull()) {
                    m_discretePointToModelMap[pointAIS] = sourceModel;
                }
            }
            p_TreeWidget->addPointToTree(pointsVec);
        }
        catch (Standard_Failure& e) {
            Msg::ShowError("离散操作异常：");
            Msg::ShowError(e.GetMessageString());
            return;
        }
    }
    else if (Selectcount > 1) {//如果选中多个边
        try {
            vector<TopoDS_Edge> edge_vec;
            vector<gp_Pnt> samplePoints;
            vector<gp_Dir> dirVec;
            for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
                TopoDS_Shape shape = myContext->SelectedShape();
                TopoDS_Edge Edge = TopoDS::Edge(shape);
                if (Edge.ShapeType() == TopAbs_EDGE) {
                    edge_vec.push_back(Edge);
                }
                TopoDS_Wire wire = OCCT_GraphOperations::ConnectEdges(edge_vec); //连接选中的边
                Msg::ShowInfo("wire connection sucuess.");
                // 在曲线上均匀采样20个点
                //OCCT_GraphOperations::SampleWireUniformly(samplePoints, dirVec, wire, numPoints);

                BRepAdaptor_Curve adaptorCurve(Edge);
                Standard_Real startParam = adaptorCurve.FirstParameter();
                Standard_Real endParam = adaptorCurve.LastParameter();

                Standard_Real uStep = (endParam - startParam) / (numPoints - 1);
                for (Standard_Integer i = 0; i < numPoints; i++)
                {
                    Standard_Real u = startParam + (i)*uStep;
                    gp_Pnt point;
                    gp_Vec dir;
                    adaptorCurve.D1(u, point, dir); //获得点
                    std::array<double, 3> xyz = { point.X(),point.Y(),point.Z() };
                    std::array<double, 12> xyzabc = { point.X(),point.Y(),point.Z(),0,0,0,0,0 };
                    Standard_Character Buffer[1024] = { 0 };
                    Sprintf(Buffer, "离散点坐标为: X=%f,Y=%f,Z=%f", point.X(), point.Y(), point.Z());
                    Msg::ShowInfo(Buffer);
                    p_TreeWidget->pointsVec.push_back(xyz);
                    vector<double> XYZDir = { point.X(),point.Y(),point.Z(),dir.X(),dir.Y(),dir.Z() };
                    AngleDialog::pointsAndVec.push_back(XYZDir);
                    pointsVec.push_back(xyzabc);
                    TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);

                    //PointToDir.SetX(point.X() + dir.X());                           
                    //PointToDir.SetY(point.Y() + dir.Y());
                    //PointToDir.SetZ(point.Z() + dir.Z());
                    //TopoDS_Vertex dirVertex = BRepBuilderAPI_MakeVertex(PointToDir);
                    //h_MyViewer->AisObjColor(Quantity_NOC_RED);
                    //h_MyViewer->Display(dirVertex); //显示方向向量
                    h_MyViewer->Display(vertex);
                }
            }
            //for (const gp_Pnt& point : samplePoints) {
            //    std::array<double, 3> xyz = { point.X(), point.Y(), point.Z() };
            //    Standard_Character Buffer[1024] = { 0 };
            //    Sprintf(Buffer, "离散点坐标为: X=%f, Y=%f, Z=%f", point.X(), point.Y(), point.Z());
            //    Msg::ShowInfo(Buffer);
            //    pointsVec.push_back(xyz);
            //    p_TreeWidget->pointsVec.push_back(xyz);
            //    TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);
            //    h_MyViewer->Display(vertex);   //显示离散点
            //}
            p_TreeWidget->addPointToTree(pointsVec);
        }
        catch (Standard_Failure& e) {
            Msg::ShowError("离散操作异常：未选择连续的边");
            Msg::ShowError(e.GetMessageString());
            return;
        }
    }
    else {
        Msg::ShowError("没有选中任何边！");
        return;
    }

    ////  输入到工程树中
    //ModelData model;
    //model.shape = extendedFace;
    //model.name = "离散点";

    //p_TreeWidget->addModelToTree(model); //将模型添加到工程树
}

//@brief 边离散 选择几条边，将所有边连接成一条线，对整条线进行均匀离散
void MdiChild::EdgeConnectDiscrete()
{
    p_TreeWidget->pointsVec.clear();
    AngleDialog::pointsAndVec.clear();
    AngleDialog::groupedPointsAndVec.clear();

    // 0. 设置选择模式为边
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
    // 1. 获取交互上下文
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (!myContext) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }
    std::vector<TopoDS_Edge> edge_vec;
    const bool hasFaces = !AngleDialog::rakeFaces.empty() && !AngleDialog::clearanceFaces.empty();
    if (hasFaces) {
        collectSharedEdgesFromFaces(AngleDialog::rakeFaces, AngleDialog::clearanceFaces, edge_vec);
        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "根据前后刀面自动捕捉到切削刃数量：%i", static_cast<int>(edge_vec.size()));
        Msg::ShowInfo(Buffer);
    }

    if (edge_vec.empty()) {
        int Selectcount = myContext->NbSelected();//获取选中对象的数量
        if (Selectcount == 0) {
            Msg::ShowInfo("请先选择一个边！");
            QEventLoop loop;
            QObject::connect(h_MyViewer->getSignals(), &MyViewerSignals::selectionChangedSignal, &loop, &QEventLoop::quit);
            loop.exec();
            Selectcount = myContext->NbSelected();
            if (Selectcount == 0) {
                Msg::ShowWarning("未选中任何对象！");
                return;
            }
        }

        collectEdgesFromSelection(myContext, edge_vec);
        if (edge_vec.empty()) {
            Msg::ShowWarning("未找到有效的边！请确保选中了边对象。");
            return;
        }
    }

    highlightEdgesInContext(edge_vec);

    // 4. 设置离散数量
    bool ok;
    int numPoints = QInputDialog::getInt(this, "输入离散点数量", "个", 100, 1, 10000, 10, &ok); //获取离散点数量
    if (!ok) return;
    // 5. 执行离散操作
    if (edge_vec.size() == 1) {//如果只选中一个边
        try {
            const TopoDS_Edge selectEdge = edge_vec.front();
            BRepAdaptor_Curve adaptorCurve(selectEdge);
            Standard_Real startParam = adaptorCurve.FirstParameter();
            Standard_Real endParam = adaptorCurve.LastParameter();
            std::vector<std::vector<double>> edgePointGroup;

            Standard_Real uStep = (endParam - startParam) / (numPoints - 1);
            for (Standard_Integer i = 0; i < numPoints; i++)
            {
                Standard_Real u = startParam + (i)*uStep;
                gp_Pnt point;
                gp_Vec dir;
                adaptorCurve.D1(u, point, dir); //获得点
                Standard_Character Buffer[1024] = { 0 };
                Sprintf(Buffer, "离散点坐标为: X=%f,Y=%f,Z=%f", point.X(), point.Y(), point.Z());
                Msg::ShowInfo(Buffer);
                appendDiscretePoint(point, dir, p_TreeWidget->pointsVec, AngleDialog::pointsAndVec, edgePointGroup);
                TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);

                //PointToDir.SetX(point.X() + dir.X());                           /
                //PointToDir.SetY(point.Y() + dir.Y());
                //PointToDir.SetZ(point.Z() + dir.Z());
                //TopoDS_Vertex dirVertex = BRepBuilderAPI_MakeVertex(PointToDir);
                //h_MyViewer->AisObjColor(Quantity_NOC_RED);
                //h_MyViewer->Display(dirVertex); //显示方向向量
                h_MyViewer->Display(vertex);
                // 以下代码为等待时间，每个点等待时间为3/numPoints秒
                QMutex mutex;
                QWaitCondition waitCondition;
                mutex.lock();
                waitCondition.wait(&mutex, 3000 / numPoints);
                mutex.unlock();
            }
            AngleDialog::groupedPointsAndVec.push_back(edgePointGroup);
        }
        catch (Standard_Failure& e) {
            Msg::ShowError("离散操作异常：");
            Msg::ShowError(e.GetMessageString());
            return;
        }
    }
    else {//如果选中多个边
        try {
            vector<gp_Pnt> samplePoints;
            vector<gp_Dir> dirVec;
            std::vector<TopoDS_Wire> wires = OCCT_GraphOperations::ConnectDisorderEdgesToWires(edge_vec); //连接选中的边
            if (wires.empty()) {
                Msg::ShowInfo("wire connection failed."); return;
            }
            else {
                Standard_Character Buffer[1024] = { 0 };
                Sprintf(Buffer, "wire connection success. wire count = %i", static_cast<int>(wires.size()));
                Msg::ShowInfo(Buffer);
            }

            for (int wireIndex = 0; wireIndex < wires.size(); ++wireIndex) {
                vector<pair<gp_Pnt, gp_Vec>> results;
                TopoDS_Wire& wire = wires[wireIndex];
                std::vector<std::vector<double>> edgePointGroup;
                OCCT_GraphOperations::SampleWireUniformly(results, wire, numPoints);
                for (int i = 0; i < results.size(); ++i)
                {
                    gp_Pnt point = results[i].first;
                    gp_Dir dir = results[i].second;
                    appendDiscretePoint(point, dir, p_TreeWidget->pointsVec, AngleDialog::pointsAndVec, edgePointGroup);
                    TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);

                    h_MyViewer->Display(vertex);
                    QMutex mutex;
                    QWaitCondition waitCondition;
                    mutex.lock();
                    waitCondition.wait(&mutex, 3000 / numPoints);
                    mutex.unlock();
                }
                if (!edgePointGroup.empty()) {
                    AngleDialog::groupedPointsAndVec.push_back(edgePointGroup);
                }
            }
        }
        catch (Standard_Failure& e) {
            Msg::ShowError("离散操作异常：未选择连续的边");
            Msg::ShowError(e.GetMessageString());
            return;
        }
    }
    int reverse = QInputDialog::getInt(this, "是否需要反向", "（0：不需要，1：需要）", 0, 0, 1, 1, &ok); //获取离散点数量
    if (reverse)
    {
        std::reverse(p_TreeWidget->pointsVec.begin(), p_TreeWidget->pointsVec.end());
        for (auto& edgeGroup : AngleDialog::groupedPointsAndVec) {
            std::reverse(edgeGroup.begin(), edgeGroup.end());
        }
        rebuildFlatPointDataFromGroups(AngleDialog::groupedPointsAndVec, AngleDialog::pointsAndVec);
    }

}


void MdiChild::clearDiscretePoints()
{
    Msg::ShowInfo("清除离散点和前后面");
    p_TreeWidget->pointsVec.clear();
    AngleDialog::pointsAndVec.clear();
    AngleDialog::groupedPointsAndVec.clear();
    AngleDialog::rakeFaces.clear();
    AngleDialog::clearanceFaces.clear();
    h_MyViewer->Redraw();
    if (!g_autoMatchedFacePreview.IsNull()) {
        h_MyViewer->getAisContext()->Remove(g_autoMatchedFacePreview, Standard_False);
        g_autoMatchedFacePreview.Nullify();
    }
    if (!g_autoMatchedEdgePreview.IsNull()) {
        h_MyViewer->getAisContext()->Remove(g_autoMatchedEdgePreview, Standard_False);
        g_autoMatchedEdgePreview.Nullify();
    }

}

void MdiChild::InputMaterialProperty()
{
    MaterialDialog dialog(this, &m_materialProperties);
    if (dialog.exec() == QDialog::Accepted) {
        Standard_Character Buffer[1024] = { 0 };
        sprintf(Buffer, "材料参数已保存，共 %1 种材料", m_materialProperties.size());
        Msg::ShowInfo(Buffer);
    }
}
//added by syj
bool MdiChild::getRepresentativeFaceNormal(const TopoDS_Face& face, gp_Dir& normal) const
{
    Standard_Real uMin = 0.0;
    Standard_Real uMax = 0.0;
    Standard_Real vMin = 0.0;
    Standard_Real vMax = 0.0;
    BRepTools::UVBounds(face, uMin, uMax, vMin, vMax);
    const Standard_Real uMid = 0.5 * (uMin + uMax);
    const Standard_Real vMid = 0.5 * (vMin + vMax);
    BRepGProp_Face faceProps(face);
    gp_Pnt samplePoint;
    gp_Vec sampleNormal;
    faceProps.Normal(uMid, vMid, samplePoint, sampleNormal);
    if (sampleNormal.Magnitude() <= Precision::Confusion()) {
        return false;
    }
    normal = gp_Dir(sampleNormal);
    return true;
}
double MdiChild::computeFaceDirectionAngleDeg(const TopoDS_Face& face, const gp_Dir& cuttingDir) const
{
    gp_Dir faceNormal;
    if (!getRepresentativeFaceNormal(face, faceNormal)) {
        return 1.0e9;
    }
    const double cosValue = clampCosValue(faceNormal.X() * cuttingDir.X()
        + faceNormal.Y() * cuttingDir.Y()
        + faceNormal.Z() * cuttingDir.Z());
    const double normalAngle = std::acos(cosValue);
    const double faceAngle = std::fabs((M_PI / 2.0) - normalAngle);
    return faceAngle * 180.0 / M_PI;
}


void MdiChild::highlightFacesInContext(const std::vector<TopoDS_Face>& faces)
{
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (myContext.IsNull()) {
        QMessageBox::warning(this, "Auto Select", "AIS context is null.");
        return;
    }

    if (!g_autoMatchedFacePreview.IsNull()) {
        myContext->Remove(g_autoMatchedFacePreview, Standard_False);
        g_autoMatchedFacePreview.Nullify();
    }

    if (faces.empty()) {
        myContext->UpdateCurrentViewer();
        Msg::ShowWarning("highlightFacesInContext: no faces to preview.");
        return;
    }

    TopoDS_Compound faceCompound;
    BRep_Builder builder;
    builder.MakeCompound(faceCompound);

    for (const TopoDS_Face& face : faces) {
        if (!face.IsNull()) {
            builder.Add(faceCompound, face);
        }
    }

    g_autoMatchedFacePreview = new AIS_Shape(faceCompound);
    g_autoMatchedFacePreview->SetDisplayMode(AIS_Shaded);
    g_autoMatchedFacePreview->SetColor(Quantity_Color(Quantity_NOC_RED));
    g_autoMatchedFacePreview->SetTransparency(0.15f);

    myContext->Display(g_autoMatchedFacePreview, Standard_False);
    myContext->Redisplay(g_autoMatchedFacePreview, Standard_False);
    myContext->UpdateCurrentViewer();

    Standard_Character buffer[1024] = { 0 };
    Sprintf(buffer, "highlight preview faces: %d", static_cast<int>(faces.size()));
    Msg::ShowInfo(buffer);
}

void MdiChild::highlightEdgesInContext(const std::vector<TopoDS_Edge>& edges)
{
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (myContext.IsNull()) {
        QMessageBox::warning(this, "Edge Test", "AIS context is null.");
        return;
    }

    if (!g_autoMatchedEdgePreview.IsNull()) {
        myContext->Remove(g_autoMatchedEdgePreview, Standard_False);
        g_autoMatchedEdgePreview.Nullify();
    }

    if (edges.empty()) {
        myContext->UpdateCurrentViewer();
        Msg::ShowWarning("highlight preview edges: no edges to preview.");
        return;
    }

    TopoDS_Compound edgeCompound;
    BRep_Builder builder;
    builder.MakeCompound(edgeCompound);

    for (const TopoDS_Edge& edge : edges) {
        if (!edge.IsNull()) {
            builder.Add(edgeCompound, edge);
        }
    }

    g_autoMatchedEdgePreview = new AIS_Shape(edgeCompound);
    g_autoMatchedEdgePreview->SetDisplayMode(AIS_WireFrame);
    g_autoMatchedEdgePreview->SetColor(Quantity_Color(Quantity_NOC_GREEN));
    g_autoMatchedEdgePreview->SetWidth(3.0);

    myContext->Display(g_autoMatchedEdgePreview, Standard_False);
    myContext->Redisplay(g_autoMatchedEdgePreview, Standard_False);
    myContext->UpdateCurrentViewer();

    Standard_Character buffer[1024] = { 0 };
    Sprintf(buffer, "highlight preview edges: %d", static_cast<int>(edges.size()));
    Msg::ShowInfo(buffer);
}

bool MdiChild::collectAutoMatchedToolFaces(std::vector<TopoDS_Face>& targetFaces, const char* faceLabel)
{
    targetFaces.clear();

    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (myContext.IsNull()) {
        QMessageBox::warning(this, "Auto Select", "AIS context is not initialized.");
        return false;
    }

    if (myContext->NbSelected() == 0) {
        QMessageBox::warning(this, "Auto Select", "Please preselect one seed face.");
        return false;
    }

    Handle(AIS_Shape) selectedShape = Handle(AIS_Shape)::DownCast(myContext->SelectedInteractive());
    if (selectedShape.IsNull()) {
        QMessageBox::warning(this, "Auto Select", "Selected interactive object is not an AIS_Shape.");
        return false;
    }

    gp_Dir cuttingDir(-1.0, 0.0, 0.0);
    if (AngleDialog::m_cuttingVec.Magnitude() > Precision::Confusion()) {
        cuttingDir = gp_Dir(AngleDialog::m_cuttingVec);
    }

    auto computeFaceArea = [](const TopoDS_Face& face) -> double
    {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        return props.Mass();
    };

    struct SeedFeature
    {
        double faceAngleDeg;
        gp_Dir faceNormal;
        double faceArea;
    };

    std::vector<SeedFeature> seedFeatures;
    int seedFaceCount = 0;

    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
        TopoDS_Shape currentShape = myContext->SelectedShape();
        if (currentShape.IsNull() || currentShape.ShapeType() != TopAbs_FACE) {
            continue;
        }

        ++seedFaceCount;

        TopoDS_Face seedFace = TopoDS::Face(currentShape);
        gp_Dir seedNormal;
        if (!getRepresentativeFaceNormal(seedFace, seedNormal)) {
            continue;
        }

        const double seedAngleDeg = computeFaceDirectionAngleDeg(seedFace, cuttingDir);
        if (seedAngleDeg > 360.0) {
            continue;
        }

        const double seedArea = computeFaceArea(seedFace);
        seedFeatures.push_back({ seedAngleDeg, seedNormal, seedArea });
    }

    if (seedFeatures.empty()) {
        QMessageBox::warning(this, "Auto Select", "No valid seed face was collected.");
        return false;
    }

    TopoDS_Shape ownerShape = selectedShape->Shape();
    int exploredFaceCount = 0;

    struct CandidateFeature
    {
        TopoDS_Face face;
        double faceAngleDeg;
        gp_Dir faceNormal;
        double faceArea;
    };

    std::vector<CandidateFeature> candidateFeatures;
    for (TopExp_Explorer explorer(ownerShape, TopAbs_FACE); explorer.More(); explorer.Next()) {
        ++exploredFaceCount;

        TopoDS_Face candidateFace = TopoDS::Face(explorer.Current());
        gp_Dir candidateNormal;
        if (!getRepresentativeFaceNormal(candidateFace, candidateNormal)) {
            continue;
        }

        const double candidateAngleDeg = computeFaceDirectionAngleDeg(candidateFace, cuttingDir);
        if (candidateAngleDeg > 360.0) {
            continue;
        }

        const double candidateArea = computeFaceArea(candidateFace);
        candidateFeatures.push_back({ candidateFace, candidateAngleDeg, candidateNormal, candidateArea });
    }

    std::vector<SeedFeature> matchedFeatures = seedFeatures;
    bool addedInPass = true;
    while (addedInPass) {
        addedInPass = false;

        for (const CandidateFeature& candidate : candidateFeatures) {
            bool alreadyAdded = false;
            for (const TopoDS_Face& existingFace : targetFaces) {
                if (existingFace.IsSame(candidate.face)) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (alreadyAdded) {
                continue;
            }

            bool matched = false;
            for (const SeedFeature& feature : matchedFeatures) {
                const bool angleMatched =
                    std::fabs(candidate.faceAngleDeg - feature.faceAngleDeg) <= kToolFaceAutoMatchAngleToleranceDeg;

                const double normalDot =
                    candidate.faceNormal.X() * feature.faceNormal.X() +
                    candidate.faceNormal.Y() * feature.faceNormal.Y() +
                    candidate.faceNormal.Z() * feature.faceNormal.Z();

                const bool normalMatched = normalDot >= kToolFaceAutoMatchNormalDotTolerance;

                const bool areaMatched =
                    feature.faceArea <= Precision::Confusion() ||
                    (candidate.faceArea >= feature.faceArea * kToolFaceAutoMatchAreaLowerRatio &&
                        candidate.faceArea <= feature.faceArea * kToolFaceAutoMatchAreaUpperRatio);

                if (angleMatched && normalMatched && areaMatched) {
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                continue;
            }

            targetFaces.push_back(candidate.face);
            matchedFeatures.push_back({ candidate.faceAngleDeg, candidate.faceNormal, candidate.faceArea });
            addedInPass = true;
        }
    }

    highlightFacesInContext(targetFaces);

    Standard_Character buffer[1024] = { 0 };
    Sprintf(buffer, "%s auto matched: seed=%i explored=%i matched=%i",
        faceLabel, seedFaceCount, exploredFaceCount, static_cast<int>(targetFaces.size()));
    Msg::ShowInfo(buffer);
    QMessageBox::information(this, "Auto Select Result", QString::fromLocal8Bit(buffer));

    return !targetFaces.empty();
}

void MdiChild::TestSelectedFacesCuttingEdge()
{
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (myContext.IsNull()) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }

    myContext->Deactivate();
    myContext->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));

    std::vector<TopoDS_Face> selectedFaces;
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
        const TopoDS_Shape shape = myContext->SelectedShape();
        if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE) {
            continue;
        }
        selectedFaces.push_back(TopoDS::Face(shape));
    }

    if (selectedFaces.size() != 2) {
        Msg::ShowWarning("请先手动选中两个面，再点击测试按钮。");
        QMessageBox::warning(this, "切削刃测试", "请先手动选中两个面，再点击测试按钮。");
        return;
    }

    std::vector<TopoDS_Edge> edgeVec;
    std::vector<TopoDS_Face> rakeFaces = { selectedFaces[0] };
    std::vector<TopoDS_Face> clearanceFaces = { selectedFaces[1] };
    Msg::ShowInfo("开始调试当前手动选中的两个面...");
    logFaceEdgeSummary(rakeFaces[0], "手动选中面1");
    logFaceEdgeSummary(clearanceFaces[0], "手动选中面2");
    logFacePairAdjacencyDebug(rakeFaces[0], clearanceFaces[0]);
    collectSharedEdgesFromFaces(rakeFaces, clearanceFaces, edgeVec);

    if (edgeVec.empty()) {
        std::swap(rakeFaces[0], clearanceFaces[0]);
        Msg::ShowInfo("第一次未识别到切削刃，交换两个面的前后角色后重试...");
        logFacePairAdjacencyDebug(rakeFaces[0], clearanceFaces[0]);
        collectSharedEdgesFromFaces(rakeFaces, clearanceFaces, edgeVec);
    }

    highlightEdgesInContext(edgeVec);

    Standard_Character buffer[1024] = { 0 };
    Sprintf(buffer, "手动测试两面选刃结果：%d", static_cast<int>(edgeVec.size()));
    Msg::ShowInfo(buffer);
    QMessageBox::information(this, "切削刃测试结果", QString::fromLocal8Bit(buffer));
}




//added by syj

void MdiChild::CaptureRakeFace()
{
    AngleDialog::rakeFaces.clear();

    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (myContext.IsNull()) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }

    // 保证当前是面选择模式
    myContext->Deactivate();
    myContext->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));

    // 先尝试：基于当前选中的“标准前刀面”自动匹配全部前刀面
    std::vector<TopoDS_Face> autoMatchedFaces;
    if (collectAutoMatchedToolFaces(autoMatchedFaces, "前刀面"))
    {
        AngleDialog::rakeFaces = autoMatchedFaces;

        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "自动捕捉到前刀面数量为：%i",
            static_cast<int>(AngleDialog::rakeFaces.size()));
        Msg::ShowInfo(Buffer);
        return;
    }

    // 如果自动匹配失败，再退回原来的手动多选捕捉逻辑
    if (myContext->NbSelected() == 0) {
        Msg::ShowWarning("请先选中一个标准前刀面！");
        return;
    }

    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
        TopoDS_Shape shape = myContext->SelectedShape();
        if (shape.IsNull()) {
            continue;
        }

        if (shape.ShapeType() == TopAbs_FACE) {
            AngleDialog::rakeFaces.push_back(TopoDS::Face(shape));
        }
    }

    Standard_Character Buffer[1024] = { 0 };
    Sprintf(Buffer, "手动捕捉到前刀面数量为：%i",
        static_cast<int>(AngleDialog::rakeFaces.size()));
    Msg::ShowInfo(Buffer);
}

void MdiChild::CaptureClearanceFace()
{
    AngleDialog::clearanceFaces.clear();

    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (myContext.IsNull()) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }

    // 保证当前是面选择模式
    myContext->Deactivate();
    myContext->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));

    // 先尝试：基于当前选中的“标准后刀面”自动匹配全部后刀面
    std::vector<TopoDS_Face> autoMatchedFaces;
    if (collectAutoMatchedToolFaces(autoMatchedFaces, "后刀面"))
    {
        AngleDialog::clearanceFaces = autoMatchedFaces;

        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "自动捕捉到后刀面数量为：%i",
            static_cast<int>(AngleDialog::clearanceFaces.size()));
        Msg::ShowInfo(Buffer);
        return;
    }

    // 如果自动匹配失败，再退回原来的手动多选捕捉逻辑
    if (myContext->NbSelected() == 0) {
        Msg::ShowWarning("请先选中一个标准后刀面！");
        return;
    }

    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
        TopoDS_Shape shape = myContext->SelectedShape();
        if (shape.IsNull()) {
            continue;
        }

        if (shape.ShapeType() == TopAbs_FACE) {
            AngleDialog::clearanceFaces.push_back(TopoDS::Face(shape));
        }
    }

    Standard_Character Buffer[1024] = { 0 };
    Sprintf(Buffer, "手动捕捉到后刀面数量为：%i",
        static_cast<int>(AngleDialog::clearanceFaces.size()));
    Msg::ShowInfo(Buffer);
}



void MdiChild::GetCondition()
{
    Msg::ShowInfo("计算切削工况");
    AngleDialog* dialog = new AngleDialog(this, q3dView);
    connect(dialog, &AngleDialog::selRakeFaceSignal, this, &MdiChild::CaptureRakeFace);
    connect(dialog, &AngleDialog::selClearanceFaceSignal, this, &MdiChild::CaptureClearanceFace);
    dialog->show(); //非模态显示
    dialog->exec();

}

void MdiChild::ComputeConditions()
{
    Msg::ShowInfo("开始计算切削工况...");
    std::vector<std::vector<std::vector<double>>> sourceGroups = AngleDialog::groupedPointsAndVec;
    if (sourceGroups.empty() && !AngleDialog::pointsAndVec.empty()) {
        sourceGroups.push_back(AngleDialog::pointsAndVec);
    }
    if (sourceGroups.empty()) {
        Msg::ShowWarning("没有可计算的切削刃离散点，请先进行离散。");
        return;
    }

    const QString exportFolder = QFileDialog::getExistingDirectory(
        this,
        tr("选择切削刃导出文件夹"),
        QDir::currentPath());
    const QString cuttingEdgeFileBaseName = "multi_broaching_100";
    if (!exportFolder.isEmpty()) {
        QDir exportDir(exportFolder);
        exportDir.mkpath(".");
    }

    p_TreeWidget->anglesVec.clear();
    AngleDialog::pointsAndVec.clear();
    AngleDialog::groupedPointsAndVec.clear();
    int exportedFileIndex = 1;

    // 创建面分类器
    BRepClass_FaceClassifier classifier1;
    BRepClass_FaceClassifier classifier2;

    for (const auto& sourceGroup : sourceGroups) {
        std::vector<std::vector<double>> computedGroup;
        std::vector<std::array<double, 12>> pointsVec;
        computedGroup.reserve(sourceGroup.size());
        pointsVec.reserve(sourceGroup.size());

        for (const auto& sourcePoint : sourceGroup) {
            std::vector<double> pointData = sourcePoint;
            trimPointComputedFields(pointData);

            bool hasRakeFace = false;
            bool hasClearanceFace = false;
            gp_Pnt pnt(pointData[0], pointData[1], pointData[2]);
            gp_Dir rakefacenormal;
            vector<TopoDS_Face> m_rakeFaces, m_clearanceFaces;
            TopoDS_Face m_rakeFace, m_clearanceFace;
            int rakefaceID = -1;
            for (const TopoDS_Face& rakeFace : AngleDialog::rakeFaces) {
                rakefaceID++;
                classifier1.Perform(rakeFace, pnt, 1e-3);
                if (classifier1.State() == TopAbs_IN || classifier1.State() == TopAbs_ON)
                {
                    m_rakeFace = rakeFace;
                    Handle(Geom_Surface) surface = BRep_Tool::Surface(rakeFace);
                    Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(surface);
                    if (!plane.IsNull()) {
                        rakefacenormal = plane->Axis().Direction();
                    }
                    m_rakeFaces.push_back(m_rakeFace);
                    break;
                }
            }

            if (m_rakeFaces.empty()) {
                if (findNearestFaceByPointProjection(AngleDialog::rakeFaces, pnt, 0.5, m_rakeFace)) {
                    Handle(Geom_Surface) surface = BRep_Tool::Surface(m_rakeFace);
                    Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(surface);
                    if (!plane.IsNull()) {
                        rakefacenormal = plane->Axis().Direction();
                    }
                    m_rakeFaces.push_back(m_rakeFace);
                    Msg::ShowInfo("点未落在前刀面上，已使用最近前刀面进行计算。");
                }
                else {
                    Msg::ShowError("该点前刀面获取失败，请重新选择面");
                    continue;
                }
            }
            for (const TopoDS_Face& clearanceFace : AngleDialog::clearanceFaces) {
                classifier2.Perform(clearanceFace, pnt, 1e-3);
                if (classifier2.State() == TopAbs_IN || classifier2.State() == TopAbs_ON)
                {
                    m_clearanceFace = clearanceFace;
                    m_clearanceFaces.push_back(m_clearanceFace);
                }
            }
            if (m_clearanceFaces.empty()) {
                if (findNearestFaceByPointProjection(AngleDialog::clearanceFaces, pnt, 0.5, m_clearanceFace)) {
                    m_clearanceFaces.push_back(m_clearanceFace);
                    Msg::ShowInfo("点未落在后刀面上，已使用最近后刀面进行计算。");
                }
                else {
                    Msg::ShowError("该点后刀面获取失败，请重新选择面");
                    continue;
                }
            }

            gp_Vec normalVec(pointData[3], pointData[4], pointData[5]);
            gp_Dir edgeTangentDir(normalVec);
            gp_Pln normalPlane(pnt, edgeTangentDir);

            TopoDS_Edge intersectionRakeEdge;
            for (int j = 0; j < m_rakeFaces.size(); j++)
            {
                m_rakeFace = m_rakeFaces[j];
                hasRakeFace = buildNormalPlaneSectionEdge(normalPlane, m_rakeFace, intersectionRakeEdge);
                if (hasRakeFace) break;
            }
            if (!hasRakeFace) {
                Msg::ShowError("法平面与前刀面求交失败，请重新选择面");
                continue;
            }

            gp_Vec crossA = AngleDialog::m_cuttingVec.Crossed(edgeTangentDir);
            double rakeAngle = -180.0;
            if (hasRakeFace) {
                rakeAngle = OCCT_GraphOperations::ComputeMinAngle(crossA, intersectionRakeEdge, 1e-3);
                rakeAngle = rakeAngle / M_PI * 180;
            }
            Standard_Character Buffer[1024] = { 0 };
            Sprintf(Buffer, "刀具前角：%f", rakeAngle);
            Msg::ShowInfo(Buffer);

            TopoDS_Edge intersectionClearanceEdge;
            for (int j = 0; j < m_clearanceFaces.size(); j++)
            {
                m_clearanceFace = m_clearanceFaces[j];
                hasClearanceFace = buildNormalPlaneSectionEdge(normalPlane, m_clearanceFace, intersectionClearanceEdge);
                if (hasClearanceFace) break;
            }

            gp_Vec crossB = AngleDialog::m_cuttingVec.Crossed(crossA);
            double inclination_angle = acos(crossB.Dot(edgeTangentDir) / crossB.Magnitude()) / M_PI * 180;
            if (inclination_angle > 90) inclination_angle = 180 - inclination_angle;
            double clearanceAngle = -180.0;
            if (hasClearanceFace) {
                clearanceAngle = OCCT_GraphOperations::ComputeMinAngle(crossA, intersectionClearanceEdge, 1e-3);
                clearanceAngle = 90 - clearanceAngle / M_PI * 180;
            }
            Sprintf(Buffer, "刀具后角：%f", clearanceAngle);
            Msg::ShowInfo(Buffer);

            pointData.push_back(inclination_angle);
            pointData.push_back(rakeAngle);
            pointData.push_back(clearanceAngle);

            gp_Vec depth_dir = AngleDialog::m_cuttingVec.Crossed(edgeTangentDir);
            double depth = 0.0;

            if (AngleDialog::m_cuttingDepDir2.X())
            {
                if (pnt.X() > 0)
                    depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                        + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                        + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude();
                else
                    depth = abs(-depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                        + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                        + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude();
            }
            if (AngleDialog::m_cuttingDepDir2.Y())
            {
                if (pnt.Y() > 0)
                    depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                        + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                        + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude();
                else
                    depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                        - depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                        + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude();
            }
            if (AngleDialog::m_cuttingDepDir2.Z())
            {
                if (pnt.Z() > 0)
                    depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                        + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                        + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude();
                else
                    depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                        + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                        - depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude();
            }
            pointData.push_back(AngleDialog::m_cuttingVec.Magnitude());
            pointData.push_back(depth);
            pointData.push_back(rakefaceID);
            pointData.push_back(rakefacenormal.X());
            pointData.push_back(rakefacenormal.Y());
            pointData.push_back(rakefacenormal.Z());

            std::array<double, 12> xyzab = buildConditionPointRow(pointData);
            p_TreeWidget->anglesVec.push_back(xyzab);
            pointsVec.push_back(xyzab);
            computedGroup.push_back(pointData);
        }

        if (!computedGroup.empty()) {
            AngleDialog::groupedPointsAndVec.push_back(computedGroup);
            AngleDialog::pointsAndVec.insert(AngleDialog::pointsAndVec.end(), computedGroup.begin(), computedGroup.end());
            p_TreeWidget->addPointToTree(pointsVec);
            if (!exportFolder.isEmpty()) {
                exportCuttingEdgePointsToFile(pointsVec, exportFolder, exportedFileIndex, cuttingEdgeFileBaseName);
            }
            ++exportedFileIndex;
        }
    }

    if (!exportFolder.isEmpty()) {
        exportCuttingEdgeFileList(exportFolder, exportedFileIndex - 1, cuttingEdgeFileBaseName);
        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "切削刃文件已自动导出到: %s，共 %i 个文件",
            exportFolder.toLocal8Bit().constData(),
            exportedFileIndex - 1);
        Msg::ShowInfo(Buffer);
    }

}

/// <summary>
/// compute the tool angle and offse the tool point(including the toolangles along the cutting direction)
/// </summary>
#include <cmath>
void MdiChild::ComputeandOffsetEdge()
{
    Msg::ShowInfo("开始计算切削工况...");
    const QString exportFolder = QFileDialog::getExistingDirectory(
        this,
        tr("选择切削刃导出文件夹"),
        QDir::currentPath());
    const QString cuttingEdgeFileBaseName = "multi_broaching_100";
    if (!exportFolder.isEmpty()) {
        QDir exportDir(exportFolder);
        exportDir.mkpath(".");
    }
    vector<array<double, 12>> pointsVec; //存储离散点的坐标，包括切向，角度，切厚等。
    // 离散点：AngleDialog::pointsAndVec
    // 创建面分类器
    BRepClass_FaceClassifier classifier1, classifier2; //点与面的关系分类器

    for (int i = 0; i < AngleDialog::pointsAndVec.size(); ++i) // 遍历所有离散点
    {
        const std::vector<double>& data = AngleDialog::pointsAndVec[i]; //离散点
        bool hasRakeFace = false;
        bool hasClearanceFace = false;
        // 提取坐标（前3列） 
        gp_Pnt pnt(data[0], data[1], data[2]);
        vector<TopoDS_Face> m_rakeFaces, m_clearanceFaces; //该点的所有前刀面和后刀面
        TopoDS_Face m_rakeFace, m_clearanceFace;
        gp_Dir rakefacenormal;

        // 1. 找到前刀面
        int rakefaceID = -1;
        for (const TopoDS_Face& rakeFace : AngleDialog::rakeFaces) {
            rakefaceID++;
            classifier1.Perform(rakeFace, pnt, 1e-3);
            if (classifier1.State() == TopAbs_IN || classifier1.State() == TopAbs_ON) // not TopAbs_OUT
            {
                m_rakeFace = rakeFace;
                // 获取面的几何表面
                Handle(Geom_Surface) surface = BRep_Tool::Surface(rakeFace);

                // 检查是否为平面
                Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(surface);
                if (!plane.IsNull()) {
                    rakefacenormal = plane->Axis().Direction();
                }
                else  Msg::ShowError("该点前刀面需要为平面"); //对于车刀刀片，前刀面不是平面，所以计算前刀面法向，可以找到面在切削刃离散点的法向。
                m_rakeFaces.push_back(m_rakeFace);
                break;
            }
        }

        if (m_rakeFaces.size() < 1) {
            if (findNearestFaceByPointProjection(AngleDialog::rakeFaces, pnt, 0.5, m_rakeFace)) {
                Handle(Geom_Surface) surface = BRep_Tool::Surface(m_rakeFace);
                Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(surface);
                if (!plane.IsNull()) {
                    rakefacenormal = plane->Axis().Direction();
                }
                else Msg::ShowError("该点前刀面需要为平面");
                m_rakeFaces.push_back(m_rakeFace);
                Msg::ShowInfo("点未落在前刀面上，已使用最近前刀面进行计算。");
            }
            else {
                Msg::ShowError("该点前刀面获取失败，请重新选择面");
                continue;
            }
        }
        // 2. 找到后刀面
        for (const TopoDS_Face& clearanceFace : AngleDialog::clearanceFaces) {
            //h_MyViewer->Display(clearanceFace, Quantity_NOC_BLUE1);
            classifier2.Perform(clearanceFace, pnt, 1e-3);
            if (classifier2.State() == TopAbs_IN || classifier2.State() == TopAbs_ON) // not TopAbs_OUT
            {
                m_clearanceFace = clearanceFace;
                m_clearanceFaces.push_back(m_clearanceFace);
            }

        }
        if (m_clearanceFaces.size() < 1) {
            if (findNearestFaceByPointProjection(AngleDialog::clearanceFaces, pnt, 0.5, m_clearanceFace)) {
                m_clearanceFaces.push_back(m_clearanceFace);
                Msg::ShowInfo("点未落在后刀面上，已使用最近后刀面进行计算。");
            }
            else {
                Msg::ShowError("该点后刀面获取失败，请重新选择面");
                continue;
            }
        }

        // 3. 切削刃法平面
        gp_Vec edgeTangentVec(data[3], data[4], data[5]);//获取切削刃切向量
        gp_Dir edgeTangentDir(edgeTangentVec);// 归一化法向量
        gp_Pln normalPlane(pnt, edgeTangentDir);   //过点pnt且法向量为edgeTangentDir的无限大平面
        //h_MyViewer->Display(normalFace,Quantity_NOC_RED);  //显示法平面

        //4. 法平面与前刀面求交，得到交线
        TopoDS_Edge intersectionRakeEdge; //法平面与前刀面交线边
        for (int j = 0; j < m_rakeFaces.size(); j++)
        {
            m_rakeFace = m_rakeFaces[j];
            hasRakeFace = buildNormalPlaneSectionEdge(normalPlane, m_rakeFace, intersectionRakeEdge);
            if (hasRakeFace) break;
        }
        if (!hasRakeFace) {
            Msg::ShowError("法平面与前刀面求交失败，请重新选择面");
            continue;
        }

        ///5. 计算前刀面线与切削速度法向夹角，也就是前角
        gp_Vec depth_dir = AngleDialog::m_cuttingVec.Crossed(edgeTangentDir); //工件表面法向
        double rakeAngle;
        if (hasRakeFace) {
            rakeAngle = OCCT_GraphOperations::ComputeMinAngle(depth_dir, intersectionRakeEdge, 1e-3);
            rakeAngle = rakeAngle / M_PI * 180; //前角
        }
        else  rakeAngle = -180;
        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "刀具前角：%f", rakeAngle);
        Msg::ShowInfo(Buffer);

        // h_MyViewer->Display(normalFace, Quantity_NOC_RED);
         //h_MyViewer->Display(m_clearanceFace, Quantity_NOC_BLUE);
         //6. 法平面与后刀面求交，得到交线
        TopoDS_Edge intersectionClearanceEdge; //法平面与后刀面交线边
        for (int j = 0; j < m_clearanceFaces.size(); j++)
        {
            m_clearanceFace = m_clearanceFaces[j];
            hasClearanceFace = buildNormalPlaneSectionEdge(normalPlane, m_clearanceFace, intersectionClearanceEdge);
            if (hasClearanceFace) break;
        }
        if (!hasClearanceFace) {
            Msg::ShowError("法平面与后刀面求交失败，请重新选择面");
            continue;
        }

        ///7. 计算后角
        gp_Vec crossB = AngleDialog::m_cuttingVec.Crossed(depth_dir);

        double clearanceAngle;
        if (hasClearanceFace) {
            clearanceAngle = OCCT_GraphOperations::ComputeMinAngle(crossB, intersectionClearanceEdge, 1e-3);
            clearanceAngle = clearanceAngle / M_PI * 180; //后角
        }
        else clearanceAngle = -180;
        Sprintf(Buffer, "刀具后角：%f", clearanceAngle);
        Msg::ShowInfo(Buffer);

        ///7. 计算刃倾角
        double inclination_angle = acos(crossB.Dot(edgeTangentDir) / crossB.Magnitude()) / M_PI * 180; //刃倾角
        if (inclination_angle > 90) inclination_angle = 180 - inclination_angle;
        Sprintf(Buffer, "刃倾角：%f", inclination_angle);
        Msg::ShowInfo(Buffer);

        ///8. 保存数据
        AngleDialog::pointsAndVec[i].push_back(inclination_angle);//在原有点坐标和切向量的基础上，添加刃倾角、前角和后角
        AngleDialog::pointsAndVec[i].push_back(rakeAngle);
        AngleDialog::pointsAndVec[i].push_back(clearanceAngle);

        // 9 计算切削厚度
        double depth; // 切削厚度到底如何计算？是投影还是什么？

        if (AngleDialog::m_cuttingDepDir2.X()) // 齿宽为X方向
        {
            if (pnt.X() > 0)
                depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                    + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                    + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude(); //
            else
                depth = abs(-depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                    + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                    + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude(); //
        }
        if (AngleDialog::m_cuttingDepDir2.Y()) // 齿宽为Y方向
        {
            if (pnt.Y() > 0)
                depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                    + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                    + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude(); //
            else
                depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                    - depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                    + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude(); //
        }
        if (AngleDialog::m_cuttingDepDir2.Z()) // 齿宽为Z方向
        {
            if (pnt.Z() > 0)
                depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                    + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                    + depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude(); //
            else
                depth = abs(depth_dir.X() * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X()
                    + depth_dir.Y() * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y()
                    - depth_dir.Z() * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z()) / depth_dir.Magnitude(); //
        }

        AngleDialog::pointsAndVec[i].push_back(AngleDialog::m_cuttingVec.Magnitude());
        AngleDialog::pointsAndVec[i].push_back(depth);
        AngleDialog::pointsAndVec[i].push_back(rakefaceID);
        AngleDialog::pointsAndVec[i].push_back(rakefacenormal.X()); //存储面法向
        AngleDialog::pointsAndVec[i].push_back(rakefacenormal.Y());
        AngleDialog::pointsAndVec[i].push_back(rakefacenormal.Z());

        std::array<double, 12> xyzab = { AngleDialog::pointsAndVec[i][0], AngleDialog::pointsAndVec[i][1], AngleDialog::pointsAndVec[i][2], AngleDialog::pointsAndVec[i][6], AngleDialog::pointsAndVec[i][7], AngleDialog::pointsAndVec[i][8], AngleDialog::pointsAndVec[i][9], AngleDialog::pointsAndVec[i][10],AngleDialog::pointsAndVec[i][11],AngleDialog::pointsAndVec[i][12], AngleDialog::pointsAndVec[i][13], AngleDialog::pointsAndVec[i][14] }; ///3-5 is the tangential direction
        //p_TreeWidget->anglesVec.push_back(xyzab);
        pointsVec.push_back(xyzab);
        //      //显示离散点处xyz坐标轴
        //      Handle(AIS_Trihedron) Trihedron = new AIS_Trihedron(
        //          new Geom_Axis2Placement(gp_Ax2(pnt, gp_Dir(0, 0, 1))));
        //      Trihedron->SetSize(10); //设置大小
        //      Trihedron->SetColor(Quantity_NOC_BLUE);
        //      Trihedron->SetTextColor(Quantity_NOC_BLUE);
              //h_MyViewer->Display(Trihedron); //显示坐标系
        //      opencascade::handle<Graphic3d_TransformPers> transform =
        //          new Graphic3d_TransformPers(Graphic3d_TMF_ZoomPers);

    }//结束遍历点

    /// <summary>
    ///  generate multiple edge
    /// </summary>
    double a, b, c, d, dx, dy, dz;
    int exportedFileIndex = 1;
    for (int edgeindex = 0; edgeindex < AngleDialog::m_offsetNumber; ++edgeindex)
    {
        vector<array<double, 12>> newpointsAndVec = pointsVec;
        // 遍历所有点
        for (int i = 0; i < newpointsAndVec.size(); ++i)
        {
            gp_Vec normalVec(AngleDialog::pointsAndVec[i][3], AngleDialog::pointsAndVec[i][4], AngleDialog::pointsAndVec[i][5]);//获取切向量
            gp_Dir edgeTangentDir(normalVec);// 归一化法向量
            gp_Vec depth_dir = AngleDialog::m_cuttingVec.Crossed(edgeTangentDir);
            if (AngleDialog::m_cuttingDepDir2.X()) // 齿宽为X方向
            {
                gp_Pnt A(sign(newpointsAndVec[i][0]) * AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X(), AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y(), AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z());
                if (AngleDialog::m_cuttingDepDir1.Y() == 0)
                {
                    a = A.X();
                    b = A.Z();
                    c = depth_dir.X();
                    d = depth_dir.Z();
                    dx = c * (c * a + d * b) / (c * c + d * d);
                    dz = d * (c * a + d * b) / (c * c + d * d);
                    dy = -(newpointsAndVec[i][9] * dx + newpointsAndVec[i][11] * dz) / newpointsAndVec[i][10];
                }
                else if (AngleDialog::m_cuttingDepDir1.Z() == 0)
                {
                    a = A.X();
                    b = A.Y();
                    c = depth_dir.X();
                    d = depth_dir.Y();
                    dx = c * (c * a + d * b) / (c * c + d * d);
                    dy = d * (c * a + d * b) / (c * c + d * d);
                    dz = -(newpointsAndVec[i][9] * dx + newpointsAndVec[i][10] * dy) / newpointsAndVec[i][11];
                }
            }
            if (AngleDialog::m_cuttingDepDir2.Y()) // 齿宽为Y方向
            {
                gp_Pnt A(AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X(), sign(newpointsAndVec[i][1]) * AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y(), AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z());
                if (AngleDialog::m_cuttingDepDir1.X() == 0)
                {
                    a = A.Y();
                    b = A.Z();
                    c = depth_dir.Y();
                    d = depth_dir.Z();
                    dy = c * (c * a + d * b) / (c * c + d * d);
                    dz = d * (c * a + d * b) / (c * c + d * d);
                    dx = -(newpointsAndVec[i][10] * dy + newpointsAndVec[i][11] * dz) / newpointsAndVec[i][9];
                }
                else if (AngleDialog::m_cuttingDepDir1.Z() == 0)
                {
                    a = A.X();
                    b = A.Y();
                    c = depth_dir.X();
                    d = depth_dir.Y();
                    dx = c * (c * a + d * b) / (c * c + d * d);
                    dy = d * (c * a + d * b) / (c * c + d * d);
                    dz = -(newpointsAndVec[i][9] * dx + newpointsAndVec[i][10] * dy) / newpointsAndVec[i][11];
                }
            }
            if (AngleDialog::m_cuttingDepDir2.Z()) // 齿宽为Z方向
            {
                gp_Pnt A(AngleDialog::m_cuttingDep.X() * AngleDialog::m_cuttingDepDir1.X(), AngleDialog::m_cuttingDep.Y() * AngleDialog::m_cuttingDepDir1.Y(), sign(newpointsAndVec[i][2]) * AngleDialog::m_cuttingDep.Z() * AngleDialog::m_cuttingDepDir1.Z());
                if (AngleDialog::m_cuttingDepDir1.X() == 0)
                {
                    a = A.Y();
                    b = A.Z();
                    c = depth_dir.Y();
                    d = depth_dir.Z();
                    dy = c * (c * a + d * b) / (c * c + d * d);
                    dz = d * (c * a + d * b) / (c * c + d * d);
                    dx = -(newpointsAndVec[i][10] * dy + newpointsAndVec[i][11] * dz) / newpointsAndVec[i][9];
                }
                else if (AngleDialog::m_cuttingDepDir1.Y() == 0)
                {
                    a = A.X();
                    b = A.Z();
                    c = depth_dir.X();
                    d = depth_dir.Z();
                    dx = c * (c * a + d * b) / (c * c + d * d);
                    dz = d * (c * a + d * b) / (c * c + d * d);
                    dy = -(newpointsAndVec[i][9] * dx + newpointsAndVec[i][11] * dz) / newpointsAndVec[i][10];
                }
            }
            newpointsAndVec[i][0] += dx * edgeindex;
            newpointsAndVec[i][1] += dy * edgeindex;
            newpointsAndVec[i][2] += dz * edgeindex; //

            newpointsAndVec[i][0] += AngleDialog::m_offsetDir.X() * AngleDialog::m_offsetDistance * edgeindex;
            newpointsAndVec[i][1] += AngleDialog::m_offsetDir.Y() * AngleDialog::m_offsetDistance * edgeindex;
            newpointsAndVec[i][2] += AngleDialog::m_offsetDir.Z() * AngleDialog::m_offsetDistance * edgeindex;

        }
        p_TreeWidget->addPointToTree(newpointsAndVec);//将点添加到工程树
        if (!exportFolder.isEmpty()) {
            exportCuttingEdgePointsToFile(newpointsAndVec, exportFolder, exportedFileIndex, cuttingEdgeFileBaseName);
        }
        ++exportedFileIndex;
    }

    if (!exportFolder.isEmpty()) {
        exportCuttingEdgeFileList(exportFolder, exportedFileIndex - 1, cuttingEdgeFileBaseName);
        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "切削刃文件已自动导出到: %s，共 %i 个文件",
            exportFolder.toLocal8Bit().constData(),
            exportedFileIndex - 1);
        Msg::ShowInfo(Buffer);
    }

}

void MdiChild::CaptureVertex()
{
    // 0. 设置选择模式为面
    h_MyViewer->getAisContext()->Deactivate();
    h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
    // 1. 获取交互上下文
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();
    if (!myContext) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }
    // 获取选中的模型
    Handle(AIS_Shape) selectedShape = Handle(AIS_Shape)::DownCast(h_MyViewer->getAisContext()->SelectedInteractive());
    if (selectedShape.IsNull()) {
        Msg::ShowWarning("请选中模型！");
        return;
    }
    // 遍历模型的面并存储到 vector 中
        // 3. 遍历选中的对象，提取面
    TopoDS_Vertex selectedVertex;
    bool hasVertex = false;
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
        // 获取选中的形状
        TopoDS_Shape shape = myContext->SelectedShape();
        //Msg::ShowInfo("开始循环");
        // 如果是面，直接使用
        if (shape.ShapeType() == TopAbs_VERTEX) {
            selectedVertex = TopoDS::Vertex(shape);
            gp_Pnt aPoint = BRep_Tool::Pnt(selectedVertex);

            //AngleDialog::edgeVertexs.push_back(selectedVertex);
            hasVertex = true;
            for (int i = 0; i < AngleDialog::pointsAndVec.size(); i++)
            {
                gp_Pnt bPoint(AngleDialog::pointsAndVec[i][0], AngleDialog::pointsAndVec[i][1], AngleDialog::pointsAndVec[i][2]);
                if (aPoint.IsEqual(bPoint, 1e-3))
                {
                    //generateAbaqusINP(AngleDialog::m_cuttingVec.Magnitude(), AngleDialog::pointsAndVec[i][9], AngleDialog::pointsAndVec[i][6], AngleDialog::pointsAndVec[i][7], AngleDialog::pointsAndVec[i][8], 0);
                }
            }

        }
        else {
            Msg::ShowInfo("请选中一个点！");
            break;
        }
    }
    Standard_Character Buffer[1024] = { 0 };
    //Sprintf(Buffer, "捕捉到点的数量为：%i", AngleDialog::edgeVertexs.size());
    Msg::ShowInfo(Buffer);
}

#include "MeshManager.h"
#include <gmsh.h>
#include <STEPControl_Writer.hxx>
#include "OCCT_GraphOperations.h"

void MdiChild::GenerateMesh() {
    Msg::ShowInfo("生成网格...");

    MeshDialog* dialog = new MeshDialog(this, q3dView);
    dialog->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动删除
    connect(dialog, &MeshDialog::generateMeshRequested, this, &MdiChild::GenerateMeshWithGmsh);
    dialog->show(); //非模态显示

}

//@brief 将当前网格保存为Gmsh格式的.msh文件 目前还没实现
bool MdiChild::SaveMeshToFile()
{
    try {
        // 检查是否有网格数据可供保存
        if (m_lastNodeCoords.empty() || m_lastElemTypes.empty()) {
            Msg::ShowError("没有可保存的网格数据，请先生成网格");
            return false;
        }

        // 弹出保存文件对话框[2,4](@ref)
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("保存网格文件"),
            QDir::currentPath(),
            tr("Gmsh网格文件 (*.msh);;所有文件 (*)")
        );

        // 检查用户是否取消了对话框
        if (fileName.isEmpty()) {
            return false;
        }

        // 确保文件扩展名为.msh[2](@ref)
        QFileInfo fileInfo(fileName);
        if (fileInfo.suffix().isEmpty()) {
            fileName += ".msh";
        }
        else if (fileInfo.suffix() != "msh") {
            fileName = fileInfo.path() + "/" + fileInfo.baseName() + ".msh";
        }

        // 重新构建Gmsh模型并保存[3](@ref)
        gmsh::initialize();

        // 创建新模型
        gmsh::model::add("MeshModel");

        // 添加节点
        std::vector<std::size_t> nodeTags;
        std::vector<double> nodeCoords = m_lastNodeCoords;
        std::vector<double> nodeParams{}; // 参数坐标可以为空

        // 重建节点标签（从1开始的连续整数）
        nodeTags.resize(m_lastNodeCoords.size() / 3);
        for (std::size_t i = 0; i < nodeTags.size(); i++) {
            nodeTags[i] = i + 1;
        }

        // 将节点添加到离散实体（维度0，标签1）
        //gmsh::model::mesh::addNodes(-1, 3, nodeTags, nodeCoords, nodeParams);
        gmsh::model::mesh::setNode(1, nodeCoords, nodeParams);
        // 添加单元
        for (std::size_t i = 0; i < m_lastElemTypes.size(); i++) {
            int elemType = m_lastElemTypes[i];
            const auto& elemNodeTags = m_lastElemNodeTags[i];

            // 重建单元标签
            std::vector<std::size_t> elemTags;
            // elemTags.resize(elemNodeTags.size() /
             //    gmsh::model::mesh::getElementProperties(elemType)[0]); // 每个单元的节点数

            for (std::size_t j = 0; j < elemTags.size(); j++) {
                elemTags[j] = j + 1;
            }

            gmsh::model::mesh::addElementsByType(0, elemType, elemTags, elemNodeTags);
        }

        // 同步模型
        gmsh::model::occ::synchronize();


        // 保存网格文件[1,4](@ref)
        gmsh::write(fileName.toStdString());

        // 清理Gmsh
        gmsh::finalize();

        // 显示成功消息
        Msg::ShowInfo("网格文件已保存");

        return true;

    }
    catch (const std::exception& e) {
        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "保存网格文件时出错：%s", e.what());
        Msg::ShowError(Buffer);
        return false;
    }
}

void MdiChild::GenerateMeshWithGmsh(const MeshParameters& params)
{
    Msg::ShowInfo("使用Gmsh生成网格...");
    // 创建进度对话框
    m_progressDialog = new QProgressDialog("正在生成网格...", "取消", 0, 0, this);
    m_progressDialog->setWindowTitle("网格生成");
    m_progressDialog->setWindowModality(Qt::WindowModal);// 模态对话框，阻止与主窗口交互
    m_progressDialog->setMinimumSize(400, 120);
    m_progressDialog->show();

    // 连接取消按钮
    connect(m_progressDialog, &QProgressDialog::canceled, this, [this]() {
        //m_meshFutureWatcher.cancel();
        m_progressDialog->close();
        setEnabled(true);
        });

    h_MyViewer->getAisContext()->Deactivate();
    //h_MyViewer->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_SOLID));
    Handle_AIS_InteractiveContext myContext = h_MyViewer->getAisContext();     // 获取交互上下文
    if (!myContext) {
        Msg::ShowError("交互上下文未初始化！");
        return;
    }
    // 检查是否有选中的对象
    if (!myContext->HasSelectedShape()) {
        Msg::ShowError("请先选择一个solid！");
        return;
    }
    // 遍历选中的对象，提取solid
    int Selectcount = myContext->NbSelected();//获取选中对象的数量
    TopoDS_Solid Solid;
    TopoDS_Shape shell;
    bool hasSolid = false;
    bool hasShell = false;
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected()) {
        TopoDS_Shape shape = myContext->SelectedShape();
        if (shape.ShapeType() == TopAbs_SOLID) {
            Solid = TopoDS::Solid(shape);
            hasSolid = true;
            break;
        }
        else if (shape.ShapeType() == TopAbs_SHELL) {
            shell = TopoDS::Shell(shape);
            hasShell = true;
            break;
        }
        else if (shape.ShapeType() == TopAbs_COMPOUND)
        {
            shell = TopoDS::Compound(shape);
            hasShell = true;
            break;
        }
        else {
            Msg::ShowInfo("请至少选中一个实体！");
            break;
        }
    }
    //if (!hasSolid) {
    //    Msg::ShowInfo("请至少选中一个实体！");
    //    return;
    //}

    //// 初始化消息处理器
    //    if (!m_gmshMessageHandler) {
    //        m_gmshMessageHandler = GmshMessageHandler::instance();
    //        connect(m_gmshMessageHandler, &GmshMessageHandler::messageReceived,
    //            this, &MdiChild::onGmshMessageReceived);
    //    }

    // 连接异步操作完成信号
    connect(&m_meshFutureWatcher, &QFutureWatcher<void>::finished,
        this, &MdiChild::onMeshGenerationFinished);

    // 禁用UI操作以避免重复执行
    setEnabled(false);
    Msg::ShowInfo("划分网格...");

    // 启动异步网格生成
    if (hasSolid) {
        QFuture<void> future = QtConcurrent::run([=]() {
            this->generateMeshAsync(params, Solid);
            });
        m_meshFutureWatcher.setFuture(future);
    }
    else if (hasShell) {
        QFuture<void> future = QtConcurrent::run([=]() {
            this->generateMeshAsync(params, shell);
            });
        m_meshFutureWatcher.setFuture(future);
    }

}

void MdiChild::SetBoundaryCondition()
{
    Msg::ShowInfo("设置边界条件...");
    if (p_BCDialog == nullptr)p_BCDialog = new BoundaryConditionDialog(q3dView, this);
    q3dView->dlg = p_BCDialog;
    //p_BCDialog->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动删除
    //connect(dialog, &BoundaryConditionDialog::boundaryConditionApplied, this, &MdiChild::GenerateMeshWithGmsh);
    p_BCDialog->show(); //非模态显示
}

void MdiChild::generateMeshAsync(const MeshParameters& params, TopoDS_Shape solid)
{
    // 启动消息捕获
    //if (m_gmshMessageHandler) {
    //    m_gmshMessageHandler->startCapturing();
    //}

    try {
        // 1. 将实体保存为临时STEP文件
        QString tempStepFile = "temp_solid.step";
        STEPControl_Writer stepWriter;
        stepWriter.Transfer(solid, STEPControl_AsIs);
        bool status = stepWriter.Write(tempStepFile.toLocal8Bit().constData());
        if (!status) {
            throw std::runtime_error("无法写入临时STEP文件");
        }

        // 2. 初始化Gmsh
        gmsh::initialize();
        //gmsh::option::setNumber("General.Terminal", 1);

        // 3. 创建Gmsh模型并导入STEP文件
        std::string stepFilePath = "temp_solid.step";
        //std::string stepFilePath = "D:/cnc/XSim/XSim/milling.step";
        std::vector<std::pair<int, int>> imported_entities; // 存储导入的实体信息
        //gmsh::open(stepFilePath);
        gmsh::model::occ::importShapes(stepFilePath, imported_entities); // 导入 STEP 文件,并将其实体信息存储到 imported_entities 中
        gmsh::model::occ::synchronize(); // 同步几何到 Gmsh 模型

        // 获取模型中所有 3D 实体（六面体）
        std::vector<std::pair<int, int>> volumes; // 存储体实体信息
        gmsh::model::getEntities(volumes, 3); // 获取所有体 实体，并存储到 volumes 中
        std::vector<std::pair<int, int>> all_entities;
        gmsh::model::getEntities(all_entities);   // 获取所有实体（0D, 1D, 2D, 3D）

        // 创建物理组[1](@ref)
        std::vector<int> volumeTags;
        for (const auto& vol : volumes) {
            volumeTags.push_back(vol.second);
        }

        MeshCounter++;
        gmsh::model::addPhysicalGroup(3, volumeTags, MeshCounter); //添加物理模型组
        gmsh::model::setPhysicalName(3, 1, "Hexahedron");

        // 4. 设置网格参数
        if (params.maxSize > 0) gmsh::option::setNumber("Mesh.CharacteristicLengthMax", params.maxSize);
        if (params.minSize > 0) gmsh::option::setNumber("Mesh.CharacteristicLengthMin", params.minSize);
        //gmsh::option::setNumber("Mesh.AngleTolerance", AngleTole); // 设置角度容差
        //gmsh::option::setNumber("Mesh.GradationFactor", GradaFactor);//设置网格渐变因子
        //gmsh::option::setNumber("Mesh.CharacteristicLengthFromPoints", ApplyGPSize ? 1 : 0); // 是否应用全局物理单元尺寸
        //gmsh::option::setNumber("Mesh.AngleBasedSmooth", AngleSmooth ? 1 : 0); // 是否启用基于角度的平滑
        //gmsh::option::setNumber("Mesh.ElementOrder", elementOrder);// 设置单元阶数（1表示线性单元，2表示二次单元）
        gmsh::option::setNumber("Mesh.Algorithm3D", params.meshAlgo); // (1=Delaunay, 2=New Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree)
        //gmsh::option::setNumber("Mesh.SubdivisionAlgorithm", 1); // 网格细分算法（1表示使用线性细分）
        //gmsh::option::setNumber("Mesh.AngleToleranceFacetoverlap", 0.01);// 设置角度容差
        //if (params.meshAlgo == "六面体网格")
        //gmsh::option::setNumber("Mesh.RecombineAll", 1); // 重组为六面体
        //gmsh::option::setNumber("Mesh.Recombine3DAll", 1); // 重组为六面体
        // 5. 生成3D网格
        //gmsh::model::mesh::generate(2);
        gmsh::model::mesh::generate(3);

        // 6. 获取网格数据
        std::vector<std::size_t> nodeTags;
        std::vector<double> nodeCoords, nodeParams;
        gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams);

        std::vector<int> elemTypes;
        std::vector<std::vector<std::size_t>> elemTags, elemNodeTags;
        gmsh::model::mesh::getElements(elemTypes, elemTags, elemNodeTags);

        m_lastNodeCoords = nodeCoords;
        m_lastElemTypes = elemTypes;
        m_lastElemNodeTags = elemNodeTags;
        OCCT_GraphOperations::PrintMeshInfo(nodeTags.size(), elemTypes, elemTags);//节点数、每个单元类型及数量

        // 7. 转换为OCCT形状并显示
        TopoDS_Shape meshShape = OCCT_GraphOperations::CreatOCCMeshShape(nodeCoords, elemTypes, elemNodeTags);
        //DisplayModel(meshShape);

        // 8. 保存网格文件（可选）
       // gmsh::write("generated_mesh.msh");

        // 9. 清理
       // gmsh::finalize();
        QFile::remove(tempStepFile); // 删除临时文件

        Handle(AIS_Shape) hAisShape = new AIS_Shape(meshShape);

        ModelData model;
        h_MyViewer->Display(hAisShape);
        model.name = "generated_mesh.msh";
        model.shape = hAisShape;
        p_TreeWidget->addMeshToTree(model);

        h_MyViewer->AisObjHide();
        h_MyViewer->Redraw();
        q3dView->fitAll();

    }
    catch (const std::exception& e) {
        Msg::ShowError(QString("网格生成错误: %1").arg(e.what()).toUtf8().constData());

    }
    // 停止消息捕获
    if (m_gmshMessageHandler) {
        m_gmshMessageHandler->stopCapturing();
    }

    //////----------------应用边界条件-----------------//////

    if (params.applyConstraints) {
        QList<BoundaryConditionData> baDataList = q3dView->getBoundaryConditions();
        for (int i = 0; i < baDataList.size(); i++) {
            baDataList[i].AssociateNodes(m_lastNodeCoords);
            Standard_Character Buffer[1024]{};
            int count = baDataList[i].associatedNodePoints.size();
            std::vector<gp_Pnt> pnts = baDataList[i].associatedNodePoints;
            for (int j = 0; j < count; ++j) {
                Handle(AIS_Point) Point = new AIS_Point(new Geom_CartesianPoint(pnts[j]));
                //Point->SetColor(Quantity_NOC_ALICEBLUE);
                Point->SetMarker(Aspect_TOM_STAR);// 设置标记形状为星形
                h_MyViewer->getAisContext()->Display(Point, Standard_True);
                h_MyViewer->Redraw();
            }
            Sprintf(Buffer, "被约束的节点数量为%i", count);
            Msg::ShowInfo(Buffer);
        }
        this->q3dView->SetBoundaryConditions(baDataList);
        //Msg::ShowInfo("在网格重建后应用边界条件。。。"); 
    }
}

void MdiChild::onMeshGenerationFinished() {
    // 重新启用UI
    setEnabled(true);
    m_progressDialog->close();

    if (!m_lastError.isEmpty()) {
        //Msg::ShowError(m_lastError);
        m_lastError.clear();
        return;
    }
}


void MdiChild::onGmshMessageReceived(const QString& message) {
    // 过滤和格式化Gmsh消息
    QString formattedMessage = formatGmshMessage(message);
    if (!formattedMessage.isEmpty()) {
        Msg::ShowInfo(formattedMessage.toUtf8().constData());
    }
}

QString MdiChild::formatGmshMessage(const QString& rawMessage) {
    QString message = rawMessage.trimmed();

    // 过滤掉空消息或无关紧要的消息
    if (message.isEmpty() || message.startsWith("Info    : ")) {
        return "";
    }

    // 移除Gmsh的消息前缀
    if (message.startsWith("Info    : ")) {
        return message.mid(10); // 移除"Info    : "前缀
    }
    if (message.startsWith("Warning : ")) {
        return QString("警告: %1").arg(message.mid(10));
    }
    if (message.startsWith("Error   : ")) {
        return QString("错误: %1").arg(message.mid(10));
    }

    return message;
}
