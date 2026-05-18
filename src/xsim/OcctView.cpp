#include "Icon.h"
#include "ComplainUtf8.h"
#include "OcctView.h"
#include "OcctWindow.h"
#include <QApplication>
#include <QPainter>
#include <QMenu>
#include <QColorDialog>
#include <QCursor>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMouseEvent>
#include <QRubberBand>
#include <QMdiSubWindow>
#include <QStyleFactory>
#include <QtGui/QRegExpValidator>
#include <QActionGroup>
#include <Standard_WarningsDisable.hxx>
#include <Standard_WarningsRestore.hxx>
#include <AIS_ViewCube.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Graphic3d_TextureEnv.hxx>
#include <AIS_Point.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Geom_Axis2Placement.hxx>
#include <AIS_Trihedron.hxx>
#include <AIS_ColorScale.hxx>
#include <Graphic3d_TransformPers.hxx>
#include <Aspect_TypeOfColorScalePosition.hxx>

// the key for multi selection :  多选键盘
#define MULTISELECTIONKEY Qt::ShiftModifier

// the key for shortcut ( use to activate dynamic rotation, panning ) 快捷键（用于激活动态旋转、平移）
#define CASCADESHORTCUTKEY Qt::ControlModifier

// for elastic bean selection
#define ValZWMin 1

static QCursor* defCursor = NULL;
static QCursor* handCursor = NULL;
static QCursor* panCursor = NULL;
static QCursor* globPanCursor = NULL;
static QCursor* zoomCursor = NULL;
static QCursor* rotCursor = NULL;

OcctView::OcctView(Handle(AIS_InteractiveContext) theContext,QWidget* parent)
	: QWidget(parent),
	myIsRaytracing(false),
	myIsShadowsEnabled(true),
	myIsReflectionsEnabled(false),
	myIsAntialiasingEnabled(false),
	myViewActions(0),
	myRaytraceActions(0),
	myBackMenu(NULL)
{
	myContext = theContext;
	{
		Handle(AIS_InteractiveContext) hAISContext = new AIS_InteractiveContext(theContext->CurrentViewer()); //创建交互式环境
		hAISContext->SetDisplayMode(AIS_Shaded, true);   //设置显示模式为遮蔽
		{
			{	//CUBE
				Handle(AIS_ViewCube) H_AisViewCube = new AIS_ViewCube();
				hAISContext->SetDisplayMode(AIS_Shaded, true);   //设置显示模式为遮蔽
				H_AisViewCube->SetBoxSideLabel(V3d_Xpos, "右");//!< (+Y+Z) view
				H_AisViewCube->SetBoxSideLabel(V3d_Ypos, "后");//!< (+X+Z) view
				H_AisViewCube->SetBoxSideLabel(V3d_Zpos, "俯");//!< (+Y+Z) view
				H_AisViewCube->SetBoxSideLabel(V3d_Xneg, "左");//!< (+Y+Z) view
				H_AisViewCube->SetBoxSideLabel(V3d_Yneg, "前");//!< (+Y+Z) view
				H_AisViewCube->SetBoxSideLabel(V3d_Zneg, "仰");//!< (+Y+Z) view
				H_AisViewCube->SetTransparency(0.7);//设置视方体透明度
				H_AisViewCube->SetBoxColor(Quantity_NOC_WHEAT); //设置视方体为金色
				H_AisViewCube->SetFont("Arial");  // 设置字体
	
				H_AisViewCube->SetTextColor(Quantity_NOC_BLACK);  // 黑色文字，提高可读性
				
				//设置视方体每个面的边界线
				//首先创建一个Prs3d_Drawer 对象，一切属性的修改在myDraw 下进行
				const Handle(Prs3d_Drawer)& myDrawer = H_AisViewCube->Attributes();
				myDrawer->SetupOwnFaceBoundaryAspect(); 
				myDrawer->SetFaceBoundaryDraw(true);
				//设置边界线的颜色为蓝色
				myDrawer->FaceBoundaryAspect()->SetColor(Quantity_NOC_SLATEGRAY);  // 石板灰，更柔和
				myDrawer->FaceBoundaryAspect()->SetWidth(1.5);  // 设置边界线宽度
				myDrawer->FaceBoundaryAspect()->SetTypeOfLine(Aspect_TOL_SOLID);  // 实线

				//设置视方体基准线
				myDrawer->SetDatumAspect(new Prs3d_DatumAspect()); //创建基准线属性对象
				const Handle_Prs3d_DatumAspect& datumAspect = H_AisViewCube->Attributes()->DatumAspect();  //设置基准线属性
				//设置轴颜色
				datumAspect->ShadingAspect(Prs3d_DP_XAxis)->SetColor(Quantity_NOC_RED2);
				datumAspect->ShadingAspect(Prs3d_DP_YAxis)->SetColor(Quantity_NOC_GREEN2);
				datumAspect->ShadingAspect(Prs3d_DP_ZAxis)->SetColor(Quantity_NOC_BLUE2);

				//设置X,Y,Z文本颜色
				Handle(Prs3d_TextAspect) textAspect = new Prs3d_TextAspect();
				textAspect->SetFont("Arial");
				textAspect->SetHeight(20);
				// X轴文本
				datumAspect->TextAspect(Prs3d_DP_XAxis)->SetColor(Quantity_NOC_RED3);
				datumAspect->TextAspect(Prs3d_DP_XAxis)->SetFont("Arial");
				datumAspect->TextAspect(Prs3d_DP_XAxis)->SetHeight(18);

				// Y轴文本
				datumAspect->TextAspect(Prs3d_DP_YAxis)->SetColor(Quantity_NOC_GREEN3);
				datumAspect->TextAspect(Prs3d_DP_YAxis)->SetFont("Arial");
				datumAspect->TextAspect(Prs3d_DP_YAxis)->SetHeight(18);

				// Z轴文本
				datumAspect->TextAspect(Prs3d_DP_ZAxis)->SetColor(Quantity_NOC_BLUE3);
				datumAspect->TextAspect(Prs3d_DP_ZAxis)->SetFont("Arial");
				datumAspect->TextAspect(Prs3d_DP_ZAxis)->SetHeight(18);

				// 添加一些额外的美化设置
				H_AisViewCube->SetDrawEdges(true);  // 绘制边线
				H_AisViewCube->SetDrawAxes(true);   // 绘制坐标轴

				// 设置圆角效果，让立方体看起来更现代
				H_AisViewCube->SetBoxFacetExtension(0.1);   // 面延伸
				H_AisViewCube->SetBoxEdgeGap(0.1);          // 边间隙

				H_AisViewCube->SetTransformPersistence(	  
					new Graphic3d_TransformPers(
						Graphic3d_TMF_TriedronPers,
						Aspect_TOTP_RIGHT_UPPER,
						Graphic3d_Vec2i(80, 80)));  // 设置位置和大小 80*80像素

				H_AisViewCube->SetSize(50);  // 调整立方体大小
				H_AisViewCube->SetFontHeight(30);  // 设置字体大小
				H_AisViewCube->SetAxesRadius(3);  // 坐标轴半径
				H_AisViewCube->SetAxesConeRadius(5);  // 坐标轴锥体半径


				hAISContext->Display(H_AisViewCube, Standard_True);
			}

			{	//XYZ坐标系
				Handle(AIS_Point) Point = new AIS_Point(new Geom_CartesianPoint(0, 0, 0));
				hAISContext->Display(Point, Standard_False);
				Handle(AIS_Trihedron) Trihedron = new AIS_Trihedron(
					new Geom_Axis2Placement(gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1))));
				Trihedron->SetSize(30);
				Trihedron->SetColor(Quantity_NOC_BLUE1);
				Trihedron->SetTextColor(Quantity_NOC_BLACK);
				hAISContext->Display(Trihedron, Standard_False);
				hAISContext->SetZLayer(Trihedron, Graphic3d_ZLayerId_Topmost); //设置在最上层
				opencascade::handle<Graphic3d_TransformPers> transform =
					new Graphic3d_TransformPers(Graphic3d_TMF_ZoomPers); //
				hAISContext->SetTransformPersistence(Trihedron, transform);//禁止缩放
			}
		}
		myXYZContext = hAISContext;
	}

	myXmin = 0;
	myYmin = 0;
	myXmax = 0;
	myYmax = 0;
	myCurZoom = 0;
	myRectBand = 0;

	setAttribute(Qt::WA_PaintOnScreen);	//直接在屏幕上绘制，而不是使用双缓冲区
	setAttribute(Qt::WA_NoSystemBackground); //不自动填充背景

	myCurrentMode = CurAction3d_Nothing;
	myHlrModeIsOn = Standard_False;
	setMouseTracking(true);

	initViewActions();//初始化右边视图操作工具框
	initCursors();	//初始化光标

	setBackgroundRole(QPalette::NoRole);//NoBackground );
	// set focus policy to threat QContextMenuEvent from keyboard  
	setFocusPolicy(Qt::StrongFocus);//强制获取焦点
	//setWindowFlag(Qt::MSWindowsOwnDC);

	init();
}

