#include "heidenhainclient.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

CncInfo_HeiDenHainCommands::CncInfo_HeiDenHainCommands(QString s,int p):ipStr(s),currentPort(p)
{
    socket = new QTcpSocket();
    hdhcommands = new ETH_HeiDenHainCommands();
    if(!connectState)
    {
        socket->connectToHost(s, p);
        if(socket->waitForConnected(1000))
        {
            qDebug() <<  "HDH connect succeeed" ;
            connectState = true;
            Handlew();
        }
        else
             qDebug() << "HDH connect false"  ;
    }
}

void CncInfo_HeiDenHainCommands::Handlew()
{
    socket->write(hdhcommands->A_lginspect);
    if (socket->waitForReadyRead(500)) {
        QByteArray arr = socket->readAll();
    }
    socket->write(hdhcommands->C_cc_07);
    if (socket->waitForReadyRead(500)) {
        QByteArray arr = socket->readAll();
    }
    socket->write(hdhcommands->R_pr);
    if (socket->waitForReadyRead(500)) {
        QByteArray arr = socket->readAll();
    }
    socket->write(hdhcommands->A_loinspect);
    if (socket->waitForReadyRead(500)) {
        QByteArray arr = socket->readAll();
    }
    socket->write(hdhcommands->A_lginspect);
    if (socket->waitForReadyRead(500)) {
        QByteArray arr = socket->readAll();
    }
    socket->write(hdhcommands->A_lgdnc);
    if (socket->waitForReadyRead(500)) {
        QByteArray arr = socket->readAll();
    }

}

bool CncInfo_HeiDenHainCommands::GetData(QByteArray byteMessage, QByteArray &result)
{
    if(socket->isOpen() && socket->isValid())
        this->socket->write(byteMessage);

    if (socket->waitForReadyRead(500)) {
        result= socket->readAll();
//        qDebug()<<"result: "<<result;
        telegram.Length = (static_cast<unsigned char>(result[0]) << 24) |
                          (static_cast<unsigned char>(result[1]) << 16) |
                          (static_cast<unsigned char>(result[2]) << 8) |
                          static_cast<unsigned char>(result[3]);
        telegram.Control = QString::fromLatin1(result.mid(4, 4));
        telegram.Message = result.mid(result.size() - telegram.Length, telegram.Length);
        return true;
    }
    else{
        qDebug()<<"receive timeout";
        if(Reconnect()){
            if(socket->isOpen() && socket->isValid())
                this->socket->write(byteMessage);
            if (socket->waitForReadyRead(500)) {
                result= socket->readAll();
                telegram.Length = (static_cast<unsigned char>(result[0]) << 24) |
                                  (static_cast<unsigned char>(result[1]) << 16) |
                                  (static_cast<unsigned char>(result[2]) << 8) |
                                  static_cast<unsigned char>(result[3]);
                telegram.Control = QString::fromLatin1(result.mid(4, 4));
                telegram.Message = result.mid(result.size() - telegram.Length, telegram.Length);
                return true;
            }
            else
                qDebug()<<"reconnect succeed，but also receive timeout";
        }
    }
    return false;
}

bool CncInfo_HeiDenHainCommands::Reconnect(){
    int retryCount = 10;
    for (int i=0; i<retryCount; ++i)
    {
        if(socket->state() == QAbstractSocket::ConnectedState)
            socket->disconnectFromHost();

        socket->connectToHost(ipStr, currentPort);
        if(socket->waitForConnected(500))
        {
            connectState = true;
            Handlew();
            qDebug()<<"reconnect succeed";
            return 1;
        }
    }

    qDebug() << "reconnect false";
    connectState = false;
    return 0;
}

void CncInfo_HeiDenHainCommands::Disconnect(){
    if(socket && socket->state()==QAbstractSocket::ConnectedState){
        socket->disconnectFromHost();
        if(socket->waitForDisconnected(1000)){
            qDebug()<<"disconnect succeed";
        }else qDebug()<<"disconnect false";
        connectState = 0;
    }
}

//***************数据采集××××××××××××××××××××××××
bool CncInfo_HeiDenHainCommands::GetNCTYpe(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(hdhcommands->GetNcVer,ReceiveData);
    result = QVariant(QString::fromLatin1(telegram.Message).remove(QChar('\0')));
    return 1;
}

