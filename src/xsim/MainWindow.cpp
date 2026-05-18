#include "mainwindow.h"
#include "mdichild.h"
#include "ICon.h"
#include "ComplainUtf8.h"
#include "Msg.h"
#include <QtWidgets>
#include "OcctView.h"
#include "AngleDialog.h"

MainWindow::MainWindow()
	:QMainWindow()
    //,mdiArea(new QMdiArea(this))
{
    mdiArea = new QMdiArea();
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setBackground(Qt::NoBrush);
	 //css图像尺寸不变
	mdiArea->setObjectName("mdiArea");
	mdiArea->setStyleSheet("#mdiArea{"
		"background-color:white;"
		"background-image:url(" IMG_BACK ");"
		"border:1px solid black;"    //黑色边框
		"background-repeat:no-repeat;"              //不重复
		"background-position:center;"
       // "background-size:1200px 1000px;"
		"}");

    setCentralWidget(mdiArea);
	createActions();
    createStatusBar();
    updateMenus();
    readSettings();
    setUnifiedTitleAndToolBarOnMac(true);
    setupUI();

	//把当前的文档窗口 设置为消息的焦点
	connect(mdiArea, &QMdiArea::subWindowActivated,
		this, [=](QMdiSubWindow * pMdiSubWindow)
	{
		if (pMdiSubWindow && pMdiSubWindow->widget())
			activeMdiChild()->SetCurrent();
		else
			Msg::RemoveCurrentMessager();
		updateMenus();
	});

	setWindowTitle(tr("XSim")); //标题名
	//this->createMdiChild(); 启动时自动创建子窗口
    this->newFile();
}

MainWindow::~MainWindow()
{
}

///根据是否有活动的子窗口，启用或禁用菜单栏
void MainWindow::updateMenus()
{
	bool hasMdiChild = (activeMdiChild() != 0);
	saveAct->setEnabled(hasMdiChild);
	saveAsAct->setEnabled(hasMdiChild);
	closeAct->setEnabled(hasMdiChild);
	closeAllAct->setEnabled(hasMdiChild);
	tileAct->setEnabled(hasMdiChild);
	cascadeAct->setEnabled(hasMdiChild);
	nextAct->setEnabled(hasMdiChild);
	previousAct->setEnabled(hasMdiChild);
	windowMenuSeparatorAct->setVisible(hasMdiChild);

	this->updateMDIMenus();
}

///更新 MDI 子窗口的视图菜单和工具栏。
void MainWindow::updateMDIMenus()
{
	bool hasMdiChild = (activeMdiChild() != nullptr);

    // 设置视图菜单的可见性
	Mdi_ViewMenu->menuAction()->setVisible(hasMdiChild);
	Mdi_ViewMenu->clear();

    // 如果有活动的 MDI 子窗口，添加子窗口的菜单动作
	if (hasMdiChild && activeMdiChild())
		Mdi_ViewMenu->addActions(activeMdiChild()->GetDockActions());
    for(auto pmenu : ChildMenus) 
        pmenu->menuAction()->setVisible(hasMdiChild);
    for(auto ptoolbar : ChildToolBars) 
        ptoolbar->setVisible(hasMdiChild);
}

