
#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>
#include <QProgressBar>
#include <QProgressDialog>
#include <QtWidgets/QAction>
#include <QtWidgets/QStatusBar>
#include <QDockWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtConcurrent>

//#include <QPluginLoader>
//#include <QMutex>

#include <src/cutsim/cutsim/cutsim.hpp>
#include <src/cutsim/cutsim/glwidget.hpp>
#include <src/cutsim/cutsim/machine.hpp>

//#include "levelmeter.hpp"

#include "src/cutsim/g2m/g2m.hpp"
#include <src/cutsim/g2m/gplayer.hpp>

//#include "version_string.hpp"
//#include "text_area.hpp"
//#include "src/cutsim/cutsim_def.hpp"

//#include "cutsim_app.hpp"

//#include "lex_analyzer.hpp"
#include <src/cutsim/cutsim/facet.hpp>
#include <src/cutsim/cutsim/volume.hpp>
#include <src/cutsim/cutsim/glvertex.hpp>
#include <src/cutsim/cutsim/mfem_analysis1.hpp>
#include <chrono>
#include <iostream>
#include <QObject>

#include "src/cutsim/cutsim/curve_widget.hpp"
#include "src/xsim/OcctVisualizer.hpp"
#include "src/xsim/myviewer.h"
#include "src/xsim/mdichild.h"


namespace digitaltwin_milling {

    class QAction;
    class QLabel;
    class QMenu;

    typedef enum {
        NO_OPERATION = 0,
        SUM_OPERATION = 1,
        DIFF_OPERATION = 2,
        INTERSECT_OPERATION = 3,
    } OperationType;

    class StockVolume {
    public:
        StockVolume() {};
        virtual ~StockVolume() {};
        cutsim::Volume* stock;
        int operation;
    };

    /// the main application window for the cutting-simulation
    /// this includes menus, toolbars, text-areas for g-code and canon-lines and debug
    /// the 3D view of tool/stock.
    class DigitalTwinMilling :public QObject {
        Q_OBJECT
    public:
        /// create window
        DigitalTwinMilling(int max_depth = 10);
        ~DigitalTwinMilling();
        int setStlStock(QString file1Path, double partoffset[3], double octreecenter[3], double octree_cube_size);
        int setRectStock(double center[3], double octree_cube_size);
        int setWorkMaterial(double rho, double E, double nu)
        {
            density = rho; youngsmodulus = E;  poisson = nu; return 1;
        };
        void extractSurfaceAndCenter();
        int setConstraints(std::array<std::array<double, 2>, 3>);
        int newMill();
        int addMill(QString file1Path);
        int setSpindleSpeed(double spindle_speed);
        int setBlade_Sum(double blade_sum);
        int setVisulization(int* visulization_item, std::array<double, 2> visulization_limits);
        int setMch_Data(std::array<double, 14>Msh_Data, std::vector<std::array<double, 3>>force_data);
        int setCutterParameters(const std::vector<Segment>& segments);
        int performFEMSimulation(MdiChild* mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits);
        int performFEMSimulation_test(MdiChild* mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits);
        int setVibrParams();
        int peformModalAnalysis();

        // �����û�ѡ��ĵ��к͵�
        void setSelectedBladePoint(int blade_id, int point_index);
        void enableSelection(bool enabled);

        cutsim::GLData* gld;
        Handle(AIS_InteractiveObject) workdeformed;
    private:

        cutsim::Cutsim* myMillDigitalTwin;

        cutsim::GLWidget* myGLWidget;

        StockVolume* stockVolume;

        std::vector<cutsim::CutterVolume*> myTools;

        unsigned int currentTool;

        double Milling_velocity[4];
        double simulation_time;
        double incrementive_time;
        double modalsteps;
        double density = 2.8e-9;
        double youngsmodulus = 68e3;
        double poisson = 0.3;

        //TextArea* debugText;

        //QStringList args;
        //QString myLastFolder;
        //QSettings settings;
        //QLabel* myStatus;
        double octree_cube_size;
        unsigned int max_depth;
        cutsim::GLVertex* octree_center;
        double cube_resolution_1, cube_resolution_2;
        double specific_cutting_force;
        double powerCoff;
        double requiredPower;

        std::vector<StockVolume*> myStocks;
        QProgressDialog* waitingDialog;

        double step_size;
        bool   variable_step_mode;

        // �����µĳ�Ա�������ڿ��ƻ���m�ķ���
        int simulation_step_interval;     // ��������������30��
        int current_step_counter;         // ��ǰ����������
        static const int BATCH_SIZE = 300;

        // 存储用户选择的刀号和点
        int selectedBladeId;              // 选中的刀具ID
        int selectedPointIndex;           // 选中的点编号
        bool selectionEnabled;            // 是否启用选择功能

        // 表面中心点
        cutsim::GLVertex surfaceCenter;   // 存储提取的表面中心点

    signals:
        void ApplyUpdateForces(double stroke, double Fx, double Fy, double Fz);
        void ApplyUpdateViewer();
        void straightnessDataUpdated(int blade_id, int point_index, double stroke_data, double straightness_data);
        void bladePointSelectionUpdated(int blade_count, int point_count);
        void surfaceCenterExtracted(double centerX, double centerY, double centerZ, int vertexCount);
    };
};