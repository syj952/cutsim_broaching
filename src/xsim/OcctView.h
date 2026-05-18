#pragma once

#include <Standard_WarningsDisable.hxx>
#include <QWidget>
#include <QAction>
#include <QList>
#include <QString>
#include <Standard_WarningsRestore.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include "PropertyView.h"
#include <AIS_Point.hxx>
#include <AIS_ColorScale.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Solid.hxx>
#include <QTreeWidget>
#include "OCCT_ShapeList.h"
#include "ProjectTree.h"
#include "BoundaryConditionDialog.h"
class TopoDS_Shape;
class QRubberBand;
struct BoundaryConditionData;
//!OCCT视图类
class OcctView : public QWidget
{
	Q_OBJECT
protected:
	enum CurrentAction3d {//交互模式
		CurAction3d_Nothing,//无交互
		CurAction3d_SingleOperation,//点击事件
		CurAction3d_DragEvent,//拖动事件
		CurAction3d_DynamicZooming,//动态缩放
		CurAction3d_WindowZooming,//窗口缩放
		CurAction3d_DynamicPanning,//动态平移
		CurAction3d_GlobalPanning,//全局平移
		CurAction3d_DynamicRotation//动态旋转
	};	
	enum ViewAction {//<视图操作
		ViewFitAllId, ViewFitAreaId, ViewZoomId, ViewPanId, ViewGlobalPanId, ViewRotationId,
		ViewFrontId, ViewBackId, ViewTopId, ViewBottomId, ViewLeftId, ViewRightId,
		ViewAxoId, ViewResetId, ViewHlrOffId, ViewHlrOnId
	};
	enum RaytraceAction { ToolRaytracingId, ToolShadowsId, ToolReflectionsId, ToolAntialiasingId };//<光线追踪相关
public:	enum SelectionMode { None, FaceSelection, EdgeSelection };//<选择模式
public:
	OcctView(Handle(AIS_InteractiveContext) theContext, QWidget* parent);

	~OcctView();

	Handle(AIS_InteractiveContext)&     getXYZContext() { return myXYZContext; }
	const Handle(V3d_View)&       getViewc() { return myView; }
	virtual void init();
	bool dump(Standard_CString theFile);//<导出图像
	QList<QAction*>*       getViewActions();//<视图操作列表
	QList<QAction*>*       getRaytraceActions();//<光线追踪相关列表
	void noActiveActions();//<恢复默认状态
	void EnableRaytracing();//<开启光线追踪
	void DisableRaytracing();//<关闭光线追踪
	void SetRaytracedShadows(bool theState);//<阴影
	void SetRaytracedReflections(bool theState);//<反射
	void SetRaytracedAntialiasing(bool theState);//<抗锯齿
	bool IsRaytracingMode() const { return myIsRaytracing; }//<是否开启光线追踪
	bool IsShadowsEnabled() const { return myIsShadowsEnabled; }//<是否开启阴影
	bool IsReflectionsEnabled() const { return myIsReflectionsEnabled; }//<是否开启反射
	bool IsAntialiasingEnabled() const { return myIsAntialiasingEnabled; }//<是否开启抗锯齿	
	void setPropertyView(PropertyView* view) { m_propertyView = view; }	//<设置属性窗口
	void setSelectionMode(SelectionMode mode);	//<设置选择模式
	/*!
	Get paint engine for the OpenGL viewer. [ virtual public ]
	*/
	virtual QPaintEngine* paintEngine() const { return 0; }//<重载paintEngine函数

signals:
	void LUP();//<鼠标左键抬起
	void selectionChanged();//<选择改变
	void SingleSelect(const Handle_AIS_InteractiveContext & myContext, const Handle_AIS_InteractiveObject & hAisObj);
	void SingleOperation_Begin(const Handle_V3d_View & myView, const Handle_AIS_InteractiveContext & myContext, QPoint Point);
	void SingleOperation_Move(const Handle_V3d_View & myView, const Handle_AIS_InteractiveContext & myContext, QPoint Point);
	void SingleOperation_End(const Handle_V3d_View & myView, const Handle_AIS_InteractiveContext & myContext, QPoint Point);
	void shapeSelected(QString& shapeType);//<选择形状
	void faceSelected(const TopoDS_Face&f);
	void edgeSelected(const TopoDS_Edge&e);
	void solidSelected(const TopoDS_Solid& s);
	void boundaryConditionAdded(const BoundaryConditionData& data);
public slots:
	void fitAll();//<适合所有
	void fitArea();
	void zoom();//<缩放
	void pan();//<平移
	void globalPan();//<全局平移
	void front();//<前视图
	void back();
	void top();
	void bottom();
	void left();
	void right();
	void axo();
	void rotation();
	void reset();
	void hlrOn();//<开启隐藏线
	void hlrOff();//<关闭隐藏线

	void updateToggled(bool);//<更新
	void onBackground();//<背景
	void onScreenShot();//<截屏
	void onRaytraceAction();//<光线追踪

