#include "mdichild.h"
#include "ComplainUtf8.h"
#include "OcctView.h"
#include "GUI_Message.h"
#include <QtWidgets>
#include "PropertyView.h"
#include "ProjectTree.h"

void MdiChild::Init()
{
    // 设置主窗口背景和样式
    this->setStyleSheet(R"(
        QMainWindow {
            background: #f8f9fa;
            border: none;
        }
    )");

    // 启用停靠窗口嵌套
    this->setDockNestingEnabled(true);

    QWidget* centralContainer = new QWidget(this);
    QHBoxLayout* centralLayout = new QHBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(2); // 两个视图之间的缝隙
    centralContainer->setLayout(centralLayout);
    this->setCentralWidget(centralContainer); // 总容器成为中心

    // 3D视图区域 - 主工作区
    {
        QWidget* p3DWidget = new QWidget(this);
		QVBoxLayout* pLayout = new QVBoxLayout(p3DWidget);//水平布局管理器
        pLayout->setContentsMargins(0, 0, 0, 0);
		pLayout->setSpacing(0);//设置布局边距为0
        p3DWidget->setLayout(pLayout);

        // 3D视图
        q3dView = new OcctView(h_MyViewer->getAisContext(), this);
        connect(q3dView, &OcctView::selectionChanged, this, &MdiChild::selectionChanged);

        
        // 工具栏 
        QToolBar* pToolbar = new QToolBar(p3DWidget);
		pToolbar->setOrientation(Qt::Horizontal);//设置方向为垂直方向
        //pToolbar->setIconSize(QSize(10, 10));
        //pToolbar->setFixedWidth(40); // 固定宽度，更紧凑
        pToolbar->setMovable(true);

        // 设置工具栏样式
        pToolbar->setStyleSheet(R"(
            QToolBar {
                //background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                //                          stop:0 #f8f9fa, stop:1 #e9ecef);
             background:#d0d7ff;  /*  工具栏背景色：淡蓝色（十六进制色值 #d0d7ff） */
                border: none;  /*  取消工具栏所有默认边框（上、下、左、右均无边框） */
                border-left: 1px solid #dee2e6;  /* 单独设置左边框：1像素宽、实线、淡灰色（#dee2e6） */
                spacing: 2px;  /* 工具栏内子控件（如工具按钮）的间距：2像素 */
                padding: 1px;  /* 工具栏内边距：控件内容（子按钮）到工具栏边框的距离为1像素 */
            }
            QToolButton {
                fixed-size: 10px 10px; 
                background: white;  /* 按钮默认背景色：白色 */
                border: 1px solid #dee2e6;  /* 按钮默认边框：1像素宽、实线、淡灰色（#dee2e6） */
                //border-radius: 2px;  /* 按钮边框圆角：半径4像素（让直角按钮变圆润） */
                //padding: 1px;  /* 按钮内边距：按钮图标/文字到按钮边框的距离为2像素 */
                //margin: 1px;  /* 按钮外边距：按钮与其他控件（或工具栏边框）的距离为1像素 */
            }
            QToolButton:hover {
                background: #e7f1ff;//悬浮时背景颜色
                border: 1px solid #0d6efd;//悬浮时边框颜色
            }
            QToolButton:pressed {
                background: #0d6efd;
                color: white;//按下时文字颜色：白色（若按钮有文字，文字会变白）
            }
        )");

        // 添加视图操作
        pToolbar->addActions(*q3dView->getViewActions());
        pToolbar->addSeparator();
        pToolbar->addActions(*q3dView->getRaytraceActions());
        pLayout->addWidget(q3dView);
        pLayout->addWidget(pToolbar);

        //this->setCentralWidget(p3DWidget);
        centralLayout->addWidget(p3DWidget);
   
        //QWidget* p3DWidget = new QWidget(this);
        //QHBoxLayout* pLayout = new QHBoxLayout();//水平布局管理器
        //pLayout->setContentsMargins(0, 0, 0, 0);//设置布局边距为0
        //p3DWidget->setLayout(pLayout);

        //q3dView = new OcctView(h_MyViewer->getAisContext(), this);//------OCC视图区域
        //connect(q3dView, &OcctView::selectionChanged, this, &MdiChild::selectionChanged);//连接信号槽

        //pLayout->addWidget(q3dView);

        //QToolBar* pToolbar = new QToolBar(p3DWidget);//------ 创建侧边工具栏
        //pToolbar->setOrientation(Qt::Vertical);//设置方向为垂直方向
        //pToolbar->setIconSize(QSize(20, 20));//设置图标大小
        //pToolbar->addActions(*q3dView->getViewActions());//添加视图相关动作
        //pToolbar->addActions(*q3dView->getRaytraceActions());//添加光线追踪等相关动作
        //pToolbar->setStyleSheet("QToolBar {padding: 4px 8px}");//浅绿色
        //pLayout->addWidget(pToolbar);//使其与q3dview并排显示
        //this->setCentralWidget(p3DWidget);
    }
    
    {
        VTKWidget* p_VtkWidget = new VTKWidget(this);
        this->p_VtkWidget = p_VtkWidget;//将局部变量赋值给 Mdichi1d的成员变量

        p_VtkWidget->setMinimumSize(600, 560);
        p_VtkWidget->setInfoOverlayVisible(false);
        centralLayout->addWidget(p_VtkWidget);
        centralLayout->setStretch(1, 1);
        // 设置比例
        centralLayout->setStretch(0, 1); // 左侧占1份
        centralLayout->setStretch(1, 1); // 右侧占1份
    }

    //  项目树 - 左侧停靠窗口
    {
        p_TreeDock = new QDockWidget(tr("项目管理器"), this);
        p_TreeWidget = new ProjectTree(p_TreeDock);
        p_TreeWidget->mdiChild = this; // 设置mdiChild指针

        // 设置项目树停靠窗口样式
        p_TreeDock->setWidget(p_TreeWidget);
        p_TreeDock->setFeatures(QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetFloatable);
        p_TreeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p_TreeDock->setMinimumWidth(250); // 根据图片调整宽度

        // 设置停靠窗口样式
        p_TreeDock->setStyleSheet(R"(
            QDockWidget {
                background: white;
                border: 1px solid #dee2e6;//颜色为浅灰色
                border-radius: 4px;
                color: #495057;//深灰色文字
                titlebar-close-icon: url(close.png);
                titlebar-normal-icon: url(float.png);
            }
            QDockWidget::title {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                          stop:0 #0d6efd, stop:1 #0a58ca);
                padding: 8px;
                text-align: center;
                color: white;
                font-weight: bold;
                border-radius: 3px;
            }
        )");

        connect(p_TreeWidget, &ProjectTree::modelDoubleClicked,
            q3dView, &OcctView::OnTreeItemDoubleClicked);

        this->addDockWidget(Qt::LeftDockWidgetArea, p_TreeDock);
    }

    //属性栏 - 左侧停靠窗口，位于项目树下方
    {
        p_PropertyDock = new QDockWidget(tr("属性栏"), this);
        p_PropertyWidget = new PropertyView(p_PropertyDock);

        p_PropertyDock->setWidget(p_PropertyWidget);
        p_PropertyDock->setFeatures(QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetFloatable);
        p_PropertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p_PropertyDock->setMinimumHeight(200);

        // 设置与项目树相同的样式
        p_PropertyDock->setStyleSheet(p_TreeDock->styleSheet());

        q3dView->setPropertyView(p_PropertyWidget);

        // 将属性栏停靠在项目树下方
        //this->splitDockWidget(p_TreeDock, p_PropertyDock, Qt::Vertical);
        this->splitDockWidget(p_TreeDock, p_PropertyDock, Qt::Vertical);
    }

    // 状态栏 - 底部停靠窗口
    {
        p_MessageDock = new QDockWidget(tr("状态栏"), this);
        p_MessageWidget = new GUI_Message(p_MessageDock);

        p_MessageDock->setWidget(p_MessageWidget);
        p_MessageDock->setFeatures(QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetFloatable);
        p_MessageDock->setAllowedAreas(Qt::BottomDockWidgetArea);
        p_MessageDock->setMinimumHeight(200);

        // 设置状态栏样式
        p_MessageDock->setStyleSheet(R"(
            QDockWidget {
                //background: #f8f9fa;//浅灰色背景
                border: 1px solid #dee2e6;
                border-radius: 4px;
            }
            QDockWidget::title {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                          stop:0 #6c757d, stop:1 #495057);
                padding: 6px;
                text-align: center;
                color: white;
                font-weight: bold;
                border-radius: 3px;
            }
        )");

        this->addDockWidget(Qt::BottomDockWidgetArea, p_MessageDock);
    }

    //  切削力 - 右侧停靠窗口
    {
        p_forceVisualDock = new QDockWidget(tr("拉削力"), this);
        forcewidget = new ForceMonitorWidget();
        p_forceVisualDock->setWidget(forcewidget);
        //p_TreeWidget = new ProjectTree(p_TreeDock);
        //p_TreeWidget->mdiChild = this; // 设置mdiChild指针

        //// 设置项目树停靠窗口样式
        //p_TreeDock->setWidget(p_TreeWidget);
        p_forceVisualDock->setFeatures(QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetFloatable);
        p_forceVisualDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p_forceVisualDock->setMinimumWidth(400); // 根据图片调整宽度

        // 设置停靠窗口样式
        p_forceVisualDock->setStyleSheet(R"(
            qdockwidget {
                background: white;
                border: 1px solid #dee2e6;//颜色为浅灰色
                border-radius: 4px;
                color: #495057;//深灰色文字
                titlebar-close-icon: url(close.png);
                titlebar-normal-icon: url(float.png);
            }
            qdockwidget::title {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                          stop:0 #0d6efd, stop:1 #0a58ca);
                padding: 8px;
                text-align: center;
                color: white;
                font-weight: bold;
                border-radius: 3px;
            }
        )");
        this->addDockWidget(Qt::RightDockWidgetArea, p_forceVisualDock);
    }

    {
        p_VtkVisualDock = new QDockWidget(QString::fromUtf8("\xE6\x9C\xBA\xE5\xBA\x8A\xE9\x80\x9A\xE8\xAE\xAF\xE4\xBF\xA1\xE6\x81\xAF"), this);

        QWidget* commWidget = new QWidget(p_VtkVisualDock);
        QVBoxLayout* commLayout = new QVBoxLayout(commWidget);
        commLayout->setContentsMargins(12, 12, 12, 12);
        commLayout->setSpacing(10);

        auto setupValueLabel = [](QLabel* label) {
            label->setMinimumWidth(120);
            label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            label->setText("--");
        };

        auto addRow = [&](QGridLayout* layout, int row, const QString& name, QLabel*& valueLabel) {
            QLabel* nameLabel = new QLabel(name, commWidget);
            nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            valueLabel = new QLabel(commWidget);
            setupValueLabel(valueLabel);
            layout->addWidget(nameLabel, row, 0);
            layout->addWidget(valueLabel, row, 1);
        };

        QGroupBox* mcsGroup = new QGroupBox("MCS", commWidget);
        QGridLayout* mcsLayout = new QGridLayout(mcsGroup);
        addRow(mcsLayout, 0, "X1", m_commPosXLabel);
        addRow(mcsLayout, 1, "Y1", m_commPosYLabel);
        addRow(mcsLayout, 2, "Z1", m_commPosZLabel);
        addRow(mcsLayout, 3, "C1", m_commPosCLabel);
        addRow(mcsLayout, 4, "A1", m_commPosALabel);
        commLayout->addWidget(mcsGroup);

        QGroupBox* toolGroup = new QGroupBox("TOOL", commWidget);
        QGridLayout* toolLayout = new QGridLayout(toolGroup);
        addRow(toolLayout, 0, "Tool", m_commToolLabel);
        m_commToolLabel->setText("0");

        addRow(toolLayout, 1, "F [mm/min]", m_commFeedLabel);
        m_commFeedLabel->setText("200");

        addRow(toolLayout, 2, "S [rpm]", m_commRpmLabel);
        m_commRpmLabel->setText("3000");
        commLayout->addWidget(toolGroup);

        QGroupBox* serialGroup = new QGroupBox("Serial Port", commWidget);
        QGridLayout* serialLayout = new QGridLayout(serialGroup);
        addRow(serialLayout, 0, "Ft/N", m_commForce1Label);
        m_commForce1Label->setText("100.000");
        addRow(serialLayout, 1, "Fr/N", m_commForce2Label);
        m_commForce2Label->setText("100.000");
        addRow(serialLayout, 2, "Fa/N", m_commForce3Label);
        m_commForce3Label->setText("0.000");
        QLabel* channelLabel = new QLabel("Channel", serialGroup);
        m_commChannelBox = new QComboBox(serialGroup);
        m_commChannelBox->addItems({ "force_1", "force_2", "force_3", "feed" });
        m_commChannelBox->setCurrentIndex(3);
        serialLayout->addWidget(channelLabel, 3, 0);
        serialLayout->addWidget(m_commChannelBox, 3, 1);
        connect(m_commChannelBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            p_VtkWidget, &VTKWidget::setColorChannelIndex);
        p_VtkWidget->setColorChannelIndex(m_commChannelBox->currentIndex());
        commLayout->addWidget(serialGroup);
        commLayout->addStretch();

        p_VtkVisualDock->setWidget(commWidget);
        p_VtkVisualDock->setFeatures(QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetFloatable);
        p_VtkVisualDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p_VtkVisualDock->setMinimumWidth(360);
        p_VtkVisualDock->setMinimumHeight(360);
        p_VtkVisualDock->setStyleSheet(p_forceVisualDock->styleSheet());

        this->splitDockWidget(p_forceVisualDock, p_VtkVisualDock, Qt::Vertical);
    }

#if 0

    //直线度 - 右侧停靠窗口，位于切削力下方
    {
        p_straightnessVisualDock = new QDockWidget(tr("直线度"), this);
        straightnessWidget = new StraightnessMonitorWidget(p_straightnessVisualDock);

        p_straightnessVisualDock->setWidget(straightnessWidget);
        p_straightnessVisualDock->setFeatures(QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetFloatable);
        p_straightnessVisualDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        p_straightnessVisualDock->setMinimumWidth(300);

        // 设置与项目树相同的样式
        p_straightnessVisualDock->setStyleSheet(p_forceVisualDock->styleSheet());

        // 连接信号槽
        connect(this, &MdiChild::straightnessDataUpdated,
            straightnessWidget, &StraightnessMonitorWidget::setData);

        // 将属性栏停靠在项目树下方
        this->splitDockWidget(p_forceVisualDock, p_straightnessVisualDock, Qt::Vertical);
    }
    //QDockWidget* p_forceVisualDock;//������ͣ����
    //QDockWidget* p_straightnessVisualDock;//������ͣ����


    // 设置标签页样式（如果使用MDI区域）
#endif
    this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    this->setDocumentMode(true);

    // 设置初始布局比例
    QList<int> mainSizes;
    mainSizes << 7 << 3; // 主区域:侧边栏 = 7:3
    this->resizeDocks({ p_TreeDock }, { 280 }, Qt::Horizontal);

    QList<int> leftSizes;
    leftSizes << 4 << 3; // 项目树:属性栏 = 4:3
    this->resizeDocks({ p_TreeDock, p_PropertyDock }, leftSizes, Qt::Vertical);

    QList<int> rightSizes;
    rightSizes << 1 << 1;
    this->resizeDocks({ p_forceVisualDock, p_VtkVisualDock }, rightSizes, Qt::Vertical);
}