OcctView::~OcctView()
{
	delete myBackMenu;
}

void OcctView::init()
{
	if (myView.IsNull())
		myView = myContext->CurrentViewer()->CreateView();
	myView->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_BLACK, 0.05, V3d_ZBUFFER);//左下角、小麦色、5%大小、深度缓冲渲染

	Handle(OcctWindow) hWnd = new OcctWindow(this);
	myView->SetWindow(hWnd);  //将Qt窗口与OCCT视图关联
	if (!hWnd->IsMapped())	//检查窗口是否已映射
		hWnd->Map();  //映射窗口
	
	myView->MustBeResized();//调整视图大小以适应窗口
	if (myIsRaytracing)
		myView->ChangeRenderingParams().Method = Graphic3d_RM_RAYTRACING;  //设置渲染方法为光线追踪

	//Quantity_Color topColor(0.890, 0.949, 0.992, Quantity_TOC_RGB);    // #E3F2FD - 很浅的蓝色
	//Quantity_Color middleColor(0.564, 0.792, 0.976, Quantity_TOC_RGB); // #90CAF9 - 中等蓝色
	//Quantity_Color bottomColor(0.098, 0.463, 0.824, Quantity_TOC_RGB); // #1976D2 - 深蓝色
	Quantity_Color topColor(0.815, 0.843, 1, Quantity_TOC_RGB);    // #d0d7ff 浅蓝色
	Quantity_Color middleColor(0.564, 0.792, 0.976, Quantity_TOC_RGB); // #90CAF9 - 中等蓝色
	Quantity_Color bottomColor(1,1,1, Quantity_TOC_RGB); // #ffffff 纯白色

	// 设置垂直渐变
	myView->SetBgGradientStyle(Aspect_GFM_VER);
	myView->SetBgGradientColors(topColor, bottomColor, Aspect_GFM_VER);

	//myView->SetBackgroundColor(Quantity_NOC_WHITE);	//设置背景颜色为白色
	myView->SetLightOn();//开启光照


	Handle(Prs3d_Drawer) defaultDrawer = myContext->DefaultDrawer();
	defaultDrawer->SetFaceBoundaryDraw(Standard_True);
	//defaultDrawer->SetEdgeAspect(edgeAspect);  // 全局边界线属性
	myContext->SetDefaultDrawer(defaultDrawer);	
	//myContext->UpdateAll();  // 更新所有显示的形状

}

void OcctView::paintEvent(QPaintEvent *)
{
	//  QApplication::syncX();
	myView->Redraw();//重绘视图
}

void OcctView::resizeEvent(QResizeEvent *)
{
	//  QApplication::syncX();
	if (!myView.IsNull())
	{
		myView->MustBeResized();
	}
}

void OcctView::fitAll()
{
	myView->FitAll();
	myView->ZFitAll();
	myView->Redraw();
}

void OcctView::fitArea()
{
	myCurrentMode = CurAction3d_WindowZooming;
}

void OcctView::zoom()
{
	myCurrentMode = CurAction3d_DynamicZooming;
}

void OcctView::pan()
{
	myCurrentMode = CurAction3d_DynamicPanning;
}

void OcctView::rotation()
{
	myCurrentMode = CurAction3d_DynamicRotation;
}

void OcctView::globalPan()
{
	// save the current zoom value
	myCurZoom = myView->Scale();
	// Do a Global Zoom
	myView->FitAll();
	// Set the mode
	myCurrentMode = CurAction3d_GlobalPanning;
}

void OcctView::front()
{
	myView->SetProj(V3d_Yneg);
}

void OcctView::back()
{
	myView->SetProj(V3d_Ypos);
}

void OcctView::top()
{
	myView->SetProj(V3d_Zpos);
}

void OcctView::bottom()
{
	myView->SetProj(V3d_Zneg);
}

void OcctView::left()
{
	myView->SetProj(V3d_Xneg);
}

void OcctView::right()
{
	myView->SetProj(V3d_Xpos);
}

void OcctView::axo()
{
	myView->SetProj(V3d_XposYnegZpos);
}

void OcctView::reset()
{
	myView->Reset();
}

void OcctView::hlrOff()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	myHlrModeIsOn = Standard_False;
	myView->SetComputedMode(myHlrModeIsOn);
	myView->Redraw();
	QApplication::restoreOverrideCursor();
}

void OcctView::hlrOn()
{
	QApplication::setOverrideCursor(Qt::WaitCursor); 
	myHlrModeIsOn = Standard_True;
	myView->SetComputedMode(myHlrModeIsOn);
	myView->Redraw();
	QApplication::restoreOverrideCursor();
}

void OcctView::SetRaytracedShadows(bool theState)
{
	myView->ChangeRenderingParams().IsShadowEnabled = theState;

	myIsShadowsEnabled = theState;

	myContext->UpdateCurrentViewer();
}

void OcctView::SetRaytracedReflections(bool theState)
{
	myView->ChangeRenderingParams().IsReflectionEnabled = theState;

	myIsReflectionsEnabled = theState;

	myContext->UpdateCurrentViewer();
}

void OcctView::onRaytraceAction()
{
	QAction* aSentBy = (QAction*)sender();

	if (aSentBy == myRaytraceActions->at(ToolRaytracingId))
	{
		bool aState = myRaytraceActions->at(ToolRaytracingId)->isChecked();

		QApplication::setOverrideCursor(Qt::WaitCursor);
		if (aState)
			EnableRaytracing();
		else
			DisableRaytracing();
		QApplication::restoreOverrideCursor();
	}

	if (aSentBy == myRaytraceActions->at(ToolShadowsId))
	{
		bool aState = myRaytraceActions->at(ToolShadowsId)->isChecked();
		SetRaytracedShadows(aState);
	}

	if (aSentBy == myRaytraceActions->at(ToolReflectionsId))
	{
		bool aState = myRaytraceActions->at(ToolReflectionsId)->isChecked();
		SetRaytracedReflections(aState);
	}

	if (aSentBy == myRaytraceActions->at(ToolAntialiasingId))
	{
		bool aState = myRaytraceActions->at(ToolAntialiasingId)->isChecked();
		SetRaytracedAntialiasing(aState);
	}
}
#include "ProjectTree.h"
#include "Msg.h"
void OcctView::OnTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {

	myContext->ClearSelected(Standard_False); // 不清屏，只清除选择
	Handle(AIS_InteractiveObject) selectedObject = ProjectTree::m_itemToAisObjectMap.value(item);

	if (!selectedObject.IsNull() && !myContext.IsNull()) {

		// 可以设置自定义的高亮颜色等。OCCT有默认的高亮样式，但也可以自定义。
		// 例如，设置选择高亮为红色：
		//myContext->HighlightStyle(Prs3d_TypeOfHighlight_Selected)->SetColor(Quantity_NOC_RED);
		//selectedObject->SetColor(Quantity_NOC_RED); // 设置对象颜色为红色
		//myContext->Display(selectedObject, Standard_True); // 显示对象
		//selectedObject->SetHilightMode(1); // 设置高亮模式
		//selectedObject->Redisplay(); // 重新显示对象以应用颜色更改
		myContext->SetSelected(selectedObject,true); // 选中对象
		myContext->HilightSelected(true); // 高亮选中对象
		//myContext->Redisplay(selectedObject, Standard_True); // 重新显示对象以

		myContext->UpdateCurrentViewer();
	}
	else {
		// 处理未找到对应交互对象的情况，例如打印警告信息。
		//qDebug() << "Warning: No corresponding AIS object found for the selected tree item.";
		//Msg::ShowInfo("未选中");
	}
}

