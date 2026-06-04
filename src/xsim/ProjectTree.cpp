#include "ProjectTree.h"
#include <QtWidgets>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "ComplainUtf8.h"
#include "Msg.h"
#include <QMouseEvent>
#include "mdichild.h"
#include "MyViewer.h"
#include "AngleDialog.h"
#include <qtextstream.h>
#include <QMap>
#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp_Explorer.hxx>

QMap<QTreeWidgetItem*, Handle(AIS_InteractiveObject)>ProjectTree::m_itemToAisObjectMap;

ProjectTree::ProjectTree(QWidget* parent)
	: QTreeWidget(parent)
{
	m_mdiChild = qobject_cast<MdiChild*>(parentWidget()->parentWidget()); // 获取 MdiChild 指针

	treeWidget = new QTreeWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(treeWidget);
	layout->setContentsMargins(0, 0, 0, 0); // 移除布局边距
	this->setLayout(layout);

	myBackMenu = new QMenu(treeWidget); // 在构造函数中初始化右键菜单
	count = 1;	// 初始化计数器
	this->initWidget();


}

ProjectTree::~ProjectTree()
{
	delete myBackMenu;  // 手动释放
	delete treeWidget; // 手动释放
}


void ProjectTree::importModel(const QString& filePath)
{
}

void ProjectTree::clearModels()
{	
	modelMap.clear();// 清空模型数据

	if (modelsItem && modelsItem->childCount() > 0) {
		// 清除Models下的子项
		qDeleteAll(modelsItem->takeChildren());
		delete modelsItem;

	}
	this->initWidget();

}

QString ProjectTree::GetShapeTypeName(TopoDS_Shape shape)
{
	TopAbs_ShapeEnum type = shape.ShapeType();
	 switch (type) {
		case TopAbs_COMPOUND:    return "Compound"; break;
        case TopAbs_COMPSOLID:   return "CompSolid"; break;
        case TopAbs_SOLID:       return "Solid"; break;
        case TopAbs_SHELL:       return "Shell"; break;
        case TopAbs_FACE:        return "Face"; break;
        case TopAbs_WIRE:        return "Wire"; break;
        case TopAbs_EDGE:        return "Edge"; break;
        case TopAbs_VERTEX:      return "Vertex"; break;
        case TopAbs_SHAPE:       return "Shape"; break;
        default:                 return "Unknown"; break;
    }
}

void ProjectTree::initWidget()
{
	//treeWidget->setColumnCount(2);
	//treeWidget->setHeaderLabels(QStringList() << "名称" << "类型"); //<< "模型大小");
	//treeWidget->setColumnWidth(0, 120);// 设置列宽
	//treeWidget->setColumnWidth(1, 100);// 设置列宽
	//treeWidget->setAlternatingRowColors(true);// 设置交替行颜色
	//treeWidget->setAnimated(true);// 设置动画效果
	//treeWidget->setIndentation(12);// 设置缩进

	//treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);// 设置单选模式
	//treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);// 设置选择行为为整行选择
	//treeWidget->setAlternatingRowColors(true);// 设置交替行颜色
	////treeWidget->setEditTriggers(QAbstractItemView::CurrentChanged);// 禁止编辑
	//treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);// 设置右键菜单策略
	//treeWidget->setIndentation(0); // 设置缩进
	////treeWidget->setStyleSheet(
	////	"QHeaderView::section {"
	////	"    font-size: 18px;"          // 文字大小
	////	"    color: #000000;"           // 文字颜色（红色）
	////	"    background-color: #F0F0F0;" // 背景色
	////	"    padding: 2px;"             // 内边距
	////	"    border: 1px solid #C0C0C0;"// 边框
	////	"}"
	////);
	//connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &ProjectTree::onItemDoubleClicked);
	//treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	//connect(treeWidget, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
	//		Popup(pos.x(), pos.y());
	//	});

	//// 添加模型工件项
	//Partitem = new QTreeWidgetItem(treeWidget);
	//Partitem->setText(0, "工件");
	//// 设置字体（加粗、斜体、大小等）
	//QFont font;
	////font.setBold(true);
	//font.setPointSize(12);
	//Partitem->setFont(0, font);  // 第 0 列的字体
	//Partitem->setForeground(0, QBrush(Qt::black));// 设置前景色（文字颜色）
	//Partitem->setBackground(0, QBrush(Qt::white));  // 白色背景
	//treeWidget->addTopLevelItem(Partitem);

	//// 添加切割平面项
	//Planeitem = new QTreeWidgetItem(treeWidget);
	//Planeitem->setText(0, "切面");
	//Planeitem->setFont(0, font);  // 第 0 列的字体
	//Planeitem->setForeground(0, QBrush(Qt::black));// 设置前景色（文字颜色）
	//Planeitem->setBackground(0, QBrush(Qt::white));  // 白色背景
	//Planeitem->setBackground(1, QBrush(Qt::white));  // 白色背景
	//treeWidget->addTopLevelItem(Planeitem);
	setupTreeStructure();
	setupTreeAppearance();

	connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &ProjectTree::onItemDoubleClicked);
	treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(treeWidget, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
		Popup(pos.x(), pos.y());
		});

}

