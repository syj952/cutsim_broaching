#include "PropertyView.h"
#include <QWidget>
#include <QtWidgets>
#include <QTableWidget>
#include "Msg.h"
#include "ComplainUtf8.h" //添加utf8文档
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <Geom_Point.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp_Face.hxx>
#include "OCCT_GraphOperations.h"
#include <Standard_Handle.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>

PropertyView::PropertyView(QWidget* parent)
{
	this->initWidget();
	this->setupTableStyle();
}

PropertyView::~PropertyView()
{
}

void PropertyView::initWidget()
{
	table = new QTableWidget(this);

	// 基本表格设置
	table->setColumnCount(2);
	table->setColumnWidth(0, 120); // 属性名称列宽
	table->setColumnWidth(1, 500); // 属性值列宽

	table->verticalHeader()->setVisible(false);   // 隐藏行标题
	table->horizontalHeader()->setVisible(false); // 隐藏列标题
	table->setSelectionMode(QAbstractItemView::NoSelection); // 禁止选择
	table->setFocusPolicy(Qt::NoFocus); // 去除焦点框

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(table);
	layout->setContentsMargins(2, 2, 2, 2); // 稍微留点边距
	this->setLayout(layout);
}	

void PropertyView::setupTableStyle()
{
	// 设置表格样式
	table->setStyleSheet(R"(
        QTableWidget {
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            gridline-color: #dee2e6;
            outline: 0;
        }
        QTableWidget::item {
            border: none;
            padding: 6px 8px;
            border-bottom: 1px solid #e9ecef;
        }
        QTableWidget::item:first {
            border-left: none;
        }
        QTableWidget::item:last {
            border-right: none;
        }
        QTableWidget::item:selected {
            background-color: transparent;
        }
    )");
}
void PropertyView::addPropertyRow(const QString& name, const QString& value, int row)
{
	// 添加属性名称单元格
	QTableWidgetItem* nameItem = new QTableWidgetItem(name);
	nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
	nameItem->setBackground(QColor(240, 242, 245));
	nameItem->setFont(QFont("Microsoft YaHei", 8, QFont::Medium));
	nameItem->setTextColor(QColor(73, 80, 87));

	// 添加属性值单元格
	QTableWidgetItem* valueItem = new QTableWidgetItem(value);
	valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
	valueItem->setFont(QFont("Consolas", 9));
	valueItem->setTextColor(QColor(33, 37, 41));

	table->setItem(row, 0, nameItem);
	table->setItem(row, 1, valueItem);
}

