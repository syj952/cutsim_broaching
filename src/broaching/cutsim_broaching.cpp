#include "cutsim_def.hpp"

//#include "cutsim_app.hpp"
#include "cutsim_broaching.hpp"

#include <src/cutsim/facet.hpp>
#include <src/cutsim/volume.hpp>
#include <QEventLoop>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <array>

namespace {
    struct MeshIdStats {
        int coordinateCount = 0;
        int maxCoordinateId = -1;
        int maxElementVertexId = -1;
        int maxBoundaryVertexId = -1;
        int maxVertexParentId = -1;
        int maxMeshVertexId = -1;
    };

    void updateMaxId(int value, int& target, int& globalMax)
    {
        if (value > target) {
            target = value;
        }
        if (value > globalMax) {
            globalMax = value;
        }
    }

    MeshIdStats readMeshIdStats(const std::string& meshFile)
    {
        MeshIdStats stats;
        std::ifstream in(meshFile);
        std::string token;

        while (in >> token) {
            if (token == "elements") {
                int count = 0;
                in >> count;
                std::string line;
                std::getline(in, line);
                for (int i = 0; i < count && std::getline(in, line); ++i) {
                    std::istringstream iss(line);
                    int attr = 0, geom = 0, order = 0, nodes = 0;
                    iss >> attr >> geom >> order >> nodes;
                    for (int j = 0; j < 8; ++j) {
                        int id = -1;
                        if (iss >> id) {
                            updateMaxId(id, stats.maxElementVertexId, stats.maxMeshVertexId);
                        }
                    }
                }
            }
            else if (token == "boundary") {
                int count = 0;
                in >> count;
                std::string line;
                std::getline(in, line);
                for (int i = 0; i < count && std::getline(in, line); ++i) {
                    std::istringstream iss(line);
                    int attr = 0, geom = 0;
                    iss >> attr >> geom;
                    int id = -1;
                    while (iss >> id) {
                        updateMaxId(id, stats.maxBoundaryVertexId, stats.maxMeshVertexId);
                    }
                }
            }
            else if (token == "vertex_parents") {
                int count = 0;
                in >> count;
                for (int i = 0; i < count; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        int id = -1;
                        in >> id;
                        updateMaxId(id, stats.maxVertexParentId, stats.maxMeshVertexId);
                    }
                }
            }
            else if (token == "coordinates") {
                int dim = 0;
                in >> stats.coordinateCount >> dim;
                stats.maxCoordinateId = stats.coordinateCount - 1;
                updateMaxId(stats.maxCoordinateId, stats.maxCoordinateId, stats.maxMeshVertexId);
                std::string line;
                std::getline(in, line);
                for (int i = 0; i < stats.coordinateCount && std::getline(in, line); ++i) {
                }
            }
        }

        return stats;
    }
}

CutsimBroaching::CutsimBroaching(int depth) {
    max_depth = depth;
    myGLWidget = new cutsim::GLWidget(DEFAULT_SCENE_RADIUS);
    gld = myGLWidget->addGLData();
    stockVolume = new StockVolume();
    octree_cube_size = 50;
}
int CutsimBroaching::setStlStock(QString file1Path, double partoffset[3], double octreecenter[3], double cube_size)
{
    octree_cube_size = cube_size;
    octree_center = new cutsim::GLVertex(octreecenter[0], octreecenter[1], octreecenter[2]);
    cutsim::StlVolume* stock2 = new cutsim::StlVolume();
    stock2->setCenter(cutsim::GLVertex(partoffset[0], partoffset[1], partoffset[2]));
    stock2->setRotationCenter(cutsim::GLVertex(0, 0, 0));
    stock2->setAngle(cutsim::GLVertex(0, 0, 0));

    double cube_resolution = octree_cube_size * 2.0 / pow(2.0, max_depth - 1);
    stock2->setCubeResolution(cube_resolution);
    int error = stock2->readStlFile(file1Path);
    if (error == 0) {
        stock2->setColor(PARTS_COLOR);
        stock2->calcBB();
        stockVolume->stock = stock2;
        stockVolume->operation = SUM_OPERATION;
        myStocks.push_back(stockVolume);

    }
    else {
        qDebug() << "STL error:" << error;
        delete stock2;
        delete stockVolume;
    }
    myBroachCutsim = new cutsim::Cutsim(octree_cube_size, max_depth, octree_center, gld, myGLWidget);
    //    myCutsim->init(4);

    //myCutsim->sum_volume_cuda(stock1, max_depth_sum);
    myBroachCutsim->sum_volume(stockVolume->stock);
    myBroachCutsim->updateGL();
    return error;
}
int CutsimBroaching::setRectStock(double center[3], double cube_size)
{
    octree_cube_size = cube_size;
    octree_center = new cutsim::GLVertex(center[0], center[1], center[2]);
    cutsim::RectVolume* stock1 = new cutsim::RectVolume();
    stock1->setColor(PARTS_COLOR);
    stock1->setlengthX(30);
    stock1->setlengthY(35);
    stock1->setlengthZ(23);
    stock1->setCenter(*octree_center);
    stock1->calcBB();

    stockVolume->stock = stock1;
    stockVolume->operation = SUM_OPERATION;
    myStocks.push_back(stockVolume);
    myBroachCutsim = new cutsim::Cutsim(octree_cube_size, max_depth, octree_center, gld, myGLWidget);
    myBroachCutsim->sum_volume_cuda(stock1, max_depth);
    //myBroachCutsim->sum_volume(stock1);
    myBroachCutsim->updateGL();
    return 1;
}
int CutsimBroaching::setConstraints(std::array<std::array<double, 2>, 3> xyzconstraints) {
    myBroachCutsim->setConstraints(xyzconstraints);
    return 1;
}

