#include "simenscommand.h"

const char SimensCommand::FIRST_HAND_SHARK[] ={ '\x03', '\x00', '\x00', '\x16', '\x11', '\xe0', '\x00', '\x00',\
                                                   '\x00', '\x48', '\x00', '\xc1', '\x02', '\x04', '\x00', '\xc2', \
                                                    '\x02', '\x0d', '\x04', '\xc0', '\x01', '\x0a' };
const char SimensCommand::SECOND_HAND_SHARK[] ={ '\x03', '\x00', '\x00', '\x19', '\x02', '\xf0', '\x80', '\x32', \
                                                '\x01', '\x00', '\x00', '\x00', '\x01', '\x00', '\x08', '\x00', \
                                                 '\x00', '\xf0', '\x00', '\x00', '\x64', '\x00', '\x64', '\x03', '\xc0' };
const char SimensCommand::THIRD_HAND_SHARK[] ={'\x03', '\x00', '\x00', '\x1d', '\x02', '\xf0', '\x80', '\x32',\
                                                '\x01', '\x00', '\x00', '\x00', '\x01', '\x00', '\x0c', '\x00', \
                                                '\x00', '\x04', '\x01', '\x12', '\x08', '\x82', '\x01', '\x00', \
                                                 '\x14', '\x00', '\x01', '\x3b', '\x01', '\x03', '\x00', '\x00', \
                                                  '\x07', '\x02', '\xf0', '\x00' };