void ProjectTree::setupTreeStructure()
{
	treeWidget->setColumnCount(1);// 设置列数为1
	treeWidget->setHeaderHidden(true);// 隐藏表头
	treeWidget->setIndentation(15);// 设置缩进
	treeWidget->setAnimated(true);// 设置动画效果
	treeWidget->setSortingEnabled(false);// 不启用排序
	treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);// 设置单选模式
	treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);// 设置整行选择
	treeWidget->setAlternatingRowColors(true);// 设置交替行颜色
	treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);// 设置右键菜单策略

	// 创建节点 - 按照Abaqus风格
	modelsItem = new QTreeWidgetItem(treeWidget);
	modelsItem->setText(0, "Models");
	modelsItem->setIcon(0, createColorIcon(Qt::transparent, "📐")); // 黄色图标+建模表情

	partsItem = new QTreeWidgetItem(modelsItem);
	partsItem->setText(0, "Parts");
	partsItem->setIcon(0, createColorIcon(Qt::transparent, "🧩")); // 蓝色图标+组件表情
	{//Parts的子节点
		Partitem = new QTreeWidgetItem(partsItem);
		Partitem->setText(0, "Part");
		Partitem->setIcon(0, createColorIcon(Qt::transparent, "🔩")); // 蓝色图标+零件表情

		Planeitem = new QTreeWidgetItem(partsItem);
		Planeitem->setText(0, "Plane");
		Planeitem->setIcon(0, createColorIcon(Qt::transparent, "📏")); // 蓝色图标+测量表情
	}

	materialsItem = new QTreeWidgetItem(modelsItem);
	materialsItem->setText(0, "Materials");
	materialsItem->setIcon(0, createColorIcon(Qt::transparent, "🧪")); // 紫色图标+材料测试表情

	sectionsItem = new QTreeWidgetItem(modelsItem);
	sectionsItem->setText(0, "Sections");
	sectionsItem->setIcon(0, createColorIcon(Qt::transparent, "✂️")); // 红色图标+截面切割表情

	assemblyItem = new QTreeWidgetItem(modelsItem);
	assemblyItem->setText(0, "Assembly");
	assemblyItem->setIcon(0, createColorIcon(Qt::transparent, "🔧")); // 灰色图标+装配工具表情

	stepsItem = new QTreeWidgetItem(modelsItem);
	stepsItem->setText(0, "Steps (0)");
	stepsItem->setIcon(0, createColorIcon(Qt::transparent, "📝")); // 橙色图标+步骤记录表情
	{// Steps的子节点
		fieldOutputItem = new QTreeWidgetItem(stepsItem);
		fieldOutputItem->setText(0, "Field Output Requests");
		fieldOutputItem->setIcon(0, createColorIcon(Qt::transparent, "📊")); // 青绿色图标+数据图表表情

		historyOutputItem = new QTreeWidgetItem(stepsItem);
		historyOutputItem->setText(0, "History Output Requests");
		historyOutputItem->setIcon(0, createColorIcon(Qt::transparent, "📈")); // 青绿色图标+趋势图表情

		timePointsItem = new QTreeWidgetItem(stepsItem);
		timePointsItem->setText(0, "Time Points");
		timePointsItem->setIcon(0, createColorIcon(Qt::transparent, "⏱️")); // 紫色图标+时间点表情
	}
	// 其他顶级节点
	aleAdaptiveMeshItem = new QTreeWidgetItem(modelsItem);
	aleAdaptiveMeshItem->setText(0, "Mesh");
	aleAdaptiveMeshItem->setIcon(0, createColorIcon(Qt::transparent, "🔲")); // 粉色图标+网格表情

	interactionsItem = new QTreeWidgetItem(modelsItem);
	interactionsItem->setText(0, "Interactions");
	interactionsItem->setIcon(0, createColorIcon(Qt::transparent, "🤝")); // 青色图标+交互表情

	interactionPropertiesItem = new QTreeWidgetItem(modelsItem);
	interactionPropertiesItem->setText(0, "Interaction Properties");
	interactionPropertiesItem->setIcon(0, createColorIcon(Qt::transparent, "📊")); // 灰色图标+属性图表表情

	contactControlsItem = new QTreeWidgetItem(modelsItem);
	contactControlsItem->setText(0, "Contact Controls");
	contactControlsItem->setIcon(0, createColorIcon(Qt::transparent, "🎛️")); // 黄色图标+控制调节表情

	contactInitializationItem = new QTreeWidgetItem(modelsItem);
	contactInitializationItem->setText(0, "Contact Initialization");
	contactInitializationItem->setIcon(0, createColorIcon(Qt::transparent, "🔄")); // 黄色图标+初始化重置表情

	contactStabilizationItem = new QTreeWidgetItem(modelsItem);
	contactStabilizationItem->setText(0, "Contact Stabilization");
	contactStabilizationItem->setIcon(0, createColorIcon(Qt::transparent, "⚖️")); // 黄色图标+平衡稳定表情

	constraintsItem = new QTreeWidgetItem(modelsItem);
	constraintsItem->setText(0, "Constraints");
	constraintsItem->setIcon(0, createColorIcon(Qt::transparent, "🔒")); // 绿色图标+约束锁定表情

	connectorSectionsItem = new QTreeWidgetItem(modelsItem);
	connectorSectionsItem->setText(0, "Connector Sections");
	connectorSectionsItem->setIcon(0, createColorIcon(Qt::transparent, "🔗")); // 紫色图标+连接表情

	fieldsItem = new QTreeWidgetItem(modelsItem);
	fieldsItem->setText(0, "Fields");
	fieldsItem->setIcon(0, createColorIcon(Qt::transparent, "🌐")); // 红色图标+场域表情

	amplitudesItem = new QTreeWidgetItem(modelsItem);
	amplitudesItem->setText(0, "Amplitudes");
	amplitudesItem->setIcon(0, createColorIcon(Qt::transparent, "📈")); // 橙色图标+振幅趋势表情

	loadsItem = new QTreeWidgetItem(modelsItem);
	loadsItem->setText(0, "Loads");
	loadsItem->setIcon(0, createColorIcon(Qt::transparent, "🏋️")); // 青绿色图标+载荷负重表情

	bcsItem = new QTreeWidgetItem(modelsItem);
	bcsItem->setText(0, "BCs");
	bcsItem->setIcon(0, createColorIcon(Qt::transparent, "📏")); // 紫色图标+边界限定表情

	// 设置默认展开状态
	modelsItem->setExpanded(true);
	partsItem->setExpanded(true);
	stepsItem->setExpanded(false);

	// 添加所有顶级节点到树
	treeWidget->addTopLevelItem(modelsItem);

}

