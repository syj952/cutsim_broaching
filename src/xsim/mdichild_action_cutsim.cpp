#include "src/cutsim/broaching/cutsim_broaching.hpp"
#include "src/cutsim/milling/cutsim_milling.hpp"
#include "src/digitaltwin/digitaltwin_milling/digitaltwin_milling.hpp"
#include "src/digitaltwin/digitaltwinctrl.h"
#include <QApplication>
#include "ComplainUtf8.h"
#include "mdichild.h"
#include "OCCT_ShapeList.h"
#include "QShapeImportUI.h"
#include "QShapeExportUI.h"
#include <AIS_Shape.hxx>
#include <QFileDialog>
#include "Msg.h"
#include "ProjectTree.h"
#include "PropertyView.h"
#include "BoundaryConditionDialog.h"
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include "AngleDialog.h"
#include <QPushButton>
#include <QCheckBox>
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

    // 鍒涘缓鑿滃崟鏍?
    QMenuBar* menuBar = new QMenuBar(&dialog);
    // 纭繚鎵€鏈夊姩浣滃湪鍚屼竴浣滅敤鍩熷唴瀹氫箟
    QAction* broachingAction = menuBar->addAction("拉销仿真");
    QAction* millingAction = menuBar->addAction("铣削仿真");
    QAction* digitaltwinAction = menuBar->addAction("数字孪生");
    broachingAction->setCheckable(true);
    millingAction->setCheckable(true);
    digitaltwinAction->setCheckable(true);
    digitaltwinAction->setChecked(true);

    // 鍒涘缓椤甸潰瀹瑰櫒
    QStackedWidget* stackedWidget = new QStackedWidget(&dialog);

    // 鎷夐攢浠跨湡椤甸潰
    QWidget* broachingPage = new QWidget();
    QVBoxLayout* broachingLayout = new QVBoxLayout(broachingPage);

    // 閾ｅ墛浠跨湡椤甸潰
    QWidget* millingPage = new QWidget();
    QVBoxLayout* millingLayout = new QVBoxLayout(millingPage);

    // 鏁板瓧瀛敓椤甸潰
    QWidget* digitaltwinPage = new QWidget();
    QVBoxLayout* digitaltwinLayout = new QVBoxLayout(digitaltwinPage);

    // 灏嗛〉闈㈡坊鍔犲埌鏍堝鍣?
    stackedWidget->addWidget(broachingPage);
    stackedWidget->addWidget(millingPage);
    stackedWidget->addWidget(digitaltwinPage);

    // 鍒涘缓涓诲竷灞€
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setMenuBar(menuBar);
    mainLayout->addWidget(stackedWidget);

    ///————————————拉销页面————————————
