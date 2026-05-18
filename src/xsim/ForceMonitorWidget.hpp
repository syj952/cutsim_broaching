#ifndef FORCEMONITORWIDGET_H
#define FORCEMONITORWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

class ForceMonitorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ForceMonitorWidget(QWidget* parent = nullptr);

    // 更新接口：传入位移 displacement 和 三向力
    void updateData(double displacement, double fx, double fy, double fz);

    // 清除数据（例如开始新一轮切削时）
    void clearData();

private:
    void setupUI();

    QChart* chart;
    QLineSeries* seriesX;
    QLineSeries* seriesY;
    QLineSeries* seriesZ;
    QValueAxis* axisDisplacement; // 横轴：位移
    QValueAxis* axisForce;        // 纵轴：力

    double minDis = 0.0;
    double maxDis = 10.0; // 初始范围 10mm
    double minForce = 0.0;
    double maxForce = 100.0; // 初始量程
    bool firstPoint = true;  // 标记是否为第一个点，用于初始化范围
};

#endif