bool CncInfo_HeiDenHainCommands::GetMtPos(QVariant &result, double position[])
{
    QByteArray ReceiveData;
    GetData(hdhcommands->GetMtPos,ReceiveData);

    int AxisCount = static_cast<unsigned char>(telegram.Message[0]) << 8 |
                    static_cast<unsigned char>(telegram.Message[1]);
    QByteArray data = telegram.Message.mid(12, telegram.Length - 12);
    //qDebug() << "data" << data;

     QList<QByteArray> parts = data.split('\x00');
     for (int i = 0; i < AxisCount && i < parts.size(); ++i) {
         position[i] = parts[i].toDouble();
     }

    result = data;
    return 1;
}

bool CncInfo_HeiDenHainCommands::GetCuPos(QVariant &result, double position[])
{
    QByteArray ReceiveData;
    GetData(hdhcommands->GetCuPos,ReceiveData);

    int AxisCount = static_cast<unsigned char>(telegram.Message[0]) << 8 |
                    static_cast<unsigned char>(telegram.Message[1]);
    QByteArray data = telegram.Message.mid(10, telegram.Length - 10);
    //qDebug() << "data" << data;

     QList<QByteArray> parts = data.split('\x00');
     for (int i = 0; i < AxisCount && i < parts.size(); ++i) {
         position[i] = parts[i].toDouble();
     }

    result = data;
    return 1;
}

bool CncInfo_HeiDenHainCommands::GetMultiplier(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(hdhcommands->A_lgdnc,ReceiveData);
    GetData(hdhcommands->GetFeed,ReceiveData);
    result = ReceiveData;
    double feed = (std::stoul(telegram.Message.left(4).toHex().toStdString(), nullptr, 16)) / 100;
    double speed = (std::stoul(telegram.Message.mid(4, 4).toHex().toStdString(), nullptr, 16)) / 100;
    double rapid = (std::stoul(telegram.Message.mid(8, 4).toHex().toStdString(), nullptr, 16)) / 100;
    qDebug()<<"feed multiplier: "<<feed;
    qDebug()<<"speed multiplier: "<<speed;
    qDebug()<<"rapid multiplier: "<<rapid;

    return 1;
}

bool CncInfo_HeiDenHainCommands::GetSpindleSpeed(QVariant &result){
    if(GetPlc("R_MB", "D368", "int", result))
        return 1;
    else
        return 0;
}

bool CncInfo_HeiDenHainCommands::GetFeed(QVariant &result){
    if(GetPlc("R_MB", "D388", "int", result))
        return 1;
    else
        return 0;
}

bool CncInfo_HeiDenHainCommands::GetToolIndex(QVariant &result){
    QByteArray ReceiveData;
    GetData(hdhcommands->GetToolIndex,ReceiveData);
    result = QVariant(QString(QChar(telegram.Message[0])));
    return 1;
}

CncInfo_HeiDenHainCommands::ToolInfo CncInfo_HeiDenHainCommands::GetCurrentToolData(){
    QByteArray ReceiveData;
    GetData(hdhcommands->GetCuToolInfo,ReceiveData);
    std::vector<uint8_t> message(telegram.Message.begin(), telegram.Message.end());

    //std::string axis[] = { "X", "Y", "Z" };
    ToolInfo tools;
    tools.ToolIndex = *reinterpret_cast<uint16_t*>(&message[0]);
    tools.ToolId = *reinterpret_cast<uint16_t*>(&message[4]);
    tools.AxisNum = *reinterpret_cast<uint16_t*>(&message[6]);
    tools.Length = *reinterpret_cast<double*>(&message[8]);
    tools.Radius = *reinterpret_cast<double*>(&message[16]);

    return tools;
}

QString CncInfo_HeiDenHainCommands::byteToHexStr(const QByteArray& bytes) {
    QString hexStr;
    for (const auto& byte : bytes) {
        hexStr.append(QString("%1").arg(static_cast<quint8>(byte), 2, 16, QChar('0')));
    }
    return hexStr;
}