#pragma region 
// 宸ヤ欢鏂囦欢閫夋嫨
    QHBoxLayout* workfileLayout = new QHBoxLayout();
    QLabel* workfileLabel = new QLabel("选择工件模型");
    QLineEdit* workfileEdit = new QLineEdit();
    workfileEdit->setReadOnly(true);
    QPushButton* workfileButton = new QPushButton("选择文件...");
    workfileLayout->addWidget(workfileLabel);
    workfileLayout->addWidget(workfileEdit, 1); // 1琛ㄧず鎷変几鍥犲瓙
    workfileLayout->addWidget(workfileButton);
    broachingLayout->addLayout(workfileLayout);
    // 鍒€鍏锋枃浠堕€夋嫨
    QHBoxLayout* broachfileLayout = new QHBoxLayout();
    QLabel* broachfileLabel = new QLabel("刀具点数据:");
    QLineEdit* broachfileEdit = new QLineEdit();
    broachfileEdit->setReadOnly(true);
    QPushButton* broachfileButton = new QPushButton("选择文件...");
    broachfileLayout->addWidget(broachfileLabel);
    broachfileLayout->addWidget(broachfileEdit, 1);
    broachfileLayout->addWidget(broachfileButton);
    broachingLayout->addLayout(broachfileLayout);

    // 鍒囧墛鍔涚郴鏁?
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
    broachingLayout->addLayout(KcnameLayout);

    // 鍒囧墛鍔涚郴鏁?KC
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
    broachingLayout->addLayout(KcLayout);

    // 鍒囧墛鍔涚郴鏁?KCn
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
    broachingLayout->addLayout(KcNLayout);

    // 淇绯绘暟
    QHBoxLayout* correctnameLayout = new QHBoxLayout();
    QLabel* correctnameLabel = new QLabel("coeff:");
    QLabel* corra0nameLabel = new QLabel("a0");
    QLabel* corra1nameLabel = new QLabel("    +a1*gamma");
    QLabel* corra2nameLabel = new QLabel("   +a2*gamma^2");

    correctnameLayout->addWidget(correctnameLabel);
    correctnameLayout->addWidget(corra0nameLabel);
    correctnameLayout->addWidget(corra1nameLabel);
    correctnameLayout->addWidget(corra2nameLabel);
    broachingLayout->addLayout(correctnameLayout);

    // 淇绯绘暟 KC
    QHBoxLayout* corrKcLayout = new QHBoxLayout();
    QLabel* corrKcnameLabel = new QLabel("Kc:");
    QLineEdit* corra0KcEdit = new QLineEdit("1.5");
    QLineEdit* corra1KcEdit = new QLineEdit("-0.068");
    QLineEdit* corra2KcEdit = new QLineEdit("0.0024");

    corrKcLayout->addWidget(corrKcnameLabel);
    corrKcLayout->addWidget(corra0KcEdit);
    corrKcLayout->addWidget(corra1KcEdit);
    corrKcLayout->addWidget(corra2KcEdit);
    broachingLayout->addLayout(corrKcLayout);

    // 淇绯绘暟 KCn
    QHBoxLayout* corrKcNLayout = new QHBoxLayout();
    QLabel* corrKcNnameLabel = new QLabel("KcN:");
    QLineEdit* corra0KcNEdit = new QLineEdit("1.439");
    QLineEdit* corra1KcNEdit = new QLineEdit("-0.0572");
    QLineEdit* corra2KcNEdit = new QLineEdit("0.002");

    corrKcNLayout->addWidget(corrKcNnameLabel);
    corrKcNLayout->addWidget(corra0KcNEdit);
    corrKcNLayout->addWidget(corra1KcNEdit);
    corrKcNLayout->addWidget(corra2KcNEdit);
    broachingLayout->addLayout(corrKcNLayout);

    // 閫熷害
    QHBoxLayout* velocityLayout = new QHBoxLayout();
    QLabel* velocityLabel = new QLabel("Velocity (x, y, z):");
    QLineEdit* vxEdit = new QLineEdit("0");
    QLineEdit* vyEdit = new QLineEdit("-1");
    QLineEdit* vzEdit = new QLineEdit("0");
    velocityLayout->addWidget(velocityLabel);
    velocityLayout->addWidget(vxEdit);
    velocityLayout->addWidget(vyEdit);
    velocityLayout->addWidget(vzEdit);
    broachingLayout->addLayout(velocityLayout);

    // 浠跨湡璁惧畾
    QHBoxLayout* simulationLayout = new QHBoxLayout();
    QLabel* simulationLabel = new QLabel("Simulation (total time, steptime for cut and mfem):");
    QLineEdit* totaltimeEdit = new QLineEdit("70");
    QLineEdit* steptime_material_removal_Edit = new QLineEdit("2.0");
    QLineEdit* steptime_MFEM_Edit = new QLineEdit("25");
    simulationLayout->addWidget(simulationLabel);
    simulationLayout->addWidget(totaltimeEdit);
    simulationLayout->addWidget(steptime_material_removal_Edit);
    simulationLayout->addWidget(steptime_MFEM_Edit);
    broachingLayout->addLayout(simulationLayout);

    // 绾︽潫
    QCheckBox* residualStressCheck = new QCheckBox("启用加工残余应力测试");
    residualStressCheck->setChecked(true);
    broachingLayout->addWidget(residualStressCheck);
    QCheckBox* modalAnalysisCheck = new QCheckBox("启用模态分析");
    modalAnalysisCheck->setChecked(false);
    broachingLayout->addWidget(modalAnalysisCheck);
    QHBoxLayout* constraintLayout = new QHBoxLayout();
    QLabel* constraintsLabel = new QLabel("Constrains:");
    constraintLayout->addWidget(constraintsLabel);
    broachingLayout->addLayout(constraintLayout);
    // 绾︽潫 x
    QHBoxLayout* xconstraint = new QHBoxLayout();
    QLabel* xLabel = new QLabel("X:");
    QLineEdit* x1Edit = new QLineEdit("-100");
    //x1Edit->setReadOnly(true);
    QLineEdit* x2Edit = new QLineEdit("100");
    //x2Edit->setReadOnly(true);
    xconstraint->addWidget(xLabel);
    xconstraint->addWidget(x1Edit);
    xconstraint->addWidget(x2Edit);
    broachingLayout->addLayout(xconstraint);
    // 绾︽潫 y
    QHBoxLayout* yconstraint = new QHBoxLayout();
    QLabel* yLabel = new QLabel("Y:");
    QLineEdit* y1Edit = new QLineEdit("-100");
    //y1Edit->setReadOnly(true);
    QLineEdit* y2Edit = new QLineEdit("100");
    //y2Edit->setReadOnly(true);
    yconstraint->addWidget(yLabel);
    yconstraint->addWidget(y1Edit);
    yconstraint->addWidget(y2Edit);
    broachingLayout->addLayout(yconstraint);
    // 绾︽潫 z
    QHBoxLayout* zconstraint = new QHBoxLayout();
    QLabel* zLabel = new QLabel("Z:");
    QLineEdit* z1Edit = new QLineEdit("-11");
    //z1Edit->setReadOnly(true);
    QLineEdit* z2Edit = new QLineEdit("100");
    //z2Edit->setReadOnly(true);
    zconstraint->addWidget(zLabel);
    zconstraint->addWidget(z1Edit);
    zconstraint->addWidget(z2Edit);
    broachingLayout->addLayout(zconstraint);

    // 鎸夐挳鍖哄煙
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* runButton = new QPushButton("Run");
    buttonLayout->addWidget(runButton);
    buttonLayout->addStretch();
    broachingLayout->addLayout(buttonLayout);

    // 杩炴帴鎸夐挳淇″彿
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
            &dialog, "Select cutter file", "", "Text Files (*.txt)");
        if (!path.isEmpty()) {
            cutedgefilePath = path;
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
        broachpar.enable_machining_residual_stress = residualStressCheck->isChecked();
        broachpar.enable_modal_analysis = modalAnalysisCheck->isChecked();
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
        braoching_executeSimulation(workfilePath, cutedgefilePath, file3Path);
        });
#pragma endregion
    ///————————————铣销页面————————————