void PropertyView::setShapeView(const TopoDS_Shape& shape)
{
    

    QString type;
    int rowCount = 1;

    switch (shape.ShapeType()) {
    case TopAbs_VERTEX: {
        type = "顶点(Vertex)";
        gp_Pnt pnt = BRep_Tool::Pnt(TopoDS::Vertex(shape));
        rowCount = 2;
        table->setRowCount(rowCount);

        addPropertyRow("OCC形状类型", type, 0);
        addPropertyRow("点坐标",
            QString("(%1, %2, %3)")
            .arg(pnt.X(), 0, 'f', 3)
            .arg(pnt.Y(), 0, 'f', 3)
            .arg(pnt.Z(), 0, 'f', 3), 1);
        break;
    }
    case TopAbs_EDGE: {
        type = "边(Edge)";
        BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
        GeomAbs_CurveType curveType = curveAdaptor.GetType();

        QString curveTypeStr;
        switch (curveType) {
        case GeomAbs_Line: curveTypeStr = "直线"; break;
        case GeomAbs_Circle: curveTypeStr = "圆"; break;
        case GeomAbs_Ellipse: curveTypeStr = "椭圆"; break;
        case GeomAbs_Hyperbola: curveTypeStr = "双曲线"; break;
        case GeomAbs_Parabola: curveTypeStr = "抛物线"; break;
        case GeomAbs_BezierCurve: curveTypeStr = "Bezier曲线"; break;
        case GeomAbs_BSplineCurve: {
            gp_Pnt center;
            double radius = 0;
            gp_Pnt startPoint, endPoint;

            if (OCCT_GraphOperations::IsCirAcr(TopoDS::Edge(shape), center, radius, startPoint, endPoint)) {
                curveTypeStr = "圆弧";
                rowCount = 4;
                table->setRowCount(rowCount);

                addPropertyRow("OCC形状类型", type, 0);
                addPropertyRow("边类型", curveTypeStr, 1);
                addPropertyRow("圆心坐标",
                    QString("(%1, %2, %3)")
                    .arg(center.X(), 0, 'f', 1)
                    .arg(center.Y(), 0, 'f', 1)
                    .arg(center.Z(), 0, 'f', 1), 2);
                addPropertyRow("半径", QString::number(radius, 'f', 3), 3);
            }
            else {
                curveTypeStr = "BSpline曲线";
                rowCount = 2;
                table->setRowCount(rowCount);
                addPropertyRow("OCC形状类型", type, 0);
                addPropertyRow("边类型", curveTypeStr, 1);
            }
            break;
        }
        default: curveTypeStr = "未知类型"; break;
        }

        if (curveType != GeomAbs_BSplineCurve || rowCount == 2) {
            table->setRowCount(rowCount);
            addPropertyRow("OCC形状类型", type, 0);
            addPropertyRow("边类型", curveTypeStr, 1);
        }
        break;
    }
    case TopAbs_FACE: {
        type = "面(Face)";
        BRepAdaptor_Surface surfaceAdaptor;
        surfaceAdaptor.Initialize(TopoDS::Face(shape));
        GeomAbs_SurfaceType surfaceType = surfaceAdaptor.GetType();

        QString surfaceTypeStr;
        switch (surfaceType) {
        case GeomAbs_Plane: surfaceTypeStr = "平面"; break;
        case GeomAbs_Cylinder: surfaceTypeStr = "圆柱面"; break;
        case GeomAbs_Cone: surfaceTypeStr = "圆锥面"; break;
        case GeomAbs_Sphere: surfaceTypeStr = "球面"; break;
        case GeomAbs_Torus: surfaceTypeStr = "环面"; break;
        case GeomAbs_BezierSurface: surfaceTypeStr = "Bezier曲面"; break;
        case GeomAbs_BSplineSurface: surfaceTypeStr = "BSpline曲面"; break;
        default: surfaceTypeStr = "未知类型"; break;
        }

        table->setRowCount(2);
        addPropertyRow("OCC形状类型", type, 0);
        addPropertyRow("面类型", surfaceTypeStr, 1);
        break;
    }
    case TopAbs_WIRE: type = "线框(Wire)"; break;
    case TopAbs_SOLID: type = "实体(Solid)"; break;
    case TopAbs_COMPOUND: type = "组合体(Compound)"; break;
    case TopAbs_SHELL: type = "壳体(Shell)"; break;
    default: type = "未知类型(Unknown)"; break;
    }

    // 对于简单类型只显示基本类型信息
    if (table->rowCount() == 0) {
        table->setRowCount(1);
        addPropertyRow("OCC形状类型", type, 0);
    }

    // 调整行高
    for (int i = 0; i < table->rowCount(); ++i) {
        table->setRowHeight(i, 28);
    }
}

void PropertyView::setAisPoint(Handle(AIS_Point) point) {
    int currentRows = table->rowCount();
    table->setRowCount(currentRows + 1);

    Handle(Geom_Point) geomPoint = point->Component();
    gp_Pnt pnt = geomPoint->Pnt();

    addPropertyRow("选取点坐标",
        QString("(%1, %2, %3)")
        .arg(pnt.X(), 0, 'g', 3)
        .arg(pnt.Y(), 0, 'g', 3)
        .arg(pnt.Z(), 0, 'g', 3),
        currentRows);

    table->setRowHeight(currentRows, 28);

}

void PropertyView::setDir(gp_Dir dir)
{
    int currentRows = table->rowCount();
    table->setRowCount(currentRows + 1);

    addPropertyRow("法向量",
        QString("(%1, %2, %3)")
        .arg(dir.X(), 0, 'g', 5)
        .arg(dir.Y(), 0, 'g', 5)
        .arg(dir.Z(), 0, 'g', 5),
        currentRows);

    table->setRowHeight(currentRows, 28);
}
