
#ifndef MDICHILD_H
#define MDICHILD_H

#include <vector>
#include <array>
#include <functional>  // 包含 std::function
#include "src/cutsim/cutsim/glvertex.hpp"  // 包含 GLVertex 类型定义
#include <QMainWindow>
#include "MyDocument.h"
#include "MyViewer.h"
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Dir.hxx>
#include <QProgressDialog>
#include <TopoDS_Solid.hxx>
#include "MeshManager.h"
//#include <QtConcurrent>
#include <QFutureWatcher>
#include "OcctView.h"
#include "MeshManager.h"
#include "MaterialDialog.h"    
#include <QList>
#include <QMap>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include "ForceMonitorWidget.hpp"
#include "src/digitaltwin/monitoring/vtkwidget.h"
#include "src/digitaltwin/monitoring/serialportcom.h"
#include "StraightnessMonitorWidget.h"


class GmshMessageHandler;
class DigitalTwinController;
namespace digitaltwin_milling {
    class DigitalTwinMilling;
}
struct MeshParameters;
struct BroachParameters {
    std::array< double, 3> velocity = { 0 };
    std::array<std::array<double, 2>, 3> constrain_limits;
    std::array< double, 3> simulation = { 0 };
    std::array<std::array<double, 11>, 4> force_coefs;
    bool enable_machining_residual_stress = false;
};

struct Segment {
    int type;
    double radius1, radius2, length;
    cutsim::GLVertex center, angle;
    double z_start, z_end;

    // 默认构造函数
    Segment() : type(0), radius1(0.0), radius2(0.0), length(0.0), center(), z_start(0.0), z_end(0.0) {}

    // 添加构造函数
    Segment(int t, double r1, double r2, double len, double zs, double ze)
        : type(t), radius1(r1), radius2(r2), length(len), center(), z_start(zs), z_end(ze) {
    }
};

struct MillParameters {
    std::array< double, 3> velocity = { 0 };
    std::array<std::array<double, 2>, 3> constrain_limits;
    std::array< double, 3> simulation = { 0 };
    double spindle_speed = 0.0, blade_sum = 0.0;
    std::array<double, 6> force_coefs;
};

struct DigitalTwin_MillParameters {
    std::array<std::array<double, 2>, 3> constrain_limits;
    double spindle_speed = 0.0;
    std::vector<Segment> cutter_segments; // 刀具段参数
};


// ���ϲ����ṹ�壨ǰ��������������MaterialDialog.h�У�
struct MaterialProperty;

class MdiChild : public QMainWindow
{
    Q_OBJECT
public:
    MdiChild(QWidget* parent);
    virtual ~MdiChild();

    QString userFriendlyCurrentFile();//<��ȡ��ǰ�ļ���
    QString currentFile() { return curFile; }//<��ȡ��ǰ�ļ���

    QList<QAction*> GetDockActions();//<��ȡͣ�����ڵĶ����б�
    Handle(MyDocument) document() { return h_MyDoc; }

    void SetCurrent();//<���õ�ǰ����
    void createMDIActions();//<����MDI�Ӵ��ڵĶ���
public: //mdichild_action1
    void newFile();//<�½��ļ�
    bool loadFile(const QString& fileName);//<�����ļ�
    bool save();//<�����ļ�
    bool saveAs();//<����Ϊ
    bool saveFile(const QString& fileName);//<�����ļ�
public slots:
    void clearModel();//<���ģ��
    void updateView();//更新视图
public:
    void AisObjDisplayAll();//<��ʾ���ж���
    void AisObjEraseAll();//<�������ж���
    void AisObjHide();//<���ض���
    void SelTrans();//<ѡ��͸��
    void SelUnTrans();//<ѡ��͸��
    void SelWireFrame();//<ѡ���߿�
    void SelShaded();//<ѡ����Ӱ
    void SelColor();//<ѡ����ɫ
    void SelUnColor();//<ѡ����ɫ
    void DeleteSelObjects();//<ɾ��ѡ�ж���
    void DisplayModel(const TopoDS_Shape& shape);//<��ʾģ��
    void EraseModel(const TopoDS_Shape& shape);//<ɾ��ģ��
    void EraseModel(const Handle(AIS_InteractiveObject)& aisObj);//<ɾ��ģ��
public slots: //mdichild_action2
    void ImportModel();//<����ģ��
    void ImportPart();//<���빤��
    void ImportPlane();//<�����и�ƽ��
    void ExportModel();//<����ģ��
    void SelNatural();//<ѡ����Ȼ
    void SelSolid();//<ѡ�����
    void SelFace();//<ѡ����
    void SelWire();//<ѡ����
    void SelEdge();//<ѡ���
    void SelVertex();//<ѡ�񶥵�
    void GetSelectedShapes();//<��ȡѡ�е���״ useless
    void TestSelectedFacesCuttingEdge();//<测试当前手动选中的两个面是否能识别切削刃

