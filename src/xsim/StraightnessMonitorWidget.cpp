#include "StraightnessMonitorWidget.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDebug>

StraightnessMonitorWidget::StraightnessMonitorWidget(QWidget* parent)
    : QWidget(parent), currentBladeId(0), currentPointIndex(0)
{
    setupChart();  // 先创建图表和chartView
    setupUI();     // 再设置UI布局
}

StraightnessMonitorWidget::~StraightnessMonitorWidget()
{
    delete chart;
}

void StraightnessMonitorWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 选择控件区域
    QHBoxLayout* selectionLayout = new QHBoxLayout();

    QLabel* bladeLabel = new QLabel("刀刃:");
    bladeComboBox = new QComboBox();
    connect(bladeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &StraightnessMonitorWidget::onBladeChanged);

    QLabel* pointLabel = new QLabel("点:");
    pointComboBox = new QComboBox();
    connect(pointComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &StraightnessMonitorWidget::onPointChanged);

    currentSelectionLabel = new QLabel("当前: 刀刃0 - 点0");

    selectionLayout->addWidget(bladeLabel);
    selectionLayout->addWidget(bladeComboBox);
    selectionLayout->addWidget(pointLabel);
    selectionLayout->addWidget(pointComboBox);
    selectionLayout->addWidget(currentSelectionLabel);
    selectionLayout->addStretch();

    mainLayout->addLayout(selectionLayout);
    mainLayout->addWidget(chartView);
}

void StraightnessMonitorWidget::setupChart()
{
    chart = new QChart();
    chart->setTitle("直线度曲线");
    chart->setAnimationOptions(QChart::NoAnimation);

    series = new QLineSeries();
    series->setName("直线度");

    chart->addSeries(series);

    axisX = new QValueAxis();
    axisX->setTitleText("行程 (mm)");
    axisX->setLabelFormat("%.1f");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    axisY = new QValueAxis();
    axisY->setTitleText("直线度 (mm)");
    axisY->setLabelFormat("%.4f"); // 修改为4位小数
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
}

void StraightnessMonitorWidget::setData(int blade_id, int point_index,
    double stroke_data, double straightness_data)
{

    // 更新当前选择
    currentBladeId = blade_id;
    currentPointIndex = point_index;

    // 将新数据添加到历史记录中
    strokeHistory.push_back(stroke_data);
    straightnessHistory.push_back(straightness_data);

    // 限制历史数据大小（避免内存无限增长）
    const size_t maxHistorySize = 1000;
    if (strokeHistory.size() > maxHistorySize) {
        strokeHistory.erase(strokeHistory.begin());
        straightnessHistory.erase(straightnessHistory.begin());
    }

    // 更新图表
    updateChartWithCurrentSelection();

    // 更新选择标签
    currentSelectionLabel->setText(QString("当前: 刀刃%1 - 点%2").arg(blade_id).arg(point_index));
}

void StraightnessMonitorWidget::updateBladePointSelection(int blade_count, int point_count)
{
    bladeComboBox->clear();
    pointComboBox->clear();

    for (int i = 0; i < blade_count; ++i)
    {
        bladeComboBox->addItem(QString("刀刃%1").arg(i));
    }

    for (int i = 0; i < point_count; ++i)
    {
        pointComboBox->addItem(QString("点%1").arg(i));
    }
}

void StraightnessMonitorWidget::onBladeChanged(int index)
{
    currentBladeId = index;
    // 更新当前选择显示
    currentSelectionLabel->setText(QString("当前: 刀刃%1 - 点%2").arg(currentBladeId).arg(currentPointIndex));
    
    // 发送选择变化信号
    emit bladePointSelected(currentBladeId, currentPointIndex);
    
    // 重新绘制当前选择的刀刃和点的数据
    updateChartWithCurrentSelection();
}

void StraightnessMonitorWidget::onPointChanged(int index)
{
    currentPointIndex = index;
    // 更新当前选择显示
    currentSelectionLabel->setText(QString("当前: 刀刃%1 - 点%2").arg(currentBladeId).arg(currentPointIndex));
    
    // 发送选择变化信号
    emit bladePointSelected(currentBladeId, currentPointIndex);
    
    // 重新绘制当前选择的刀刃和点的数据
    updateChartWithCurrentSelection();
}

void StraightnessMonitorWidget::clearChart()
{
    series->clear();
}

void StraightnessMonitorWidget::updateChartWithCurrentSelection()
{
    // 清空图表
    clearChart();

    // 检查是否有存储的数据
    if (strokeHistory.empty() || straightnessHistory.empty()) {
        qDebug() << "No data available for blade" << currentBladeId << "point" << currentPointIndex;
        return;
    }

    // 重新绘制历史数据
    for (size_t i = 0; i < strokeHistory.size() && i < straightnessHistory.size(); ++i)
    {
        series->append(strokeHistory[i], straightnessHistory[i]);
    }

    // 更新坐标轴范围
    if (!strokeHistory.empty() && !straightnessHistory.empty())
    {
        axisX->setRange(*std::min_element(strokeHistory.begin(), strokeHistory.end()),
            *std::max_element(strokeHistory.begin(), strokeHistory.end()));
        axisY->setRange(*std::min_element(straightnessHistory.begin(), straightnessHistory.end()),
            *std::max_element(straightnessHistory.begin(), straightnessHistory.end()));
    }

}