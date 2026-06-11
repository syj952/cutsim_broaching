//#include "cutsim_app.hpp"
#include "cutsim_milling.hpp"

#include <src/cutsim/cutsim/facet.hpp>
#include <src/cutsim/cutsim/volume.hpp>
#include <QEventLoop>
#include <QDebug>
#include <QString>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QMutexLocker>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <array>

using milling::CutsimMilling;

namespace {
    void setExportError(QString* errorMessage, const QString& message)
    {
        if (errorMessage) {
            *errorMessage = message;
        }
    }

    void writeAsciiStlFacet(QTextStream& out, const cutsim::GLVertex& a, const cutsim::GLVertex& b, const cutsim::GLVertex& c)
    {
        const double ux = b.x - a.x;
        const double uy = b.y - a.y;
        const double uz = b.z - a.z;
        const double vx = c.x - a.x;
        const double vy = c.y - a.y;
        const double vz = c.z - a.z;
        double nx = uy * vz - uz * vy;
        double ny = uz * vx - ux * vz;
        double nz = ux * vy - uy * vx;
        const double len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 1e-12) {
            nx /= len;
            ny /= len;
            nz /= len;
        }
        else {
            nx = 0.0;
            ny = 0.0;
            nz = 1.0;
        }

        out << "  facet normal " << nx << " " << ny << " " << nz << "\n";
        out << "    outer loop\n";
        out << "      vertex " << a.x << " " << a.y << " " << a.z << "\n";
        out << "      vertex " << b.x << " " << b.y << " " << b.z << "\n";
        out << "      vertex " << c.x << " " << c.y << " " << c.z << "\n";
        out << "    endloop\n";
        out << "  endfacet\n";
    }

    bool exportGLDataToAsciiStl(cutsim::GLData* glData, const QString& filePath, QString* errorMessage)
    {
        if (!glData) {
            setExportError(errorMessage, QStringLiteral("仿真结果为空，无法导出 STL。"));
            return false;
        }

        QFile outFile(filePath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            setExportError(errorMessage, QStringLiteral("无法创建 STL 文件：") + filePath);
            return false;
        }

        QTextStream out(&outFile);
        out.setRealNumberPrecision(9);

        QMutexLocker locker(&(glData->renderMutex));
        const int vertexCount = glData->vertexCount();
        const int indexCount = glData->indexCount();
        const int polyVerts = glData->polygonVertices();
        const cutsim::GLVertex* vertices = glData->getVertexArray();
        const GLuint* indices = glData->getIndexArray();

        if (vertexCount <= 0 || indexCount <= 0 || !vertices || !indices) {
            setExportError(errorMessage, QStringLiteral("当前没有可导出的切削后表面网格。"));
            return false;
        }
        if (polyVerts != 3 && polyVerts != 4) {
            setExportError(errorMessage, QStringLiteral("当前渲染网格不是三角形/四边形面片，无法导出 STL。"));
            return false;
        }

        out << "solid cutsim_result\n";
        int facetCount = 0;
        for (int i = 0; i + polyVerts - 1 < indexCount; i += polyVerts) {
            const GLuint i0 = indices[i];
            const GLuint i1 = indices[i + 1];
            const GLuint i2 = indices[i + 2];
            if (i0 >= static_cast<GLuint>(vertexCount) || i1 >= static_cast<GLuint>(vertexCount) || i2 >= static_cast<GLuint>(vertexCount)) {
                continue;
            }

            writeAsciiStlFacet(out, vertices[i0], vertices[i1], vertices[i2]);
            ++facetCount;

            if (polyVerts == 4) {
                const GLuint i3 = indices[i + 3];
                if (i3 < static_cast<GLuint>(vertexCount)) {
                    writeAsciiStlFacet(out, vertices[i0], vertices[i2], vertices[i3]);
                    ++facetCount;
                }
            }
        }
        out << "endsolid cutsim_result\n";

        if (facetCount == 0) {
            setExportError(errorMessage, QStringLiteral("没有写出任何 STL 三角面片。"));
            return false;
        }

        return true;
    }

    bool ensureParentDirectoryForPath(const std::string& filePath, std::string* errorMessage)
    {
        const QFileInfo fileInfo(QString::fromStdString(filePath));
        QDir parentDir = fileInfo.absoluteDir();
        if (parentDir.exists() || parentDir.mkpath(QStringLiteral("."))) {
            return true;
        }

        if (errorMessage) {
            *errorMessage = "failed to create output directory: " + parentDir.absolutePath().toStdString();
        }
        return false;
    }

    bool writeMechanicsMapFileFromBody(const cutsim::MechanicsMapLibrary& headerData,
        const std::string& recordsBodyPath,
        const std::string& outputPath,
        std::size_t recordCount,
        std::string* errorMessage)
    {
        if (!ensureParentDirectoryForPath(outputPath, errorMessage)) {
            return false;
        }

        std::ifstream recordsIn;
        if (recordCount > 0) {
            recordsIn.open(recordsBodyPath, std::ios::in);
            if (!recordsIn.is_open()) {
                if (errorMessage) {
                    *errorMessage = "failed to open mechanics map temporary records file";
                }
                return false;
            }
        }

        std::ofstream out(outputPath, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            if (errorMessage) {
                *errorMessage = "failed to open mechanics map file for writing";
            }
            return false;
        }

        out << std::setprecision(17);
        if (!headerData.writeHeader(out, recordCount, errorMessage)) {
            return false;
        }

        if (recordCount > 0) {
            out << recordsIn.rdbuf();
        }

        if (!out) {
            if (errorMessage) {
                *errorMessage = "failed to write mechanics map file";
            }
            return false;
        }
        return true;
    }

    cutsim::Bbox buildMillingToolBbox(const cutsim::milling_AptCutterVolume* tool)
    {
        cutsim::Bbox bbox;
        if (!tool) {
            return bbox;
        }

        const cutsim::GLVertex stepOffset(
            static_cast<GLfloat>(tool->dx),
            static_cast<GLfloat>(tool->dy),
            static_cast<GLfloat>(tool->dz));
        auto addSweptCylinderBbox = [&bbox, tool, &stepOffset](double radius, double minZ, double maxZ) {
            const cutsim::GLVertex centers[2] = {
                tool->center,
                tool->center - stepOffset
            };
            for (const cutsim::GLVertex& center : centers) {
                bbox.addPoint(cutsim::GLVertex(
                    static_cast<GLfloat>(center.x - radius),
                    static_cast<GLfloat>(center.y - radius),
                    static_cast<GLfloat>(center.z + minZ)));
                bbox.addPoint(cutsim::GLVertex(
                    static_cast<GLfloat>(center.x + radius),
                    static_cast<GLfloat>(center.y + radius),
                    static_cast<GLfloat>(center.z + maxZ)));
            }
            };

        double minZ = 0.0;
        double maxZ = 0.0;
        double maxRadius = 0.0;
        bool hasSegment = false;
        for (const auto& segment : tool->segments) {
            minZ = hasSegment ? std::min(minZ, segment.z_start) : segment.z_start;
            maxZ = hasSegment ? std::max(maxZ, segment.z_end) : segment.z_end;
            maxRadius = std::max(maxRadius, std::max(std::abs(segment.radius1), std::abs(segment.radius2)));
            hasSegment = true;
        }

        if (hasSegment) {
            addSweptCylinderBbox(maxRadius, minZ, maxZ);
            return bbox;
        }

        bool hasPoint = false;
        for (const auto& point : tool->original_blade_points) {
            minZ = hasPoint ? std::min(minZ, static_cast<double>(point.z)) : static_cast<double>(point.z);
            maxZ = hasPoint ? std::max(maxZ, static_cast<double>(point.z)) : static_cast<double>(point.z);
            maxRadius = std::max(maxRadius, std::sqrt(
                static_cast<double>(point.x) * point.x +
                static_cast<double>(point.y) * point.y));
            hasPoint = true;
        }
        addSweptCylinderBbox(maxRadius, minZ, maxZ);
        return bbox;
    }

    bool millingToolOverlapsStock(const cutsim::milling_AptCutterVolume* tool,
        const std::vector<milling::StockVolume*>& stocks,
        const cutsim::Cutsim* cutsim)
    {
        const cutsim::Bbox toolBbox = buildMillingToolBbox(tool);
        for (const milling::StockVolume* stockVolume : stocks) {
            if (stockVolume && stockVolume->stock && toolBbox.overlaps(stockVolume->stock->bb)) {
                return true;
            }
        }

        return stocks.empty() &&
            cutsim &&
            cutsim->tree &&
            cutsim->tree->root &&
            toolBbox.overlaps(cutsim->tree->root->bb);
    }
}
CutsimMilling::CutsimMilling(int depth) {
    max_depth = depth;
    myGLWidget = new cutsim::GLWidget(DEFAULT_SCENE_RADIUS);
    gld = myGLWidget->addGLData();
    stockVolume = new StockVolume();
    octree_cube_size = 50;

    // 锟斤拷始锟斤拷选锟斤拷锟斤拷乇锟斤拷锟?
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

    myMillCutsim = new cutsim::Cutsim(octree_cube_size, max_depth, octree_center, gld, myGLWidget);

    myMillCutsim->sum_stl_volume_cuda(stock2, max_depth);
    //myMillCutsim->sum_volume(stockVolume->stock);
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

// 锟斤拷锟斤拷锟矫伙拷选锟斤拷牡锟斤拷泻偷锟?
void CutsimMilling::setSelectedBladePoint(int blade_id, int point_index)
{
    selectedBladeId = blade_id;
    selectedPointIndex = point_index;
    qDebug() << "Selected blade and point updated:" << blade_id << "," << point_index;
}

// 锟斤拷锟矫伙拷锟斤拷锟窖★拷锟斤拷锟?
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
    currentMill->max_depth_1 = max_depth; //锟斤拷锟斤拷diff锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷

    //diff锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟接︼拷锟斤拷锟斤拷爻叽锟?
    currentMill->cube_resolution_1 = octree_cube_size * 2.0 / pow(2.0, currentMill->max_depth_1 - 1);
    /*currentMill->cube_resolution_2 = octree_cube_size * 2.0 / pow(2.0, currentMill->max_depth_2 - 1);
    currentMill->inv_cube_resolution_2 = 1.0 / currentMill->cube_resolution_2;*/
    currentMill->type = cutsim::APT_VOLUME;
    currentMill->cuttertype = cutsim::APT;
    currentMill->setColor(0.5, 0.2, 0.1);
    myTools.push_back(currentMill);

    currentMill->force_coefs = force_coefs;

    //mm,锟斤拷锟斤拷锟斤拷小锟斤拷锟斤拷
    currentMill->step = incrementive_time;

    // 娉ㄦ剰锛氳繖閲寁_x, v_y, v_z, spindle_speed, blade_sum绛夊彉閲忓彲鑳借繕鏈缃?
    // 灏嗗湪鍚庣画鐨剆etVelocity, setSpindleSpeed, setBlade_Sum鍑芥暟涓缃?
    // 杩欎簺璁＄畻灏嗗湪performFEMSimulation涓噸鏂拌繘琛?

    // Newmark锟斤拷锟斤拷锟斤拷平锟斤拷锟斤拷锟劫度凤拷锟斤拷
    currentMill->vibr_beta = 0.25;
    currentMill->vibr_gamma = 0.5;
    currentMill->stock_vibr_damping_ratio = 0.001;//锟斤拷锟斤拷锟斤拷锟斤拷

    currentMill->tool_angle = 0;
    currentMill->deform_color_max = 0.1;//锟斤拷锟接伙拷锟斤拷锟轿伙拷锟?
    currentMill->deform_color_min = 0;//锟斤拷锟接伙拷锟斤拷小位锟斤拷

    myMillCutsim->updateGL();
    workdeformed = OcctViewer::getGraphic3d(gld);

    return 1;
}

