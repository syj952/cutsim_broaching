#ifndef CURVEWIDGET_H
#define CURVEWIDGET_H

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPainter>
//#include <QDockWidget>
#include <map>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include "cutsim.hpp"
#include "octnode.hpp"
#include "glvertex.hpp"
using namespace cutsim;
using namespace std;

class ForceCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit ForceCurveWidget(QWidget *parent = nullptr)
        : QWidget(parent), margin(50), curveWidth(2) {
        dialog = new QDialog(parent);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        //dialog->resize(850, 650);

        container = new QWidget(dialog);
        mainLayout = new QHBoxLayout(dialog);
        legendWidget = new QWidget(container);
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }

    //void plotForceCurves(const std::map<double, GLVertex>& angle_total_force) {
    //    setData(angle_total_force);
    //};
    void setData(const std::map<double, GLVertex>& data) {
        forceData = data;  
        
        /*legendWidget->setFixedWidth(150);
        QVBoxLayout* legendLayout = new QVBoxLayout(legendWidget);

        auto addLegendItem = [legendLayout, legendWidget](const QString& text, const QColor& color) {
            QLabel* label = new QLabel(legendWidget);
            label->setText(QString("<span style='color:%1;'>鈻?/span> %2")
                .arg(color.name())
                .arg(text));
            legendLayout->addWidget(label);
            };

        addLegendItem("X Component", Qt::red);
        addLegendItem("Y Component", Qt::green);
        addLegendItem("Z Component", Qt::blue);

        legendLayout->addStretch();
        legendWidget->setLayout(legendLayout);*/
        //mainLayout->addWidget(legendWidget);

        mainLayout->addWidget(this, 1);
        
        container->setLayout(mainLayout);

        QVBoxLayout* dialogLayout = new QVBoxLayout(dialog);
        dialogLayout->addWidget(container);

//dialog->setWindowTitle("Force Curves Visualization");
        this->show();
        //drawCurves(painter);
        //update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制背景和坐标轴
        drawAxes(painter);

        // 绘制曲线
        drawCurves(painter);
    }