QByteArray CncInfo_HeiDenHainCommands::PlcAdd2Byte(QString RequireType, QString PlcAdd){
    QByteArray RequireByte = RequireType.toLatin1();

    // Determine the PLC address type based on the alphabetic part of PlcAdd
    QByteArray PlcAddType(4, 0);
    QRegExp regex("[^0-9]");
    QString plcType = PlcAdd;
    plcType.remove(QRegExp("[0-9]"));

    if (plcType == "D") {
        qToLittleEndian<quint32>(4, reinterpret_cast<uchar*>(PlcAddType.data()));
    } else if (plcType == "W") {
        qToLittleEndian<quint32>(2, reinterpret_cast<uchar*>(PlcAddType.data()));
    } else if (plcType == "M") {
        qToLittleEndian<quint32>(1, reinterpret_cast<uchar*>(PlcAddType.data()));
    }

    // Extract the numeric part of PlcAdd and convert to bytes
    QString plcNumStr = PlcAdd;
    plcNumStr.remove(QRegExp("[^0-9]"));
    quint32 plcNum = plcNumStr.toUInt() + 64;
    QByteArray PlcAddNum(sizeof(quint32), 0);
    qToBigEndian<quint32>(plcNum, reinterpret_cast<uchar*>(PlcAddNum.data()));

    // Calculate the message length
    quint32 messageLength = 1 + PlcAddNum.size();
    QByteArray MessageLength(sizeof(quint32), 0);
    qToBigEndian<quint32>(messageLength, reinterpret_cast<uchar*>(MessageLength.data()));

    // Combine all parts into the final byte array
    QByteArray b;
    b.append(MessageLength);
    b.append(RequireByte);
    b.append(PlcAddNum);
    b.append(PlcAddType[0]);

    return b;
}

bool CncInfo_HeiDenHainCommands::logout(){
    QByteArray ReceiveData;
    GetData(hdhcommands->A_loplcdebug,ReceiveData);//Logout from PLC path
    if (telegram.Control == "T_ER") {
        qDebug()<<"logout false";
        return false;
    }else
        return true;
}

bool CncInfo_HeiDenHainCommands::login(){
    QByteArray ReceiveData;
    GetData(hdhcommands->A_lgplcdebug,ReceiveData);//Login to PLC path
    if (telegram.Control == "T_ER") {
        qDebug()<<"login false";
        return false;
    }else
        return true;
}

bool CncInfo_HeiDenHainCommands::GetPlc(QString address1, QString address2, QString type, QVariant &result) {
    try {
        QByteArray ReceiveData;
//        GetData(hdhcommands->A_lgplcdebug,ReceiveData);//Login to PLC path
//        if (telegram.Control == "T_ER") {
//            qDebug()<<"login false";
//            return false;
//        }
        GetData(hdhcommands->R_pr,ReceiveData);
        if(GetData(PlcAdd2Byte(address1, address2),ReceiveData))

        if (type == "int") {
            if (address2.at(0) == 'W') {
                QString str = byteToHexStr(telegram.Message);
                bool ok;
                result = (str.mid(2,2) + str.mid(0,2)).toInt(&ok, 16);//将字符串拼接后转为十六进制的整数
            } else {
//                result = qFromLittleEndian<int32_t>(reinterpret_cast<const uchar*>(telegram.Message.constData()));
                if (telegram.Message.size() >= 4) {
                    // 直接读取4字节数据作为32位整数（与C# BitConverter.ToInt32行为一致）
//                    int32_t value = *reinterpret_cast<const int32_t*>(telegram.Message.constData());
//                    result = value;
                    int32_t value = static_cast<int32_t>(
                                (static_cast<uint8_t>(telegram.Message[0]) << 0) |
                            (static_cast<uint8_t>(telegram.Message[1]) << 8) |
                            (static_cast<uint8_t>(telegram.Message[2]) << 16) |
                            (static_cast<uint8_t>(telegram.Message[3]) << 24)
                            );
                    result = value;
                } else {
                    qDebug()<<"telegram.Message.size() < 4";
                    result = -1;
                    return false;
                }
            }
        } else if (type == "string") {
            result = QString::fromUtf8(telegram.Message);
        } else if (type == "bool") {
            result = static_cast<bool>(telegram.Message[0]);
        }

//        GetData(hdhcommands->A_loplcdebug,ReceiveData);//Logout from PLC path
        return true;
    } catch (...) {
        return false;
    }
}
