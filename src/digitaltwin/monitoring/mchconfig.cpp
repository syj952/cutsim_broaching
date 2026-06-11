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
    lineEdit_offset_X = new QLineEdit("0", this);
    lineEdit_offset_Y = new QLineEdit("0", this);
    lineEdit_offset_Z = new QLineEdit("58", this);//43.363//61.428

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
        timer->setInterval(25);

        if (cncType == 0) {
            //Simensclient = new simensclient("192.168.101.50", 102);
            openMchDataFile("../../data/F01.txt");
            connect(timer, &QTimer::timeout, this, &MchConfig::GetMchData);
        }
        else if (cncType == 1) {
            hdhclient = new CncInfo_HeiDenHainCommands("169.254.232.209", 19000);
            QVariant a = 0;
            if (hdhclient->GetToolIndex(a))
                emit mchSingleDataSignal(a.toInt());
            if (hdhclient->login() && hdhclient->r_pr())//建立连接后立即登陆了PLC！
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
    
    readGCodeFromTxt(ppppp, act_feed, act_rpm);
    emit mchDataSignal_forGCode(ppppp, act_rpm, act_feed);

    //readActualCoorFromTxt(ppppp);
    //emit mchDataSignal(ppppp, 0, 0);
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
    }

    //QFile file(txtPath2);
    //if (file.open(QIODevice::Append | QIODevice::Text))
    //{
    //    QTextStream out(&file);
    //    out.setCodec("UTF-8");  // Release 不乱码

    //    QString currentTime = QTime::currentTime().toString("HH:mm:ss.zzz");

    //    out << currentTime << "\t" << ppppp[0] << "\t" << ppppp[1] << "\t" << ppppp[2] << "\t" << ppppp[4] << "\t" << ppppp[3] << "\n";
    //}

    if (hdhclient->GetFeed(a))
        act_feed = a.toDouble();
    if (hdhclient->GetSpindleSpeed(a))
        act_rpm = a.toDouble();
    emit mchDataSignal(ppppp, act_rpm, act_feed);
}

void MchConfig::openMchDataFile(const QString& filePath)
{
    if (m_mchDataFile.isOpen())
        m_mchDataFile.close();

    m_mchDataFile.setFileName(filePath);

    if (!m_mchDataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Open txt fail:" << filePath << m_mchDataFile.errorString();
        m_mchDataFileOpened = false;
        return;
    }

    m_mchDataStream.setDevice(&m_mchDataFile);
    m_mchDataFileOpened = true;

    qDebug() << "Open txt succeed:" << filePath;
}

bool MchConfig::readActualCoorFromTxt(std::array<double, 6>& macpos)
{
    if (!m_mchDataFileOpened || !m_mchDataFile.isOpen()) {
        qDebug() << "txt didn't open";
        return false;
    }

    if (m_mchDataStream.atEnd()) {
        qDebug() << "txt reading ended";
        return true;
    }

    QString time;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double a = 0.0;
    double c = 0.0;

    m_mchDataStream >> time >> x >> y >> z >> a >> c;

    if (m_mchDataStream.status() != QTextStream::Ok) {
        qDebug() << "txt read failed";
        return false;
    }

    macpos[0] = x;
    macpos[1] = y;
    macpos[2] = z;
    macpos[4] = a;
    macpos[3] = c;

    return true;
}

bool MchConfig::readGCodeFromTxt(std::array<double, 6>& macpos, double& F, double& S)
{
    if (!m_mchDataFileOpened || !m_mchDataFile.isOpen()) {
        qDebug() << "txt didn't open";
        return 0;
    }

    if (m_mchDataStream.atEnd()) {
        qDebug() << "txt reading ended";
        return 1;
    }

    QString line = m_mchDataStream.readLine().trimmed();

    if (line.isEmpty()) return 1;

    // 去掉行号，例如：123 L X192.968 Y36.292 ...
    QStringList tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

    for (int i = 0; i < tokens.size(); ++i) {
        QString token = tokens[i];

        if (token.isEmpty()) {
            continue;
        }

        QChar axis = token.at(0).toUpper();

        if (axis != 'X' && axis != 'Y' && axis != 'Z' && axis != 'A' && axis != 'C' && axis != 'S' && axis != 'F') {
            continue;
        }

        bool ok = false;
        double value = 0.0;

        if (token.length() >= 2) {
            // 情况1：X192.968
            value = token.mid(1).toDouble(&ok);
        }
        else if (i + 1 < tokens.size()) {
            // 情况2：X 192.968
            value = tokens[i + 1].toDouble(&ok);
            if (ok) {
                ++i;   // 跳过已经读取的数值
            }
        }

        if (!ok) {
            continue;
        }

        switch (axis.toLatin1()) {
        case 'X':
            macpos[0] = value;
            break;
        case 'Y':
            macpos[1] = value;
            break;
        case 'Z':
            macpos[2] = value;
            break;
        case 'C':
            macpos[3] = value;
            break;
        case 'A':
            macpos[4] = value;
            break;
        case 'F':
            F = value;
            break;
        case 'S':
            S = value;
            break;
        default:
            break;
        }
    }

    return 1;
}