// 添加窗口状态保存和恢复功能
void MdiChild::saveWindowState()
{
    QSettings settings("XSim", "Layout");
    settings.setValue("mainWindowState", this->saveState());
    settings.setValue("mainWindowGeometry", this->saveGeometry());
}

void MdiChild::restoreWindowState()
{
    QSettings settings("XSim", "Layout");
	this->restoreState(settings.value("mainWindowState").toByteArray());//恢复停靠窗口状态
	this->restoreGeometry(settings.value("mainWindowGeometry").toByteArray());//恢复窗口几何形状
}

// 添加布局重置功能
void MdiChild::resetLayout()
{
    // 重置所有停靠窗口到默认位置
    this->removeDockWidget(p_TreeDock);
    this->removeDockWidget(p_PropertyDock);
    this->removeDockWidget(p_MessageDock);

    // 重新添加停靠窗口
    this->addDockWidget(Qt::LeftDockWidgetArea, p_TreeDock);
    this->splitDockWidget(p_TreeDock, p_PropertyDock, Qt::Vertical);
    this->addDockWidget(Qt::BottomDockWidgetArea, p_MessageDock);

    // 重置大小比例
    QList<int> leftSizes;
    leftSizes << 200 << 150;
    this->resizeDocks({ p_TreeDock, p_PropertyDock }, leftSizes, Qt::Vertical);
}

