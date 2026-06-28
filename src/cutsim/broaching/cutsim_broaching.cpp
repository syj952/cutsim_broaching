//#include "cutsim_app.hpp"
#include "cutsim_broaching.hpp"

#include <src/cutsim/cutsim/facet.hpp>
#include <src/cutsim/cutsim/volume.hpp>
#include <QEventLoop>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaObject>
#include <QTextStream>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>
#include <array>

using broaching::CutsimBroaching;

namespace {

QJsonArray vertexToJson(const cutsim::GLVertex& vertex)
{
    QJsonArray values;
    values.append(vertex.x);
    values.append(vertex.y);
    values.append(vertex.z);
    return values;
}

bool looksLikeBladePointRow(const QString& line)
{
    std::istringstream iss(line.toStdString());
    std::vector<double> values;
    double val = 0.0;

    while (iss >> val) {
        values.push_back(val);
    }

    return values.size() >= 12;
}

QJsonArray denseSigmaXxProfile()
{
    const std::array<std::array<double, 2>, 21> points = { {
        { 0.00, -120.0 },
        { 0.05, -193.0 },
        { 0.10, -242.0 },
        { 0.15, -260.0 },
        { 0.20, -245.0 },
        { 0.25, -208.0 },
        { 0.30, -162.0 },
        { 0.35, -120.0 },
        { 0.40,  -81.0 },
        { 0.45,  -40.0 },
        { 0.50,    1.0 },
        { 0.55,   35.0 },
        { 0.60,   60.0 },
        { 0.65,   78.0 },
        { 0.70,   94.0 },
        { 0.75,  108.0 },
        { 0.80,  117.0 },
        { 0.85,  120.0 },
        { 0.90,  102.0 },
        { 0.95,   58.0 },
        { 1.00,    0.0 }
    } };

    QJsonArray jsonPoints;
    for (const auto& point : points) {
        QJsonObject jsonPoint;
        jsonPoint["r"] = point[0];
        jsonPoint["sigma_xx_mpa"] = point[1];
        jsonPoints.append(jsonPoint);
    }
    return jsonPoints;
}

QJsonObject machiningResidualStressTemplate(bool enabled)
{
    QJsonObject components;
    components["sigma_xx_enabled"] = true;
    components["sigma_yy_enabled"] = false;
    components["sigma_zz_enabled"] = false;
    components["tau_xy_enabled"] = false;
    components["tau_xz_enabled"] = false;
    components["tau_yz_enabled"] = false;

    QJsonArray localStressOrder;
    localStressOrder.append("sigma_xx");
    localStressOrder.append("sigma_yy");
    localStressOrder.append("sigma_zz");
    localStressOrder.append("tau_xy");
    localStressOrder.append("tau_xz");
    localStressOrder.append("tau_yz");

    QJsonObject profile;
    profile["type"] = "piecewise_linear";
    profile["interpolation"] = "linear";
    profile["depth_coordinate"] = "r=d/H";
    profile["layer_depth_mm"] = 0.30;
    profile["points"] = denseSigmaXxProfile();

    QJsonObject stress;
    stress["enabled"] = enabled;
    stress["local_stress_order"] = localStressOrder;
    stress["stress_components"] = components;
    stress["profile"] = profile;
    return stress;
}

QJsonObject contactEventToJson(const cutsim::broaching_AptCutterVolume::MachiningContactEvent& event)
{
    QJsonObject localFrame;
    localFrame["x"] = vertexToJson(event.local_x);
    localFrame["y"] = vertexToJson(event.local_y);
    localFrame["z"] = vertexToJson(event.local_z);

    QJsonObject object;
    object["step_id"] = event.step_id;
    object["stroke_mm"] = event.stroke_mm;
    object["tool_angle"] = event.tool_angle;
    object["blade_id"] = event.blade_id;
    object["inside_index"] = event.inside_index;
    object["surface_p0_mm"] = vertexToJson(event.surface_p0_mm);
    object["surface_p1_mm"] = vertexToJson(event.surface_p1_mm);
    object["local_frame"] = localFrame;
    object["sweep_width_mm"] = event.sweep_width_mm;
    object["layer_depth_mm"] = event.layer_depth_mm;
    return object;
}

void appendModalVectorStats(const QString& filePath,
                            int runIndex,
                            double toolAngle,
                            double newAngle,
                            const std::vector<double>& eigenvalues,
                            const std::vector<std::vector<double>>& rawVectors,
                            const std::vector<std::vector<std::vector<double>>>& usedVectors)
{
    const QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.absolutePath());

