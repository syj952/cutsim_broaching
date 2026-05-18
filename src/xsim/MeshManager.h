#pragma once
#ifndef MESHMANAGER_H
#define MESHMANAGER_H

#include <iostream>
#include <qdialog.h>

#include <QLabel>
#include <QDialog>
#include "OcctView.h"
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <vector>
#include <utility>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QToolButton>
#include <QCheckBox>
#include "BoundaryConditionDialog.h"
using namespace std;
class OcctView;

// 创建新的网格参数结构，包含约束信息
struct MeshConstraintInfo {
    int constraintId;
    QString constraintName;
    int entityId;
    QString entityType;
    QList<int> nodeIds;
};

struct MeshParameters{
    int meshAlgo = 1;
    double maxSize = 0;
    double minSize = 0;
    double angleTolerance = 0;
    double gradationFactor = 0;
    bool characteristicLengthFromPhysicalGroups = false;  
    bool angleSmoothFactor = false;
    int divisionsX = 0;
    int divisionsY = 0;
    int divisionsZ = 0;
    double growthRate = 0;
	bool enableSmoothing = false;
    // 新增约束信息
    QList<MeshConstraintInfo> constraintInfos;
    bool applyConstraints = true;

    // 添加约束信息
    void addConstraintInfo(const ConstraintSet& constraint, const QList<int>& nodeIds) {
        MeshConstraintInfo info;
        info.constraintId = constraint.entityId;
        info.constraintName = constraint.name;
        info.entityId = constraint.entityId;
        info.entityType = constraint.entityType;
        info.nodeIds = nodeIds;
        constraintInfos.append(info);
    }

    // 获取所有约束节点数量
    int totalConstrainedNodes() const {
        int count = 0;
        for (const auto& info : constraintInfos) {
            count += info.nodeIds.size();
        }
        return count;
    }
};



class MeshDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeshDialog(QWidget* parent, OcctView* view);
    ~MeshDialog();

    // 获取网格类型
    int meshAlgo() const;
	MeshParameters MeshParams() const;

    // 获取网格尺寸参数
    double MaxcellSize() const;
	double MincellSize() const;
	double AngleTolerance() const;
	double GradationFactor() const;
	bool CharacteristicLengthFromPhysicalGroups() const;
	bool AngleSmoothFactor() const;

    // 获取高级参数
    int divisionsX() const;
    int divisionsY() const;
    int divisionsZ() const;
    double growthRate() const;
    bool enableSmoothing() const;

public slots:
	void onGenerateMesh();
	void onSolidSelected(const TopoDS_Solid& solid);

private slots:
    void onMeshTypeChanged(int index);
    void updatePreview();
    void onHelpButtonClicked();  // 帮助按钮点击事件
    void showHelpInformation();  // 显示帮助信息

signals:  //网格生成请求信号
	//&param minSize 最小单元尺寸
	//&param maxSize 最大单元尺寸
	//&param AngleTole 角度容差
	//&param GradaFactor 渐变因子
	//&param ApplyGPSize 确保物理组使用全局尺寸设置
	//&param AngleSmooth 启用基于角度的网格细化
	//&param elementOrder 元素阶数
    //void generateMeshRequested(double minSize, double maxSize, double AngleTole,
      //  double GradaFactor,bool ApplyGPSize,bool AngleSmooth, int elementOrder);

    void generateMeshRequested(const MeshParameters& params);

private:
    void setupUI();
    void setupHelpButton();
    void setupConnections();

    // 网格类型选择
    QComboBox* m_meshTypeCombo;

    // 基本参数
	QDoubleSpinBox* m_cellmaxSizeSpin;//设置最大单元尺寸
	QDoubleSpinBox* m_cellminSizeSpin;//设置最小单元尺寸
    QDoubleSpinBox* m_AngleTolerance;//设置角度容差
    QDoubleSpinBox* m_GradationFactor;//设置渐变因子
    QCheckBox* m_CharacteristicLengthFromPhysicalGroups;//确保物理组使用全局尺寸设置
    QCheckBox* m_AngleSmoothFactor;//启用基于角度的网格细化

    // 高级参数
	QSpinBox* m_divisionsXSpin;//设置X方向划分数
	QSpinBox* m_divisionsYSpin;//设置Y方向划分数
	QSpinBox* m_divisionsZSpin;//设置Z方向划分数
	QDoubleSpinBox* m_growthRateSpin;//设置网格增长率
	QCheckBox* m_smoothingCheck;//启用网格平滑

    //网格生成按钮
    QPushButton* m_generatedMeshBox;

    // 按钮框
    QDialogButtonBox* m_buttonBox;
    QToolButton* m_helpButton;
    QString m_helpText;         // 帮助文本内容

    // 预览区域
    QLabel* m_previewLabel;

private:
    OcctView* m_view;
    QWidget* m_parent;
 

    ///以下是边界条件约束相关
 private slots:
    void onApplyConstraintsChanged(int state);
    void onBoundaryConditionsUpdated();
    
private:
    QCheckBox* m_applyConstraintsCheck;//< 是否将固定约束应用到网格节点
    QLabel* m_constraintInfoLabel;  //< 约束信息标签
    BoundaryConditionDialog* findBoundaryConditionDialog();
    BoundaryConditionDialog* m_bcDialog = nullptr; // 直接存储指针
    void updateGenerateMeshButton();
    void updateConstraintInfo();
public:
    void setBoundaryConditionDialog(BoundaryConditionDialog* dialog) {
        m_bcDialog = dialog;
        if (m_bcDialog) {
            connect(m_bcDialog, &BoundaryConditionDialog::constraintsUpdated,
                this, &MeshDialog::onBoundaryConditionsUpdated);
        }
    }
};

// 新增MeshConstraintProcessor类
class MeshConstraintProcessor {
public:
    MeshConstraintProcessor(Handle(AIS_InteractiveContext) context);
    ~MeshConstraintProcessor() {};

    // 处理约束集，找到几何实体上的网格节点
    bool processConstraints(const QList<ConstraintSet>& constraints,
        MeshParameters& meshParams,
        const QList<MeshNode>& meshNodes);

    // 获取处理结果
    QList<MeshConstraintInfo> getConstraintInfos() const { return m_constraintInfos; }

    // 可视化约束节点
    void visualizeConstraintNodes(Handle(AIS_InteractiveContext) context);

private:
    Handle(AIS_InteractiveContext) m_context;
    QList<MeshConstraintInfo> m_constraintInfos;

    // 几何实体到网格节点的映射方法
    QList<int> findNodesOnFace(const TopoDS_Face& face, const QList<MeshNode>& nodes);
    QList<int> findNodesOnEdge(const TopoDS_Edge& edge, const QList<MeshNode>& nodes);
    QList<int> findNodesOnVertex(const TopoDS_Vertex& vertex, const QList<MeshNode>& nodes);

    // 几何判断方法
    bool isPointOnFace(const gp_Pnt& point, const TopoDS_Face& face, double tolerance = 1e-5);
    bool isPointOnEdge(const gp_Pnt& point, const TopoDS_Edge& edge, double tolerance = 1e-5);
    double distanceToShape(const gp_Pnt& point, const TopoDS_Shape& shape);
};



#include <QObject>
#include <string>

class GmshMessageHandler : public QObject {
    Q_OBJECT

public:
    static GmshMessageHandler* instance();
    static void gmshMessageCallback(const std::string& message);

    void startCapturing();
    void stopCapturing();

signals:
    void messageReceived(const QString& message);

public:
    GmshMessageHandler(QObject* parent = nullptr);
    static GmshMessageHandler* m_instance;
    bool m_isCapturing = false;
};


#endif