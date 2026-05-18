#pragma once
#include "MyDocument.h"
#include <Standard_Transient.hxx>
#include <Standard_Handle.hxx>
#include <Standard_Type.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Manipulator.hxx>
#include <QObject>

class MyViewerSignals : public QObject
{
	Q_OBJECT

public:
	explicit MyViewerSignals(QObject* parent = nullptr) : QObject(parent) {}

signals:
	void selectionChangedSignal(); // 声明信号

};



//!OCCT视图类
class MyViewer:public Standard_Transient
{
	DEFINE_STANDARD_RTTIEXT(MyViewer, Standard_Transient)
public:
	MyViewer(const Handle(MyDocument) & hDoc);
	virtual ~MyViewer();

	const Handle(MyDocument) & getDocument();
	const Handle(AIS_InteractiveContext) & getAisContext() { return h_AisContext; }
	const Handle(AIS_InteractiveContext) & getTempAisContext() { return h_TempAisContext; }

	NCollection_List<Handle(AIS_InteractiveObject)> GetAisObj();//<获取所有图形对象
	NCollection_List<Handle(AIS_InteractiveObject)> GetSelAisObj();//<获取选中图形对象

	bool getAisObjColor(Quantity_Color& Color);//<获取颜色
	void AisObjColor(Quantity_Color Color);//<对选中的图形设置颜色
	void AisObjUnColor();//<取消颜色
	void AisObjTransparency(Standard_Real Transparency);//<设置透明度
	void AisObjUnTransparency();//<取消透明
	void AisObjDisplayAll();//<显示全部
	void AisObjEraseAll();//<隐藏
	void AisObjHide();//<隐藏选中
	void AisObjWireFrame();//<线框显示
	void AisObjShaded();//<平面显示
	void AisObjDeleteAll();//<全部删除
	void DeleteSelectedObjects();//<删除选中对象

	Quantity_Color getAisSelColor();
	void AisSelColor(Quantity_Color Color);	//<设置选中颜色
	
	void selectionChanged();
	void Display(const TopoDS_Shape& Shape); //<显示形状
	void Display(const TopoDS_Shape& Shape, Quantity_Color Color);//<显示形状并且着色
	void Display(const Handle(AIS_InteractiveObject)&shape);
	void Display(const Handle(AIS_InteractiveObject)& shape, Quantity_Color Color);
	void Erase(const TopoDS_Shape& Shape);	//<隐藏形状
	void Erase(const Handle(AIS_InteractiveObject)& shape);
	void Redraw() { h_Viewer->Redraw(); }

	MyViewerSignals* getSignals() { return m_Signals; }

protected:
	Handle(MyDocument)					h_Doc;
	Handle(V3d_Viewer)					h_Viewer;
	Handle(AIS_InteractiveContext)		h_AisContext;//主上下文
	Handle(AIS_InteractiveContext)		h_TempAisContext;//临时上下文

	MyViewerSignals* m_Signals; // 信号管理对象
	//操作器
	//======无===============
};
DEFINE_STANDARD_HANDLE(MyViewer, Standard_Transient)