void ProjectTree::setupTreeAppearance()
{
	// 设置样式表
	treeWidget->setStyleSheet(R"(
        QTreeWidget {
            background: white;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            outline: none;
            font-size: 15px;
            font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
        }
        
        QTreeWidget::item {
            padding: 6px 4px;
            border-bottom: 1px solid transparent;
            color: #495057;
        }
        
        QTreeWidget::item:selected {
            background: #e7f1ff;
            color: #0d6efd;
            border-radius: 4px;
        }
        
        QTreeWidget::item:hover {
            background: #f8f9fa;
        }
        
        QHeaderView::section {
            background: #f8f9fa;
            padding: 8px;
            border: none;
            border-bottom: 1px solid #dee2e6;
            font-weight: bold;
            color: #495057;
            font-size: 12px;
        }
    )");

	// 设置图标大小
	treeWidget->setIconSize(QSize(16, 16));

	// 设置字体
	QFont font = treeWidget->font();
	font.setPointSize(15);
	treeWidget->setFont(font);

	// 设置类别节点的特殊样式
	for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
		QTreeWidgetItem* item = treeWidget->topLevelItem(i);
		QFont itemFont = item->font(0);
		itemFont.setBold(true);
		item->setFont(0, itemFont);
	}
}

QIcon ProjectTree::createColorIcon(const QColor& color, const QString& text) const
{
	QPixmap pixmap(16, 16);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制圆形背景
	painter.setBrush(color);
	painter.setPen(QPen(Qt::gray, 1));
	painter.drawEllipse(2, 2, 12, 12);

	// 如果有文本，绘制文本
	if (!text.isEmpty()) {
		painter.setPen(Qt::white);
		QFont font = painter.font();
		font.setPointSize(8);
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
	}

	return QIcon(pixmap);
}


