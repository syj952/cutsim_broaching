#include "ForceMonitorWidget.hpp"
#include <QVBoxLayout>

ForceMonitorWidget::ForceMonitorWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ForceMonitorWidget::setupUI() {
    chart = new QChart();
    //chart->setTitle("切削力 - 位移特性曲线");
    chart->legend()->setAlignment(Qt::AlignTop);

    seriesX = new QLineSeries(this); seriesX->setName("Fx");
    seriesY = new QLineSeries(this); seriesY->setName("Fy");
    seriesZ = new QLineSeries(this); seriesZ->setName("Fz");

    chart->addSeries(seriesX);
    chart->addSeries(seriesY);
    chart->addSeries(seriesZ);

    // 配置位移横轴
    axisDisplacement = new QValueAxis();
    axisDisplacement->setTitleText("Stroke (mm)");
    axisDisplacement->setRange(minDis, maxDis);
    axisDisplacement->setLabelFormat("%.2f");

    // 配置力纵轴
    axisForce = new QValueAxis();
    axisForce->setTitleText("Force (N)");
    axisForce->setRange(-50, 500); // 根据实际工况预设

    chart->addAxis(axisDisplacement, Qt::AlignBottom);
    chart->addAxis(axisForce, Qt::AlignLeft);

    seriesX->attachAxis(axisDisplacement); seriesX->attachAxis(axisForce);
    seriesY->attachAxis(axisDisplacement); seriesY->attachAxis(axisForce);
    seriesZ->attachAxis(axisDisplacement); seriesZ->attachAxis(axisForce);

    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing); // 抗锯齿

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}

void ForceMonitorWidget::updateData(double displacement, double fx, double fy, double fz) {
    // 添加数据点
    seriesX->append(displacement, fx);
    seriesY->append(displacement, fy);
    seriesZ->append(displacement, fz);

    // 动态调整 X 轴范围
    if (displacement > maxDis) {
        maxDis = displacement * 1.2; // 留出 20% 的余量
        axisDisplacement->setRange(minDis, maxDis);
    }

    // 3. 处理纵坐标力值自适应
    double currentMin = std::min({ fx, fy, fz });
    double currentMax = std::max({ fx, fy, fz });

    if (firstPoint) {
        minForce = currentMin;
        maxForce = currentMax;
        firstPoint = false;
    }
    else {
        if (currentMin < minForce) minForce = currentMin;
        if (currentMax > maxForce) maxForce = currentMax;
    }
    // 为坐标轴设置 15% 的边距，避免曲线贴边
    double padding = (maxForce - minForce) * 0.15;
    // 如果三个力都很小且接近（例如都是0），给一个默认最小量程
    if (padding < 1.0) padding = 10.0;

    axisForce->setRange(minForce - padding, maxForce + padding);
}

void ForceMonitorWidget::clearData() {
    seriesX->clear();
    seriesY->clear();
    seriesZ->clear();
    minForce = 0.0;
    maxForce = 100.0;
    maxDis = 10.0;
    firstPoint = true;
    axisDisplacement->setRange(0, maxDis);
    axisForce->setRange(minForce, maxForce);
}
