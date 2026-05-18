#include "QShapeImportUI.h"
#include "ComplainUtf8.h"
#include "Msg.h"
#include <QString>
#include <QFileDialog>
#include <QApplication>

QShapeImportUI::QShapeImportUI()
{
}

QShapeImportUI::~QShapeImportUI()
{
}

OCCT_Utf8String QShapeImportUI::getExtUpper(const OCCT_Utf8String & Utf8String)
{
	QString Str = Utf8String.ToCString();
	Str = Str.split(".").last().toUpper();
	return OCCT_Utf8String(Str.toUtf8().data());
}

OCCT_Utf8StringList QShapeImportUI::getLoadPathList(int partorcutter)
{
	bool OpenFlag = true;
	QString Dir = ".";
	QString workTitle = "选择导入的工件STL几何模型";
	QString cutterTitle = "选择导入的刀具STP几何模型";
	QString workFilter = 
		"STL文件(*.stl)"
		";;STEP文件(*.step *.stp)"
		//"STEP文件(*.step *.stp)"
		";;BREP文件(*.brep)"
		";;IGES文件(*.iges *.igs)"
		";;STL文件(*.stl)"
		";;All files (*.*)";
	QString cutterFilter =
		"STEP文件(*.step *.stp)"
		";;STL文件(*.stl)"
		";;BREP文件(*.brep)"
		";;IGES文件(*.iges *.igs)"		
		";;All files (*.*)";
	QStringList QPaths;
	switch (partorcutter){
		case 0:
		QPaths = QFileDialog::getOpenFileNames(nullptr, workTitle, Dir, workFilter);
		break;
		case 1:
			QPaths = QFileDialog::getOpenFileNames(nullptr, cutterTitle, Dir, cutterFilter);
			break;
	}
    for(QString Str : QPaths)
		Paths.Append(OCCT_Utf8String(Str.toUtf8().data()));
	return Paths;
}

OCCT_Utf8String QShapeImportUI::getLoadPath()
{
	bool OpenFlag = true;
	QString Title = "选择导入的几何模型";
	QString Dir = ".";
	QString Filter =
		"STEP文件(*.step *.stp)"
		";;BREP文件(*.brep)"
		";;IGES文件(*.iges *.igs)"
		";;STL文件(*.stl)"
		";;All files (*.*)";

	QString QPath = QFileDialog::getOpenFileName(nullptr, Title, Dir, Filter);
	return OCCT_Utf8String(QPath.toUtf8().data());
}

void QShapeImportUI::showPathSuccess(const OCCT_Utf8String & Path)
{
	Standard_Character Buffer[1024] = { 0 };
	Sprintf(Buffer, "文件[%s]打开成功!", Path.ToCString());
	Msg::ShowInfo(OCCT_Utf8String(Buffer));
}

void QShapeImportUI::showPathError(const OCCT_Utf8String & Path)
{
	Standard_Character Buffer[1024] = { 0 };
	Sprintf(Buffer, "文件[%s]打开失败!", Path.ToCString());
	Msg::ShowError(OCCT_Utf8String(Buffer));
}

void QShapeImportUI::showError(const OCCT_Utf8String & ErrorMsg)
{
	Msg::ShowError(ErrorMsg);
}

void QShapeImportUI::showSuccess(const OCCT_Utf8String & SuccessMsg)
{
	Msg::ShowInfo(SuccessMsg);
}

void QShapeImportUI::BeginWaitCursor()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void QShapeImportUI::EndWaitCursor()
{
	QApplication::restoreOverrideCursor();
}
