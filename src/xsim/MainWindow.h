
/**@mainpage  刀具参数化建模
* <table>
* <tr><th>Project  <td>XSim
* <tr><th>Author   <td>Hust b220 叶锋
* </table>
* @section   项目详细描述
* -# 实现了基本的CAD功能的软件，包含模型导入、导出、显示、简单编辑等功能
* -# 基于OpenCASCADE 7.7.0开发，使用Qt 5.14.2进行界面设计,采用OpenGL进行三维渲染
* -# 包含项目管理器、属性栏、消息栏等常用模块
*
* @section   功能描述
* -# 目前支持STEP、IGES等格式的模型导入，支持基本的模型显示、旋转、缩放、平移等操作
* -# 能够设置点线面的拾取，设置光照、材质等基本渲染参数
* -# 具备简单的模型编辑功能，如布尔运算、拉伸、旋转等和测量功能，包括点坐标获取，边的离散等
* -# 可以获取点的坐标，边和面的类型
* -# 实现了边的离散化功能，能够获取边上的离散点及切向量，并计算刀具的前后角
*
* @section   用法描述
* -# “新建”创建一个新的子窗口
* -# “模型”-“导入模型”可以选择STEP或IGES或STL格式的文件进行导入
*
* @section   更新日志
* <table>
* <tr><th>Date        <th>H_Version  <th>Author    <th>Description  </tr>
* <tr><td>2025/11/13  <td>1.0       <td>Hust b220 叶锋  <td>创建初始版本 </tr>
* </tr>
* </table>
**********************************************************************************
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MdiChild;
QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
	virtual ~MainWindow();
    bool openFile(const QString &fileName);
	void setupUI();
 /*   void setupModernToolbars();
    void addToolbarAction(QToolBar* toolbar, QAction* action, const QString& text, const QString& iconText);

    void setupEnhancedProjectTree();

    void setupEnhancedPropertyPanel();

    QGroupBox* createPropertyGroup(const QString& title);

    QLabel* createValueLabel(const QString& text);*/

protected:
    void closeEvent(QCloseEvent *event) override;

private slots://声明槽，私有槽只能被该类的成员函数或其他具有适当访问权限的代码调用
    void newFile();
    void open();
    void save();
    void saveAs();
    void updateRecentFileActions();
    void openRecentFile();
    void about();
    void updateMenus();
	void updateMDIMenus();
    void updateWindowMenu();
    MdiChild *createMdiChild();
    void switchLayoutDirection();//<调整布局方向
   // void onCalculateRakeAngles(); // 新增槽函数
private:
    enum { MaxRecentFiles = 5 };

    void createActions();
	void createMDIActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool loadFile(const QString &fileName);
    static bool hasRecentFiles();
    void prependToRecentFiles(const QString &fileName,bool Add = true);
    void setRecentFilesVisible(bool visible);
    MdiChild *activeMdiChild() const;//<获取当前活动子窗口，并将其转换为midchild类型的指针
    QMdiSubWindow *findMdiChild(const QString &fileName) const;

    QMdiArea *	mdiArea;//<子窗口
	QMenu *		windowMenu;//<菜单：窗口
	QAction *	newAct;//<新建
	QAction *	openAct;//<打开
	QAction *	saveAct;//<保存
	QAction *	saveAsAct;//<另存为
    QAction *	recentFileActs[MaxRecentFiles];//<最近打开文件(最大数量为5)
    QAction *	recentFileSeparator;//<最近打开文件栏分隔符
    QAction *	recentFileSubMenuAct;//<最近打开文件菜单栏的行为操作
    QAction *	closeAct;//<关闭
    QAction *	closeAllAct;//<关闭全部
    QAction *	tileAct;//<标题
    QAction *	cascadeAct;//<排列
    QAction *	nextAct;//<下一个
    QAction *	previousAct;//<上一个
    QAction *	windowMenuSeparatorAct;//<窗口目录分隔符
private:
	QMenu *				Mdi_ViewMenu;//<菜单：视图
	QList<QMenu *>		ChildMenus;//<菜单栏
	QList<QToolBar *>	ChildToolBars;//<子窗口工具栏

private:
    QAction* actionCalculateRakeAngles; // 新增动作
};

#endif
