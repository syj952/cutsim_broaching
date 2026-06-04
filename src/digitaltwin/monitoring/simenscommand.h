#ifndef SIMENSCOMMAND_H
#define SIMENSCOMMAND_H

#include <QByteArray>

class SimensCommand
{
public:
    SimensCommand();
    static const char FIRST_HAND_SHARK[];
    static const char SECOND_HAND_SHARK[];
    static const char THIRD_HAND_SHARK[];
    static const char READ_VER_INFO[];
    static const char CNC_ID[];
    static const char CNC_TYPE[];
    static const char CNC_SPINDLEACTSPEED[];//实际 主轴转速
    static const char CNC_SPINDLESETSPEED[];
    static const char CNC_SPINDRATE[];
    static const char CNC_FEEDRATE[];//进给倍率
    static const char CNC_FEEDSETSPEED[];
    static const char CNC_FEEDACTSPEED[];
    static const char POSFLAG[];
    static const char MACHINE_POS[];//机床坐标
    static const char DRIVER_CURRENT[];//实际电流
    static const char DRIVER_TEMPER[];//温度
    static const char DRIVER_LOAD1[];//电机功率
    static const char DRIVER_SPLOAD[];//负载
    static const char CURRENT_PRO[];//当前加工代码
    static const char PROGRAM_NAME[];//当前程序名
    static const char CNC_WSC_FRAME[];
    static const char CNC_WSC[];//G54
    static const char READ_R[];//R
    static const char WRITE_R[];
    static const char TOOLNAME[];//刀具名称
    static const char TOOLTABLEINFO[];
    static const char TOOLINDEX[];
    static const char TOOLFLAG[];

    QByteArray FirstHandShank;
    QByteArray SecondHandShank;
    QByteArray ThirdHandShank;
    QByteArray ReadVerInfo;//string
    QByteArray CNC_Id;//string
    QByteArray CNC_Type;//string
    QByteArray CNC_SpindleActSpeed;//double
    QByteArray CNC_SpindleSetSpeed;
    QByteArray CNC_SpindRate;
    QByteArray CNC_FeedRate;//double
    QByteArray CNC_FeedSetRate;//double
    QByteArray CNC_FeedActRate;
    QByteArray PosFlag;//double
    QByteArray MachinePos;//double
    QByteArray DriverCurrent;//float
    QByteArray DriverTemper;//float
    QByteArray DriverLoad1;//float
    QByteArray DriverSpload;//float
    QByteArray CurrentPro;//string
    QByteArray ProgramName;//string
    QByteArray CNC_WSC_Frame;//Int
    QByteArray CNC_Wsc;//double
    QByteArray Read_R;//double
    QByteArray Write_R;
    QByteArray ToolName;//string
    QByteArray ToolTableInfo;//double
    QByteArray ToolIndex;
    QByteArray ToolFlag;
};

#endif // SIMENSCOMMAND_H