int CutsimMilling::setVelocity(double vx, double vy, double vz)
{

    for (int i = 0; i < myTools.size(); i++)
    {
        //currentTool = 
        cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[i]);
        //锟斤拷锟斤拷锟狡讹拷锟斤拷锟斤拷锟斤拷锟斤拷
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
        currentMill->deform_color_var = *visulization_item;//锟斤拷锟接伙拷锟斤拷小位锟斤拷
        currentMill->deform_color_min = visulization_limits[0];//锟斤拷锟接伙拷锟斤拷小位锟斤拷
        currentMill->deform_color_max = visulization_limits[1];//锟斤拷锟接伙拷锟斤拷锟轿伙拷锟?
    }
    return 1;
}

int CutsimMilling::performFEMSimulation(MdiChild* mdichild, Handle(MyViewer) h_MyViewer, int* visulization_item, std::array<double, 2> visulization_limits)
{
    QMetaObject::invokeMethod(mdichild, [this, h_MyViewer]() {
        if (!h_MyViewer.IsNull() && !workdeformed.IsNull()) {
            h_MyViewer->Display(workdeformed);
            emit ApplyUpdateViewer();
        }
        }, Qt::BlockingQueuedConnection);

    //peformModalAnalysis();
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::milling_AptCutterVolume* s1 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[currentTool]);
        s1->updatestockVibrParams();//锟斤拷始锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
        //mdichild->forcewidget->setData( s1->angle_total_force);
    }

    cutsim::milling_AptCutterVolume* s1 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[0]);
    
    // 鍦ㄤ豢鐪熷紑濮嬪墠閲嶆柊璁＄畻渚濊禆浜庨€熷害銆佷富杞磋浆閫熺瓑鍙傛暟鐨勫彉閲?
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
    s1->enable_mechanics_map_capture = true;
    s1->mechanics_map_library.clear();
    s1->mechanics_map_library.angle_step = s1->step;
    s1->mechanics_map_library.theta_f = s1->force_coefs;

    const std::string mechanics_map_path = cutsim::defaultMechanicsMapPath();
    const std::string mechanics_map_records_path = mechanics_map_path + ".records.tmp";
    QFile::remove(QString::fromStdString(mechanics_map_records_path));

    cutsim::MechanicsMapLibrary mechanics_map_header;
    mechanics_map_header.angle_step = s1->step;
    mechanics_map_header.theta_f = s1->force_coefs;

    std::string mechanics_map_error;
    bool mechanics_map_stream_ok = ensureParentDirectoryForPath(mechanics_map_records_path, &mechanics_map_error);
    std::ofstream mechanics_map_records_out;
    if (mechanics_map_stream_ok) {
        mechanics_map_records_out.open(mechanics_map_records_path, std::ios::out | std::ios::trunc);
        mechanics_map_records_out << std::setprecision(17);
        mechanics_map_stream_ok = mechanics_map_records_out.is_open();
        if (!mechanics_map_stream_ok) {
            mechanics_map_error = "failed to open mechanics map temporary records file";
        }
    }
    if (!mechanics_map_stream_ok) {
        qDebug() << "Failed to initialize mechanics map streaming:" << QString::fromStdString(mechanics_map_error);
        s1->enable_mechanics_map_capture = false;
    }

    std::size_t mechanics_map_record_count = 0;
    for (double t = 0; t <= simulation_time; t = t + incrementive_time)
    {
        double x = -50 + s1->v_x * t / (2 * M_PI);
        double y = 0 + s1->v_y * t / (2 * M_PI);
        double z = 7.0;
        double a = 0.0;
        double b = 0.0;
        double c = 0.0;
        s1->dx = s1->v_x * s1->step / (2 * M_PI);
        s1->dy = s1->v_y * s1->step / (2 * M_PI);
        s1->dz = s1->v_z * s1->step / (2 * M_PI);
        s1->setCenter(cutsim::GLVertex(x, y, z));
        s1->tool_angle = s1->tool_angle + s1->step;
        const bool tool_overlaps_stock = millingToolOverlapsStock(s1, myStocks, myMillCutsim);
        if (tool_overlaps_stock) {
        // 杩唬璁＄畻锛氬彉褰㈠奖鍝嶅垏鍓婂姏锛屽垏鍓婂姏褰卞搷鍙樺舰
        const int max_iterations = 10;
        const double convergence_threshold = 0.01; // 鏀舵暃闃堝€?3N
        double force_change = 1e6; // 鍒濆鍖栦负澶у€?

        // 瀛樺偍涓婁竴娆＄殑鍔涙暟鎹敤浜庢敹鏁涙鏌?
        GLVertex previous_total_force;
        for (int iter = 0; iter<max_iterations && force_change > convergence_threshold; iter++)
        {
            qDebug() << "iter_num: " << iter;

            // 瀛樺偍褰撳墠鍔涙暟鎹敤浜庝笅涓€娆¤凯浠ｇ殑鏀舵暃妫€鏌?
            previous_total_force = s1->temp_total_force;
        for (int n = 0; n < s1->blade_sum; n = n + 1)
        {
            s1->blade_num = n;//锟斤拷锟斤拷锟斤拷锟?
            s1->blade_angle = s1->tool_angle - n * 2 * M_PI / s1->blade_sum;
            s1->calculatePosition_balde();
            myMillCutsim->milling_diff_volume_blade_cuda(s1);
            s1->calculateForceData();
            s1->cut_h.clear();
            qDebug() << "blade_angle: " << s1->blade_angle;
        }
        s1->calculateTotalForce();
        s1->calculatestockVibration();
        // 璁＄畻鍔涘彉鍖栭噺
        force_change = 0.0;
        force_change += (s1->temp_total_force - previous_total_force).norm();

        qDebug() << "Force change: " << force_change << ", Convergence threshold: " << s1->temp_total_force.norm() * convergence_threshold;
        if (force_change <= s1->temp_total_force.norm() * convergence_threshold) {
            qDebug() << "Converged after " << iter + 1 << " iterations!";
        }
    }
        }
        else {
            s1->cut_h.clear();
            s1->force_map.clear();
            s1->angle_force_map.clear();
            s1->temp_total_force = cutsim::GLVertex(0, 0, 0);
            s1->angle_total_force[s1->tool_angle] = s1->temp_total_force;
        }
    if (fabs(fmod(s1->tool_angle, 1000.0)) < s1->step)//姣?000寮у害璁板綍鏁版嵁
    {
        //plotForceCurves(s1->angle_total_force);
        s1->outputTotalForceData("src/data/Force");
        //s1->outputSurfaceData("src/data/Surface");
    }
    //s1->outputForceData("src/data/Force");
    qDebug() << "-----------------------\n";
    double Fx = 0.0;
    double Fy = 0.0;
    double Fz = 0.0;
    s1->getCurrentTotalForce(&Fx, &Fy, &Fz);
    emit ApplyUpdateForces(s1->tool_angle, Fx, Fy, Fz);
    Standard_Character Buffer[1024] = { 0 };
    Sprintf(Buffer, "Stroke: %f mm, Fx: %f N, Fy: %f N, Fz: %f N\n", s1->tool_angle, Fx, Fy, Fz);
    Msg::ShowInfo(Buffer);

        QMetaObject::invokeMethod(mdichild, [mdichild, x, y, z]() {
            mdichild->MoveToolModel(true, x, y, z, 0, 0, 0);
            }, Qt::QueuedConnection);

    updategl_num++;
    if (updategl_num == 1000) {
        myMillCutsim->updateGL();
        QMetaObject::invokeMethod(mdichild, [this, h_MyViewer]() {
            if (!workdeformed.IsNull()) {
                h_MyViewer->getAisContext()->Remove(workdeformed, Standard_False);
            }
            workdeformed = OcctViewer::getGraphic3d(gld);
            h_MyViewer->Display(workdeformed);
            emit ApplyUpdateViewer();
            Msg::ShowInfo("Simulation done!");
            }, Qt::BlockingQueuedConnection);
        updategl_num = 0;
    }

    if (s1->enable_mechanics_map_capture && !s1->mechanics_map_library.empty()) {
        const std::size_t written_records = s1->mechanics_map_library.size();
        mechanics_map_header.modal_count = std::max(mechanics_map_header.modal_count, s1->mechanics_map_library.modal_count);
        mechanics_map_error.clear();
        if (mechanics_map_stream_ok && s1->mechanics_map_library.writeRecords(mechanics_map_records_out, &mechanics_map_error)) {
            mechanics_map_record_count += written_records;
        }
        else {
            mechanics_map_stream_ok = false;
            s1->enable_mechanics_map_capture = false;
            qDebug() << "Failed to stream mechanics map records:" << QString::fromStdString(mechanics_map_error);
        }

        s1->mechanics_map_library.clear();
        s1->mechanics_map_library.angle_step = s1->step;
        s1->mechanics_map_library.theta_f = s1->force_coefs;
    }

    }
    if (mechanics_map_records_out.is_open()) {
        mechanics_map_records_out.close();
    }
    mechanics_map_header.modal_count = std::max(mechanics_map_header.modal_count, s1->mechanics_map_library.modal_count);
    mechanics_map_error.clear();
    if (mechanics_map_stream_ok && writeMechanicsMapFileFromBody(mechanics_map_header,
        mechanics_map_records_path,
        mechanics_map_path,
        mechanics_map_record_count,
        &mechanics_map_error)) {
        qDebug() << "Saved mechanics map records:" << static_cast<unsigned long long>(mechanics_map_record_count)
            << "to" << cutsim::defaultMechanicsMapPath();
    }
    else {
        qDebug() << "Failed to save mechanics map:" << QString::fromStdString(mechanics_map_error);
    }
    QFile::remove(QString::fromStdString(mechanics_map_records_path));
    s1->mechanics_map_library.clear();
    for (int currentTool = 0; currentTool < myTools.size(); currentTool++)
    {
        cutsim::milling_AptCutterVolume* currentMill = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[currentTool]);
        currentMill->updatestockVibrParams();//锟斤拷始锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
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
    MeshIDExport(myMillCutsim->tree, meshFile, normalvertices);//锟斤拷锟今导筹拷锟斤拷锟斤拷

    cutsim::milling_AptCutterVolume* s0 = dynamic_cast<cutsim::milling_AptCutterVolume*>(myTools[0]);
    s0->normalvertices_size = normalvertices.size();
    std::vector<double> materialprops;
    materialprops.push_back(density); materialprops.push_back(youngsmodulus); materialprops.push_back(poisson);
    runEx12p(meshFile, materialprops, s0->vibration_values, s0->pre_vibration_vectors);//锟斤拷锟斤拷模态锟斤拷锟斤拷锟斤拷锟斤拷
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

bool CutsimMilling::hasRenderableResult() const
{
    return gld != nullptr && gld->vertexCount() > 0;
}

bool CutsimMilling::exportCurrentStl(const QString& stlFilePath, QString* errorMessage)
{
    if (!myMillCutsim) {
        setExportError(errorMessage, QStringLiteral("尚未创建铣削仿真实例，无法导出 STL。"));
        return false;
    }
    if (stlFilePath.isEmpty()) {
        setExportError(errorMessage, QStringLiteral("STL 导出路径为空。"));
        return false;
    }

    myMillCutsim->updateGL();
    return exportGLDataToAsciiStl(gld, stlFilePath, errorMessage);
}
CutsimMilling::~CutsimMilling()
{
    delete myMillCutsim;
    delete myGLWidget;
}