///更新窗口菜单，显示所有打开的子窗口。
///每个子窗口对应一个菜单项，点击后可以切换到对应的子窗口。
void MainWindow::updateWindowMenu()
{
	windowMenu->clear();

	windowMenu->addAction(tileAct);
	windowMenu->addAction(cascadeAct);
	windowMenu->addSeparator();
	windowMenu->addAction(nextAct);
	windowMenu->addAction(previousAct);
	windowMenu->addAction(windowMenuSeparatorAct);

	windowMenu->addSeparator();
	windowMenu->addAction(tr("切换布局方向&S"), this, &MainWindow::switchLayoutDirection);
	windowMenu->addSeparator();

	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	windowMenuSeparatorAct->setVisible(!windows.isEmpty());

	for (int i = 0; i < windows.size(); ++i) {
		QMdiSubWindow *mdiSubWindow = windows.at(i);
		MdiChild *child = qobject_cast<MdiChild *>(mdiSubWindow->widget());

		QString text;
		if (i < 9) {
			text = tr("&%1 %2").arg(i + 1)
				.arg(child->userFriendlyCurrentFile());
		}
		else {
			text = tr("%1 %2").arg(i + 1)
				.arg(child->userFriendlyCurrentFile());
		}
		QAction *action = windowMenu->addAction(text, mdiSubWindow, [this, mdiSubWindow]() {
			mdiArea->setActiveSubWindow(mdiSubWindow);
		});
		action->setCheckable(true);
		action->setChecked(child == activeMdiChild());
	}
}

MdiChild *MainWindow::createMdiChild()
{
	MdiChild *child = new MdiChild(this);
	mdiArea->addSubWindow(child);
	child->showMaximized();  
	return child;
}

void MainWindow::createStatusBar()
{
    statusBar()->setStyleSheet(R"(
        QStatusBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #f8f9fa, stop:1 #e9ecef);
            color: #6c757d;
            border-top: 1px solid #dee2e6;
            font-size: 12px;
        }
    )");
	statusBar()->showMessage(tr("Ready"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        writeSettings();
        event->accept();
    }
}

void MainWindow::newFile()
{
    MdiChild *child = createMdiChild();
    child->newFile();
}

void MainWindow::open()
{
	QString Title = "选择导入的几何模型";
	QString Dir = ".";
	QString Filter = "My文件(*.myocc *.myocc1)"
		";;My文件(*.myocc2 *.myocc3)";

	QString fileName = QFileDialog::getOpenFileName(nullptr, Title, Dir, Filter);
    if (!fileName.isEmpty()) openFile(fileName);
}

bool MainWindow::openFile(const QString &fileName)
{
    if (QMdiSubWindow *existing = findMdiChild(fileName)) {
        mdiArea->setActiveSubWindow(existing);
        return true;
    }
    const bool succeeded = loadFile(fileName);
    if (succeeded)
        statusBar()->showMessage(tr("File loaded"), 2000);
    return succeeded;
}

bool MainWindow::loadFile(const QString &fileName)
{
    MdiChild *child = createMdiChild();
    const bool succeeded = child->loadFile(fileName);
	if (succeeded)
	{
		child->show();
		MainWindow::prependToRecentFiles(fileName);
	}
	else
	{
		child->close();
		mdiArea->closeActiveSubWindow();
		MainWindow::prependToRecentFiles(fileName,false);
	}
      
    
    return succeeded;
}

static inline QString recentFilesKey() { return QStringLiteral("recentFileList"); }
static inline QString fileKey() { return QStringLiteral("file"); }
static QStringList readRecentFiles(QSettings &settings)
{
    QStringList result;
    const int count = settings.beginReadArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        result.append(settings.value(fileKey()).toString());
    }
    settings.endArray();
    return result;
}
static void writeRecentFiles(const QStringList &files, QSettings &settings)
{
    const int count = files.size();
    settings.beginWriteArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        settings.setValue(fileKey(), files.at(i));
    }
    settings.endArray();
}

bool MainWindow::hasRecentFiles()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const int count = settings.beginReadArray(recentFilesKey());
    settings.endArray();
    return count > 0;
}

void MainWindow::prependToRecentFiles(const QString &fileName,bool Add)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    const QStringList oldRecentFiles = readRecentFiles(settings);
    QStringList recentFiles = oldRecentFiles;
    recentFiles.removeAll(fileName);
	if(Add)
		recentFiles.prepend(fileName);
    if (oldRecentFiles != recentFiles)
        writeRecentFiles(recentFiles, settings);

    setRecentFilesVisible(!recentFiles.isEmpty());
}

void MainWindow::setRecentFilesVisible(bool visible)
{
    recentFileSubMenuAct->setVisible(visible);
    recentFileSeparator->setVisible(visible);
}

