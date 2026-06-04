#ifndef STRAIGHTNESSMONITORWIDGET_HPP
#define STRAIGHTNESSMONITORWIDGET_HPP

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <vector>

QT_CHARTS_USE_NAMESPACE

class StraightnessMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StraightnessMonitorWidget(QWidget* parent = nullptr);
    ~StraightnessMonitorWidget();

public slots:
    void setData(int blade_id, int point_index,double stroke_data, double straightness_data);
    void updateBladePointSelection(int blade_count, int point_count);

signals:
    // 当用户选择了新的刀刃或点时发送信号
    void bladePointSelected(int blade_id, int point_index);
    
private slots:
    void onBladeChanged(int index);
    void onPointChanged(int index);

private:
    void setupUI();
    void setupChart();
    void clearChart();
    void updateChartWithCurrentSelection();

    QChart* chart;
    QChartView* chartView;
    QLineSeries* series;
    QValueAxis* axisX;
    QValueAxis* axisY;

    QComboBox* bladeComboBox;
    QComboBox* pointComboBox;
    QLabel* currentSelectionLabel;

    int currentBladeId;
    int currentPointIndex;
    
    // 存储历史数据用于图表显示
    std::vector<double> strokeHistory;
    std::vector<double> straightnessHistory;

    double currentStrokeData;
    double currentStraightnessData;
};

#endif // STRAIGHTNESSMONITORWIDGET_HPP