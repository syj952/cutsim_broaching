#include "AngleDialog.h"
#include "ComplainUtf8.h"
#include <QMessageBox>
#include <gp_Vec.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <qdialog.h>
#include <QtWidgets>
#include <BRepAdaptor_Curve.hxx>
#include "Msg.h"
#include <qobjectdefs.h>
#include <BRepClass_FaceClassifier.hxx>
#include <utility>
#include <gp_Pln.hxx>
#include "mdichild.h"
#include <BRepBuilderAPI_MakeFace.hxx>
#include "TranslationDialog.h"
using namespace std;

vector<TopoDS_Face> AngleDialog::rakeFaces; //<前刀面
vector<TopoDS_Face> AngleDialog::clearanceFaces;  //<后刀面
std::vector<vector<double>> AngleDialog::pointsAndVec; //存储离散点的坐标和切向以及前后角度 x, y, z, 切向x, y, z, 刃倾角，前角，后角，切削速度，切削厚度，面ID
std::vector<std::vector<std::vector<double>>> AngleDialog::groupedPointsAndVec; //按切削刃分组存储离散点
gp_Vec AngleDialog::m_cuttingVec;
gp_Vec AngleDialog::m_cuttingDep; //齿升量，x，y，z
gp_Vec AngleDialog::m_cuttingDepDir1; // 齿高方向 (1,1,1) 代表不需要反向 (1,1,-1)代表z需要为负方向
gp_Vec AngleDialog::m_cuttingDepDir2; // 齿宽方向
gp_Vec AngleDialog::m_offsetDir; // 偏移方向
double AngleDialog::m_offsetDistance; // 偏移距离
array<int, 7> AngleDialog::m_depthoffset; // 齿升和偏移设置
int AngleDialog::m_offsetNumber; // 偏移数量
//vector<TopoDS_Vertex> AngleDialog::edgeVertexs; //<后刀面

// AngleDialog.cpp
AngleDialog::AngleDialog(QWidget* parent, OcctView* view)
	: QDialog(parent), m_view(view), m_parent(parent)
{
	Msg::ShowInfo("打开计算前后角对话框");
    setWindowTitle(tr("计算前后角"));
    this->setFixedWidth(300);
    QVBoxLayout* layout = new QVBoxLayout(this);

    // 0. 多条边融合离散按钮（添加在最前面）
    QPushButton* btnEdgeConnectDiscrete = new QPushButton(" 多条边融合离散", this);
    layout->addWidget(btnEdgeConnectDiscrete);
    MdiChild* mdiChild = qobject_cast<MdiChild*>(m_parent);
    if (mdiChild) {
        connect(btnEdgeConnectDiscrete, &QPushButton::clicked, mdiChild, &MdiChild::EdgeConnectDiscrete);
    }

    // 1. 前刀面选择按钮
    QPushButton* btnRake = new QPushButton(" 选择前刀面", this);
    layout->addWidget(btnRake);
    connect(btnRake, &QPushButton::clicked, this, &AngleDialog::onSelectRakeFace);
    lblRakeStatus = new QLabel(tr("未选择"), this);
    layout->addWidget(lblRakeStatus);

    // 2. 后刀面选择按钮
    QPushButton* btnClearance = new QPushButton(" 选择后刀面", this);
    layout->addWidget(btnClearance);
    connect(btnClearance, &QPushButton::clicked, this, &AngleDialog::onSelectClearanceFace);
    lblClearanceStatus = new QLabel(tr("未选择"), this);
    layout->addWidget(lblClearanceStatus);

    // 2.1 手动测试两面是否可识别切削刃
    btnTestSelectedFacesEdge = new QPushButton(" 测试当前选中两面选刃", this);
    layout->addWidget(btnTestSelectedFacesEdge);
    if (mdiChild) {
        connect(btnTestSelectedFacesEdge, &QPushButton::clicked, mdiChild, &MdiChild::TestSelectedFacesCuttingEdge);
    }
    // 3. 切削方向设置按钮
    btnCuttingDir = new QPushButton(" 设置切削速度和方向", this);
    layout->addWidget(btnCuttingDir);
    connect(btnCuttingDir, &QPushButton::clicked, this, &AngleDialog::onSetCuttingVelocity);

    // 4. 切削厚度设置按钮
    btnCuttingDep = new QPushButton(" 设置切削厚度", this);
    layout->addWidget(btnCuttingDep);
    connect(btnCuttingDep, &QPushButton::clicked, this, &AngleDialog::onSetCuttingDepth);

	// 5. 计算前后角按钮
	btnComputeAngles = new QPushButton(" 计算切削工况并生成一个刃", this);
	layout->addWidget(btnComputeAngles);
    mdiChild = qobject_cast<MdiChild*>(m_parent);
	connect(btnComputeAngles, &QPushButton::clicked, mdiChild, &MdiChild::ComputeConditions);// 只生成一个刃
    
    // 6. 切削刃偏置，生成多个切削刃
    btnEdgeOffset = new QPushButton("计算切削工况并偏置生成多个刃", this);
    layout->addWidget(btnEdgeOffset);
    connect(btnEdgeOffset, &QPushButton::clicked, mdiChild, &MdiChild::ComputeandOffsetEdge); // 生成多个刃

    // 连接视图信号
    connect(m_view, &OcctView::faceSelected, this, &AngleDialog::onRakeFaceSelected);
    connect(m_view, &OcctView::faceSelected, this, &AngleDialog::onClearanceFaceSelected);
    connect(m_view, &OcctView::edgeSelected, this, &AngleDialog::onEdgeSelected);
}

