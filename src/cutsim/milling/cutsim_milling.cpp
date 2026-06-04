//#include "cutsim_app.hpp"
#include "cutsim_milling.hpp"

#include <src/cutsim/cutsim/facet.hpp>
#include <src/cutsim/cutsim/volume.hpp>
#include <QEventLoop>
#include <QTimer>
#include <cmath>
#include <vector>
#include <array>

using milling::CutsimMilling;

CutsimMilling::CutsimMilling(int depth) {
    max_depth = depth;
    myGLWidget = new cutsim::GLWidget(DEFAULT_SCENE_RADIUS);
    gld = myGLWidget->addGLData();
    stockVolume = new StockVolume();
    octree_cube_size = 50;

    // ��ʼ��ѡ����ر���
    selectedBladeId = 0;
    selectedPointIndex = 0;
    selectionEnabled = false;
}
int CutsimMilling::setStlStock(QString file1Path, double partoffset[3], double octreecenter[3], double cube_size)
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
    myMillCutsim = new cutsim::Cutsim(octree_cube_size, max_depth, octree_center, gld, myGLWidget);
    //    myCutsim->init(4);

    //myCutsim->sum_volume_cuda(stock1, max_depth_sum);
    myMillCutsim->sum_volume(stockVolume->stock);
    myMillCutsim->updateGL();
    return error;
}
int CutsimMilling::setRectStock(double center[3], double cube_size)
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
    myMillCutsim = new cutsim::Cutsim(octree_cube_size, max_depth, octree_center, gld, myGLWidget);
    myMillCutsim->sum_volume_cuda(stock1, max_depth);
    myMillCutsim->updateGL();
    return 1;
}

// �����û�ѡ��ĵ��к͵�
void CutsimMilling::setSelectedBladePoint(int blade_id, int point_index)
{
    selectedBladeId = blade_id;
    selectedPointIndex = point_index;
    qDebug() << "Selected blade and point updated:" << blade_id << "," << point_index;
}

// ���û����ѡ����
void CutsimMilling::enableSelection(bool enabled)
{
    selectionEnabled = enabled;
    qDebug() << "Selection " << (enabled ? "enabled" : "disabled");
}
int CutsimMilling::setConstraints(std::array<std::array<double, 2>, 3> xyzconstraints) {
    myMillCutsim->setConstraints(xyzconstraints);
    return 1;
}

int CutsimMilling::addMill(QString file1Path)
{
    cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*> (myTools[0]);
    currentMill->readTestPointsFromFile(file1Path.toStdString());
    return 1;
}

int CutsimMilling::newMill(std::array<double,6> force_coefs)
{
    cutsim::milling_AptCutterVolume* currentMill = new cutsim::milling_AptCutterVolume();
    currentMill->max_depth_1 = max_depth; //����diff����������

    //diff�����������Ӧ�����سߴ�
    currentMill->cube_resolution_1 = octree_cube_size * 2.0 / pow(2.0, currentMill->max_depth_1 - 1);
    /*currentMill->cube_resolution_2 = octree_cube_size * 2.0 / pow(2.0, currentMill->max_depth_2 - 1);
    currentMill->inv_cube_resolution_2 = 1.0 / currentMill->cube_resolution_2;*/
    currentMill->type = cutsim::APT_VOLUME;
    currentMill->cuttertype = cutsim::APT;
    currentMill->setColor(0.5, 0.2, 0.1);
    myTools.push_back(currentMill);

    currentMill->force_coefs = force_coefs;

    //mm,������С����
    currentMill->step = incrementive_time;

    // 注意：这里v_x, v_y, v_z, spindle_speed, blade_sum等变量可能还未设置
    // 将在后续的setVelocity, setSpindleSpeed, setBlade_Sum函数中设置
    // 这些计算将在performFEMSimulation中重新进行

    // Newmark������ƽ�����ٶȷ���
    currentMill->vibr_beta = 0.25;
    currentMill->vibr_gamma = 0.5;
    currentMill->stock_vibr_damping_ratio = 0.001;//��������

    currentMill->tool_angle = 0;
    currentMill->deform_color_max = 0.01;//���ӻ����λ��
    currentMill->deform_color_min = 0;//���ӻ���Сλ��

    return 1;
}

