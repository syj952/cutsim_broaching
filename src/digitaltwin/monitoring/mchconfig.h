#ifndef MCHCONFIG_H
#define MCHCONFIG_H

#include <QWidget>
#include <QObject>
#include "simensclient.h"
#include "heidenhainclient.h"
//#include "serialportcom.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDebug>
#include <QAction>
#include <QFileDialog>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <array>
#include <QFile>
#include <QTextStream>

class MchConfig : public QWidget
{
    Q_OBJECT

public:
    explicit MchConfig(QWidget *parent = nullptr);
    ~MchConfig() override;
//    void start();
//    void stop();

private slots:
    void selectMchFilePath();
    void onButtonClicked();
    void offButtonClicked();

private:
    simensclient* Simensclient = nullptr;
    CncInfo_HeiDenHainCommands *hdhclient = nullptr;
    void GetMchData();
    void GetHDHData();
    QLineEdit *lineEdit_mch;
    QComboBox *comboBox_CNC;
    QPushButton *btn_CNCconnect;
    QPushButton *btn_CNCdisconnect;
    QLineEdit *lineEdit_wpcoor_X;
    QLineEdit *lineEdit_wpcoor_Y;
    QLineEdit *lineEdit_wpcoor_Z;
    QLineEdit *lineEdit_lengthToolTip;
    QLineEdit *lineEdit_offset_X;
    QLineEdit *lineEdit_offset_Y;
    QLineEdit *lineEdit_offset_Z;
    bool isCollecting = 0;

    QThread *thread = nullptr;
    QTimer *timer = nullptr;
    std::array<double, 6> ppppp{ {0,0,0,0,0,0} };// 正常用这个
    //std::array<double, 6> ppppp{ {564.059 +5.5 , -397.368 +60 , -42.595 - (600 - 150 - 218.493) -15 , 0, 0, 0}};// 测试用
    double act_rpm = -1;//zhushidiao***777
    double act_feed = -1;//zhushidiao***777

signals:
    void mchFileSelected(const QString &filePath);
    //void mchDataSignal(double macpos[6], double act_rpm, double act_feed);
    void mchDataSignal(const std::array<double, 6>& macpos, double act_rpm, double act_feed);
    void mchDataSignal_forGCode(const std::array<double, 6>& macpos, double act_rpm, double act_feed);
    void mchSingleDataSignal(int tool_index);
    void lengthToolTipUpdated(double lengthToolTip, const QVector<double>& workpieceOffset, const QVector<double>& manualOffset);

private:
    QFile m_mchDataFile;
    QTextStream m_mchDataStream;
    bool m_mchDataFileOpened = false;
    void openMchDataFile(const QString& filePath);
    bool readActualCoorFromTxt(std::array<double, 6>& macpos);
    bool readGCodeFromTxt(std::array<double, 6>& macpos, double& F, double& S);
};

#endif // MCHCONFIG_H