void ProjectTree::onItemDoubleClicked(QTreeWidgetItem* item, int column) {

	if (item && item->parent() == bcsItem) {
		// 双击的是边界条件项
		QString bcName = item->text(0);
		showBoundaryConditionDetails(bcName);
		return;
	}

	// 检查是否是离散点item
	if (item && item->parent() == Pointitem) {
		// 双击的是离散点item
		if (m_pointItemToPointsMap.contains(item)) {
			std::vector<std::array<double, 12>> points = m_pointItemToPointsMap[item];
			//std::vector<std::array<double, 3>> points = m_pointItemToPointsMap[item];
			
			// 在OcctView中选中对应的点（通过坐标匹配）
			if (mdiChild && mdiChild->q3dView) {
				Handle(AIS_InteractiveContext) context = mdiChild->q3dView->getContext();
				if (!context.IsNull()) {
					// 清除之前的选择
					context->ClearSelected(Standard_False);
					
					// 通过坐标匹配找到对应的点并选中
					NCollection_List<Handle(AIS_InteractiveObject)> aisList = mdiChild->h_MyViewer->GetAisObj();
					for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator it(aisList); it.More(); it.Next()) {
						Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(it.Value());
						if (!aisShape.IsNull() && aisShape->Shape().ShapeType() == TopAbs_VERTEX) {
							TopoDS_Vertex vertex = TopoDS::Vertex(aisShape->Shape());
							gp_Pnt pnt = BRep_Tool::Pnt(vertex);
							
							// 检查是否匹配任何一个离散点坐标（容差1e-6）
							for (const auto& point : points) {
								if (abs(pnt.X() - point[0]) < 1e-6 && 
									abs(pnt.Y() - point[1]) < 1e-6 && 
									abs(pnt.Z() - point[2]) < 1e-6) {
									context->AddOrRemoveSelected(aisShape, Standard_False);
									break;
								}
							}
						}
					}
					context->UpdateCurrentViewer();
				}
			}
			
			// 弹出对话框显示离散点信息
			QDialog* pointInfoDialog = new QDialog(this);
			pointInfoDialog->setWindowTitle("离散点信息");
			pointInfoDialog->resize(800, 800);
			
			QVBoxLayout* mainLayout = new QVBoxLayout(pointInfoDialog);
			
			QLabel* infoLabel = new QLabel(QString("离散点数量：%1").arg(points.size()), pointInfoDialog);
			mainLayout->addWidget(infoLabel);
			
			QTableWidget* pointTable = new QTableWidget(pointInfoDialog);
			pointTable->setColumnCount(17);  // 增加一列用于复选框
			pointTable->setHorizontalHeaderLabels(QStringList() << "序号" << "X坐标" << "Y坐标" << "Z坐标" << "刃倾角" << "前角" << "后角" << "切削速度" << "切削厚度" << "前刀面ID" << "前刀面法矢X" << "前刀面法矢Y" << "前刀面法矢Z" << "2D or 3D"  << " 圆角半径 (mm)" << "摩擦系数" << "选择");
			pointTable->setRowCount(points.size());
			static int index_checkbox = 16;
			for (size_t i = 0; i < points.size(); ++i) {
				pointTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
				pointTable->setItem(i, 1, new QTableWidgetItem(QString::number(points[i][0], 'f', 6)));
				pointTable->setItem(i, 2, new QTableWidgetItem(QString::number(points[i][1], 'f', 6)));
				pointTable->setItem(i, 3, new QTableWidgetItem(QString::number(points[i][2], 'f', 6)));
				pointTable->setItem(i, 4, new QTableWidgetItem(QString::number(points[i][3], 'f', 6)));
				pointTable->setItem(i, 5, new QTableWidgetItem(QString::number(points[i][4], 'f', 6)));
				pointTable->setItem(i, 6, new QTableWidgetItem(QString::number(points[i][5], 'f', 6)));
				pointTable->setItem(i, 7, new QTableWidgetItem(QString::number(points[i][6], 'f', 6)));
				pointTable->setItem(i, 8, new QTableWidgetItem(QString::number(points[i][7], 'f', 6)));
				pointTable->setItem(i, 9, new QTableWidgetItem(QString::number(points[i][8], 'i', 0)));
				pointTable->setItem(i, 10, new QTableWidgetItem(QString::number(points[i][9], 'f', 6)));
				pointTable->setItem(i, 11, new QTableWidgetItem(QString::number(points[i][10], 'f', 6)));
				pointTable->setItem(i, 12, new QTableWidgetItem(QString::number(points[i][11], 'f', 6)));
				pointTable->setItem(i, 13, new QTableWidgetItem(QString::number(0, 'i', 0)));
				pointTable->setItem(i, 14, new QTableWidgetItem(QString::number(0.005, 'f', 6)));
				pointTable->setItem(i, 15, new QTableWidgetItem(QString::number(0.5, 'f', 6)));

				// 添加复选框（第index_checkbox列，索引为index_checkbox）
				QTableWidgetItem* checkItem = new QTableWidgetItem();
				checkItem->setCheckState(Qt::Checked);  // 默认选中
				checkItem->setFlags(checkItem->flags() | Qt::ItemIsUserCheckable);
				checkItem->setTextAlignment(Qt::AlignCenter);
				pointTable->setItem(i, index_checkbox, checkItem);
			}
			
			pointTable->resizeColumnsToContents();
			mainLayout->addWidget(pointTable);
			
			// 添加全选/全不选按钮
			QHBoxLayout* buttonLayout = new QHBoxLayout();
			QPushButton* btnSelectAll = new QPushButton("全选", pointInfoDialog);
			QPushButton* btnDeselectAll = new QPushButton("全不选", pointInfoDialog);
			
			connect(btnSelectAll, &QPushButton::clicked, [pointTable]() {
				for (int i = 0; i < pointTable->rowCount(); ++i) {
					QTableWidgetItem* item = pointTable->item(i, index_checkbox);
					if (item) {
						item->setCheckState(Qt::Checked);
					}
				}
			});
			
			connect(btnDeselectAll, &QPushButton::clicked, [pointTable]() {
				for (int i = 0; i < pointTable->rowCount(); ++i) {
					QTableWidgetItem* item = pointTable->item(i, index_checkbox);
					if (item) {
						item->setCheckState(Qt::Unchecked);
					}
				}
			});
			
			buttonLayout->addWidget(btnSelectAll);
			buttonLayout->addWidget(btnDeselectAll);
			buttonLayout->addStretch();
			mainLayout->addLayout(buttonLayout);
			
			// 添加输出离散点按钮
			// 收集选中的点
			
			QPushButton* btnExportPoints = new QPushButton("输出离散点", pointInfoDialog);
			connect(btnExportPoints, &QPushButton::clicked, [this, points, pointTable, pointInfoDialog]() {
				selectedPoints.clear();
				for (int i = 0; i < pointTable->rowCount(); ++i) {
					QTableWidgetItem* checkItem = pointTable->item(i, index_checkbox);
					if (checkItem && checkItem->checkState() == Qt::Checked) {
						std::array<double, 15> temparray;
						for(int j=1;j< index_checkbox;j++) temparray[j-1] = pointTable->item(i, j)->text().toDouble();
						selectedPoints.push_back(temparray);
					}
				}
				
				if (selectedPoints.empty()) {
					Msg::ShowInfo("请至少选择一个离散点进行输出。");
					return;
				}
				
				// 保存选中的离散点到txt文件
				QString fileName = QFileDialog::getSaveFileName(pointInfoDialog, tr("保存点坐标"), "", tr("txt文件 (*.txt);;All file(*.*)"));
				if (fileName.isEmpty()) return;
				if (!fileName.endsWith(".txt", Qt::CaseInsensitive)) {
					fileName += ".txt";
				}
				QFile file(fileName);
				if (!file.open(QFile::WriteOnly)) {
					Msg::ShowError("无法打开文件进行写入，请检查文件路径或权限。");
					return;
				}
				QTextStream outFile(&file);
				
				try {
					for (const auto& point : selectedPoints) {
						outFile << point[0] << "\t" << point[1] << "\t" << point[2] << "\t" << point[3] << "\t" << point[4] << "\t" << point[5] << "\t" << point[6] << "\t"  << point[7] << "\t" << point[8] << "\t" << point[9] << "\t" << point[10] << "\t" << point[11] << "\n";
					}
					// 自动刷新并关闭文件
					outFile.flush();
					file.close();
					//Msg::ShowInfo(QString("离散点输出成功，共输出 %1 个点").arg(selectedPoints.size()));
				}
				catch (const std::exception& e) {
					Msg::ShowError("保存点坐标失败，请检查文件路径或权限。");
					return;
				}
			});
			mainLayout->addWidget(btnExportPoints);
			
			QPushButton* btnExportAbaqus = new QPushButton("输出abaqus", pointInfoDialog);
			connect(btnExportAbaqus, &QPushButton::clicked, [this, points, pointTable, pointInfoDialog]() {
				if (mdiChild)
				{
					selectedPoints.clear();
					for (int i = 0; i < pointTable->rowCount(); ++i) {
						QTableWidgetItem* checkItem = pointTable->item(i, index_checkbox);
						if (checkItem && checkItem->checkState() == Qt::Checked) {
							std::array<double, 15> temparray;
							for (int j = 1; j < index_checkbox; j++) temparray[j-1] = pointTable->item(i, j)->text().toDouble();
							selectedPoints.push_back(temparray);
						}
					}

					if (selectedPoints.empty()) {
						Msg::ShowInfo("请至少选择一个离散点进行输出。");
						return;
					}
					for (int i = 0; i < selectedPoints.size(); i++) {
						m_mdiChild->generateAbaqusINP(selectedPoints[i][12], selectedPoints[i][6], selectedPoints[i][7], selectedPoints[i][3], selectedPoints[i][4], selectedPoints[i][5], selectedPoints[i][13], selectedPoints[i][14]);
					}
				}else Msg::ShowError("没有子窗口");
			});
			mainLayout->addWidget(btnExportAbaqus);
			QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, pointInfoDialog);
			connect(buttonBox, &QDialogButtonBox::accepted, pointInfoDialog, &QDialog::accept);
			mainLayout->addWidget(buttonBox);
			
			pointInfoDialog->show();
			pointInfoDialog->exec();
			pointInfoDialog->deleteLater();
			
		}
		return;
	}

	emit modelDoubleClicked(item,column);  //发送模型树双击信号

};

// 添加模型到树中
void ProjectTree::addModelToTree(const ModelData& model)
{
	if (model.type == "工件") {
		addPartToTree(model);
	}
	else if (model.type == "切割平面") {
		addPlaneToTree(model);
	}

}

