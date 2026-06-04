#pragma once

#include <QObject>
#include <QStringList>
#include <array>
#include <vector>
#include <QtConcurrent/QtConcurrentRun>
#include "src/xsim/myviewer.h"
#include "src/xsim/mdichild.h"

namespace digitaltwin_milling {
    class DigitalTwinMilling;
}

class DigitalTwinController : public QObject
{
    Q_OBJECT

public:
    explicit DigitalTwinController(QObject* parent = nullptr);
    void setSimulationObject(digitaltwin_milling::DigitalTwinMilling* simObj);
    void setRuntimeContext(MdiChild* mdichild, Handle(MyViewer) viewer, int* visItem, std::array<double, 2> visLimits);

public slots:
    void onMchDataUpdated(std::array<double, 8> mchData);
    void onForceDataUpdated(QStringList result);
    void onInitializationFinished();
    void onSegmentSimulationFinished();

private:
    void tryStartSimulation();
    std::array<double, 14> buildStandardData(const std::array<double, 8>& start, const std::array<double, 8>& end);

private:
    digitaltwin_milling::DigitalTwinMilling* m_simObj = nullptr;

    bool m_initialized = false;
    bool m_isRunning = false;

    std::vector<std::array<double, 8>> m_mchDataList;
    std::vector<std::vector<std::array<double, 3>>> m_forceSegments;
    std::vector<std::array<double, 3>> m_pendingForceSegment;

    int m_currentSegmentIndex = 0;// 当前准备仿真的段号
    
private:
    MdiChild* m_mdichild = nullptr;
    Handle(MyViewer) m_viewer;
    int* m_visulizationItem = nullptr;
    std::array<double, 2> m_visulizationLimits{};
    int aaaccc = 0;
};