private:
    void drawCurves(QPainter &painter) {
        if (forceData.empty()) return;

        QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);

        // 1. 首先计算所有分量的最小最大值
        float minX = forceData.empty() ? 0 : forceData.begin()->second.x;
        float maxX = minX;
        float minY = forceData.empty() ? 0 : forceData.begin()->second.y;
        float maxY = minY;
        float minZ = forceData.empty() ? 0 : forceData.begin()->second.z;
        float maxZ = minZ;

        for (const auto& [angle, vertex] : forceData) {
            minX = std::min(minX, vertex.x);
            maxX = std::max(maxX, vertex.x);
            minY = std::min(minY, vertex.y);
            maxY = std::max(maxY, vertex.y);
            minZ = std::min(minZ, vertex.z);
            maxZ = std::max(maxZ, vertex.z);
        }

        // 2. 计算全局最小最大值
        float minVal = std::min({minX, minY, minZ});
        float maxVal = std::max({maxX, maxY, maxZ});
        float range = maxVal - minVal;
        if (qFuzzyIsNull(range)) range = 1.0f; // 避免除以零


        // 绘制X分量曲线（红色）
        QPainterPath pathX;
        bool firstPoint = true;
        for (const auto& [angle, vertex] : forceData) {
            // X轴：角度线性映射到宽度
            int x = rect.left() + (angle - forceData.begin()->first) /
                    (forceData.rbegin()->first - forceData.begin()->first) * rect.width();
            // Y轴：直接使用原始值映射到高度范围
            int y = rect.bottom() - (vertex.x - minVal) / range * rect.height();


            if (firstPoint) {
                pathX.moveTo(x, y);
                firstPoint = false;
            } else {
                pathX.lineTo(x, y);
            }
        }
        painter.setPen(QPen(Qt::red, curveWidth));
        painter.drawPath(pathX);

        // 绘制Y分量曲线（绿色）同理
        QPainterPath pathY;
        firstPoint = true;
        for (const auto& [angle, vertex] : forceData) {
            int x = rect.left() + (angle - forceData.begin()->first) /
                    (forceData.rbegin()->first - forceData.begin()->first) * rect.width();
            int y = rect.bottom() - (vertex.y - minVal) / range * rect.height();

            if (firstPoint) {
                pathY.moveTo(x, y);
                firstPoint = false;
            } else {
                pathY.lineTo(x, y);
            }
        }
        painter.setPen(QPen(Qt::green, curveWidth));
        painter.drawPath(pathY);

        // 绘制Z分量曲线（蓝色）同理
        QPainterPath pathZ;
        firstPoint = true;
        for (const auto& [angle, vertex] : forceData) {
            int x = rect.left() + (angle - forceData.begin()->first) /
                    (forceData.rbegin()->first - forceData.begin()->first) * rect.width();
            int y = rect.bottom() - (vertex.z - minVal) / range * rect.height();

            if (firstPoint) {
                pathZ.moveTo(x, y);
                firstPoint = false;
            } else {
                pathZ.lineTo(x, y);
            }
        }
        painter.setPen(QPen(Qt::blue, curveWidth));
        painter.drawPath(pathZ);
    }
    void drawAxes(QPainter &painter) {
        QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);

        // 1. 首先计算所有分量的最小最大值
        float minX = forceData.empty() ? 0 : forceData.begin()->second.x;
        float maxX = minX;
        float minY = forceData.empty() ? 0 : forceData.begin()->second.y;
        float maxY = minY;
        float minZ = forceData.empty() ? 0 : forceData.begin()->second.z;
        float maxZ = minZ;

        for (const auto& [angle, vertex] : forceData) {
            minX = std::min(minX, vertex.x);
            maxX = std::max(maxX, vertex.x);
            minY = std::min(minY, vertex.y);
            maxY = std::max(maxY, vertex.y);
            minZ = std::min(minZ, vertex.z);
            maxZ = std::max(maxZ, vertex.z);
        }

        // 2. 计算全局最小最大值
        float minVal = std::min({minX, minY, minZ});
        float maxVal = std::max({maxX, maxY, maxZ});
        float range = maxVal - minVal;
        if (qFuzzyIsNull(range)) range = 1.0f; // 避免除以零

        // 3. 绘制坐标轴
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(rect.bottomLeft(), rect.bottomRight()); // X轴
        painter.drawLine(rect.bottomLeft(), rect.topLeft());     // Y轴

        // 4. 绘制X轴刻度
        painter.setFont(QFont("Arial", 9));
        if (!forceData.empty()) {
            double minAngle = forceData.begin()->first;
            double maxAngle = forceData.rbegin()->first;
            double angleStep = (maxAngle - minAngle) / 5;

            for (double angle = minAngle; angle <= maxAngle + 0.001; angle += angleStep) {
                int x = rect.left() + (angle-minAngle)/(maxAngle-minAngle) * rect.width();
                painter.drawLine(x, rect.bottom(), x, rect.bottom() + 5);
                painter.drawText(x - 20, rect.bottom() + 20,
                                 QString::number(angle, 'f', 1));
            }
        }

        // 5. 绘制Y轴刻度
        for (int i = 0; i <= 5; ++i) {
            float value = minVal + (range * i / 5);
            int y = rect.bottom() - (value - minVal) / range * rect.height();

            painter.drawLine(rect.left() - 5, y, rect.left(), y);
            painter.drawText(rect.left() - 50, y + 5,
                             QString::number(value, 'f', 4));
        }

        // 6. 绘制Y轴标签
        painter.save();
        painter.translate(rect.left() - 40, rect.top() + rect.height()/2);
        painter.rotate(-90);
        painter.drawText(0, 0, "Force Value (N)"); // 假设单位是牛顿
        painter.restore();
    }

private:
    std::map<double, GLVertex> forceData;
    int margin;
    int curveWidth;
    QPainter painter;
    QDialog* dialog;
    QWidget* container;
    QHBoxLayout* mainLayout;
    QWidget* legendWidget;
};

// 在ForceCurveWidget类定义后添加
class VibrationCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit VibrationCurveWidget(QWidget *parent = nullptr)
        : QWidget(parent), margin(50), pointSize(3) {
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }

    void setData(const std::unordered_map<Octnode*, std::vector<GLVertex>>& vertexMap,
                 const std::vector<GLVertex>& bladePoints) {
        // 提取所有顶点XZ坐标
        points.clear();
        bladePointsProjected.clear();

        for (const auto& [node, vertices] : vertexMap) {
            for (const auto& v : vertices) {
                // 存储原始Y值用于透明度计算
                points.emplace_back(v.x, v.z, v.y);
            }
        }

        for (const auto& v : bladePoints) {
            bladePointsProjected.emplace_back(v.x, v.z);
        }

        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制坐标轴
        drawAxes(painter);

        // 绘制刀具轨迹
        drawBladePath(painter);

        // 绘制振动点云
        drawVibrationPoints(painter);
    }