#pragma region 
// 宸ヤ欢鏂囦欢閫夋嫨
    QHBoxLayout* mill_workfileLayout = new QHBoxLayout();
    QLabel* mill_workfileLabel = new QLabel("选择工件模型");
    QLineEdit* mill_workfileEdit = new QLineEdit();
    workfileEdit->setReadOnly(true);
    QPushButton* mill_workfileButton = new QPushButton("选择文件...");
    mill_workfileLayout->addWidget(mill_workfileLabel);
    mill_workfileLayout->addWidget(mill_workfileEdit, 1); // 1琛ㄧず鎷変几鍥犲瓙
    mill_workfileLayout->addWidget(mill_workfileButton);
    millingLayout->addLayout(mill_workfileLayout);
    // 鍒€鍏锋枃浠堕€夋嫨
    QHBoxLayout* mill_fileLayout = new QHBoxLayout();
    QLabel* mill_fileLabel = new QLabel("刀具点数据:");
    QLineEdit* mill_fileEdit = new QLineEdit();
    mill_fileEdit->setReadOnly(true);
    QPushButton* mill_fileButton = new QPushButton("选择文件...");
    mill_fileLayout->addWidget(mill_fileLabel);
    mill_fileLayout->addWidget(mill_fileEdit, 1);
    mill_fileLayout->addWidget(mill_fileButton);
    millingLayout->addLayout(mill_fileLayout);

    // 鍒囧墛鍔涚郴鏁?
    QHBoxLayout* mill_KcnameLayout = new QHBoxLayout();
    QLabel* mill_forcecoeffnameLabel = new QLabel("coeff:");
    QLabel* mill_a0nameLabel = new QLabel("K_rc");
    QLabel* mill_a1nameLabel = new QLabel("K_tc");
    QLabel* mill_a2nameLabel = new QLabel("K_ac");
    QLabel* mill_a3nameLabel = new QLabel("K_re");
    QLabel* mill_a4nameLabel = new QLabel("K_te");
    QLabel* mill_a5nameLabel = new QLabel("K_ae");


    mill_KcnameLayout->addWidget(mill_forcecoeffnameLabel);
    mill_KcnameLayout->addWidget(mill_a0nameLabel);
    mill_KcnameLayout->addWidget(mill_a1nameLabel);
    mill_KcnameLayout->addWidget(mill_a2nameLabel);
    mill_KcnameLayout->addWidget(mill_a3nameLabel);
    mill_KcnameLayout->addWidget(mill_a4nameLabel);
    mill_KcnameLayout->addWidget(mill_a5nameLabel);

    millingLayout->addLayout(mill_KcnameLayout);

    // 鍒囧墛鍔涚郴鏁?KC
    QHBoxLayout* mill_KcLayout = new QHBoxLayout();
    QLabel* mill_KcnameLabel = new QLabel("Kc:");
    QLineEdit* mill_a0KcEdit = new QLineEdit("381.2");
    QLineEdit* mill_a1KcEdit = new QLineEdit("1227.7");
    QLineEdit* mill_a2KcEdit = new QLineEdit("153.0");
    QLineEdit* mill_a3KcEdit = new QLineEdit("6.6");
    QLineEdit* mill_a4KcEdit = new QLineEdit("6.3");
    QLineEdit* mill_a5KcEdit = new QLineEdit("0.2");


    mill_KcLayout->addWidget(mill_KcnameLabel);
    mill_KcLayout->addWidget(mill_a0KcEdit);
    mill_KcLayout->addWidget(mill_a1KcEdit);
    mill_KcLayout->addWidget(mill_a2KcEdit);
    mill_KcLayout->addWidget(mill_a3KcEdit);
    mill_KcLayout->addWidget(mill_a4KcEdit);
    mill_KcLayout->addWidget(mill_a5KcEdit);
    millingLayout->addLayout(mill_KcLayout);

    // 鍒€鍒冩暟锛屼富杞磋浆閫?
    QHBoxLayout* mill_spindleLabelout = new QHBoxLayout();
    QLabel* mill_spindleLabel = new QLabel(" blade_sum, Spindle:");
    QLineEdit* mill_blade_sum_Edit = new QLineEdit("2");
    QLineEdit* mill_spindle_speed_Edit = new QLineEdit("1100");
    mill_spindleLabelout->addWidget(mill_spindleLabel);
    mill_spindleLabelout->addWidget(mill_blade_sum_Edit);
    mill_spindleLabelout->addWidget(mill_spindle_speed_Edit);
    millingLayout->addLayout(mill_spindleLabelout);

    // 閫熷害
    QHBoxLayout* mill_velocityLayout = new QHBoxLayout();
    QLabel* mill_velocityLabel = new QLabel("Velocity (x, y, z)(mm/r):");
    QLineEdit* mill_vxEdit = new QLineEdit("0");
    QLineEdit* mill_vyEdit = new QLineEdit("0.182");
    QLineEdit* mill_vzEdit = new QLineEdit("0");
    mill_velocityLayout->addWidget(mill_velocityLabel);
    mill_velocityLayout->addWidget(mill_vxEdit);
    mill_velocityLayout->addWidget(mill_vyEdit);
    mill_velocityLayout->addWidget(mill_vzEdit);
    millingLayout->addLayout(mill_velocityLayout);

    // 浠跨湡璁惧畾
    QHBoxLayout* mill_simulationLayout = new QHBoxLayout();
    QLabel* mill_simulationLabel = new QLabel("Simulation (total time, steptime for cut and mfem):");
    QLineEdit* mill_totaltimeEdit = new QLineEdit("1000");
    QLineEdit* mill_steptime_material_removal_Edit = new QLineEdit("0.05");
    QLineEdit* mill_steptime_MFEM_Edit = new QLineEdit("25");
    mill_simulationLayout->addWidget(mill_simulationLabel);
    mill_simulationLayout->addWidget(mill_totaltimeEdit);
    mill_simulationLayout->addWidget(mill_steptime_material_removal_Edit);
    mill_simulationLayout->addWidget(mill_steptime_MFEM_Edit);
    millingLayout->addLayout(mill_simulationLayout);

    // 绾︽潫
    QHBoxLayout* mill_constraintLayout = new QHBoxLayout();
    QLabel* mill_constraintsLabel = new QLabel("Constrains:");
    mill_constraintLayout->addWidget(mill_constraintsLabel);
    millingLayout->addLayout(mill_constraintLayout);
    // 绾︽潫 x
    QHBoxLayout* mill_xconstraint = new QHBoxLayout();
    QLabel* mill_xLabel = new QLabel("X:");
    QLineEdit* mill_x1Edit = new QLineEdit("-100");
    //x1Edit->setReadOnly(true);
    QLineEdit* mill_x2Edit = new QLineEdit("100");
    //x2Edit->setReadOnly(true);
    mill_xconstraint->addWidget(mill_xLabel);
    mill_xconstraint->addWidget(mill_x1Edit);
    mill_xconstraint->addWidget(mill_x2Edit);
    millingLayout->addLayout(mill_xconstraint);
    // 绾︽潫 y
    QHBoxLayout* mill_yconstraint = new QHBoxLayout();
    QLabel* mill_yLabel = new QLabel("Y:");
    QLineEdit* mill_y1Edit = new QLineEdit("-100");
    //y1Edit->setReadOnly(true);
    QLineEdit* mill_y2Edit = new QLineEdit("100");
    //y2Edit->setReadOnly(true);
    mill_yconstraint->addWidget(mill_yLabel);
    mill_yconstraint->addWidget(mill_y1Edit);
    mill_yconstraint->addWidget(mill_y2Edit);
    millingLayout->addLayout(mill_yconstraint);
    // 绾︽潫 z
    QHBoxLayout* mill_zconstraint = new QHBoxLayout();
    QLabel* mill_zLabel = new QLabel("z:");
    QLineEdit* mill_z1Edit = new QLineEdit("-100");
    //y1Edit->setReadOnly(true);
    QLineEdit* mill_z2Edit = new QLineEdit("0");
    //y2Edit->setReadOnly(true);
    mill_zconstraint->addWidget(mill_zLabel);
    mill_zconstraint->addWidget(mill_z1Edit);
    mill_zconstraint->addWidget(mill_z2Edit);
    millingLayout->addLayout(mill_zconstraint);

    // 鎸夐挳鍖哄煙
    QHBoxLayout* mill_buttonLayout = new QHBoxLayout();
    QPushButton* mill_runButton = new QPushButton("Run");
    mill_buttonLayout->addWidget(mill_runButton);
    mill_buttonLayout->addStretch();
    millingLayout->addLayout(mill_buttonLayout);

    // 杩炴帴鎸夐挳淇″彿
    connect(mill_workfileButton, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(
            &dialog, "选择工件模型", "", "STL Files (*.stl)");
        if (!path.isEmpty()) {
            workfilePath = path;
            mill_workfileEdit->setText(path);
        }
        });

    connect(mill_fileButton, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(
            &dialog, "Select cutter file", "", "Text Files (*.txt)");
        if (!path.isEmpty()) {
            cutedgefilePath = path;
            mill_fileEdit->setText(path);
        }
        });

    connect(mill_runButton, &QPushButton::clicked, [&]() {
        millpar.velocity[0] = mill_vxEdit->text().toDouble();
        millpar.velocity[1] = mill_vyEdit->text().toDouble();
        millpar.velocity[2] = mill_vzEdit->text().toDouble();
        millpar.constrain_limits[0][0] = mill_x1Edit->text().toDouble();
        millpar.constrain_limits[0][1] = mill_x2Edit->text().toDouble();
        millpar.constrain_limits[1][0] = mill_y1Edit->text().toDouble();
        millpar.constrain_limits[1][1] = mill_y2Edit->text().toDouble();
        millpar.constrain_limits[2][0] = mill_z1Edit->text().toDouble();
        millpar.constrain_limits[2][1] = mill_z2Edit->text().toDouble();
        millpar.simulation[0] = mill_totaltimeEdit->text().toDouble();
        millpar.simulation[1] = mill_steptime_material_removal_Edit->text().toDouble();
        millpar.simulation[2] = mill_steptime_MFEM_Edit->text().toDouble();
        millpar.blade_sum = mill_blade_sum_Edit->text().toDouble();
        millpar.spindle_speed = mill_spindle_speed_Edit->text().toDouble();
        millpar.force_coefs[0] = mill_a0KcEdit->text().toDouble();
        millpar.force_coefs[1] = mill_a1KcEdit->text().toDouble();
        millpar.force_coefs[2] = mill_a2KcEdit->text().toDouble();
        millpar.force_coefs[3] = mill_a3KcEdit->text().toDouble();
        millpar.force_coefs[4] = mill_a4KcEdit->text().toDouble();
        millpar.force_coefs[5] = mill_a5KcEdit->text().toDouble();


        dialog.accept();
        milling_executeSimulation(workfilePath, cutedgefilePath, file3Path);
        });