void OcctView::SetRaytracedAntialiasing(bool theState)	 
{
	myView->ChangeRenderingParams().IsAntialiasingEnabled = theState;

	myIsAntialiasingEnabled = theState;

	myContext->UpdateCurrentViewer();
}

void OcctView::setSelectionMode(SelectionMode mode)
{
	m_selectionMode = mode;
}

void OcctView::EnableRaytracing()
{
	if (!myIsRaytracing)
		myView->ChangeRenderingParams().Method = Graphic3d_RM_RAYTRACING;

	myIsRaytracing = true;

	myContext->UpdateCurrentViewer();
}

void OcctView::DisableRaytracing()
{
	if (myIsRaytracing)
		myView->ChangeRenderingParams().Method = Graphic3d_RM_RASTERIZATION;

	myIsRaytracing = false;

	myContext->UpdateCurrentViewer();
}

void OcctView::updateToggled(bool isOn)
{
	QAction* sentBy = (QAction*)sender();

	if (!isOn)
		return;
	for (int i = ViewFitAllId; i < ViewHlrOffId; i++)
	{
		QAction* anAction = myViewActions->at(i);
		if ((anAction == myViewActions->at(ViewFitAreaId)) ||
			(anAction == myViewActions->at(ViewZoomId)) ||
			(anAction == myViewActions->at(ViewPanId)) ||
			(anAction == myViewActions->at(ViewGlobalPanId)) ||
			(anAction == myViewActions->at(ViewRotationId)))
		{
			if (anAction && (anAction != sentBy))
			{
				anAction->setCheckable(true);
				anAction->setChecked(false);
			}
			else
			{
				sentBy->setCheckable(false);
			}
		}
	}
}

void OcctView::initCursors()
{
	if (!defCursor)
		defCursor = new QCursor(Qt::ArrowCursor);//箭头光标
	if (!handCursor)
		handCursor = new QCursor(Qt::PointingHandCursor);//手型光标
	if (!panCursor)
		panCursor = new QCursor(Qt::SizeAllCursor);//平移光标
	if (!globPanCursor)
		globPanCursor = new QCursor(Qt::CrossCursor);//全局平移光标
	if ( !zoomCursor )
	  zoomCursor = new QCursor(QPixmap(ICON_CURSOR_ZOOM));//缩放光标
	if ( !rotCursor )
	  rotCursor = new QCursor(QPixmap(ICON_CURSOR_ROTATE));//旋转光标
}

QList<QAction*>* OcctView::getViewActions()
{
	initViewActions();
	return myViewActions;
}

QList<QAction*>* OcctView::getRaytraceActions()
{
	initRaytraceActions();
	return myRaytraceActions;
}

