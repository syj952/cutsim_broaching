#include "mchconfig.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QTime>
QString txtPath2 = "C:/Users/b220/Desktop/Jixie2.txt";
QFileInfo fileInfo2(txtPath2);
QDir dir2 = fileInfo2.absoluteDir();

MchConfig::MchConfig(QWidget* parent)
    : QWidget(parent)
{
    //QWidget* mchwidget = new QWidget(this);
    QGridLayout* gridLayout = new QGridLayout(this);
    setWindowTitle("Machine CNC Configuration");

    QLabel* label_IP = new QLabel("IP", this);
    QLabel* label_port = new QLabel("   port", this);
    QLabel* label_CNC = new QLabel("CNC", this);
    QLabel* label_mch = new QLabel("机床模型导入", this);
    QLabel* label_wpcoor = new QLabel("工件坐标系", this);
    QLabel* label_lengthToolTip = new QLabel("刀柄及刀长", this);
    QLabel* label_offset = new QLabel("偏置", this);
    QLineEdit* lineEdit_IP = new QLineEdit("169.254.232.209", this);
    QLineEdit* lineEdit_port = new QLineEdit("19000", this);
    lineEdit_mch = new QLineEdit("", this);
    comboBox_CNC = new QComboBox(this);
    btn_CNCconnect = new QPushButton("连接数控系统", this);
    btn_CNCdisconnect = new QPushButton("断开连接", this);
    lineEdit_IP->setPlaceholderText("请输入IP地址");
    lineEdit_port->setPlaceholderText("请输入端口号");
    lineEdit_mch->setPlaceholderText("选择模型配置文件");
    comboBox_CNC->addItems({ "Siemens", "Heidenhain", "Fanuc" });
    connect(btn_CNCconnect, &QPushButton::clicked, this, &MchConfig::onButtonClicked);
    connect(btn_CNCdisconnect, &QPushButton::clicked, this, &MchConfig::offButtonClicked);
    btn_CNCdisconnect->setEnabled(0);
    lineEdit_wpcoor_X = new QLineEdit("-411.796", this);
    lineEdit_wpcoor_Y = new QLineEdit("397.633", this);
    lineEdit_wpcoor_Z = new QLineEdit("300.465", this);
    lineEdit_lengthToolTip = new QLineEdit("219.469", this);
    lineEdit_offset_X = new QLineEdit("6", this);
    lineEdit_offset_Y = new QLineEdit("0", this);
    lineEdit_offset_Z = new QLineEdit("0", this);//43.363//61.428

    //qss
    this->setStyleSheet(R"(
                        font-family: '微软雅黑', 'Microsoft YaHei', 'Microsoft YaHei UI', sans-serif;

                        QLineEdit, QComboBox, QSpinBox, QDateTimeEdit {
                        background: white;
                        border: none;
                        border-bottom: 1px solid #C0C0C0;
                        padding: 2px 20px 2px 4px;
                        min-height: 18px;
                        font: 13px "Microsoft YaHei";
                        }

                        QComboBox::down-arrow {
                        image: url(:/first/Resources/2.png);
                        border: none;
                        width: 12px;
                        height: 12px;
                        }
                        QComboBox::drop-down {
                        border: none;
                        background: transparent;
                        }
                        QComboBox QAbstractItemView {
                        color: #333333;
                        selection-color: #000000;
                        }

                        QLineEdit:focus, QComboBox:focus {
                        border-bottom: 2px solid #cccccc;
                        }
                        )");
    lineEdit_mch->setStyleSheet("padding-right:20px;");

    QAction* searchAction = lineEdit_mch->addAction(
        //QIcon(":/xsim/images/open.png"), QLineEdit::TrailingPosition
        QIcon::fromTheme("edit-find"), QLineEdit::TrailingPosition
    );
    connect(searchAction, &QAction::triggered, this, &MchConfig::selectMchFilePath);

    gridLayout->addWidget(label_IP, 0, 0);
    gridLayout->addWidget(lineEdit_IP, 0, 1);
    gridLayout->addWidget(label_port, 0, 2);
    gridLayout->addWidget(lineEdit_port, 0, 3);
    gridLayout->addWidget(label_CNC, 1, 0);
    gridLayout->addWidget(comboBox_CNC, 1, 1);
    gridLayout->addWidget(btn_CNCconnect, 1, 2);
    gridLayout->addWidget(btn_CNCdisconnect, 1, 3);
    gridLayout->addWidget(label_mch, 2, 0);
    gridLayout->addWidget(lineEdit_mch, 2, 1, 1, 3);
    gridLayout->addWidget(label_wpcoor, 3, 0);
    gridLayout->addWidget(lineEdit_wpcoor_X, 3, 1);
    gridLayout->addWidget(lineEdit_wpcoor_Y, 3, 2);
    gridLayout->addWidget(lineEdit_wpcoor_Z, 3, 3);
    gridLayout->addWidget(label_offset, 4, 0);
    gridLayout->addWidget(lineEdit_offset_X, 4, 1);
    gridLayout->addWidget(lineEdit_offset_Y, 4, 2);
    gridLayout->addWidget(lineEdit_offset_Z, 4, 3);
    gridLayout->addWidget(label_lengthToolTip, 5, 0);
    gridLayout->addWidget(lineEdit_lengthToolTip, 5, 1);
    gridLayout->setColumnStretch(6, 1);

    //mchwidget->setLayout(gridLayout);
}

