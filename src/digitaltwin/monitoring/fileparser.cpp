
#include "fileparser.h"
#include <QFile>
#include <QDomDocument>
#include <QDebug>

QList<Component> FileParser::ParseMachineFile(const QString &filePath) {
    QList<Component> components;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << filePath;
        return components;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        qDebug() << "Failed to parse XML.";
        file.close();
        return components;
    }
    file.close();

    QDomNodeList componentNodes = doc.elementsByTagName("Component");
    for (int i = 0; i < componentNodes.count(); ++i) {
        QDomElement element = componentNodes.at(i).toElement();//将节点转换为QDomElement类型，用于访问其内容和属性

        // 检查父节点的标签名是否是 "Collision"
        //qDebug() << "Processing component with Name:" << element.attribute("Name");
        //if (element.parentNode().nodeName() != "Between") {
        if (element.attribute("Name") != "" ) {
            if (!element.isNull()) {
                Component comp;
                comp.name = element.attribute("Name");
                comp.type = element.attribute("Type");
                comp.attach = element.elementsByTagName("Attach").isEmpty() ? ""
                              : element.elementsByTagName("Attach").at(0).toElement().text();

                QDomNodeList stlNodes = element.elementsByTagName("STL");
                for (int j = 0; j < stlNodes.count(); ++j) {
                    QDomElement stlElement = stlNodes.at(j).toElement();
                    if (!stlElement.isNull()) {
                        comp.stlFiles.append(stlElement.firstChildElement("File").text());
                        comp.visiable.append(stlElement.attribute("Visible", "off"));
                        comp.XRGB.append(stlElement.attribute("XRGB", "0x00000000"));
                    }
                }

                QDomElement positionElement = element.firstChildElement("Position");
                if (!positionElement.isNull()) {
                    comp.positionX = positionElement.attribute("X", "0").toDouble();
                    comp.positionY = positionElement.attribute("Y", "0").toDouble();
                    comp.positionZ = positionElement.attribute("Z", "0").toDouble();
                }

                QDomElement rotationElement = element.firstChildElement("Rotation");
                if (!rotationElement.isNull()) {
                    comp.rotationI = rotationElement.attribute("I", "0").toDouble();
                    comp.rotationJ = rotationElement.attribute("J", "0").toDouble();
                    comp.rotationK = rotationElement.attribute("K", "0").toDouble();
                }

                components.append(comp);
            }
        }
    }
    return components;
}
