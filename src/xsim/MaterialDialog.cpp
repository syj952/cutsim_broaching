#include "MaterialDialog.h"
#include "Msg.h"
#include <QMessageBox>
#include <QMetaType>

MaterialDialog::MaterialDialog(QWidget* parent, QList<MaterialProperty>* materials)
	: QDialog(parent), m_materials(materials), m_currentIndex(-1)
{
	setWindowTitle("输入材料参数");
	setMinimumSize(700, 500);
	setupUI();
	setupConnections();
	if (m_materials) {
		updateMaterialList();
	}
}

MaterialDialog::~MaterialDialog()
{
}

void MaterialDialog::setupUI()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// 左侧：材料参数输入区域
	QHBoxLayout* contentLayout = new QHBoxLayout();

	// 左侧：输入表单
	QGroupBox* inputGroup = new QGroupBox("材料参数输入");
	QFormLayout* formLayout = new QFormLayout(inputGroup);

	// 材料名称
	m_nameEdit = new QLineEdit();
	m_nameEdit->setPlaceholderText("请输入材料名称");
	formLayout->addRow("材料名称:", m_nameEdit);

	// 杨氏模量
	m_youngsModulusEdit = new QDoubleSpinBox();
	m_youngsModulusEdit->setRange(0.0, 1e6);
	m_youngsModulusEdit->setValue(0.0);
	m_youngsModulusEdit->setDecimals(6);
	m_youngsModulusEdit->setSuffix(" MPa");
	m_youngsModulusEdit->setSingleStep(1e0);
	formLayout->addRow("杨氏模量:", m_youngsModulusEdit);

	// 泊松比
	m_poissonRatioEdit = new QDoubleSpinBox();
	m_poissonRatioEdit->setRange(0.0, 0.5);
	m_poissonRatioEdit->setValue(0.0);
	m_poissonRatioEdit->setDecimals(6);
	m_poissonRatioEdit->setSingleStep(0.01);
	formLayout->addRow("泊松比:", m_poissonRatioEdit);

	// 密度
	m_densityEdit = new QDoubleSpinBox();
	m_densityEdit->setRange(0.0, 20);
	m_densityEdit->setValue(0.0);
	m_densityEdit->setDecimals(3);
	m_densityEdit->setSuffix(" e-9ton/mm³");
	m_densityEdit->setSingleStep(0.01);
	formLayout->addRow("密度:", m_densityEdit);

	//QDoubleSpinBox* m_thermalConductivityEdit; // 热传导系数输入框
	//QDoubleSpinBox* m_heatCapacityEdit;  // 比热容输入框
	//QDoubleSpinBox* m_AEdit;       // JC A输入框
	//QDoubleSpinBox* m_BEdit; // JC B输入框
	//QDoubleSpinBox* m_nEdit;  // JC n输入框
	//QDoubleSpinBox* m_cEdit;       // JC c输入框
	//QDoubleSpinBox* m_mEdit;  // JC m输入框
	//QDoubleSpinBox* m_TmEdit;       // JC Tm输入框
	//QDoubleSpinBox* m_T0Edit;       // JC T0输入框
	//QDoubleSpinBox* m_epspdotEdit;       // JC epspdot输入框
	// 热传导系数
	m_thermalConductivityEdit = new QDoubleSpinBox();
	m_thermalConductivityEdit->setRange(0.0, 1e15);
	m_thermalConductivityEdit->setValue(0.0);
	m_thermalConductivityEdit->setDecimals(6);
	m_thermalConductivityEdit->setSuffix(" W/m/℃");
	m_thermalConductivityEdit->setSingleStep(1e1);
	formLayout->addRow("热传导系数:", m_thermalConductivityEdit);

	// 比热容
	m_heatCapacityEdit = new QDoubleSpinBox();
	m_heatCapacityEdit->setRange(0.0, 0.5);
	m_heatCapacityEdit->setValue(0.0);
	m_heatCapacityEdit->setDecimals(6);
	m_thermalConductivityEdit->setSuffix(" J/Kg/℃");
	m_heatCapacityEdit->setSingleStep(0.01);
	formLayout->addRow("比热容:", m_heatCapacityEdit);

	// JC A输入框
	m_AEdit = new QDoubleSpinBox();
	m_AEdit->setRange(0.0, 1e5);
	m_AEdit->setValue(0.0);
	m_AEdit->setDecimals(3);
	m_AEdit->setSuffix(" MPa");
	m_AEdit->setSingleStep(1);
	formLayout->addRow("JC A:", m_AEdit);

	// JC B输入框
	m_BEdit = new QDoubleSpinBox();
	m_BEdit->setRange(0.0, 1e5);
	m_BEdit->setValue(0.0);
	m_BEdit->setDecimals(3);
	m_BEdit->setSuffix(" MPa");
	m_BEdit->setSingleStep(100.0);
	formLayout->addRow("JC B:", m_BEdit);

	// JC n输入框
	m_nEdit = new QDoubleSpinBox();
	m_nEdit->setRange(0.0, 1e6);
	m_nEdit->setValue(0.0);
	m_nEdit->setDecimals(3);
	//m_nEdit->setSuffix(" kg/m³");
	m_nEdit->setSingleStep(0.1);
	formLayout->addRow("JC n:", m_nEdit);

	// JC c输入框
	m_cEdit = new QDoubleSpinBox();
	m_cEdit->setRange(0.0, 1);
	m_cEdit->setValue(0.0);
	m_cEdit->setDecimals(3);
	//m_cEdit->setSuffix(" kg/m³");
	m_cEdit->setSingleStep(0.01);
	formLayout->addRow("JC C:", m_cEdit);

	// JC m输入框
	m_mEdit = new QDoubleSpinBox();
	m_mEdit->setRange(0.0, 10);
	m_mEdit->setValue(0.0);
	m_mEdit->setDecimals(3);
	//m_mEdit->setSuffix(" kg/m³");
	m_mEdit->setSingleStep(0.1);
	formLayout->addRow("JC m:", m_mEdit);

	// JC Tm输入框
	m_TmEdit = new QDoubleSpinBox();
	m_TmEdit->setRange(0.0, 1e4);
	m_TmEdit->setValue(0.0);
	m_TmEdit->setDecimals(3);
	m_TmEdit->setSuffix(" ℃");
	m_TmEdit->setSingleStep(100.0);
	formLayout->addRow("JC Tm:", m_TmEdit);
	// JC T0输入框
	m_T0Edit = new QDoubleSpinBox();
	m_T0Edit->setRange(0.0, 1e4);
	m_T0Edit->setValue(0.0);
	m_T0Edit->setDecimals(3);
	m_T0Edit->setSuffix("  ℃");
	m_T0Edit->setSingleStep(10.0);
	formLayout->addRow("JC T0:", m_T0Edit);
	// JC epspdot输入框
	m_epspdotEdit = new QDoubleSpinBox();
	m_epspdotEdit->setRange(0.0, 1e6);
	m_epspdotEdit->setValue(0.0);
	m_epspdotEdit->setDecimals(3);
	m_epspdotEdit->setSuffix(" 1/s");
	m_epspdotEdit->setSingleStep(0.01);
	formLayout->addRow("JC epsdot:", m_epspdotEdit);
	// 添加按钮
	m_addButton = new QPushButton("添加材料");
	m_addButton->setStyleSheet("QPushButton { background-color: #0d6efd; color: white; padding: 8px; border-radius: 4px; }");
	formLayout->addRow(m_addButton);

	contentLayout->addWidget(inputGroup, 1);

	// 右侧：已输入材料列表
	QGroupBox* listGroup = new QGroupBox("已输入的材料参数");
	QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

	m_materialList = new QListWidget();
	m_materialList->setSelectionMode(QAbstractItemView::SingleSelection);
	listLayout->addWidget(m_materialList);

	// 删除按钮
	m_removeButton = new QPushButton("删除选中");
	m_removeButton->setStyleSheet("QPushButton { background-color: #dc3545; color: white; padding: 8px; border-radius: 4px; }");
	m_removeButton->setEnabled(false);
	listLayout->addWidget(m_removeButton);

	contentLayout->addWidget(listGroup, 1);

	mainLayout->addLayout(contentLayout, 1);

	// 底部：确定和取消按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_okButton = buttonBox->button(QDialogButtonBox::Ok);
	m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
	m_okButton->setText("确定");
	m_cancelButton->setText("取消");
	mainLayout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &MaterialDialog::onAccept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void MaterialDialog::setupConnections()
{
	connect(m_addButton, &QPushButton::clicked, this, &MaterialDialog::onAddMaterial);
	connect(m_removeButton, &QPushButton::clicked, this, &MaterialDialog::onRemoveMaterial);
	connect(m_materialList, &QListWidget::itemSelectionChanged, this, &MaterialDialog::onMaterialListSelectionChanged);
}

