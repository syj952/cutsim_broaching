#include "vtkwidget.h"
#include "ui_vtkwidget.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QTime>
QString txtPath = "C:/Users/b220/Desktop/MyLog2.txt";
QFileInfo fileInfo(txtPath);
QDir dir = fileInfo.absoluteDir();

vtkSmartPointer<vtkPolyData> VTKWidget::polyData;
const double PI = 3.14159265358979323846;

VTKWidget::VTKWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VTKWidget)
{
    vtkObject::GlobalWarningDisplayOff();//VTK8.2全局关闭警告

    ui->setupUi(this);

    vtkWidget = new QVTKOpenGLNativeWidget(this);
    vtkWidget->resize(this->size()); ///< make vtkWidget the same size as the widget.
    vtkWidget->SetRenderWindow(renderWindow);
    renderWindow->AddRenderer(renderer);

    renderer->SetBackground(1, 1, 1);
    renderer->SetBackground2(0.9176, 0.9294, 1);
    renderer->SetGradientBackground(true);

    renderWindow->Render();

    ui->frame->raise();
    ui->frame->setStyleSheet("background: transparent; border: none;");
    ui->frame_2->raise();
    ui->frame_2->setStyleSheet("background: transparent; border: none;");

    file2.setFileName("C:/Users/Administrator/Desktop/cnc-test.txt");
//    if (!file2.open(QIODevice::Append | QIODevice::Text)) {
//        qDebug() << "无法打开文件";
//    }

    this->setStyleSheet(R"(
                        font-family: 'Microsoft YaHei', 'Microsoft YaHei UI', sans-serif;)");

    QStringList items;
    items << "force_1" << "force_2" << "force_3" << "feed";
    ui->channelBox->addItems(items);
    ui->channelBox->setCurrentIndex(3);
    ui->channelBox->setStyleSheet(
        "QComboBox QAbstractItemView {"
        "    background-color: white;"                 // 设置下拉列表背景�?        "    selection-background-color: lightblue;"   // 设置选中项背景色
        "    font-family: Microsoft YaHei, Microsoft YaHei UI;"  // 下拉项字�?        "}"
    );

    //    double triangleArray[15000*3*3];
    //    vtkIdType numTriangles = 15000;
    //    // 填充数组
    //    for(int i = 0; i < numTriangles *3 *3; ++i) {
    //        triangleArray[i] = QRandomGenerator::global()->generateDouble() * 3000 -1500; // 生成0-1之间的随机double
    //    }
    //    //
    //    double colorArray[15000*3];
    //    for(int i = 0; i < numTriangles *3; ++i) {
    //        colorArray[i] = QRandomGenerator::global()->generateDouble();  // 生成0-1之间的随机颜色�?    //    }
    //    AddTriangles(renderer, triangleArray, numTriangles, colorArray);
}

VTKWidget::~VTKWidget()
{
    file2.close();
}

void VTKWidget::loadMchFile(const QString &filePath){
    ClearSTL();
    components = FileParser::ParseMachineFile(filePath);
    InitializeRenderer();
    emit componentsLoaded(components);
}

void VTKWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    vtkWidget->resize(this->size());
}

void VTKWidget::setInfoOverlayVisible(bool visible)
{
    ui->frame->setVisible(visible);
    ui->frame_2->setVisible(visible);
}

void VTKWidget::setColorChannelIndex(int index)
{
    if (index >= 0 && index < ui->channelBox->count()) {
        ui->channelBox->setCurrentIndex(index);
    }
}

void VTKWidget::WorldAxesDisplay(){
    axes->SetTotalLength(1100, 1100, 1100);
    axes->GetXAxisCaptionActor2D()->GetTextActor()->SetTextScaleModeToNone();
    axes->GetXAxisCaptionActor2D()->GetCaptionTextProperty()->SetFontSize(20);
    axes->GetYAxisCaptionActor2D()->GetTextActor()->SetTextScaleModeToNone();
    axes->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->SetFontSize(20);
    axes->GetZAxisCaptionActor2D()->GetTextActor()->SetTextScaleModeToNone();
    axes->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->SetFontSize(20);
    renderer->AddActor(axes);
}

