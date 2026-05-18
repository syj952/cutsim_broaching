#pragma once

#ifndef MATERIALDIALOG_H
#define MATERIALDIALOG_H

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QtWidgets>
#include <QMetaType>
#include "ComplainUtf8.h"

// 材料参数结构体
struct MaterialProperty {
	QString name;              // 材料名称
	double youngsModulus;      // 杨氏模量 (Pa)
	double poissonRatio;       // 泊松比
	double density;            // 密度 (kg/m³)
	double thermalConductivity;
	double heatCapacity;
	double JCA;
	double JCB;
	double JCn;
	double JCc;
	double JCm;
	double JCTm;
	double JCT0;
	double JCEPSDOT;

	MaterialProperty() : name(""), youngsModulus(0.0), poissonRatio(0.0), density(0.0) {}
	MaterialProperty(const QString& ns, double E, double nu, double rho, double K, double Cp, double A, double B, double n, double c, double m, double Tm, double T0, double epsdot)
		: name(ns), youngsModulus(E), poissonRatio(nu), density(rho), thermalConductivity(K), heatCapacity(Cp), JCA(A), JCB(B), JCn(n), JCc(c), JCm(m), JCTm(Tm), JCT0(T0), JCEPSDOT(epsdot) {}
};

// 注册 MaterialProperty 到 Qt 元对象系统
Q_DECLARE_METATYPE(MaterialProperty)

// 材料参数输入对话框
class MaterialDialog : public QDialog
{
	Q_OBJECT

public:
	explicit MaterialDialog(QWidget* parent = nullptr, QList<MaterialProperty>* materials = nullptr);
	~MaterialDialog();

	MaterialProperty getMaterialProperty() const;
	void updateMaterialList();

private slots:
	void onAddMaterial();
	void onRemoveMaterial();
	void onMaterialListSelectionChanged();
	void onAccept();

private:
	void setupUI();
	void setupConnections();

	// UI控件
	QLineEdit* m_nameEdit;              // 材料名称输入框
	QDoubleSpinBox* m_youngsModulusEdit; // 杨氏模量输入框
	QDoubleSpinBox* m_poissonRatioEdit;  // 泊松比输入框
	QDoubleSpinBox* m_densityEdit;       // 密度输入框
	QDoubleSpinBox* m_thermalConductivityEdit; // 热传导系数输入框
	QDoubleSpinBox* m_heatCapacityEdit;  // 比热容输入框
	QDoubleSpinBox* m_AEdit;       // JC A输入框
	QDoubleSpinBox* m_BEdit; // JC B输入框
	QDoubleSpinBox* m_nEdit;  // JC n输入框
	QDoubleSpinBox* m_cEdit;       // JC c输入框
	QDoubleSpinBox* m_mEdit;  // JC m输入框
	QDoubleSpinBox* m_TmEdit;       // JC Tm输入框
	QDoubleSpinBox* m_T0Edit;       // JC T0输入框
	QDoubleSpinBox* m_epspdotEdit;       // JC epspdot输入框

	QListWidget* m_materialList;         // 已输入的材料列表
	QPushButton* m_addButton;            // 添加按钮
	QPushButton* m_removeButton;         // 删除按钮
	QPushButton* m_okButton;             // 确定按钮
	QPushButton* m_cancelButton;         // 取消按钮

	QList<MaterialProperty>* m_materials; // 指向外部材料列表的指针
	int m_currentIndex;                   // 当前选中的材料索引
};

#endif // MATERIALDIALOG_H