///更新最近文件菜单栏，显示所有最近打开的文件
void MainWindow::updateRecentFileActions()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    const QStringList recentFiles = readRecentFiles(settings);
    const int count = qMin(int(MaxRecentFiles), recentFiles.size());
    int i = 0;
    for ( ; i < count; ++i) {
        const QString fileName = QFileInfo(recentFiles.at(i)).fileName();
        recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
        recentFileActs[i]->setData(recentFiles.at(i));
        recentFileActs[i]->setVisible(true);
    }
    for ( ; i < MaxRecentFiles; ++i)
        recentFileActs[i]->setVisible(false);
}

///打开最近文件菜单项对应的文件
void MainWindow::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction *>(sender()))
        openFile(action->data().toString());
}

void MainWindow::save()
{
    if (activeMdiChild() && activeMdiChild()->save())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveAs()
{
    MdiChild *child = activeMdiChild();
    if (child && child->saveAs()) {
        statusBar()->showMessage(tr("File saved"), 2000);
        MainWindow::prependToRecentFiles(child->currentFile());
    }
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("关于"),
            tr("<b>XSim</b> "
               "\t\t<b>作者：华中科技大学 张东，电话：18971488706"));
}

void MainWindow::readSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

MdiChild *MainWindow::activeMdiChild() const
{
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<MdiChild *>(activeSubWindow->widget());
    return nullptr;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName) const
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
        if (mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return 0;
}

void MainWindow::switchLayoutDirection()
{
    if (layoutDirection() == Qt::LeftToRight)
        QGuiApplication::setLayoutDirection(Qt::RightToLeft);
    else
        QGuiApplication::setLayoutDirection(Qt::LeftToRight);
}

