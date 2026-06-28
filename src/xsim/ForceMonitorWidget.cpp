#include "ForceMonitorWidget.hpp"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <limits>

ForceMonitorWidget::ForceMonitorWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ForceMonitorWidget::setupUI() {
    chart = new QChart();
    //chart->setTitle("????????? - ????????????????");
    chart->legend()->setAlignment(Qt::AlignTop);

    seriesX = new QLineSeries(this); seriesX->setName("Fx");
    seriesY = new QLineSeries(this); seriesY->setName("Fy");
    seriesZ = new QLineSeries(this); seriesZ->setName("Fz");

    chart->addSeries(seriesX);
    chart->addSeries(seriesY);
    chart->addSeries(seriesZ);

    // ??????????????
    axisDisplacement = new QValueAxis();
    axisDisplacement->setTitleText("Stroke (mm)");
    maxDis = displacementAxisWindowMm;
    axisDisplacement->setRange(minDis, maxDis);
    axisDisplacement->setLabelFormat("%.2f");

    // ???????????????
    axisForce = new QValueAxis();
    axisForce->setTitleText("Force (N)");
    axisForce->setRange(-50, 500); // ??????????????????

    chart->addAxis(axisDisplacement, Qt::AlignBottom);
    chart->addAxis(axisForce, Qt::AlignLeft);

    seriesX->attachAxis(axisDisplacement); seriesX->attachAxis(axisForce);
    seriesY->attachAxis(axisDisplacement); seriesY->attachAxis(axisForce);
    seriesZ->attachAxis(axisDisplacement); seriesZ->attachAxis(axisForce);

    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing); // ????????

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}

void ForceMonitorWidget::updateData(double displacement, double fx, double fy, double fz) {
    // ????????????
    seriesX->append(displacement, fx);
    seriesY->append(displacement, fy);
    seriesZ->append(displacement, fz);

    // ?????????? X ????
    const double window = std::max(displacementAxisWindowMm, 1.0);
    maxDis = std::max(window, displacement);
    minDis = maxDis - window;
    axisDisplacement->setRange(minDis, maxDis);

    // 3. ??????????????????????????
    updateForceAxis();
    // ???????????????? 15% ???????????????????????
    // ?????????????????????????????????????????????????????????????????
}

void ForceMonitorWidget::updateForceAxis() {
    const QVector<QPointF> xPoints = seriesX->pointsVector();
    const QVector<QPointF> yPoints = seriesY->pointsVector();
    const QVector<QPointF> zPoints = seriesZ->pointsVector();
    const int count = qMin(xPoints.size(), qMin(yPoints.size(), zPoints.size()));
    if (count <= 0) {
        return;
    }

    const int start = std::max(0, count - forceAxisWindowPoints);
    double currentMin = std::numeric_limits<double>::max();
    double currentMax = -std::numeric_limits<double>::max();

    auto includePoint = [&](double value) {
        if (!std::isfinite(value)) {
            return;
        }
        currentMin = std::min(currentMin, value);
        currentMax = std::max(currentMax, value);
    };

    for (int i = start; i < count; ++i) {
        includePoint(xPoints[i].y());
        includePoint(yPoints[i].y());
        includePoint(zPoints[i].y());
    }

    if (currentMax < currentMin) {
        return;
    }

    double span = currentMax - currentMin;
    if (span < minForceAxisSpan) {
        const double center = 0.5 * (currentMin + currentMax);
        span = minForceAxisSpan;
        currentMin = center - 0.5 * span;
        currentMax = center + 0.5 * span;
    }

    const double padding = std::max(span * 0.15, 1.0);
    minForce = currentMin - padding;
    maxForce = currentMax + padding;
    axisForce->setRange(minForce, maxForce);
}

void ForceMonitorWidget::clearData() {
    seriesX->clear();
    seriesY->clear();
    seriesZ->clear();
    minForce = 0.0;
    maxForce = 100.0;
    minDis = 0.0;
    maxDis = displacementAxisWindowMm;
    firstPoint = true;
    axisDisplacement->setRange(0, maxDis);
    axisForce->setRange(minForce, maxForce);
}

bool ForceMonitorWidget::hasData() const {
    return seriesX && seriesX->count() > 0;
}

bool ForceMonitorWidget::exportDataToCsv(const QString& filePath, QString* errorMessage) const {
    if (!hasData()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No cutting force data to export.");
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot create cutting force data file: ") + filePath;
        }
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(9);
    out << "stroke,Fx,Fy,Fz\n";

    const QVector<QPointF> xPoints = seriesX->pointsVector();
    const QVector<QPointF> yPoints = seriesY->pointsVector();
    const QVector<QPointF> zPoints = seriesZ->pointsVector();
    const int count = qMin(xPoints.size(), qMin(yPoints.size(), zPoints.size()));
    for (int i = 0; i < count; ++i) {
        out << xPoints[i].x() << ","
            << xPoints[i].y() << ","
            << yPoints[i].y() << ","
            << zPoints[i].y() << "\n";
    }

    return true;
}