void VTKWidget::SetCamera(){
    renderer->GetActiveCamera()->SetPosition(800, -600, 900);
    renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    renderer->GetActiveCamera()->SetClippingRange(0.1, 3000);//
    renderer->GetActiveCamera()->SetViewUp(0, 0, 1);

    renderer->GetActiveCamera()->SetParallelProjection(true);

    renderer->ResetCamera();
}

void VTKWidget::InitializeRenderer() {
    //
    for (const Component& comp : components) {
        //qDebug()<<comp.name;
        vtkSmartPointer<vtkAssembly> assembly = CreateComponentAssembly(comp);
        assemblyMap[comp.name] = assembly;

        if (comp.attach.isEmpty()) {
            renderer->AddActor(assembly);//
        }
        //        assembly->SetPosition(comp.positionX, comp.positionY, comp.positionZ);
    }

//    WorldAxesDisplay();
    SetCamera();

    renderWindow->Render();
}

vtkSmartPointer<vtkActor> VTKWidget::LoadSTLFile(const QString& filename, QColor color) {
    vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
    reader->SetFileName(filename.toStdString().c_str());
    reader->Update();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(reader->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
    actor->GetProperty()->SetOpacity(1.0);

    return actor;
}

vtkSmartPointer<vtkAssembly> VTKWidget::CreateComponentAssembly(const Component& component) {
    vtkSmartPointer<vtkAssembly> assembly = vtkSmartPointer<vtkAssembly>::New();
    for (int i=0; i<component.stlFiles.count(); ++i) {
        const QString& stlFile = component.stlFiles.at(i);
        // set visibility
        bool visiable = (component.visiable.at(i) == "on");
        // set color
        QColor color(component.XRGB.at(i).toUInt(nullptr, 16));
        vtkSmartPointer<vtkActor> actor = LoadSTLFile(stlFile, color);
        assembly->AddPart(actor);
        actor->SetVisibility(visiable);
    }

    if (!component.attach.isEmpty() && assemblyMap.contains(component.attach)) {
        //
        vtkSmartPointer<vtkAssembly> parentAssembly = assemblyMap[component.attach];
        parentAssembly->AddPart(assembly);
    }

    return assembly;
}

void VTKWidget::UpdateComponentTransform(vtkSmartPointer<vtkAssembly> assembly,
                                         double posX, double posY, double posZ,
                                         double rotA, double rotC) {
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    if(assembly == assemblyMap["A"]){
        transform->Translate(0, -49.985, 120.042);
        transform->RotateWXYZ(rotA, 1, 0, 0);
        transform->Translate(0, 49.985, -120.042);
        assembly->SetUserTransform(transform);
        //Y=49.985 Z=-120.042   :UCP800
    }else{
        transform->Translate(posX, posY, posZ);
        //transform->RotateWXYZ(rotA, 1, 0, 0);
        transform->RotateWXYZ(rotC, 0, 0, 1);
        assembly->SetUserTransform(transform);
    }
}

void VTKWidget::ToolPathDisplay(vtkSmartPointer<vtkPoints> points, double *data, int index, double feed)
{
    //static vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData = vtkSmartPointer<vtkPolyData>::New();
    static vtkSmartPointer<vtkPoints> allPoints = vtkSmartPointer<vtkPoints>::New();
    static vtkSmartPointer<vtkCellArray> allLines = vtkSmartPointer<vtkCellArray>::New();
    static vtkSmartPointer<vtkPolyDataMapper> mapper_line = vtkSmartPointer<vtkPolyDataMapper>::New();
    static vtkSmartPointer<vtkActor> actor_line = vtkSmartPointer<vtkActor>::New();
    static std::vector<vtkSmartPointer<vtkUnsignedCharArray>> colorsList(4);

    if(needClearToolpath){
        needClearToolpath = 0;
        allPoints->Initialize();
        allLines->Initialize();
        for (int i = 0; i < 4; ++i) if (colorsList[i]) colorsList[i]->Initialize();
        polyData->Initialize();
    }

    for (int i=0; i < 3; ++i) {
        if (!colorsList[i]) {
            colorsList[i] = vtkSmartPointer<vtkUnsignedCharArray>::New();
            colorsList[i]->SetNumberOfComponents(3);
            colorsList[i]->SetName(("Channel_" + std::to_string(i + 1)).c_str());
        }
    }
    if(!colorsList[3]){
        colorsList[3] = vtkSmartPointer<vtkUnsignedCharArray>::New();
        colorsList[3]->SetNumberOfComponents(3);
        colorsList[3]->SetName("feed");}

    //
    if (polyData->GetPoints() == nullptr) {
        polyData->SetPoints(allPoints);
        polyData->SetLines(allLines);

        mapper_line->SetInputData(polyData);
        actor_line->SetMapper(mapper_line);
        actor_line->GetProperty()->SetLineWidth(2);

        assemblyMap["Attach"]->AddPart(actor_line); ///< synchronizing the toolpath with the workpiece.
    }

    //
    vtkIdType startId = allPoints->InsertNextPoint(points->GetPoint(0));
    vtkIdType endId = allPoints->InsertNextPoint(points->GetPoint(1));

    //
    vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
    line->GetPointIds()->SetId(0, startId);
    line->GetPointIds()->SetId(1, endId);
    //
    std::vector<double> dataList = {data[0], data[1], data[2], feed};
    for (int i=0; i < 4; ++i) {
        double normalization_factor = (i < 3) ? 200 : 8000.0;
        double data_norm = dataList[i] / normalization_factor; ///< data normalization
        unsigned char rgbColor[3];
        rgbColor[0] = static_cast<unsigned char>(data_norm * 255);
        rgbColor[1] = static_cast<unsigned char>((1 - fabs(2 * data_norm - 1)) * 255);
        rgbColor[2] = static_cast<unsigned char>((1 - data_norm) * 255);
        colorsList[i]->InsertNextTypedTuple(rgbColor);
        //
        polyData->GetCellData()->AddArray(colorsList[i]);
    }

    if(index == 3)
        polyData->GetCellData()->SetActiveScalars((std::string("feed")).c_str());
    else
        polyData->GetCellData()->SetActiveScalars((std::string("Channel_")+std::to_string(index + 1)).c_str());

    allLines->InsertNextCell(line);

    polyData->Modified();
    allPoints->Modified();
    allLines->Modified();
}

void VTKWidget::linshi(const std::array<double, 6>& ppp, double rpm, double feed){
    v_rpm = rpm;
    ui->label_posX->setText(QString::number(ppp[0]));//g_workpieceOffset[0]
    ui->label_posY->setText(QString::number(ppp[1]));//g_workpieceOffset[1]
    ui->label_posZ->setText(QString::number(ppp[2]));//g_workpieceOffset[2]
    ui->label_posA->setText(QString::number(ppp[4]));
    ui->label_posC->setText(QString::number(ppp[3]));//+0.0003
    ui->label_feed->setText(QString::number(feed));
    ui->label_rpm->setText(QString::number(rpm));
    MachineMotion(const_cast<double*>(ppp.data()), force_port, ui->channelBox->currentIndex(), feed, rpm);
}

void VTKWidget::linshi_toolindex(int index){
    ui->label_tool->setText(QString::number(index));
}

void VTKWidget::linshi_2(QStringList result){
    for(int i=0; i<3; i++){
        force_port[i] = result[i].toDouble();
    }
    ui->label_force1->setText(QString::number(force_port[0]));
    ui->label_force2->setText(QString::number(force_port[1]));
    ui->label_force3->setText(QString::number(force_port[2]));
}


void VTKWidget::MachineMotion(double *MacPos, double *data, int index, double feed, double rpm)
{
    //X/Y/Z/A/C/SP
    double axis_pos[5] = {MacPos[0], MacPos[1], MacPos[2], MacPos[4], MacPos[3]};

//    QTextStream out(&file2);
//    currentTime = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
//    out << currentTime;
//    for (int i = 0; i < 5; ++i) {
//        out << "\t" << axis_pos[i] ;}
//    out << "\t" << feed;
//    out << "\t" << rpm;
//    out << "\n";

    axis_pos[0] += g_machcoor_to_vtk[0] +g_manualOffset[0];
    axis_pos[1] += g_machcoor_to_vtk[1] +g_manualOffset[1];
    axis_pos[2] += g_machcoor_to_vtk[2] +g_manualOffset[2];

    for (const Component& comp : components) {
        if (assemblyMap.contains(comp.name)) {
            vtkSmartPointer<vtkAssembly> assembly = assemblyMap[comp.name];

            if (comp.name == "X") {
                UpdateComponentTransform(assembly, axis_pos[0], 0, 0, 0, 0);
            } else if (comp.name == "Y") {
                UpdateComponentTransform(assembly, 0, axis_pos[1], 0, 0, 0);
            } else if (comp.name == "Z") {
                UpdateComponentTransform(assembly, 0, 0, axis_pos[2], 0, 0);
            } else if (comp.name == "A") {
                UpdateComponentTransform(assembly, 0, 0, 0, -axis_pos[3], 0);
            } else if (comp.name == "C") {
                UpdateComponentTransform(assembly, 0, 0, 0, 0, -axis_pos[4]);
            }
        }
    }

    //
    vtkMatrix4x4* C_matrix = assemblyMap["C"]->GetMatrix();
    vtkMatrix4x4* A_matrix = assemblyMap["A"]->GetMatrix();
    vtkNew<vtkMatrix4x4> Full_matrix, Inverse_matrix;
    vtkMatrix4x4::Multiply4x4(A_matrix, C_matrix, Full_matrix);
    vtkMatrix4x4::Invert(Full_matrix, Inverse_matrix);

    double worldPoint[3] = {axis_pos[0], axis_pos[1], axis_pos[2]+600-toolTipLength};
    double wpPoint[3];
    vtkNew<vtkTransform> Point_transform;
    Point_transform->SetMatrix(Inverse_matrix);
    Point_transform->TransformPoint(worldPoint, wpPoint);

    if(runtime > 1){
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        points->InsertNextPoint(p_last_2wp[0],p_last_2wp[1],p_last_2wp[2]);
        points->InsertNextPoint(wpPoint[0],wpPoint[1],wpPoint[2]);
        ToolPathDisplay(points, data, index, feed);
    }

    p_last_2wp[0] = wpPoint[0];
    p_last_2wp[1] = wpPoint[1];
    p_last_2wp[2] = wpPoint[2];
    runtime++;

    renderWindow->Render();

    computeToolTip();
}

void VTKWidget::AddTriangles(vtkRenderer* renderer, double* triangleArray, vtkIdType numTriangles, double* colorArray)
{
    vtkIdType numPoints = numTriangles * 3;

    //
    if (!actor_wp) {
        polydata_wp = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        points->SetDataTypeToDouble();
        polydata_wp->SetPoints(points);

        vtkSmartPointer<vtkCellArray> triangles = vtkSmartPointer<vtkCellArray>::New();
        polydata_wp->SetPolys(triangles);

        //
        vtkSmartPointer<vtkDoubleArray> cellColors = vtkSmartPointer<vtkDoubleArray>::New();
        cellColors->SetNumberOfComponents(3); // RGB
        cellColors->SetName("Colors");
        polydata_wp->GetCellData()->SetScalars(cellColors);

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(polydata_wp);
        mapper->SetScalarModeToUseCellData(); //

        actor_wp = vtkSmartPointer<vtkActor>::New();
        actor_wp->SetMapper(mapper);
        renderer->AddActor(actor_wp);  //
    }

    //
    vtkPoints* points = polydata_wp->GetPoints();
    points->SetNumberOfPoints(numPoints);
    std::memcpy(points->GetVoidPointer(0), triangleArray, numPoints * 3 * sizeof(double));

    vtkCellArray* cells = polydata_wp->GetPolys();
    cells->Reset();
    cells->Allocate(numTriangles * 4);
    for (vtkIdType i = 0; i < numTriangles; i++) {
        vtkIdType pts[3] = {i*3, i*3+1, i*3+2};
        cells->InsertNextCell(3, pts);
    }

    //
    vtkDoubleArray* cellColors = vtkDoubleArray::SafeDownCast(polydata_wp->GetCellData()->GetScalars());
    cellColors->SetNumberOfTuples(numTriangles);
    std::memcpy(cellColors->GetVoidPointer(0), colorArray, numTriangles * 3 * sizeof(double));

    polydata_wp->Modified();
    renderer->GetRenderWindow()->Render();
}

void VTKWidget::onlengthToolTipUpdated(double lengthToolTip, const QVector<double>& workpieceOffset, const QVector<double>& manualOffset) {
    toolTipLength = lengthToolTip;
    for(int i=0; i<3; i++){
        g_workpieceOffset[i] = workpieceOffset[i];
        g_manualOffset[i] = manualOffset[i];
    }
}

void VTKWidget::ClearSTL()
{
    renderer->RemoveAllViewProps();

    assemblyMap.clear();
    components.clear();
    needClearToolpath = 1;
    runtime = 0;
    p_last_2wp[0] = p_last_2wp[1] = p_last_2wp[2] = 0.0;

    SetCamera();
    renderWindow->Render();
}

vtkSmartPointer<vtkMatrix4x4> VTKWidget::cloneMatrix(vtkMatrix4x4* src) const
{
    vtkSmartPointer<vtkMatrix4x4> dst = vtkSmartPointer<vtkMatrix4x4>::New();
    dst->DeepCopy(src);
    return dst;
}

vtkSmartPointer<vtkMatrix4x4> VTKWidget::multiplyMatrix(vtkMatrix4x4* left, vtkMatrix4x4* right) const
{
    vtkSmartPointer<vtkMatrix4x4> result = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4(left, right, result);
    return result;
}

void VTKWidget::printMatrix(const QString& name, vtkMatrix4x4* matrix) const
{
    qDebug() << name;
    for (int i = 0; i < 4; ++i) {
        qDebug() << matrix->GetElement(i, 0)
            << matrix->GetElement(i, 1)
            << matrix->GetElement(i, 2)
            << matrix->GetElement(i, 3);
    }
}

vtkSmartPointer<vtkMatrix4x4> VTKWidget::getToolToWorldMatrixManual() const
{
    if (!assemblyMap.contains("X") || !assemblyMap.contains("Y") || !assemblyMap.contains("Z")) {
        qDebug() << "getToolToWorldMatrixManual: X/Y/Z not found.";
        vtkSmartPointer<vtkMatrix4x4> identity = vtkSmartPointer<vtkMatrix4x4>::New();
        identity->Identity();
        return identity;
    }

    vtkMatrix4x4* xMatrix = assemblyMap["X"]->GetMatrix();
    vtkMatrix4x4* yMatrix = assemblyMap["Y"]->GetMatrix();
    vtkMatrix4x4* zMatrix = assemblyMap["Z"]->GetMatrix();

    vtkSmartPointer<vtkMatrix4x4> xy = multiplyMatrix(xMatrix, yMatrix);
    vtkSmartPointer<vtkMatrix4x4> xyz = multiplyMatrix(xy, zMatrix);

    return xyz;
}

vtkSmartPointer<vtkMatrix4x4> VTKWidget::getWorkpieceToWorldMatrixManual() const
{
    if (!assemblyMap.contains("A") || !assemblyMap.contains("C")) {
        qDebug() << "getWorkpieceToWorldMatrixManual: A/C not found.";
        vtkSmartPointer<vtkMatrix4x4> identity = vtkSmartPointer<vtkMatrix4x4>::New();
        identity->Identity();
        return identity;
    }

    vtkMatrix4x4* aMatrix = assemblyMap["A"]->GetMatrix();
    vtkMatrix4x4* cMatrix = assemblyMap["C"]->GetMatrix();

    vtkSmartPointer<vtkMatrix4x4> ac = multiplyMatrix(aMatrix, cMatrix);
    return ac;
}

void VTKWidget::computeToolTip()
{
    vtkSmartPointer<vtkMatrix4x4> toolToWorld = getToolToWorldMatrixManual();
    vtkSmartPointer<vtkMatrix4x4> workpieceToWorld = getWorkpieceToWorldMatrixManual();

    vtkNew<vtkTransform> tipToToolTransform;
    tipToToolTransform->PostMultiply();
    tipToToolTransform->Identity();
    tipToToolTransform->Translate(0, 0, 600 - toolTipLength);
    vtkMatrix4x4* tipToTool = tipToToolTransform->GetMatrix();

    vtkNew<vtkMatrix4x4> tipToWorld;
    vtkMatrix4x4::Multiply4x4(toolToWorld, tipToTool, tipToWorld);

    vtkNew<vtkMatrix4x4> worldToWorkpiece;
    vtkMatrix4x4::Invert(workpieceToWorld, worldToWorkpiece);

    vtkNew<vtkMatrix4x4> tipToWorkpiece;
    vtkMatrix4x4::Multiply4x4(worldToWorkpiece, tipToWorld, tipToWorkpiece);

    // 工件上一点相对于工件原点
    vtkNew<vtkTransform> pointToWorkpieceTransform;
    pointToWorkpieceTransform->PostMultiply();
    pointToWorkpieceTransform->Identity();
    pointToWorkpieceTransform->Translate(185, 0, 150);// (185, 0, 150);
    vtkMatrix4x4* pointToWorkpiece = pointToWorkpieceTransform->GetMatrix();

    vtkNew<vtkMatrix4x4> workpieceToPoint;
    vtkMatrix4x4::Invert(pointToWorkpiece, workpieceToPoint);

    vtkNew<vtkMatrix4x4> tipToPoint;
    vtkMatrix4x4::Multiply4x4(workpieceToPoint, tipToWorkpiece, tipToPoint);

    double x = tipToPoint->GetElement(0, 3);
    double y = tipToPoint->GetElement(1, 3);
    double z = tipToPoint->GetElement(2, 3);

    // R = Rz(C) * Ry(B) * Rx(A)
    double r00 = tipToPoint->GetElement(0, 0);
    double r01 = tipToPoint->GetElement(0, 1);
    double r02 = tipToPoint->GetElement(0, 2);

    double r10 = tipToPoint->GetElement(1, 0);
    double r11 = tipToPoint->GetElement(1, 1);
    double r12 = tipToPoint->GetElement(1, 2);

    double r20 = tipToPoint->GetElement(2, 0);
    double r21 = tipToPoint->GetElement(2, 1);
    double r22 = tipToPoint->GetElement(2, 2);

    // B = asin(-r20)
    // A = atan2(r21, r22)
    // C = atan2(r10, r00)
    double A = 0.0;
    double B = 0.0;
    double C = 0.0;

    B = std::asin(-r20);
    double cosB = std::cos(B);

    const double eps = 1e-8;

    if (std::fabs(cosB) > eps) {
        A = std::atan2(r21, r22);
        C = std::atan2(r10, r00);
    }
    else {
        // 万向节锁：B 接近 ±90°
        C = 0.0;
        if (r20 <= -1.0 + eps) {
            // B = +90°
            B = PI / 2.0;
            A = std::atan2(-r01, r11);
        }
        else {
            // B = -90°
            B = -PI / 2.0;
            A = std::atan2(-r01, r11);
        }
    }

    //-------------------------------------//
    //A = normalizeRad(A);
    //B = normalizeRad(B);
    //C = normalizeRad(C);
    //-------------------------------------//

    mchData[0] = 3000; //v_rpm;
    mchData[1] = 6.28;
    mchData[2] = x;
    mchData[3] = y;
    mchData[4] = z;
    mchData[5] = A;
    mchData[6] = B;
    mchData[7] = C;
    emit mchDatatoCutsim(mchData);

    //QFile file(txtPath);
    //if (file.open(QIODevice::Append | QIODevice::Text))
    //{
    //    QTextStream out(&file);
    //    out.setCodec("UTF-8");  // Release 不乱�?
    //    QString currentTime = QTime::currentTime().toString("HH:mm:ss.zzz");

    //    out << currentTime << "\t" << mchData[2] << "\t" << mchData[3] << "\t" << mchData[4] << "\t" << mchData[5] << "\t" << mchData[6] << "\t" << mchData[7] << "\n";
    //}
}

double VTKWidget::normalizeRad(double angle)
{
    const double twoPI = 2.0 * PI;

    angle = std::fmod(angle + PI, twoPI);

    if (angle < 0.0) {
        angle += twoPI;
    }

    return angle - PI;
}

void VTKWidget::linshi_GCode(const std::array<double, 6>& ppp, double rpm, double feed) {
    // visualization
    v_rpm = rpm;
    ui->label_posX->setText(QString::number(ppp[0]));
    ui->label_posY->setText(QString::number(ppp[1]));
    ui->label_posZ->setText(QString::number(ppp[2]));
    ui->label_posA->setText(QString::number(ppp[4]));
    ui->label_posC->setText(QString::number(ppp[3]));
    ui->label_feed->setText(QString::number(feed));
    ui->label_rpm->setText(QString::number(rpm));
    
    double axis_pos[5] = { ppp[0], ppp[1], ppp[2], ppp[4], ppp[3] };//X/Y/Z/A/C/SP
    // VTK: A and C motion
    for (const Component& comp : components) {
        if (assemblyMap.contains(comp.name)) {
            vtkSmartPointer<vtkAssembly> assembly = assemblyMap[comp.name];

            if (comp.name == "A") {
                UpdateComponentTransform(assembly, 0, 0, 0, -axis_pos[3], 0);
            }
            else if (comp.name == "C") {
                UpdateComponentTransform(assembly, 0, 0, 0, 0, -axis_pos[4]);
            }
        }
    }

	// workpiece coordinate system adjustment
    vtkMatrix4x4* C_matrix = assemblyMap["C"]->GetMatrix();
    vtkMatrix4x4* A_matrix = assemblyMap["A"]->GetMatrix();
    vtkNew<vtkMatrix4x4> Full_matrix;
    vtkMatrix4x4::Multiply4x4(A_matrix, C_matrix, Full_matrix);

    double wpPoint[3] = { axis_pos[0] + Coor_workpiece_to_vtkworld[0], axis_pos[1] + Coor_workpiece_to_vtkworld[1], axis_pos[2] + Coor_workpiece_to_vtkworld[2] };
    double vtkWorldPoint[3];
    vtkNew<vtkTransform> Point_transform;
    Point_transform->SetMatrix(Full_matrix);
    Point_transform->TransformPoint(wpPoint, vtkWorldPoint);

	// VTK: X/Y/Z motion
    for (const Component& comp : components) {
        if (assemblyMap.contains(comp.name)) {
            vtkSmartPointer<vtkAssembly> assembly = assemblyMap[comp.name];

            if (comp.name == "X") {
                UpdateComponentTransform(assembly, vtkWorldPoint[0], 0, 0, 0, 0);
            }
            else if (comp.name == "Y") {
                UpdateComponentTransform(assembly, 0, vtkWorldPoint[1], 0, 0, 0);
            }
            else if (comp.name == "Z") {
                UpdateComponentTransform(assembly, 0, 0, vtkWorldPoint[2] - (600 - toolTipLength), 0, 0);
            }
        }
    }

	// create toolpath line
    if (runtime > 1) {
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        points->InsertNextPoint(p_last_2wp[0], p_last_2wp[1], p_last_2wp[2]);
        points->InsertNextPoint(wpPoint[0], wpPoint[1], wpPoint[2]);
        ToolPathDisplay(points, force_port, ui->channelBox->currentIndex(), feed);
    }
    p_last_2wp[0] = wpPoint[0];
    p_last_2wp[1] = wpPoint[1];
    p_last_2wp[2] = wpPoint[2];
    runtime++;

    renderWindow->Render();

    // run simulation every 10 read Gcode
    CallCutsimCounts++;
    if (CallCutsimCounts >= 10) {
        computeToolTip();
        CallCutsimCounts = 0;
    }
}