//添加工件
void ProjectTree::addPartToTree(const ModelData& model)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(Partitem);
	item->setText(0, model.name);
	item->setToolTip(0, "工件模型");
	//item->setText(1, GetShapeTypeName(model.shape)); // 存储模型数据
	item->setIcon(0, createColorIcon(QColor(13, 110, 253), "P")); // 蓝色零件图标
	// 更新模型计数
	int modelCount = modelsItem->childCount();

	modelMap[model.name] = model;
	m_itemToAisObjectMap[item] = model.shape;	

}

//添加切割平面
void ProjectTree::addPlaneToTree(const ModelData& model)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(Planeitem);
	item->setText(0, model.name);
	item->setIcon(0, createColorIcon(QColor(220, 53, 69), "C")); // 红色切割平面图标
	item->setToolTip(0, "切割平面");
	QString tt = Planeitem->child(0)->text(0);
	modelMap[model.name] = model;
	m_itemToAisObjectMap[item] = model.shape;

}

void ProjectTree::addPointToTree(std::vector<std::array<double, 12>> vec)
{
	if (Pointitem == nullptr)
	{
		Pointitem = new QTreeWidgetItem(partsItem);
		Pointitem->setText(0, "Point");
		Pointitem->setIcon(0, createColorIcon(QColor(253, 126, 20), "P"));
		partsItem->addChild(Pointitem);

	}
	QTreeWidgetItem* item = new QTreeWidgetItem(Pointitem);
	int num = vec.size();
	item->setText(0, "切削刃 " + QString::number(m_pointItemToPointsMap.size()+1) + "(离散点数量：" + QString::number(num)+ ")"); // 存储模型数据
	Pointitem->addChild(item);
	
	// 存储离散点item到点坐标的映射
	m_pointItemToPointsMap[item] = vec;
	
	++count;
}

void ProjectTree::addMeshToTree(const ModelData& model)
{
	//if (Meshitem == nullptr) {
	//	Meshitem = new QTreeWidgetItem(modelsItem);
	//	Meshitem->setText(0, "Mesh");
	//	Meshitem->setIcon(0, createColorIcon(QColor(255, 85, 255), "M"));
	//	modelsItem->addChild(Meshitem);
	//}
	QTreeWidgetItem* item = new QTreeWidgetItem(aleAdaptiveMeshItem);
	item->setText(0, model.name);
	item->setIcon(0, createColorIcon(QColor(85, 255, 255), "M")); 
	item->setToolTip(0, "网格");

	modelMap[model.name] = model;
	m_itemToAisObjectMap[item] = model.shape;
}

void ProjectTree::addBoundaryConditionToTree(const BoundaryConditionData& bcData)
{
	// 确保BCs节点存在
	if (!bcsItem) {
		bcsItem = new QTreeWidgetItem(treeWidget);
		bcsItem->setText(0, "BCs");
		bcsItem->setIcon(0, createColorIcon(QColor(102, 16, 242), "BC")); // 紫色图标
		treeWidget->addTopLevelItem(bcsItem);
	}

	// 添加边界条件项
	QTreeWidgetItem* bcItem = new QTreeWidgetItem(bcsItem);
	bcItem->setText(0, bcData.name);
	bcItem->setIcon(0, createColorIcon(QColor(147, 51, 234), "BC"));
	bcItem->setToolTip(0, QString("类型: %1\n实体数量: %2")
		.arg(bcData.type)
		.arg(0));

	// 存储边界条件数据
	this->m_boundaryConditionsmap[bcData.name] = bcData;

	// 更新BCs节点文本显示数量
	updateBCsCount();
}

void ProjectTree::updateBCsCount()
{
	if (bcsItem) {
		int bcscount = bcsItem->childCount();
		bcsItem->setText(0, QString("BCs (%1)").arg(bcscount));
	}
}

//@brief 显示边界条件详细信息
void ProjectTree::showBoundaryConditionDetails(const QString& bcName)
{
	if (!this->m_boundaryConditionsmap.contains(bcName)) {
		return;
	}
	MdiChild* mdichild = qobject_cast<MdiChild*>(parentWidget()->parentWidget());
	BoundaryConditionData bcData;
	for (const auto& bc : mdichild->q3dView->getBoundaryConditions()) {
		if (bc.name == bcName) {
			bcData = bc;
		}
		else Msg::ShowError("未找到对应的边界条件数据。");
	}

	// 创建详细信息对话框
	QDialog* detailsDialog = new QDialog(this);
	detailsDialog->setWindowTitle(QString("边界条件详情 - %1").arg(bcName));
	detailsDialog->resize(600, 400);

	QVBoxLayout* mainLayout = new QVBoxLayout(detailsDialog);

	// 创建选项卡
	QTabWidget* tabWidget = new QTabWidget(detailsDialog);

	// 选项卡1: 几何形状信息
	QWidget* geometryTab = new QWidget();
	QVBoxLayout* geometryLayout = new QVBoxLayout(geometryTab);

	QLabel* geometryLabel = new QLabel("选中的几何形状:", geometryTab);
	geometryLayout->addWidget(geometryLabel);

	QTableWidget* geometryTable = new QTableWidget(geometryTab);
	geometryTable->setColumnCount(2);
	geometryTable->setColumnWidth(0, 150);
	geometryTable->horizontalHeader()->setStretchLastSection(true);

	geometryTable->setHorizontalHeaderLabels(QStringList() << "类型" << "数量");
	geometryLayout->addWidget(geometryTable);

	// 填充几何形状数据
	populateGeometryTable(geometryTable, bcData);

	// 选项卡2: 网格节点信息
	QWidget* meshTab = new QWidget();
	QVBoxLayout* meshLayout = new QVBoxLayout(meshTab);

	QLabel* meshLabel = new QLabel("关联的网格节点:", meshTab);
	meshLayout->addWidget(meshLabel);

	QTableWidget* meshTable = new QTableWidget(meshTab);
	meshTable->setColumnCount(4);
	meshTable->setHorizontalHeaderLabels(QStringList() << "节点ID" << "X坐标" << "Y坐标" << "Z坐标");
	meshLayout->addWidget(meshTable);

	// 填充网格节点数据
	populateMeshTable(meshTable, bcData);

	tabWidget->addTab(geometryTab, "几何形状");
	tabWidget->addTab(meshTab, "网格节点");

	mainLayout->addWidget(tabWidget);

	// 添加按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, detailsDialog);
	connect(buttonBox, &QDialogButtonBox::accepted, detailsDialog, &QDialog::accept);
	mainLayout->addWidget(buttonBox);

	detailsDialog->show();
	detailsDialog->exec();
	detailsDialog->deleteLater();
}

