#include "mainwindow.h"
#include "mdichild.h"
#include "ICon.h"
#include "ComplainUtf8.h"
#include "Msg.h"
#include <QtWidgets>
#include "ProjectTree.h"
#include <algorithm>
#include <QFormLayout>
#include <QDoubleSpinBox>
void MainWindow::createActions()
{
	//文件
	{
		QMenu *fileMenu = menuBar()->addMenu(tr("文件"));
		//fileMenu->setStyleSheet(
		//	"QMenu {"
		//	"   background-color: #D3D3D3;" // 绿色背景
		//	"   color: #000000;"            // 黑色文字
		//	"   font-size: 12pt; "           // 增大字体大小
		//	"   padding-top: 4px;"
		//	"   padding-bottom: 12px;"
		//	"}"
		//	"QMenu::item {"
		//	"   background-color: transparent;" // 菜单项背景透明
		//	"}"
		//	"QMenu::item:selected {"
		//	"   background-color: #FFFFFF;" // 选中项背景为白色
		//	"}"
		//);
		menuBar()->setFixedHeight(40);


		const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
		newAct = new QAction(newIcon, tr("新建"), this);
		newAct->setShortcuts(QKeySequence::New);
		newAct->setStatusTip(tr("Create a new file"));
		connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
		fileMenu->addAction(newAct);

		const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
		openAct = new QAction(openIcon, tr("打开..."), this);
		openAct->setShortcuts(QKeySequence::Open);
		openAct->setStatusTip(tr("Open an existing file"));
		connect(openAct, &QAction::triggered, this, &MainWindow::open);
		fileMenu->addAction(openAct);

		const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
		saveAct = new QAction(saveIcon, tr("保存"), this);
		saveAct->setShortcuts(QKeySequence::Save);//设置快捷键
		saveAct->setStatusTip(tr("Save the document to disk"));
		connect(saveAct, &QAction::triggered, this, &MainWindow::save);
		fileMenu->addAction(saveAct);

		const QIcon saveAsIcon = QIcon::fromTheme("document-save-as", QIcon(":/images/saveas.png"));
		saveAsAct = new QAction(saveAsIcon, tr("另存为..."), this);
		saveAsAct->setShortcuts(QKeySequence::SaveAs);
		saveAsAct->setStatusTip(tr("Save the document under a new name"));
		connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);
		fileMenu->addAction(saveAsAct);
		fileMenu->addSeparator();//添加分隔线

		closeAct = new QAction(tr("关闭"), this);
		closeAct->setStatusTip(tr("Close the active window"));
		connect(closeAct, &QAction::triggered,
			mdiArea, &QMdiArea::closeActiveSubWindow);
		fileMenu->addAction(closeAct);

		closeAllAct = new QAction(tr("关闭全部"), this);
		closeAllAct->setStatusTip(tr("Close all the windows"));
		connect(closeAllAct, &QAction::triggered, mdiArea, &QMdiArea::closeAllSubWindows);
		fileMenu->addAction(closeAllAct);

		fileMenu->addSeparator();
		QMenu *recentMenu = fileMenu->addMenu(tr("最近打开..."));
		connect(recentMenu, &QMenu::aboutToShow, this, &MainWindow::updateRecentFileActions);
		recentFileSubMenuAct = recentMenu->menuAction();

		for (int i = 0; i < MaxRecentFiles; ++i) {
			recentFileActs[i] = recentMenu->addAction(QString(), this, &MainWindow::openRecentFile);
			recentFileActs[i]->setVisible(false);
		}

		recentFileSeparator = fileMenu->addSeparator();

		setRecentFilesVisible(MainWindow::hasRecentFiles());

		//! [0]
		const QIcon exitIcon = QIcon::fromTheme("application-exit");
		QAction *exitAct = fileMenu->addAction(exitIcon, tr("退出&X"), qApp, &QApplication::closeAllWindows);
		exitAct->setShortcuts(QKeySequence::Quit);
		exitAct->setStatusTip(tr("Exit the application"));
		fileMenu->addAction(exitAct);
		//! [0]
	}

	//编辑
	{
		windowMenu = menuBar()->addMenu(tr("编辑"));
		connect(windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

		tileAct = new QAction(tr("标题"), this);
		tileAct->setStatusTip(tr("Tile the windows"));
		connect(tileAct, &QAction::triggered, mdiArea, &QMdiArea::tileSubWindows);

		cascadeAct = new QAction(tr("排列"), this);
		cascadeAct->setStatusTip(tr("Cascade the windows"));
		connect(cascadeAct, &QAction::triggered, mdiArea, &QMdiArea::cascadeSubWindows);

		nextAct = new QAction(tr("下一个"), this);
		nextAct->setShortcuts(QKeySequence::NextChild);
		nextAct->setStatusTip(tr("Move the focus to the next window"));
		connect(nextAct, &QAction::triggered, mdiArea, &QMdiArea::activateNextSubWindow);

		previousAct = new QAction(tr("上一个"), this);
		previousAct->setShortcuts(QKeySequence::PreviousChild);
		previousAct->setStatusTip(tr("Move the focus to the previous window"));
		connect(previousAct, &QAction::triggered, mdiArea, &QMdiArea::activatePreviousSubWindow);

		windowMenuSeparatorAct = new QAction(this);
		windowMenuSeparatorAct->setSeparator(true);	 //设置为分隔符

		updateWindowMenu();

		menuBar()->addSeparator();
	}

	{//工具栏
		QToolBar* fileToolBar = this->addToolBar(tr("文件"));

		QToolButton* newButton = new QToolButton(this);
		newButton->setDefaultAction(newAct);
		newButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		newButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 8pt;");
		newButton->setIconSize(QSize(32, 32));
		fileToolBar->addWidget(newButton);

		QToolButton* openButton = new QToolButton(this);
		openButton->setDefaultAction(openAct);
		openButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		openButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 8pt;");
		fileToolBar->addWidget(openButton);

		QToolButton* saveButton = new QToolButton(this);
		saveButton->setDefaultAction(saveAct);
		saveButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		saveButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 8pt;");
		fileToolBar->addWidget(saveButton);
		fileToolBar->setIconSize(QSize(40, 40));

	};

	//MDI菜单
	{
		this->createMDIActions();
		
	}
	//帮助
	{
		QMenu *helpMenu = menuBar()->addMenu(tr("帮助"));
		QAction *aboutAct = helpMenu->addAction(tr("关于作者"), this, &MainWindow::about);
		aboutAct->setStatusTip(tr("Show the application's About box"));
		QAction *aboutQtAct = helpMenu->addAction(tr("关于Qt"), qApp, &QApplication::aboutQt);
		aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
	}
	//仿真_syj
	{
		QMenu* CutsimMenu = menuBar() ->addMenu("Cutsim仿真");
		QAction* CutsimAct = new QAction(tr("Cutsim仿真"), this);
		CutsimAct->setStatusTip(tr("Cutsim仿真"));
		connect(CutsimAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->RunCutsim();
			});
		CutsimMenu->addAction(CutsimAct);
	}

}

