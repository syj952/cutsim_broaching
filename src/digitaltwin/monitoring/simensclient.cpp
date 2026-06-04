#include "simensclient.h"

/**
 * @brief 构造函数
 * @details
 * 初始化西门子CNC客户端，创建QTcpSocket对象并建立TCP连接
 * @param s IP
 * @param p 端口号
 */
simensclient::simensclient(QString s,int p):ipStr(s),currentPort(p)
{
    socket = new QTcpSocket();
    QObject::connect(socket, &QTcpSocket::disconnected, [this]() {
        onDisconnected();
    });
    simenscommand = new SimensCommand();
    if(!connectState)
    {
        socket->connectToHost(ipStr, currentPort);
        if(socket->waitForConnected(2000))
        {
            qDebug() <<  "连接成功" ;
            connectState = true;
        }
        else{
             qDebug() << "连接失败"  ;
             exit(1);
        }
    }
    HandShark();
    //MysqlCon();
}

void simensclient::MysqlCon()
{
    QSqlDatabase db( QSqlDatabase::addDatabase( "QMYSQL" ) );
    db.setHostName("192.168.101.62");
    db.setDatabaseName("SiemensData");
    db.setUserName("root");
    db.setPassword("080509");
    db.setPort(3306);
    if(!db.open())
    {
       qDebug()<<db.lastError()<<endl;
    }
    else
    {
       qDebug()<<"数据库连接成功"<<endl;
    }
}

void simensclient::HandShark()
{
    socket->write(simenscommand->FirstHandShank);
    //qDebug()<<"第一次握手："<<simenscommand->FirstHandShank.toHex();
    if (socket->waitForReadyRead(500)) {
        QByteArray arr = socket->readAll();
        //qDebug() <<"接收信息： "<<arr.toHex();
    }
    else
    {
        //qDebug() <<"第一次握手:接收信息超时";
    }
    socket->write(simenscommand->SecondHandShank);
    //qDebug() <<"第二次握手： "<<simenscommand->SecondHandShank.toHex();
    if (socket->waitForReadyRead(100)) {
        QByteArray arr = socket->readAll();
        //qDebug() <<"接收信息： "<<arr.toHex();
    }
    else
    {
        //qDebug() <<"第二次握手:接收信息超时";
    }
    socket->write(simenscommand-> ThirdHandShank);
    //qDebug() <<"第三次握手： "<<simenscommand->ThirdHandShank.toHex();
    if (socket->waitForReadyRead(100)) {
        QByteArray arr = socket->readAll();
        //qDebug() <<"接收信息： "<<arr.toHex();
    }
    else
    {
        //qDebug() <<"第三次握手:接收信息超时";
    }
}

bool simensclient::GetData(QByteArray byteMessage, QByteArray &result)
{
    if(socket->isOpen() && socket->isValid())
        this->socket->write(byteMessage);

    if (socket->waitForReadyRead(1000)) {
        result= socket->readAll();
        //qDebug()<<result;
        return true;
    }else{
        qDebug()<<"接收超时啦";
        if(Reconnect()){
            if(socket->isOpen() && socket->isValid())
                this->socket->write(byteMessage);
            if (socket->waitForReadyRead(500)){
                result= socket->readAll();
                return true;
            }
            else
                qDebug()<<"重连成功后发送请求，接收数据仍超时";
        }
    }
    return false;
}

bool simensclient::Reconnect(){
    int retryCount = 10;
    for (int i=0; i<retryCount; ++i)
    {
        if(socket->state() == QAbstractSocket::ConnectedState)
            socket->disconnectFromHost();

        socket->connectToHost(ipStr, currentPort);
        if(socket->waitForConnected(500))
        {
            connectState = true;
            HandShark();
            qDebug()<<"重连成功";
            return 1;
        }
    }

    qDebug() << "重连失败";
    connectState = false;
    return 0;
}

//***************数据解析××××××××××××××××××××××××

