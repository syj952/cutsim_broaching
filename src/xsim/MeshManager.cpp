// MeshDialog.cpp
#include "MeshManager.h"
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QMessageBox>
#include <QTimer>
#include "ComplainUtf8.h"
#include "gmsh.h"
#include "Msg.h"

MeshDialog::MeshDialog(QWidget* parent,  OcctView* view)
    : QDialog(parent),m_view(view)
{
    setupHelpButton();
    setupUI(); 
    setupConnections(); //建立信号与槽连接关系
    setWindowTitle(tr("网格设置"));
    resize(400, 300);

}

MeshDialog::~MeshDialog()
{
    // 清理 GmshMessageHandler 单例实例
    if (GmshMessageHandler::m_instance) {
        delete GmshMessageHandler::m_instance;
        GmshMessageHandler::m_instance = nullptr;
    }
}

void MeshDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 网格类型选择
    QGroupBox* typeGroup = new QGroupBox(tr("网格划分算法"), this);
    QFormLayout* typeLayout = new QFormLayout(typeGroup);
    m_meshTypeCombo = new QComboBox(this);
    m_meshTypeCombo->addItem(tr("Delaunay"),1);
    m_meshTypeCombo->addItem(tr("New Delaunay"), 2);
    m_meshTypeCombo->addItem(tr("Frontal"), 4);
    m_meshTypeCombo->addItem(tr("Frontal Delaunay"), 5);
    m_meshTypeCombo->addItem(tr("Frontal Hex"), 6);
    m_meshTypeCombo->addItem(tr("MMG3D"), 7);
    m_meshTypeCombo->addItem(tr("R-tree"), 9);
    QHBoxLayout* comboButtonLayout = new QHBoxLayout;
    comboButtonLayout->addWidget(m_meshTypeCombo); // 先添加下拉框（在左）
    comboButtonLayout->addWidget(m_helpButton);    // 再添加按钮（在右）
    typeLayout->addRow(tr("网格划分算法:"), comboButtonLayout);
    typeGroup->setLayout(typeLayout);

    // 基本参数
    QGroupBox* basicGroup = new QGroupBox(tr("基本参数"), this);
    QFormLayout* basicLayout = new QFormLayout(basicGroup);

    m_cellminSizeSpin = new QDoubleSpinBox(this);
    //m_cellminSizeSpin->setRange(0.00001, 1000.0); //设置范围
    m_cellminSizeSpin->setDecimals(2);
    //m_cellminSizeSpin->setValue(0.0001);
    m_cellminSizeSpin->setSuffix(" mm");

    m_cellmaxSizeSpin = new QDoubleSpinBox(this);
	//m_cellmaxSizeSpin->setRange(0.00001, 1000.0);//设置范围
    m_cellmaxSizeSpin->setDecimals(2);
	//m_cellmaxSizeSpin->setValue(0.001);//默认值
    m_cellmaxSizeSpin->setSuffix(" mm");

    m_AngleTolerance = new QDoubleSpinBox(this);
    m_AngleTolerance->setRange(0.001, 1000.0);
    m_AngleTolerance->setValue(0.0);
    m_AngleTolerance->setSuffix(" °");

    m_GradationFactor = new QDoubleSpinBox(this);
    m_GradationFactor->setRange(0.001, 1000.0);
    m_GradationFactor->setValue(0);

    m_CharacteristicLengthFromPhysicalGroups = new QCheckBox(this);
	m_CharacteristicLengthFromPhysicalGroups->setChecked(true);

    m_AngleSmoothFactor = new QCheckBox( this);
    m_AngleSmoothFactor->setChecked(false);

    basicLayout->addRow(tr("单元最大尺寸:"), m_cellmaxSizeSpin);
	basicLayout->addRow(tr("单元最小尺寸:"), m_cellminSizeSpin);
    basicLayout->addRow(tr("设置几何近似角度:"), m_AngleTolerance);
    basicLayout->addRow(tr("设置网格尺寸渐变因子:"), m_GradationFactor);
    basicLayout->addRow(tr("物理组使用全局尺寸设置:"), m_CharacteristicLengthFromPhysicalGroups);
    basicLayout->addRow(tr("基于角度的网格细化:"), m_AngleSmoothFactor);
    basicGroup->setLayout(basicLayout);


    // 在基本参数组后添加约束处理组
    QGroupBox* constraintGroup = new QGroupBox(tr("约束处理"), this);
    QVBoxLayout* constraintLayout = new QVBoxLayout(constraintGroup);

    m_applyConstraintsCheck = new QCheckBox(tr("将固定约束应用到网格节点"), this);
    m_applyConstraintsCheck->setChecked(false);

    m_constraintInfoLabel = new QLabel(tr("约束信息: 无"), this);
    m_constraintInfoLabel->setStyleSheet("color: #666; font-size: 9pt;");

    constraintLayout->addWidget(m_applyConstraintsCheck);
    constraintLayout->addWidget(m_constraintInfoLabel);
    constraintGroup->setLayout(constraintLayout);

    // 插入到布局中合适的位置
    mainLayout->insertWidget(2, constraintGroup);

    // 高级参数
    QGroupBox* advancedGroup = new QGroupBox(tr("高级参数"), this);
    QFormLayout* advancedLayout = new QFormLayout(advancedGroup);

	m_divisionsXSpin = new QSpinBox(this);
	m_divisionsXSpin->setRange(1, 1000);
	m_divisionsXSpin->setValue(10);

	m_divisionsYSpin = new QSpinBox(this);
	m_divisionsYSpin->setRange(1, 1000);
	m_divisionsYSpin->setValue(10);

	m_divisionsZSpin = new QSpinBox(this);
	m_divisionsZSpin->setRange(1, 1000);
	m_divisionsZSpin->setValue(10);

    m_growthRateSpin = new QDoubleSpinBox(this);
    m_growthRateSpin->setRange(1.0, 2.0);
    m_growthRateSpin->setValue(1.2);
    m_growthRateSpin->setSingleStep(0.1);

    m_smoothingCheck = new QCheckBox(tr("启用网格平滑"), this);
    m_smoothingCheck->setChecked(false);

	advancedLayout->addRow(tr("X方向划分数:"), m_divisionsXSpin);
	advancedLayout->addRow(tr("Y方向划分数:"), m_divisionsYSpin);
	advancedLayout->addRow(tr("Z方向划分数:"), m_divisionsZSpin);
    advancedLayout->addRow(tr("增长率:"), m_growthRateSpin);
    advancedLayout->addRow(m_smoothingCheck);
    advancedGroup->setLayout(advancedLayout);

    // 预览
    QGroupBox* previewGroup = new QGroupBox(tr("预览"), this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    m_previewLabel = new QLabel(tr("网格参数预览"), this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setFrameStyle(QFrame::Box);
    m_previewLabel->setMinimumHeight(80);
    previewLayout->addWidget(m_previewLabel);
    previewGroup->setLayout(previewLayout);

    // 按钮
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    //网格生成相关
	m_generatedMeshBox = new QPushButton("生成网格", this);
    m_generatedMeshBox->setStyleSheet(R"(
    QPushButton {
        background-color: #4CAF50;
        color: white;
        font-size: 18px;
        font-family: "微软雅黑";
        min-width: 80px;
        min-height: 36px;
        padding: 6px 12px;
        border: none; /* 去掉默认边框 */
    }
    QPushButton:hover { /* 鼠标悬停 */
        background-color: #45a049; /* 稍暗的绿色 */
    }
    QPushButton:pressed { /* 鼠标按下 */
        background-color: #3d8b40; /* 更深的绿色 */
    }
)");
	connect(m_generatedMeshBox, &QPushButton::clicked, this, &MeshDialog::onGenerateMesh);

    // 布局
    mainLayout->addWidget(typeGroup);
    mainLayout->addWidget(basicGroup);
    mainLayout->addWidget(advancedGroup);
    mainLayout->addWidget(previewGroup);
	mainLayout->addWidget(m_generatedMeshBox);
    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);
}

