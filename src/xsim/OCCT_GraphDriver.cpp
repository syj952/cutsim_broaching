#include "OCCT_GraphDriver.h"
#include "ComplainUtf8.h"
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Geom_Axis2Placement.hxx>
#include <TPrsStd_AISViewer.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <AIS_Trihedron.hxx>
#include <AIS_Point.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_AmbientLight.hxx>

const Handle(OpenGl_GraphicDriver) & GetGraphicDriver()
{
	static Handle(OpenGl_GraphicDriver)     H_GraphicDriver = NULL;
	if (H_GraphicDriver.IsNull())
	{
		//================================OPENGL物理设备============================
		try
		{
			Handle(Aspect_DisplayConnection) theDisp = new Aspect_DisplayConnection();
			H_GraphicDriver = new OpenGl_GraphicDriver(theDisp,false);
		}
		catch (Standard_Failure)
		{
			ExitProcess(1);
		}
		//================================OPENGL物理设备============================
	}

	return H_GraphicDriver;
}

Handle(V3d_Viewer) OCCT_GraphDriver::CreateViewer()
{
	const Handle(OpenGl_GraphicDriver) & hGraphicDriver = ::GetGraphicDriver();
	const Handle(V3d_Viewer) & hViewer = new V3d_Viewer(hGraphicDriver);
	//hViewer->SetDefaultLights();  //设置默认光源
	  // Lightning.
	Handle(V3d_DirectionalLight) LightDir = new V3d_DirectionalLight(V3d_Zneg, Quantity_Color(Quantity_NOC_GRAY97), 1);
	Handle(V3d_AmbientLight)     LightAmb = new V3d_AmbientLight();
	//
	LightDir->SetDirection(1.0, -2.0, -10.0);
	//
	hViewer->AddLight(LightDir);
	hViewer->AddLight(LightAmb);
	hViewer->SetLightOn(LightDir);
	hViewer->SetLightOn(LightAmb);

	hViewer->SetDefaultShadingModel(Graphic3d_TypeOfShadingModel_Phong); //设置默认着色模型为Phong

	return hViewer;
}

Handle(AIS_InteractiveContext) OCCT_GraphDriver::CreateAISContext(const Handle(V3d_Viewer) & hViewer)
{
	Handle(AIS_InteractiveContext) hAISContext =  new AIS_InteractiveContext(hViewer); //创建交互式环境
	hAISContext->SetDisplayMode(AIS_Shaded, true);   //设置显示模式为遮蔽
		const Handle(Prs3d_Drawer)& aDefaultDrawer = hAISContext->DefaultDrawer();
		aDefaultDrawer->SetFaceBoundaryDraw(Standard_True); // 启用面边界绘制
		aDefaultDrawer->SetupOwnFaceBoundaryAspect(); // 确保拥有独立的面边界属性，避免影响其他设置
	return hAISContext;
}

Handle(AIS_InteractiveContext) OCCT_GraphDriver::CreateViewCubeAISContext(const Handle(V3d_Viewer) & hViewer)
{
	Handle(AIS_InteractiveContext) hAISContext = new AIS_InteractiveContext(hViewer); //创建交互式环境
	hAISContext->SetDisplayMode(AIS_Shaded, true);   //设置显示模式为遮蔽
	{
		Handle(AIS_ViewCube) H_AisViewCube = new AIS_ViewCube();
		H_AisViewCube->SetBoxSideLabel(V3d_Xpos, "右视图");//!< (+Y+Z) view
		H_AisViewCube->SetBoxSideLabel(V3d_Ypos, "背视图");//!< (+X+Z) view
		H_AisViewCube->SetBoxSideLabel(V3d_Zpos, "俯视图");//!< (+Y+Z) view
		H_AisViewCube->SetBoxSideLabel(V3d_Xneg, "左视图");//!< (+Y+Z) view
		H_AisViewCube->SetBoxSideLabel(V3d_Yneg, "正视图");//!< (+Y+Z) view
		H_AisViewCube->SetBoxSideLabel(V3d_Zneg, "仰视图");//!< (+Y+Z) view
														//H_AisViewCube->Set();
		H_AisViewCube->SetTransparency(0.8); //
		H_AisViewCube->SetSize(80); //设置大小
		hAISContext->Display(H_AisViewCube, Standard_True);

		H_AisViewCube->SetTransformPersistence(
			new Graphic3d_TransformPers(
				Graphic3d_TMF_TriedronPers,
				Aspect_TOTP_RIGHT_UPPER,
				Graphic3d_Vec2i(85, 85)));
	}
	return hAISContext;
}

Handle(AIS_InteractiveContext) OCCT_GraphDriver::CreateXYZAISContext(const Handle(V3d_Viewer)& hViewer)
{
	Handle(AIS_InteractiveContext) hAISContext = new AIS_InteractiveContext(hViewer); //创建交互式环境
	hAISContext->SetDisplayMode(AIS_Shaded, true);   //设置显示模式为遮蔽
	Handle(AIS_Point) Point = new AIS_Point(new Geom_CartesianPoint(0, 0, 0));
	hAISContext->Display(Point, Standard_False);
	Handle(AIS_Trihedron) Trihedron = new AIS_Trihedron(new Geom_Axis2Placement(gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1))));
	Trihedron->SetSize(40);
	hAISContext->Display(Trihedron, Standard_False);
	hAISContext->SetZLayer(Trihedron, Graphic3d_ZLayerId_Topmost);
	opencascade::handle<Graphic3d_TransformPers> transform =
		new Graphic3d_TransformPers(Graphic3d_TMF_ZoomPers);
	hAISContext->SetTransformPersistence(Trihedron, transform);//禁止缩放
	return hAISContext;
}

Handle(V3d_View) OCCT_GraphDriver::CreateView(const Handle(V3d_Viewer) & hViewer)
{
	Handle(V3d_View) hView;
	hView = hViewer->CreateView();
	Standard_Boolean myHlrModeIsOn = Standard_False;
	hView->SetComputedMode(myHlrModeIsOn);
	return hView;
}