AngleDialog::~AngleDialog()
{
   
}

// 选择前刀面
void AngleDialog::onSelectRakeFace() {
    m_view->setSelectionMode(OcctView::FaceSelection);
	Msg::ShowInfo("请选择前刀面...");
	emit selRakeFaceSignal();

    if (rakeFaces.size() > 0)
    {
        lblRakeStatus->setText(tr("前刀面已捕捉"));
    }

   
}
// 选择后刀面
void AngleDialog::onSelectClearanceFace()
{
    m_view->setSelectionMode(OcctView::FaceSelection);
    Msg::ShowInfo("请选择后刀面...");
    emit selClearanceFaceSignal();
    if (clearanceFaces.size() > 0)
    {
        lblClearanceStatus->setText(tr("后刀面已捕捉"));
    }
}

// 前刀面选择回调
void AngleDialog::onRakeFaceSelected(const TopoDS_Face& face) {
    //m_rakeFace = face;
    lblRakeStatus->setText(tr("已选择"));
    //statusBar()->clearMessage();
}

//后刀面选择回调
void AngleDialog::onClearanceFaceSelected(const TopoDS_Face& face)
{
	//m_clearanceFace = face;
	lblClearanceStatus->setText(tr("已选择"));
	//statusBar()->clearMessage();
}



// 切削方向设置（弹出子对话框）
void AngleDialog::onSetCuttingVelocity() {
    //QDialog subDialog(this);
    //QVBoxLayout* subLayout = new QVBoxLayout(&subDialog);

    // 选项1：手动输入向量
    TranslationDialog dialog(this, 6, m_cuttingVec.X(), m_cuttingVec.Y(), m_cuttingVec.Z());


    if (dialog.exec() != QDialog::Accepted) return;
    double x = dialog.getX();
    double y = dialog.getY();
    double z = dialog.getZ();
	m_cuttingVec.SetCoord(x, y, z);

    //// 选项2：选择边计算切向
    //QPushButton* btnEdge = new QPushButton(tr("选择边计算切向"), &subDialog);
    //subLayout->addWidget(btnEdge);
    //connect(btnEdge, &QPushButton::clicked, [&] {
    //    m_view->setSelectionMode(OcctView::EdgeSelection);
    //    //statusBar()->showMessage(tr("请选择一条边..."));
    //    subDialog.hide(); // 隐藏子对话框等待用户选择
    //    });

    //subDialog.exec();
}

// 切削厚度设置（弹出子对话框）
void AngleDialog::onSetCuttingDepth() {
    //QDialog subDialog(this);
    //QVBoxLayout* subLayout = new QVBoxLayout(&subDialog);

    // 选项1：手动输入向量
    TranslationDialog dialog(this, 7, m_cuttingDep.X(), m_cuttingDep.Y(), m_cuttingDep.Z(), m_depthoffset.data(), m_offsetDistance, m_offsetNumber);
    if (dialog.exec() != QDialog::Accepted) return;
    double x = dialog.getX();
    double y = dialog.getY();
    double z = dialog.getZ();    
    m_cuttingDep.SetCoord(x, y, z);
    std::array<double, 11> direction = { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    dialog.getDepthDirection(direction.data(), direction.size(), m_depthoffset.data());
    m_cuttingDepDir1.SetCoord(direction[0], direction[1], direction[2]);
    m_cuttingDepDir2.SetCoord(direction[3], direction[4], direction[5]);
    m_offsetDir.SetCoord(direction[6], direction[7], direction[8]);
    m_offsetDistance = direction[9];
    m_offsetNumber = direction[10];
    //m_offsetDistance
}

// 边选择回调（计算切向）
void AngleDialog::onEdgeSelected(const TopoDS_Edge& edge) {
    BRepAdaptor_Curve curve(edge);
    gp_Pnt p1, p2;
    curve.D0(curve.FirstParameter(), p1);
    curve.D0(curve.LastParameter(), p2);
    m_cuttingVec = gp_Vec(p1, p2).Normalized();
    //statusBar()->showMessage(tr("切削向量已设置: (%1, %2, %3)")
   //     .arg(m_cuttingVector.X()).arg(m_cuttingVector.Y()).arg(m_cuttingVector.Z()));
}