void ProjectTree::savePoint()
{
	QString fileName = QFileDialog::getSaveFileName(nullptr, tr("保存点坐标"), "", tr("txt文件 (*.txt)"
		";;All file(*.*)"));
	if (pointsVec.empty()){
		Msg::ShowInfo("没有离散点可保存，请先进行离散操作。");
	}    
	if (fileName.isEmpty()) return;
	if (!fileName.endsWith(".txt", Qt::CaseInsensitive)) {
		fileName += ".txt";
	}
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly)) {
		Msg::ShowError("无法打开文件进行写入，请检查文件路径或权限。");
		return;
	}
	QTextStream outFile(&file);

	try {
		for (const auto& point : pointsVec) {
			Standard_Character Buffer[1024] = { 0 };
			Sprintf(Buffer, "离散点坐标为: X=%f, Y=%f, Z=%f", point[0], point[1], point[2]);
			Msg::ShowInfo(Buffer);
			outFile << point[0] << " " << point[1] << " " << point[2] << "\n";
		}
		// 自动刷新并关闭文件
		outFile.flush();
		file.close();
		Msg::ShowInfo("离散点输出成功");
	}
	catch (const std::exception& e) {
		Msg::ShowError("保存点坐标失败，请检查文件路径或权限。");
		return;
	}

}

void ProjectTree::saveAngle()
{
	QString fileName = QFileDialog::getSaveFileName(nullptr, tr("保存点坐标"), "", tr("txt文件 (*.txt)"
		";;All file(*.*)"));
	if (anglesVec.empty()) {
		Msg::ShowInfo("没有离散点可保存，请先进行离散操作。");
	}
	if (fileName.isEmpty()) return;
	if (!fileName.endsWith(".txt", Qt::CaseInsensitive)) {
		fileName += ".txt";
	}
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly)) {
		Msg::ShowError("无法打开文件进行写入，请检查文件路径或权限。");
		return;
	}
	QTextStream outFile(&file);

	try {
		for (const auto& point : anglesVec) {
			Standard_Character Buffer[1024] = { 0 };
			Sprintf(Buffer, "前后角点坐标为: X=%f, Y=%f, Z=%f，前角=%f,后角=%f", point[0], point[1], point[2], point[3], point[4]);
			Msg::ShowInfo(Buffer);
			outFile << point[0] << " " << point[1] << " " << point[2] << " " << point[3] << " " << point[4] << " " << point[5] << " " << point[6] << "\n";
		}
		// 自动刷新并关闭文件
		outFile.flush();
		file.close();
		Msg::ShowInfo("前后角输出成功");
	}
	catch (const std::exception& e) {
		Msg::ShowError("保存点坐标失败，请检查文件路径或权限。");
		return;
	}

}

void ProjectTree::deleteSelect(const QString& modelName)
{
	QTreeWidgetItem* currentItem = treeWidget->currentItem();
	if (currentItem) {
		QString shapename = currentItem->text(0);
		ModelData value = modelMap.value(shapename);
		MdiChild* mdiChild = new MdiChild(this);
		mdiChild->EraseModel(value.shape); // 从视图中删除形状
		delete currentItem;
		Msg::ShowInfo("删除选择的模型");
	}
	else {
		Msg::ShowInfo("没有选择的模型");
	}
}