void MainWindow::createMDIActions()
{
	//视图
	{
		Mdi_ViewMenu = menuBar()->addMenu("窗口");
		ChildMenus += Mdi_ViewMenu;
		{
		//	QAction* saveWindowStateAct = new QAction(tr("保存窗口状态"), this);
		//	saveWindowStateAct->setStatusTip(tr("保存当前窗口状态"));
		//	connect(saveWindowStateAct, &QAction::triggered, this, [=]()
		//	{
		//	if (activeMdiChild()) {
		//		activeMdiChild()->saveWindowState();
		//		statusBar()->showMessage(tr("保存窗口状态ing"), 2000);
		//		}
		//	});
		//	Mdi_ViewMenu->addAction(saveWindowStateAct);

		//	QAction* restoreWindowStateAct = new QAction(tr("恢复窗口状态"), this);
		//	restoreWindowStateAct->setStatusTip(tr("恢复上次保存的窗口状态"));
		//	connect(restoreWindowStateAct, &QAction::triggered, this, [=]()
		//	{
		//		if (activeMdiChild()) {
		//			activeMdiChild()->restoreWindowState();
		//			statusBar()->showMessage(tr("恢复窗口状态ing"), 2000);
		//		}
		//		});
		//	Mdi_ViewMenu->addAction(restoreWindowStateAct);

		//	QAction* resetWindowStateAct = new QAction(tr("重置窗口状态"), this);
		//	resetWindowStateAct->setStatusTip(tr("重置窗口状态为默认"));
		//	connect(resetWindowStateAct, &QAction::triggered, this, [=]()
		//	{
		//		if (activeMdiChild()) {
		//			activeMdiChild()->resetLayout();
		//			statusBar()->showMessage(tr("重置窗口状态ing"), 2000);
		//		}
		//		});
		//	Mdi_ViewMenu->addAction(resetWindowStateAct);
		}
	}
	//模型
	{
		QMenu *  Mdi_ModelMenu = menuBar()->addMenu("模型");
		ChildMenus += Mdi_ModelMenu;
		{
			QAction * ImportModelAct = new QAction(tr("导入模型"), this);
			ImportModelAct->setStatusTip(tr("导入模型到OCC系统"));
			connect(ImportModelAct, &QAction::triggered, this, [=]()
			{
				if (activeMdiChild())
				{
					activeMdiChild()->ImportModel();
					statusBar()->showMessage(tr("导入模型ing"), 2000);
				}
			});
			Mdi_ModelMenu->addAction(ImportModelAct);
		}
		{
			QAction * ExportModelAct = new QAction(tr("导出模型"), this);
			ExportModelAct->setStatusTip(tr("导出模型到文件"));
			connect(ExportModelAct, &QAction::triggered, this, [=]()
			{
				if (activeMdiChild())
				{
					activeMdiChild()->ExportModel();
					statusBar()->showMessage(tr("导出模型ing"), 2000);
				}
			});
			Mdi_ModelMenu->addAction(ExportModelAct);
		}
		{
			QAction* ClearModelAct = new QAction(tr("清理模型"), this);
			ClearModelAct->setStatusTip(tr("清理所有模型"));
			connect(ClearModelAct, &QAction::triggered, this, [=]()
				{
					if (activeMdiChild())
					{
						activeMdiChild()->clearModel();
						statusBar()->showMessage(tr("清理模型ing"), 2000);
					}
				});
			Mdi_ModelMenu->addAction(ClearModelAct);
		}
		{
			QAction* DeleteSelAct = new QAction(tr("清理选中"), this);
			DeleteSelAct->setStatusTip(tr("清理选中"));
			connect(DeleteSelAct, &QAction::triggered, this, [=]() 
				{
				if (activeMdiChild())
				{
					activeMdiChild()->DeleteSelObjects();
					statusBar()->showMessage(tr("清理选中ing"), 2000);
				}
				});
			Mdi_ModelMenu->addAction(DeleteSelAct);
		}
	}
	//选择
	{
		QMenu * Mdi_SelMenu = menuBar()->addMenu("选择");
		ChildMenus += Mdi_SelMenu;
		QAction * SelNaturalAct = new QAction(QIcon(ICON_SEL_SHAPE), tr("自然"), this);
		SelNaturalAct->setStatusTip(tr("选择自然模型"));
		connect(SelNaturalAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelNatural();
		});
		Mdi_SelMenu->addAction(SelNaturalAct);
		QAction * SelSolidAct = new QAction(QIcon(ICON_SEL_SOLID), tr("体"), this);
		SelSolidAct->setStatusTip(tr("选择模型——体"));
		connect(SelSolidAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelSolid();
		});
		Mdi_SelMenu->addAction(SelSolidAct);
		QAction * SelFaceAct = new QAction(QIcon(ICON_SEL_FACE), tr("面"), this);
		SelFaceAct->setStatusTip(tr("选择模型——面"));
		connect(SelFaceAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelFace();
		});
		Mdi_SelMenu->addAction(SelFaceAct);
		QAction* SelWireAct = new QAction(QIcon(ICON_SEL_EDGE), tr("线"), this);
		SelWireAct->setStatusTip(tr("选择模型——线"));
		connect(SelWireAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelWire();
			});
		Mdi_SelMenu->addAction(SelWireAct);
		QAction * SelEdgeAct = new QAction(QIcon(ICON_SEL_EDGE), tr("边"), this);
		SelEdgeAct->setStatusTip(tr("选择模型——边"));
		connect(SelEdgeAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelEdge();
		});
		Mdi_SelMenu->addAction(SelEdgeAct);
		QAction * SelVertexAct = new QAction(QIcon(ICON_SEL_VERTEX), tr("点"), this);
		SelVertexAct->setStatusTip(tr("选择模型——点"));
		connect(SelVertexAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelVertex();
		});
		Mdi_SelMenu->addAction(SelVertexAct);

	/*	QToolBar * Mdi_SelToolBar = this->addToolBar("拓扑选择工具栏");
		ChildToolBars += Mdi_SelToolBar;
		Mdi_SelToolBar->addAction(SelNaturalAct);
		Mdi_SelToolBar->addAction(SelSolidAct);
		Mdi_SelToolBar->addAction(SelFaceAct);
		Mdi_SelToolBar->addAction(SelEdgeAct);
		Mdi_SelToolBar->addAction(SelVertexAct);
		Mdi_SelToolBar->setIconSize(QSize(40, 40));*/
		
		//拓扑选择工具栏
		QToolBar* selectToolBar = this->addToolBar("拓扑选择工具栏");

		QToolButton* SelNaturalButton = new QToolButton(this);
		SelNaturalButton->setDefaultAction(SelNaturalAct);
		SelNaturalButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		SelNaturalButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 12pt;");
		selectToolBar->addWidget(SelNaturalButton);

		QToolButton* SelSolidButton = new QToolButton(this);
		SelSolidButton->setDefaultAction(SelSolidAct);
		SelSolidButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		SelSolidButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 12pt;");
		selectToolBar->addWidget(SelSolidButton);

		QToolButton* SelFaceButton = new QToolButton(this);
		SelFaceButton->setDefaultAction(SelFaceAct);
		SelFaceButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		SelFaceButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 12pt;");
		selectToolBar->addWidget(SelFaceButton);

		QToolButton* SelWireButton = new QToolButton(this);
		SelWireButton->setDefaultAction(SelWireAct);
		SelWireButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		SelWireButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 12pt;");
		selectToolBar->addWidget(SelWireButton);

		QToolButton* SelEdgeButton = new QToolButton(this);
		SelEdgeButton->setDefaultAction(SelEdgeAct);
		SelEdgeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		SelEdgeButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 12pt;");
		selectToolBar->addWidget(SelEdgeButton);

		QToolButton* SelVertexButton = new QToolButton(this);
		SelVertexButton->setDefaultAction(SelVertexAct);
		SelVertexButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);// 设置 QToolButton 的样式，使文字显示在图标下方
		SelVertexButton->setStyleSheet("font-family: \"微软雅黑\"; font-size: 12pt;");
		selectToolBar->addWidget(SelVertexButton);

		selectToolBar->setIconSize(QSize(40, 40));
		ChildToolBars += selectToolBar;


		//selectToolBar->addActions(activeMdiChild()->*q3dView->getViewActions());
	}
	//工具
	{
		QMenu* Mdi_CalcuMenu = menuBar()->addMenu("工具");
		ChildMenus += Mdi_CalcuMenu;
		//调用fortran计算
		QAction* CalcuAct = new QAction(tr("计算"), this);
		CalcuAct->setStatusTip(tr("调用fortran计算"));
		connect(CalcuAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->Calcu();
			}
			});
		//Mdi_CalcuMenu->addAction(CalcuAct);
		//Mdi_CalcuMenu->addSeparator();
		//面的偏置
		QAction* OffsetAct = new QAction(tr("偏置"), this);
		OffsetAct->setStatusTip(tr("选择表面进行偏置"));
		connect(OffsetAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->FaceOffset();
			}
			});
		Mdi_CalcuMenu->addAction(OffsetAct);
		//面的扩展
		QAction* ExtendAct = new QAction(tr("扩展"), this);
		ExtendAct->setStatusTip(tr("选择表面进行扩展"));
		connect(ExtendAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->FaceExtend();
			}
			});
		Mdi_CalcuMenu->addAction(ExtendAct);
		//移动模型
		QAction* MoveAct = new QAction(tr("移动"), this);
		MoveAct->setStatusTip(tr("选择模型进行移动"));
		connect(MoveAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->MoveModel();
			}
			});
		Mdi_CalcuMenu->addAction(MoveAct);
		//旋转模型
		QAction* RotateAct = new QAction(tr("旋转"), this);
		RotateAct->setStatusTip(tr("选择模型进行旋转"));
		connect(RotateAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->RotateModel();
			}
		});
		Mdi_CalcuMenu->addAction(RotateAct);
		//缩放模型
		QAction* ScaleAct = new QAction(tr("缩放"), this);
		ScaleAct->setStatusTip(tr("选择模型进行缩放"));
		connect(ScaleAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->ScaleModel();
			}
		});
		Mdi_CalcuMenu->addAction(ScaleAct);
		Mdi_CalcuMenu->addSeparator();
		//边的离散
		QAction* EdgeDiscreteAct = new QAction(tr("边离散"), this);
		EdgeDiscreteAct->setStatusTip(tr("选择边进行离散"));
		QAction* OutputDiscreteAct = new QAction(tr("输出离散点"), this);
		OutputDiscreteAct->setStatusTip(tr("保存离散点"));
		OutputDiscreteAct->setEnabled(false);
		connect(EdgeDiscreteAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->EdgeDiscrete();
				OutputDiscreteAct->setEnabled(true);
			}
			});
		Mdi_CalcuMenu->addAction(EdgeDiscreteAct);

		//输入材料参数
		QAction* InputMaterialAct = new QAction(tr("输入材料参数"), this);
		InputMaterialAct->setStatusTip(tr("输入工件材料参数（杨氏模量、泊松比、密度）"));
		connect(InputMaterialAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->InputMaterialProperty();
			}
			});
		Mdi_CalcuMenu->addAction(InputMaterialAct);

		//多边融合离散
		QAction* EdgeConnectDiscreteAct = new QAction(tr("多条边融合离散"), this);
		EdgeConnectDiscreteAct->setStatusTip(tr("选择边进行离散"));
		//QAction* OutputDiscreteAct = new QAction(tr("输出离散点"), this);
		OutputDiscreteAct->setStatusTip(tr("保存离散点"));
		OutputDiscreteAct->setEnabled(false);
		connect(EdgeConnectDiscreteAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->EdgeConnectDiscrete();
				OutputDiscreteAct->setEnabled(true);
			}
			});
		Mdi_CalcuMenu->addAction(EdgeConnectDiscreteAct);

		//输出离散点
		connect(OutputDiscreteAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->p_TreeWidget->savePoint();
			}
			});
		Mdi_CalcuMenu->addAction(OutputDiscreteAct);

		//清理离散点
		QAction* ClearDiscreteAct = new QAction(tr("清理离散点"), this);
		ClearDiscreteAct->setStatusTip(tr("清理离散点"));
		connect(ClearDiscreteAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->clearDiscretePoints();
			}
			});
		Mdi_CalcuMenu->addAction(ClearDiscreteAct);

		//计算角度
		QAction* CalculateFrontAngleAct = new QAction(tr("计算切削工况"), this);
		CalculateFrontAngleAct->setStatusTip(tr("计算切削工况"));
		QAction* OutputFrontAngleAct = new QAction(tr("输出切削工况"), this);
		OutputFrontAngleAct->setStatusTip(tr("输出切削工况"));
		OutputFrontAngleAct->setEnabled(false);
		connect(CalculateFrontAngleAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->GetCondition();
				OutputFrontAngleAct->setEnabled(true);//计算后才允许输出
			}
			});
		Mdi_CalcuMenu->addAction(CalculateFrontAngleAct);

		//输出角度
		connect(OutputFrontAngleAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->p_TreeWidget->saveAngle();
			}
			});
		Mdi_CalcuMenu->addAction(OutputFrontAngleAct);
		Mdi_CalcuMenu->addSeparator();

		// 输出abaqus INP文件
		QAction* OutputAbaqusAct = new QAction(tr("输出Abaqus"), this);
		OutputAbaqusAct->setStatusTip(tr("输出Abaqus求解文件"));
		connect(OutputAbaqusAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				activeMdiChild()->CaptureVertex();
			}
			});
		Mdi_CalcuMenu->addAction(OutputAbaqusAct);



	}
	//网格和边界条件
	{
		QMenu* Mdi_MeshMenu = menuBar()->addMenu("前处理");
		ChildMenus += Mdi_MeshMenu;
		{
			QAction* MeshAct = new QAction(tr("生成网格"), this);
			MeshAct->setStatusTip(tr("生成网格"));
			connect(MeshAct, &QAction::triggered, this, [=]()
				{
					if (activeMdiChild())
					{
						activeMdiChild()->GenerateMesh();
						statusBar()->showMessage(tr("生成网格ing"), 2000);
					}
				});
			Mdi_MeshMenu->addAction(MeshAct);

			QAction* SaveMeshAct = new QAction(tr("保存网格——还没实现"), this);
			SaveMeshAct->setStatusTip(tr("保存网格"));
			connect(SaveMeshAct, &QAction::triggered, this, [=]()
				{
					if (activeMdiChild())
					{
						activeMdiChild()->SaveMeshToFile();
						statusBar()->showMessage(tr("保存网格ing"), 2000);
					}
				});
			Mdi_MeshMenu->addAction(SaveMeshAct);

			QAction* SetBoundaryAct = new QAction(tr("设置边界条件"), this);
			SetBoundaryAct->setStatusTip(tr("设置边界条件"));
			connect(SetBoundaryAct, &QAction::triggered, this, [=]() {
				if (activeMdiChild()) {
					activeMdiChild()->SetBoundaryCondition();
					statusBar()->showMessage(tr("设置边界条件ing"), 2000);
				}
				});
			Mdi_MeshMenu->addAction(SetBoundaryAct);
		}

	}																				  
	//显示
	{
		QMenu * Mdi_AISMenu = menuBar()->addMenu("显示");
		ChildMenus += Mdi_AISMenu;
		QAction * DisplayAllAct = new QAction(QIcon(ICON_AIS_DISPLAYALL), tr("显示所有"), this);
		DisplayAllAct->setStatusTip(tr("显示所有"));
		connect(DisplayAllAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->AisObjDisplayAll();
		});
		Mdi_AISMenu->addAction(DisplayAllAct);
		QAction * EraseAllAct = new QAction(QIcon(ICON_AIS_ERASEALL), tr("隐藏所有"), this);
		EraseAllAct->setStatusTip(tr("隐藏所有"));
		connect(EraseAllAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->AisObjEraseAll();
		});
		Mdi_AISMenu->addAction(EraseAllAct);
		QAction * SetColorAct = new QAction(QIcon(ICON_AIS_OBJCOLOR), tr("设置对象颜色"), this);
		SetColorAct->setStatusTip(tr("设置对象颜色"));
		connect(SetColorAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelColor();
		});
		Mdi_AISMenu->addAction(SetColorAct);
		QAction * SetTransparencyAct = new QAction(QIcon(ICON_AIS_TRANSPARENCY), tr("设置透明度"), this);
		SetTransparencyAct->setStatusTip(tr("设置透明度"));
		connect(SetTransparencyAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelTrans();
		});
		Mdi_AISMenu->addAction(SetTransparencyAct);
		QAction * HideAct = new QAction(QIcon(ICON_AIS_ERASE), tr("隐藏"), this);
		HideAct->setStatusTip(tr("隐藏"));
		connect(HideAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->AisObjHide();
		});
		Mdi_AISMenu->addAction(HideAct);

		QAction * SelColorAct = new QAction(QIcon(ICON_AIS_SELCOLOR), tr("设置选择高亮颜色"), this);
		SelColorAct->setStatusTip(tr("设置选择高亮颜色"));
		//SelColorAct->setToolTip(tr("颜色"));
		connect(SelColorAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild())activeMdiChild()->SelColor();
		});
		Mdi_AISMenu->addAction(SelColorAct);


		// 颜色条显示
		QAction* ColorBarAct = new QAction(tr("颜色条显示"), this);
		ColorBarAct->setStatusTip(tr("显示/隐藏颜色条"));
		ColorBarAct->setCheckable(true);  // 设置为可切换状态
		ColorBarAct->setChecked(false);    // 默认为隐藏
		connect(ColorBarAct, &QAction::triggered, this, [=]() {
			if (activeMdiChild()) {
				bool visible = ColorBarAct->isChecked();
				
				// 如果显示颜色条，弹出对话框让用户输入参数
				if (visible) {
					QDialog dialog(this);
					dialog.setWindowTitle("颜色条设置");
					dialog.setMinimumSize(300, 150);
					
					QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
					QFormLayout* formLayout = new QFormLayout();
					
					// 标题输入框
					QLineEdit* titleEdit = new QLineEdit(&dialog);
					titleEdit->setPlaceholderText("输入颜色条标题");
					titleEdit->setText("Displacement");  // 默认标题
					formLayout->addRow("标题:", titleEdit);
					
					// 创建下拉框
					QComboBox* comboBox = new QComboBox(&dialog);

					// 添加项
					comboBox->addItem("U Magnitude");
					comboBox->addItem("Ux");
					comboBox->addItem("Uy");
					comboBox->addItem("Uz");
					formLayout->addRow("", comboBox);
					//comboBox->addItem("U Magnitude");
					//comboBox->addItem("Ux");
					//comboBox->addItem("Uy");
					//comboBox->addItem("Uz");
					

					// 最小值输入框
					QDoubleSpinBox* minSpinBox = new QDoubleSpinBox(&dialog);
					minSpinBox->setRange(-1e10, 1e10);
					minSpinBox->setDecimals(3);
					minSpinBox->setValue(0.0);
					formLayout->addRow("最小值:", minSpinBox);
					
					// 最大值输入框
					QDoubleSpinBox* maxSpinBox = new QDoubleSpinBox(&dialog);
					maxSpinBox->setRange(-1e10, 1e10);
					maxSpinBox->setDecimals(3);
					maxSpinBox->setValue(1.0);
					formLayout->addRow("最大值:", maxSpinBox);
					
					mainLayout->addLayout(formLayout);
					
					// 按钮
					QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
					connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
					connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
					mainLayout->addWidget(buttonBox);
					
					if (dialog.exec() == QDialog::Accepted) {
						QString title = titleEdit->text().trimmed();
						int currentIndex = comboBox->currentIndex();
						double minVal = minSpinBox->value();
						double maxVal = maxSpinBox->value();
						
						// 验证最大值和最小值
						if (minVal >= maxVal) {
							Msg::ShowWarning("最大值必须大于最小值！");
							ColorBarAct->setChecked(false);  // 取消选中
							return;
						}
						
						//// 如果标题为空，使用默认值
						//if (title.isEmpty()) {
						//	
						//}
						*(activeMdiChild()->visulization_item) = currentIndex;
						activeMdiChild()->visulization_limits[0] = minVal;
						activeMdiChild()->visulization_limits[1] = maxVal;
						switch (currentIndex){
							case 0:
								title = "U magnitude";
								break;
							case 1:
								title = "Ux";
								break;
							case 2:
								title = "Uy";
								break;
							case 3:
								title = "Uz";
								break;
							default:
								title = "Displacement";
						}
						// 设置颜色条
						activeMdiChild()->q3dView->setColorBarVisible(true, title, minVal, maxVal);
					} else {
						// 用户取消，恢复按钮状态
						ColorBarAct->setChecked(false);
					}
				} else {
					// 隐藏颜色条
					activeMdiChild()->q3dView->setColorBarVisible(false);
				}
			}
			});
		Mdi_AISMenu->addSeparator();
		Mdi_AISMenu->addAction(ColorBarAct);
	}
	//测试
	{
		QMenu * Mdi_TestMenu = menuBar()->addMenu("测试");
		ChildMenus += Mdi_TestMenu;
		{
			QAction * TestActT1 = new QAction(tr("测试-测试1"), this);
			connect(TestActT1, &QAction::triggered, this, [=]() {
				if (activeMdiChild())
				{
					activeMdiChild()->Test1();
					statusBar()->showMessage(tr("测试-测试1"), 2000);
				}
			});
			Mdi_TestMenu->addAction(TestActT1);

			QAction* TestActT2 = new QAction(tr("测试-测试2"), this);
			connect(TestActT2, &QAction::triggered, this, [=]() {
				if (activeMdiChild())
				{
					activeMdiChild()->Test2();
					statusBar()->showMessage(tr("测试-测试1"), 2000);
				}
				});
			Mdi_TestMenu->addAction(TestActT2);
		}
	}
	
}
