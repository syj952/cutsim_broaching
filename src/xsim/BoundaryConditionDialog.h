#pragma once
#pragma once

#ifndef BOUNDARYCONDITIONDIALOG_H
#define BOUNDARYCONDITIONDIALOG_H

#include <QDialog>
#include <QMap>
#include <QList>
#include <QString>
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <gp_Pnt.hxx>
#include "OCCT_ShapeList.h"
#include <TopTools_IndexedMapOfShape.hxx>
#include <vector>
#include "OCCT_GraphOperations.h"
//#include "MeshManager.h"

class QComboBox;
class QLineEdit;
class QDoubleSpinBox;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class OcctView;   


// 网格节点结构
struct MeshNode {
    int id;           // 节点ID
    double x, y, z;   // 节点坐标
    bool isConstrained; // 是否被约束
    QString constraintType; // 约束类型
    int constraintId; // 所属约束集的ID

    MeshNode() : id(-1), x(0), y(0), z(0), isConstrained(false), constraintId(-1) {}
    MeshNode(int nodeId, double xCoord, double yCoord, double zCoord)
        : id(nodeId), x(xCoord), y(yCoord), z(zCoord), isConstrained(false), constraintId(-1) {
    }

    // 转换为gp_Pnt
    gp_Pnt toPoint() const {
        return gp_Pnt(x, y, z);
    }

    // 计算到另一点的距离
    double distanceTo(const MeshNode& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }

    // 计算到gp_Pnt的距离
    double distanceTo(const gp_Pnt& point) const {
        double dx = x - point.X();
        double dy = y - point.Y();
        double dz = z - point.Z();
        return sqrt(dx * dx + dy * dy + dz * dz);
    }
};

struct ConstraintSet {
    QString name;                    // 约束集名称
    QString entityType;              // 几何实体类型：面、边、点
    int entityId;
    OCCT_ShapeList shapes;              // 几何实体形状
    QList<int> constrainedNodeIds;   // 被约束的网格节点ID列表
    QColor displayColor;             // 显示颜色

    // 检查约束集是否有效
    bool isValid() const {
        return !name.isEmpty() && !shapes.IsEmpty();
    }

    // 获取约束节点数量
    int nodeCount() const {
        return constrainedNodeIds.size();
    }
};

// 边界条件数据类型
struct BoundaryConditionData
{
    QString name;                   // 边界条件名称
    QString type;                   // 类型：固定、位移、力、压力等
    QString entityType;             // 施加对象类型：面、边、点
    int entityId = 0;                   // 施加对象ID
    QMap<QString, double> values;   // 边界条件值
    QColor displayColor;           // 显示颜色

OCCT_ShapeList shapes;//施加边界条件对象
std::vector<int> meshIdsToConstraint{}; // 边界条件关联的网格节点ID
std::vector<gp_Pnt> associatedNodePoints{}; // 边界条件关联的网格节点pnt

    //@brief 关联网格节点
    void AssociateNodes(std::vector<double>& nodeCoords) {
        OCCT_GraphOperations::AssociateNodes(shapes, nodeCoords, meshIdsToConstraint, associatedNodePoints);
    };

    bool isConstraint() const {
        return type == "固定约束" || type == "位移约束";
    }

    // 转换为约束集格式（用于网格处理）
    ConstraintSet toConstraintSet() const {
        ConstraintSet cs;
        cs.name = name;
        cs.entityType = entityType;
        cs.entityId = entityId;
        cs.shapes = shapes;
        cs.displayColor = displayColor;
        return cs;
    }
};

class BoundaryConditionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BoundaryConditionDialog(OcctView* view, QWidget* parent = nullptr);
    ~BoundaryConditionDialog();

    BoundaryConditionData getBoundaryConditionData() const;
    void setSelectedShape(const Handle(AIS_InteractiveObject)& shape);//<设置当前选择的形状

signals:
    void boundaryConditionApplied(const BoundaryConditionData& data);//< 应用边界条件触发信号
    void constraintsUpdated(const QList<ConstraintSet>& constraints); // 新增信号
    void boundaryConditionAdded(const BoundaryConditionData& bcData);//< 边界条件添加信号

public slots:
    void onBCTypeChanged(int index);
    void onEntityTypeChanged();//< 改变施加对象类型
    void onAddBC();//< 添加边界条件
    void onRemoveBC(); //< 移除边界条件
    void onApply();//< 应用边界条件
    void onSelectionChanged();//< 选择变化槽函数
    void updateValueFields();//< 更新数值输入字段
    BoundaryConditionData getBCFromItem(QTreeWidgetItem* item);
public:
    void setupUI();//< 设置UI布局
    void setupConnections();//< 设置信号与槽连接机制
    void populateEntityList();//< 设置施加对象列表
    void createBCPreview();//< 创建边界条件预览
    void updateBCList();//< 更新边界条件列表
    void addBCToList(const BoundaryConditionData& bcData);//< 将边界条件添加到列表
    void applyBoundaryConditions(const QList<BoundaryConditionData>& bcList);//<应用边界条件
    void displayBC(const BoundaryConditionData& bcData);//< 显示边界条件
    OcctView* m_view;
    TopoDS_Shape m_selectedShape;
    BoundaryConditionData m_currentBC;

    // UI控件
    QComboBox* m_bcTypeCombo;    //< 边界条件约束类型下拉框
    QComboBox* m_entityTypeCombo; //< 边界条件施加实体类型下拉框
    QLineEdit* m_bcNameEdit; //<边界条件名称编辑框
    QTreeWidget* m_entityTree; //< 施加对象列表
    QTableWidget* m_valueTable;//<数值输入表格
    QTreeWidget* m_bcList;//< 边界条件列表
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_applyButton;

    QMap<QString, QDoubleSpinBox*> m_valueWidgets;  //< 数值输入控件映射
    QList<BoundaryConditionData> bcList{};//< 存储所有边界条件
    //QLabel* m_frontStatusLabel;
    //QLabel* m_rearStatusLabel;

private:
    QMap<int, TopoDS_Shape> m_entityMap; // 实体ID到形状的映射
    TopTools_IndexedMapOfShape uniqueShapes;  // 存储唯一拓扑形状
    int m_CurrentEntityId = 1; // 下一个实体ID

public:
    // 新增方法
    QList<ConstraintSet> getConstraintSets() const { return m_constraintSets; }
    static QList<ConstraintSet> getAllConstraints(QList<BoundaryConditionData>& bcList) ;
    QList<BoundaryConditionData> getAllBoundaryConditions() const { return bcList; };
    void clearConstraintSets() { m_constraintSets.clear(); }

private:
    QList<ConstraintSet> m_constraintSets;  // 存储所有约束集

    int m_bcCounter = 1; // 边界条件计数器
    QString generateNextBCName(); // 生成下一个边界条件名称

    //// 新增私有方法
    //void createFixedConstraint();
    //void updateConstraintDisplay();

};

#endif // BOUNDARYCONDITIONDIALOG_H