void MainWindow::setupUI()
{
    // 现代深色主题配色方案
    QString styleSheet = R"(
        /* 主窗口样式 */
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                      stop:0 #f8f9fa, stop:1 #e9ecef);
            color: #212529;
            font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
            font-size: 13px;
        }

        /* 菜单栏样式 */
        QMenuBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #ffffff, stop:1 #f8f9fa);
            color: #495057;
            border: none;
            border-bottom: 1px solid #dee2e6;
            padding: 4px 8px;
            font-weight: 500;
            font-size: 13px;
        }

        QMenuBar::item {
            background: transparent;
            padding: 6px 12px;
            border-radius: 4px;
            margin: 1px 2px;
            color: #495057;
        }

        QMenuBar::item:selected {
            background: #e7f1ff;
            color: #0d6efd;
        }

        QMenuBar::item:pressed {
            background: #0d6efd;
            color: white;
        }

        QMenu {
            background: white;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            padding: 4px;
            color: #495057;
        }

        QMenu::item {
            padding: 6px 24px 6px 32px;
            border-radius: 4px;
            margin: 2px;
        }

        QMenu::item:selected {
            background: #e7f1ff;
            color: #0d6efd;
        }

        QMenu::separator {
            height: 1px;
            background: #dee2e6;
            margin: 4px 8px;
        }

        /* 工具栏样式 */
        QToolBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #ffffff, stop:1 #f8f9fa);
            border: none;
            border-bottom: 1px solid #dee2e6;
            spacing: 2px;
            padding: 4px 8px;
        }

        //QToolBar QToolButton {
        //    background: white;
        //    border: 1px solid #dee2e6;
        //    border-radius: 6px;
        //    padding: 4px 6px;
        //    color: #495057;
        //    min-width: 40px;
        //    font-size: 12px;
        //}

        QToolBar QToolButton:hover {
            background: #f8f9fa;
            border: 1px solid #0d6efd;
            color: #0d6efd;
        }

        QToolBar QToolButton:pressed {
            background: #0d6efd;
            border: 1px solid #0a58ca;
            color: white;
        }

        QToolBar QToolButton:checked {
            background: #0d6efd;
            border: 1px solid #0a58ca;
            color: white;
        }

        QToolBar QToolButton::menu-indicator {
            image: none;
            subcontrol-origin: padding;
            subcontrol-position: bottom right;
        }

        /* 停靠窗口样式 */
        QDockWidget {
            background: white;
            color: #212529;//颜色为深灰色
            border: 1px solid #dee2e6;
            border-radius: 8px;
            margin: 2px;
            font-size: 12px;
        }

        QDockWidget::title {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #0d6efd, stop:1 #0a58ca);
            padding: 8px 12px;
            border-radius: 6px 6px 0 0;
            text-align: center;
            color: white;
            font-weight: bold;
            font-size: 12px;
        }

        QDockWidget::close-button, QDockWidget::float-button {
            background: transparent;
            border: none;
            padding: 2px;
            color: white;
        }

        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background: rgba(255,255,255,0.2);
            border-radius: 3px;
        }

        /* 树形视图样式 */
        QTreeView {
            background: white;
            color: #212529;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            outline: none;
            font-size: 12px;
        }

        QTreeView::item {
            padding: 6px 8px;
            border-bottom: 1px solid #f8f9fa;
        }

        QTreeView::item:selected {
            background: #e7f1ff;
            color: #0d6efd;
            border-radius: 4px;
            border: none;
        }

        QTreeView::item:hover {
            background: #f8f9fa;
        }

        QTreeView::branch:has-siblings:!adjoins-item {
            border-image: url(vline.png) 0;
        }

        QTreeView::branch:has-siblings:adjoins-item {
            border-image: url(branch-more.png) 0;
        }

        QTreeView::branch:!has-children:!has-siblings:adjoins-item {
            border-image: url(branch-end.png) 0;
        }

        QTreeView::branch:has-children:!has-siblings:closed,
        QTreeView::branch:closed:has-children:has-siblings {
            border-image: none;
            image: url(right-arrow.png);
        }

        QTreeView::branch:open:has-children:!has-siblings,
        QTreeView::branch:open:has-children:has-siblings {
            border-image: none;
            image: url(down-arrow.png);
        }

        /* 属性栏样式 */
        QGroupBox {
            background: white;
            color: #212529;
            border: 1px solid #dee2e6;
            border-radius: 8px;
            margin-top: 1ex;
            padding-top: 10px;
            font-weight: 600;
            font-size: 12px;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top center;
            padding: 0 12px;
            background: #6c757d;
            color: white;
            border-radius: 4px;
            margin-top: -2ex;
        }

        QLabel {
            color: #495057;
            font-size: 12px;
            padding: 2px 0px;
        }

        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background: white;
            border: 1px solid #ced4da;
            border-radius: 4px;
            padding: 6px 8px;
            color: #212529;
            font-size: 12px;
            min-height: 20px;
        }

        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid #0d6efd;
            outline: none;
        }

        QComboBox::drop-down {
            border: none;
            width: 20px;
        }

        QComboBox::down-arrow {
            image: url(down-arrow.png);
            width: 12px;
            height: 12px;
        }

        /* 状态栏样式 */
        QStatusBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #f8f9fa, stop:1 #e9ecef);
            color: #000000;//浅灰色文字
            border-top: 1px solid #dee2e6;
            font-size: 12px;
        }

        QStatusBar::item {
            border: none;
        }

        /* 3D视图区域样式 */
        #mdiArea {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                      stop:0 #ffffff, stop:1 #f8f9fa);
            border: 1px solid #dee2e6;
            border-radius: 4px;
        }

        QMdiArea {
            background: #f8f9fa;
        }

        QMdiSubWindow {
            background: white;
            border: 1px solid #dee2e6;
            border-radius: 4px;
        }

        QMdiSubWindow::title {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                      stop:0 #0d6efd, stop:1 #0a58ca);
            text-align: center;
            color: white;
            font-weight: bold;
            padding: 4px;
        }

        /* 滚动条样式 */
        QScrollBar:vertical {
            background: #f8f9fa;
            width: 12px;
            margin: 0px;
            border-radius: 6px;
        }

        QScrollBar::handle:vertical {
            background: #ced4da;
            border-radius: 6px;
            min-height: 20px;
        }

        QScrollBar::handle:vertical:hover {
            background: #adb5bd;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
        }

        /* 标签页样式 */
        QTabWidget::pane {
            border: 1px solid #dee2e6;
            border-radius: 6px;
            background: white;
        }

        QTabWidget::tab-bar {
            alignment: center;
        }

        QTabBar::tab {
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            padding: 6px 12px;
            margin-right: 2px;
            color: #6c757d;
        }

        QTabBar::tab:selected {
            background: white;
            border-color: #dee2e6;
            border-bottom-color: white;
            color: #0d6efd;
            font-weight: bold;
        }

        QTabBar::tab:hover:!selected {
            background: #e9ecef;
            color: #495057;
        }

        /* 按钮样式 */
        QPushButton {
            background: black;
            border: 1px solid #ced4da;
            border-radius: 4px;
            padding: 6px 12px;
            color: #212529;//深灰色文字
            font-size: 12px;
            font-weight: 500;
        }

        QPushButton:hover {
            background: #f8f9fa;
            border-color: #0d6efd;
            color: #0d6efd;
        }

        QPushButton:pressed {
            background: #0d6efd;
            border-color: #0a58ca;
            color: white;
        }

        QPushButton:checked {
            background: #0d6efd;
            border-color: #0a58ca;
            color: white;
        }

        /* 分隔符样式 */
        QSplitter::handle {
            background: #dee2e6;
            margin: 1px;
        }

        QSplitter::handle:hover {
            background: #adb5bd;
        }

        QSplitter::handle:horizontal {
            width: 3px;
        }

        QSplitter::handle:vertical {
            height: 3px;
        }
    )";


    // 设置窗口属性
    //setWindowIcon(QIcon(":/icons/xsim.png"));
    setWindowTitle("XSim");

    // 应用现代UI改进
    //setupModernToolbars();
    //setupEnhancedProjectTree();
    //setupEnhancedPropertyPanel();
    //setupEnhancedStatusBar(); 
    this->setStyleSheet(styleSheet);
}

