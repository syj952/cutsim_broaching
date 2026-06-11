#include "mdichild.h"
#include "../digitaltwin/monitoring/mchconfig.h"
#include <QDialog>
#include <QVBoxLayout>
#include "../digitaltwin/monitoring/vtkwidget.h"

void MdiChild::RunMchConfig()
{
    // 复用已存在的对话框（按 objectName 查找），避免在头文件里加成员变量
    QDialog* dialog = this->findChild<QDialog*>("MchConfigDialog");
    MchConfig* configWidget = nullptr;
    if (!dialog) {
        dialog = new QDialog(this);
        dialog->setObjectName("MchConfigDialog");
        dialog->setWindowTitle("机床配置");
        dialog->resize(750, 320);
        dialog->setAttribute(Qt::WA_DeleteOnClose, false); // 关闭只隐藏，不销毁

        configWidget = new MchConfig(dialog);
        configWidget->setObjectName("MchConfigWidget");

        // 只在首次创建时建立连接，后续复用
        connect(configWidget, &MchConfig::mchFileSelected, this->p_VtkWidget, &VTKWidget::loadMchFile, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchDataSignal, this->p_VtkWidget, &VTKWidget::linshi, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchDataSignal, this, &MdiChild::updateMachineCommData, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchDataSignal_forGCode, this->p_VtkWidget, &VTKWidget::linshi_GCode, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchDataSignal_forGCode, this, &MdiChild::updateMachineCommData, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchSingleDataSignal, this->p_VtkWidget, &VTKWidget::linshi_toolindex, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchSingleDataSignal, this, &MdiChild::updateMachineCommToolIndex, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::lengthToolTipUpdated, this->p_VtkWidget, &VTKWidget::onlengthToolTipUpdated, Qt::UniqueConnection);

        QVBoxLayout* layout = new QVBoxLayout(dialog);
        layout->addWidget(configWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        dialog->setLayout(layout);
    }
    else {
        // 已存在的对话框中也能拿到同一个 MchConfig（如需用到）
        configWidget = dialog->findChild<MchConfig*>("MchConfigWidget");
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