void MeshDialog::setupHelpButton()
{
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    // 创建问号按钮
    m_helpButton = new QToolButton(this);
    m_helpButton->setText("?");
    m_helpButton->setFixedSize(30, 30);
    m_helpButton->setStyleSheet(R"(
        QToolButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #1976d2, stop:1 #1565c0);
            color: white;
            border: 2px solid #0d47a1;
            border-radius: 15px;
            font-weight: bold;
            font-size: 16px;
        }
        QToolButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #42a5f5, stop:1 #1e88e5);
            border: 2px solid #1976d2;
        }
        QToolButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #0d47a1, stop:1 #082e65);
        }
    )");

    // 连接信号槽
    connect(m_helpButton, &QToolButton::clicked, this, &MeshDialog::onHelpButtonClicked);
}

void MeshDialog::onHelpButtonClicked()
{
    showHelpInformation();
}

void MeshDialog::showHelpInformation() {
    QMessageBox* helpBox = new QMessageBox(this); 
    helpBox->setWindowTitle("网格设置帮助");
    helpBox->setIcon(QMessageBox::Information);
    helpBox->setText(
        "<h3>网格设置帮助</h3>"
        "<p>此窗口用于设置网格生成参数。</p>"
        "<b>网格划分算法</b>"
        "<ul>"
        "<li><b>Delaunay</b>: 基于Delaunay 准则的四面体划分（外接球内无其他顶点）,适用于简单几何模型。</li>"
        "<li><b>New Delaunay</b>: 结合了局部优化策略（如 Laplacian 平滑、边 / 面交换），优化了顶点插入顺序和边界处理逻辑</li>"
        "<li><b>Frontal</b>: 从几何边界（面 / 边）生成初始 “前沿面”，逐步向内推进前沿，填充四面体直至内部空间。适用于复杂几何和需要精确拟合场景</li>"
        "<li><b>Frontal Delaunay</b>: 融合前沿推进边界适应性的Delaunay算法，适用于复杂几何且需高网格质量的场景</li>"
        "<li><b>Frontal Hex</b>: 用前沿推进法生成 ** 六面体主导（Hex-dominant）** 网格（主体为六面体，局部用金字塔 / 四面体过渡）</li>"
        "<li><b>MMG3D </b>: 调用开源库MMG3D（MultiMesh Generation），专注于自适应四面体网格生成与优化，支持各向异性加密、网格自适应调整</li>"
        "</ul>"
        "<b>基本参数</b>"
        "<ul>"
        "<li><b>单元最大尺寸</b>: 网格单元的最大尺寸。</li>"
        "<li><b>单元最小尺寸</b>: 网格单元的最小尺寸。</li>"
        "<li><b>几何近似角度</b>: 用于几何近似的角度容差。</li>"
        "<li><b>网格尺寸渐变因子</b>: 控制网格尺寸变化的梯度。</li>"
        "<li><b>物理组使用全局尺寸设置</b>: 是否对物理组使用全局尺寸设置。</li>"
        "<li><b>基于角度的网格细化</b>: 根据角度细化网格。</li>"
        "</ul>"
        "<b>高级参数</b>"
        "<ul>"
        "<li><b>X方向划分分数</b>: X方向的网格划分数。</li>"
        "<li><b>Y方向划分分数</b>: Y方向的网格划分数。</li>"
        "<li><b>Z方向划分分数</b>: Z方向的网格划分数。</li>"
        "<li><b>增长率</b>: 网格尺寸增长的比率。</li>"
        "<li><b>启用网格平滑</b>: 是否启用网格平滑算法。</li>"
        "</ul>"
        "<b>预览</b>"
        "<ul>"
        "<li>显示当前网格设置的简要信息。</li>"
        "</ul>"
        "<p>点击“生成网格”按钮将根据当前设置生成网格。</p>"
    );
    helpBox->setAttribute(Qt::WA_DeleteOnClose);
    helpBox->setModal(false);// 设置非模态对话框
    helpBox->show();
   // helpBox.exec();
}

