#pragma once


#ifndef PROJECTTREE_H
#define PROJECTTREE_H

#include <QWidget>
#include <TopoDS_Shape.hxx>
#include <QString>
#include <QTreeWidget>
#include <QMap>
#include "mdichild.h"
#include <vector>
#include <array>
#include <TopoDS_Edge.hxx>
#include <AIS_Shape.hxx>
#include "BoundaryConditionDialog.h"
class MdiChild;

struct ModelData {
	int index;
	QString name;          // 模型名称
	QString path;
	Handle(AIS_Shape) shape;    // OCCT 模型数据
	QString type;       // 类型：工件或切割平面
};

struct DiscreteData {
	std::vector<TopoDS_Edge>	 EdgeVec;
	std::vector<std::array<double, 3>> pointsVec;
};

//工程树，管理导入的模型
class ProjectTree : public QTreeWidget
{
	Q_OBJECT
public:
	//< 构造函数和析构函数
	ProjectTree(QWidget* parent);
	~ProjectTree();

	void importModel(const QString& filePath); // 导入模型
	QString GetShapeTypeName(TopoDS_Shape shape); // 获取模型类型名称

signals:
	void modelDoubleClicked(QTreeWidgetItem* item, int column); // 模型双击信号
	void clickedModel(const QString& modelName); // 模型点击信号

public:
	void initWidget();							// 初始化控件
	void addModelToTree(const ModelData& model); // 将模型添加到工程树
	void addPartToTree(const ModelData& model); // 将工件添加到工程树
	void addPlaneToTree(const ModelData& model); // 将切割平面添加到工程树
	void addPointToTree(std::vector<std::array<double, 12>>); // 存储离散点的坐标
	void addMeshToTree(const ModelData& model);
	void addBoundaryConditionToTree(const BoundaryConditionData& bcData);
	void updateBCsCount();
	void showBoundaryConditionDetails(const QString& bcName);

	QTreeWidget* treeWidget;                   // 工程树控件
    std::vector<std::array<double,3>> pointsVec; //存储离散点的坐标
	void savePoint(); // 存储离散点的坐标

	std::vector<std::array<double, 12>> anglesVec; //存储离散点的坐标 分别
	void saveAngle(); // 存储离散点的坐标
	
public slots:
	void onItemDoubleClicked(QTreeWidgetItem* item, int column); // 处理树项双击事件
	void deleteSelect(const QString& modelName);
	void clearModels();
	void showModel();
	void hideModel();
	void setTransparency();//设置透明度
	void toggleModelVisibility();
	void deleteSelectedModel();

	void onItemClicked(QTreeWidgetItem* item, int column); // 处理树项点击事件

	void populateGeometryTable(QTableWidget* table, const BoundaryConditionData& bcData);

	void populateMeshTable(QTableWidget* table, const BoundaryConditionData& bcData);

protected:
	void  	Popup(const int x, const int y);//<弹出右键菜单

	virtual void mousePressEvent(QMouseEvent*);//<鼠标按下事件
	//virtual void onLButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标左键点击事件
	//virtual void onMButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标中键点击事件
	//virtual void onRButtonDown(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标右键点击事件
	//virtual void onLButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标左键点击事件
	//virtual void onMButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标中键点击事件
	//virtual void onRButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标右键点击事件
	//virtual void onMouseMove(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point);//<鼠标移动事件

public:
	QMenu* myBackMenu;//< 右键菜单
	static QMap<QTreeWidgetItem*, Handle(AIS_InteractiveObject)>m_itemToAisObjectMap; // 树项到AIS对象的映射

	MdiChild* mdiChild;
	QTreeWidgetItem* Partitem = nullptr; // 工件项
	QTreeWidgetItem* Planeitem = nullptr; // 切割平面项
	QTreeWidgetItem* Pointitem = nullptr; // 离散点项
	QTreeWidgetItem* Meshitem = nullptr; //网格项

	QTreeWidgetItem* modelsItem = nullptr; // Models节点
	QTreeWidgetItem* partsItem = nullptr; // Parts节点
	QTreeWidgetItem* materialsItem = nullptr; // Materials节点
	QTreeWidgetItem* sectionsItem = nullptr; // Sections节点
	QTreeWidgetItem* assemblyItem = nullptr; // Assembly节点
	QTreeWidgetItem* stepsItem = nullptr; // Steps节点
	QTreeWidgetItem* fieldOutputItem = nullptr; // Field Output Requests节点
	QTreeWidgetItem* historyOutputItem = nullptr; // History Output Requests节点
	QTreeWidgetItem* timePointsItem = nullptr; // Time Points节点
	QTreeWidgetItem* aleAdaptiveMeshItem = nullptr; // ALE Adaptive Mesh节点
	QTreeWidgetItem* interactionsItem = nullptr; // Interactions节点
	QTreeWidgetItem* interactionPropertiesItem = nullptr; // Interaction Properties节点
	QTreeWidgetItem* contactControlsItem = nullptr; // Contact Controls节点
	QTreeWidgetItem* contactInitializationItem = nullptr; // Contact Initialization节点
	QTreeWidgetItem* contactStabilizationItem = nullptr; // Contact Stabilization节点
	QTreeWidgetItem* constraintsItem = nullptr; // Constraints节点
	QTreeWidgetItem* connectorSectionsItem = nullptr; // Connector Sections节点
	QTreeWidgetItem* fieldsItem = nullptr; // Fields节点
	QTreeWidgetItem* amplitudesItem = nullptr; // Amplitudes节点
	QTreeWidgetItem* loadsItem = nullptr; // Loads节点
	QTreeWidgetItem* bcsItem = nullptr; // BCs节点


	int count = 1;
	// 内部数据结构
	QMap<QString, ModelData> modelMap; // 模型映射
	QMap<QString, BoundaryConditionData> m_boundaryConditionsmap; // 存储边界条件数据

private:

	// 图标管理
	QIcon createColorIcon(const QColor& color, const QString& text = "") const;
	void setupTreeStructure(); // 设置树结构
	void setupTreeAppearance(); // 设置树外观


public:
	QMap<QTreeWidgetItem*, Quantity_Color> m_originalColors; // 保存原始颜色
	QTreeWidgetItem* m_lastSelectedItem = nullptr; // 记录最后选中的item
	
	// 离散点item到点坐标的映射
	QMap<QTreeWidgetItem*, std::vector<std::array<double, 12>>> m_pointItemToPointsMap; // 存储每个item对应的所有点

private:
		MdiChild* m_mdiChild; //  MdiChild 指针


		std::vector<std::array<double, 15>> selectedPoints;
};

#endif 
// PROJECTTREE_H