// 弹出右键菜单
void ProjectTree::Popup(const int x, const int y)
{

	if (!treeWidget) {
		Msg::ShowInfo("treeWidget 无效");
		return;
	}
	if (!myBackMenu) {
		Msg::ShowInfo("myBackMenu 无效");
		return;
	}
	Msg::ShowInfo("鼠标右键点击");
	myBackMenu = new QMenu(treeWidget);

	// 添加清空功能
	//QAction* deleteAction = new QAction("清空", this);
	//deleteAction->setStatusTip("清空所有内容");
	//connect(deleteAction, &QAction::triggered, mdiChild, &MdiChild::clearModel);
	//myBackMenu->addAction(deleteAction);

	// 当前选中的树项
	QTreeWidgetItem* currentItem = treeWidget->currentItem();

	if (currentItem && m_itemToAisObjectMap.contains(currentItem)) {
		// 添加显示/隐藏菜单项
		QAction* showAction = new QAction("显示", this);
		QAction* hideAction = new QAction("隐藏", this);
		QAction* toggleAction = new QAction("切换显示/隐藏", this);
		QAction* deleteAction = new QAction("删除模型", this); // 新增删除功能

		connect(showAction, &QAction::triggered, this, &ProjectTree::showModel);
		connect(hideAction, &QAction::triggered, this, &ProjectTree::hideModel);
		connect(toggleAction, &QAction::triggered, this, &ProjectTree::toggleModelVisibility);
		connect(deleteAction, &QAction::triggered, this, &ProjectTree::deleteSelectedModel); // 连接删除功能

		myBackMenu->addAction(showAction);
		myBackMenu->addAction(hideAction);
		myBackMenu->addAction(toggleAction);
		myBackMenu->addSeparator();
		myBackMenu->addAction(deleteAction); // 添加到菜单
		myBackMenu->addSeparator();
	}

	// 如果是离散点item，添加“删除离散点”功能
	if (currentItem && currentItem->parent() == Pointitem) {
		QAction* deletePointsAction = new QAction("删除离散点", this);
		deletePointsAction->setStatusTip("删除该项对应的离散点");
		connect(deletePointsAction, &QAction::triggered, this, [this, currentItem]() {
			if (!currentItem) {
				Msg::ShowInfo("没有选择的离散点项");
				return;
			}

			// 找到该item对应的离散点
			if (!m_pointItemToPointsMap.contains(currentItem)) {
				Msg::ShowInfo("未找到该项对应的离散点数据");
				return;
			}

			std::vector<std::array<double, 12>> points = m_pointItemToPointsMap[currentItem];

			// 1. 从视图中删除对应的顶点
			if (mdiChild && mdiChild->h_MyViewer) {
				Handle(AIS_InteractiveContext) context = mdiChild->h_MyViewer->getAisContext();
				if (!context.IsNull()) {
					NCollection_List<Handle(AIS_InteractiveObject)> aisList = mdiChild->h_MyViewer->GetAisObj();
					for (NCollection_List<Handle(AIS_InteractiveObject)>::Iterator it(aisList); it.More(); it.Next()) {
						Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(it.Value());
						if (!aisShape.IsNull() && aisShape->Shape().ShapeType() == TopAbs_VERTEX) {
							TopoDS_Vertex vertex = TopoDS::Vertex(aisShape->Shape());
							gp_Pnt pnt = BRep_Tool::Pnt(vertex);
							// 检查是否匹配要删除的离散点
							bool matched = false;
							for (const auto& point : points) {
								if (std::abs(pnt.X() - point[0]) < 1e-6 &&
									std::abs(pnt.Y() - point[1]) < 1e-6 &&
									std::abs(pnt.Z() - point[2]) < 1e-6) {
									matched = true;
									break;
								}
							}
							if (matched) {
								mdiChild->h_MyViewer->Erase(aisShape);
							}
						}
					}
				}
			}

			// 2. 从 ProjectTree::pointsVec 中移除对应点
			if (!pointsVec.empty()) {
				std::vector<std::array<double, 3>> filtered;
				filtered.reserve(pointsVec.size());
				for (const auto& p : pointsVec) {
					bool matched = false;
					for (const auto& target : points) {
						if (std::abs(p[0] - target[0]) < 1e-6 &&
							std::abs(p[1] - target[1]) < 1e-6 &&
							std::abs(p[2] - target[2]) < 1e-6) {
							matched = true;
							break;
						}
					}
					if (!matched) {
						filtered.push_back(p);
					}
				}
				pointsVec.swap(filtered);
			}

			// 3. 从 AngleDialog::pointsAndVec 中移除对应点（根据前三列坐标匹配）
			if (!AngleDialog::pointsAndVec.empty()) {
				std::vector<std::vector<double>> filteredPV;
				filteredPV.reserve(AngleDialog::pointsAndVec.size());
				for (const auto& data : AngleDialog::pointsAndVec) {
					if (data.size() < 3) {
						filteredPV.push_back(data);
						continue;
					}
					bool matched = false;
					for (const auto& target : points) {
						if (std::abs(data[0] - target[0]) < 1e-6 &&
							std::abs(data[1] - target[1]) < 1e-6 &&
							std::abs(data[2] - target[2]) < 1e-6) {
							matched = true;
							break;
						}
					}
					if (!matched) {
						filteredPV.push_back(data);
					}
				}
				AngleDialog::pointsAndVec.swap(filteredPV);
			}

			// 4. 从映射中移除该item
			m_pointItemToPointsMap.remove(currentItem);

			// 5. 从树中删除该item
			delete currentItem;

			Msg::ShowInfo("离散点已删除");
			});
		myBackMenu->addAction(deletePointsAction);
	}

	// 添加删除选择功能（针对模型）
	QAction* deleteSelectedAction = new QAction("删除选择", this);
	deleteSelectedAction->setStatusTip("删除选择的模型");
	connect(deleteSelectedAction, &QAction::triggered, this, [this]() {
		QTreeWidgetItem* currentItem = treeWidget->currentItem();
		if (currentItem && currentItem->parent() != Pointitem) {
			// 仅对非离散点项执行模型删除逻辑
			QString shapename = currentItem->text(0);
			ModelData value = modelMap.value(shapename);
			MdiChild* localMdiChild = new MdiChild(this);
			localMdiChild->EraseModel(value.shape); // 从视图中删除形状
			delete currentItem;
			Msg::ShowInfo("删除选择的模型");
		}
		else if (!currentItem) {
			Msg::ShowInfo("没有选择的模型");
		}
		});
	myBackMenu->addAction(deleteSelectedAction);

	// 添加重命名功能
	QAction* renameAction = new QAction("重命名", this);
	renameAction->setStatusTip("重命名选择的模型");
	connect(renameAction, &QAction::triggered, this, [this]() {
		QTreeWidgetItem* currentItem = treeWidget->currentItem();
		if (currentItem) {
			bool ok;
			QString text = QInputDialog::getText(this, tr("重命名"),
				tr("请输入新名称:"), QLineEdit::Normal,
				currentItem->text(0), &ok);
			if (ok && !text.isEmpty()) {
				currentItem->setText(0, text);
				Msg::ShowInfo("重命名成功");
			}
		}
		else {
			Msg::ShowInfo("没有选择的模型");
		}});
	myBackMenu->addSeparator(); // 添加分隔线
	myBackMenu->addAction(renameAction);
	if (treeWidget && myBackMenu) {
		myBackMenu->exec(treeWidget->mapToGlobal(QPoint(x, y)));
	}

}

void ProjectTree::mousePressEvent(QMouseEvent*)
{
	Msg::ShowInfo("鼠标按下事件");
	emit onItemDoubleClicked(treeWidget->currentItem(), 0);
}

//void ProjectTree::onRButtonUp(Qt::MouseButtons nFlags, Qt::KeyboardModifiers nKeys, const QPoint point)
//{
//	Msg::ShowInfo("鼠标右键按下事件");
//	// 鼠标右键点击事件
//		// 获取鼠标点击的坐标
//	int x = point.x();
//	int y = point.y();
//
//	// 弹出右键菜单
//	Popup(x, y);
//}
//	

void ProjectTree::showModel()
{
	QTreeWidgetItem* currentItem = treeWidget->currentItem();
	if (!currentItem || !m_mdiChild) return;

	if (m_itemToAisObjectMap.contains(currentItem)) {
		Handle(AIS_InteractiveObject) aisObject = m_itemToAisObjectMap[currentItem];
		Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(aisObject);

		if (!aisShape.IsNull()) {
			Handle_AIS_InteractiveContext context = m_mdiChild->h_MyViewer->getAisContext();
			context->Display(aisShape, Standard_False);
			context->UpdateCurrentViewer();

			currentItem->setIcon(0, createColorIcon(QColor(13, 110, 253), "S"));
			currentItem->setToolTip(0, "");
		}
	}
}

