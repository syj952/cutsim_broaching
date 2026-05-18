#include "curve_widget.hpp"

//void ForceCurveWidget::plotForceCurves(const std::map<double, GLVertex>& angle_total_force) {
//    
//    setData(angle_total_force);
//
//}

void plotForceCurves(QWidget* dockWidget, const std::map<double, GLVertex>&angle_total_force) {
    QDialog* dialog = new QDialog(dockWidget);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    //dialog->resize(850, 650);

    QWidget* container = new QWidget(dialog);
    QHBoxLayout* mainLayout = new QHBoxLayout(dialog);

    ForceCurveWidget* widget = new ForceCurveWidget(container);
    widget->setData(angle_total_force);

    QWidget* legendWidget = new QWidget(container);
    legendWidget->setFixedWidth(150);
    QVBoxLayout* legendLayout = new QVBoxLayout(legendWidget);

    auto addLegendItem = [legendLayout, legendWidget](const QString& text, const QColor& color) {
        QLabel* label = new QLabel(legendWidget);
        label->setText(QString("<span style='color:%1;'>�?/span> %2")
                       .arg(color.name())
                       .arg(text));
        legendLayout->addWidget(label);
    };

    addLegendItem("X Component", Qt::red);
    addLegendItem("Y Component", Qt::green);
    addLegendItem("Z Component", Qt::blue);

    legendLayout->addStretch();
    legendWidget->setLayout(legendLayout);

    mainLayout->addWidget(widget, 1);
    mainLayout->addWidget(legendWidget);
    container->setLayout(mainLayout);

    QVBoxLayout* dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->addWidget(container);

    dialog->setWindowTitle("Force Curves Visualization");
    dialog->show();
}

void plotVibrationCurves(const std::unordered_map<Octnode*, std::vector<GLVertex>>& vertexMap,
                          const std::vector<GLVertex>& bladePoints) {
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(500, 400);

    VibrationCurveWidget* widget = new VibrationCurveWidget(dialog);
    widget->setData(vertexMap, bladePoints);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->addWidget(widget);
    dialog->setLayout(layout);

    dialog->setWindowTitle("X-Z Vibration Profile");
    dialog->show();
}

// 添加plot函数实现
void plotStraightnessCurves(const std::map<double, std::map<int, double>>& straightness_map,
                           const std::vector<int>& curveIds) {
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(850, 650);

    QWidget* container = new QWidget(dialog);
    QHBoxLayout* mainLayout = new QHBoxLayout();

    StraightnessCurveWidget* widget = new StraightnessCurveWidget(container);
    widget->setData(straightness_map, curveIds);

    // 添加图例
    QWidget* legendWidget = new QWidget(container);
    legendWidget->setFixedWidth(150);
    QVBoxLayout* legendLayout = new QVBoxLayout(legendWidget);

    auto addLegendItem = [legendLayout, legendWidget](const QString& text, const QColor& color) {
        QLabel* label = new QLabel(legendWidget);
        label->setText(QString("<span style='color:%1;'>�?/span> %2")
                       .arg(color.name())
                       .arg(text));
        legendLayout->addWidget(label);
    };

    const QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta};
    for (size_t i = 0; i < curveIds.size(); ++i) {
        addLegendItem(QString("ID %1").arg(curveIds[i]), colors[i % 5]);
    }

    legendLayout->addStretch();
    legendWidget->setLayout(legendLayout);

    mainLayout->addWidget(widget, 1);
    mainLayout->addWidget(legendWidget);
    container->setLayout(mainLayout);

    QVBoxLayout* dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->addWidget(container);

    dialog->setWindowTitle("Straightness Curves");
    dialog->show();
}

void plotRoughnessCurves(const std::map<double, std::map<int, double>>& ra_map,
                        const std::vector<int>& curveIds) {
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(850, 650);

    QWidget* container = new QWidget(dialog);
    QHBoxLayout* mainLayout = new QHBoxLayout();

    RoughnessCurveWidget* widget = new RoughnessCurveWidget(container);
    widget->setData(ra_map, curveIds);

    // 添加图例（同上）
    QWidget* legendWidget = new QWidget(container);
    legendWidget->setFixedWidth(150);
    QVBoxLayout* legendLayout = new QVBoxLayout(legendWidget);

    auto addLegendItem = [legendLayout, legendWidget](const QString& text, const QColor& color) {
        QLabel* label = new QLabel(legendWidget);
        label->setText(QString("<span style='color:%1;'>�?/span> %2")
                       .arg(color.name())
                       .arg(text));
        legendLayout->addWidget(label);
    };

    const QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta};
    for (size_t i = 0; i < curveIds.size(); ++i) {
        addLegendItem(QString("ID %1").arg(curveIds[i]), colors[i % 5]);
    }

    legendLayout->addStretch();
    legendWidget->setLayout(legendLayout);

    mainLayout->addWidget(widget, 1);
    mainLayout->addWidget(legendWidget);
    container->setLayout(mainLayout);

    QVBoxLayout* dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->addWidget(container);

    dialog->setWindowTitle("Surface Roughness (Ra) Curves");
    dialog->show();
}