void MeshDialog::setupConnections()
{
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_meshTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MeshDialog::onMeshTypeChanged);

    // 连接参数变化信号
    connect(m_cellmaxSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MeshDialog::updatePreview);
    connect(m_cellminSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MeshDialog::updatePreview);
    connect(m_divisionsXSpin, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &MeshDialog::updatePreview);

    // 连接约束相关信号
    connect(m_applyConstraintsCheck, &QCheckBox::stateChanged,
        this, &MeshDialog::onApplyConstraintsChanged);

    // 初始更新约束信息
   // QTimer::singleShot(100, this, &MeshDialog::updateConstraintInfo);
}

//@brief 
void MeshDialog::onApplyConstraintsChanged(int state)
{
    updateConstraintInfo();
    //updatePreview();
}

void MeshDialog::onBoundaryConditionsUpdated()
{
}

//
BoundaryConditionDialog* MeshDialog::findBoundaryConditionDialog()
{

    // 待实现
    
    // 方法1: 通过父窗口查找
    //QWidget* parentWidget = parentWidget();
    //while (parentWidget) {
    //    // 查找类型为 BoundaryConditionDialog 的子窗口
    //    BoundaryConditionDialog* dialog = parentWidget->findChild<BoundaryConditionDialog*>();
    //    if (dialog) {
    //        return dialog;
    //    }
    //    parentWidget = parentWidget->parentWidget();
    //}

     //方法2: 通过应用程序的活动窗口查找
   // QWidget* activeWindow = qobject_cast<QWidget*>(QApplication::activeWindow());
    //if (activeWindow) {
        BoundaryConditionDialog* dialog = qobject_cast<MdiChild*>(parentWidget())->q3dView->dlg;
        if (dialog) {
            return dialog;
        }
   // }

    //// 方法3: 遍历所有顶层窗口
    //QList<QWidget*> topLevelWidgets = QApplication::topLevelWidgets();
    //for (QWidget* widget : topLevelWidgets) {
    //    BoundaryConditionDialog* dialog = widget->findChild<BoundaryConditionDialog*>();
    //    if (dialog) {
    //        return dialog;
    //    }
    //}

    return nullptr;
}