//(1)从第25个字节开始提取数据，并返回剩余部分。剩余部分转换为QString
QString simensclient::AnalysisStrData(QByteArray byteMessage)
{
    return QString(byteMessage.mid(25));
}
//(2)第4个字节的值是否为33。转换为double
double simensclient::AnalysisDoubleData(QByteArray byteMessage)
{
    double buffer[2];//8字节双精度浮点数
    if (byteMessage.at(3) == 33)
    {
        memcpy(buffer,byteMessage.mid(25,8),8);//第25个字节开始提取长度为8字节的数据
        return buffer[0];
    }
    else
    {
       return byteMessage.toDouble();
    }
}

double simensclient::AnalysisDoubleData2(QByteArray byteMessage)
{
    double value = 0;
    return value;
}
//(3)提取 反转 转换为float
float simensclient::AnalysisFloatData(QByteArray byteMessage)
{
    float buffer;
    char *data = byteMessage.mid(25, 4).data(); // 从第25个字节开始提取长度为4字节的数据
    // 将四个字节的数据反转
    char temp = data[0];
    data[0] = data[3];
    data[3] = temp;
    temp = data[1];
    data[1] = data[2];
    data[2] = temp;
    // 将反转后的数据拷贝到buffer中
    memcpy(&buffer, data, 4);
    return buffer;
}
//(4)
qint32 simensclient::AnalysisInt32Data(QByteArray byteMessage)
{
    QByteArray int16Data = byteMessage.mid(25, 2);
    // 将2字节转换为小端序的16位整型（大端序：qFromBigEndian）
    qint16 value = qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(int16Data.constData()));
    return static_cast<qint32>(value);
}


//***************数据采集××××××××××××××××××××××××

bool simensclient::GetSimensVersion(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->ReadVerInfo,ReceiveData);
    result = AnalysisStrData(ReceiveData);
    return true;
}

bool simensclient::GetSimensCNCId(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_Id,ReceiveData);
    result = AnalysisStrData(ReceiveData);
    return true;
}

bool simensclient::GetSimensCNCType(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_Type,ReceiveData);
    result = AnalysisStrData(ReceiveData);
    return true;
}

bool simensclient::GetSpindleActSpeedRate(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_SpindleActSpeed,ReceiveData);
    result = AnalysisDoubleData(ReceiveData);
    return true;
}

bool simensclient::GetFeedRate(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_FeedRate,ReceiveData);
    result = AnalysisDoubleData(ReceiveData);
    return true;
}

bool simensclient::GetMacPos(QVariant &result,double positions[])
{
    QByteArray reveivedata;
    int i=0;
    int j=0;
    double ret=0.0;
    QString retlist;
    while(i<simenscommand->PosFlag.size()){
        simenscommand->MachinePos[26]=simenscommand->PosFlag.at(i);
        GetData(simenscommand->MachinePos,reveivedata);
        ret = AnalysisDoubleData(reveivedata);
        positions[j] = ret;
        retlist = retlist+QString::number(ret)+'/';
        i++;
        j++;
    }
    result = retlist;
    return true;
}

bool simensclient::GetDriverCurrent(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->DriverCurrent,ReceiveData);
    result = AnalysisFloatData(ReceiveData);
    return true;
}

bool simensclient::GetDriverTemper(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->DriverTemper,ReceiveData);
    result = AnalysisFloatData(ReceiveData);
    return true;
}

bool simensclient::GetDriverLoad1(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->DriverLoad1,ReceiveData);
    result = AnalysisFloatData(ReceiveData);
    return true;
}

bool simensclient::GetDriverSpload(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->DriverSpload,ReceiveData);
    result = AnalysisFloatData(ReceiveData);
    return true;
}

bool simensclient::GetCurrentPro(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CurrentPro,ReceiveData);
    result = AnalysisStrData(ReceiveData);
    return true;
}

bool simensclient::GetFeedSetRate(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_FeedSetRate,ReceiveData);
    result = AnalysisDoubleData(ReceiveData);
    return true;
}

bool simensclient::GetFeedActRate(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_FeedActRate,ReceiveData);
    result = AnalysisDoubleData(ReceiveData);
    return true;
}