void MaterialDialog::onAddMaterial()
{
	QString name = m_nameEdit->text().trimmed();
	double youngsModulus = m_youngsModulusEdit->value();
	double poissonRatio = m_poissonRatioEdit->value();
	double density = m_densityEdit->value()/1e9;
	double thermalConductivity = m_thermalConductivityEdit->value();
	double heatCapacity = m_heatCapacityEdit->value();
	double JCA = m_AEdit->value();
	double JCB = m_BEdit->value();
	double JCn = m_nEdit->value();
	double JCc = m_cEdit->value();
	double JCm = m_mEdit->value();
	double JCTm = m_TmEdit->value();
	double JCT0 = m_T0Edit->value();
	double JCEPSDOT = m_epspdotEdit->value();


	// 验证输入
	if (name.isEmpty()) {
		Msg::ShowError("请输入材料名称！");
		return;
	}

	if (youngsModulus <= 0.0) {
		Msg::ShowError("杨氏模量必须大于0！");
		return;
	}

	if (poissonRatio < 0.0 || poissonRatio >= 0.5) {
		Msg::ShowError("泊松比必须在0到0.5之间！");
		return;
	}

	if (density <= 0.0) {
		Msg::ShowError("密度必须大于0！");
		return;
	}

	// 创建材料属性
	MaterialProperty material(name, youngsModulus, poissonRatio, density, thermalConductivity, heatCapacity, JCA, JCB, JCn, JCc, JCm, JCTm, JCT0, JCEPSDOT);

	// 添加到列表
	if (m_materials) {
		m_materials->append(material);
	}

	// 更新显示
	updateMaterialList();

	// 清空输入框
	m_nameEdit->clear();
	m_youngsModulusEdit->setValue(0.0);
	m_poissonRatioEdit->setValue(0.0);
	m_densityEdit->setValue(0.0);

	//Msg::ShowInfo(QString("材料 '%1' 已添加").arg(name));
}

