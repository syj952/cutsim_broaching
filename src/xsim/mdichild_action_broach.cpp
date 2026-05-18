#include "src/broaching/cutsim_broaching.hpp"
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
#include "Msg.h"
#include "ProjectTree.h"
#include "PropertyView.h"
#include "BoundaryConditionDialog.h"
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <QMessageBox>
#include "AngleDialog.h"
#include <QPushButton>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <AIS_Triangulation.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <QtConcurrent/QtConcurrentRun>
#include <AIS_Trihedron.hxx>
#include <Geom_Axis2Placement.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Prs3d_PointAspect.hxx>
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
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <vector>

namespace
{
    QString resolveBroachEntryPath(const QString& listFilePath, const QString& entry)
    {
        const QString trimmedEntry = entry.trimmed();
        if (trimmedEntry.isEmpty()) {
            return QString();
        }

        QFileInfo entryInfo(trimmedEntry);
        if (entryInfo.isAbsolute() && entryInfo.exists()) {
            return entryInfo.absoluteFilePath();
        }

        const QFileInfo listInfo(listFilePath);
        const QDir listDir = listInfo.absoluteDir();
        const QDir parentDir(listDir.absolutePath() + "/..");
        const QString entryFileName = QFileInfo(trimmedEntry).fileName();

        const QStringList candidates = {
            listDir.filePath(trimmedEntry),
            parentDir.filePath(trimmedEntry),
            listDir.filePath(entryFileName),
            parentDir.filePath(entryFileName)
        };

        for (const QString& candidate : candidates) {
            QFileInfo candidateInfo(candidate);
            if (candidateInfo.exists() && candidateInfo.isFile()) {
                return candidateInfo.absoluteFilePath();
            }
        }

        return QString();
    }

    QString buildResolvedBroachListFile(const QString& listFilePath)
    {
        QFile inputFile(listFilePath);
        if (!inputFile.open(QFile::ReadOnly | QFile::Text)) {
            Msg::ShowError("无法打开刀刃清单文件。");
            return QString();
        }

        QFileInfo listInfo(listFilePath);
        const QString resolvedListPath = listInfo.absoluteDir().filePath(listInfo.completeBaseName() + "_resolved.txt");
        QFile outputFile(resolvedListPath);
        if (!outputFile.open(QFile::WriteOnly | QFile::Text)) {
            Msg::ShowError("无法创建刀刃清单解析文件。");
            return QString();
        }

        QTextStream input(&inputFile);
        QTextStream output(&outputFile);
        int validCount = 0;
        while (!input.atEnd()) {
            const QString line = input.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }

            const QString resolvedPath = resolveBroachEntryPath(listFilePath, line);
            if (resolvedPath.isEmpty()) {
                Msg::ShowError(QString("刀刃文件不存在：%1").arg(line).toLocal8Bit().constData());
                continue;
            }

            output << QDir::toNativeSeparators(resolvedPath) << "\n";
            ++validCount;
        }

        output.flush();
        outputFile.close();
        inputFile.close();

        if (validCount == 0) {
            Msg::ShowError("刀刃清单中没有可用的刀刃文件，仿真已停止。");
            return QString();
        }

        Standard_Character buffer[1024] = { 0 };
        Sprintf(buffer, "刀刃清单解析成功，有效刀刃文件数量：%i", validCount);
        Msg::ShowInfo(buffer);
        return resolvedListPath;
    }
}
#include <array>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <StlAPI_Reader.hxx>
#include "MyViewer.h"
#include "OcctVisualizer.hpp"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