private:
    void drawAxes(QPainter &painter) {
        QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);

        // 计算坐标范围
        auto [xRange, zRange] = calculateRanges();
                float xMin = xRange.first, xMax = xRange.second;
                float zMin = zRange.first, zMax = zRange.second;

                // 绘制坐标轴
                painter.setPen(QPen(Qt::black, 2));
                painter.drawLine(rect.bottomLeft(), rect.bottomRight()); // X轴
                painter.drawLine(rect.bottomLeft(), rect.topLeft());     // Z轴

                // 设置字体
                painter.setFont(QFont("Arial", 9));

                // 绘制X轴刻度
                float xStep = (xMax - xMin) / 5;
                for (int i = 0; i <= 5; ++i) {
            float value = xMin + i * xStep;
            int xPos = rect.left() + (value - xMin)/(xMax - xMin) * rect.width();

            painter.drawLine(xPos, rect.bottom(), xPos, rect.bottom() + 5);
            painter.drawText(xPos - 20, rect.bottom() + 20,
                             QString::number(value, 'f', 2));
        }

        // 绘制Z轴刻度
        float zStep = (zMax - zMin) / 5;
        for (int i = 0; i <= 5; ++i) {
            float value = zMin + i * zStep;
            int zPos = rect.bottom() - (value - zMin)/(zMax - zMin) * rect.height();

            painter.drawLine(rect.left() - 5, zPos, rect.left(), zPos);
            painter.drawText(rect.left() - 50, zPos + 5,
                             QString::number(value, 'f', 2));
        }

        // 绘制轴标签
        painter.save();
        // X轴标签
        painter.drawText(rect.center().x(), rect.bottom() + 40, "X Coordinate");
        // Z轴标签
        painter.translate(rect.left() - 40, rect.top() + rect.height()/2);
        painter.rotate(-90);
        painter.drawText(0, 0, "Z Coordinate");
        painter.restore();
    }

    void drawVibrationPoints(QPainter &painter) {
        if (points.empty()) return;

        QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);
        auto [xRange, zRange] = calculateRanges();

                // 创建带Y值的数据结构并排序
                std::vector<std::tuple<float, float, float>> sortedPoints;
                for (const auto& [x, z, y] : points) { // 修改数据结构包含Y值
            sortedPoints.emplace_back(x, z, y);
        }
        std::sort(sortedPoints.begin(), sortedPoints.end(),
                  [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });

        // 计算Y值范围
        float minY = std::get<2>(sortedPoints.front());
        float maxY = std::get<2>(sortedPoints.back());
        float yRange = maxY - minY;
        if (qFuzzyIsNull(yRange)) yRange = 1.0f;

        painter.setPen(Qt::NoPen);

        // 绘制排序后的点
        for (const auto& [x, z, y] : sortedPoints) {
            // 计算透明度（Y值越大越透明）
            float alpha = (maxY - y) / yRange;
            alpha=alpha*alpha*alpha;
            painter.setBrush(QColor(255, 0, 0, alpha * 255)); // 红色带透明度

            int screenX = rect.left() + (x - xRange.first) / (xRange.second - xRange.first) * rect.width();
            int screenZ = rect.bottom() - (z - zRange.first) / (zRange.second - zRange.first) * rect.height();
            painter.drawEllipse(QPointF(screenX, screenZ), 2.0, 2.0);
        }
    }

    void drawBladePath(QPainter &painter) {
        if (bladePointsProjected.empty()) return;

        QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);
        auto [xRange, zRange] = calculateRanges();

        // 绘制连线
        QPen bladePen(QColor(0, 0, 255, 32), 5); // 添加透明度参数（128=50%透明度）
        painter.setPen(bladePen);
        QPainterPath path;

        bool first = true;
        for (const auto& [x, z] : bladePointsProjected) {
            int screenX = rect.left() + (x - xRange.first) / (xRange.second - xRange.first) * rect.width();
            int screenZ = rect.bottom() - (z - zRange.first) / (zRange.second - zRange.first) * rect.height();

            if (first) {
                path.moveTo(screenX, screenZ);
                first = false;
            } else {
                path.lineTo(screenX, screenZ);
            }
        }
        painter.drawPath(path);
    }

    std::pair<std::pair<float, float>, std::pair<float, float>> calculateRanges() {
        if (points.empty() && bladePointsProjected.empty()) {
            return {{0.0f, 1.0f}, {0.0f, 1.0f}};
        }

        // 初始化最小最大值
        float xMin = std::numeric_limits<float>::max();
        float xMax = std::numeric_limits<float>::min();
        float zMin = std::numeric_limits<float>::max();
        float zMax = std::numeric_limits<float>::min();

        // 处理points（三元组）
        for (const auto& [x, z, y] : points) {
            xMin = std::min(xMin, x);
            xMax = std::max(xMax, x);
            zMin = std::min(zMin, z);
            zMax = std::max(zMax, z);
        }

        // 处理bladePointsProjected（二元组）
        for (const auto& [x, z] : bladePointsProjected) {
            xMin = std::min(xMin, x);
            xMax = std::max(xMax, x);
            zMin = std::min(zMin, z);
            zMax = std::max(zMax, z);
        }

        // 处理所有点相同的情况
        if (qFuzzyCompare(xMin, xMax)) {
            xMin -= 0.5f;
            xMax += 0.5f;
        }
        if (qFuzzyCompare(zMin, zMax)) {
            zMin -= 0.5f;
            zMax += 0.5f;
        }

        return {{xMin, xMax}, {zMin, zMax}};
    }

    std::vector<std::tuple<float, float, float>> points;
    std::vector<std::pair<float, float>> bladePointsProjected;
    int margin;
    int pointSize;
};


