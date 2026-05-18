#include "BoundaryConditionDialog.h"
#include "OcctView.h"
#include "Msg.h"
#include <QtWidgets>
#include <TopExp_Explorer.hxx>
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include "ComplainUtf8.h"
#include <TopTools_IndexedMapOfShape.hxx>

BoundaryConditionDialog::BoundaryConditionDialog(OcctView* view, QWidget* parent)
    : QDialog(parent), m_view(view)
{
    setupUI();
    setupConnections();
    setWindowTitle("施加边界条件");
    setMinimumSize(800, 600);
}

BoundaryConditionDialog::~BoundaryConditionDialog()
{

}

BoundaryConditionData BoundaryConditionDialog::getBoundaryConditionData() const
{
    return BoundaryConditionData();
}

void BoundaryConditionDialog::setSelectedShape(const Handle(AIS_InteractiveObject)& shape)
{

}

void BoundaryConditionDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 顶部：边界条件设置区域
    QGroupBox* settingsGroup = new QGroupBox("边界条件设置");
    QGridLayout* settingsLayout = new QGridLayout(settingsGroup);

    // 第一行：名称和类型
    settingsLayout->addWidget(new QLabel("名称:"), 0, 0);
    m_bcNameEdit = new QLineEdit("BC-1");
    settingsLayout->addWidget(m_bcNameEdit, 0, 1);

    settingsLayout->addWidget(new QLabel("类型:"), 0, 2);
    m_bcTypeCombo = new QComboBox();
    m_bcTypeCombo->addItems({ "固定约束", "位移约束", "力载荷", "压力载荷", "温度载荷" });
    settingsLayout->addWidget(m_bcTypeCombo, 0, 3);

    settingsLayout->addWidget(new QLabel("施加对象:"), 0, 4);
    m_entityTypeCombo = new QComboBox();
    m_entityTypeCombo->addItems({ "面", "边", "点" });
    settingsLayout->addWidget(m_entityTypeCombo, 0, 5);

    // 第二行：数值设置表格
    m_valueTable = new QTableWidget(0, 2);
    m_valueTable->setHorizontalHeaderLabels({ "参数", "数值" });
    m_valueTable->horizontalHeader()->setStretchLastSection(true);
    settingsLayout->addWidget(m_valueTable, 1, 0, 1, 6);

    mainLayout->addWidget(settingsGroup);

    // 中部：几何实体列表和边界条件列表
    QHBoxLayout* middleLayout = new QHBoxLayout();

    // 几何实体列表
    QGroupBox* entityGroup = new QGroupBox("几何实体");
    QVBoxLayout* entityLayout = new QVBoxLayout(entityGroup);
    m_entityTree = new QTreeWidget();
    m_entityTree->setHeaderLabels(QStringList() << "ID" << "类型" << "描述");
    entityLayout->addWidget(m_entityTree);

    // 边界条件列表
    QGroupBox* bcGroup = new QGroupBox("已定义的边界条件");
    QVBoxLayout* bcLayout = new QVBoxLayout(bcGroup);
    m_bcList = new QTreeWidget();
    m_bcList->setHeaderLabels(QStringList() << "名称" << "类型" << "对象" << "施加对象数量");
    
    QHeaderView* bcHeader = m_bcList->header();    // 新增：设置列宽策略
    // 自适应内容宽度
    bcHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents); 
    bcHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents); 
    bcHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents); 
    
    /*bcHeader->setSectionResizeMode(3, QHeaderView::Stretch);*/       //自动拉伸，填充剩余空间 避免被隐藏

    bcLayout->addWidget(m_bcList);

    middleLayout->addWidget(entityGroup, 1);
    middleLayout->addWidget(bcGroup, 1);
    mainLayout->addLayout(middleLayout, 1);

    // 底部：按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_addButton = new QPushButton("添加");
    m_removeButton = new QPushButton("删除");
    m_applyButton = new QPushButton("应用");
    QPushButton* closeButton = new QPushButton("关闭");

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // 设置样式
    setStyleSheet(R"(
        QDialog {
            background: white;
        }
        QGroupBox {
            font-weight: bold;
            margin-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top center;
            padding: 0 8px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #1976d2, stop:1 #1565c0);
            color: white;
            border-radius: 4px;
        }
        QTableWidget {
            border: 1px solid #ddd;
            gridline-color: #ddd;
        }
        QTreeWidget {
            border: 1px solid #ddd;
        }
    )");
}