    void CaptureRakeFace();//<��׽ǰ����
    void CaptureClearanceFace();//<��׽����

    void InputMaterialProperty();


    //void CaptureEdge();//<��׽��
    void CaptureVertex();//<�����������
    void RunCutsim();
    void RunMchConfig();
    void RunDigitalTwin();
    void RunSerialPort();
    void braoching_executeSimulation(QString stlfile, QString TestPointsfile, QString BladeAnglesfile);
    void milling_executeSimulation(QString stlfile, QString TestPointsfile, QString BladeAnglesfile);
    void digitaltwin_milling_executeSimulation(QString stlfile);
    void digitaltwin_milling_test_executeSimulation(QString stlfile, std::array<double, 14>Msh_Data, std::vector<std::array<double, 3>>force_data);
public:
    void MoveModel();//<�ƶ�ģ��
    void MoveToolModel(bool, double, double, double, double, double, double);
    void RotateModel();//<��תģ��
    void ScaleModel();//<����ģ��
    void FaceOffset();//<��ƫ��
    void FaceExtend();//<������
    void EdgeDiscrete();//<����ɢ
    void EdgeConnectDiscrete();//<��������ɢ
    void clearDiscretePoints();//<�������ɢ
    void GetCondition();// ��ȡ��������
    void Calcu();
    void Test1();
    void Test2();
    void GenerateMesh();//<��������
    bool SaveMeshToFile();//<���������ļ�
    void GenerateMeshWithGmsh(const MeshParameters& params);//ʹ��Gmsh��������
    void SetBoundaryCondition();//���ñ߽�����

public slots:
    void onMeshGenerationFinished();
    void onGmshMessageReceived(const QString& message);

    void ComputeConditions();
    void ComputeandOffsetEdge();
    int generateAbaqusINP(bool planestrain, double V, double t, double lambda, double beta, double gama, double r, double miu);
private:
    bool getRepresentativeFaceNormal(const TopoDS_Face& face, gp_Dir& normal) const;
    double computeFaceDirectionAngleDeg(const TopoDS_Face& face, const gp_Dir& cuttingDir) const;
    void highlightFacesInContext(const std::vector<TopoDS_Face>& faces);
    void highlightEdgesInContext(const std::vector<TopoDS_Edge>& edges);
    bool collectAutoMatchedToolFaces(std::vector<TopoDS_Face>& targetFaces, const char* faceLabel);
protected slots: //mdichild_slots
    void selectionChanged();
protected:
    void closeEvent(QCloseEvent* event) override;
protected: //mdichild_init
    void Init();
    //void createToolbarGroups(QToolBar* toolbar, OcctView* view);
public:
    void saveWindowState();//<���洰�ڲ���
    void restoreWindowState();//<�ָ����ڲ���
    void resetLayout();//<���ò���
    //<���Ӵ��ڲ��ֽ��г�ʼ��
private slots:
    void documentWasModified();

    //void generateMeshAsync(double minSize, double maxSize, double AngleTole,
    //	double GradaFactor, bool ApplyGPSize, bool AngleSmooth,
    //	int elementOrder, TopoDS_Shape solid);

    void generateMeshAsync(const MeshParameters& params, TopoDS_Shape solid);
public:
    bool maybeSave();//<�Ƿ񱣴�
    void setCurrentFile(const QString& fileName);
    QString strippedName(const QString& fullFileName);

    QString					curFile;
    bool					isUntitled;

    //Occt
    class OcctView* q3dView;//occ��ͼ��

    //Message
    QDockWidget* p_MessageDock;//״̬��ͣ����
    class GUI_Message* p_MessageWidget;

    //Property
    QDockWidget* p_PropertyDock;//������ͣ����
    class PropertyView* p_PropertyWidget;

    //��Ŀ��
    //ForceCurveWidget* forceCurveWidget;//
    QDockWidget* p_TreeDock;//��Ŀ��ͣ����
    class ProjectTree* p_TreeWidget = nullptr;

    QDockWidget* p_forceVisualDock;//������ͣ����
    class ForceMonitorWidget* forcewidget;
    QDockWidget* p_straightnessVisualDock;//������ͣ����
    class StraightnessMonitorWidget* straightnessWidget;

    QDockWidget* p_VtkVisualDock; //VTK

    class BoundaryConditionDialog* p_BCDialog = nullptr;//<�߽������Ի���

    std::vector<double> displacement = { 1,2 };//<����λ�Ʊ���

    //CutsimBroaching* myBroach = nullptr


    QList<MaterialProperty> m_materialProperties;

    // 刀具参数输入控件
    QLineEdit* cutterParamsEdit = nullptr;

    // 离散点到原模型的映射关系（离散点AIS对象 -> 原模型AIS对象）
    QMap<Handle(AIS_InteractiveObject), Handle(AIS_Shape)> m_discretePointToModelMap;

public:
    Handle(MyDocument)		h_MyDoc;
    Handle(MyViewer)		h_MyViewer;

private:
    QProgressDialog* m_progressDialog;	// ���ȶԻ���

    std::vector<double> m_lastNodeCoords; //���һ�����ɵ�����ڵ�����
    std::vector<int> m_lastElemTypes; //���һ�����ɵ�����Ԫ����
    std::vector<std::vector<std::size_t>> m_lastElemNodeTags; //���һ�����ɵ�����Ԫ�ڵ�����
    QString m_lastError;
    int MeshCounter = 0; // ��������������ڼ�¼�������ɵĴ���

private:
    QFutureWatcher<void> m_meshFutureWatcher;
    std::shared_ptr<GmshMessageHandler> m_gmshMessageHandler;
    QString formatGmshMessage(const QString& rawMessage);

public:
    // Helper functions
    //���ɵȲ�����
    inline std::vector<double> linspace(double start, double end, int num) {
        std::vector<double> result(num);
        if (num == 1) {
            result[0] = start;
            return result;
        }
        double step = (end - start) / (num - 1);
        for (int i = 0; i < num; i++) {
            result[i] = start + i * step;
        }
        return result;
    };

    inline bool x_in_range(double a, double b, double x) {
        return (x >= std::min(a, b) && x <= std::max(a, b));
    };

    inline bool y_in_range(double a, double b, double y) {
        return (y >= std::min(a, b) && y <= std::max(a, b));
    };

    inline bool z_in_range(double a, double b, double z) {
        return (z >= std::min(a, b) && z <= std::max(a, b));
    };

    // Function to check if a point is inside a polygon (used for inradius calculation)
    inline bool inradius(std::vector<double> A, std::vector<double> B, std::vector<double> C, std::vector<double> D, std::vector<double> P) {
        std::vector<double> AB = { B[0] - A[0], B[1] - A[1] };
        std::vector<double> BC = { C[0] - B[0], C[1] - B[1] };
        std::vector<double> CD = { D[0] - C[0], D[1] - C[1] };
        std::vector<double> DA = { A[0] - D[0], A[1] - D[1] };
        std::vector<double> AP = { P[0] - A[0], P[1] - A[1] };
        std::vector<double> BP = { P[0] - B[0], P[1] - B[1] };
        std::vector<double> CP = { P[0] - C[0], P[1] - C[1] };
        std::vector<double> DP = { P[0] - D[0], P[1] - D[1] };

        double cross1 = AB[0] * AP[1] - AB[1] * AP[0];
        double cross2 = BC[0] * BP[1] - BC[1] * BP[0];
        double cross3 = CD[0] * CP[1] - CD[1] * CP[0];
        double cross4 = DA[0] * DP[1] - DA[1] * DP[0];

        double gama = -1e-8;

        if (cross1 >= gama && cross2 >= gama && cross3 >= gama && cross4 >= gama) {
            return true;
        }
        else {
            return false;
        }
    };
    //����ָ�����ȵ��ַ���
    inline std::string convertDoubleToString(double value, int precision) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }
    // 函数功能：判断双精度浮点数的符号
// 返回值：正数返回 1，负数返回 -1，接近于零返回 0
    inline int sign(const double x, const double eps = 1e-10) {
        //if (std::abs(x) < eps) return 0;
        if (x > 0) return 1;
        return -1;
    }
public:
    int* visulization_item = 0; // U magnitude:0, Ux: 1; Uy: 2; Uz: 3;
    std::array<double, 2> visulization_limits;
    BroachParameters broachpar;
    MillParameters millpar;
    DigitalTwin_MillParameters digitaltwin_millpar;
    // 存储文件路径的变量
    QString workfilePath;
    QString cutedgefilePath;
    QString file3Path;

private:
    VTKWidget* p_VtkWidget = nullptr;
    DigitalTwinController* m_digitalTwinController = nullptr;

signals:
    void straightnessDataUpdated(int blade_id, int point_index, double stroke_data, double straightness_data);
    void bladePointSelectionUpdated(int blade_count, int point_count);

public slots:
    void onStraightnessDataUpdated(int blade_id, int point_index, double stroke_data, double straightness_data);
};
#endif