//@brief 根据约束状态更新生成网格按钮的外观和提示信息
void MeshDialog::updateGenerateMeshButton()
{
    bool hasConstraints = false;

    BoundaryConditionDialog* bcDialog = findBoundaryConditionDialog();
    hasConstraints = !(this->m_view->getBoundaryConditions().empty());

    // 如果有约束且启用了约束应用，可以添加一些视觉提示
    if (hasConstraints && m_applyConstraintsCheck->isChecked()) {
        m_generatedMeshBox->setStyleSheet(R"(
            QPushButton {
                background-color: #4CAF50;
                color: white;
                font-size: 18px;
                font-family: "微软雅黑";
                min-width: 80px;
                min-height: 36px;
                padding: 6px 12px;
                border: 2px solid #2E7D32;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:pressed {
                background-color: #3d8b40;
            }
        )");
        m_generatedMeshBox->setToolTip(tr("生成包含约束的网格"));
    }
    else {
        m_generatedMeshBox->setStyleSheet(R"(
            QPushButton {
                background-color: #2196F3;
                color: white;
                font-size: 18px;
                font-family: "微软雅黑";
                min-width: 80px;
                min-height: 36px;
                padding: 6px 12px;
                border: 2px solid #0D47A1;
            }
            QPushButton:hover {
                background-color: #1976D2;
            }
            QPushButton:pressed {
                background-color: #1565C0;
            }
        )");
        m_generatedMeshBox->setToolTip(tr("生成网格"));
    }
}

void MeshDialog::updateConstraintInfo()
{
    //if (!m_bcDialog) {
    //    m_constraintInfoLabel->setText(tr("边界条件对话框未连接"));
    //    return;
    //}
    Msg::ShowInfo("更新约束信息");
    QList<BoundaryConditionData> BoundaryConditions = m_view->getBoundaryConditions();
    QList<ConstraintSet> constraints = BoundaryConditionDialog::getAllConstraints(BoundaryConditions);
    int constraintCount = constraints.size();
    int totalEntities = 0;

    for (const auto& constraint : constraints) {
        totalEntities += constraint.shapes.Extent();
    }

    QString infoText;
    if (constraintCount > 0) {
        infoText = QString(tr("约束信息: %1 个约束集，%2 个几何实体"))
            .arg(constraintCount).arg(totalEntities);
    }
    else {
        infoText = tr("约束信息: 无约束集定义");
    }

    m_constraintInfoLabel->setText(infoText);
    updateGenerateMeshButton();
}

void MeshDialog::onMeshTypeChanged(int index)
{
    // 根据网格类型启用/禁用相关控件
    bool isRectangular = (index == 0); // 均匀矩形网格
    m_divisionsXSpin->setEnabled(isRectangular);
    m_divisionsYSpin->setEnabled(isRectangular);
    m_divisionsZSpin->setEnabled(isRectangular);

    updatePreview();
}

void MeshDialog::updatePreview()
{
    QString preview = QString("网格算法: %1\n最大尺寸: %2 mm\n最小尺寸：%3 mm\n 划分: %4×%5×%6")
        .arg(m_meshTypeCombo->currentData().toInt())
        .arg(m_cellmaxSizeSpin->value())
        .arg(m_cellminSizeSpin->value())
        .arg(m_divisionsXSpin->value())
        .arg(m_divisionsYSpin->value())
        .arg(m_divisionsZSpin->value());

    m_previewLabel->setText(preview);
}

