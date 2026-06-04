#include <Windows.h>

//#include "cutsim_app.hpp"
#include "digitaltwin_milling.hpp"

#include <src/cutsim/cutsim/facet.hpp>
#include <src/cutsim/cutsim/volume.hpp>
#include <src/cutsim/cutsim/marching_cubes.hpp>
#include <src/cutsim/cutsim/octnode.hpp>
#include <QEventLoop>
#include <QTimer>
#include <cmath>
#include <vector>
#include <array>
#include <unordered_map>

using digitaltwin_milling::DigitalTwinMilling;

DigitalTwinMilling::DigitalTwinMilling(int depth) {
    max_depth = depth;
    myGLWidget = new cutsim::GLWidget(DEFAULT_SCENE_RADIUS);
    gld = myGLWidget->addGLData();
    stockVolume = new StockVolume();
    octree_cube_size = 50;

    selectedBladeId = 0;
    selectedPointIndex = 0;
    selectionEnabled = false;
}

// 提取Marching Cubes算法重建后的表面并计算表面中心点
void DigitalTwinMilling::extractSurfaceAndCenter() {
    for (int i = 0; i < myTools.size(); i++)
    {
        cutsim::digitaltwin_AptCutterVolume* currentMill = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[i]);
        if (!myMillDigitalTwin) {
            qDebug() << "Error: myMillDigitalTwin is not initialized";
            return;
        }

        // 获取八叉树
        cutsim::Octree* tree = myMillDigitalTwin->tree;
        if (!tree) {
            qDebug() << "Error: Octree is not available";
            return;
        }

        // 获取Marching Cubes算法实例
        cutsim::MarchingCubes* mc = dynamic_cast<cutsim::MarchingCubes*>(myMillDigitalTwin->iso_algo);
        if (!mc) {
            qDebug() << "Error: Marching Cubes algorithm not available";
            return;
        }

        // 直接使用myMillDigitalTwin中的nodes_to_process
        const std::vector<cutsim::Octnode*>& nodes_to_process = myMillDigitalTwin->nodes_to_process;

        qDebug() << "Processing" << nodes_to_process.size() << "nodes for surface extraction";

        // 存储所有表面顶点
        std::vector<cutsim::GLVertex> surfaceVertices;

        // 处理每个节点，提取表面顶点
        for (cutsim::Octnode* node : nodes_to_process) {
            if (!node) continue;

            // 检查节点是否包含表面（有符号距离场穿过零等值面）
            if (node->is_undecided()) {
                // 使用Marching Cubes算法提取该节点的表面顶点
                mc->renderNode(node);

                // 从vertexMap中获取该节点的顶点
                auto it = mc->vertexMap.find(node);
                if (it != mc->vertexMap.end()) {
                    const std::vector<cutsim::GLVertex>& nodeVertices = it->second;

                    // 筛选距离刀具表面足够近的点
                    for (int j = 0; j < 8; j++) {
                        if (node->updated[j] && node->f[j] < currentMill->cube_resolution_1 && node->vertex[j]) {
                            surfaceVertices.push_back(*node->vertex[j]);
                        }
                    }
                }
            }
        }

        // 计算表面中心点
        if (!surfaceVertices.empty()) {
            double centerX = 0.0, centerY = 0.0, centerZ = 0.0;

            for (const cutsim::GLVertex& vertex : surfaceVertices) {
                centerX += vertex.x;
                centerY += vertex.y;
                centerZ += vertex.z;
            }

            centerX /= surfaceVertices.size();
            centerY /= surfaceVertices.size();
            centerZ /= surfaceVertices.size();

            // 找出距离中心点最近的节点顶点ID
            int closestVertexId = -1;
            double minDistance = std::numeric_limits<double>::max();
            cutsim::Octnode* closestNode = nullptr;
            int closestVertexIndex = -1;

            // 遍历所有需要处理的节点
            for (cutsim::Octnode* node : nodes_to_process) {
                if (!node) continue;

                // 检查节点的8个顶点
                for (int j = 0; j < 8; j++) {
                    if (node->vertex[j]) {
                        // 计算顶点到中心点的距离
                        double dx = node->vertex[j]->x - centerX;
                        double dy = node->vertex[j]->y - centerY;
                        double dz = node->vertex[j]->z - centerZ;
                        double distance = sqrt(dx * dx + dy * dy + dz * dz);

                        // 更新最小距离和对应的顶点ID
                        if (distance < minDistance) {
                            minDistance = distance;
                            closestVertexId = node->vertex[j]->id;
                            closestNode = node;
                            closestVertexIndex = j;
                        }
                    }
                }
            }

            currentMill->has_surface = true;
            currentMill->surfaceCenter_id = closestVertexId;
            currentMill->surfaceCenter.x = closestNode->vertex[closestVertexIndex]->x;
            currentMill->surfaceCenter.y = closestNode->vertex[closestVertexIndex]->y;
            currentMill->surfaceCenter.z = closestNode->vertex[closestVertexIndex]->z;


            // 存储中心点信息供后续使用
            surfaceCenter = cutsim::GLVertex(static_cast<float>(centerX), static_cast<float>(centerY), static_cast<float>(centerZ));
        }
        else {
            currentMill->has_surface = false;
            currentMill->surfaceCenter_id = 0;
            currentMill->surfaceCenter.x = 0;
            currentMill->surfaceCenter.y = 0;
            currentMill->surfaceCenter.z = 0;
            qDebug() << "No surface vertices found";
        }
    }
}
int DigitalTwinMilling::setStlStock(QString file1Path, double partoffset[3], double octreecenter[3], double cube_size)
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
    myMillDigitalTwin = new cutsim::Cutsim(octree_cube_size, max_depth, octree_center, gld, myGLWidget);
    myMillDigitalTwin->sum_stl_volume_cuda(stock2, max_depth);
    myMillDigitalTwin->updateGL();
    return error;
}
int DigitalTwinMilling::setRectStock(double center[3], double cube_size)
{
    octree_cube_size = cube_size;
    octree_center = new cutsim::GLVertex(center[0], center[1], center[2]);
    cutsim::RectVolume* stock1 = new cutsim::RectVolume();
    stock1->setColor(PARTS_COLOR);
    stock1->setlengthX(20);
    stock1->setlengthY(20);
    stock1->setlengthZ(20);
    stock1->setCenter(*octree_center);
    stock1->calcBB();

    stockVolume->stock = stock1;
    stockVolume->operation = SUM_OPERATION;
    myStocks.push_back(stockVolume);
    myMillDigitalTwin = new cutsim::Cutsim(octree_cube_size, max_depth, octree_center, gld, myGLWidget);
    myMillDigitalTwin->sum_volume_cuda(stock1, max_depth);
    myMillDigitalTwin->updateGL();
    return 1;
}


