#ifndef FORCEMONITORWIDGET_H
#define FORCEMONITORWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QString>

QT_CHARTS_USE_NAMESPACE

class ForceMonitorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ForceMonitorWidget(QWidget* parent = nullptr);

    // ���½ӿڣ�����λ�� displacement �� ������
    void updateData(double displacement, double fx, double fy, double fz);

    // ������ݣ����翪ʼ��һ������ʱ��
    void clearData();
    bool exportDataToCsv(const QString& filePath, QString* errorMessage = nullptr) const;
    bool hasData() const;

private:
    void setupUI();
    void updateForceAxis();

    QChart* chart;
    QLineSeries* seriesX;
    QLineSeries* seriesY;
    QLineSeries* seriesZ;
    QValueAxis* axisDisplacement; // ���᣺λ��
    QValueAxis* axisForce;        // ���᣺��

    double minDis = 0.0;
    double displacementAxisWindowMm = 200.0;
    double maxDis = 10.0; // ��ʼ��Χ 10mm
    double minForce = 0.0;
    int forceAxisWindowPoints = 30;
    double minForceAxisSpan = 10.0;
    double maxForce = 100.0; // ��ʼ����
    bool firstPoint = true;  // ����Ƿ�Ϊ��һ���㣬���ڳ�ʼ����Χ
};

#endif