    const bool writeHeader = !fileInfo.exists() || fileInfo.size() == 0;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Cannot write modal vector stats:" << filePath;
        return;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(12);

    if (writeHeader) {
        out << "run_index\ttool_angle\tnew_angle\tmode\teigenvalue"
            << "\traw_count\traw_max_abs\traw_rms"
            << "\tused_count\tused_max_abs\tused_rms"
            << "\tused_max_abs_x\tused_max_abs_y\tused_max_abs_z\n";
    }

    const size_t modeCount = std::max(rawVectors.size(), usedVectors.size());
    for (size_t mode = 0; mode < modeCount; ++mode) {
        double rawMaxAbs = 0.0;
        double rawSumSq = 0.0;
        size_t rawCount = 0;
        if (mode < rawVectors.size()) {
            for (double value : rawVectors[mode]) {
                const double absValue = std::abs(value);
                rawMaxAbs = std::max(rawMaxAbs, absValue);
                rawSumSq += value * value;
                ++rawCount;
            }
        }

        double usedMaxAbs = 0.0;
        double usedMaxAbsByDof[3] = { 0.0, 0.0, 0.0 };
        double usedSumSq = 0.0;
        size_t usedCount = 0;
        if (mode < usedVectors.size()) {
            for (size_t dof = 0; dof < usedVectors[mode].size(); ++dof) {
                for (double value : usedVectors[mode][dof]) {
                    const double absValue = std::abs(value);
                    usedMaxAbs = std::max(usedMaxAbs, absValue);
                    if (dof < 3) {
                        usedMaxAbsByDof[dof] = std::max(usedMaxAbsByDof[dof], absValue);
                    }
                    usedSumSq += value * value;
                    ++usedCount;
                }
            }
        }

        const double eigenvalue = mode < eigenvalues.size() ? eigenvalues[mode] : 0.0;
        const double rawRms = rawCount > 0 ? std::sqrt(rawSumSq / static_cast<double>(rawCount)) : 0.0;
        const double usedRms = usedCount > 0 ? std::sqrt(usedSumSq / static_cast<double>(usedCount)) : 0.0;

        out << runIndex << '\t'
            << toolAngle << '\t'
            << newAngle << '\t'
            << mode << '\t'
            << eigenvalue << '\t'
            << static_cast<qulonglong>(rawCount) << '\t'
            << rawMaxAbs << '\t'
            << rawRms << '\t'
            << static_cast<qulonglong>(usedCount) << '\t'
            << usedMaxAbs << '\t'
            << usedRms << '\t'
            << usedMaxAbsByDof[0] << '\t'
            << usedMaxAbsByDof[1] << '\t'
            << usedMaxAbsByDof[2] << '\n';
    }
}

bool writeResidualStressConfigSnapshot(const QString& baseConfigPath,
                                       const QString& snapshotPath,
                                       const std::vector<cutsim::CutterVolume*>& tools,
                                       int stepId,
                                       double strokeMm)
{
    QJsonObject root;
    QFile baseFile(baseConfigPath);
    if (baseFile.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        const QJsonDocument baseDoc = QJsonDocument::fromJson(baseFile.readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError && baseDoc.isObject()) {
            root = baseDoc.object();
        }
        else {
            qDebug() << "Cannot parse base residual stress config:" << baseConfigPath
                     << parseError.errorString();
        }
    }
    else {
        qDebug() << "Base residual stress config not found, creating machining-only config:"
                 << baseConfigPath;
    }

    QJsonArray events;
    for (cutsim::CutterVolume* tool : tools) {
        auto* broach = dynamic_cast<cutsim::broaching_AptCutterVolume*>(tool);
        if (!broach) {
            continue;
        }
        for (const auto& event : broach->machining_contact_events) {
            events.append(contactEventToJson(event));
        }
    }

    root["step_id"] = stepId;
    root["stroke_mm"] = strokeMm;
    root["machining_residual_stress"] = machiningResidualStressTemplate(!events.isEmpty());
    root["machining_event_count"] = events.size();
    root["machining_events"] = events;

    qDebug() << "Writing residual stress config snapshot:"
             << snapshotPath
             << "step" << stepId
             << "stroke_mm" << strokeMm
             << "machining_event_count" << events.size();

    const QFileInfo snapshotInfo(snapshotPath);
    QDir().mkpath(snapshotInfo.absolutePath());

    QFile snapshotFile(snapshotPath);
    if (!snapshotFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "Cannot write residual stress config snapshot:" << snapshotPath;
        return false;
    }
    snapshotFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

}

