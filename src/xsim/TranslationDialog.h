#pragma once

#ifndef TRANSLATIONDIALOG_H
#define TRANSLATIONDIALOG_H

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QtWidgets>
#include "ComplainUtf8.h"
#include <array>
// 变换对话框类
class TranslationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranslationDialog(QWidget* parent = nullptr, int i = 0, double x = 0, double y = 0, double z = 0, int* m_depthoffset=0, double offsetditance=0, int offsetnumber=0);

    void move();//< 移动模型

	void rotate();//< 旋转模型 

	void scale();  //< 缩放模型

	void extend();  //< 扩展面

	void EdgeDiscrete();//< 边离散

    void InputCuttingVelocity(double x, double y, double z); //< 输入切削速度

    void InputCuttingDepth(double x, double y, double z, int* m_depthoffset, double offsetditance, int offsetnumber);//< 输入切削厚度

    double getAngle()const;

    double getX() const;   //< 获取 X 坐标值

    double getY() const;  //< 获取 Y 坐标值

    double getZ() const;   //< 获取 Z 坐标值

	double getScale() const; //< 获取缩放比例

	double getDiscreteCount() const; //< 获取离散点数量
	
	bool isAbsolutePosition() const;  //< 获取是否为绝对位置移动

    void getDepthDirection(double*, int, int* depthoffset) const;

protected:
    
    QLineEdit* xLineEdit = nullptr;
    QLineEdit* yLineEdit = nullptr;
    QLineEdit* zLineEdit = nullptr;
    QLineEdit* angleLineEdit = nullptr; // 旋转角度输入框
    QLineEdit* scaleEdit = nullptr; // 缩放比例输入框
	QLineEdit* discreteCountEdit = nullptr; // 离散点数量输入框
    QButtonGroup* modeGroup = nullptr;
	QCheckBox* absolutePositionCheckBox = nullptr;  // 绝对位置移动复选框
    QCheckBox* x_positive = nullptr;
    QCheckBox* x_negative = nullptr;
    QCheckBox* y_positive = nullptr;
    QCheckBox* y_negative = nullptr;
    QCheckBox* z_positive = nullptr;
    QCheckBox* z_negative = nullptr;
    QComboBox* offsetDirectioncomboBox = nullptr;
    QLineEdit* offsetDistanceEdit = nullptr;
    QLineEdit* edgeNumEdit = nullptr;
public:
    // 保存上次输入的切削方向默认值
    static double s_lastCuttingDirX;
    static double s_lastCuttingDirY;
    static double s_lastCuttingDirZ;
    static double s_lastCuttingDepX;
    static double s_lastCuttingDepY;
    static double s_lastCuttingDepZ;
};

#endif // TRANSLATIONDIALOG_H
