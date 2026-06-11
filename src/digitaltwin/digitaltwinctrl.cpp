#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "digitaltwinctrl.h"
#include "src/digitaltwin/digitaltwin_milling/digitaltwin_milling.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QtConcurrent/QtConcurrentRun>

DigitalTwinController::DigitalTwinController(QObject* parent)
    : QObject(parent)
{
}

void DigitalTwinController::setSimulationObject(digitaltwin_milling::DigitalTwinMilling* simObj)
{
    m_simObj = simObj;
}

void DigitalTwinController::onMchDataUpdated(std::array<double, 8> mchData)
{
    // spindle_speed/step/x/y/z/a/b/c
    m_mchDataList.push_back(mchData);

    if (m_mchDataList.size() >= 2) {
        m_forceSegments.push_back(m_pendingForceSegment);
        m_pendingForceSegment.clear();
    }

    tryStartSimulation();
}

void DigitalTwinController::onForceDataUpdated(QStringList result)
{
    std::array<double, 3> forceData = { result[0].toDouble(), result[1].toDouble(), result[2].toDouble()};
    m_pendingForceSegment.push_back(forceData);
}

void DigitalTwinController::onInitializationFinished()
{
    m_initialized = true;
    tryStartSimulation();
}

void DigitalTwinController::onSegmentSimulationFinished()
{
    //当前段仿真完成，进入下一段
    m_isRunning = false;
    ++m_currentSegmentIndex;
    tryStartSimulation();
}

void DigitalTwinController::tryStartSimulation()
{
    qDebug() << "m_currentSegmentIndex" << m_currentSegmentIndex;

    if (!m_mdichild || m_viewer.IsNull() || !m_visulizationItem) {
        qDebug() << "Runtime context is not ready.";
        return;
    }

    if (!m_initialized) {
        qDebug() << "Simulation is not initialized.";
        return;}

    if (!m_simObj) {
        qDebug() << "Simulation object is null.";
        return;}

    if (m_isRunning) {
        qDebug() << "Previous segment is still running.";
        return;}

    // 至少需要两个mchData才能形成一段
    if (m_currentSegmentIndex + 1 >= static_cast<int>(m_mchDataList.size())) {
        return;}
    // 当前段必须已经有forceData
    if (m_currentSegmentIndex >= static_cast<int>(m_forceSegments.size())) {
        return;}

    const std::array<double, 8>& mchDataStart = m_mchDataList[m_currentSegmentIndex];
    const std::array<double, 8>& mchDataEnd = m_mchDataList[m_currentSegmentIndex + 1];
    const std::vector<std::array<double, 3>>& forceData = m_forceSegments[m_currentSegmentIndex];

    auto Msh_Data = buildStandardData(mchDataStart, mchDataEnd);

    // 求力均值
    std::vector<std::array<double, 3>> avgForceDataVector;
    const int totalForceCount = static_cast<int>(forceData.size());
    const int num = 10;

    if (totalForceCount > 0 && num > 0) {
        avgForceDataVector.reserve(num);
        for (int i = 0; i < num; ++i) {
            int beginIndex = i * totalForceCount / num;
            int endIndex = (i + 1) * totalForceCount / num;

            std::array<double, 3> avgForceData = { 0.0, 0.0, 0.0 };
            int count = endIndex - beginIndex;

            for (int j = beginIndex; j < endIndex; ++j) {
                avgForceData[0] += forceData[j][0];
                avgForceData[1] += forceData[j][1];
                avgForceData[2] += forceData[j][2];
            }
            if (count > 0) {
                avgForceData[0] /= count;
                avgForceData[1] /= count;
                avgForceData[2] /= count;
            }
            avgForceDataVector.push_back(avgForceData);
        }
    }
    else {
        // 如果该段没有采集到力数据，则给一组默认0力
        avgForceDataVector.push_back({ 0.0, 0.0, 0.0 });
    }


    // 传参
    m_simObj->setMch_Data(Msh_Data, avgForceDataVector);

    m_isRunning = true;

    digitaltwin_milling::DigitalTwinMilling* simObj = m_simObj;
    MdiChild* mdichild = m_mdichild;
    Handle(MyViewer) viewer = m_viewer;
    int* visItem = m_visulizationItem;
    std::array<double, 2> visLimits = m_visulizationLimits;


    QtConcurrent::run([this, simObj, mdichild, viewer, visItem, visLimits]() {
        simObj->performFEMSimulation(mdichild, viewer, visItem, visLimits);

        //从子线程调用主线程函数
        QMetaObject::invokeMethod(
            this,
            "onSegmentSimulationFinished",
            Qt::QueuedConnection
        );
        });
}

std::array<double, 14> DigitalTwinController::buildStandardData(const std::array<double, 8>& start, const std::array<double, 8>& end)
{
    std::array<double, 14> data{};

    data[0] = start[0];
    data[1] = start[1];

    data[2] = end[2];
    data[3] = end[3];
    data[4] = end[4];

    data[5] = start[2];
    data[6] = start[3];
    data[7] = start[4];

    data[8] = end[5];
    data[9] = end[6];
    data[10] = end[7];

    data[11] = start[5];
    data[12] = start[6];
    data[13] = start[7];

    return data;
}

void DigitalTwinController::setRuntimeContext(MdiChild* mdichild, Handle(MyViewer) viewer, int* visItem, std::array<double, 2> visLimits)
{
    m_mdichild = mdichild;
    m_viewer = viewer;
    m_visulizationItem = visItem;
    m_visulizationLimits = visLimits;
}