// Getter 方法实现
int MeshDialog::meshAlgo() const {
    return m_meshTypeCombo->currentData().toInt();
}

MeshParameters MeshDialog::MeshParams() const
{
	MeshParameters params;
	params.meshAlgo = meshAlgo();
	params.maxSize = MaxcellSize();
	params.minSize = MincellSize();
	params.angleTolerance = AngleTolerance();
	params.gradationFactor = GradationFactor();
	params.characteristicLengthFromPhysicalGroups = CharacteristicLengthFromPhysicalGroups();
	params.angleSmoothFactor = AngleSmoothFactor();

	params.divisionsX = divisionsX();
	params.divisionsY = divisionsY();
	params.divisionsZ = divisionsZ();
	params.growthRate = growthRate();
	params.enableSmoothing = enableSmoothing();
	return params;

}

double MeshDialog::MaxcellSize() const {
    return m_cellmaxSizeSpin->value();
}

double MeshDialog::MincellSize() const {
    return m_cellminSizeSpin->value();
}

double MeshDialog::AngleTolerance() const
{
	return m_AngleTolerance->value();
}

double MeshDialog::GradationFactor() const
{
	return m_GradationFactor->value();
}

bool MeshDialog::CharacteristicLengthFromPhysicalGroups() const
{
	return m_CharacteristicLengthFromPhysicalGroups->isChecked();
}

bool MeshDialog::AngleSmoothFactor() const
{
	return m_AngleSmoothFactor->isChecked();
}

int MeshDialog::divisionsX() const {
    return m_divisionsXSpin->value();
}

int MeshDialog::divisionsY() const {
    return m_divisionsYSpin->value();
}

int MeshDialog::divisionsZ() const {
    return m_divisionsZSpin->value();
}

double MeshDialog::growthRate() const {
    return m_growthRateSpin->value();
}

bool MeshDialog::enableSmoothing() const {
    return m_smoothingCheck->isChecked();
}

void MeshDialog::onGenerateMesh()
{
    MeshParameters params = MeshParams();
    params.applyConstraints = m_applyConstraintsCheck->isChecked();
    generateMeshRequested(params);
}

void MeshDialog::onSolidSelected(const TopoDS_Solid& solid)
{

}

#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS.hxx>
bool MeshConstraintProcessor::processConstraints(const QList<ConstraintSet>& constraints,
    MeshParameters& meshParams,
    const QList<MeshNode>& meshNodes) {
    m_constraintInfos.clear();

    for (const auto& constraint : constraints) {
        if (!constraint.isValid()) continue;

        QList<int> constrainedNodeIds;

        // 遍历约束集中的所有形状
        for (OCCT_ShapeList::Iterator it(constraint.shapes); it.More(); it.Next()) {
            const TopoDS_Shape& currentShape = it.Value();
            QList<int> nodeIdsForShape;

            // 根据几何实体类型查找节点
            switch (currentShape.ShapeType()) {
            case TopAbs_FACE:
                nodeIdsForShape = findNodesOnFace(TopoDS::Face(currentShape), meshNodes);
                break;
            case TopAbs_EDGE:
                nodeIdsForShape = findNodesOnEdge(TopoDS::Edge(currentShape), meshNodes);
                break;
            case TopAbs_VERTEX:
                nodeIdsForShape = findNodesOnVertex(TopoDS::Vertex(currentShape), meshNodes);
                break;
            default:
                break;
            }

            // 合并当前形状的节点ID
            constrainedNodeIds.append(nodeIdsForShape);
        }

        if (!constrainedNodeIds.isEmpty()) {
            // 添加到网格参数
            meshParams.addConstraintInfo(constraint, constrainedNodeIds);

            // 存储到处理器中
            MeshConstraintInfo info;
            info.constraintId = constraint.entityId;
            info.constraintName = constraint.name;
            info.entityId = constraint.entityId;
            info.entityType = constraint.entityType;
            info.nodeIds = constrainedNodeIds;
            m_constraintInfos.append(info);
        }
    }

    return true;
}