//@brief 设置信号与槽连接机制
void BoundaryConditionDialog::setupConnections()
{
	MdiChild* mdichild = qobject_cast<MdiChild*>(parentWidget());
    connect(m_bcTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &BoundaryConditionDialog::onBCTypeChanged);
    connect(m_entityTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &BoundaryConditionDialog::onEntityTypeChanged);
    connect(m_addButton, &QPushButton::clicked, this, &BoundaryConditionDialog::onAddBC);
    connect(m_removeButton, &QPushButton::clicked, this, &BoundaryConditionDialog::onRemoveBC);
    connect(m_applyButton, &QPushButton::clicked, this, &BoundaryConditionDialog::onApply);
    // 连接边界条件对话框的信号
    connect(this, &BoundaryConditionDialog::boundaryConditionAdded,
        mdichild->p_TreeWidget, &ProjectTree::addBoundaryConditionToTree);
    if (m_view) {
		connect(m_view, &OcctView::selectionChanged, this, &BoundaryConditionDialog::onEntityTypeChanged); // 连接选择变化信号
    }
}

//@brief 根据当前边界条件类型更新数值输入字段
void BoundaryConditionDialog::populateEntityList()
{
    m_entityTree->clear();
    uniqueShapes.Clear();
    if (m_view->myContext->HasSelectedShape()) {

        Handle(AIS_InteractiveObject) selectedObject = m_view->myContext->SelectedInteractive();

        Handle(AIS_Shape) selectedAISShape = Handle(AIS_Shape)::DownCast(selectedObject);

        m_selectedShape = m_view->myContext->SelectedShape();
        //m_selectedShape = selectedAISShape->Shape();

    }

    if (m_selectedShape.IsNull()) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, "0");
        item->setText(1, "无");
        item->setText(2, "请选择几何实体");
        m_entityTree->addTopLevelItem(item);
        return;
    }
    QString entityType = m_entityTypeCombo->currentText();
   
    // 根据实体类型遍历几何形状
    TopExp_Explorer explorer;
    if (entityType == "面") {
        explorer.Init(m_selectedShape, TopAbs_FACE);
    }
    else if (entityType == "边") {
        explorer.Init(m_selectedShape, TopAbs_EDGE);
    }
    else if (entityType == "点") {
        explorer.Init(m_selectedShape, TopAbs_VERTEX);
    }

    int entityId = 1;
    while (explorer.More()) {
        const TopoDS_Shape& currentShape = explorer.Current();
        if (!uniqueShapes.Contains(currentShape)) {
            uniqueShapes.Add(currentShape);
            QTreeWidgetItem* item = new QTreeWidgetItem();
            item->setText(0, QString::number(entityId));
            item->setText(1, entityType);
            item->setText(2, QString("%1 %2").arg(entityType).arg(entityId));
            m_entityTree->addTopLevelItem(item);
            entityId++;
        }
        explorer.Next();
    }
    m_view->myContext->InitSelected();
}

//@brief 创建边界条件预览
void BoundaryConditionDialog::createBCPreview()
{
}

//@breif 更新边界条件列表
void BoundaryConditionDialog::updateBCList()
{                                   
}