int CutsimBroaching::addBroach(QString file1Path)
{
    cutsim::AptCutterVolume* currentBroach = dynamic_cast<cutsim::AptCutterVolume*> (myTools[0]);

    int blade_id = currentBroach->blade_sum;  // 初始化索引计数器
    if(currentBroach->readBladeAnglesFromFile(file1Path.toStdString(), blade_id))    currentBroach->blade_sum++; //读取刀刃1离散点角度信息
    return 1;
}
int CutsimBroaching::addBroachs(QString filePath)
{

    std::ifstream file(filePath.toStdString());

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filePath.toStdString() << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(file, line)) {
        addBroach(QString::fromStdString(line));
    }
    return 1;
}

int CutsimBroaching::addBroach(vector<array<double, 12>>& points)
{
    cutsim::AptCutterVolume* currentBroach = dynamic_cast<cutsim::AptCutterVolume*> (myTools[0]);
    int blade_id = currentBroach->blade_sum;  // 初始化索引计数器
    for (const auto& point : points)
    {
        // 前三列（索引0~2）赋值给original_blade_points[blade_id]
        currentBroach->original_blade_points[blade_id].emplace_back(point[0], point[1], point[2]);
        // 第五列（索引4）赋值给blade_angles_gamma[blade_id]
        currentBroach->blade_angles_gamma[blade_id].push_back(point[4]);
        // 第六列（索引5）赋值给blade_angles_alpha[blade_id]
        currentBroach->blade_angles_alpha[blade_id].push_back(point[5]);
        // 第八列（索引7）赋值给blade_cut_h[blade_id]
        currentBroach->blade_cut_h[blade_id].push_back(point[7]);
        // 第九列（索引8）赋值给blade_rakeface_id[blade_id]
        currentBroach->blade_rakeface_id[blade_id].push_back(static_cast<int>(point[8]));
        // 第十、十一、十二列（索引9~11）赋值给blade_rakeface_vertex[blade_id]
        currentBroach->blade_rakeface_vertex[blade_id].emplace_back(point[9], point[10], point[11]);
    }

    //刀刃数量
    currentBroach->blade_sum++;

    return 1;
}

