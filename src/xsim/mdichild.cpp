#include "mdichild.h"
#include "ComplainUtf8.h"
#include "GUI_Message.h"

#include <QtWidgets>
#include <QDockWidget>

MdiChild::MdiChild(QWidget * parent)
	:QMainWindow(parent)
{
	//InitMBD
	h_MyDoc = new MyDocument();
	h_MyViewer = new MyViewer(h_MyDoc);
	//Config
	this->setMenuBar(nullptr);
	setAttribute(Qt::WA_DeleteOnClose);
	isUntitled = true;
	//Init
	this->Init();
    
    m_gmshMessageHandler = std::make_shared<GmshMessageHandler>();// ʹ�� make_shared ��������
    
    connect(m_gmshMessageHandler.get(), &GmshMessageHandler::messageReceived,
        this, &MdiChild::onGmshMessageReceived);// �����źŲ�

    visulization_item = new int(0);
    visulization_limits[0] = 0;
    visulization_limits[1] = 0.05;
}

MdiChild::~MdiChild()
{
}

void MdiChild::SetCurrent()
{
	p_MessageWidget->SetCurrentMessager();
}

void MdiChild::createMDIActions()
{
}

QString MdiChild::userFriendlyCurrentFile()
{
    return strippedName(curFile);
}

QList<QAction*> MdiChild::GetDockActions()
{
    return QList<QAction*>()
        << p_MessageDock->toggleViewAction()
        << p_TreeDock->toggleViewAction()
        << p_PropertyDock->toggleViewAction();
		//<< p_AddGD_T_Dock->toggleViewAction()
		//<< p_AddSR_Dock->toggleViewAction();
}

void MdiChild::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MdiChild::documentWasModified()
{
    setWindowModified(document()->isModified());
}


bool MdiChild::maybeSave()
{
    if (!document()->isModified())
        return true;
    const QMessageBox::StandardButton ret
            = QMessageBox::warning(this, tr("MDI"),
                                   tr("'%1' has been modified.\n"
                                      "Do you want to save your changes?")
                                   .arg(userFriendlyCurrentFile()),
                                   QMessageBox::Save | QMessageBox::Discard
                                   | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return true;
}

void MdiChild::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(fileName).canonicalFilePath();
    isUntitled = false;
    document()->setModified(false);
    setWindowModified(false);
    setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString MdiChild::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MdiChild::onStraightnessDataUpdated(int blade_id, int point_index,
    double stroke_data,
    double straightness_data)
{
    qDebug() << "MdiChild::onStraightnessDataUpdated called";
    qDebug() << "Blade:" << blade_id << "Point:" << point_index;
    qDebug() << "Data values - Stroke:" << stroke_data << "Straightness:" << straightness_data;  // 修改这里
    // 转发数据到直线度窗口
    if (straightnessWidget) {
        straightnessWidget->setData(blade_id, point_index, stroke_data, straightness_data);
    }
}