const char SimensCommand::READ_VER_INFO[] ={'\x03', '\x00', '\x00', '\x1d', '\x02', '\xf0', '\x80', '\x32', '\x01',\
                                             '\x00', '\x00', '\x00', '\x14', '\x00', '\x0c','\x00', '\x00','\x04', \
                                             '\x01','\x12', '\x08', '\x82', '\x01', '\x46', '\x78','\x00','\x01',\
                                             '\x1a', '\x01','\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CNC_ID[] = {'\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                      '\x02', '\xf0', '\x80', '\x32', '\x01',
                                      '\x00', '\x00', '\x00', '\x14',
                                      '\x00', '\x0c',  //10+2
                                      '\x00', '\x00',
                                      '\x04',
                                      '\x01',
                                      '\x12', '\x08', '\x82', '\x01', '\x46', '\x6e', '\x00', '\x01', '\x1a', '\x01',
                                      '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CNC_TYPE[] = {'\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                        '\x02', '\xf0', '\x80', '\x32', '\x01',
                                        '\x00', '\x00', '\x00', '\x14',
                                        '\x00', '\x0c',  //10+2
                                        '\x00', '\x00',
                                        '\x04',
                                        '\x01',
                                        '\x12', '\x08', '\x82', '\x01', '\x46', '\x78', '\x00', '\x04', '\x1a', '\x01',
                                        '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CNC_SPINDLEACTSPEED[] ={'\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                  '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                  '\x00', '\x00', '\x00', '\x14',
                                                  '\x00', '\x0c',  //10+2
                                                  '\x00', '\x00',
                                                  '\x04',
                                                  '\x01',
                                                  '\x12', '\x08', '\x82', '\x41', '\x00', '\x02', '\x00', '\x01', '\x72', '\x01',
                                                  '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CNC_FEEDRATE[] =  {   '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                          '\x02', '\xf0', '\x80', '\x32', '\x01',
                                          '\x00', '\x00', '\x00', '\x13',
                                          '\x00', '\x0c',  //10+2
                                          '\x00', '\x00',
                                          '\x04',
                                          '\x01',
                                          '\x12', '\x08', '\x82', '\x41', '\x00', '\x03', '\x00', '\x01', '\x7f', '\x01',
                                          '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::MACHINE_POS[] =  { '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                        '\x02', '\xf0', '\x80', '\x32', '\x01',
                                        '\x00', '\x00', '\x00', '\x14',
                                        '\x00', '\x0c',  //10+2
                                        '\x00', '\x00',
                                        '\x04',
                                        '\x01',                                                    //26 01 02 03
                                        '\x12', '\x08', '\x82', '\x41', '\x00', '\x02', '\x00',  '\x01', '\x74', '\x01',
                                        '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::POSFLAG[] =   { '\x01', '\x02', '\x03', '\x05', '\x06', '\x04'};//X/Y/Z/A/C/SP
const char SimensCommand::DRIVER_CURRENT[] =   {   '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                   '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                   '\x00', '\x00', '\x00', '\x14',
                                                   '\x00', '\x0c',  //10+2
                                                   '\x00', '\x00',
                                                   '\x04',
                                                   '\x01',
                                                   '\x12', '\x08', '\x82', '\xa1', '\x00', '\x1e', '\x00', '\x01', '\x82', '\x01',
                                                   '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::DRIVER_TEMPER[] =   {'\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                '\x00', '\x00', '\x00', '\x14',
                                                '\x00', '\x0c',  //10+2
                                                '\x00', '\x00',
                                                '\x04',
                                                '\x01',
                                                '\x12', '\x08', '\x82', '\xa1', '\x00', '\x23', '\x00', '\x01', '\x82', '\x01',
                                                '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00' };
const char SimensCommand::DRIVER_LOAD1[] =   {'\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                              '\x02', '\xf0', '\x80', '\x32', '\x01',
                                              '\x00', '\x00', '\x00', '\x15',
                                              '\x00', '\x0c',  //10+2
                                              '\x00', '\x00',
                                              '\x04',
                                              '\x01',
                                              '\x12', '\x08', '\x82', '\xa2', '\x00', '\x20', '\x00', '\x01', '\x82', '\x01', //0xa2为X轴  0x1a为编号
                                              '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::DRIVER_SPLOAD[] =   {'\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                               '\x02', '\xf0', '\x80', '\x32', '\x01',
                                               '\x00', '\x00', '\x00', '\x15',
                                               '\x00', '\x0c',  //10+2
                                               '\x00', '\x00',
                                               '\x04',
                                               '\x01',
                                               '\x12', '\x08', '\x82', '\xa1', '\x00', '\x21', '\x00', '\x01', '\x82', '\x01',
                                               '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CURRENT_PRO[] =   {'\x03','\x00','\x00','\x1d',
                                             '\x02','\xf0','\x80','\x32','\x01',
                                             '\x00','\x00','\x00','\x50',
                                             '\x00','\x0c',
                                             '\x00','\x00',
                                             '\x04',
                                             '\x01',
                                             '\x12','\x08','\x82','\x41', '\x00','\x1f',       //1e  可以
                                             '\x00','\x01',
                                             '\x7d','\x01',
                                             '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CNC_FEEDSETSPEED[] =   {   '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                     '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                     '\x00', '\x00', '\x00', '\x12',
                                                     '\x00', '\x0c',  //10+2
                                                     '\x00', '\x00',
                                                     '\x04',
                                                     '\x01',
                                                     '\x12', '\x08', '\x82', '\x41', '\x00', '\x02', '\x00', '\x01', '\x7f', '\x01',
                                                     '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00' };
const char SimensCommand::CNC_FEEDACTSPEED[] =   {  '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                    '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                    '\x00', '\x00', '\x00', '\x12',
                                                    '\x00', '\x0c',  //10+2
                                                    '\x00', '\x00',
                                                    '\x04',
                                                    '\x01',
                                                    '\x12', '\x08', '\x82', '\x41', '\x00', '\x01', '\x00', '\x01', '\x7f', '\x01',
                                                    '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};//FACT};
const char SimensCommand::CNC_SPINDLESETSPEED[] =   {  '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                       '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                       '\x00', '\x00', '\x00', '\x14',
                                                       '\x00', '\x0c',  //10+2
                                                       '\x00', '\x00',
                                                       '\x04',
                                                       '\x01',
                                                       '\x12', '\x08', '\x82', '\x01', '\x00', '\x03', '\x00', '\x04', '\x72', '\x01',
                                                       '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CNC_SPINDRATE[] =   {   '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                  '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                  '\x00', '\x00', '\x00', '\x14',
                                                  '\x00', '\x0c',  //10+2
                                                  '\x00', '\x00',
                                                  '\x04',
                                                  '\x01',
                                                  '\x12', '\x08', '\x82', '\x41', '\x00', '\x04', '\x00', '\x01', '\x72', '\x01',
                                                  '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::PROGRAM_NAME[] =   {    '\x03', '\x00', '\x00', '\x1d',    //12+17=29
                                                  '\x02', '\xf0', '\x80', '\x32', '\x01',
                                                  '\x00', '\x00', '\x00', '\x14',
                                                  '\x00', '\x0c',  //10+2
                                                  '\x00', '\x00',
                                                  '\x04',
                                                  '\x01',
                                                  '\x12', '\x08', '\x82', '\x41', '\x00', '\x0c', '\x00', '\x01', '\x7a', '\x01',
                                                  '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::CNC_WSC_FRAME[] =   {'\x03', '\x00', '\x00', '\x1d',
                                               '\x02', '\xf0', '\x80', '\x32',
                                               '\x01',
                                               '\x00', '\x00', '\x00', '\x0c',
                                               '\x00', '\x0c',
                                               '\x00', '\x00',
                                               '\x04',
                                               '\x01',
                                               '\x12', '\x08', '\x83', '\x41', '\x00', '\x10', '\x00', '\x01', '\x7f', '\x01'};
const char SimensCommand::CNC_WSC[] =   {'\x03', '\x00', '\x00', '\x1d',
                                         '\x02', '\xf0', '\x80',
                                         '\x32', '\x01',
                                         '\x00', '\x00', '\x00', '\x02',
                                         '\x00', '\x0c',
                                         '\x00', '\x00',
                                         '\x04',
                                         '\x01',
                                         '\x12', '\x08', '\x82', '\x41', '\x00', '\x01', '\x00', '\x07', '\x12', '\x01'};
const char SimensCommand::READ_R[] =   {'\x03', '\x00', '\x00', '\x1d',
                                        '\x02', '\xf0', '\x80', '\x32', '\x01',
                                        '\x00', '\x00', '\x00', '\x14',
                                        '\x00', '\x0c',
                                        '\x00', '\x00',
                                        '\x04',
                                        '\x01',
                                        '\x12', '\x08', '\x82', '\x41', '\x00', '\x01', '\x00', '\x01', '\x15', '\x01',};
const char SimensCommand::WRITE_R[] =   {'\x03', '\x00', '\x00', '\x29',
                                         '\x02', '\xf0', '\x80', '\x32', '\x01',
                                         '\x00', '\x00', '\x00', '\x12',
                                         '\x00', '\x0c', '\x00', '\x0c',
                                         '\x05',
                                         '\x01',
                                         '\x12', '\x08',
                                         '\x82', '\x41',
                                         '\x00', '\x01',
                                         '\x00', '\x01',//25 26
                                         '\x15', '\x01',
                                         '\x00', '\x09',
                                         '\x00', '\x08',
                                         '\x77', '\xbe', '\x9f', '\x1a', '\x2f', '\xdd', '\x5e', '\x40'};//33-40//123.456
const char SimensCommand::TOOLNAME[] =   {'\x03', '\x00', '\x00', '\x1d',
                                          '\x02', '\xf0', '\x80', '\x32', '\x01',
                                          '\x00', '\x00', '\x00', '\x14',
                                          '\x00', '\x0c',
                                          '\x00', '\x00',
                                          '\x04',
                                          '\x01',
                                          '\x12', '\x08', '\x82', '\x81', '\x00', '\x01', '\x00', '\x01', '\x21', '\x01',
                                          '\x03', '\x00', '\x00', '\x07', '\x02', '\xf0', '\x00'};
const char SimensCommand::TOOLTABLEINFO[] =   {'\x03', '\x00', '\x00', '\x1d',
                                        '\x02', '\xf0', '\x80', '\x32', '\x01',
                                        '\x00', '\x00', '\x00', '\x14',
                                        '\x00', '\x0c',
                                        '\x00', '\x00',
                                        '\x04',
                                        '\x01',
                                        '\x12', '\x08', '\x82', '\x81', '\x00', '\x01', '\x00', '\x0f', '\x14', '\x01'};
const char SimensCommand::TOOLINDEX[] =   {'\x03', '\x00', '\x00', '\x1d',    // 12+17=29
                                           '\x02', '\xf0', '\x80',
                                           '\x32', '\x01',
                                           '\x00', '\x00', '\x00', '\x14',
                                           '\x00', '\x0c',    // 10+2
                                           '\x00', '\x00',
                                           '\x04',
                                           '\x01',
                                           '\x12', '\x08', '\x82', '\x41', '\x00', '\x17', '\x00', '\x01', '\x7f', '\x01'
                                           //'\x12', '\x08', '\x82', '\x41', '\x00', '\x21', '\x00', '\x01', '\x7f', '\x01'
                                          };
const char SimensCommand::TOOLFLAG[] =   { '\x01', '\x03', '\x07', '\x06', '\x09', '\x0b',
                                           '\x0c', '\x0f', '\x10', '\x12', '\x14', '\x18', '\x22'};

SimensCommand::SimensCommand()
{
    FirstHandShank = QByteArray::fromRawData( FIRST_HAND_SHARK, sizeof( FIRST_HAND_SHARK));
    SecondHandShank = QByteArray::fromRawData( SECOND_HAND_SHARK, sizeof(SECOND_HAND_SHARK));
    ThirdHandShank = QByteArray::fromRawData(THIRD_HAND_SHARK, sizeof(THIRD_HAND_SHARK));
    ReadVerInfo = QByteArray::fromRawData(READ_VER_INFO, sizeof(READ_VER_INFO));
    CNC_Id = QByteArray::fromRawData(CNC_ID, sizeof(CNC_ID));
    CNC_Type = QByteArray::fromRawData(CNC_TYPE, sizeof(CNC_TYPE));
    CNC_FeedRate = QByteArray::fromRawData(CNC_FEEDRATE, sizeof(CNC_FEEDRATE));
    CNC_FeedSetRate = QByteArray::fromRawData(CNC_FEEDSETSPEED, sizeof(CNC_FEEDSETSPEED));
    CNC_FeedActRate = QByteArray::fromRawData(CNC_FEEDACTSPEED, sizeof(CNC_FEEDACTSPEED));
    CNC_SpindleActSpeed = QByteArray::fromRawData(CNC_SPINDLEACTSPEED, sizeof(CNC_SPINDLEACTSPEED));
    CNC_SpindleSetSpeed = QByteArray::fromRawData(CNC_SPINDLESETSPEED, sizeof(CNC_SPINDLESETSPEED));
    CNC_SpindRate = QByteArray::fromRawData(CNC_SPINDRATE, sizeof(CNC_SPINDRATE));
    MachinePos = QByteArray::fromRawData(MACHINE_POS, sizeof(MACHINE_POS));
    PosFlag = QByteArray::fromRawData(POSFLAG, sizeof(POSFLAG));
    DriverCurrent = QByteArray::fromRawData(DRIVER_CURRENT, sizeof(DRIVER_CURRENT));
    DriverTemper = QByteArray::fromRawData(DRIVER_TEMPER, sizeof(DRIVER_TEMPER));
    DriverLoad1 = QByteArray::fromRawData(DRIVER_LOAD1, sizeof(DRIVER_LOAD1));
    DriverSpload = QByteArray::fromRawData(DRIVER_SPLOAD, sizeof(DRIVER_SPLOAD));
    ProgramName = QByteArray::fromRawData(PROGRAM_NAME, sizeof(PROGRAM_NAME));
    CurrentPro = QByteArray::fromRawData(CURRENT_PRO, sizeof(CURRENT_PRO));
    CNC_WSC_Frame = QByteArray::fromRawData(CNC_WSC_FRAME, sizeof(CNC_WSC_FRAME));
    CNC_Wsc = QByteArray::fromRawData(CNC_WSC, sizeof(CNC_WSC));
    Read_R = QByteArray::fromRawData(READ_R, sizeof(READ_R));
    Write_R = QByteArray::fromRawData(WRITE_R, sizeof(WRITE_R));
    ToolName = QByteArray::fromRawData(TOOLNAME, sizeof(TOOLNAME));
    ToolTableInfo = QByteArray::fromRawData(TOOLTABLEINFO, sizeof(TOOLTABLEINFO));
    ToolIndex = QByteArray::fromRawData(TOOLINDEX, sizeof(TOOLINDEX));
    ToolFlag = QByteArray::fromRawData(TOOLFLAG, sizeof(TOOLFLAG));
}