int CutsimBroaching::newBroach(std::array<std::array<double, 11>, 4> force_coefs)
{
    cutsim::AptCutterVolume* currentBroach = new cutsim::AptCutterVolume();
    currentBroach->max_depth_1 = max_depth; //设置diff仿真最大层数

    //diff最大仿真层数对应的体素尺寸
    currentBroach->cube_resolution_1 = octree_cube_size * 2.0 / pow(2.0, currentBroach->max_depth_1 - 1);
    /*currentBroach->cube_resolution_2 = octree_cube_size * 2.0 / pow(2.0, currentBroach->max_depth_2 - 1);
    currentBroach->inv_cube_resolution_2 = 1.0 / currentBroach->cube_resolution_2;*/
    currentBroach->type = cutsim::APT_VOLUME;
    currentBroach->cuttertype = cutsim::APT;
    currentBroach->setColor(0.5, 0.2, 0.1);
    myTools.push_back(currentBroach);

    currentBroach->force_coefs = force_coefs;
    //m/min，刀具切削速率
    currentBroach->v_c = 5;
    //mm,仿真最小步长
    currentBroach->step = incrementive_time;
    //刀具移动方向向量
    currentBroach->v_x = broaching_velocity[0];
    currentBroach->v_y = broaching_velocity[1];
    currentBroach->v_z = broaching_velocity[2];
    //刀具单步位移量
    currentBroach->dx = currentBroach->v_x * currentBroach->step;
    currentBroach->dy = currentBroach->v_y * currentBroach->step;
    currentBroach->dz = currentBroach->v_z * currentBroach->step;

    // Newmark参数（平均加速度法）
    currentBroach->vibr_beta = 0.25;
    currentBroach->vibr_gamma = 0.5;
    currentBroach->stock_vibr_damping_ratio = 0.001;//刀具阻尼

    //s，仿真单步时间
    currentBroach->dt = currentBroach->step / (currentBroach->v_c * 1000 / 60) / DT_SUM;
    currentBroach->tool_angle = 0;
    currentBroach->deform_color_max = 500;//可视化最大位移
    currentBroach->deform_color_min = 0;//可视化最小位移

    //刀刃数量
    currentBroach->blade_sum = 0;

    return 1;
}

int CutsimBroaching::setVelocity(double vx, double vy, double vz)
{

    broaching_velocity[3] = sqrt(vx * vx + vy * vy + vz * vz);
    broaching_velocity[0] = vx / broaching_velocity[3];
    broaching_velocity[1] = vy / broaching_velocity[3];
    broaching_velocity[2] = vz / broaching_velocity[3];
    for (int i = 0; i < myTools.size(); i++)
    {
        //currentTool = 
        cutsim::AptCutterVolume* currentBroach = dynamic_cast<cutsim::AptCutterVolume*>(myTools[i]);
        currentBroach->v_c = 5;
        //刀具移动方向向量
        currentBroach->v_x = broaching_velocity[0];
        currentBroach->v_y = broaching_velocity[1];
        currentBroach->v_z = broaching_velocity[3];
    }
    return 1;
}

int CutsimBroaching::setVisulization(int *visulization_item, std::array<double, 2> visulization_limits)
{

    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::AptCutterVolume* currentBroach = dynamic_cast<cutsim::AptCutterVolume*>(myTools[currentTool]);
        currentBroach->deform_color_var = *visulization_item;//可视化最小位移
        currentBroach->deform_color_min = visulization_limits[0];//可视化最小位移
        currentBroach->deform_color_max = visulization_limits[1];//可视化最大位移
    }
    return 1;
}

