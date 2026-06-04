#include "serialportcom.h"

SerialPortCom::SerialPortCom(QWidget *parent)
    : QWidget(parent)
{
    QWidget* portwidget = new QWidget(this);
    QGridLayout *gridLayout = new QGridLayout(portwidget);
    setWindowTitle("serialport");

    botton_Refresh = new QPushButton("Refresh", portwidget);
    botton_openPort = new QPushButton("Connect", portwidget);
    botton_disConnect = new QPushButton("Disconnect", portwidget);
    botton_Reset = new QPushButton("Reset", portwidget);
    QLabel *label_port = new QLabel("PORT", portwidget);
    comboPort = new QComboBox(portwidget);
    connect(botton_Refresh, &QPushButton::clicked, this, &SerialPortCom::on_Refresh_clicked);
    connect(botton_openPort, &QPushButton::clicked, this, &SerialPortCom::on_openport_clicked);
    connect(botton_disConnect, &QPushButton::clicked, this, &SerialPortCom::on_disconnect_clicked);
    connect(botton_Reset, &QPushButton::clicked, this, &SerialPortCom::on_reset_clicked);

    portwidget->setStyleSheet(R"(
                        font-family: 'Microsoft YaHei', 'Microsoft YaHei UI', sans-serif;

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

    QStringList portNames;
    for (const QSerialPortInfo& port : QSerialPortInfo::availablePorts()) {
        portNames << port.portName();
    }
    if (QSerialPortInfo::availablePorts().size() == 0)
        qDebug()<<"No ports detected";
    comboPort->addItems(portNames);

    gridLayout->addWidget(botton_Refresh, 0, 0);
    gridLayout->addWidget(botton_Reset, 0, 1);
    gridLayout->addWidget(label_port, 1, 0);
    gridLayout->addWidget(comboPort, 1, 1);
    gridLayout->addWidget(botton_openPort, 2, 0);
    gridLayout->addWidget(botton_disConnect, 2, 1);
    gridLayout->setColumnStretch(3, 1);
    botton_openPort->setEnabled (1);
    botton_disConnect->setEnabled (0);
    botton_Reset->setEnabled(0);

    portwidget->setLayout(gridLayout);

    // initialize
    windowBuffers.resize(4);
    windowSums.resize(4);
    for(int i = 0; i < 4; ++i) {
        windowSums[i] = 0.0;
    }

}

SerialPortCom::~SerialPortCom()
{
    if (serialPort) {
        disconnect(serialPort, &QSerialPort::readyRead, this, &SerialPortCom::ReadData);
        if (serialPort->isOpen()) {
            serialPort->close();
        }
        delete serialPort;
        serialPort = nullptr;
    }

    //if (plotmanager) {
    //    delete plotmanager;
    //    plotmanager = nullptr;
    //}
}

void SerialPortCom::on_openport_clicked()
{
//    // 若已存在串口对象，先关闭并释放
//    if (serialPort) {
//        if (serialPort->isOpen()) {
//            serialPort->close();
//        }
//        disconnect(serialPort, &QSerialPort::readyRead, this, &SerialPortCom::ReadData);
//        delete serialPort;
//        serialPort = nullptr;
//    }

    //if (plotmanager) {
    //    plotmanager->initializePlot(4, -2000, 2000);
    //}//xiecheng hanshu

//    file.setFileName("C:/Users/Administrator/Desktop/serialport_test.txt");
//    if (!file.open(QIODevice::Append | QIODevice::Text)) {
//        qDebug() << "无法打开文件";
//        out = nullptr;
//    }else{
//        out = new QTextStream(&file);}

    QSerialPortInfo portInfo(comboPort->currentText());
    serialPort = new QSerialPort (portInfo, nullptr);

    if (serialPort->open(QIODevice::ReadWrite)){
        qDebug()<<"open port succeed";
        serialPort->setBaudRate (921600);
        serialPort->setParity (QSerialPort::NoParity);
        serialPort->setDataBits (QSerialPort::Data8);
        serialPort->setStopBits (QSerialPort::OneStop);
        connect(serialPort, SIGNAL(readyRead()), this, SLOT(ReadData()));

        botton_openPort->setEnabled (0);
        botton_disConnect->setEnabled (1);
        botton_Reset->setEnabled(1);

        QTimer::singleShot(1, this, [this]() {
            if (serialPort && serialPort->isOpen()) {
                serialPort->write("R");
//                qDebug() << "Sent auto-reset command: R";

                QTimer::singleShot(500, this, [this]() {
                    for(int i = 0; i < 25; i++) {
                        OFFSET[i] = chanal_save_when_reset_triger[i];
                    }
//                    qDebug() << "Auto-reset completed, offsets updated";
                });
            }
        });
    }
    else{
        qDebug()<<"open port fail" << serialPort->errorString();
        delete serialPort;
        serialPort = nullptr;
    }

    //if(plotmanager){
    //    plotmanager->resetCounterandClear();
    //}
}

void SerialPortCom::on_disconnect_clicked(){
    //if (plotmanager) {
    //    plotmanager->resetCounterandClear();
    //}

    for(int i = 0; i < 25; i++) {
        OFFSET[i] = 0.0;
        chanal_save_when_reset_triger[i] = 0.0;
    }

    for(int i = 0; i < 4; ++i) {
        windowBuffers[i].clear();
        windowSums[i] = 0.0;
    }

    if(serialPort){
        serialPort->close();                                                              // Close serial port
        delete serialPort;                                                                // Delete the pointer
        serialPort = nullptr;
    }
                                                                // Assign NULL to dangling pointer
    botton_openPort->setEnabled (true);
    botton_disConnect->setEnabled (0);
    botton_Reset->setEnabled(0);
    receivedData.clear();                                                             // Clear received string
}

void SerialPortCom::ReadData()
{
    if(serialPort->bytesAvailable()) {                                                    // Check if any bytes are available
        QByteArray data = serialPort->readAll();                                          // Read all data in QByteArray
        //QByteArray data = "$123.45 67.89 -12.34 5.67;";
        //qDebug()<<"data"<<data;

        if(!data.isEmpty()) {                                                             // If the byte array is not empty
            char *temp = data.data();                                                     // Get a '\0'-terminated char* to the data

            for(int i = 0; temp[i] != '\0'; i++) {                                        // Iterate over the char*
                switch(STATE) {                                                           // Switch the current state of the message
                case WAIT_START:                                                          // If waiting for start [$], examine each char
                    if(temp[i] == START_MSG) {                                            // If the char is $, change STATE to IN_MESSAGE
                        STATE = IN_MESSAGE;
                        receivedData.clear();                                             // Clear temporary QString that holds the message
                        break;                                                            // Break out of the switch
                    }
                    break;
                case IN_MESSAGE:                                                          // If state is IN_MESSAGE
                    if(temp[i] == END_MSG) {                                              // If char examined is ;, switch state to END_MSG
                        STATE = WAIT_START;
                        QStringList incomingData = receivedData.split(' ');               // Split string received from port and put it into list

                        //偏置数据
                        QStringList result;
                        for(int channel=0; channel<incomingData.size(); channel++) {
                            chanal_save_when_reset_triger[channel] = incomingData[channel].toDouble(); //更新值
                            result.push_back(QString::number(incomingData[channel].toDouble() - OFFSET[channel]));
                        }

                        if(result.size() == 4){
//                            for(int i=0; i<4; i++){
//                                *out << result[i] << "\t";
//                            }
//                            *out << "\n";

                            //if(plotmanager){
                            //    for(int i=0; i<4; i++){
                            //        plotmanager->addDataPoint(i, 0, result[i].toDouble());
                            //    }
                            //}

                            QStringList processedResult = processData(result);
                            QStringList forceData = calculateForces(processedResult);
                            emit newData(forceData);                                             // Emit signal for data received with the list
                            break;
                        }
                    }
                    else if (isdigit (temp[i]) || isspace (temp[i]) || temp[i] =='-' || temp[i] =='.')
                    {
                        /* If examined char is a digit, and not '$' or ';', append it to temporary string */
                        receivedData.append(temp[i]);
                    }
                    break;
                default: break;
                }
            }
        }
    }
}

void SerialPortCom::on_Refresh_clicked()
{
    comboPort->clear();
    /* List all available serial ports and populate ports combo box */
    for (QSerialPortInfo port : QSerialPortInfo::availablePorts()){
        comboPort->addItem (port.portName());
    }
}

void SerialPortCom::on_reset_clicked()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->write("R");
//        qDebug() << "Sent manual-reset command: R";

        QTimer::singleShot(1000, this, [this]() {
            for(int i = 0; i < 25; i++) {
                OFFSET[i] = chanal_save_when_reset_triger[i];
            }
//            qDebug() << "Manual-reset completed, offsets updated";
        });
    }
}