#pragma endregion
    ///————————————数字孪生页面————————————
#pragma region 
// 工件文件选择
    QHBoxLayout* digitaltwin_workfileLayout = new QHBoxLayout();
    QLabel* digitaltwin_workfileLabel = new QLabel("选择工件模型");
    QLineEdit* digitaltwin_workfileEdit = new QLineEdit();
    workfileEdit->setReadOnly(true);
    QPushButton* digitaltwin_workfileButton = new QPushButton("选择文件...");
    digitaltwin_workfileLayout->addWidget(digitaltwin_workfileLabel);
    digitaltwin_workfileLayout->addWidget(digitaltwin_workfileEdit, 1); // 1琛ㄧず鎷変几鍥犲瓙
    digitaltwin_workfileLayout->addWidget(digitaltwin_workfileButton);
    digitaltwinLayout->addLayout(digitaltwin_workfileLayout);


    // 绾︽潫
    QHBoxLayout* digitaltwin_constraintLayout = new QHBoxLayout();
    QLabel* digitaltwin_constraintsLabel = new QLabel("Constrains:");
    digitaltwin_constraintLayout->addWidget(digitaltwin_constraintsLabel);
    digitaltwinLayout->addLayout(digitaltwin_constraintLayout);
    // 绾︽潫 x
    QHBoxLayout* digitaltwin_xconstraint = new QHBoxLayout();
    QLabel* digitaltwin_xLabel = new QLabel("X:");
    QLineEdit* digitaltwin_x1Edit = new QLineEdit("-100");
    //x1Edit->setReadOnly(true);
    QLineEdit* digitaltwin_x2Edit = new QLineEdit("100");
    //x2Edit->setReadOnly(true);
    digitaltwin_xconstraint->addWidget(digitaltwin_xLabel);
    digitaltwin_xconstraint->addWidget(digitaltwin_x1Edit);
    digitaltwin_xconstraint->addWidget(digitaltwin_x2Edit);
    digitaltwinLayout->addLayout(digitaltwin_xconstraint);
    // 绾︽潫 y
    QHBoxLayout* digitaltwin_yconstraint = new QHBoxLayout();
    QLabel* digitaltwin_yLabel = new QLabel("Y:");
    QLineEdit* digitaltwin_y1Edit = new QLineEdit("-100");
    //y1Edit->setReadOnly(true);
    QLineEdit* digitaltwin_y2Edit = new QLineEdit("100");
    //y2Edit->setReadOnly(true);
    digitaltwin_yconstraint->addWidget(digitaltwin_yLabel);
    digitaltwin_yconstraint->addWidget(digitaltwin_y1Edit);
    digitaltwin_yconstraint->addWidget(digitaltwin_y2Edit);
    digitaltwinLayout->addLayout(digitaltwin_yconstraint);
    // 绾︽潫 z
    QHBoxLayout* digitaltwin_zconstraint = new QHBoxLayout();
    QLabel* digitaltwin_zLabel = new QLabel("z:");
    QLineEdit* digitaltwin_z1Edit = new QLineEdit("-100");
    //y1Edit->setReadOnly(true);
    QLineEdit* digitaltwin_z2Edit = new QLineEdit("5");
    //y2Edit->setReadOnly(true);
    digitaltwin_zconstraint->addWidget(digitaltwin_zLabel);
    digitaltwin_zconstraint->addWidget(digitaltwin_z1Edit);
    digitaltwin_zconstraint->addWidget(digitaltwin_z2Edit);
    digitaltwinLayout->addLayout(digitaltwin_zconstraint);

    // 鍒€鍏峰弬鏁拌緭鍏?
    QHBoxLayout* cutterParamsLayout = new QHBoxLayout();
    QLabel* cutterParamsLabel = new QLabel("Cutter params:");
    QLineEdit* cutterParamsEdit = new QLineEdit("1,6,6,31,0,6;0,6,6,31,6,31;");
    cutterParamsEdit->setPlaceholderText("格式: 类型,半径1,半径2,长度,z_start,z_end;...");
    cutterParamsLayout->addWidget(cutterParamsLabel);
    cutterParamsLayout->addWidget(cutterParamsEdit, 1);
    digitaltwinLayout->addLayout(cutterParamsLayout);

    // 鎸夐挳鍖哄煙
    QHBoxLayout* digitaltwin_buttonLayout = new QHBoxLayout();
    QPushButton* digitaltwin_runButton = new QPushButton("运行仿真");
    digitaltwin_buttonLayout->addWidget(digitaltwin_runButton);
    digitaltwin_buttonLayout->addStretch();
    digitaltwinLayout->addLayout(digitaltwin_buttonLayout);

    // 杩炴帴鎸夐挳淇″彿
    connect(digitaltwin_workfileButton, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(
            &dialog, "选择工件模型", "", "STL Files (*.stl)");
        if (!path.isEmpty()) {
            workfilePath = path;
            digitaltwin_workfileEdit->setText(path);
        }
        });

    std::vector<std::array<double, 3>> force_data;
    force_data.assign(350, { 100.0, 100.0, 0.0 });
    std::array<double, 14> Msh_Data{};
    // 淇濆瓨鍒€鍏峰弬鏁拌緭鍏ユ帶浠剁殑鎸囬拡
    this->cutterParamsEdit = cutterParamsEdit;

    connect(digitaltwin_runButton, &QPushButton::clicked, [&]() {
        digitaltwin_millpar.constrain_limits[0][0] = digitaltwin_x1Edit->text().toDouble();
        digitaltwin_millpar.constrain_limits[0][1] = digitaltwin_x2Edit->text().toDouble();
        digitaltwin_millpar.constrain_limits[1][0] = digitaltwin_y1Edit->text().toDouble();
        digitaltwin_millpar.constrain_limits[1][1] = digitaltwin_y2Edit->text().toDouble();
        digitaltwin_millpar.constrain_limits[2][0] = digitaltwin_z1Edit->text().toDouble();
        digitaltwin_millpar.constrain_limits[2][1] = digitaltwin_z2Edit->text().toDouble();

        // 瑙ｆ瀽鍒€鍏峰弬鏁?
        digitaltwin_millpar.cutter_segments.clear();
        QString params = cutterParamsEdit->text();
        QStringList segmentList = params.split(';');
        for (const QString& segStr : segmentList) {
            QStringList values = segStr.split(',');
            if (values.size() != 6) continue;

            int type = values[0].toInt();
            double radius1 = values[1].toDouble();
            double radius2 = values[2].toDouble();
            double length = values[3].toDouble();
            double z_start = values[4].toDouble();
            double z_end = values[5].toDouble();

            digitaltwin_millpar.cutter_segments.emplace_back(type, radius1, radius2, length, z_start, z_end);
        }

        dialog.accept();
        digitaltwin_milling_test_executeSimulation(workfilePath, Msh_Data, force_data);
        });