int CutsimBroaching::performResidualRelease(int stepId, double stroke_mm)
{
    const QString projectRoot = QString::fromStdString(residualReleaseProjectRoot());
    const std::string meshFile =
        QDir(projectRoot).filePath("data/tasat2.mesh").toStdString();
    const QString baseStressConfigPath =
        QDir(projectRoot).filePath("data/residual_stress_config.json");
    const QString stressConfigSnapshotPath =
        QDir(projectRoot).filePath(QString("data/residual/step_%1_stress_config.json")
                                       .arg(stepId, 4, 10, QChar('0')));

    std::vector<cutsim::GLVertex*> normalvertices;
    MeshIDExport(myBroachCutsim->tree, meshFile, normalvertices);

    const QString outputPrefix =
        QDir(projectRoot).filePath(QString("data/residual/step_%1")
                                       .arg(stepId, 4, 10, QChar('0')));

    if (!writeResidualStressConfigSnapshot(baseStressConfigPath,
                                           stressConfigSnapshotPath,
                                           myTools,
                                           stepId,
                                           stroke_mm)) {
        return 0;
    }

    ResidualReleaseSummary summary;
    const int ok = runResidualRelease(meshFile,
                                      stressConfigSnapshotPath.toStdString(),
                                      youngsmodulus,
                                      poisson,
                                      1,
                                      stepId,
                                      outputPrefix.toStdString(),
                                      &summary);
    if (ok) {
        qDebug() << "Residual release result:"
                 << "stroke_mm" << stroke_mm
                 << "step" << summary.step
                 << "max_disp_mm" << summary.max_disp_mm
                 << "max_ux_mm" << summary.max_ux_mm
                 << "max_uy_mm" << summary.max_uy_mm
                 << "max_uz_mm" << summary.max_uz_mm;
    }
    else {
        qDebug() << "Residual release failed at stroke_mm" << stroke_mm
                 << "step" << stepId;
    }

    return ok;
}
CutsimBroaching::CutsimBroaching(int depth) {
    max_depth = depth;
    myGLWidget = new cutsim::GLWidget(DEFAULT_SCENE_RADIUS);
    gld = myGLWidget->addGLData();
    stockVolume = new StockVolume();
    octree_cube_size = 50;

    // 初始化选择相关变量
    selectedBladeId = 0;
    selectedPointIndex = 0;
    selectionEnabled = false;
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
    if (error != 0 || stock2->facets.empty()) {
        qDebug() << "STL error:" << error << "facets:" << stock2->facets.size();
        delete stock2;
        return error != 0 ? error : 1;
    }

    stock2->setColor(PARTS_COLOR);
    stock2->calcBB();
    if (stock2->facets.empty()) {
        qDebug() << "STL error: no valid facets after calcBB";
        delete stock2;
        return 1;
    }
    stockVolume->stock = stock2;
    stockVolume->operation = SUM_OPERATION;
    myStocks.push_back(stockVolume);

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

// 设置用户选择的刀刃和点
void CutsimBroaching::setSelectedBladePoint(int blade_id, int point_index)
{
    selectedBladeId = blade_id;
    selectedPointIndex = point_index;
    qDebug() << "Selected blade and point updated:" << blade_id << "," << point_index;
}

// 启用或禁用选择功能
void CutsimBroaching::enableSelection(bool enabled)
{
    selectionEnabled = enabled;
    qDebug() << "Selection " << (enabled ? "enabled" : "disabled");
}
int CutsimBroaching::setConstraints(std::array<std::array<double, 2>, 3> xyzconstraints) {
    myBroachCutsim->setConstraints(xyzconstraints);
    return 1;
}

int CutsimBroaching::addBroach(QString file1Path)
{
    cutsim::broaching_AptCutterVolume* currentBroach = dynamic_cast<cutsim::broaching_AptCutterVolume*> (myTools[0]);

    int blade_id = currentBroach->blade_sum;  // 初始化索引计数器
    const QByteArray bladePathUtf8 = file1Path.toUtf8();
    if (currentBroach->readBladeAnglesFromFile(std::string(bladePathUtf8.constData(), bladePathUtf8.size()), blade_id))    currentBroach->blade_sum++; //读取刀刃1离散点角度信息
    return 1;
}
int CutsimBroaching::addBroachs(QString filePath)
{
    cutsim::broaching_AptCutterVolume* currentBroach =
        myTools.empty() ? nullptr : dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[0]);

    if (!currentBroach) {
        qDebug() << "Cannot add broach points before broach tool is initialized.";
        return -1;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error opening broach list file:" << filePath << file.errorString();
        return -1;
    }

    std::vector<QString> entries;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString entry = in.readLine().trimmed();
        if (!entry.isEmpty()) {
            entries.push_back(entry);
        }
    }
    file.close();

    if (entries.empty()) {
        qDebug() << "Broach point file is empty:" << filePath;
        return -1;
    }

    if (looksLikeBladePointRow(entries.front())) {
        const int bladeCountBefore = currentBroach->blade_sum;
        addBroach(filePath);

        if (currentBroach->blade_sum <= bladeCountBefore) {
            qDebug() << "No blade points imported from direct file:" << filePath;
            return -1;
        }

        qDebug() << "Imported direct blade point file:" << filePath
                 << "blade count:" << currentBroach->blade_sum;
        return 1;
    }

    QFileInfo listFileInfo(filePath);
    QDir listDir = listFileInfo.absoluteDir();

    int importedCount = 0;
    for (QString bladePath : entries) {
        QFileInfo bladeInfo(bladePath);
        if (!bladeInfo.isAbsolute()) {
            QFileInfo siblingBlade(listDir.filePath(bladeInfo.fileName()));
            if (siblingBlade.exists()) {
                bladePath = siblingBlade.absoluteFilePath();
            }
            else {
                QFileInfo relativeBlade(listDir.filePath(bladePath));
                if (relativeBlade.exists()) {
                    bladePath = relativeBlade.absoluteFilePath();
                }
            }
        }

        const int bladeCountBefore = currentBroach->blade_sum;
        addBroach(bladePath);
        if (currentBroach->blade_sum > bladeCountBefore) {
            ++importedCount;
        }
        else {
            qDebug() << "No blade points imported from list entry:" << bladePath;
        }
    }

    if (importedCount == 0) {
        qDebug() << "No blade point files imported from list:" << filePath;
        return -1;
    }

    qDebug() << "Imported blade point list:" << filePath
             << "imported blades:" << importedCount
             << "blade count:" << currentBroach->blade_sum;
    return 1;
}

