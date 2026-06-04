#ifndef CNCINFO_HEIDENHAINCOMMANDS_H
#define CNCINFO_HEIDENHAINCOMMANDS_H

#include "heidenhaincommand.h"
#include <QTcpSocket>
#include <thread>
#include <QtEndian>

/**
 * @class CncInfo_HeiDenHainCommands
 * @brief 与海德汉CNC通信的客户端类
 * @details
 * - 通过TCP连接海德汉CNC
 * - 提供获取数控参数的方法
 */
class CncInfo_HeiDenHainCommands
{
public:
    CncInfo_HeiDenHainCommands(QString s,int p);
    QString ipStr ;
    int currentPort ;
    QTcpSocket *socket;
    ETH_HeiDenHainCommands *hdhcommands;

    struct ToolInfo {
        int ToolIndex;      ///< 当前刀具号
        int ToolId;         ///< 刀具编号
        int AxisNum;        ///< 轴号(0/1/2：X/Y/Z)
        double Length;      ///< 刀具长度
        double Radius;      ///< 刀具半径
    };
    bool GetNCTYpe(QVariant &result);                   ///< 获取机床版本信息
    bool GetMtPos(QVariant &result,double position[]);  ///< 获取机械位置:mm
    bool GetCuPos(QVariant &result,double position[]);  ///< 获取相对位置:mm
    bool GetMultiplier(QVariant &result);               ///< 获取进给、转速倍率:%
    bool GetSpindleSpeed(QVariant &result);             ///< 获取主轴转速:rpm
    bool GetFeed(QVariant &result);                     ///< 获取进给速率:mm/min
    bool GetToolIndex(QVariant &result);                ///< 获取刀具号
    ToolInfo GetCurrentToolData();                      ///< 获取当前刀具信息(ToolInfo)

    struct Telegram {
        QString T_OK;         ///< 状态字段 OK
        QString T_ER;         ///< 状态字段 ER
        int Length;           ///< 电报长度
        QString Control;      ///< 电报控制符
        QByteArray Message;   ///< 电报报文（数据部分）
    };
    Telegram telegram;

private:
//    bool connectState = 0;
    void Handlew();
    bool Reconnect();
    bool GetData(QByteArray byteMessage, QByteArray &result);
    bool GetPlc(QString address1, QString address2, QString type, QVariant &result);    ///< 获取指定PLC信息
    QString byteToHexStr(const QByteArray& bytes);
    QByteArray PlcAdd2Byte(QString RequireType, QString PlcAdd);
public:
    bool connectState = 0;
    void Disconnect();
    bool logout();
    bool login();
};

#endif // CNCINFO_HEIDENHAINCOMMANDS_H