//@brief 添加信息到右下角边界条件列表
void BoundaryConditionDialog::addBCToList(const BoundaryConditionData& bcData)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, bcData.name);
    item->setText(1, bcData.type);
    item->setText(2, bcData.entityType + QString::number(bcData.entityId));
    QStringList valueStrings;
    for (auto it = bcData.values.begin(); it != bcData.values.end(); ++it) {
        valueStrings.append(QString("%1: %2").arg(it.key()).arg(it.value()));
    }
    //item->setText(3, valueStrings.join(", "));
    item->setText(3, QString::number(uniqueShapes.Size()));
	m_bcList->addTopLevelItem(item);//将边界条件添加到右下角的列表中
	m_view->AddBoundaryCondition(bcData); // 将边界条件添加到OcctView的列表中
}


//@brief 应用边界条件
void BoundaryConditionDialog::applyBoundaryConditions(const QList<BoundaryConditionData>& bcList)
{
    for (const BoundaryConditionData& bcData : bcList) {
       //在这里实现将边界条件应用到模型的逻辑
        emit boundaryConditionAdded(bcData);
    }
}

//@brief 将边界条件显示在视图中
void BoundaryConditionDialog::displayBC(const BoundaryConditionData& bcData)
{
}

//@brief 获取所有的约束集
QList<ConstraintSet> BoundaryConditionDialog::getAllConstraints(QList<BoundaryConditionData>& bcList)
{
    QList<ConstraintSet> constraints;
    for (const auto& bcData : bcList) {
        constraints.append(bcData.toConstraintSet());
    }
    return constraints;
}

QString BoundaryConditionDialog::generateNextBCName()
{
    m_bcCounter++;
    return QString("BC-%1").arg(m_bcCounter);
}

//@brief 边界条件类型改变相应函数
void BoundaryConditionDialog::onBCTypeChanged(int index)
{
    updateValueFields();
}

//@brief 对象类型改变相应函数
void BoundaryConditionDialog::onEntityTypeChanged()
{
    populateEntityList(); // 实体类型改变时重新填充列表
}

//@brief 添加边界条件
void BoundaryConditionDialog::onAddBC()
{
    Msg::ShowInfo("添加边界条件");
    if (!m_bcNameEdit || !m_bcTypeCombo) {
        Msg::ShowError("UI控件未初始化");
        return;
    }


    QString name = m_bcNameEdit->text().trimmed();
    if (name.isEmpty()) {
        Msg::ShowWarning("请输入边界条件名称");
        return;
    }

    // 创建边界条件数据
    BoundaryConditionData bcData;
    bcData.name = name;
    bcData.type = m_bcTypeCombo->currentText();
	bcData.entityId = m_CurrentEntityId;
	bcData.entityType = m_entityTypeCombo->currentText();
    OCCT_ShapeList shapeList;
    for (Standard_Integer i = 1; i <= uniqueShapes.Size(); ++i)
    {
        const TopoDS_Shape& shape = uniqueShapes(i);
        shapeList.Append(shape);
    }
    bcData.shapes = shapeList;
    // 收集数值
    for (auto it = m_valueWidgets.begin(); it != m_valueWidgets.end(); ++it) {
        if (it.value()) {
            bcData.values[it.key()] = it.value()->value();
        }
    }

    // 添加到列表
    addBCToList(bcData);
    //displayBC(bcData);//还没实现
    m_CurrentEntityId++;

    // 生成自动名称
    QString bcName = generateNextBCName();
    m_bcNameEdit->setText(bcName);
}

//@brief 移除选中的边界条件
void BoundaryConditionDialog::onRemoveBC()
{
    QList<QTreeWidgetItem*> selectedItems = m_bcList->selectedItems();
    if (selectedItems.isEmpty()) {
        Msg::ShowWarning("请选择要删除的边界条件");
        return;
    }
    for (QTreeWidgetItem* item : selectedItems) {
        delete item;
    }
}

