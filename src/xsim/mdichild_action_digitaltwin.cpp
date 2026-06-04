#include "mdichild.h"
#include "src/digitaltwin/monitoring/mchconfig.h"
#include <QDialog>
#include <QVBoxLayout>
#include "src/digitaltwin/monitoring/vtkwidget.h"
#include "src/digitaltwin/digitaltwinctrl.h"

void MdiChild::RunDigitalTwin()
{
    // 复用已存在的对话框（按 objectName 查找），避免在头文件里加成员变量
    QDialog* dialog = this->findChild<QDialog*>("DigitalTwinDialog");
    MchConfig* configWidget = nullptr;
    SerialPortCom* serialportWidget = nullptr;
    if (!dialog) {
        dialog = new QDialog(this);
        dialog->setObjectName("DigitalTwinDialog");
        dialog->setWindowTitle("数字孪生");
        dialog->resize(1200, 500);
        dialog->setAttribute(Qt::WA_DeleteOnClose, false); // 关闭只隐藏，不销毁

        QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
        QHBoxLayout* topLayout = new QHBoxLayout();
        QVBoxLayout* bottomLayout = new QVBoxLayout();
        mainLayout->addLayout(topLayout);
        mainLayout->addLayout(bottomLayout);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        dialog->setLayout(mainLayout);

        ///————————————机床通讯————————————
        configWidget = new MchConfig(dialog);
        configWidget->setObjectName("MchConfigWidget");
        topLayout->addWidget(configWidget, 1);

        if (!m_digitalTwinController) {
            m_digitalTwinController = new DigitalTwinController(this);
        }

        connect(configWidget, &MchConfig::mchFileSelected, this->p_VtkWidget, &VTKWidget::loadMchFile, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchDataSignal, this->p_VtkWidget, &VTKWidget::linshi, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::mchSingleDataSignal, this->p_VtkWidget, &VTKWidget::linshi_toolindex, Qt::UniqueConnection);
        connect(configWidget, &MchConfig::lengthToolTipUpdated, this->p_VtkWidget, &VTKWidget::onlengthToolTipUpdated, Qt::UniqueConnection);
        connect(this->p_VtkWidget, &VTKWidget::mchDatatoCutsim, m_digitalTwinController, &DigitalTwinController::onMchDataUpdated, Qt::UniqueConnection);

        ///————————————串口通讯————————————
        serialportWidget = new SerialPortCom(dialog);
        serialportWidget->setObjectName("SerialPortComWidget");
        topLayout->addWidget(serialportWidget, 1);

        connect(serialportWidget, &SerialPortCom::newData, this->p_VtkWidget, &VTKWidget::linshi_2, Qt::UniqueConnection);
        connect(serialportWidget, &SerialPortCom::newData, m_digitalTwinController, &DigitalTwinController::onForceDataUpdated, Qt::UniqueConnection);

        ///———————————材料去除仿真———————————
#pragma region 
// 工件文件选择
        QHBoxLayout* digitaltwin_workfileLayout = new QHBoxLayout();
        QLabel* digitaltwin_workfileLabel = new QLabel("Select workpiece model");
        QLineEdit* digitaltwin_workfileEdit = new QLineEdit();
        digitaltwin_workfileEdit->setReadOnly(true);
        QPushButton* digitaltwin_workfileButton = new QPushButton("Select file...");
        digitaltwin_workfileLayout->addWidget(digitaltwin_workfileLabel);
        digitaltwin_workfileLayout->addWidget(digitaltwin_workfileEdit, 1); // 1表示拉伸因子
        digitaltwin_workfileLayout->addWidget(digitaltwin_workfileButton);
        bottomLayout->addLayout(digitaltwin_workfileLayout);


        // 约束
        QHBoxLayout* digitaltwin_constraintLayout = new QHBoxLayout();
        QLabel* digitaltwin_constraintsLabel = new QLabel("Constrains:");
        digitaltwin_constraintLayout->addWidget(digitaltwin_constraintsLabel);
        bottomLayout->addLayout(digitaltwin_constraintLayout);
        // 约束 x
        QHBoxLayout* digitaltwin_xconstraint = new QHBoxLayout();
        QLabel* digitaltwin_xLabel = new QLabel("X:");
        QLineEdit* digitaltwin_x1Edit = new QLineEdit("-100");
        //x1Edit->setReadOnly(true);
        QLineEdit* digitaltwin_x2Edit = new QLineEdit("100");
        //x2Edit->setReadOnly(true);
        digitaltwin_xconstraint->addWidget(digitaltwin_xLabel);
        digitaltwin_xconstraint->addWidget(digitaltwin_x1Edit);
        digitaltwin_xconstraint->addWidget(digitaltwin_x2Edit);
        bottomLayout->addLayout(digitaltwin_xconstraint);
        // 约束 y
        QHBoxLayout* digitaltwin_yconstraint = new QHBoxLayout();
        QLabel* digitaltwin_yLabel = new QLabel("Y:");
        QLineEdit* digitaltwin_y1Edit = new QLineEdit("-100");
        //y1Edit->setReadOnly(true);
        QLineEdit* digitaltwin_y2Edit = new QLineEdit("100");
        //y2Edit->setReadOnly(true);
        digitaltwin_yconstraint->addWidget(digitaltwin_yLabel);
        digitaltwin_yconstraint->addWidget(digitaltwin_y1Edit);
        digitaltwin_yconstraint->addWidget(digitaltwin_y2Edit);
        bottomLayout->addLayout(digitaltwin_yconstraint);
        // 约束 z
        QHBoxLayout* digitaltwin_zconstraint = new QHBoxLayout();
        QLabel* digitaltwin_zLabel = new QLabel("z:");
        QLineEdit* digitaltwin_z1Edit = new QLineEdit("-100");
        //y1Edit->setReadOnly(true);
        QLineEdit* digitaltwin_z2Edit = new QLineEdit("0");
        //y2Edit->setReadOnly(true);
        digitaltwin_zconstraint->addWidget(digitaltwin_zLabel);
        digitaltwin_zconstraint->addWidget(digitaltwin_z1Edit);
        digitaltwin_zconstraint->addWidget(digitaltwin_z2Edit);
        bottomLayout->addLayout(digitaltwin_zconstraint);

        // 刀具参数输入
        QHBoxLayout* cutterParamsLayout = new QHBoxLayout();
        QLabel* cutterParamsLabel = new QLabel("Tool Parameters");
        QLineEdit* cutterParamsEdit = new QLineEdit("0,4,4,31,0,31;");
        cutterParamsEdit->setPlaceholderText("Format: Type, Radius1, Radius2, Length, z_start, z_end;...");
        cutterParamsLayout->addWidget(cutterParamsLabel);
        cutterParamsLayout->addWidget(cutterParamsEdit, 1);
        bottomLayout->addLayout(cutterParamsLayout);

        // 按钮区域
        QHBoxLayout* digitaltwin_buttonLayout = new QHBoxLayout();
        QPushButton* digitaltwin_runButton = new QPushButton("RunCutsim");
        digitaltwin_buttonLayout->addWidget(digitaltwin_runButton);
        digitaltwin_buttonLayout->addStretch();
        bottomLayout->addLayout(digitaltwin_buttonLayout);

        // 连接按钮信号
        //connect(digitaltwin_workfileButton, &QPushButton::clicked, [&]() {
        connect(digitaltwin_workfileButton, &QPushButton::clicked,
            this,
            [this, dialog, digitaltwin_workfileEdit]() {
                QString path = QFileDialog::getOpenFileName(dialog, "Select workpiece model", "", "STL Files (*.stl)");
                if (!path.isEmpty()) {
                    workfilePath = path;
                    digitaltwin_workfileEdit->setText(path);
                }
            });

        //std::vector<std::array<double, 3>> force_data;
        //force_data.assign(350, { 100.0, 100.0, 0.0 });
        //std::array<double, 14> Msh_Data{};
        // 保存刀具参数输入控件的指针
        this->cutterParamsEdit = cutterParamsEdit;

        //connect(digitaltwin_runButton, &QPushButton::clicked, [&]() {
        connect(digitaltwin_runButton, &QPushButton::clicked,
            this,
            [this,
            digitaltwin_x1Edit, digitaltwin_x2Edit,
            digitaltwin_y1Edit, digitaltwin_y2Edit,
            digitaltwin_z1Edit, digitaltwin_z2Edit,
            cutterParamsEdit]() {
                //std::vector<std::array<double, 3>> force_data(350, { 100.0, 100.0, 0.0 });
                //std::array<double, 14> Msh_Data{};
                digitaltwin_millpar.constrain_limits[0][0] = digitaltwin_x1Edit->text().toDouble();
                digitaltwin_millpar.constrain_limits[0][1] = digitaltwin_x2Edit->text().toDouble();
                digitaltwin_millpar.constrain_limits[1][0] = digitaltwin_y1Edit->text().toDouble();
                digitaltwin_millpar.constrain_limits[1][1] = digitaltwin_y2Edit->text().toDouble();
                digitaltwin_millpar.constrain_limits[2][0] = digitaltwin_z1Edit->text().toDouble();
                digitaltwin_millpar.constrain_limits[2][1] = digitaltwin_z2Edit->text().toDouble();

                // 解析刀具参数
                digitaltwin_millpar.cutter_segments.clear();
                QString params = cutterParamsEdit->text();
                QStringList segmentList = params.split(';');
                for (const QString& segStr : segmentList) {
                    QStringList values = segStr.split(',');
                    if (values.size() != 6) continue;

                    int type = values[0].toInt();
                    double radius1 = values[1].toDouble();
                    double radius2 = values[2].toDouble();
                    double length = values[3].toDouble();
                    double z_start = values[4].toDouble();
                    double z_end = values[5].toDouble();

                    digitaltwin_millpar.cutter_segments.emplace_back(type, radius1, radius2, length, z_start, z_end);
                }

                //dialog.accept();
                //digitaltwin_milling_executeSimulation(workfilePath, Msh_Data, force_data);
                digitaltwin_milling_executeSimulation(workfilePath);
                qDebug() << "cutsim<<<<<<<<";
            });
#pragma endregion

    }
    else {
        // 已存在的对话框中也能拿到同一个 MchConfig（如需用到）
        configWidget = dialog->findChild<MchConfig*>("MchConfigWidget");
        serialportWidget = dialog->findChild<SerialPortCom*>("SerialPortComWidget");
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}