#pragma endregion

    // 杩炴帴鑿滃崟鏍忓垏鎹俊鍙?
    connect(broachingAction, &QAction::toggled, [&](bool checked) {
        if (checked) {
            millingAction->setChecked(false);
            stackedWidget->setCurrentIndex(0);
        }
        });

    connect(millingAction, &QAction::toggled, [&](bool checked) {
        if (checked) {
            broachingAction->setChecked(false);
            stackedWidget->setCurrentIndex(1);
        }
        });

    connect(digitaltwinAction, &QAction::toggled, [&](bool checked) {
        if (checked) {
            try {
                // 鍙栨秷鍏朵粬椤甸潰鐨勯€変腑鐘舵€?
                if (broachingAction) broachingAction->setChecked(false);
                if (millingAction) millingAction->setChecked(false);
                // 鍒囨崲鍒版暟瀛楀鐢熼〉闈?
                if (stackedWidget) stackedWidget->setCurrentIndex(2);
            }
            catch (const std::exception& e) {
                qDebug() << "数字孪生页面切换错误:" << e.what();
            }
        }
        });

    // 鏄剧ず瀵硅瘽妗?
    dialog.exec();
}

#if 0
void MdiChild::ExportCutsimResults()
{
    if (m_isMillingSimulationRunning) {
        QMessageBox::warning(this, tr("结果导出"), tr("铣削仿真仍在运行，请等待仿真完成后再导出结果。"));
        return;
    }

    if (!m_lastMillingCutsim) {
        QMessageBox::warning(this, tr("结果导出"), tr("当前没有可导出的铣削仿真结果，请先运行一次铣削仿真。"));
        return;
    }

    QString stlPath = QFileDialog::getSaveFileName(
        this,
        tr("导出切削后 STL"),
        QDir::currentPath() + "/cutsim_result.stl",
        tr("STL Files (*.stl)"));
    if (stlPath.isEmpty()) {
        return;
    }
    if (!stlPath.endsWith(".stl", Qt::CaseInsensitive)) {
        stlPath += ".stl";
    }

    QString errorMessage;
    if (!m_lastMillingCutsim->exportCurrentStl(stlPath, &errorMessage)) {
        QMessageBox::critical(this, tr("结果导出"), errorMessage);
        return;
    }

    QFileInfo stlInfo(stlPath);
    const QString forcePath = stlInfo.absolutePath() + "/" + stlInfo.completeBaseName() + "_force.csv";
    bool forceExported = false;
    if (forcewidget && forcewidget->hasData()) {
        forceExported = forcewidget->exportDataToCsv(forcePath, &errorMessage);
    }
    else {
        errorMessage = tr("当前没有切削力曲线数据可导出。STL 已成功导出。");
    }

    QString message = tr("STL 已导出：\n%1").arg(stlPath);
    if (forceExported) {
        message += tr("\n\n切削力数据已导出：\n%1").arg(forcePath);
        QMessageBox::information(this, tr("结果导出"), message);
    }
    else {
        message += tr("\n\n") + errorMessage;
        QMessageBox::warning(this, tr("结果导出"), message);
    }
}