//@brief 应用边界条件
void BoundaryConditionDialog::onApply()
{
    if (m_bcList->topLevelItemCount() == 0) {
        Msg::ShowWarning("没有可应用的边界条件");
        return;
    }

    // 收集所有边界条件
    for (int i = 0; i < m_bcList->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_bcList->topLevelItem(i);
        // 从item中获取边界条件数据
        BoundaryConditionData bcData = getBCFromItem(item);
        if (!bcData.name.isEmpty()) {
            bcList.append(bcData);
        }
    }
    
    // 应用边界条件
    applyBoundaryConditions(m_view->getBoundaryConditions());
    Standard_Character buffer[1024];
    Sprintf(buffer, "应用了%d个边界条件", bcList.size());
	Msg::ShowInfo(buffer);
    //Msg::ShowInfo("边界条件应用完成");

}

// 辅助函数：从列表项获取边界条件数据
BoundaryConditionData BoundaryConditionDialog::getBCFromItem(QTreeWidgetItem* item)
{
    BoundaryConditionData data;
    if (!item) return data;

    data.name = item->text(0);
    data.type = item->text(1);
    // 从item的数据中恢复其他信息

    return data;
}

void BoundaryConditionDialog::onSelectionChanged()
{
    // 需要与OcctView的选择信号连接
    if (m_view) {
        // 获取当前选中的形状
        // TopoDS_Shape selectedShape = m_view->getSelectedShape();
        // setSelectedShape(selectedShape);
    }
}

//@brief 根据当前边界条件类型更新数值输入字段
void BoundaryConditionDialog::updateValueFields()
{
    m_valueTable->setRowCount(0);
    m_valueWidgets.clear();

    QString bcType = m_bcTypeCombo->currentText();

    if (bcType == "固定约束") {
        // 固定约束通常不需要数值
        m_valueTable->setRowCount(0);
    }
    else if (bcType == "位移约束") {
        m_valueTable->setRowCount(3);

        QStringList dofs = { "X方向位移", "Y方向位移", "Z方向位移" };
        for (int i = 0; i < 3; i++) {
            m_valueTable->setItem(i, 0, new QTableWidgetItem(dofs[i]));

            QDoubleSpinBox* spinBox = new QDoubleSpinBox();
            spinBox->setRange(-1000, 1000);
            spinBox->setDecimals(6);
            spinBox->setSuffix(" mm");
            m_valueTable->setCellWidget(i, 1, spinBox);
            m_valueWidgets[dofs[i]] = spinBox;
        }
    }
    else if (bcType == "力载荷") {
        m_valueTable->setRowCount(3);

        QStringList forces = { "X方向力", "Y方向力", "Z方向力" };
        for (int i = 0; i < 3; i++) {
            m_valueTable->setItem(i, 0, new QTableWidgetItem(forces[i]));

            QDoubleSpinBox* spinBox = new QDoubleSpinBox();
            spinBox->setRange(-10000, 10000);
            spinBox->setDecimals(3);
            spinBox->setSuffix(" N");
            m_valueTable->setCellWidget(i, 1, spinBox);
            m_valueWidgets[forces[i]] = spinBox;
        }
    }
    else if (bcType == "压力载荷") {
        m_valueTable->setRowCount(1);

        m_valueTable->setItem(0, 0, new QTableWidgetItem("压力值"));

        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        spinBox->setRange(-100, 100);
        spinBox->setDecimals(3);
        spinBox->setSuffix(" MPa");
        m_valueTable->setCellWidget(0, 1, spinBox);
        m_valueWidgets["压力值"] = spinBox;
    }
    else if (bcType == "温度载荷") {
        m_valueTable->setRowCount(1);

        m_valueTable->setItem(0, 0, new QTableWidgetItem("温度值"));

        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        spinBox->setRange(-273, 3000);
        spinBox->setDecimals(1);
        spinBox->setSuffix(" °C");
        m_valueTable->setCellWidget(0, 1, spinBox);
        m_valueWidgets["温度值"] = spinBox;
    }
}

// 其他成员函数实现...