//void MainWindow::setupModernToolbars()
//{
//    // 清除现有工具栏
//    //clearToolbars();
//
//    // 主工具栏 - 水平布局
//    QToolBar* mainToolbar = addToolBar(tr("主工具栏"));
//    mainToolbar->setObjectName("mainToolbar");
//    mainToolbar->setIconSize(QSize(20, 20));
//    mainToolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    mainToolbar->setMovable(false);
//
//    // 文件操作组
//    addToolbarAction(mainToolbar, newAct, "新建", "📄");
//    addToolbarAction(mainToolbar, openAct, "打开", "📁");
//    addToolbarAction(mainToolbar, saveAct, "保存", "💾");
//    mainToolbar->addSeparator();
//
//    //// 视图操作组
//    //addToolbarAction(mainToolbar, zoomAllAct, "全图", "🔍");
//    //addToolbarAction(mainToolbar, zoomWindowAct, "窗口缩放", "🔍");
//    //addToolbarAction(mainToolbar, panAct, "平移", "✋");
//    //addToolbarAction(mainToolbar, rotateAct, "旋转", "🔄");
//    //mainToolbar->addSeparator();
//
//    //// 选择模式组
//    //QToolButton* selectModeBtn = createToolButton("选择模式", "⬛");
//    //QMenu* selectMenu = new QMenu(this);
//    //selectMenu->addAction(m_selectSolidAct);
//    //selectMenu->addAction(m_selectFaceAct);
//    //selectMenu->addAction(m_selectEdgeAct);
//    //selectMenu->addAction(m_selectVertexAct);
//    //selectModeBtn->setMenu(selectMenu);
//    //selectModeBtn->setPopupMode(QToolButton::InstantPopup);
//    //mainToolbar->addWidget(selectModeBtn);
//    //mainToolbar->addSeparator();
//
//    //// 分析工具组
//    //addToolbarAction(mainToolbar, m_cuttingAnalysisAct, "切削分析", "⚙️");
//    //addToolbarAction(mainToolbar, m_meshGenerateAct, "生成网格", "🔲");
//    //addToolbarAction(mainToolbar, m_exportAbaqusAct, "导出Abaqus", "📤");
//
//    // 垂直工具栏 - 左侧工具
//    QToolBar* leftToolbar = new QToolBar(tr("建模工具"));
//    leftToolbar->setObjectName("leftToolbar");
//    leftToolbar->setIconSize(QSize(24, 24));
//    leftToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
//    leftToolbar->setOrientation(Qt::Vertical);
//
//    //// 建模工具
//    //addToolbarAction(leftToolbar, m_createBoxAct, "", "⬛");
//    //addToolbarAction(leftToolbar, m_createCylinderAct, "", "⭕");
//    //addToolbarAction(leftToolbar, m_createSphereAct, "", "🔴");
//    //leftToolbar->addSeparator();
//    //addToolbarAction(leftToolbar, m_extrudeAct, "", "📏");
//    //addToolbarAction(leftToolbar, m_revolveAct, "", "🔄");
//
//    addToolBar(Qt::LeftToolBarArea, leftToolbar);
//}