int CutsimBroaching::performFEMSimulation(MdiChild *mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits)
{
    myBroachCutsim->updateGL();
    Handle(AIS_InteractiveObject) workdeformed = OcctViewer::getGraphic3d(gld);
    h_MyViewer->Display(workdeformed);
    h_MyViewer->Redraw();
    //peformModalAnalysis();
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::AptCutterVolume* s2 = dynamic_cast<cutsim::AptCutterVolume*>(myTools[currentTool]);
        s2->updatestockVibrParams();//初始化工件变形
        //mdichild->forcewidget->setData( s2->angle_total_force);
    }

    cutsim::AptCutterVolume* s2 = dynamic_cast<cutsim::AptCutterVolume*>(myTools[0]);
    peformModalAnalysis();
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::AptCutterVolume* currentBroach = dynamic_cast<cutsim::AptCutterVolume*>(myTools[currentTool]);
        currentBroach->updatestockVibrParams();//初始化工件变形
    }
    for (double t = 0; t <= simulation_time; t = t + incrementive_time )
    {
        setVisulization(visulization_item, visulization_limits);
        double x = 0;//更新刀具位移点
        double y = 0;
        double z = 0;
        for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
        {
            cutsim::AptCutterVolume* s2 = dynamic_cast<cutsim::AptCutterVolume*>(myTools[currentTool]);
            //double dx = s2->v_x * incrementive_time;
            //double dy = s2->v_y * incrementive_time;
            //double dz = s2->v_z * incrementive_time;
            x = 0 + s2->v_x * t;//更新刀具位移点
            y = 0 + s2->v_y * t;
            z = 0 + s2->v_z * t;
            s2->setCenter(cutsim::GLVertex(x, y, z));//更新刀具位置
            s2->tool_angle = s2->tool_angle + s2->step ;//更新刀具角度
            s2->tool_angle = std::round(s2->tool_angle * 1000) / 1000.0;;//保留小数点后三位
            s2->new_angle = s2->tool_angle + s2->step ;//更新刀具下一步的角度
            s2->new_angle = std::round(s2->new_angle * 1000) / 1000.0;//保留小数点后三位
            qDebug() << "tool_angle: " << s2->tool_angle;

            // 迭代计算：变形影响切削力，切削力影响变形
            const int max_iterations = 100;
            const double convergence_threshold = 0.001; // 收敛阈值,3N
            double force_change = 1e6; // 初始化为大值

            // 存储上一次的力数据用于收敛检查
            GLVertex previous_total_force;

            for (int iter = 0; iter<max_iterations && force_change > s2->temp_total_force.norm() * convergence_threshold; iter++)
            {
                qDebug() << "iter_num: " << iter;

                // 存储当前力数据用于下一次迭代的收敛检查
                previous_total_force = s2->temp_total_force;
                for (int i = 0; i < s2->blade_sum; i++)
                {
                    s2->blade_num = i;
                    s2->calculatePosition_balde();//更新当前刀刃离散点位置
                    myBroachCutsim->diff_volume_blade_cuda(s2);//材料去除
                    s2->calculateForceData();
                    s2->cut_h.clear();//清空切削厚度             
                }
                s2->calculateTotalForce();
                s2->outputForceData("data/Force");
                s2->calculatestockVibration();
                s2->calculateStraightness();
                s2->cut_h_map.clear();

                // 计算力变化量
                force_change = 0.0;
                force_change += (s2->temp_total_force - previous_total_force).norm();

                qDebug() << "Force change: " << force_change << ", Convergence threshold: " << s2->temp_total_force.norm() * convergence_threshold;
                if (force_change <= s2->temp_total_force.norm() * convergence_threshold) {
                    qDebug() << "Converged after " << iter + 1 << " iterations!";
                }
                qDebug() << "-----------------------\n";
            }
            double* Fx = new double(); double* Fy = new double(); double* Fz = new double();
            s2->getCurrentTotalForce(Fx, Fy, Fz);
            emit ApplyUpdateForces(sqrt(x* x + y * y + z * z), *Fx, *Fy, *Fz);
            Standard_Character Buffer[1024] = { 0 };
            Sprintf(Buffer, "Stroke: %f mm, Fx: %f N, Fy: %f N, Fz: %f N\n", sqrt(x* x + y * y + z * z), *Fx, *Fy, *Fz);
            Msg::ShowInfo(Buffer);
            //Msg::ShowInfo("Stroke:");
        }
        double dynamicColorMin = std::numeric_limits<double>::max();
        double dynamicColorMax = std::numeric_limits<double>::lowest();
        for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
        {
            cutsim::AptCutterVolume* currentBroach = dynamic_cast<cutsim::AptCutterVolume*>(myTools[currentTool]);
            if (!currentBroach)
            {
                continue;
            }
            dynamicColorMin = std::min(dynamicColorMin, currentBroach->deform_color_min);
            dynamicColorMax = std::max(dynamicColorMax, currentBroach->deform_color_max);
        }
        if (dynamicColorMin != std::numeric_limits<double>::max() &&
            dynamicColorMax != std::numeric_limits<double>::lowest())
        {
            visulization_limits[0] = dynamicColorMin;
            visulization_limits[1] = dynamicColorMax;
            mdichild->visulization_limits[0] = dynamicColorMin;
            mdichild->visulization_limits[1] = dynamicColorMax;
            emit ApplyUpdateColorBar(dynamicColorMin, dynamicColorMax);
        }
        myBroachCutsim->updateGL();
        h_MyViewer->Erase(workdeformed);
        workdeformed = OcctViewer::getGraphic3d(gld);
        h_MyViewer->Display(workdeformed);
        mdichild->MoveToolModel(true, x, y, z);
        emit ApplyUpdateViewer();
        //if (int(t / incrementive_time) % int(modalsteps / incrementive_time) == 0 && t > 0)
        //{
        //    peformModalAnalysis();
        //}
    }
    Msg::ShowInfo("Simulation done!");
    //cout << "fem finished" << endl;
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::AptCutterVolume* s2 = dynamic_cast<cutsim::AptCutterVolume*>(myTools[currentTool]);
        //plotStraightnessCurves(s2->straightness_map, { 30 });
        //plotStraightnessCurves(s2->straightness_map, { 161 });
        //plotStraightnessCurves(s2->straightness_map, { 337 });
        //plotStraightnessCurves(s2->straightness_map, { 468 });
        s2->outputStraightnessData("data/Straightness");
        s2->outputTotalForceData("data/Force");
        //plotRoughnessCurves(s2->ra_map, {3});
        //plotRoughnessCurves(s2->ra_map, {45});
        //plotRoughnessCurves(s2->ra_map, {95});
        //mdichild->forcewidget->setData(s2->angle_total_force);
    }
    return 1;
}