void DigitalTwinMilling::enableSelection(bool enabled)
{
    selectionEnabled = enabled;
    qDebug() << "Selection " << (enabled ? "enabled" : "disabled");
}
int DigitalTwinMilling::setConstraints(std::array<std::array<double, 2>, 3> xyzconstraints) {
    myMillDigitalTwin->setConstraints(xyzconstraints);
    return 1;
}

int DigitalTwinMilling::newMill()
{
    cutsim::digitaltwin_AptCutterVolume* currentMill = new cutsim::digitaltwin_AptCutterVolume();
    currentMill->max_depth_1 = max_depth;

    currentMill->cube_resolution_1 = octree_cube_size * 2.0 / pow(2.0, currentMill->max_depth_1 - 1);
    currentMill->type = cutsim::APT_VOLUME;
    currentMill->cuttertype = cutsim::APT;
    currentMill->setColor(PARTS_COLOR);
    myTools.push_back(currentMill);

    currentMill->step = incrementive_time;

    currentMill->vibr_beta = 0.25;
    currentMill->vibr_gamma = 0.5;
    currentMill->stock_vibr_damping_ratio = 0.001;

    currentMill->tool_angle = 0;
    currentMill->deform_color_max = 0.07;
    currentMill->deform_color_min = 0;

    myMillDigitalTwin->updateGL();
    workdeformed = OcctViewer::getGraphic3d(gld);

    return 1;
}

int DigitalTwinMilling::setSpindleSpeed(double spindle_speed)
{
    for (int i = 0; i < myTools.size(); i++)
    {
        cutsim::digitaltwin_AptCutterVolume* currentMill = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[i]);
        currentMill->spindle_speed = spindle_speed;
    }
    return 1;
}



int DigitalTwinMilling::setVisulization(int* visulization_item, std::array<double, 2> visulization_limits)
{

    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::digitaltwin_AptCutterVolume* currentMill = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[currentTool]);
        currentMill->deform_color_var = *visulization_item;
        currentMill->deform_color_min = visulization_limits[0];
        currentMill->deform_color_max = visulization_limits[1];
    }
    return 1;
}

