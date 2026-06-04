#include "Icon.h"
#include "ComplainUtf8.h"
#include "mainwindow.h"

#include <QTranslator>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <Windows.h>
#include <qfontdatabase.h>
// 手动初始化VTK
//#include<vtkAutoInit.h>

#define __APP_NAME__ "XSim"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(mdi);
    QApplication app(argc, argv);


    //VTK_MODULE_INIT(vtkRenderingOpenGL2)
    //VTK_MODULE_INIT(vtkInteractionStyle);
    //VTK_MODULE_INIT(vtkRenderingFreeType);

    app.setWindowIcon(QIcon(ICON_APP));
    QCoreApplication::setApplicationName(__APP_NAME__);
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription(__APP_NAME__" v1.0");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "openfile");
    parser.process(app);

    //app.setStyleSheet("QDockWidget{"
    //    "   background-color: #87CEEB;" 
    //    "   color: #000000;"            
    //    "   font-size: 24px;"
    //    "   padding-top: 4px;"
    //    "   padding-bottom: 12px;"
    //    "}"
    //    "QPushButton {"
    //    "    background-color: #87CEEB;"
    //    "    color: white;"
    //    "    border: none;"
    //    "    padding: 10px;"
    //    "    border-radius: 5px;"
    //    "}"
    //    "QMenuBar {"
    //   // "   background-color: #d8d8d8;" 
    //    "   color: #000000;"          
    //    "   font-size: 12pt; "           
    //    "   padding-top: 4px;"
    //    "   padding-bottom: 12px;"
    //    "}"
    //    //"QMenuBar::item {"
    //    //"   background-color: transparent;" 
    //    //"}"
    //    "QMenuBar::item:selected {"
    //    "   background-color: #d8d8d8;" 
    //    "}"
    //    "QMenu {"
    //    "   color: #000000;"           
    //    "   font-size: 12pt; "           
    //    "   padding-top: 4px;"
    //    "   padding-bottom: 12px;"
    //    "}");

    MainWindow mainWin;
    foreach (const QString &fileName, parser.positionalArguments())
        mainWin.openFile(fileName);
    //MbdUI::SetUIer(new QT_UIer(&mainWin)); 
	mainWin.setGeometry(200, 100, 2000, 1200);//设置窗口位置和大小
    mainWin.show();
    return app.exec();
}