void OcctView::initViewActions()
{
	if (myViewActions)
		return;

	myViewActions = new QList<QAction*>();

	QAction* a = nullptr;
	a = new QAction(QIcon(ICON_VIEW_FITALL), QObject::tr("调整视图—自动"), this);
	a->setToolTip(QObject::tr("调整视图—自动"));
	a->setStatusTip(QObject::tr("自动调整视图至合适的大小"));
	connect(a, SIGNAL(triggered()), this, SLOT(fitAll()));
	myViewActions->insert(ViewFitAllId, a);

	a = new QAction(QIcon(ICON_VIEW_FITAREA), QObject::tr("调整视图—框选"), this);
	a->setToolTip(QObject::tr("调整视图—框选"));
	a->setStatusTip(QObject::tr("调整视图至框选的区域"));
	connect(a, SIGNAL(triggered()), this, SLOT(fitArea()));
	a->setCheckable(true);
	connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
	myViewActions->insert(ViewFitAreaId, a);

	a = new QAction(QIcon(ICON_VIEW_ZOOM), QObject::tr("缩放视图"), this);
	a->setToolTip(QObject::tr("缩放视图"));
	a->setStatusTip(QObject::tr("拖拽视图控制缩放视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(zoom()));
	a->setCheckable(true);
	connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
	myViewActions->insert(ViewZoomId, a);

	a = new QAction(QIcon(ICON_VIEW_PAN), QObject::tr("移动视图"), this);
	a->setToolTip(QObject::tr("移动视图"));
	a->setStatusTip(QObject::tr("移动视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(pan()));
	a->setCheckable(true);
	connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
	myViewActions->insert(ViewPanId, a);

	a = new QAction(QIcon(ICON_VIEW_GLOBALPAN), QObject::tr("移动视图—位置"), this);
	a->setToolTip(QObject::tr("移动视图—位置"));
	a->setStatusTip(QObject::tr("移动视图至鼠标指定的位置"));
	connect(a, SIGNAL(triggered()), this, SLOT(globalPan()));
	a->setCheckable(true);
	connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
	myViewActions->insert(ViewGlobalPanId, a);

	a = new QAction(QIcon(ICON_VIEW_ROTATE), QObject::tr("旋转视图"), this);
	a->setToolTip(QObject::tr("旋转视图"));
	a->setStatusTip(QObject::tr("旋转视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(rotation()));
	a->setCheckable(true);
	connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
	myViewActions->insert(ViewRotationId, a);

	a = new QAction(QIcon(ICON_VIEW_FRONT), QObject::tr("正视图"), this);
	a->setToolTip(QObject::tr("正视图"));
	a->setStatusTip(QObject::tr("正视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(front()));
	myViewActions->insert(ViewFrontId, a);

	a = new QAction(QIcon(ICON_VIEW_BACK), QObject::tr("背视图"), this);
	a->setToolTip(QObject::tr("背视图"));
	a->setStatusTip(QObject::tr("背视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(back()));
	myViewActions->insert(ViewBackId, a);

	a = new QAction(QIcon(ICON_VIEW_TOP), QObject::tr("俯视图"), this);
	a->setToolTip(QObject::tr("俯视图"));
	a->setStatusTip(QObject::tr("俯视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(top()));
	myViewActions->insert(ViewTopId, a);

	a = new QAction(QIcon(ICON_VIEW_BOTTOM), QObject::tr("仰视图"), this);
	a->setToolTip(QObject::tr("仰视图"));
	a->setStatusTip(QObject::tr("仰视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(bottom()));
	myViewActions->insert(ViewBottomId, a);

	a = new QAction(QIcon(ICON_VIEW_LEFT), QObject::tr("左视图"), this);
	a->setToolTip(QObject::tr("左视图"));
	a->setStatusTip(QObject::tr("左视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(left()));
	myViewActions->insert(ViewLeftId, a);

	a = new QAction(QIcon(ICON_VIEW_RIGHT), QObject::tr("右视图"), this);
	a->setToolTip(QObject::tr("右视图"));
	a->setStatusTip(QObject::tr("右视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(right()));
	myViewActions->insert(ViewRightId, a);

	a = new QAction(QIcon(ICON_VIEW_AXO), QObject::tr("轴测视图"), this);
	a->setToolTip(QObject::tr("轴测视图"));
	a->setStatusTip(QObject::tr("轴测视图"));
	connect(a, SIGNAL(triggered()), this, SLOT(axo()));
	myViewActions->insert(ViewAxoId, a);

	a = new QAction(QIcon(ICON_VIEW_RESET), QObject::tr("视图—复原"), this);
	a->setToolTip(QObject::tr("视图—复原"));
	a->setStatusTip(QObject::tr("视图复原至初始位置"));
	connect(a, SIGNAL(triggered()), this, SLOT(reset()));
	myViewActions->insert(ViewResetId, a);

	QActionGroup* ag = new QActionGroup(this);

	a = new QAction(QIcon(ICON_VIEW_COMP_ON), QObject::tr("MNU_HLROFF"), this);
	a->setToolTip(QObject::tr("TBR_HLROFF"));
	a->setStatusTip(QObject::tr("TBR_HLROFF"));
	connect(a, SIGNAL(triggered()), this, SLOT(hlrOff()));
	a->setCheckable(true);
	a->setChecked(true);
	ag->addAction(a);
	myViewActions->insert(ViewHlrOffId, a);

	a = new QAction(QIcon(ICON_VIEW_COMP_OFF), QObject::tr("MNU_HLRON"), this);
	a->setToolTip(QObject::tr("TBR_HLRON"));
	a->setStatusTip(QObject::tr("TBR_HLRON"));
	connect(a, SIGNAL(triggered()), this, SLOT(hlrOn()));

	a->setCheckable(true);
	ag->addAction(a);
	myViewActions->insert(ViewHlrOnId, a);
}

void OcctView::initRaytraceActions()
{
	if (myRaytraceActions)
		return;

	myRaytraceActions = new QList<QAction*>();
	QString dir;
	QAction* a;

	a = new QAction(QPixmap(ICON_RAYTRACE_RAYTRACING), QObject::tr("启动光线追踪"), this);
	a->setToolTip(QObject::tr("启动光线追踪"));
	a->setStatusTip(QObject::tr("启动光线追踪"));
	a->setCheckable(true);
	a->setChecked(true);//默认开启
	connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
	myRaytraceActions->insert(ToolRaytracingId, a);

	a = new QAction(QPixmap(ICON_RAYTRACE_SHADOWS), QObject::tr("启用阴影"), this);
	a->setToolTip(QObject::tr("启用阴影"));
	a->setStatusTip(QObject::tr("启用阴影"));
	a->setCheckable(true);
	a->setChecked(true);
	connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
	myRaytraceActions->insert(ToolShadowsId, a);

	a = new QAction(QPixmap(ICON_RAYTRACE_REFLECTIONS), QObject::tr("启用反射"), this);
	a->setToolTip(QObject::tr("启用反射"));
	a->setStatusTip(QObject::tr("启用反射"));
	a->setCheckable(true);
	a->setChecked(false);//默认关闭
	connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
	myRaytraceActions->insert(ToolReflectionsId, a);

	a = new QAction(QPixmap(ICON_RAYTRACE_ANTIALIASING), QObject::tr("启用抗锯齿"), this);
	a->setToolTip(QObject::tr("启用抗锯齿"));
	a->setStatusTip(QObject::tr("启用抗锯齿"));
	a->setCheckable(true);
	a->setChecked(false);
	connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
	myRaytraceActions->insert(ToolAntialiasingId, a);
}

#include <AIS_Shape.hxx>
void OcctView::mousePressEvent(QMouseEvent* e)
{
	//if (m_selectionMode == FaceSelection) {
	//	// 实现面选择逻辑（使用AIS_InteractiveContext）
	//	TopoDS_Face face; // 通过拾取获取
	//	myContext->Deactivate();
	//	myContext->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
	//	// 1. 获取交互上下文
	//	if (!myContext) {
	//		Msg::ShowError("交互上下文未初始化！");
	//		return;
	//	}
	//	// 2. 检查是否有选中的对象
	//	if (!myContext->HasSelectedShape()) {
	//		Msg::ShowInfo("请先选择一个边！");
	//		// 创建事件循环，等待用户选择边
	//		//QEventLoop loop;
	//		//QObject::connect(h_MyViewer->getSignals(), &MyViewerSignals::selectionChangedSignal, &loop, &QEventLoop::quit);
	//		//loop.exec(); // 等待用户选择边
	//	}
	//	face = TopoDS::Face(myContext->SelectedShape());
	//	emit faceSelected(face);
	//}
	//else if (m_selectionMode == EdgeSelection) {
	//	// 实现边选择逻辑
	//	TopoDS_Edge edge;
	//	emit edgeSelected(edge);
	//}
	//QWidget::mousePressEvent(e);

	if (e->button() == Qt::LeftButton)
		onLButtonDown(e->buttons(), e->modifiers(), e->pos());

	else if (e->button() == Qt::MiddleButton)
		onMButtonDown(e->buttons(), e->modifiers(), e->pos());
	else if (e->button() == Qt::RightButton)
		onRButtonDown(e->buttons(), e->modifiers(), e->pos());
}

void OcctView::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
		onLButtonUp(e->buttons(), e->modifiers(), e->pos());
	else if (e->button() == Qt::MiddleButton)
		onMButtonUp(e->buttons(), e->modifiers(), e->pos());
	else if (e->button() == Qt::RightButton)
		onRButtonUp(e->buttons(), e->modifiers(), e->pos());
}

void OcctView::mouseMoveEvent(QMouseEvent* e)
{
	onMouseMove(e->buttons(), e->modifiers(), e->pos());
}

void OcctView::wheelEvent(QWheelEvent * event)
{
	double scar = 1.0;
	double zDelta = event->angleDelta().y();
	if (zDelta>0)
	{
		scar = 1.2;
		if (myView->Scale() <= 10000)
			myView->SetZoom(scar);
	}
	else
	{
		scar = 0.8;
		if (myView->Scale()>0.0001)
			myView->SetZoom(scar);
	}
}

void OcctView::activateCursor(const CurrentAction3d mode)
{
	switch( mode )
	{
	  case CurAction3d_DynamicPanning:
	    setCursor( *panCursor );
	    break;
	  case CurAction3d_DynamicZooming:
	    setCursor( *zoomCursor );
	    break;
	  case CurAction3d_DynamicRotation:
	    setCursor( *rotCursor );
	    break;
	  case CurAction3d_GlobalPanning:
	    setCursor( *globPanCursor );
	    break;
	  case CurAction3d_WindowZooming:
	    setCursor( *handCursor );
	    break;
	  case CurAction3d_DragEvent:
	  case CurAction3d_Nothing:
	  default:
	    setCursor( *defCursor );
	    break;
	}
}

void OcctView::onLButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point)
{
	//  save the current mouse coordinate in min
	myXmin = point.x();
	myYmin = point.y();
	myXmax = point.x();
	myYmax = point.y();

	if (nKeys & CASCADESHORTCUTKEY)
	{
		//if (myHlrModeIsOn)
		//{
		//	myView->SetComputedMode(Standard_False);   // 关闭隐藏线显示模式
		//}
		myCurrentMode = CurAction3d_DynamicRotation;
		myView->StartRotation(point.x(), point.y());
	}
	else
	{
		switch (myCurrentMode)
		{
		case CurAction3d_Nothing:
			if (nKeys & MULTISELECTIONKEY)
				MultiInputEvent(point.x(), point.y());
			else
				InputEvent(point.x(), point.y());

			if (this->getSelectNb() == 0)
			{
				myCurrentMode = CurAction3d_DragEvent;
				if (nKeys & MULTISELECTIONKEY)
					MultiDragEvent(myXmax, myYmax, -1);
				else
					DragEvent(myXmax, myYmax, -1);
			}
			else if (this->getSelectNb() == 1)
			{
				myCurrentMode = CurAction3d_SingleOperation;
				emit SingleOperation_Begin(myView, myContext, point);
				{
					myContext->InitSelected();
					emit SingleSelect(myContext, myContext->SelectedInteractive());
				}
			}
			break;
		case CurAction3d_SingleOperation:
			break;
		case CurAction3d_DragEvent:
			break;
		case CurAction3d_DynamicZooming:
			break;
		case CurAction3d_WindowZooming:
			break;
		case CurAction3d_DynamicPanning:
			break;
		case CurAction3d_GlobalPanning:
			break;
		case CurAction3d_DynamicRotation:
			//if (myHlrModeIsOn)
			//{
			//	myView->SetComputedMode(Standard_False);
			//}
			myView->StartRotation(point.x(), point.y());
			break;
		default:
			throw Standard_Failure("incompatible Current Mode");
			break;
		}
	}
	activateCursor(myCurrentMode);
}

void OcctView::onMButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point)
{
	myXmin = point.x();
	myYmin = point.y();
	myXmax = point.x();
	myYmax = point.y();

	myCurrentMode = CurAction3d_DynamicRotation;
	myView->StartRotation(point.x(), point.y());

	if (nKeys & CASCADESHORTCUTKEY)
		myCurrentMode = CurAction3d_DynamicPanning;
	  
	activateCursor(myCurrentMode);
}

void OcctView::onRButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point)
{
	myXmin = point.x();
	myYmin = point.y();
	myXmax = point.x();
	myYmax = point.y();

	if (nKeys & CASCADESHORTCUTKEY)
	{
		myCurrentMode = CurAction3d_DynamicZooming;
	}
	activateCursor(myCurrentMode);
}

#include <TopoDS_Shape.hxx>
#include <BRepIntCurveSurface_Inter.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <AIS_Point.hxx>
#include <Geom_CartesianPoint.hxx>
#include "Msg.h"
#include "PropertyView.h"

void OcctView::onLButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point)
{
	switch (myCurrentMode)
	{
	case CurAction3d_Nothing:
		break;
	case CurAction3d_SingleOperation:
		if (point != QPoint(myXmin, myYmin))
			emit SingleOperation_End(myView, myContext, point);
		else if (point == QPoint(myXmin, myYmin))
		{
			emit selectionChanged();
			myContext->InitSelected();
			//Handle(Prs3d_Drawer) t_hilight_style = myContext->HighlightStyle(); // 获取高亮风格
			//t_hilight_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
			//t_hilight_style->SetColor(Quantity_NOC_LIGHTSTEELBLUE);    // 设置高亮颜色
			//t_hilight_style->SetDisplayMode(1); // 整体高亮
			//t_hilight_style->SetTransparency(0.2f); // 设置透明度
			// 设置选择模型的风格
			TopoDS_Shape Shape = myContext->SelectedShape();
			Handle(Prs3d_Drawer) t_select_style = myContext->SelectionStyle();  // 获取选择风格
			t_select_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
			t_select_style->SetColor(Quantity_NOC_LIGHTSEAGREEN);   // 设置选择后颜色
			t_select_style->SetDisplayMode(1); // 整体高亮
			t_select_style->SetTransparency(0.1f); // 设置透明度
			if (Shape.IsNull()) {
				break;
			}
			QString Type;
			switch (Shape.ShapeType()) {
			case TopAbs_VERTEX:
				Msg::ShowInfo("拾取顶点成功");
				Type = "顶点";
				break;
			case TopAbs_EDGE:
				Msg::ShowInfo("拾取边成功");
				Type = "边";
				break;
			case TopAbs_WIRE:
				Msg::ShowInfo("拾取线框成功");
				Type = "线框";
				break;
			case TopAbs_FACE:
				Msg::ShowInfo("拾取面成功");
				Type = "面";
				break;
			case TopAbs_SOLID:
				Msg::ShowInfo("拾取实体成功");
				Type = "实体";
				break;
			case TopAbs_COMPOUND:
				Msg::ShowInfo("拾取组合体成功");
				Type = "组合体";
				break;
			case TopAbs_SHELL:
				Msg::ShowInfo("拾取壳体成功");
				Type = "壳体";
				break;
			default:
				Msg::ShowInfo("未知类型");
				Type = "未知类型";
				break;
			}
			if (m_propertyView) {
				m_propertyView->setShapeView(Shape);  // 将获取的图形输入到属性栏中显示
			}
			//如果选择的是一个面 (TopAbs_FACE)，则进一步计算鼠标点击位置在视图中的三维坐标，并创建一条视图线。
			//使用 BRepIntCurveSurface_Inter 类计算这条视图线与选中面的交点。
			//如果存在交点，获取交点的位置和法向量，并在视图上显示一个点标记 AisPnt。
			if (Shape.IsNull() == false && Shape.ShapeType() == TopAbs_FACE)
			{
				Standard_Integer Xs = point.x();
				Standard_Integer Ys = point.y();
				Standard_Real Xv, Yv, Zv;
				Standard_Real Vx, Vy, Vz;
				myView->Convert(Xs, Ys, Xv, Yv, Zv); //向量点
				myView->Proj(Vx, Vy, Vz);     //向量方向
				gp_Pnt Pnt(Xv, Yv, Zv);
				gp_Dir Dir(Vx, Vy, Vz);
				gp_Lin ViewLine = gp_Lin(Pnt, Dir);
				TopoDS_Face TouchFace = TopoDS::Face(Shape);
				BRepIntCurveSurface_Inter IntCS;
			
				IntCS.Init(TouchFace, ViewLine, Precision::Confusion());
			
				if (IntCS.More()) {
					BRepGProp_Face G(TopoDS::Face(TouchFace));
					gp_Vec GetVec;	//该点处的法向量
					gp_Pnt GetTouchPnt;	 //该点处的触摸点
					G.Normal(IntCS.U(), IntCS.V(), GetTouchPnt, GetVec);
					gp_Dir GetNormalDir = gp_Dir(GetVec); //该点处单位向量
					TopoDS_Vertex V;
					if (AisPnt.IsNull())
					{
						AisPnt = new AIS_Point(new Geom_CartesianPoint(GetTouchPnt));
						myContext->Display(AisPnt,Standard_True);
					}
					else
					{
						AisPnt->SetComponent(new Geom_CartesianPoint(GetTouchPnt));
						myContext->Redisplay(AisPnt, Standard_True);
					}
					if (m_propertyView) {
						m_propertyView->setAisPoint(AisPnt);  // 将获取的图形输入到属性栏中显示
						m_propertyView->setDir(GetNormalDir);  // 将获取的法向量输入到属性栏中显示
					}
				}
			}
		}
		myCurrentMode = CurAction3d_Nothing;
		break;
	case CurAction3d_DragEvent:
		if ((point.x() == myXmin && point.y() == myYmin) == false)
		{
			DrawRectangle(myXmin, myYmin, myXmax, myYmax, Standard_False);
			myXmax = point.x();
			myYmax = point.y();
			if (nKeys & MULTISELECTIONKEY)
				MultiDragEvent(point.x(), point.y(), 1);
			else
				DragEvent(point.x(), point.y(), 1);
		}
		myCurrentMode = CurAction3d_Nothing;
		break;
	case CurAction3d_DynamicZooming:
		myCurrentMode = CurAction3d_Nothing;
		noActiveActions();
		break;
	case CurAction3d_WindowZooming:
		DrawRectangle(myXmin, myYmin, myXmax, myYmax, Standard_False);//,LongDash);
		myXmax = point.x();
		myYmax = point.y();
		if ((abs(myXmin - myXmax) > ValZWMin) ||
			(abs(myYmin - myYmax) > ValZWMin))
			myView->WindowFitAll(myXmin, myYmin, myXmax, myYmax);
		myCurrentMode = CurAction3d_Nothing;
		noActiveActions();
		break;
	case CurAction3d_DynamicPanning:
		myCurrentMode = CurAction3d_Nothing;
		noActiveActions();
		break;
	case CurAction3d_GlobalPanning:
		myView->Place(point.x(), point.y(), myCurZoom);
		myCurrentMode = CurAction3d_Nothing;
		noActiveActions();
		break;
	case CurAction3d_DynamicRotation:
		myCurrentMode = CurAction3d_Nothing;
		noActiveActions();
		break;
	default:
		throw Standard_Failure(" incompatible Current Mode ");
		break;
	}
	activateCursor(myCurrentMode);
}

void OcctView::onMButtonUp(Qt::MouseButtons /*nFlags*/, Qt::KeyboardModifiers nKeys, const QPoint /*point*/)
{
	myCurrentMode = CurAction3d_Nothing;
	activateCursor(myCurrentMode);
}

void OcctView::onRButtonUp(Qt::MouseButtons /*nFlags*/, Qt::KeyboardModifiers nKeys, const QPoint point)
{
	if (myCurrentMode == CurAction3d_Nothing)
	{
		if ((point.x() == myXmin && point.y() == myYmin) /*&& (nKeys & MULTISELECTIONKEY)*/)
			Popup(point.x(), point.y());
	}
	else
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		// reset tyhe good Degenerated mode according to the strored one
		//   --> dynamic rotation may have change it
		//if (myHlrModeIsOn)
		//{
		//	myView->SetComputedMode(myHlrModeIsOn);//<恢复隐藏线模式
		//	myView->Redraw();
		//}
		QApplication::restoreOverrideCursor();
		myCurrentMode = CurAction3d_Nothing;
	}
	activateCursor(myCurrentMode);
}

void OcctView::onMouseMove(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point)
{
	if (nFlags & Qt::LeftButton || nFlags & Qt::RightButton || nFlags & Qt::MiddleButton)
	{
		switch (myCurrentMode)
		{
		case CurAction3d_Nothing:
			myXmax = point.x();
			myYmax = point.y();
			break;
		case CurAction3d_SingleOperation:
			myXmax = point.x();
			myYmax = point.y();
			emit SingleOperation_Move(myView, myContext, point);
			break;
		case CurAction3d_DragEvent:
			if (nFlags & Qt::LeftButton)
			{
				myXmax = point.x();
				myYmax = point.y();
				DrawRectangle(myXmin, myYmin, myXmax, myYmax, Standard_False);
				if (nKeys & MULTISELECTIONKEY)
					MultiDragEvent(myXmax, myYmax, 0);
				else
					DragEvent(myXmax, myYmax, 0);
				DrawRectangle(myXmin, myYmin, myXmax, myYmax, Standard_True);
			}
			break;
		case CurAction3d_DynamicZooming:
			myView->Zoom(myXmax, myYmax, point.x(), point.y());
			myXmax = point.x();
			myYmax = point.y();
			break;
		case CurAction3d_WindowZooming:
			myXmax = point.x();
			myYmax = point.y();
			DrawRectangle(myXmin, myYmin, myXmax, myYmax, Standard_False);
			DrawRectangle(myXmin, myYmin, myXmax, myYmax, Standard_True);
			break;
		case CurAction3d_DynamicPanning:
			myView->Pan(point.x() - myXmax, myYmax - point.y());
			myXmax = point.x();
			myYmax = point.y();
			break;
		case CurAction3d_GlobalPanning:
			break;
		case CurAction3d_DynamicRotation:
			myView->Rotation(point.x(), point.y());
			myView->Redraw();
			break;
		default:
			throw Standard_Failure("incompatible Current Mode");
			break;
		}
	}
	else
	{
		myXmax = point.x();
		myYmax = point.y();
		if (nKeys & MULTISELECTIONKEY)
			MultiMoveEvent(point.x(), point.y());
		else
			MoveEvent(point.x(), point.y());
	}
}

void OcctView::DragEvent(const int x, const int y, const int TheState)
{
	// TheState == -1  button down
	// TheState ==  0  move
	// TheState ==  1  button up

	static Standard_Integer theButtonDownX = 0;
	static Standard_Integer theButtonDownY = 0;

	if (TheState == -1)
	{
		theButtonDownX = x;
		theButtonDownY = y;
	}

	if (TheState == 0)
	{
		// 移动时：实时更新选择预览（可选，用于实时反馈）
		// 注意：这里不执行最终选择，只是预览
		myContext->Select(theButtonDownX, theButtonDownY, x, y, myView, Standard_False);
	}

	if (TheState == 1)
	{
		// 释放时：执行最终选择
		myContext->Select(theButtonDownX, theButtonDownY, x, y, myView, Standard_True);
		myContext->UpdateCurrentViewer(); // 确保视图更新
		emit selectionChanged();
	}
}

void OcctView::InputEvent(const int /*x*/, const int /*y*/)
{
	myContext->Select(Standard_True);
	if (!myXYZContext.IsNull())
		myXYZContext->Select(Standard_True);
	emit selectionChanged();
}

void OcctView::MoveEvent(const int x, const int y)
{
	myContext->MoveTo(x, y, myView, Standard_True);
	if (!myXYZContext.IsNull())
		myXYZContext->MoveTo(x, y, myView, Standard_True);
}

void OcctView::MultiMoveEvent(const int x, const int y)
{
	myContext->MoveTo(x, y, myView, Standard_True);
}

void OcctView::MultiDragEvent(const int x, const int y, const int TheState)
{
	static Standard_Integer theButtonDownX = 0;
	static Standard_Integer theButtonDownY = 0;

	if (TheState == -1)
	{
		theButtonDownX = x;
		theButtonDownY = y;
	}
	if (TheState == 0)
	{
		myContext->ShiftSelect(theButtonDownX, theButtonDownY, x, y, myView, Standard_True);
		emit selectionChanged();
	}
}

void OcctView::MultiInputEvent(const int /*x*/, const int /*y*/)
{
	myContext->ShiftSelect(Standard_True);
	emit selectionChanged();
}

void OcctView::Popup(const int /*x*/, const int /*y*/)
{
	//ApplicationCommonWindow* stApp = ApplicationCommonWindow::getApplication();
	//QMdiArea* ws = ApplicationCommonWindow::getWorkspace();
	//QMdiSubWindow* w = ws->activeSubWindow();
	//if ( myContext->NbSelected() )
	//{
	//  QList<QAction*>* aList = stApp->getToolActions();
	//  QMenu* myToolMenu = new QMenu( 0 );
	//  myToolMenu->addAction( aList->at( ApplicationCommonWindow::ToolWireframeId ) );
	//  myToolMenu->addAction( aList->at( ApplicationCommonWindow::ToolShadingId ) );
	//  myToolMenu->addAction( aList->at( ApplicationCommonWindow::ToolColorId ) );
	//      
	//  QMenu* myMaterMenu = new QMenu( myToolMenu );
	//  QList<QAction*>* aMeterActions = ApplicationCommonWindow::getApplication()->getMaterialActions();
	//      
	//  QString dir = ApplicationCommonWindow::getResourceDir() + QString( "/" );
	//  myMaterMenu = myToolMenu->addMenu( QPixmap( dir+QObject::tr("ICON_TOOL_MATER")), QObject::tr("MNU_MATER") );
	//  for ( int i = 0; i < aMeterActions->size(); i++ )
	//    myMaterMenu->addAction( aMeterActions->at( i ) );
	//     
	//  myToolMenu->addAction( aList->at( ApplicationCommonWindow::ToolTransparencyId ) );
	//  myToolMenu->addAction( aList->at( ApplicationCommonWindow::ToolDeleteId ) );
	//  addItemInPopup(myToolMenu);
	//  myToolMenu->exec( QCursor::pos() );
	//  delete myToolMenu;
	//}
	//else
	{
		if (!myBackMenu)
		{
			myBackMenu = new QMenu(0);
			
			//背景
			{
				QMenu * BackMenu = new QMenu("背景", myBackMenu);
				QAction* a = new QAction(QObject::tr("设置背景颜色"), this);
				a->setToolTip(QObject::tr("设置背景颜色"));
				connect(a, SIGNAL(triggered()), this, SLOT(onBackground()));
				BackMenu->addAction(a);

				a = new QAction(QObject::tr("截屏"), this);
				a->setToolTip(QObject::tr("截屏"));
				connect(a, SIGNAL(triggered()), this, SLOT(onScreenShot()));
				BackMenu->addAction(a);
				myBackMenu->addMenu(BackMenu);
			}
			//剖视图
			{
				QMenu * ClipMenu = new QMenu("剖视图", myBackMenu);

				QAction* a = new QAction(QObject::tr("无剖视图"), this);
				a->setToolTip(QObject::tr("无剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());});
				ClipMenu->addAction(a);

				a = new QAction(QObject::tr("X轴正向"), this);
				a->setToolTip(QObject::tr("X轴正向-剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{
					myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());
					myView->AddClipPlane(new Graphic3d_ClipPlane(gp_Pln(gp::Origin(), gp::DX())));
				});
				ClipMenu->addAction(a);

				a = new QAction(QObject::tr("X轴反向"), this);
				a->setToolTip(QObject::tr("X轴反向-剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{
					myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());
					myView->AddClipPlane(new Graphic3d_ClipPlane(gp_Pln(gp::Origin(), -gp::DX())));
				});
				ClipMenu->addAction(a);

				a = new QAction(QObject::tr("Y轴正向"), this);
				a->setToolTip(QObject::tr("Y轴正向-剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{
					myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());
					myView->AddClipPlane(new Graphic3d_ClipPlane(gp_Pln(gp::Origin(), gp::DY())));
				});
				ClipMenu->addAction(a);

				a = new QAction(QObject::tr("Y轴反向"), this);
				a->setToolTip(QObject::tr("Y轴反向-剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{
					myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());
					myView->AddClipPlane(new Graphic3d_ClipPlane(gp_Pln(gp::Origin(), -gp::DY())));
				});
				ClipMenu->addAction(a);

				a = new QAction(QObject::tr("Z轴正向"), this);
				a->setToolTip(QObject::tr("Z轴正向-剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{
					myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());
					myView->AddClipPlane(new Graphic3d_ClipPlane(gp_Pln(gp::Origin(), gp::DZ())));
				});
				ClipMenu->addAction(a);

				a = new QAction(QObject::tr("Z轴反向"), this);
				a->setToolTip(QObject::tr("Z轴反向-剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{
					myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());
					myView->AddClipPlane(new Graphic3d_ClipPlane(gp_Pln(gp::Origin(), -gp::DZ())));
				});
				ClipMenu->addAction(a);

				a = new QAction(QObject::tr("自定义平面"), this);
				a->setToolTip(QObject::tr("自定义平面-剖视图"));
				connect(a, &QAction::triggered, this, [=]()
				{
					//创建对话框
					QDialog Dialog;
					Dialog.setWindowTitle("请输入一个自定义平面");
					Dialog.setMaximumHeight(40);
					//创建编辑框
					QRegExp regExp("-?[0-9]*\\.[0-9]*");
					QLineEdit * pEdits[6] = {nullptr};
					for (int index = 0; index < 6; index++)
					{
						QLineEdit * pEdit = new QLineEdit("0", &Dialog);
						pEdit->setValidator(new QRegExpValidator(regExp, pEdit));
						pEdit->setMinimumWidth(5);
						pEdits[index] = pEdit;
					}
					//布局
					QVBoxLayout * pLayout = new QVBoxLayout();
					QString LabelStr[2] = { "平面上一点","平面的方向" };
					for (int m = 0; m < 2; m++)
					{
						QHBoxLayout * pLayout_in = new QHBoxLayout();
						pLayout->addLayout(pLayout_in);
						pLayout_in->addWidget(new QLabel(LabelStr[m], &Dialog));
						for (int n = 0; n < 3; n++)
							pLayout_in->addWidget(pEdits[3 * m + n]);
					}
					QDialogButtonBox * pBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &Dialog);
					connect(pBox, &QDialogButtonBox::accepted, &Dialog, &QDialog::accept);
					connect(pBox, &QDialogButtonBox::rejected, &Dialog, &QDialog::reject);
					pLayout->addWidget(pBox);
					Dialog.setLayout(pLayout);
					if (QDialog::Accepted == Dialog.exec())
					{
						double P[6] = { 0 };
						for(int index = 0;index<6;index++)
							P[index] = pEdits[index]->text().toDouble();
						if (sqrt(P[3] * P[3] + P[4] * P[4] + P[5] * P[5]) > 1e-7)
						{
							gp_Pnt Pnt(P[0], P[1], P[2]);
							gp_Dir Dir(P[3], P[4], P[5]);
							myView->SetClipPlanes(new Graphic3d_SequenceOfHClipPlane());
							myView->AddClipPlane(new Graphic3d_ClipPlane(gp_Pln(Pnt, Dir)));
						}
						else
							QMessageBox::information(this, "ERROR", "向量的模长不能为零");
					}
				});
				ClipMenu->addAction(a);
				myBackMenu->addMenu(ClipMenu);
			}
		}

		myBackMenu->exec(QCursor::pos());
	}
}

void OcctView::DrawRectangle(const int MinX, const int MinY,
	const int MaxX, const int MaxY, const bool Draw)
{
	static Standard_Integer StoredMinX, StoredMaxX, StoredMinY, StoredMaxY;
	static Standard_Boolean m_IsVisible;

	StoredMinX = (MinX < MaxX) ? MinX : MaxX;
	StoredMinY = (MinY < MaxY) ? MinY : MaxY;
	StoredMaxX = (MinX > MaxX) ? MinX : MaxX;
	StoredMaxY = (MinY > MaxY) ? MinY : MaxY;

	QRect aRect;
	aRect.setRect(StoredMinX, StoredMinY, abs(StoredMaxX - StoredMinX), abs(StoredMaxY - StoredMinY));

	if (!myRectBand)
	{
		myRectBand = new QRubberBand(QRubberBand::Rectangle, this);
		myRectBand->setStyle(QStyleFactory::create("windows"));
		myRectBand->setGeometry(aRect);
		myRectBand->show();
	}

	if (m_IsVisible && !Draw) // move or up  : erase at the old position
	{
		myRectBand->hide();
		delete myRectBand;
		myRectBand = 0;
		m_IsVisible = false;
	}

	if (Draw) // move : draw
	{
		//aRect.setRect( StoredMinX, StoredMinY, abs(StoredMaxX-StoredMinX), abs(StoredMaxY-StoredMinY));
		m_IsVisible = true;
		myRectBand->setGeometry(aRect);
		//myRectBand->show();
	}
}

void OcctView::noActiveActions()
{
	for (int i = ViewFitAllId; i < ViewHlrOffId; i++)
	{
		QAction* anAction = myViewActions->at(i);
		if ((anAction == myViewActions->at(ViewFitAreaId)) ||
			(anAction == myViewActions->at(ViewZoomId)) ||
			(anAction == myViewActions->at(ViewPanId)) ||
			(anAction == myViewActions->at(ViewGlobalPanId)) ||
			(anAction == myViewActions->at(ViewRotationId)))
		{
			setCursor(*defCursor);
			anAction->setCheckable(true);
			anAction->setChecked(false);
		}
	}
}
//设置背景颜色
void OcctView::onBackground()
{
	myView->SetBgGradientStyle(Aspect_GradientFillMethod_None,Standard_False);
	QColor aColor;
	Standard_Real R1;
	Standard_Real G1;
	Standard_Real B1;
	myView->BackgroundColor(Quantity_TOC_RGB, R1, G1, B1);
	aColor.setRgb((Standard_Integer)(R1 * 255), (Standard_Integer)(G1 * 255), (Standard_Integer)(B1 * 255));
	QColor aRetColor = QColorDialog::getColor(aColor);
	if (aRetColor.isValid())
	{
		R1 = aRetColor.red() / 255.;
		G1 = aRetColor.green() / 255.;
		B1 = aRetColor.blue() / 255.;
		myView->SetBackgroundColor(Quantity_TOC_RGB, R1, G1, B1);
	}
	myView->Redraw();//刷新
}

//截图
void OcctView::onScreenShot()
{
	//QMessageBox::information(nullptr, "截图", "请选择截图保存路径");
	Msg::ShowInfo("截图—选择保存路径");

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), "", tr("png文件 (*.png)"
																						";;jpg文件(*.jpg)"
																						";;bmp文件( *.bmp)"));

	if (fileName.isEmpty()) {return;}

	if (this->dump(fileName.toUtf8())) {
		QMessageBox::information(this, tr("截图保存"), tr("截图" + fileName.toUtf8() + "保存成功"));
	}
	else {
		QMessageBox::warning(this, tr("Error"), tr("截图保存失败"));
	}
}

//保存截图
bool OcctView::dump(Standard_CString theFile)
{
	return myView->Dump(theFile);
}

int OcctView::getSelectNb()
{
	return myContext->NbSelected();
}

OCCT_ShapeList OcctView::getSelectedShapes()
{
	OCCT_ShapeList Shapes;
	for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())//遍历上下文中的图形对象
		Shapes.Append(myContext->SelectedShape());//获取选中的图形对象
	return Shapes;
}

Handle(V3d_View)& OcctView::getView()
{
	return myView;
}

Handle(AIS_InteractiveContext)& OcctView::getContext()
{
	return myContext;
}

OcctView::CurrentAction3d OcctView::getCurrentMode()
{
	return myCurrentMode;
}

void OcctView::showBoundaryCondition(const TopoDS_Shape& shape)
{
	if (shape.IsNull()) {
		return;
	}

	// 清除之前的边界条件可视化
	//clearBoundaryConditions();

	// 根据形状类型创建不同的可视化标记
	TopAbs_ShapeEnum shapeType = shape.ShapeType();

	switch (shapeType) {
	case TopAbs_FACE:
		visualizeFaceBoundaryCondition(shape);
		break;
	case TopAbs_EDGE:
		visualizeEdgeBoundaryCondition(shape);
		break;
	case TopAbs_VERTEX:
		visualizeVertexBoundaryCondition(shape);
		break;
	//case TopAbs_SOLID:
	//	// 对于实体，在其所有面上显示边界条件
	//	visualizeSolidBoundaryCondition(shape);
		// break;
	default:
		break;
	}

	myView->Redraw();
}

void OcctView::clearBoundaryConditions()
{
	m_boundaryConditions.clear();
}

void OcctView::AddBoundaryCondition(const BoundaryConditionData& bcData)
{
	m_boundaryConditions.append(bcData);
}

void OcctView::SetBoundaryConditions(const QList<BoundaryConditionData>& bcList)
{
	m_boundaryConditions = bcList;
}

QList<BoundaryConditionData> OcctView::getBoundaryConditions()
{
	 return m_boundaryConditions; 
}

void OcctView::visualizeFaceBoundaryCondition(const TopoDS_Shape& shape)
{
}
void OcctView::visualizeEdgeBoundaryCondition(const TopoDS_Shape& shape)
{
}
void OcctView::visualizeVertexBoundaryCondition(const TopoDS_Shape& shape)
{
}

#include <Graphic3d_TransformPers.hxx>
// 设置颜色条显示/隐藏
void OcctView::setColorBarVisible(bool visible, const QString& title, double minVal, double maxVal)
{
	m_colorBarVisible = visible;
	
	if (m_colorScale.IsNull())
	{
		// 创建颜色条对象
		m_colorScale = new AIS_ColorScale();
		
		// 设置颜色条大小（宽度，高度）
		m_colorScale->SetSize(70, 400);
		
		// 设置间隔数量（刻度数量）
		m_colorScale->SetNumberOfIntervals(10);
		
		// 设置标签位置（右侧）
		m_colorScale->SetLabelPosition(Aspect_TOCSP_RIGHT);
		
		// 设置颜色条颜色（从蓝色到红色）
		// OpenCASCADE 默认使用蓝色到红色的渐变
		
		// 设置标签在边界显示
		m_colorScale->SetLabelAtBorder(Standard_True);
		
		// 设置颜色条在顶层显示（OSD层）
		m_colorScale->SetZLayer(Graphic3d_ZLayerId_TopOSD);

		m_colorScale->SetSmoothTransition(true);
		m_colorScale->SetColor(Quantity_NOC_BLACK);
		
		// 设置2D变换持久性，使颜色条固定在屏幕位置（左上角）
		Handle(Graphic3d_TransformPers) aTrsfPers = new Graphic3d_TransformPers(Graphic3d_TMF_2d, Aspect_TOTP_LEFT_LOWER, Graphic3d_Vec2i(0, 0));
		m_colorScale->SetTransformPersistence(aTrsfPers);//显示位置
		//m_colorScale->SetPosition(0,0);
	}
	
	// 如果提供了参数，更新标题和范围
	if (!title.isEmpty())
	{
		setColorScaleTitle(title);
	}
	if (minVal != maxVal)
	{
		m_displacementMin = minVal;
		m_displacementMax = maxVal;
	}
	
	if (visible)
	{
		// 显示颜色条
		if (!myContext->IsDisplayed(m_colorScale))
		{
			myContext->Display(m_colorScale, Standard_False);
		}
		updateColorScale();
	}
	else
	{
		// 隐藏颜色条
		if (myContext->IsDisplayed(m_colorScale))
		{
			myContext->Erase(m_colorScale, Standard_False);
		}
	}
	
	myContext->UpdateCurrentViewer();
}

// 设置 displacement 范围
void OcctView::setDisplacementRange(double minVal, double maxVal)
{
	m_displacementMin = minVal;
	m_displacementMax = maxVal;
	
	if (!m_colorScale.IsNull())
	{
		updateColorScale();
	}
}

// 设置颜色条标题
void OcctView::setColorScaleTitle(const QString& title)
{
	if (!m_colorScale.IsNull())
	{
		m_colorScale->SetTitle(title.toStdString().c_str());
		if (myContext->IsDisplayed(m_colorScale))
		{
			myContext->Redisplay(m_colorScale, Standard_False);
			myContext->UpdateCurrentViewer();
		}
	}
}

// 更新颜色条
void OcctView::updateColorScale()
{
	if (m_colorScale.IsNull())
		return;
	
	// 更新范围
	m_colorScale->SetRange(m_displacementMin, m_displacementMax);
	
	// 更新显示
	if (myContext->IsDisplayed(m_colorScale))
	{
		myContext->Redisplay(m_colorScale, Standard_False);
		myContext->UpdateCurrentViewer();
	}
}