#endif

void MdiChild::ExportCutsimResults()
{
    if (m_isMillingSimulationRunning || m_isDigitalTwinSimulationRunning) {
        QMessageBox::warning(this, tr("Result Export"), tr("Simulation is still running. Please export after it finishes."));
        return;
    }

    int exportSource = 0;
    if (m_lastCutsimExportSource == 2 && m_lastDigitalTwinMilling) {
        exportSource = 2;
    }
    else if (m_lastCutsimExportSource == 1 && m_lastMillingCutsim) {
        exportSource = 1;
    }
    else if (m_lastDigitalTwinMilling) {
        exportSource = 2;
    }
    else if (m_lastMillingCutsim) {
        exportSource = 1;
    }

    if (exportSource == 0) {
        QMessageBox::warning(this, tr("Result Export"), tr("No cut simulation result is available. Please run a milling or digital twin simulation first."));
        return;
    }

    const QString defaultFileName = (exportSource == 2)
        ? QStringLiteral("/digitaltwin_milling_result.stl")
        : QStringLiteral("/cutsim_result.stl");
    QString stlPath = QFileDialog::getSaveFileName(
        this,
        tr("Export Cut STL"),
        QDir::currentPath() + defaultFileName,
        tr("STL Files (*.stl)"));
    if (stlPath.isEmpty()) {
        return;
    }
    if (!stlPath.endsWith(".stl", Qt::CaseInsensitive)) {
        stlPath += ".stl";
    }

    QString errorMessage;
    bool stlExported = false;
    QString sourceName;
    if (exportSource == 2) {
        sourceName = QStringLiteral("digital twin milling");
        stlExported = m_lastDigitalTwinMilling->exportCurrentStl(stlPath, &errorMessage);
    }
    else {
        sourceName = QStringLiteral("Cutsim milling");
        stlExported = m_lastMillingCutsim->exportCurrentStl(stlPath, &errorMessage);
    }

    if (!stlExported) {
        QMessageBox::critical(this, tr("Result Export"), errorMessage);
        return;
    }

    QFileInfo stlInfo(stlPath);
    const QString forcePath = stlInfo.absolutePath() + "/" + stlInfo.completeBaseName() + "_force.csv";
    bool forceExported = false;
    if (forcewidget && forcewidget->hasData()) {
        forceExported = forcewidget->exportDataToCsv(forcePath, &errorMessage);
    }
    else {
        errorMessage = tr("No cutting force curve data is available. STL was exported successfully.");
    }

    QString message = tr("%1 STL exported:\n%2").arg(sourceName, stlPath);
    if (forceExported) {
        message += tr("\n\nCutting force data exported:\n%1").arg(forcePath);
        QMessageBox::information(this, tr("Result Export"), message);
    }
    else {
        message += tr("\n\n") + errorMessage;
        QMessageBox::warning(this, tr("Result Export"), message);
    }
}

void MdiChild::braoching_executeSimulation(QString stlfile, QString edgePointsfile, QString BladeAnglesfile)
{
    using broaching::CutsimBroaching;
    CutsimBroaching* myBroach = new CutsimBroaching(10);
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
            // 澶勭悊绌哄寘鍥寸洅
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
                // 鏂规硶1锛氫娇鐢?Shape() 鏂规硶
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
            Msg::ShowError("Input content is empty; using default model.");
            octreecenter[0] = 0;
            octreecenter[1] = -20;
            octreecenter[2] = -22;
            myBroach->setRectStock(octreecenter, 50.0);
        }
    }

    myBroach->setConstraints(broachpar.constrain_limits);
    myBroach->setVelocity(broachpar.velocity[0], broachpar.velocity[1], broachpar.velocity[2]);
    myBroach->setSimulationTimes(broachpar.simulation[0], broachpar.simulation[1], broachpar.simulation[2]); // total simulation time, increment time, very time for modal analysis
    myBroach->setResidualReleaseEnabled(broachpar.enable_machining_residual_stress);
    myBroach->setModalAnalysisEnabled(broachpar.enable_modal_analysis);
    qDebug() << "Machining residual stress test enabled:"
        << broachpar.enable_machining_residual_stress;
    qDebug() << "Modal analysis enabled:" << broachpar.enable_modal_analysis;
    myBroach->newBroach(broachpar.force_coefs);
    myBroach->addBroachs(edgePointsfile);
    //myBroach->performFEMSimulation(h_MyViewer);
    connect(myBroach, &CutsimBroaching::ApplyUpdateViewer, this, &MdiChild::updateView);
    connect(myBroach, &CutsimBroaching::ApplyUpdateForces, this->forcewidget, &ForceMonitorWidget::updateData);

    // 使用现代连接语法