//void MainWindow::addToolbarAction(QToolBar* toolbar, QAction* action, const QString& text, const QString& iconText)
//{
//    if (!action) return;
//
//    // 设置工具按钮样式
//    QToolButton* button = new QToolButton;
//    button->setDefaultAction(action);
//    button->setToolButtonStyle(toolbar->toolButtonStyle());
//    button->setText(text);
//
//    // 使用文本作为临时图标（实际项目中应使用真实图标）
//    QFont font;
//    font.setPointSize(14);
//    QPixmap pixmap(24, 24);
//    pixmap.fill(Qt::transparent);
//    QPainter painter(&pixmap);
//    painter.setFont(font);
//    painter.drawText(pixmap.rect(), Qt::AlignCenter, iconText);
//    action->setIcon(QIcon(pixmap));
//
//    toolbar->addWidget(button);
//}

//void MainWindow::setupEnhancedProjectTree()
//{
//    // 项目树停靠窗口
//    QDockWidget* projectDock = new QDockWidget(tr("项目管理器"), this);
//    projectDock->setObjectName("projectDock");
//    projectDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
//    projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//
//    QWidget* projectWidget = new QWidget;
//    QVBoxLayout* projectLayout = new QVBoxLayout(projectWidget);
//    projectLayout->setContentsMargins(8, 8, 8, 8);
//    projectLayout->setSpacing(8);
//
//    // 搜索框
//    QLineEdit* searchBox = new QLineEdit;
//    searchBox->setPlaceholderText(tr("搜索模型..."));
//    searchBox->setClearButtonEnabled(true);
//    searchBox->setStyleSheet(R"(
//        QLineEdit {
//            background: white;
//            border: 1px solid #ced4da;
//            border-radius: 4px;
//            padding: 6px 8px;
//            font-size: 12px;
//        }
//        QLineEdit:focus {
//            border: 1px solid #0d6efd;
//        }
//    )");
//
//    // 项目树
//    m_projectTree = new QTreeWidget;
//    m_projectTree->setHeaderLabels(QStringList() << tr("名称") << tr("类型") << tr("状态"));
//    m_projectTree->setColumnCount(3);
//    m_projectTree->setAlternatingRowColors(true);
//    m_projectTree->setAnimated(true);
//    m_projectTree->setIndentation(12);
//
//    // 设置列宽
//    m_projectTree->setColumnWidth(0, 120);
//    m_projectTree->setColumnWidth(1, 80);
//    m_projectTree->setColumnWidth(2, 60);
//
//    projectLayout->addWidget(new QLabel(tr("项目结构")));
//    projectLayout->addWidget(searchBox);
//    projectLayout->addWidget(m_projectTree);
//
//    projectDock->setWidget(projectWidget);
//    addDockWidget(Qt::LeftDockWidgetArea, projectDock);
//}