QStringList SerialPortCom::processData(const QStringList& originalData)
{
    QStringList processedResult;

    for(int i = 0; i < originalData.size(); i++) {
        double absValue = fabs(originalData[i].toDouble());

        QQueue<double>& buffer = windowBuffers[i];
        double& sum = windowSums[i];

        if(buffer.size() >= WINDOW_SIZE) {
            sum -= buffer.dequeue();
        }

        buffer.enqueue(absValue);
        sum += absValue;

        double movingAvg = buffer.isEmpty() ? 0.0 : sum / buffer.size();
        processedResult.push_back(QString::number(movingAvg));
    }

    return processedResult;
}

QStringList SerialPortCom::calculateForces(const QStringList& rawData)
{
    QStringList result;

    bool ok1 = false, ok2 = false, ok3 = false, ok4 = false;

    double V1 = rawData[0].toDouble(&ok1);
    double V2 = rawData[1].toDouble(&ok2);
    double V3 = rawData[2].toDouble(&ok3);
    double V4 = rawData[3].toDouble(&ok4);

    if (!ok1 || !ok2 || !ok3 || !ok4) {
        return QStringList{ "0", "0", "0"};
    }

    // ==================== 标定参数 ====================
    const double S = 9.24;   // 轴向传感器总灵敏度 (mV/N)成哥论文
    const double A = 6.4;    // 径径向力综合灵敏度系数 (mV/N)成哥论文
    const double kt = 2.0;   // 切向力比例系数 (N/mV)  未作实验：无
    const double e = 4.0;    // 刀具半径 (mm)可改
    const double c0 = 156.0;
    const double c1 = 40;   //刀具伸出长度 (mm)可改
    const double c = c0 + c1;   //力臂长度
    // ==================================================

    // 1. 轴向力和切向力
    double Fz = (V1 + V2 + V3) / S;
    double Ft = kt * V4;

    // 2. 提取交流分量
    double V0 = (V1 + V2 + V3) / 3.0;
    double V1p = V1 - V0;
    double V2p = V2 - V0;
    double V3p = V3 - V0;

    // 3. 转换为正交分量
    double Y = V1p;
    double X = (V2p - V3p) / std::sqrt(3.0);

    // 4. 计算中间变量K
    double K_sq = (X * X + Y * Y) / (A * A) - Ft * Ft;
    double K = (K_sq >= 0.0) ? std::sqrt(K_sq) : 0.0;

    // 5. 计算接触点角度
    double theta = 0.0;

    if (K != 0.0 || Ft != 0.0) {
        double theta_rad = std::atan2(Y * K + X * Ft,
            X * K - Y * Ft);

        theta = theta_rad * 180.0 / M_PI;

        while (theta < 0.0) {
            theta += 360.0;
        }

        while (theta >= 360.0) {
            theta -= 360.0;
        }
    }

    // 6. 真实径向力
    double P = K + (Fz * e) / c;

    //Ft：切向力  P：径向力  Fz：轴向力    
    result << QString::number(Ft, 'f', 4) << QString::number(P*100, 'f', 4) << QString::number(Fz*5, 'f', 4);
    return result;
}