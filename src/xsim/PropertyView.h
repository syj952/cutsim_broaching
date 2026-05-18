#pragma once


#ifndef PROPERTYVIEW_H
#define PROPERTYVIEW_H
#include <QTableWidget>
#include <QWidget>
#include <TopoDS_Shape.hxx>
#include <AIS_Point.hxx>
//属性窗口
class PropertyView : public QWidget
{
	Q_OBJECT

public:
	PropertyView(QWidget* parent = Q_NULLPTR);
	~PropertyView();

	void setShapeView(const TopoDS_Shape& shape);//<设置形状视图
	void setAisPoint(Handle(AIS_Point) point);//<设置交互式点
	void setDir(gp_Dir dir);

private:
	void initWidget();

	void setupTableStyle();                          // 设置表格样式
	void addPropertyRow(const QString& name, const QString& value, int row); // 添加属性行

	QTableWidget* table;
	QTableWidgetItem* item1;//表格第二行

};


#endif // DEBUG
