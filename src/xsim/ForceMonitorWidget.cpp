#include "ForceMonitorWidget.hpp"
#include <QVBoxLayout>

ForceMonitorWidget::ForceMonitorWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ForceMonitorWidget::setupUI() {
    chart = new QChart();
    //chart->setTitle("������ - λ����������");
    chart->legend()->setAlignment(Qt::AlignTop);

    seriesX = new QLineSeries(this); seriesX->setName("Fx");
    seriesY = new QLineSeries(this); seriesY->setName("Fy");
    seriesZ = new QLineSeries(this); seriesZ->setName("Fz");

    chart->addSeries(seriesX);
    chart->addSeries(seriesY);
    chart->addSeries(seriesZ);

    // ����λ�ƺ���
    axisDisplacement = new QValueAxis();
    axisDisplacement->setTitleText("Stroke (mm)");
    axisDisplacement->setRange(minDis, maxDis);
    axisDisplacement->setLabelFormat("%.2f");

    // ����������
    axisForce = new QValueAxis();
    axisForce->setTitleText("Force (N)");
    axisForce->setRange(-50, 500); // ����ʵ�ʹ���Ԥ��

    chart->addAxis(axisDisplacement, Qt::AlignBottom);
    chart->addAxis(axisForce, Qt::AlignLeft);

    seriesX->attachAxis(axisDisplacement); seriesX->attachAxis(axisForce);
    seriesY->attachAxis(axisDisplacement); seriesY->attachAxis(axisForce);
    seriesZ->attachAxis(axisDisplacement); seriesZ->attachAxis(axisForce);

    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing); // �����

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}

void ForceMonitorWidget::updateData(double displacement, double fx, double fy, double fz) {
    // ������ݵ�
    seriesX->append(displacement, fx);
    seriesY->append(displacement, fy);
    seriesZ->append(displacement, fz);

    // ��̬���� X �᷶Χ
    if (displacement > maxDis) {
        maxDis = displacement * 1.2; // ��� 20% ������
        axisDisplacement->setRange(minDis, maxDis);
    }

    // 3. ������������ֵ����Ӧ
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
    // Ϊ���������� 15% �ı߾࣬������������
    double padding = (maxForce - minForce) * 0.15;
    // �������������С�ҽӽ������綼��0������һ��Ĭ����С����
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