QList<int> MeshConstraintProcessor::findNodesOnFace(const TopoDS_Face& face,
    const QList<MeshNode>& nodes) {
    QList<int> nodeIds;
    double tolerance = 1e-5;

    for (int i = 0; i < nodes.size(); ++i) {
        const MeshNode& node = nodes[i];
        gp_Pnt point(node.x, node.y, node.z);

        if (isPointOnFace(point, face, tolerance)) {
            nodeIds.append(i);
        }
    }

    return nodeIds;
}

bool MeshConstraintProcessor::isPointOnFace(const gp_Pnt& point, const TopoDS_Face& face, double tolerance) {
    BRepExtrema_DistShapeShape distCalc;
    distCalc.LoadS1(face);

    TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);
    distCalc.LoadS2(vertex);

    if (distCalc.Perform() && distCalc.Value() < tolerance) {
        return true;
    }

    return false;
}

QList<int> MeshConstraintProcessor::findNodesOnEdge(const TopoDS_Edge& edge,
    const QList<MeshNode>& nodes) {
    QList<int> nodeIds;
    double tolerance = 1e-5;

    for (int i = 0; i < nodes.size(); ++i) {
        const MeshNode& node = nodes[i];
        gp_Pnt point(node.x, node.y, node.z);

        if (isPointOnEdge(point, edge, tolerance)) {
            nodeIds.append(i);
        }
    }

    return nodeIds;
}

QList<int> MeshConstraintProcessor::findNodesOnVertex(const TopoDS_Vertex& vertex,
    const QList<MeshNode>& nodes) {
    QList<int> nodeIds;
    double tolerance = 1e-5;

    // 获取顶点坐标
    gp_Pnt vertexPoint = BRep_Tool::Pnt(vertex);

    for (int i = 0; i < nodes.size(); ++i) {
        const MeshNode& node = nodes[i];
        gp_Pnt nodePoint(node.x, node.y, node.z);

        // 计算节点到顶点的距离
        double distance = vertexPoint.Distance(nodePoint);
        if (distance < tolerance) {
            nodeIds.append(i);
        }
    }

    return nodeIds;
}

bool MeshConstraintProcessor::isPointOnEdge(const gp_Pnt& point, const TopoDS_Edge& edge, double tolerance) {
    BRepExtrema_DistShapeShape distCalc;
    distCalc.LoadS1(edge);

    TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);
    distCalc.LoadS2(vertex);

    if (distCalc.Perform() && distCalc.Value() < tolerance) {
        return true;
    }

    return false;
}

double MeshConstraintProcessor::distanceToShape(const gp_Pnt& point, const TopoDS_Shape& shape) {
    BRepExtrema_DistShapeShape distCalc;
    distCalc.LoadS1(shape);

    TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);
    distCalc.LoadS2(vertex);

    if (distCalc.Perform()) {
        return distCalc.Value();
    }

    return DBL_MAX; // 返回最大值表示计算失败
}

// 实现MeshConstraintProcessor的构造函数
MeshConstraintProcessor::MeshConstraintProcessor(Handle(AIS_InteractiveContext) context)
    : m_context(context) {
}

// 实现可视化约束节点方法
void MeshConstraintProcessor::visualizeConstraintNodes(Handle(AIS_InteractiveContext) context) {
    if (context.IsNull()) return;

    // 清除之前的可视化
    // 这里需要根据实际需求实现可视化逻辑
    // 例如，将约束节点用特殊颜色或标记显示出来
}


///////////////////////////*********************************///////////////////////////
///Gmsh消息处理类实现（还未完成）
#include "Msg.h"
#include <QMetaObject>
#include <QApplication>

GmshMessageHandler* GmshMessageHandler::m_instance = nullptr;

GmshMessageHandler* GmshMessageHandler::instance() {
    if (!m_instance) {
        m_instance = new GmshMessageHandler();
    }
    return m_instance;
}

GmshMessageHandler::GmshMessageHandler(QObject* parent) : QObject(parent) {
}

void GmshMessageHandler::gmshMessageCallback(const std::string& message) {
    if (m_instance && m_instance->m_isCapturing) {
        // 使用信号槽机制线程安全地发送消息
        QMetaObject::invokeMethod(m_instance, "messageReceived",
            Qt::QueuedConnection,
            Q_ARG(QString, QString::fromStdString(message)));
    }
}

void GmshMessageHandler::startCapturing() {
    m_isCapturing = true;
    // 设置Gmsh消息回调
    gmsh::option::setString("General.Verbosity", "5"); // 设置详细输出级别
}

void GmshMessageHandler::stopCapturing() {
    m_isCapturing = false;
}