//void MainWindow::setupEnhancedPropertyPanel()
//{
//    QDockWidget* propertyDock = new QDockWidget(tr("属性面板"), this);
//    propertyDock->setObjectName("propertyDock");
//    propertyDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
//    propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//
//    QScrollArea* scrollArea = new QScrollArea;
//    scrollArea->setWidgetResizable(true);
//    scrollArea->setFrameShape(QFrame::NoFrame);
//
//    QWidget* propertyWidget = new QWidget;
//    QVBoxLayout* propertyLayout = new QVBoxLayout(propertyWidget);
//    propertyLayout->setContentsMargins(8, 8, 8, 8);
//    propertyLayout->setSpacing(12);
//
//    // 几何属性组
//    QGroupBox* geometryGroup = createPropertyGroup(tr("几何属性"));
//    QFormLayout* geometryLayout = new QFormLayout(geometryGroup);
//    geometryLayout->setSpacing(6);
//
//    geometryLayout->addRow(tr("体积:"), createValueLabel("1250.5 mm³"));
//    geometryLayout->addRow(tr("表面积:"), createValueLabel("856.3 mm²"));
//    geometryLayout->addRow(tr("边界框:"), createValueLabel("100×50×25 mm"));
//    geometryLayout->addRow(tr("质心:"), createValueLabel("X:50.0 Y:25.0 Z:12.5"));
//
//    // 材料属性组
//    QGroupBox* materialGroup = createPropertyGroup(tr("材料属性"));
//    QFormLayout* materialLayout = new QFormLayout(materialGroup);
//    materialLayout->setSpacing(6);
//
//    QComboBox* materialCombo = new QComboBox;
//    materialCombo->addItems(QStringList() << "45号钢" << "铝合金" << "不锈钢" << "钛合金");
//    materialLayout->addRow(tr("材料:"), materialCombo);
//    materialLayout->addRow(tr("密度:"), createValueLabel("7.85 g/cm³"));
//    materialLayout->addRow(tr("弹性模量:"), createValueLabel("210 GPa"));
//    materialLayout->addRow(tr("泊松比:"), createValueLabel("0.3"));
//
//    propertyLayout->addWidget(geometryGroup);
//    propertyLayout->addWidget(materialGroup);
//    propertyLayout->addStretch();
//
//    scrollArea->setWidget(propertyWidget);
//    propertyDock->setWidget(scrollArea);
//    addDockWidget(Qt::RightDockWidgetArea, propertyDock);
//}

//QGroupBox* MainWindow::createPropertyGroup(const QString& title)
//{
//    QGroupBox* group = new QGroupBox(title);
//    group->setStyleSheet(R"(
//        QGroupBox {
//            background: white;
//            border: 1px solid #dee2e6;
//            border-radius: 6px;
//            margin-top: 1ex;
//            padding-top: 10px;
//            font-weight: 600;
//            font-size: 12px;
//        }
//        QGroupBox::title {
//            subcontrol-origin: margin;
//            subcontrol-position: top center;
//            padding: 0 8px;
//            background: #6c757d;
//            color: white;
//            border-radius: 4px;
//        }
//    )");
//    return group;
//}

//QLabel* MainWindow::createValueLabel(const QString& text)
//{
//    QLabel* label = new QLabel(text);
//    label->setStyleSheet(R"(
//        QLabel {
//            background: #f8f9fa;
//            border: 1px solid #dee2e6;
//            border-radius: 4px;
//            padding: 4px 8px;
//            color: #495057;
//            font-size: 12px;
//        }
//    )");
//    label->setMinimumHeight(24);
//    return label;
//}

