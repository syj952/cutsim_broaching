#include "mdichild.h"
#include "ComplainUtf8.h"
#include <QtWidgets>
#include <QDockWidget>
#include "ProjectTree.h"
#include "Msg.h"

void MdiChild::newFile()
{
	static int sequenceNumber = 1;

	isUntitled = true;
	curFile = tr("My文件%1.occfile").arg(sequenceNumber++);
	setWindowTitle(curFile + "[*]");

	//connect(document(), &mdidocument::contentsChanged,
	//        this, &MdiChild::documentWasModified);
}

bool MdiChild::save()
{
	if (isUntitled) {
		return saveAs();
	}
	else {
		return saveFile(curFile);
	}
}

bool MdiChild::saveAs()
{
	QString Title = "SAVE AS";
	QString Filter = "My文件(*.occfile)"
		";;My文件2(*.occfile2)";
	QString fileName = QFileDialog::getSaveFileName(nullptr, Title, curFile, Filter);
	if (fileName.isEmpty())
		return false;

	return saveFile(fileName);
}

bool MdiChild::loadFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly)) {
		QMessageBox::warning(this, tr("MDI"),
			tr("不能读取文件 %1:\n%2.")
			.arg(fileName)
			.arg(file.errorString()));
		return false;
	}

	QDataStream in(&file);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	//document()->LoadData(in); //LOAD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	QApplication::restoreOverrideCursor();
	setCurrentFile(fileName);

	//connect(document(), &mdidocument::contentsChanged,
	//	this, &MdiChild::documentWasModified);

	return true;
}

bool MdiChild::saveFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly)) {
		QMessageBox::warning(this, tr("MDI"),
			tr("Cannot write file %1:\n%2.")
			.arg(QDir::toNativeSeparators(fileName), file.errorString()));
		return false;
	}

	QDataStream out(&file);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	//document()->SaveData(out); //SAVE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	QApplication::restoreOverrideCursor();

	setCurrentFile(fileName);
	return true;
}

void MdiChild::DeleteSelObjects()
{
	//p_TreeWidget->deleteSelect();
	h_MyViewer->DeleteSelectedObjects();
}

void MdiChild::clearModel()
{
	p_TreeWidget->clearModels();
	h_MyViewer->AisObjDeleteAll();
	Msg::ShowInfo("清空模型成功");
}


#include "ComplainUtf8.h"
#include "mdichild.h"
#include "QShapeImportUI.h"
#include "OCCT_ShapeList.h"
#include <AIS_Shape.hxx>

void MdiChild::AisObjDisplayAll()
{
	h_MyViewer->getAisContext()->DisplayAll(Standard_True);
}

void MdiChild::AisObjEraseAll()
{
	h_MyViewer->getAisContext()->EraseAll(Standard_True);
}

void MdiChild::AisObjHide()
{
	h_MyViewer->AisObjHide();
}

#include <QColorDialog>
namespace
{
	Quantity_Color GetColor(Quantity_Color oldColor)
	{
		QColor color = QColorDialog::getColor(Qt::red, nullptr, "颜色对话框", QColorDialog::ShowAlphaChannel);
		double R = color.red() / 255.0;
		double G = color.green() / 255.0;
		double B = color.blue() / 255.0;
		Quantity_Color Color(R, G, B, Quantity_TOC_RGB);
		return Color;
	}
	OCCT_Utf8String GetInPut(const OCCT_Utf8String & Title,QWidget * parent)
	{
		QDialog Dialog(parent);
		Dialog.setWindowTitle(Title.ToCString());
		Dialog.setMinimumWidth(400);
		Dialog.setMaximumHeight(120);
		
		QVBoxLayout * pMainLayout = new QVBoxLayout(&Dialog);
		
		// 创建滑块和标签的布局
		QHBoxLayout * pSliderLayout = new QHBoxLayout();
		
		// 创建左侧"不透明"标签
		QLabel * pOpaqueLabel = new QLabel("不透明", &Dialog);
		pOpaqueLabel->setMinimumWidth(50);
		pOpaqueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		
		// 创建滑块
		QSlider * pSlider = new QSlider(Qt::Horizontal, &Dialog);
		pSlider->setMinimum(0);      // 最小值为0（完全不透明）
		pSlider->setMaximum(100);    // 最大值为100（完全透明）
		pSlider->setValue(0);        // 默认值为0
		pSlider->setMinimumWidth(300);
		
		// 创建右侧"透明"标签
		QLabel * pTransparentLabel = new QLabel("透明", &Dialog);
		pTransparentLabel->setMinimumWidth(50);
		pTransparentLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		
		// 创建显示当前值的标签
		QLabel * pValueLabel = new QLabel("0.00", &Dialog);
		pValueLabel->setMinimumWidth(60);
		pValueLabel->setAlignment(Qt::AlignCenter);
		
		// 连接滑块值变化信号，更新标签显示
		QObject::connect(pSlider, &QSlider::valueChanged, [pValueLabel](int value) {
			double transparency = value / 100.0;
			pValueLabel->setText(QString::number(transparency, 'f', 2));
		});
		
		pSliderLayout->addWidget(pOpaqueLabel);
		pSliderLayout->addWidget(pSlider);
		pSliderLayout->addWidget(pTransparentLabel);
		pSliderLayout->addWidget(pValueLabel);
		
		// 创建按钮
		QDialogButtonBox * pBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &Dialog);
		QDialog::connect(pBox, &QDialogButtonBox::accepted, &Dialog, &QDialog::accept);
		QDialog::connect(pBox, &QDialogButtonBox::rejected, &Dialog, &QDialog::reject);
		
		pMainLayout->addLayout(pSliderLayout);
		pMainLayout->addWidget(pBox);
		Dialog.setLayout(pMainLayout);
		
		if (QDialog::Accepted == Dialog.exec())
		{
			// 将滑块值转换为0-1之间的浮点数字符串
			double transparency = pSlider->value() / 100.0;
			return QString::number(transparency).toUtf8().data();
		}
		else
			return "";
	}
}
void MdiChild::SelTrans()
{
	OCCT_Utf8String Str = GetInPut("设置透明度值",this);
	if (Str.IsEmpty())
		return;
	double val = atof(Str.ToCString());
	h_MyViewer->AisObjTransparency(val);
}


void MdiChild::SelColor()
{
	Quantity_Color Color = GetColor(Quantity_NOC_RED);
	h_MyViewer->AisObjColor(Color);
}

void MdiChild::SelUnColor()
{
	h_MyViewer->AisObjUnColor();
}

void MdiChild::SelUnTrans()
{
	h_MyViewer->AisObjUnTransparency();
}

void MdiChild::SelWireFrame()
{
	h_MyViewer->AisObjWireFrame();
}

void MdiChild::SelShaded()
{
	h_MyViewer->AisObjShaded();
}

void MdiChild::DisplayModel(const TopoDS_Shape& shape)
{
	h_MyViewer->Display(shape);
}

void MdiChild::EraseModel(const TopoDS_Shape& shape)
{
	h_MyViewer->Erase(shape);
}

void MdiChild::EraseModel(const Handle(AIS_InteractiveObject)& aisObj)
{
	h_MyViewer->Erase(aisObj);
}

void MdiChild::updateView() {
	h_MyViewer->Redraw();
}