bool simensclient::GetSpindleSetSpeedRate(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_SpindleSetSpeed,ReceiveData);
    result = AnalysisDoubleData(ReceiveData);
    return true;
}

bool simensclient::GetSpindRate(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->CNC_SpindRate,ReceiveData);
    result = AnalysisDoubleData(ReceiveData);
    return true;
}

bool simensclient::GetProgramName(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->ProgramName,ReceiveData);
    result = AnalysisStrData(ReceiveData);
    return true;
}

bool simensclient::GetMSCFrame(QVariant &result)
{
    //QByteArray ReceiveData;
    //GetData(simenscommand->CNC_WSC_Frame,ReceiveData);
    QByteArray ReceiveData;
    if (!GetData(simenscommand->CNC_WSC_Frame, ReceiveData)) {
        qWarning() << "Failed to get data for CNC_WSC_Frame";
        return false;
    }
    result = AnalysisInt32Data(ReceiveData);
    return true;
}

bool simensclient::GetMSC(QVariant &result,double MSC_pos[])
{
    QVariant a;
        if (!GetMSCFrame(a) || !a.isValid()) {
            qWarning() << "GetMSCFrame failed or returned invalid data";
            return false;
        }
    int Frame;
    GetMSCFrame(a);
    Frame = a.toInt();

    if(Frame != 0){
        QByteArray reveivedata;
        int i=0;
        int j=0;
        double ret=0.0;
        QString retlist;
        while(i<simenscommand->PosFlag.size()){
            simenscommand->CNC_Wsc[26] = simenscommand->PosFlag.at(i) + 6*Frame;
            GetData(simenscommand->CNC_Wsc,reveivedata);
            ret = AnalysisDoubleData(reveivedata);
            MSC_pos[j] = ret;
            retlist = retlist+QString::number(ret)+'/';
            i++;
            j++;
        }
        result = retlist;//not used
    }
    return true;
}

bool simensclient::Read_R(QString index,QVariant &result)
{
    QByteArray ReceiveData;
    if (index.toInt() > 254) {
            simenscommand->Read_R[25] = static_cast<unsigned char>(((index.toInt() + 1) >> 8) & 0xFF); // 高字节
            simenscommand->Read_R[26] = static_cast<unsigned char>((index.toInt() + 1) & 0xFF);        // 低字节
        } else {
            simenscommand->Read_R[25] = 0; // 高字节清零
            simenscommand->Read_R[26] = static_cast<unsigned char>((index.toInt() + 1));
        }
    //qDebug()<<simenscommand->Read_R.toHex();
    GetData(simenscommand->Read_R,ReceiveData);
    result = AnalysisDoubleData(ReceiveData);
    return true;
}

bool simensclient::Write_R(int index, double value)
{
    QByteArray ReceiveData;
    if (index > 254) {
            simenscommand->Write_R[25] = static_cast<unsigned char>(((index + 1) >> 8) & 0xFF);
            simenscommand->Write_R[26] = static_cast<unsigned char>((index + 1) & 0xFF);
        } else {
            simenscommand->Write_R[25] = 0;
            simenscommand->Write_R[26] = static_cast<unsigned char>((index + 1));
        }

    QByteArray aa(reinterpret_cast<const char*>(&value), sizeof(value));
    simenscommand->Write_R.replace(simenscommand->Write_R.size() - 8, 8, aa);

    GetData(simenscommand->Write_R,ReceiveData);
    if(ReceiveData[21] == static_cast<char>(0xFF))
        return true;
    else
        return false;
}