int CutsimBroaching::addBroach(vector<array<double, 12>>& points)
{
    cutsim::broaching_AptCutterVolume* currentBroach = dynamic_cast<cutsim::broaching_AptCutterVolume*> (myTools[0]);
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
    cutsim::broaching_AptCutterVolume* currentBroach = new cutsim::broaching_AptCutterVolume();
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
    currentBroach->deform_color_max = 0.05;//可视化最大位移
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
        cutsim::broaching_AptCutterVolume* currentBroach = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[i]);
        currentBroach->v_c = 5;
        //刀具移动方向向量
        currentBroach->v_x = broaching_velocity[0];
        currentBroach->v_y = broaching_velocity[1];
        currentBroach->v_z = broaching_velocity[2];
    }
    return 1;
}

int CutsimBroaching::setVisulization(int* visulization_item, std::array<double, 2> visulization_limits)
{

    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::broaching_AptCutterVolume* currentBroach = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[currentTool]);
        currentBroach->deform_color_var = *visulization_item;//可视化最小位移
        currentBroach->deform_color_min = visulization_limits[0];//可视化最小位移
        currentBroach->deform_color_max = visulization_limits[1];//可视化最大位移
    }
    return 1;
}

int CutsimBroaching::performFEMSimulation(MdiChild* mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits)
{
    const bool enableModalAnalysis = modalAnalysisEnabled;
    const bool enableResidualRelease = residualReleaseEnabled;

    for (cutsim::CutterVolume* tool : myTools) {
        auto* broach = dynamic_cast<cutsim::broaching_AptCutterVolume*>(tool);
        if (broach) {
            broach->clearMachiningContactEvents();
            broach->setMachiningResidualEventsEnabled(false);
        }
    }

    myBroachCutsim->updateGL();
    Handle(AIS_InteractiveObject) workdeformed;
    auto runOnUiThread = [&](const auto& fn) {
        if (QThread::currentThread() == mdichild->thread()) {
            fn();
        }
        else {
            QMetaObject::invokeMethod(mdichild, fn, Qt::BlockingQueuedConnection);
        }
        };
    auto refreshViewer = [&](bool erasePrevious, double x, double y, double z, bool moveTool) {
        runOnUiThread([&]() {
            if (erasePrevious && !workdeformed.IsNull()) {
                h_MyViewer->Erase(workdeformed);
            }
            workdeformed = OcctViewer::getGraphic3d(gld);
            h_MyViewer->Display(workdeformed);
            if (moveTool) {
                mdichild->MoveToolModel(true, x, y, z, 0, 0, 0);
            }
            h_MyViewer->Redraw();
            });
        };
    auto colorBarTitle = [](int item) {
        switch (item) {
        case 1:
            return QStringLiteral("Ux");
        case 2:
            return QStringLiteral("Uy");
        case 3:
            return QStringLiteral("Uz");
        default:
            return QStringLiteral("U magnitude");
        }
        };
    auto syncDynamicColorBar = [&]() {
        if (!mdichild || !mdichild->q3dView) {
            return;
        }

        double minVal = std::numeric_limits<double>::max();
        double maxVal = -std::numeric_limits<double>::max();
        bool found = false;

        for (cutsim::CutterVolume* tool : myTools) {
            auto* broach = dynamic_cast<cutsim::broaching_AptCutterVolume*>(tool);
            if (!broach || !std::isfinite(broach->deform_color_min) || !std::isfinite(broach->deform_color_max)) {
                continue;
            }

            minVal = std::min(minVal, broach->deform_color_min);
            maxVal = std::max(maxVal, broach->deform_color_max);
            found = true;
        }

        if (!found || maxVal <= minVal) {
            return;
        }

        const QString title = colorBarTitle(visulization_item ? *visulization_item : 0);
        runOnUiThread([=]() {
            if (mdichild->q3dView) {
                mdichild->q3dView->setColorBarVisible(true, title, minVal, maxVal);
            }
            });
        };
    refreshViewer(false, 0, 0, 0, false);
    if (enableModalAnalysis) {
        peformModalAnalysis();
        for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
        {
            cutsim::broaching_AptCutterVolume* s2 = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[currentTool]);
            s2->updatestockVibrParams();//初始化工件变形
            //mdichild->forcewidget->setData( s2->angle_total_force);
        }
    }

    cutsim::broaching_AptCutterVolume* s2 = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[0]);
    for (double t = 0; t <= 100; t = t + incrementive_time)
    {
        setVisulization(visulization_item, visulization_limits);
        double x = 0 + s2->v_x * t;//更新刀具位移点
        double y = 0 + s2->v_y * t;
        double z = 0 + s2->v_z * t;
        s2->tool_angle = s2->tool_angle + s2->step;//更新刀具角度
        s2->tool_angle = std::round(s2->tool_angle * 1000) / 1000.0;;//保留小数点后三位
        s2->new_angle = s2->tool_angle + s2->step;//更新刀具下一步的角度
        s2->new_angle = std::round(s2->new_angle * 1000) / 1000.0;//保留小数点后三位
        s2->setCenter(cutsim::GLVertex(x, y, z));//更新刀具位置
        s2->blade_num = 0;
        s2->calculatePosition_balde();//更新当前刀刃离散点位置
        s2->setMachiningResidualContext(-1, sqrt(x * x + y * y + z * z));
        s2->setMachiningResidualEventsEnabled(enableResidualRelease);
        //setVisulization(visulization_item, visulization_limits);
        myBroachCutsim->broaching_diff_volume_blade_cuda(s2);//材料去除
        s2->setMachiningResidualEventsEnabled(false);
        s2->cut_h.clear();//清空切削厚度             
        s2->cut_h_map.clear();
        myBroachCutsim->updateGL();
        syncDynamicColorBar();
        refreshViewer(true, x, y, z, true);

        emit ApplyUpdateViewer();
    }

    // 发送刀刃和点的数量信息
    if (s2->blade_sum > 0) {
        // 假设每个刀刃的点数相同，取第一个刀刃的点数
        int point_count = 0;
        if (s2->original_blade_points.size() > 0 && s2->original_blade_points[0].size() > 0) {
            point_count = s2->original_blade_points[0].size();
        }
        emit bladePointSelectionUpdated(s2->blade_sum, point_count);
    }
    if (enableModalAnalysis) {
        peformModalAnalysis();
        for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
        {
            cutsim::broaching_AptCutterVolume* currentBroach = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[currentTool]);
            currentBroach->updatestockVibrParams();//初始化工件变形
        }
    }

    int residualStepId = 0;
    int residualIntervalSteps = 1;
    int modalIntervalSteps = 1;
    if ((enableResidualRelease || enableModalAnalysis) && modalsteps > 0.0 && incrementive_time > 0.0) {
        residualIntervalSteps = std::max(1, static_cast<int>(std::round(modalsteps / incrementive_time)));
        modalIntervalSteps = residualIntervalSteps;
    }
    int timeStepIndex = 0;
    double finalStroke = 0.0;

    for (double t = 0; t <= simulation_time; t = t + incrementive_time)
    {
        setVisulization(visulization_item, visulization_limits);
        double x = 0;//更新刀具位移点
        double y = 0;
        double z = 0;
        for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
        {
            cutsim::broaching_AptCutterVolume* s2 = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[currentTool]);
            //double dx = s2->v_x * incrementive_time;
            //double dy = s2->v_y * incrementive_time;
            //double dz = s2->v_z * incrementive_time;
            x = 0 + s2->v_x * t;//更新刀具位移点
            y = 0 + s2->v_y * t;
            z = 0 + s2->v_z * t;
            s2->setCenter(cutsim::GLVertex(x, y, z));//更新刀具位置
            s2->tool_angle = s2->tool_angle + s2->step;//更新刀具角度
            s2->tool_angle = std::round(s2->tool_angle * 1000) / 1000.0;;//保留小数点后三位
            s2->new_angle = s2->tool_angle + s2->step;//更新刀具下一步的角度
            s2->new_angle = std::round(s2->new_angle * 1000) / 1000.0;//保留小数点后三位
            qDebug() << "tool_angle: " << s2->tool_angle;

            // 迭代计算：变形影响切削力，切削力影响变形
            const int max_iterations = 3;
            const double convergence_threshold = 0.03; // 收敛阈值
            double force_change = 1e6; // 初始化为大值

            // 存储上一次的力数据用于收敛检查
            GLVertex previous_total_force;

            for (int iter = 0; iter<max_iterations && force_change > s2->temp_total_force.norm() * convergence_threshold; iter++)
            {
                qDebug() << "iter_num: " << iter;
                const double currentStroke = sqrt(x * x + y * y + z * z);
                s2->setMachiningResidualContext(timeStepIndex, currentStroke);
                s2->setMachiningResidualEventsEnabled(enableResidualRelease && iter == 0);

                // 存储当前力数据用于下一次迭代的收敛检查
                previous_total_force = s2->temp_total_force;
                for (int i = 0; i < s2->blade_sum; i++)
                {
                    s2->blade_num = i;
                    s2->calculatePosition_balde();//更新当前刀刃离散点位置
                    myBroachCutsim->broaching_diff_volume_blade_cuda(s2);//材料去除
                    s2->calculateForceData();
                    s2->cut_h.clear();//清空切削厚度             
                }
                s2->setMachiningResidualEventsEnabled(false);
                s2->calculateTotalForce();
                s2->outputForceData("data/Force");
                s2->outputDeformedBladePointsData("data/DeformedBladePoints");
                if (enableModalAnalysis) {
                    s2->calculatestockVibration();
                }
                s2->calculateStraightness();

                // 在 cutsim_broaching.cpp 中添加验证
                if (s2->straightness_map.empty()) {
                    qDebug() << "Straightness map is empty!";
                }
                else {
                    qDebug() << "Straightness map size:" << s2->straightness_map.size();
                    qDebug() << "Available angles in map:";
                    for (const auto& pair : s2->straightness_map) {
                        qDebug() << "Angle:" << pair.first;
                    }
                }

                // 提取当前时间步的直线度数据
                double current_angle = s2->tool_angle;  // 使用角度作为查找键值

                qDebug() << "current_angle:" << current_angle;

                // 遍历刀刃和点，发送直线度数据
                if (s2 && s2->blade_sum > 0) {
                    if (selectionEnabled) {
                        // 如果启用了选择，只发送选中的刀刃和点的数据
                        auto angle_it = s2->straightness_map.find(current_angle);
                        if (angle_it != s2->straightness_map.end()) {
                            const auto& blade_map = angle_it->second;
                            auto blade_it = blade_map.find(selectedBladeId);
                            if (blade_it != blade_map.end()) {
                                const auto& point_map = blade_it->second;
                                auto point_it = point_map.find(selectedPointIndex);
                                if (point_it != point_map.end()) {
                                    double straightness_value = point_it->second;

                                    // 调试信息
                                    qDebug() << "About to emit straightnessDataUpdated signal (selected)";
                                    qDebug() << "Selected blade:" << selectedBladeId << "point:" << selectedPointIndex;
                                    qDebug() << "this object address:" << this;

                                    // 发送选中点的数据
                                    emit straightnessDataUpdated(selectedBladeId, selectedPointIndex, sqrt(x * x + y * y + z * z), straightness_value);

                                    qDebug() << "Signal emitted successfully";
                                }
                            }
                        }
                    }
                    else {
                        // 如果未启用选择，发送所有刀刃和点的数据
                        for (int blade_id = 0; blade_id < s2->blade_sum; blade_id++) {
                            auto angle_it = s2->straightness_map.find(current_angle);
                            if (angle_it != s2->straightness_map.end()) {
                                const auto& blade_map = angle_it->second;
                                auto blade_it = blade_map.find(blade_id);
                                if (blade_it != blade_map.end()) {
                                    const auto& point_map = blade_it->second;
                                    for (const auto& point_pair : point_map) {
                                        int point_index = point_pair.first;
                                        double straightness_value = point_pair.second;

                                        // 调试信息
                                        qDebug() << "About to emit straightnessDataUpdated signal (all)";
                                        qDebug() << "this object address:" << this;

                                        // 为每个点发送单个数据点
                                        emit straightnessDataUpdated(blade_id, point_index, sqrt(x * x + y * y + z * z), straightness_value);

                                        qDebug() << "Signal emitted successfully";
                                    }
                                }
                            }
                        }
                    }
                }
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
            emit ApplyUpdateForces(sqrt(x * x + y * y + z * z), *Fx, *Fy, *Fz);
            Standard_Character Buffer[1024] = { 0 };
            Sprintf(Buffer, "Stroke: %f mm, Fx: %f N, Fy: %f N, Fz: %f N\n", sqrt(x * x + y * y + z * z), *Fx, *Fy, *Fz);
            Msg::ShowInfo(Buffer);
            //Msg::ShowInfo("Stroke:");
        }
        myBroachCutsim->updateGL();
        syncDynamicColorBar();
        refreshViewer(true, x, y, z, true);
        emit ApplyUpdateViewer();
        finalStroke = sqrt(x * x + y * y + z * z);
        if (enableResidualRelease && timeStepIndex % residualIntervalSteps == 0) {
            performResidualRelease(residualStepId++, finalStroke);
        }
        ++timeStepIndex;
        if (enableModalAnalysis && timeStepIndex % modalIntervalSteps == 0 && t > 0)
        {
            peformModalAnalysis();
        }
    }
    if (enableResidualRelease) {
        performResidualRelease(residualStepId++, finalStroke);
    }
    Msg::ShowInfo("Simulation done!");
    //cout << "fem finished" << endl;
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::broaching_AptCutterVolume* s2 = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[currentTool]);
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

    cutsim::broaching_AptCutterVolume* s0 = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[0]);
    s0->normalvertices_size = normalvertices.size();
    std::vector<double> materialprops;
    materialprops.push_back(density); materialprops.push_back(youngsmodulus); materialprops.push_back(poisson);
    runEx12p(meshFile, materialprops, s0->vibration_values, s0->pre_vibration_vectors);//调用模态分析程序
    std::vector<size_t> target_modes = { 0,1 };
    s0->vibration_vectors = s0->convertTo3DVibrationVectors(s0->pre_vibration_vectors, 3, target_modes);
    s0->updatestockVibrParams();
    static int modalAnalysisRunIndex = 0;
    appendModalVectorStats(QDir("data/Modal").filePath("modal_vector_stats.tsv"),
        modalAnalysisRunIndex++,
        s0->tool_angle,
        s0->new_angle,
        s0->vibration_values,
        s0->pre_vibration_vectors,
        s0->vibration_vectors);
    for (int currentTool = 1; currentTool < myTools.size(); currentTool++)
    {
        cutsim::broaching_AptCutterVolume* s2 = dynamic_cast<cutsim::broaching_AptCutterVolume*>(myTools[currentTool]);
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