int CutsimBroaching::peformModalAnalysis()
{
    ///added by syj
    std::string meshFile = "data/tasat2.mesh";
    std::vector<GLVertex*> normalvertices;
    MeshIDExport(myBroachCutsim->tree, meshFile, normalvertices);//网格导出函数
    const MeshIdStats meshStats = readMeshIdStats(meshFile);
    qDebug() << "mesh coordinate count:" << meshStats.coordinateCount
        << "max coordinate id:" << meshStats.maxCoordinateId
        << "max element vertex id:" << meshStats.maxElementVertexId
        << "max boundary vertex id:" << meshStats.maxBoundaryVertexId
        << "max vertex_parent id:" << meshStats.maxVertexParentId
        << "max mesh vertex id:" << meshStats.maxMeshVertexId;
    cutsim::AptCutterVolume* s0 = dynamic_cast<cutsim::AptCutterVolume*>(myTools[0]);
    s0->normalvertices_size = normalvertices.size();
    std::vector<double> materialprops;
    materialprops.push_back(density); materialprops.push_back(youngsmodulus); materialprops.push_back(poisson);
    runEx12p(meshFile, materialprops, s0->vibration_values, s0->pre_vibration_vectors);//调用模态分析程序
    if (!s0->pre_vibration_vectors.empty()) {
        const size_t eigenNodeCount = s0->pre_vibration_vectors[0].size() / 3;
        qDebug() << "mesh max vertex id:" << meshStats.maxMeshVertexId
            << "eigen node count:" << eigenNodeCount
            << "max valid eigen node id:" << (eigenNodeCount > 0 ? eigenNodeCount - 1 : 0);
        if (meshStats.maxMeshVertexId >= static_cast<int>(eigenNodeCount)) {
            qDebug() << "ERROR: mesh vertex id exceeds eigenvector node count.";
        }
    }
    std::vector<size_t> target_modes = { 0,1,2,3,4,5};
    s0->vibration_vectors = s0->convertTo3DVibrationVectors(s0->pre_vibration_vectors, 3, target_modes);
    s0->updatestockVibrParams();
    for (int currentTool = 1; currentTool < myTools.size(); currentTool++)
    {
        cutsim::AptCutterVolume* s2 = dynamic_cast<cutsim::AptCutterVolume*>(myTools[currentTool]);
        s2->normalvertices_size = s0->normalvertices_size;
        s2->vibration_values = s0->vibration_values;
        s2->pre_vibration_vectors = s0->pre_vibration_vectors;
        s2->vibration_vectors = s2->convertTo3DVibrationVectors(s2->pre_vibration_vectors, 3, target_modes);
        s2->updatestockVibrParams();
        std::cout << "eigenvalues:" << s2->vibration_values.size() << std::endl;
        std::cout << "eigenvalue modes:" << s2->vibration_vectors.size()
            << "eigenvalue dofs:" << s2->vibration_vectors[0].size()
            << "eigenvalue nodes:" << s2->vibration_vectors[0][0].size()
            << std::endl;
    }

    return 1;
}

CutsimBroaching::~CutsimBroaching()
{
    delete myBroachCutsim;
    delete myGLWidget;
}
