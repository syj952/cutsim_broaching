#include "mdichild.h"
#include "../digitaltwin/monitoring/serialportcom.h"
#include <QDialog>
#include <QVBoxLayout>
#include "../digitaltwin/monitoring/vtkwidget.h"

void MdiChild::RunSerialPort()
{
    // 复用已存在的对话框（按 objectName 查找），避免在头文件里加成员变量
    QDialog* dialog = this->findChild<QDialog*>("SerialportcomDialog");
    SerialPortCom* serialportWidget = nullptr;
    if (!dialog) {
        dialog = new QDialog(this);
        dialog->setObjectName("SerialportcomDialog");
        dialog->setWindowTitle("串口通讯");
        dialog->resize(750, 320);
        dialog->setAttribute(Qt::WA_DeleteOnClose, false); // 关闭只隐藏，不销毁

        serialportWidget = new SerialPortCom(dialog);
        serialportWidget->setObjectName("SerialPortComDialog");

        connect(serialportWidget, &SerialPortCom::newData, this->p_VtkWidget, &VTKWidget::linshi_2, Qt::UniqueConnection);
        connect(serialportWidget, &SerialPortCom::newData, this, &MdiChild::updateMachineCommForceData, Qt::UniqueConnection);

        QVBoxLayout* layout = new QVBoxLayout(dialog);
        layout->addWidget(serialportWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        dialog->setLayout(layout);
    }
    else {
        // 已存在的对话框中也能拿到同一个 MchConfig（如需用到）
        serialportWidget = dialog->findChild<SerialPortCom*>("SerialPortComDialog");
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