#if 0
    connect(myBroach, &CutsimBroaching::straightnessDataUpdated,
        this, &MdiChild::onStraightnessDataUpdated);

    connect(myBroach, &CutsimBroaching::bladePointSelectionUpdated, this, [this](int blade_count, int point_count) {
        if (this->straightnessWidget) {
            this->straightnessWidget->updateBladePointSelection(blade_count, point_count);
        }
        });

    // 连接用户选择信号到 CutsimBroaching
    if (this->straightnessWidget) {
        connect(this->straightnessWidget, &StraightnessMonitorWidget::bladePointSelected, myBroach, &CutsimBroaching::setSelectedBladePoint);
    }

    // 启用选择功能
#endif
    myBroach->enableSelection(true);


    // 异步执行 FEM 模拟

    QFuture<void> future = QtConcurrent::run([this, myBroach]() {
        myBroach->performFEMSimulation(this, h_MyViewer, visulization_item, visulization_limits);
        });

}

void MdiChild::milling_executeSimulation(QString stlfile, QString edgePointsfile, QString BladeAnglesfile)
{
    using milling::CutsimMilling;
    CutsimMilling* myMill = new CutsimMilling(10);
    m_lastMillingCutsim = myMill;
    m_lastDigitalTwinMilling = nullptr;
    m_lastCutsimExportSource = 1;
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
        //h_MyViewer->Display(aShape);
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
        myMill->setStlStock(stlfile.toUtf8().constData(), partoffset, octreecenter, cube_size);
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
            myMill->setStlStock(p_TreeWidget->modelMap.value(tt).path, partoffset, octreecenter, cube_size);
        }
        else
        {
            Msg::ShowError("Input content is empty; using default model.");
            octreecenter[0] = 0;
            octreecenter[1] = 0;
            octreecenter[2] = 0;
            myMill->setRectStock(octreecenter, 50.0);
        }
    }
    myMill->newMill(millpar.force_coefs);
    myMill->setConstraints(millpar.constrain_limits);
    myMill->setVelocity(millpar.velocity[0], millpar.velocity[1], millpar.velocity[2]);
    myMill->setBlade_Sum(millpar.blade_sum);
    myMill->setSpindleSpeed(millpar.spindle_speed);
    myMill->setSimulationTimes(millpar.simulation[0], millpar.simulation[1], millpar.simulation[2]); // total simulation time, increment time, very time for modal analysis
    myMill->addMill(edgePointsfile);
    //myMill->performFEMSimulation(h_MyViewer);
    connect(myMill, &CutsimMilling::ApplyUpdateViewer, this, &MdiChild::updateView);
    connect(myMill, &CutsimMilling::ApplyUpdateForces, this->forcewidget, &ForceMonitorWidget::updateData);


    m_isMillingSimulationRunning = true;
    if (forcewidget) {
        forcewidget->clearData();
    }

    QFuture<void> future = QtConcurrent::run([this, myMill]() {
        myMill->performFEMSimulation(this, h_MyViewer, visulization_item, visulization_limits);
        QMetaObject::invokeMethod(this, [this]() {
            m_isMillingSimulationRunning = false;
            }, Qt::QueuedConnection);
        });
}

void MdiChild::digitaltwin_milling_test_executeSimulation(QString stlfile, std::array<double, 14>Msh_Data, std::vector<std::array<double, 3>>force_data)
{
    using digitaltwin_milling::DigitalTwinMilling;
    DigitalTwinMilling* mydigitaltwin_Mill = new DigitalTwinMilling(12);
    m_lastDigitalTwinMilling = mydigitaltwin_Mill;
    m_lastMillingCutsim = nullptr;
    m_lastCutsimExportSource = 2;
    if (forcewidget) {
        forcewidget->clearData();
    }

    double partoffset[3] = { 0,0,0 };
    double octreecenter[3] = { 0,0,0 };

    TopoDS_Shape aShape;
    StlAPI_Reader aReader;
    TopoDS_Shape aTempShape;
    Standard_Boolean Res = aReader.Read(aShape, stlfile.toUtf8().constData());
    if (Res)
    {
        //p_UI->showSuccess("STL璇嗗埆鎴愬姛");
        Msg::ShowInfo("STL璇嗗埆鎴愬姛");
        //aShape = aTempShape;
    }
    else Msg::ShowInfo("STL璇诲彇澶辫触");
    if (!aShape.IsNull()) {
        //h_MyViewer->Display(aShape);
        Bnd_Box bbox;
        BRepBndLib::Add(aShape, bbox);

        if (bbox.IsVoid()) {
            // 澶勭悊绌哄寘鍥寸洅
            return;
        }
        double xMin, yMin, zMin, xMax, yMax, zMax;
        bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        octreecenter[0] = (xMin + xMax) / 2;
        octreecenter[1] = (yMin + yMax) / 2;
        octreecenter[2] = (zMin + zMax) / 2;
        double cube_size = std::max({ -xMin + xMax, -yMin + yMax, -zMin + zMax }) + 1.0;
        mydigitaltwin_Mill->setStlStock(stlfile.toUtf8().constData(), partoffset, octreecenter, cube_size);
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
                // 鏂规硶1锛氫娇鐢?Shape() 鏂规硶
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
                // 澶勭悊绌哄寘鍥寸洅
                return;
            }
            double xMin, yMin, zMin, xMax, yMax, zMax;
            bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

            octreecenter[0] = (xMin + xMax) / 2 + partoffset[0];
            octreecenter[1] = (yMin + yMax) / 2 + partoffset[1];
            octreecenter[2] = (zMin + zMax) / 2 + partoffset[2];
            double cube_size = std::max({ -xMin + xMax, -yMin + yMax, -zMin + zMax }) + 1.0;
            mydigitaltwin_Mill->setStlStock(p_TreeWidget->modelMap.value(tt).path, partoffset, octreecenter, cube_size);
        }
        else
        {
            Msg::ShowError("Input content is empty; using default model.");
            octreecenter[0] = 0;
            octreecenter[1] = 0;
            octreecenter[2] = 0;
            mydigitaltwin_Mill->setRectStock(octreecenter, 50.0);
        }
    }
    mydigitaltwin_Mill->newMill();
    mydigitaltwin_Mill->setMch_Data(Msh_Data, force_data);
    mydigitaltwin_Mill->setConstraints(digitaltwin_millpar.constrain_limits);
    mydigitaltwin_Mill->setCutterParameters(digitaltwin_millpar.cutter_segments);

    connect(mydigitaltwin_Mill, &DigitalTwinMilling::ApplyUpdateViewer, this, &MdiChild::updateView);
    connect(mydigitaltwin_Mill, &DigitalTwinMilling::ApplyUpdateForces, this->forcewidget, &ForceMonitorWidget::updateData);
    // 鍚敤閫夋嫨鍔熻兘
    mydigitaltwin_Mill->enableSelection(true);

    // 寮傛鎵ц FEM 妯℃嫙

    m_isDigitalTwinSimulationRunning = true;
    if (forcewidget) {
        forcewidget->clearData();
    }

    QFuture<void> future = QtConcurrent::run([this, mydigitaltwin_Mill]() {
        mydigitaltwin_Mill->performFEMSimulation_test(this, h_MyViewer, visulization_item, visulization_limits);
        QMetaObject::invokeMethod(this, [this]() {
            m_isDigitalTwinSimulationRunning = false;
            }, Qt::QueuedConnection);
        });

}