void ProjectTree::hideModel()
{
	QTreeWidgetItem* currentItem = treeWidget->currentItem();
	if (!currentItem || !m_mdiChild) return;

	if (m_itemToAisObjectMap.contains(currentItem)) {
		Handle(AIS_InteractiveObject) aisObject = m_itemToAisObjectMap[currentItem];
		Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(aisObject);

		if (!aisShape.IsNull()) {
			Handle(AIS_InteractiveContext) context = m_mdiChild->h_MyViewer->getAisContext();
			context->Erase(aisShape, Standard_False);
			context->UpdateCurrentViewer();

			currentItem->setIcon(0, createColorIcon(QColor(128, 128, 128), "H"));
			currentItem->setToolTip(0, "已隐藏");
		}
	}
}

void ProjectTree::setTransparency()
{

}

void ProjectTree::toggleModelVisibility()
{
	QTreeWidgetItem* currentItem = treeWidget->currentItem();
	if (!currentItem || !m_mdiChild) return;

	if (m_itemToAisObjectMap.contains(currentItem)) {
		Handle(AIS_InteractiveObject) aisObject = m_itemToAisObjectMap[currentItem];
		Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(aisObject);

		if (!aisShape.IsNull()) {
			// 获取交互上下文
			Handle_AIS_InteractiveContext context = m_mdiChild->h_MyViewer->getAisContext();

			if (context->IsDisplayed(aisShape)) {
				// 如果当前显示，则隐藏
				context->Erase(aisShape, Standard_False);
				currentItem->setIcon(0, createColorIcon(QColor(128, 128, 128), "H")); // 灰色图标表示隐藏
				currentItem->setToolTip(0, "已隐藏");
			}
			else {
				// 如果当前隐藏，则显示
				context->Display(aisShape, Standard_False);
				currentItem->setIcon(0, createColorIcon(QColor(13, 110, 253), "S")); // 恢复原图标
				currentItem->setToolTip(0, "");
			}
			context->UpdateCurrentViewer();
		}
	}
}

void ProjectTree::deleteSelectedModel()
{
	QTreeWidgetItem* currentItem = treeWidget->currentItem();
	if (!currentItem || !m_mdiChild) {
		Msg::ShowInfo("无法删除：没有选中的模型或MdiChild未初始化");
		return;
	}

	// 确认对话框
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "确认删除",
		"确定要删除选中的模型吗？",
		QMessageBox::Yes | QMessageBox::No);

	if (reply != QMessageBox::Yes) {
		return;
	}

	QString modelName = currentItem->text(0);

	// 从3D视图中删除
	if (m_itemToAisObjectMap.contains(currentItem)) {
		Handle(AIS_InteractiveObject) aisObject = m_itemToAisObjectMap[currentItem];
		Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(aisObject);

		if (!aisShape.IsNull()) {
			Handle_AIS_InteractiveContext context = m_mdiChild->h_MyViewer->getAisContext();
			context->Erase(aisShape, Standard_True);
			context->UpdateCurrentViewer();
		}

		// 从映射中移除
		m_itemToAisObjectMap.remove(currentItem);
	}

	// 从模型映射中移除
	if (modelMap.contains(modelName)) {
		modelMap.remove(modelName);
	}

	// 从树中删除项目
	if (currentItem->parent()) {
		currentItem->parent()->removeChild(currentItem);
	}
	else {
		int index = treeWidget->indexOfTopLevelItem(currentItem);
		if (index >= 0) {
			treeWidget->takeTopLevelItem(index);
		}
	}

	delete currentItem;
}


void ProjectTree::onItemClicked(QTreeWidgetItem* item, int column)
{
	// 处理树项点击事件
	// 获取当前选中的项
	QTreeWidgetItem* currentItem = treeWidget->currentItem();
	if (currentItem) {
		QString itemName = currentItem->text(0);
		Msg :: ShowInfo("Clicked item:") ;
	}
}



void ProjectTree::populateGeometryTable(QTableWidget* table, const BoundaryConditionData& bcData)
{
	// 统计几何形状类型和数量
	QMap<QString, int> shapeCounts;
	QMap<QString, QStringList> shapeIds;

	// 这里需要根据实际的边界条件数据结构来填充
	// 假设 bcData 包含几何形状信息
	for (const auto& shape : bcData.shapes) {
		QString type = GetShapeTypeName(shape);
		shapeCounts[type]++;
		// shapeIds[type].append(shape.getId()); // 需要根据实际数据结构调整
	}

	table->setRowCount(shapeCounts.size());
	int row = 0;

	for (auto it = shapeCounts.begin(); it != shapeCounts.end(); ++it, ++row) {
		table->setItem(row, 0, new QTableWidgetItem(it.key()));
		table->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));
		// table->setItem(row, 2, new QTableWidgetItem(shapeIds[it.key()].join(", ")));
	}

	table->resizeColumnsToContents();
}

void ProjectTree::populateMeshTable(QTableWidget* table, const BoundaryConditionData& bcData)
{
	// 填充网格节点信息
	// 假设 bcData 包含 meshIdsToConstraint 和 associatedNodePoints

	int rowCount = bcData.meshIdsToConstraint.size();
	table->setRowCount(rowCount);

	for (int i = 0; i < rowCount; ++i) {
		int nodeId = bcData.meshIdsToConstraint[i];
		gp_Pnt point = bcData.associatedNodePoints[i];

		table->setItem(i, 0, new QTableWidgetItem(QString::number(nodeId)));
		table->setItem(i, 1, new QTableWidgetItem(QString::number(point.X())));
		table->setItem(i, 2, new QTableWidgetItem(QString::number(point.Y())));
		table->setItem(i, 3, new QTableWidgetItem(QString::number(point.Z())));
	}

	table->resizeColumnsToContents();
}