MchConfig::~MchConfig() {
    if (isCollecting || thread) {
        offButtonClicked();
    }
}

void MchConfig::selectMchFilePath() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("选择机床模型文件"),
        QDir::currentPath(),
        tr("所有文件 (*.*);;文本文件 (*.txt);;mch文件 (*.mch)")
    );

    if (!filePath.isEmpty()) {
        lineEdit_mch->setText(filePath);
        emit mchFileSelected(filePath);
    }
}

void MchConfig::onButtonClicked() {
    double lengthToolTip = lineEdit_lengthToolTip->text().toDouble();
    QVector<double> workpieceOffset = {
        lineEdit_wpcoor_X->text().toDouble(),
        lineEdit_wpcoor_Y->text().toDouble(),
        lineEdit_wpcoor_Z->text().toDouble()
    };
    QVector<double> manualOffset = {
        lineEdit_offset_X->text().toDouble(),
        lineEdit_offset_Y->text().toDouble(),
        lineEdit_offset_Z->text().toDouble()
    };
    emit lengthToolTipUpdated(lengthToolTip, workpieceOffset, manualOffset);


    if (isCollecting || thread != nullptr) {
        qDebug() << "采集线程已在运行，无需重复启动";
        return;
    }
    int cncType = comboBox_CNC->currentIndex();//主线程中获取，避免子线程操作UI
    thread = new QThread(nullptr);//独立线程，避免随dialog被销毁

    connect(thread, &QThread::started, this, [=]() {
        timer = new QTimer(nullptr);
        timer->setInterval(20);//94.12

        if (cncType == 0) {
            //            Simensclient = new simensclient("192.168.101.50", 102);
            connect(timer, &QTimer::timeout, this, &MchConfig::GetMchData);
        }
        else if (cncType == 1) {
            hdhclient = new CncInfo_HeiDenHainCommands("169.254.232.209", 19000);
            QVariant a = 0;
            if (hdhclient->GetToolIndex(a))
                emit mchSingleDataSignal(a.toInt());
            if (hdhclient->login())//建立连接后立即登陆了PLC！
                connect(timer, &QTimer::timeout, this, &MchConfig::GetHDHData);
            else
                qDebug() << "PLC login false";
        }
        else {
            qDebug() << "cnc type error";
            return;
        }
        timer->start();
        }, Qt::QueuedConnection);

    connect(thread, &QThread::finished, this, [=]() {
        if (timer) {
            timer->stop();
            timer->deleteLater();
            timer = nullptr;
        }
        if (hdhclient) {
            hdhclient->logout();
            hdhclient->Disconnect();
            delete hdhclient;
            hdhclient = nullptr;
        }
        if (Simensclient) {
            Simensclient->Disconnect();
            delete Simensclient;
            Simensclient = nullptr;
        }
        }, Qt::QueuedConnection);

    connect(thread, &QThread::finished, this, [=]() {
        thread = nullptr;
        isCollecting = false;
        btn_CNCconnect->setEnabled(true);
        btn_CNCdisconnect->setEnabled(false);
        }, Qt::QueuedConnection);

    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    isCollecting = true;
    btn_CNCconnect->setEnabled(false);
    btn_CNCdisconnect->setEnabled(true);
}

void MchConfig::offButtonClicked() {
    if (!isCollecting || thread == nullptr) {
        qDebug() << "no data-getting thread";
        return;
    }

    isCollecting = false;

    if (timer) {
        timer->stop();
        timer->deleteLater();
        timer = nullptr;
    }

    thread->quit();
    thread->wait(1000);
    thread->deleteLater();
    thread = nullptr;

    if (hdhclient) {
        hdhclient->logout();
        hdhclient->Disconnect();
        delete hdhclient;
        hdhclient = nullptr;
    }
    if (Simensclient) {
        Simensclient->Disconnect();
        delete Simensclient;
        Simensclient = nullptr;
    }

    btn_CNCconnect->setEnabled(true);
    btn_CNCdisconnect->setEnabled(false);
}

void MchConfig::GetMchData() {
    //    if (!isCollecting || QThread::currentThread() != thread) {
    //        return;
    //    }

    //ppppp[3] += 0.1;
    //ppppp[4] += 0.1;
    //ppppp[0] = 564.059;
    //ppppp[1] = -397.368;
    //ppppp[2] = -42.595 * 2 - (600 - 150 - 218.493);
    ppppp[1] -= 0.182;
    emit mchDataSignal(ppppp, 0, 0);
}

void MchConfig::GetHDHData() {
    //    if (!isCollecting || QThread::currentThread() != thread || !hdhclient) {
    //        return;
    //    }

    QVariant a;
    if (hdhclient->GetMtPos(a, ppppp.data())) {
        //        ppppp[3] -= 360;
        if (ppppp[4] > 120)
            ppppp[4] -= 360;
        ppppp[3] += 1.3303;
    }

    QFile file(txtPath2);
    if (file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&file);
        out.setCodec("UTF-8");  // Release 不乱码

        QString currentTime = QTime::currentTime().toString("HH:mm:ss.zzz");

        out << currentTime << "\t" << ppppp[0] << "\t" << ppppp[1] << "\t" << ppppp[2] << "\t" << ppppp[4] << "\t" << ppppp[3] << "\n";
    }

    if (hdhclient->GetFeed(a))
        act_feed = a.toDouble();
    if (hdhclient->GetSpindleSpeed(a))
        act_rpm = a.toDouble();
    emit mchDataSignal(ppppp, act_rpm, act_feed);
}