std::vector<simensclient::ToolInfo> simensclient::GetToolData()
{
    QByteArray ReceiveData;
    std::vector<ToolInfo> tools;
    int count = 0;
    for (int i = 1;; i++)
    {
        // 刀具名
        simenscommand->ToolName[26] = static_cast<unsigned char>(i);
        GetData(simenscommand->ToolName,ReceiveData);
        QVariant ToolName;
        ToolName = AnalysisStrData(ReceiveData);

        // 数控系统中删除某行刀具信息，将会读到空信息；
        // 仅当连续15行空白信息时，才会停止读取.
        if (count == 15)
        {
            break;
        }
        if (ReceiveData[21] != static_cast<char>(0xFF)) {
            count++;
            tools.push_back({"", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
            continue;
        }
        else {
            count = 0;
        }

        QVariant TypeId;
        QVariant Length, WearLength;
        QVariant Radius, WearRadius;
        QVariant CornerRadius, WearCornerRadius;
        QVariant Length_5, WearLength_5;
        QVariant Angle_2, WearAngle_2;
        QVariant ClearanceAngle;
        QVariant Teeth;

        TypeId = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(0), ReceiveData);

        Length = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(1), ReceiveData);
        CornerRadius = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(2), ReceiveData);///< xiugaile
        Radius = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(3), ReceiveData);
        Length_5 = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(4), ReceiveData);
        Angle_2 = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(5), ReceiveData);

        WearLength = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(6), ReceiveData);
        WearRadius = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(7), ReceiveData);
        WearCornerRadius = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(8), ReceiveData);
        WearLength_5 = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(9), ReceiveData);
        WearAngle_2 = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(10), ReceiveData);

        ClearanceAngle = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(11), ReceiveData);
        Teeth = gettooldata(simenscommand->ToolTableInfo, i, simenscommand->ToolFlag.at(12), ReceiveData);

        tools.push_back({ToolName.toString(),
                         TypeId.toDouble(),
                         Length.toDouble(),
                         Radius.toDouble(),
                         CornerRadius.toDouble(),
                         Length_5.toDouble(),
                         Angle_2.toDouble(),
                         WearLength.toDouble(),
                         WearRadius.toDouble(),
                         WearCornerRadius.toDouble(),
                         WearLength_5.toDouble(),
                         WearAngle_2.toDouble(),
                         ClearanceAngle.toDouble(),
                         Teeth.toDouble()
                        });

        //qDebug()<<ToolName.toString()<<TypeId.toDouble()<<"L"<<Length.toDouble()<<"R"<<Radius.toDouble()<<"CR"<<CornerRadius.toDouble()<<"L5"<<Length_5.toDouble()<<"A2"<<Angle_2.toDouble()<<"WL"<<WearLength.toDouble()<<"WR"<<WearRadius.toDouble()<<"WCR"<<WearCornerRadius.toDouble()<<"WL5"<<WearLength_5.toDouble()<<"WA"<<WearAngle_2.toDouble()<<"renyi"<<ClearanceAngle.toDouble()<<"T"<<Teeth.toDouble();
        qDebug()<<ToolName.toString()<<TypeId.toDouble()<<Length.toDouble()<<Radius.toDouble()<<CornerRadius.toDouble()<<Length_5.toDouble()<<Angle_2.toDouble()<<WearLength.toDouble()<<WearRadius.toDouble()<<WearCornerRadius.toDouble()<<WearLength_5.toDouble()<<WearAngle_2.toDouble()<<ClearanceAngle.toDouble()<<Teeth.toDouble();
    }
    return tools;
}

QVariant simensclient::gettooldata(QByteArray command, int index, unsigned char toolFlag, QByteArray ReceiveData) {
    command[24] = static_cast<unsigned char>(index);
    command[26] = toolFlag;
    GetData(command, ReceiveData);
    return AnalysisDoubleData(ReceiveData);
}

bool simensclient::GetToolIndex(QVariant &result)
{
    QByteArray ReceiveData;
    GetData(simenscommand->ToolIndex,ReceiveData);
    result = AnalysisInt32Data(ReceiveData);
    return true;
}

void simensclient::Disconnect()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
        if (socket->state() == QAbstractSocket::UnconnectedState || socket->waitForDisconnected()) {
            qDebug() << "成功断开连接";
        } else {
            qDebug() << "断开连接失败";
        }
    }
    else {
        qDebug() << "当前没有连接";
    }
}