using namespace std;
void MdiChild::RunCutsim()
{
    QDialog dialog(this);
    dialog.setWindowTitle("仿真设置");
    dialog.setMinimumSize(800, 300);

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // 工件文件选择
    QHBoxLayout* workfileLayout = new QHBoxLayout();
    QLabel* workfileLabel = new QLabel("选择工件模型");
    QLineEdit* workfileEdit = new QLineEdit();
    workfileEdit->setReadOnly(true);
    QPushButton* workfileButton = new QPushButton("选择文件...");
    workfileLayout->addWidget(workfileLabel);
    workfileLayout->addWidget(workfileEdit, 1); // 1表示拉伸因子
    workfileLayout->addWidget(workfileButton);
    mainLayout->addLayout(workfileLayout);
    // 刀具文件选择
    QHBoxLayout* broachfileLayout = new QHBoxLayout();
    QLabel* broachfileLabel = new QLabel("刀具点数据:");
    QLineEdit* broachfileEdit = new QLineEdit();
    broachfileEdit->setReadOnly(true);
    QPushButton* broachfileButton = new QPushButton("选择文件...");
    broachfileLayout->addWidget(broachfileLabel);
    broachfileLayout->addWidget(broachfileEdit, 1);
    broachfileLayout->addWidget(broachfileButton);
    mainLayout->addLayout(broachfileLayout);

    // 切削力系数
    QHBoxLayout* KcnameLayout = new QHBoxLayout();
    QLabel* forcecoeffnameLabel = new QLabel("coeff:");
    QLabel* a0nameLabel = new QLabel("a0");
    QLabel* a1nameLabel = new QLabel("    +a1*Vc");
    QLabel* a2nameLabel = new QLabel("   +a2*gamma");
    QLabel* a3nameLabel = new QLabel(" +a3*alpha");
    QLabel* a4nameLabel = new QLabel("  +a4*h");
    QLabel* a5nameLabel = new QLabel("  +a5*Vc*gamma");
    QLabel* a6nameLabel = new QLabel("+a6*Vc*alpha");
    QLabel* a7nameLabel = new QLabel("+a7*Vc*h");
    QLabel* a8nameLabel = new QLabel("+a8*gamma*alpha");
    QLabel* a9nameLabel = new QLabel("+a9*gamma*h");
    QLabel* a10nameLabel = new QLabel("+a10*alpha*h");

    KcnameLayout->addWidget(forcecoeffnameLabel);
    KcnameLayout->addWidget(a0nameLabel);
    KcnameLayout->addWidget(a1nameLabel);
    KcnameLayout->addWidget(a2nameLabel);
    KcnameLayout->addWidget(a3nameLabel);
    KcnameLayout->addWidget(a4nameLabel);
    KcnameLayout->addWidget(a5nameLabel);
    KcnameLayout->addWidget(a6nameLabel);
    KcnameLayout->addWidget(a7nameLabel);
    KcnameLayout->addWidget(a8nameLabel);
    KcnameLayout->addWidget(a9nameLabel);
    KcnameLayout->addWidget(a10nameLabel);
    mainLayout->addLayout(KcnameLayout);

    // 切削力系数 KC
    QHBoxLayout* KcLayout = new QHBoxLayout();
    QLabel* KcnameLabel = new QLabel("Kc:");
    QLineEdit* a0KcEdit = new QLineEdit("17587");
    QLineEdit* a1KcEdit = new QLineEdit("-29.54");
    QLineEdit* a2KcEdit = new QLineEdit("-199.7");
    QLineEdit* a3KcEdit = new QLineEdit("-572.45");
    QLineEdit* a4KcEdit = new QLineEdit("-469307");
    QLineEdit* a5KcEdit = new QLineEdit();
    QLineEdit* a6KcEdit = new QLineEdit();
    QLineEdit* a7KcEdit = new QLineEdit();
    QLineEdit* a8KcEdit = new QLineEdit();
    QLineEdit* a9KcEdit = new QLineEdit("6411");
    QLineEdit* a10KcEdit = new QLineEdit("21542");

    KcLayout->addWidget(KcnameLabel);
    KcLayout->addWidget(a0KcEdit);
    KcLayout->addWidget(a1KcEdit);
    KcLayout->addWidget(a2KcEdit);
    KcLayout->addWidget(a3KcEdit);
    KcLayout->addWidget(a4KcEdit);
    KcLayout->addWidget(a5KcEdit);
    KcLayout->addWidget(a6KcEdit);
    KcLayout->addWidget(a7KcEdit);
    KcLayout->addWidget(a8KcEdit);
    KcLayout->addWidget(a9KcEdit);
    KcLayout->addWidget(a10KcEdit);
    mainLayout->addLayout(KcLayout);

    // 切削力系数 KCn
    QHBoxLayout* KcNLayout = new QHBoxLayout();
    QLabel* KcNnameLabel = new QLabel("KcN:");
    QLineEdit* a0KcNEdit = new QLineEdit("16476");
    QLineEdit* a1KcNEdit = new QLineEdit("65");
    QLineEdit* a2KcNEdit = new QLineEdit("-331.1");
    QLineEdit* a3KcNEdit = new QLineEdit("-192.68");
    QLineEdit* a4KcNEdit = new QLineEdit("-445119");
    QLineEdit* a5KcNEdit = new QLineEdit();
    QLineEdit* a6KcNEdit = new QLineEdit();
    QLineEdit* a7KcNEdit = new QLineEdit();
    QLineEdit* a8KcNEdit = new QLineEdit();
    QLineEdit* a9KcNEdit = new QLineEdit("8808.7");
    QLineEdit* a10KcNEdit = new QLineEdit();

    KcNLayout->addWidget(KcNnameLabel);
    KcNLayout->addWidget(a0KcNEdit);
    KcNLayout->addWidget(a1KcNEdit);
    KcNLayout->addWidget(a2KcNEdit);
    KcNLayout->addWidget(a3KcNEdit);
    KcNLayout->addWidget(a4KcNEdit);
    KcNLayout->addWidget(a5KcNEdit);
    KcNLayout->addWidget(a6KcNEdit);
    KcNLayout->addWidget(a7KcNEdit);
    KcNLayout->addWidget(a8KcNEdit);
    KcNLayout->addWidget(a9KcNEdit);
    KcNLayout->addWidget(a10KcNEdit);
    mainLayout->addLayout(KcNLayout);

    // 修正系数
    QHBoxLayout* correctnameLayout = new QHBoxLayout();
    QLabel* correctnameLabel = new QLabel("coeff:");
    QLabel* corra0nameLabel = new QLabel("a0");
    QLabel* corra1nameLabel = new QLabel("    +a1*gamma");
    QLabel* corra2nameLabel = new QLabel("   +a2*gamma^2");

    correctnameLayout->addWidget(correctnameLabel);
    correctnameLayout->addWidget(corra0nameLabel);
    correctnameLayout->addWidget(corra1nameLabel);
    correctnameLayout->addWidget(corra2nameLabel);
    mainLayout->addLayout(correctnameLayout);

    // 修正系数 KC
    QHBoxLayout* corrKcLayout = new QHBoxLayout();
    QLabel* corrKcnameLabel = new QLabel("Kc:");
    QLineEdit* corra0KcEdit = new QLineEdit("1.5");
    QLineEdit* corra1KcEdit = new QLineEdit("-0.068");
    QLineEdit* corra2KcEdit = new QLineEdit("0.0024");

    corrKcLayout->addWidget(corrKcnameLabel);
    corrKcLayout->addWidget(corra0KcEdit);
    corrKcLayout->addWidget(corra1KcEdit);
    corrKcLayout->addWidget(corra2KcEdit);
    mainLayout->addLayout(corrKcLayout);

    // 修正系数 KCn
    QHBoxLayout* corrKcNLayout = new QHBoxLayout();
    QLabel* corrKcNnameLabel = new QLabel("KcN:");
    QLineEdit* corra0KcNEdit = new QLineEdit("1.439");
    QLineEdit* corra1KcNEdit = new QLineEdit("-0.0572");
    QLineEdit* corra2KcNEdit = new QLineEdit("0.002");

    corrKcNLayout->addWidget(corrKcNnameLabel);
    corrKcNLayout->addWidget(corra0KcNEdit);
    corrKcNLayout->addWidget(corra1KcNEdit);
    corrKcNLayout->addWidget(corra2KcNEdit);
    mainLayout->addLayout(corrKcNLayout);

    // 速度
    QHBoxLayout* velocityLayout = new QHBoxLayout();
    QLabel* velocityLabel = new QLabel("Velocity (x, y, z):");
    QLineEdit* vxEdit = new QLineEdit("0");
    QLineEdit* vyEdit = new QLineEdit("-1");
    QLineEdit* vzEdit = new QLineEdit("0");
    velocityLayout->addWidget(velocityLabel);
    velocityLayout->addWidget(vxEdit);
    velocityLayout->addWidget(vyEdit);
    velocityLayout->addWidget(vzEdit);
    mainLayout->addLayout(velocityLayout);

    // 仿真设定
    QHBoxLayout* simulationLayout = new QHBoxLayout();
    QLabel* simulationLabel = new QLabel("Simulation (total time, steptime for cut and mfem):");
    QLineEdit* totaltimeEdit = new QLineEdit("70");
    QLineEdit* steptime_material_removal_Edit = new QLineEdit("1.0");
    QLineEdit* steptime_MFEM_Edit = new QLineEdit("25");
    simulationLayout->addWidget(simulationLabel);
    simulationLayout->addWidget(totaltimeEdit);
    simulationLayout->addWidget(steptime_material_removal_Edit);
    simulationLayout->addWidget(steptime_MFEM_Edit);
    mainLayout->addLayout(simulationLayout);

    // 约束
    QHBoxLayout* constraintLayout = new QHBoxLayout();
    QLabel* constraintsLabel = new QLabel("Constrains:");
    constraintLayout->addWidget(constraintsLabel);
    mainLayout->addLayout(constraintLayout);
    // 约束 x
    QHBoxLayout* xconstraint = new QHBoxLayout();
    QLabel* xLabel = new QLabel("X:");
    QLineEdit* x1Edit = new QLineEdit("-100");
    //x1Edit->setReadOnly(true);
    QLineEdit* x2Edit = new QLineEdit("100");
    //x2Edit->setReadOnly(true);
    xconstraint->addWidget(xLabel);
    xconstraint->addWidget(x1Edit);
    xconstraint->addWidget(x2Edit);
    mainLayout->addLayout(xconstraint);
    // 约束 y
    QHBoxLayout* yconstraint = new QHBoxLayout();
    QLabel* yLabel = new QLabel("Y:");
    QLineEdit* y1Edit = new QLineEdit("-100");
    //y1Edit->setReadOnly(true);
    QLineEdit* y2Edit = new QLineEdit("100");
    //y2Edit->setReadOnly(true);
    yconstraint->addWidget(yLabel);
    yconstraint->addWidget(y1Edit);
    yconstraint->addWidget(y2Edit);
    mainLayout->addLayout(yconstraint);
    // 约束 z
    QHBoxLayout* zconstraint = new QHBoxLayout();
    QLabel* zLabel = new QLabel("Z:");
    QLineEdit* z1Edit = new QLineEdit("-11");
    //z1Edit->setReadOnly(true);
    QLineEdit* z2Edit = new QLineEdit("100");
    //z2Edit->setReadOnly(true);
    zconstraint->addWidget(zLabel);
    zconstraint->addWidget(z1Edit);
    zconstraint->addWidget(z2Edit);
    mainLayout->addLayout(zconstraint);

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* runButton = new QPushButton("开始运行");
    buttonLayout->addWidget(runButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // 存储文件路径的变量
    QString workfilePath, broachfilePath, file3Path;

    // 连接按钮信号
    connect(workfileButton, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(
            &dialog, "选择工件模型", "", "STL Files (*.stl)");
        if (!path.isEmpty()) {
            workfilePath = path;
            workfileEdit->setText(path);
        }
        });

    connect(broachfileButton, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(
            &dialog, "选择刀片文件", "", "Text Files (*.txt)");
        if (!path.isEmpty()) {
            broachfilePath = path;
            broachfileEdit->setText(path);
        }
        });


    connect(runButton, &QPushButton::clicked, [&]() {
        broachpar.velocity[0] = vxEdit->text().toDouble();
        broachpar.velocity[1] = vyEdit->text().toDouble();
        broachpar.velocity[2] = vzEdit->text().toDouble();
        broachpar.constrain_limits[0][0] = x1Edit->text().toDouble();
        broachpar.constrain_limits[0][1] = x2Edit->text().toDouble();
        broachpar.constrain_limits[1][0] = y1Edit->text().toDouble();
        broachpar.constrain_limits[1][1] = y2Edit->text().toDouble();
        broachpar.constrain_limits[2][0] = z1Edit->text().toDouble();
        broachpar.constrain_limits[2][1] = z2Edit->text().toDouble();
        broachpar.simulation[0] = totaltimeEdit->text().toDouble();
        broachpar.simulation[1] = steptime_material_removal_Edit->text().toDouble();
        broachpar.simulation[2] = steptime_MFEM_Edit->text().toDouble();
        broachpar.force_coefs[0][0] = a0KcEdit->text().toDouble();
        broachpar.force_coefs[0][1] = a1KcEdit->text().toDouble();
        broachpar.force_coefs[0][2] = a2KcEdit->text().toDouble();
        broachpar.force_coefs[0][3] = a3KcEdit->text().toDouble();
        broachpar.force_coefs[0][4] = a4KcEdit->text().toDouble();
        broachpar.force_coefs[0][5] = a5KcEdit->text().toDouble();
        broachpar.force_coefs[0][6] = a6KcEdit->text().toDouble();
        broachpar.force_coefs[0][7] = a7KcEdit->text().toDouble();
        broachpar.force_coefs[0][8] = a8KcEdit->text().toDouble();
        broachpar.force_coefs[0][9] = a9KcEdit->text().toDouble();
        broachpar.force_coefs[0][10] = a10KcNEdit->text().toDouble();
        broachpar.force_coefs[1][0] = a0KcNEdit->text().toDouble();
        broachpar.force_coefs[1][1] = a1KcNEdit->text().toDouble();
        broachpar.force_coefs[1][2] = a2KcNEdit->text().toDouble();
        broachpar.force_coefs[1][3] = a3KcNEdit->text().toDouble();
        broachpar.force_coefs[1][4] = a4KcNEdit->text().toDouble();
        broachpar.force_coefs[1][5] = a5KcNEdit->text().toDouble();
        broachpar.force_coefs[1][6] = a6KcNEdit->text().toDouble();
        broachpar.force_coefs[1][7] = a7KcNEdit->text().toDouble();
        broachpar.force_coefs[1][8] = a8KcNEdit->text().toDouble();
        broachpar.force_coefs[1][9] = a9KcNEdit->text().toDouble();
        broachpar.force_coefs[1][10] = a10KcNEdit->text().toDouble();

        broachpar.force_coefs[2][0] = corra0KcEdit->text().toDouble();
        broachpar.force_coefs[2][1] = corra1KcEdit->text().toDouble();
        broachpar.force_coefs[2][2] = corra2KcEdit->text().toDouble();

        broachpar.force_coefs[3][0] = corra0KcNEdit->text().toDouble();
        broachpar.force_coefs[3][1] = corra1KcNEdit->text().toDouble();
        broachpar.force_coefs[3][2] = corra2KcNEdit->text().toDouble();

        dialog.accept();
        executeSimulation(workfilePath, broachfilePath, file3Path);
        });

    // 显示对话框
    dialog.exec();
}