	void OnTreeItemDoubleClicked(QTreeWidgetItem* item, int column);//<树形项双击
protected:			               
	virtual void paintEvent(QPaintEvent*);//<绘制事件
	virtual void resizeEvent(QResizeEvent*);//<大小改变事件
	virtual void mousePressEvent(QMouseEvent*);//<鼠标按下事件
	virtual void mouseReleaseEvent(QMouseEvent*);//<鼠标释放事件
	virtual void mouseMoveEvent(QMouseEvent*);//<鼠标移动事件
	virtual void wheelEvent(QWheelEvent *event);  // 滚轮事件
public:
	int   getSelectNb();//<获取选择的数量
	OCCT_ShapeList getSelectedShapes();//<获取选择的shape列表
	Handle(V3d_View)& getView();//<获取视图
	Handle(AIS_InteractiveContext)& getContext();//<获取交互式上下文
	void  	activateCursor(const CurrentAction3d);//<激活光标
	void  	Popup(const int x, const int y);//<弹出右键菜单
	CurrentAction3d  	getCurrentMode();//<获取当前模式	

	Handle(AIS_Point) AisPnt;//<鼠标左键交互点

	virtual void onLButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标左键点击事件
	virtual void onMButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标中键点击事件
	virtual void onRButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标右键点击事件
	virtual void onLButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标左键点击事件
	virtual void onMButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标中键点击事件
	virtual void onRButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标右键点击事件
	virtual void onMouseMove(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标移动事件

private:
	void initCursors();//<初始化鼠标光标
	void initViewActions();//<初始化视图操作
	void initRaytraceActions();//<初始化光线追踪操作
	void DragEvent(const int x, const int y, const int TheState);//<拖动事件
	void InputEvent(const int x, const int y);//<输入事件
	void MoveEvent(const int x, const int y);//<移动事件
	void MultiMoveEvent(const int x, const int y);//<多选移动事件
	void MultiDragEvent(const int x, const int y, const int TheState);//<多选拖动事件
	void MultiInputEvent(const int x, const int y);//<多选输入事件
	void DrawRectangle(const int MinX, const int MinY, const int MaxX, const int MaxY, const bool Draw);//<绘制矩形
//private:
public:
	bool  myIsRaytracing;//<是否开启光线追踪
	bool  myIsShadowsEnabled;//<是否开启阴影
	bool  myIsReflectionsEnabled;//<是否开启反射
	bool  myIsAntialiasingEnabled;//<是否开启抗锯齿
	bool  myDrawRect;      // 当使用鼠标拖动选择时，是否绘制矩形
	Handle(V3d_View)					myView;//<控制3d视图的显示和控制功能
	Handle(AIS_InteractiveContext)		myContext;//<管理交互对象
	Handle(AIS_InteractiveContext)		myXYZContext;//<管理XYZ坐标系
	CurrentAction3d						myCurrentMode;//<当前模式
	Standard_Integer        myXmin;//<鼠标点击的坐标X
	Standard_Integer        myYmin;//<鼠标点击的坐标Y
	Standard_Integer        myXmax;//<鼠标点击的坐标X
	Standard_Integer        myYmax;//<鼠标点击的坐标Y
	Standard_Real			myCurZoom;//<当前缩放比例
	Standard_Boolean        myHlrModeIsOn;//<是否开启隐藏线	
	QList<QAction*>*		myViewActions;//<视图操作列表
	QList<QAction*>*		myRaytraceActions;//<光线追踪相关操作列表
	QMenu* myBackMenu;//<右键目录
	QRubberBand*  myRectBand; //!< 矩形选择框
	PropertyView* m_propertyView;//<属性栏
	BoundaryConditionDialog* dlg; //< 边界条件对话框
	SelectionMode m_selectionMode = None;
	
public:
	// 边界条件功能
	void showBoundaryCondition(const TopoDS_Shape& shape);
	void clearBoundaryConditions();

public:
	// 边界条件相关成员
	QList<BoundaryConditionData> m_boundaryConditions; //< 存储所有边界条件
	void AddBoundaryCondition(const BoundaryConditionData& bcData);//< 添加边界条件
	void SetBoundaryConditions(const QList<BoundaryConditionData>& bcList);//< 设置所有边界条件
	QList<BoundaryConditionData> getBoundaryConditions();//< 获取所有边界条件

	// 可视化边界条件的交互对象映射(全部未实现）
	QMap<int, Handle(AIS_InteractiveObject)> m_bcVisualizations;
	void visualizeFaceBoundaryCondition(const TopoDS_Shape& shape);
	void visualizeEdgeBoundaryCondition(const TopoDS_Shape& shape);
	void visualizeVertexBoundaryCondition(const TopoDS_Shape& shape);

	// 颜色条相关功能
	void setColorBarVisible(bool visible, const QString& title = "", double minVal = 0.0, double maxVal = 1.0);
	bool isColorBarVisible() const { return m_colorBarVisible; }
	void setDisplacementRange(double minVal, double maxVal);
	void setColorScaleTitle(const QString& title);
	void updateColorScale();

private:
	bool m_colorBarVisible = false;  // 颜色条是否显示，默认为隐藏
	double m_displacementMin = 0.0;  // displacement 最小值
	double m_displacementMax = 1.0;  // displacement 最大值
	Handle(AIS_ColorScale) m_colorScale;  // OpenCASCADE 颜色条对象

};