int CutsimMilling::setVelocity(double vx, double vy, double vz)
{

    for (int i = 0; i < myTools.size(); i++)
    {
        //currentTool = 
        cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[i]);
        //�����ƶ���������
        currentMill->v_x = vx;
        currentMill->v_y = vy;
        currentMill->v_z = vz;
    }
    return 1;
}

int CutsimMilling::setSpindleSpeed(double spindle_speed)
{
    for (int i = 0; i < myTools.size(); i++)
    {
        //currentTool = 
        cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[i]);
        currentMill->spindle_speed = spindle_speed;
    }
    return 1;
}

int CutsimMilling::setBlade_Sum(double blade_sum)
{
    for (int i = 0; i < myTools.size(); i++)
    {
        cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[i]);
        currentMill->blade_sum = blade_sum;
    }
    return 1;
}


int CutsimMilling::setVisulization(int* visulization_item, std::array<double, 2> visulization_limits)
{

    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[currentTool]);
        currentMill->deform_color_var = *visulization_item;//���ӻ���Сλ��
        currentMill->deform_color_min = visulization_limits[0];//���ӻ���Сλ��
        currentMill->deform_color_max = visulization_limits[1];//���ӻ����λ��
    }
    return 1;
}

int CutsimMilling::performFEMSimulation(MdiChild* mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits)
{
    myMillCutsim->updateGL();
    Handle(AIS_InteractiveObject) workdeformed = OcctViewer::getGraphic3d(gld);
    h_MyViewer->Display(workdeformed);
    h_MyViewer->Redraw();
    //peformModalAnalysis();
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::milling_AptCutterVolume* s1 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[currentTool]);
        s1->updatestockVibrParams();//��ʼ����������
        //mdichild->forcewidget->setData( s1->angle_total_force);
    }

    cutsim::milling_AptCutterVolume* s1 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[0]);
    
    // 在仿真开始前重新计算依赖于速度、主轴转速等参数的变量
    s1->dx = s1->v_x * s1->step;
    s1->dy = s1->v_y * s1->step;
    s1->dz = s1->v_z * s1->step;
    s1->angle_dx = 0.0;
    s1->angle_dy = 0.0;
    s1->angle_dz = 0.0;
    s1->setAngle(cutsim::GLVertex(0.0, 0.0, 0.0));
    s1->step = incrementive_time;
    s1->dt = s1->step / (s1->spindle_speed * 2 * M_PI / 60.0);
    peformModalAnalysis();
    s1->initstockVibration();
    for (double t = 0; t <= simulation_time; t = t + incrementive_time)
    {
        double x = 6.5 + s1->v_x * t / (2 * M_PI);
        double y = -13.5 + s1->v_y * t / (2 * M_PI);
        double z = 0.0;
        s1->dx = s1->v_x * s1->step / (2 * M_PI);
        s1->dy = s1->v_y * s1->step / (2 * M_PI);
        s1->dz = s1->v_z * s1->step / (2 * M_PI);
        s1->setCenter(cutsim::GLVertex(x, y, z));
        s1->tool_angle = s1->tool_angle + s1->step;
        // 迭代计算：变形影响切削力，切削力影响变形
        const int max_iterations = 10;
        const double convergence_threshold = 0.01; // 收敛阈值,3N
        double force_change = 1e6; // 初始化为大值

        // 存储上一次的力数据用于收敛检查
        GLVertex previous_total_force;
        for (int iter = 0; iter<max_iterations && force_change > convergence_threshold; iter++)
        {
            qDebug() << "iter_num: " << iter;

            // 存储当前力数据用于下一次迭代的收敛检查
            previous_total_force = s1->temp_total_force;
        for (int n = 0; n < s1->blade_sum; n = n + 1)
        {
            s1->blade_num = n;//�������
            s1->blade_angle = s1->tool_angle - n * 2 * M_PI / s1->blade_sum;
            s1->calculatePosition_balde();
            myMillCutsim->milling_diff_volume_blade_cuda(s1);
            s1->calculateForceData();
            s1->cut_h.clear();
            qDebug() << "blade_angle: " << s1->blade_angle;
        }
        s1->calculateTotalForce();
        s1->calculatestockVibration();
        // 计算力变化量
        force_change = 0.0;
        force_change += (s1->temp_total_force - previous_total_force).norm();

        qDebug() << "Force change: " << force_change << ", Convergence threshold: " << s1->temp_total_force.norm() * convergence_threshold;
        if (force_change <= s1->temp_total_force.norm() * convergence_threshold) {
            qDebug() << "Converged after " << iter + 1 << " iterations!";
        }
    }
    if (fabs(fmod(s1->tool_angle, 1000.0)) < s1->step)//每1000弧度记录数据
    {
        //plotForceCurves(s1->angle_total_force);
        s1->outputTotalForceData("src/data/Force");
        //s1->outputSurfaceData("src/data/Surface");
    }
    //s1->outputForceData("src/data/Force");
    qDebug() << "-----------------------\n";
    double* Fx = new double(); double* Fy = new double(); double* Fz = new double();
    s1->getCurrentTotalForce(Fx, Fy, Fz);
    emit ApplyUpdateForces(sqrt(x * x + y * y + z * z), *Fx, *Fy, *Fz);
    Standard_Character Buffer[1024] = { 0 };
    Sprintf(Buffer, "Stroke: %f mm, Fx: %f N, Fy: %f N, Fz: %f N\n", sqrt(x * x + y * y + z * z), *Fx, *Fy, *Fz);
    Msg::ShowInfo(Buffer);
    myMillCutsim->updateGL();
    h_MyViewer->Erase(workdeformed);
    workdeformed = OcctViewer::getGraphic3d(gld);
    h_MyViewer->Display(workdeformed);
    mdichild->MoveToolModel(true, x, y, z,0,0,0);
    emit ApplyUpdateViewer();
    }
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[currentTool]);
        currentMill->updatestockVibrParams();//��ʼ����������
    }
    Msg::ShowInfo("Simulation done!");
    //cout << "fem finished" << endl;
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::milling_AptCutterVolume* s2 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[currentTool]);
        //plotStraightnessCurves(s2->straightness_map, { 30 });
        //plotStraightnessCurves(s2->straightness_map, { 161 });
        //plotStraightnessCurves(s2->straightness_map, { 337 });
        //plotStraightnessCurves(s2->straightness_map, { 468 });
        s2->outputSurfaceData("data/Surface");
        s2->outputTotalForceData("data/Force");
        //plotRoughnessCurves(s2->ra_map, {3});
        //plotRoughnessCurves(s2->ra_map, {45});
        //plotRoughnessCurves(s2->ra_map, {95});
        //mdichild->forcewidget->setData(s2->angle_total_force);
    }
    return 1;
}

int CutsimMilling::peformModalAnalysis()
{
    ///added by syj
    std::string meshFile = "data/tasat2.mesh";
    std::vector<GLVertex*> normalvertices;
    MeshIDExport(myMillCutsim->tree, meshFile, normalvertices);//���񵼳�����

    cutsim::milling_AptCutterVolume* s0 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[0]);
    s0->normalvertices_size = normalvertices.size();
    std::vector<double> materialprops;
    materialprops.push_back(density); materialprops.push_back(youngsmodulus); materialprops.push_back(poisson);
    runEx12p(meshFile, materialprops, s0->vibration_values, s0->pre_vibration_vectors);//����ģ̬��������
    std::vector<size_t> target_modes = { 0,1,2,3,4,5 };
    s0->vibration_vectors = s0->convertTo3DVibrationVectors(s0->pre_vibration_vectors, 3, target_modes);
    s0->updatestockVibrParams();
    for (int currentTool =1; currentTool < myTools.size(); currentTool++)
    {
        cutsim::milling_AptCutterVolume* s2 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[currentTool]);
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

CutsimMilling::~CutsimMilling()
{
    delete myMillCutsim;
    delete myGLWidget;
}