void plotVibrationCurves(const std::unordered_map<Octnode*, std::vector<GLVertex>>& vertexMap,
                         const std::vector<GLVertex>& bladePoints);

void plotForceCurves(QWidget* dockWidget, const std::map<double, GLVertex>& angle_total_force);

// 在ForceCurveWidget类定义后添加

class StraightnessCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit StraightnessCurveWidget(QWidget *parent = nullptr)
        : QWidget(parent), margin(50), curveWidth(2) {
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }

    // 修改参数类型为新的map结构
    void setData(const std::map<double, std::map<int, double>>& data,
                const std::vector<int>& curveIds) {
        curveData = data;
        selectedIds = curveIds;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);

        // 计算坐标范围
        double minX = curveData.empty() ? 0 : curveData.begin()->first;
        double maxX = minX;
        double minVal = std::numeric_limits<double>::max();
        double maxVal = std::numeric_limits<double>::lowest();

        for (const auto& [xKey, innerMap] : curveData) {
            minX = std::min(minX, xKey);
            maxX = std::max(maxX, xKey);
            for (const auto& [id, value] : innerMap) {  // 现在value是double类型
                if (std::find(selectedIds.begin(), selectedIds.end(), id) != selectedIds.end()) {
                    minVal = std::min(minVal, value);  // 直接使用value
                    maxVal = std::max(maxVal, value);   // 直接使用value
                }
            }
        }

        // 绘制坐标轴
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(rect.bottomLeft(), rect.bottomRight()); // X轴
        painter.drawLine(rect.bottomLeft(), rect.topLeft());     // Y轴

        // 添加坐标轴刻度
        painter.setFont(QFont("Arial", 9));

        // X轴刻度
        double xStep = (maxX - minX)/5.0;
        for (int i = 0; i <= 5; ++i) {
            double value = minX + i*xStep;
            int xPos = rect.left() + static_cast<int>((value - minX)/(maxX - minX)*rect.width());

            // 绘制刻度线
            painter.drawLine(xPos, rect.bottom(), xPos, rect.bottom() + 5);

            // 绘制刻度值
            painter.drawText(xPos - 20, rect.bottom() + 20,
                            QString::number(value, 'f', 2));
        }

        // Y轴刻度
        double yStep = (maxVal - minVal)/5.0;
        for (int i = 0; i <= 5; ++i) {
            double value = minVal + i*yStep;
            int yPos = rect.bottom() - static_cast<int>((value - minVal)/(maxVal - minVal)*rect.height());

            // 绘制刻度线
            painter.drawLine(rect.left() - 5, yPos, rect.left(), yPos);

            // 绘制刻度值
            painter.drawText(rect.left() - 50, yPos + 5,
                            QString::number(value, 'f', 10));
        }

        // 绘制曲线
        const QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta};
        int colorIndex = 0;

        for (const auto& id : selectedIds) {
            QPainterPath path; // 只保留单一路径
            bool firstPoint = true;

            for (const auto& [xKey, innerMap] : curveData) {
                if (innerMap.find(id) != innerMap.end()) {
                    double x = rect.left() + (xKey - minX)/(maxX - minX) * rect.width();
                    // 直接使用数值代替pair
                    double y = rect.bottom() - (innerMap.at(id) - minVal)/(maxVal - minVal) * rect.height();

                    if (firstPoint) {
                        path.moveTo(x, y);
                        firstPoint = false;
                    } else {
                        path.lineTo(x, y);
                    }
                }
            }

            // 单一线条绘制
            painter.setPen(QPen(colors[colorIndex % 5], curveWidth));
            painter.drawPath(path);
            colorIndex++;
        }

    }