void MaterialDialog::onRemoveMaterial()
{
	if (m_currentIndex < 0 || !m_materials || m_currentIndex >= m_materials->size()) {
		return;
	}

	QString name = m_materials->at(m_currentIndex).name;
	int ret = QMessageBox::question(this, "确认删除", 
		QString("确定要删除材料 '%1' 吗？").arg(name),
		QMessageBox::Yes | QMessageBox::No);

	if (ret == QMessageBox::Yes) {
		m_materials->removeAt(m_currentIndex);
		updateMaterialList();
		m_currentIndex = -1;
		m_removeButton->setEnabled(false);
		//Msg::ShowInfo(QString("材料 '%1' 已删除").arg(name));
	}
}

void MaterialDialog::onMaterialListSelectionChanged()
{
	QList<QListWidgetItem*> selectedItems = m_materialList->selectedItems();
	if (selectedItems.isEmpty()) {
		m_currentIndex = -1;
		m_removeButton->setEnabled(false);
		return;
	}

	m_currentIndex = m_materialList->row(selectedItems.first());
	m_removeButton->setEnabled(true);
}

void MaterialDialog::onAccept()
{
	if (!m_materials || m_materials->isEmpty()) {
		QMessageBox::warning(this, "警告", "请至少添加一种材料参数！");
		return;
	}
	accept();
}

MaterialProperty MaterialDialog::getMaterialProperty() const
{
	if (m_currentIndex >= 0 && m_materials && m_currentIndex < m_materials->size()) {
		return m_materials->at(m_currentIndex);
	}
	return MaterialProperty();
}

void MaterialDialog::updateMaterialList()
{
	if (!m_materialList || !m_materials) {
		return;
	}

	m_materialList->clear();

	for (const MaterialProperty& material : *m_materials) {
		QString itemText = QString("%1\n  杨氏模量: %2 Pa\n  泊松比: %3\n  密度: %4 kg/m³")
			.arg(material.name)
			.arg(material.youngsModulus, 0, 'e', 3)
			.arg(material.poissonRatio, 0, 'f', 6)
			.arg(material.density, 0, 'f', 3);

		QListWidgetItem* item = new QListWidgetItem(itemText, m_materialList);
		item->setData(Qt::UserRole, QVariant::fromValue(material));
	}
}