int DigitalTwinMilling::setMch_Data(std::array<double, 14>Msh_Data, std::vector<std::array<double, 3>>force_data)
{
    for (int i = 0; i < myTools.size(); i++)
    {
        cutsim::digitaltwin_AptCutterVolume* currentMill = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[i]);
        currentMill->spindle_speed = Msh_Data[0];
        currentMill->step = Msh_Data[1];
        currentMill->now_x = Msh_Data[2];
        currentMill->now_y = Msh_Data[3];
        currentMill->now_z = Msh_Data[4];
        currentMill->pre_x = Msh_Data[5];
        currentMill->pre_y = Msh_Data[6];
        currentMill->pre_z = Msh_Data[7];
        currentMill->A = Msh_Data[8];
        currentMill->B = Msh_Data[9];
        currentMill->C = Msh_Data[10];
        currentMill->pre_A = Msh_Data[11];
        currentMill->pre_B = Msh_Data[12];
        currentMill->pre_C = Msh_Data[13];
        currentMill->force_data_vector = force_data;
        //currentMill->spindle_speed = 1100;
        //currentMill->step = 3.14;
        //currentMill->now_x = 6.8;
        //currentMill->now_y = -18;
        //currentMill->now_z = 15;
        //currentMill->pre_x = 6.8;
        //currentMill->pre_y = 18;
        //currentMill->pre_z = 15;
        //currentMill->A = 0;
        //currentMill->B = 0;
        //currentMill->C = 0;
        //currentMill->pre_A = 0;
        //currentMill->pre_B = 0;
        //currentMill->pre_C = 0;
        //currentMill->force_data_vector.assign(10, { 100.0, 100.0, 0.0 });

        qDebug() << "aaaaaaa" << Msh_Data[0] << Msh_Data[1] << Msh_Data[2] << Msh_Data[3] << Msh_Data[4] << Msh_Data[8] << Msh_Data[9] << Msh_Data[10];

    }
    return 1;
}

int DigitalTwinMilling::setCutterParameters(const std::vector<Segment>& segments) {
    for (int i = 0; i < myTools.size(); i++) {
        cutsim::digitaltwin_AptCutterVolume* currentMill = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[i]);
        currentMill->segments.clear();
        for (const auto& seg : segments) {
            cutsim::digitaltwin_AptCutterVolume::Segment targetSeg(
                static_cast<cutsim::digitaltwin_AptCutterVolume::Segment::Type>(seg.type),
                seg.radius1,
                seg.radius2,
                seg.length,
                seg.z_start,
                seg.z_end
            );
            targetSeg.center = seg.center;
            currentMill->segments.push_back(targetSeg);
        }
        currentMill->calcBB();
    }
    return 1;
}

int DigitalTwinMilling::setVibrParams() {
    for (int i = 0; i < myTools.size(); i++) {
        cutsim::digitaltwin_AptCutterVolume* currentMill = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[i]);
        currentMill->updatestockVibrParams();
        currentMill->initstockVibration();
    }
    return 1;
}

int DigitalTwinMilling::performFEMSimulation(MdiChild* mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits)
{

    cutsim::digitaltwin_AptCutterVolume* s1 = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[0]);

    s1->dt = s1->step / (s1->spindle_speed * 2 * M_PI / 60.0);
    //peformModalAnalysis();

    int step_length = static_cast<int>(s1->force_data_vector.size());
    for (int step_num = 0; step_num <= step_length - 1; step_num++)
    {
        double t = static_cast<double>(step_num) / step_length;
        double x = s1->pre_x + t * (s1->now_x - s1->pre_x);
        double y = s1->pre_y + t * (s1->now_y - s1->pre_y);
        double z = s1->pre_z + t * (s1->now_z - s1->pre_z);
        s1->setCenter(cutsim::GLVertex(x, y, z));
        double a = s1->pre_A + t * (s1->A - s1->pre_A);
        double b = s1->pre_B + t * (s1->B - s1->pre_B);
        double c = s1->pre_C + t * (s1->C - s1->pre_C);
        s1->setAngle(cutsim::GLVertex(a, b, c));
        s1->tool_angle = s1->tool_angle + s1->step;
        s1->force_data = s1->force_data_vector[step_num];
        myMillDigitalTwin->digitaltwin_milling_diff_volume_blade_cuda(s1);
        extractSurfaceAndCenter();  // 新增：提取表面和中心点
        qDebug() << "tool_angle: " << s1->tool_angle;
        s1->calculateTotalForce();
        s1->calculatestockVibration();
        qDebug() << "-----------------------\n";
        double* Fx = new double(); double* Fy = new double(); double* Fz = new double();
        emit ApplyUpdateForces(s1->tool_angle, s1->force_xyz[0], s1->force_xyz[1], s1->force_xyz[2]);
        Standard_Character Buffer[1024] = { 0 };
        Sprintf(Buffer, "Stroke: %f mm, Fx: %f N, Fy: %f N, Fz: %f N\n", s1->tool_angle, *Fx, *Fy, *Fz);
        Msg::ShowInfo(Buffer);
        mdichild->MoveToolModel(true, x, y, z, a, b, c);
    }
    myMillDigitalTwin->updateGL();
    h_MyViewer->Erase(workdeformed);
    workdeformed = OcctViewer::getGraphic3d(gld);
    h_MyViewer->Display(workdeformed);
    emit ApplyUpdateViewer();
    Msg::ShowInfo("Simulation done!");
    return 1;
}