private:
    std::map<double, std::map<int, double>> curveData;
    std::vector<int> selectedIds;
    int margin;
    int curveWidth;
};

class RoughnessCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit RoughnessCurveWidget(QWidget *parent = nullptr)
        : QWidget(parent), margin(50), curveWidth(2) {
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
    }

    void setData(const std::map<double, std::map<int, double>>& data,
                const std::vector<int>& curveIds) {

        curveData = data;
        selectedIds = curveIds;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) {
        Q_UNUSED(event);
         QPainter painter(this);
         painter.setRenderHint(QPainter::Antialiasing);

         QRect rect = this->rect().adjusted(margin, margin, -margin, -margin);

         // 修正1：初始化minVal/maxVal为第一个有效值
         double minVal = std::numeric_limits<double>::max();
         double maxVal = std::numeric_limits<double>::lowest();
         double minX = DBL_MAX, maxX = -DBL_MAX;

         // 遍历数据计算范围
         for (const auto& [xKey, innerMap] : curveData) {
             // 修正2：正确处理X轴范围
             minX = std::min(minX, xKey);
             maxX = std::max(maxX, xKey);

             for (const auto& [id, value] : innerMap) {
                 if (std::find(selectedIds.begin(), selectedIds.end(), id) != selectedIds.end()) {
                     // 修正3：确保只处理选中的曲线ID
                     minVal = std::min(minVal, value);
                     maxVal = std::max(maxVal, value);
                 }
             }
         }

         // 修正4：处理全等数据的情况
         if (qFuzzyCompare(minVal, maxVal)) {
             maxVal += 1e-6;
             minVal -= 1e-6;
         }
         if (qFuzzyCompare(minX, maxX)) {
             maxX += 1e-6;
             minX -= 1e-6;
         }

         // 绘制坐标轴
         painter.setPen(QPen(Qt::black, 2));
         painter.drawLine(rect.bottomLeft(), rect.bottomRight()); // X轴
         painter.drawLine(rect.bottomLeft(), rect.topLeft());     // Y轴

         // 添加坐标轴刻度
         painter.setFont(QFont("Arial", 9));

         // X轴刻度（与StraightnessCurveWidget相同）
         double xStep = (maxX - minX)/5.0;
         for (int i = 0; i <= 5; ++i) {
             double value = minX + i*xStep;
             int xPos = rect.left() + static_cast<int>((value - minX)/(maxX - minX)*rect.width());
             painter.drawLine(xPos, rect.bottom(), xPos, rect.bottom() + 5);
             painter.drawText(xPos - 20, rect.bottom() + 20,
                             QString::number(value, 'f', 2));
         }

         // Y轴刻度
         double yStep = (maxVal - minVal)/5.0;
         for (int i = 0; i <= 5; ++i) {
             double value = minVal + i*yStep;
             int yPos = rect.bottom() - static_cast<int>((value - minVal)/(maxVal - minVal)*rect.height());
             painter.drawLine(rect.left() - 5, yPos, rect.left(), yPos);
             painter.drawText(rect.left() - 50, yPos + 5,
                             QString::number(value, 'f', 10));
         }

        // 绘制曲线
        const QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta};
        int colorIndex = 0;

        for (const auto& id : selectedIds) {
            QPainterPath path;
            bool firstPoint = true;

            for (const auto& [xKey, innerMap] : curveData) {
                if (innerMap.find(id) != innerMap.end()) {
                    double x = rect.left() + (xKey - minX)/(maxX - minX) * rect.width();
                    double y = rect.bottom() - (innerMap.at(id) - minVal)/(maxVal - minVal) * rect.height();

                    if (firstPoint) {
                        path.moveTo(x, y);
                        firstPoint = false;
                    } else {
                        path.lineTo(x, y);
                    }
                }
            }

            painter.setPen(QPen(colors[colorIndex % 5], curveWidth));
            painter.drawPath(path);
            colorIndex++;
        }
    }

private:
    std::map<double, std::map<int, double>> curveData;
    std::vector<int> selectedIds;
    int margin;
    int curveWidth;
};

// 在文件末尾函数声明处添加
void plotStraightnessCurves(const std::map<double, std::map<int, double>>& straightness_map,
                           const std::vector<int>& curveIds);

void plotRoughnessCurves(const std::map<double, std::map<int, double>>& ra_map,
                        const std::vector<int>& curveIds);
#endif