void MdiChild::executeSimulation(QString stlfile, QString edgePointsfile, QString BladeAnglesfile)
{
    const QString resolvedEdgePointsfile = buildResolvedBroachListFile(edgePointsfile);
    if (resolvedEdgePointsfile.isEmpty()) {
        return;
    }

    CutsimBroaching* myBroach = new CutsimBroaching(9);
    double partoffset[3] = { 0,0,0 };
    double octreecenter[3] = { 0,0,0 };

    TopoDS_Shape aShape;
    StlAPI_Reader aReader;
    TopoDS_Shape aTempShape;
    Standard_Boolean Res = aReader.Read(aShape, stlfile.toUtf8().constData());
    if (Res)
    {
        //p_UI->showSuccess("STL识别成功");
        Msg::ShowInfo("STL识别成功");
        //aShape = aTempShape;
    }
    else Msg::ShowInfo("STL读取失败");
    if (!aShape.IsNull()) {
        h_MyViewer->Display(aShape);
        Bnd_Box bbox;
        BRepBndLib::Add(aShape, bbox);

        if (bbox.IsVoid()) {
            // 处理空包围盒
            return;
        }
        double xMin, yMin, zMin, xMax, yMax, zMax;
        bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        octreecenter[0] = (xMin + xMax) / 2;
        octreecenter[1] = (yMin + yMax) / 2;
        octreecenter[2] = (zMin + zMax) / 2;
        double cube_size = std::max({ -xMin + xMax, -yMin + yMax, -zMin + zMax }) + 1.0;
        myBroach->setStlStock(stlfile.toUtf8().constData(), partoffset, octreecenter, cube_size);
    }
    else
    {
        if (p_TreeWidget->Partitem->childCount() > 0)
        {
            QString tt = p_TreeWidget->Partitem->child(0)->text(0);
            Handle(AIS_Shape) aisShape;
            TopoDS_Shape aShape;
            aisShape = p_TreeWidget->modelMap.value(tt).shape;
            if (!aisShape.IsNull()) {
                // 方法1：使用 Shape() 方法
                aShape = aisShape->Shape();
            }
            gp_Trsf currentTrsf = aisShape->LocalTransformation();
            gp_Pnt currentLocation = gp_Pnt(0, 0, 0).Transformed(currentTrsf);
            partoffset[0] = currentLocation.X();
            partoffset[1] = currentLocation.Y();
            partoffset[2] = currentLocation.Z();

            StlAPI_Reader aReader;
            TopoDS_Shape aTempShape;
            Bnd_Box bbox;
            BRepBndLib::Add(aShape, bbox);

            if (bbox.IsVoid()) {
                // 处理空包围盒
                return;
            }
            double xMin, yMin, zMin, xMax, yMax, zMax;
            bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

            octreecenter[0] = (xMin + xMax) / 2 + partoffset[0];
            octreecenter[1] = (yMin + yMax) / 2 + partoffset[1];
            octreecenter[2] = (zMin + zMax) / 2 + partoffset[2];
            double cube_size = std::max({ -xMin + xMax, -yMin + yMax, -zMin + zMax }) + 1.0;
            myBroach->setStlStock(p_TreeWidget->modelMap.value(tt).path, partoffset, octreecenter, cube_size);
        }
        else
        {
            Msg::ShowError("读取内容为空，采用默认模型");
            octreecenter[0] = 0;
            octreecenter[1] = -20;
            octreecenter[2] = -22;
            myBroach->setRectStock(octreecenter, 50.0);
        }
    }
    myBroach->setConstraints(broachpar.constrain_limits);
    myBroach->setVelocity(broachpar.velocity[0], broachpar.velocity[1], broachpar.velocity[2]);
    myBroach->setSimulationTimes(broachpar.simulation[0], broachpar.simulation[1], broachpar.simulation[2]); // total simulation time, increment time, very time for modal analysis
    myBroach->newBroach(broachpar.force_coefs);
    myBroach->addBroachs(resolvedEdgePointsfile);
    //myBroach->performFEMSimulation(h_MyViewer);
    connect(myBroach, &CutsimBroaching::ApplyUpdateViewer, this, &MdiChild::updateView);
    connect(myBroach, &CutsimBroaching::ApplyUpdateColorBar, this->q3dView, &OcctView::setDisplacementRange);
    connect(myBroach, &CutsimBroaching::ApplyUpdateForces, this->forcewidget, &ForceMonitorWidget::updateData);

    // 异步执行 FEM 模拟

    QFuture<void> future = QtConcurrent::run([this, myBroach]() {
        myBroach->performFEMSimulation(this, h_MyViewer, visulization_item, visulization_limits);
        });


    /// <summary>
    /// test 1 for the stl model
    /// </summary>
    /// <param name="stlfile"></param>
    /// <param name="edgePointsfile"></param>
    /// <param name="BladeAnglesfile"></param>
    //CutsimBroaching* myBroach = new CutsimBroaching(8);
    //double center[3] = { 0,0,0 };
    ////QString tt = p_TreeWidget->Partitem->child(0)->text(0);
    ////myBroach->setStlStock(p_TreeWidget->modelMap.value(tt).path, center, center, 1.5);
    //myBroach->setStlStock("cad/disk_4_2_cut.STL", center, center, 50.0);
    //if (m_materialProperties.size() > 0)    myBroach->setWorkMaterial(m_materialProperties[0].density, m_materialProperties[0].youngsModulus, m_materialProperties[0].poissonRatio);
    //myBroach->setConstraints(broachpar.constrain_limits);
    //myBroach->setVelocity(broachpar.velocity[0], broachpar.velocity[1], broachpar.velocity[2]);
    //myBroach->setSimulationTimes(broachpar.simulation[0], broachpar.simulation[1], broachpar.simulation[2]); // total simulation time, increment time, very time for modal analysis
    //
    //myBroach->newBroach(broachpar.force_coefs);
    //myBroach->addBroachs(edgePointsfile);

    //QFuture<void> future = QtConcurrent::run([this, myBroach]() {
    //    myBroach->performFEMSimulation(this, h_MyViewer, visulization_item, visulization_limits);
    //    });
    ///end test 1 for the stl model
    //CutsimBroaching* myBroach = new CutsimBroaching(9);
    //double center[3] = { 0, 10, -9 };
    //myBroach->setRectStock(center, 50.0);
    //myBroach->setVelocity(0, -1, 0);
    //myBroach->setSimulationTimes(50.0, 0.5, 50); // total simulation time, increment time, very time for modal analysis
    //myBroach->newBroach();
    //myBroach->addBroach("data/multi_broaching_500_0.txt");
    //myBroach->addBroach("data/multi_broaching_500_1.txt");
    //myBroach->addBroach("data/multi_broaching_500_2.txt");
    ////myBroach->performFEMSimulation(h_MyViewer);
    //// 异步执行 FEM 模拟
    //QFuture<void> future = QtConcurrent::run([this, myBroach]() {
    //    myBroach->performFEMSimulation(h_MyViewer, visulization_item, visulization_limits);
    //});

}