int DigitalTwinMilling::performFEMSimulation_test(MdiChild* mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits)
{
    myMillDigitalTwin->updateGL();
    Handle(AIS_InteractiveObject) workdeformed = OcctViewer::getGraphic3d(gld);
    h_MyViewer->Display(workdeformed);
    h_MyViewer->Redraw();
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::digitaltwin_AptCutterVolume* s1 = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[currentTool]);
        s1->updatestockVibrParams();
    }

    cutsim::digitaltwin_AptCutterVolume* s1 = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[0]);

    s1->dt = s1->step / (s1->spindle_speed * 2 * M_PI / 60.0);
    peformModalAnalysis();
    s1->initstockVibration();
    int step_length = static_cast<int>(s1->force_data_vector.size());
    for (int step_num = 0; step_num <= step_length - 1; step_num++)
    {
        double t = static_cast<double>(step_num) / step_length;
        double x = s1->pre_x + t * (s1->now_x - s1->pre_x);
        double y = s1->pre_y + t * (s1->now_y - s1->pre_y);
        double z = s1->pre_z + t * (s1->now_z - s1->pre_z);
        s1->setCenter(cutsim::GLVertex(x, y, z));
        double a = s1->pre_A + t * (s1->A - s1->pre_A);
        double b = s1->pre_B + t * (s1->B - s1->pre_B);
        double c = s1->pre_C + t * (s1->C - s1->pre_C);
        s1->setAngle(cutsim::GLVertex(a, b, c));
        s1->tool_angle = s1->tool_angle + s1->step;
        s1->force_data = s1->force_data_vector[step_num];
        myMillDigitalTwin->digitaltwin_milling_diff_volume_blade_cuda(s1);
        extractSurfaceAndCenter();  // 新增：提取表面和中心点
        qDebug() << "tool_angle: " << s1->tool_angle;
        s1->calculateTotalForce();
        s1->calculatestockVibration();
        qDebug() << "-----------------------\n";
        //double* Fx = new double(); double* Fy = new double(); double* Fz = new double();
        //emit ApplyUpdateForces(sqrt(x * x + y * y + z * z), s1->force_xyz[0], s1->force_xyz[1], s1->force_xyz[2]);
        //Standard_Character Buffer[1024] = { 0 };
        //Sprintf(Buffer, "Stroke: %f mm, Fx: %f N, Fy: %f N, Fz: %f N\n", sqrt(x * x + y * y + z * z), *Fx, *Fy, *Fz);
        //Msg::ShowInfo(Buffer);
        myMillDigitalTwin->updateGL();
        h_MyViewer->Erase(workdeformed);
        workdeformed = OcctViewer::getGraphic3d(gld);
        h_MyViewer->Display(workdeformed);
        mdichild->MoveToolModel(true, x, y, z, a, b, c);
        emit ApplyUpdateViewer();
    }
    Msg::ShowInfo("Simulation done!");
    return 1;
}


int DigitalTwinMilling::peformModalAnalysis()
{
    std::string meshFile = "data/tasat2.mesh";
    std::vector<GLVertex*> normalvertices;
    MeshIDExport(myMillDigitalTwin->tree, meshFile, normalvertices);

    cutsim::digitaltwin_AptCutterVolume* s0 = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[0]);
    s0->normalvertices_size = normalvertices.size();
    std::vector<double> materialprops;
    materialprops.push_back(density); materialprops.push_back(youngsmodulus); materialprops.push_back(poisson);
    runEx12p(meshFile, materialprops, s0->vibration_values, s0->pre_vibration_vectors);
    std::vector<size_t> target_modes = { 0,1,2,3,4,5 };
    s0->vibration_vectors = s0->convertTo3DVibrationVectors(s0->pre_vibration_vectors, 3, target_modes);
    s0->updatestockVibrParams();
    for (int currentTool = 1; currentTool < myTools.size(); currentTool++)
    {
        cutsim::digitaltwin_AptCutterVolume* s2 = dynamic_cast<cutsim::digitaltwin_AptCutterVolume*>(myTools[currentTool]);
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

DigitalTwinMilling::~DigitalTwinMilling()
{
    delete myMillDigitalTwin;
    delete myGLWidget;
}
