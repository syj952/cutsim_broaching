/**
* @file simensclient.h
* @brief Siemens CNC客户端类头文件
* 本文件定义了与西门子数控系统通信的客户端类 "simensclient"
* 该类通过TCP连接西门子数控系统，提供了获取数控设备状态、参数和控制数据的方法
* 内部包含数据解析功能，用于解析设备返回的二进制数据
* @author 李澈
* @version 1.0
* @date 2024-12-24
* @copyright
*
* @par 修改日志
* <table>
* <tr><th>Date        <th>Version  <th>Author    <th>Description
* <tr><td>2024/12/24  <td>1.0      <td>李澈       <td>初始版本
* </table>
*/

#ifndef SIMENSCLIENT_H
#define SIMENSCLIENT_H

#include <QTcpSocket>
#include "simenscommand.h"
#include <vector>
#include <cstring>
#include <QtEndian>
#include <QThread>
#include <QObject>

#include <QSqlError>
#include <QSqlDatabase>

/**
 * @class simensclient
 * @brief 与西门子CNC通信的客户端类
 * @details
 * - 通过TCP连接西门子CNC
 * - 提供获取数控设备状态、参数及控制数据的方法
 * - 内置数据解析功能，用于解析设备返回的二进制数据
 */
class simensclient{
public:
    /**
     * @brief 构造函数#include <QtEndian>
     * @param s IP地址
     * @param p 端口号
     */
    simensclient(QString s,int p);

    void Disconnect();

    /// @brief QTcpSocket 对象，用于与设备建立TCP连接
    QTcpSocket *socket;

    /// @brief 指向SimensCommand的指针，用于封装报文
    SimensCommand *simenscommand;

    QString ipStr ;
    int currentPort ;//quint16

    /// @brief 客户端连接状态
    bool connectState = 0;

    /**
     * @brief 获取机床版本、型号等等参数
     * @param result 用于存储返回的数据
     * @return 获取成功返回 true，失败返回 false
     */
    bool GetSimensVersion(QVariant &result);
    bool GetSimensCNCId(QVariant &result);
    bool GetSimensCNCType(QVariant &result);
    bool GetSpindleActSpeedRate (QVariant &result);
    bool GetSpindleSetSpeedRate (QVariant &result);
    bool GetSpindRate(QVariant &result);
    bool GetFeedRate(QVariant &result);
    bool GetMacPos(QVariant &result,double position[]);
    bool GetDriverCurrent(QVariant &result);
    bool GetDriverTemper(QVariant &result);
    bool GetDriverLoad1(QVariant &result);
    bool GetDriverSpload(QVariant &result);
    bool GetCurrentPro(QVariant &result);
    bool GetFeedSetRate(QVariant &result);
    bool GetFeedActRate(QVariant &result);
    bool GetProgramName(QVariant &result);
    bool GetMSCFrame(QVariant &result);
    bool GetMSC(QVariant &result,double MSC_pos[]);
    bool Read_R(QString index,QVariant &result);
    bool Write_R(int index,double value);
    bool GetToolIndex(QVariant &result);

    /**
     * @struct ToolInfo
     * @brief 刀具信息结构体
     * @details 用于存储刀具的各种属性信息
     */
    struct ToolInfo {
        QString  ToolName;
        double TypeId;
        double Length;
        double Radius;
        double CornerRadius;
        double Length_5;
        double Angle_2;
        double WearLength;
        double WearRadius;
        double WearCornerRadius;
        double WearLength_5;
        double WearAngle_2;
        double ClearanceAngle;
        double Teeth;
    };

    /**
     * @brief 获取刀具信息
     * @return 返回刀具信息
     */
    std::vector<ToolInfo> GetToolData();

    void onDisconnected(){
        qDebug() << "Disconnected!!!";
    }

private:

    bool Reconnect();

    /**
     * @brief 执行TCP握手操作
     * @details 用于建立与CNC的连接
     */
    void HandShark();

    /**
     * @brief 连接MySQL数据库
     */
    void MysqlCon();

    /**
     * @brief 发送命令并获取返回数据
     * @param byteMessage 待发送的报文
     * @param result 用于存储返回的原始数据
     * @return 获取成功返回 true，失败返回 false
     */
    bool GetData(QByteArray byteMessage, QByteArray &result);

    /**
     * @brief 解析设备返回的字符串数据
     * @param byteMessage 原始数据
     * @return 返回解析后的字符串
     */
    QString AnalysisStrData(QByteArray byteMessage);
    double AnalysisDoubleData(QByteArray byteMessage);
    double AnalysisDoubleData2(QByteArray byteMessage);
    float AnalysisFloatData(QByteArray byteMessage);
    qint32 AnalysisInt32Data(QByteArray byteMessage);
    QVariant gettooldata(QByteArray command, int index, unsigned char toolFlag, QByteArray ReceiveData);

};

#endif // SIMENSCLIENT_H