void MdiChild::digitaltwin_milling_executeSimulation(QString stlfile)
{
    using digitaltwin_milling::DigitalTwinMilling;
    DigitalTwinMilling* mydigitaltwin_Mill = new DigitalTwinMilling(10);
    m_lastDigitalTwinMilling = mydigitaltwin_Mill;
    m_lastMillingCutsim = nullptr;
    m_lastCutsimExportSource = 2;
    if (forcewidget) {
        forcewidget->clearData();
    }

    double partoffset[3] = { 0,0,0 };
    double octreecenter[3] = { 0,0,0 };

    TopoDS_Shape aShape;
    StlAPI_Reader aReader;
    TopoDS_Shape aTempShape;
    Standard_Boolean Res = aReader.Read(aShape, stlfile.toUtf8().constData());
    if (Res)
    {
        //p_UI->showSuccess("STL璇嗗埆鎴愬姛");
        Msg::ShowInfo("STL璇嗗埆鎴愬姛");
        //aShape = aTempShape;
    }
    else Msg::ShowInfo("STL璇诲彇澶辫触");
    if (!aShape.IsNull()) {
        //h_MyViewer->Display(aShape);
        Bnd_Box bbox;
        BRepBndLib::Add(aShape, bbox);

        if (bbox.IsVoid()) {
            // 澶勭悊绌哄寘鍥寸洅
            return;
        }
        double xMin, yMin, zMin, xMax, yMax, zMax;
        bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

        octreecenter[0] = (xMin + xMax) / 2;
        octreecenter[1] = (yMin + yMax) / 2;
        octreecenter[2] = (zMin + zMax) / 2;
        double cube_size = std::max({ -xMin + xMax, -yMin + yMax, -zMin + zMax }) + 1.0;
        mydigitaltwin_Mill->setStlStock(stlfile.toUtf8().constData(), partoffset, octreecenter, cube_size);
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
                // 鏂规硶1锛氫娇鐢?Shape() 鏂规硶
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
                // 澶勭悊绌哄寘鍥寸洅
                return;
            }
            double xMin, yMin, zMin, xMax, yMax, zMax;
            bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

            octreecenter[0] = (xMin + xMax) / 2 + partoffset[0];
            octreecenter[1] = (yMin + yMax) / 2 + partoffset[1];
            octreecenter[2] = (zMin + zMax) / 2 + partoffset[2];
            double cube_size = std::max({ -xMin + xMax, -yMin + yMax, -zMin + zMax }) + 1.0;
            mydigitaltwin_Mill->setStlStock(p_TreeWidget->modelMap.value(tt).path, partoffset, octreecenter, cube_size);
        }
        else
        {
            Msg::ShowError("Input content is empty; using default model.");
            octreecenter[0] = 0;
            octreecenter[1] = 0;
            octreecenter[2] = 0;
            mydigitaltwin_Mill->setRectStock(octreecenter, 50.0);
        }
    }
    mydigitaltwin_Mill->newMill();
    mydigitaltwin_Mill->setConstraints(digitaltwin_millpar.constrain_limits);
    mydigitaltwin_Mill->setCutterParameters(digitaltwin_millpar.cutter_segments);

    connect(mydigitaltwin_Mill, &DigitalTwinMilling::ApplyUpdateViewer, this, &MdiChild::updateView);
    connect(mydigitaltwin_Mill, &DigitalTwinMilling::ApplyUpdateForces, this->forcewidget, &ForceMonitorWidget::updateData);
    // 鍚敤閫夋嫨鍔熻兘
    mydigitaltwin_Mill->enableSelection(true);

    // 寮傛鎵ц FEM 妯℃嫙

    mydigitaltwin_Mill->peformModalAnalysis();
    mydigitaltwin_Mill->setVibrParams();

    if (m_digitalTwinController) {
        m_digitalTwinController->setSimulationObject(mydigitaltwin_Mill);
        m_digitalTwinController->setRuntimeContext(
            this,
            h_MyViewer,
            visulization_item,
            visulization_limits
        );
        m_digitalTwinController->onInitializationFinished();
    }

}
