#ifndef SERIALPORTCOM_H
#define SERIALPORTCOM_H

#include <QWidget>
#include <QtSerialPort/QtSerialPort>
#include <QScrollBar>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
//#include "plotmanager.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QQueue>
#include <QStringList>
#include <QtMath>
#include <cmath>

#define START_MSG       '$'
#define END_MSG         ';'

#define WAIT_START      1
#define IN_MESSAGE      2
#define UNDEFINED       3


class SerialPortCom : public QWidget
{
    Q_OBJECT

public:
    SerialPortCom(QWidget *parent = nullptr);
    ~SerialPortCom();

    //void setPlotManager(PlotManager *plottt){this->plotmanager = plottt;}

private slots:
    void ReadData();
    void on_openport_clicked();
    void on_Refresh_clicked();
    void on_disconnect_clicked();
    void on_reset_clicked();

private:
    QStringList processData(const QStringList& originalData);
    QStringList calculateForces(const QStringList& rawData);

private:
    QSerialPort* serialPort = nullptr;
    int STATE = WAIT_START;     // State of recieiving message from port
    QString receivedData;       // Used for reading from the port
    double OFFSET[4] = {0};
    double chanal_save_when_reset_triger[25] = {0};

    QPushButton *botton_Refresh;
    QComboBox *comboPort;
    QPushButton *botton_openPort;
    QPushButton *botton_disConnect;
    QPushButton *botton_Reset;

    //PlotManager *plotmanager = nullptr;
    QFile file;
    QTextStream* out;

    //data processing
    static const int WINDOW_SIZE = 10;
    QVector<QQueue<double>> windowBuffers;
    QVector<double> windowSums;

signals:
    void newData(QStringList result);

};
#endif // SERIALPORTCOM_H
