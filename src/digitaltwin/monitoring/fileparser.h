//#pragma execution_character_set("utf-8")

#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <QString>
#include <QList>
#include <QDebug>

struct Component {
    QString name;
    QString type;
    QString attach;
    QStringList stlFiles;
    QStringList visiable;
    QStringList XRGB;
    double positionX = 0;
    double positionY = 0;
    double positionZ = 0;
    double rotationI = 0;
    double rotationJ = 0;
    double rotationK = 0;
};//全局命名空间

class FileParser {
public:
    static QList<Component> ParseMachineFile(const QString &filePath);
};

#endif // FILEPARSER_H
