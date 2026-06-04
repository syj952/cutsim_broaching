#ifndef ETH_HEIDENHAINCOMMANDS_H
#define ETH_HEIDENHAINCOMMANDS_H

#include <QString>

class ETH_HeiDenHainCommands
{
public:
    ETH_HeiDenHainCommands();

    static const QString A_LGINSPECT;
    static const QString C_CC_07;
    static const QString R_PR;
    static const QString A_LOINSPECT;
    static const QString A_LGDNC;
    static const QString A_LGPLCDEBUG;
    static const QString A_LOPLCDEBUG;

    static const QString GETNCVER;
    static const QString GETMTPOS;
    static const QString GETCUPOS;
    static const QString GETFEED;
    static const QString GETTOOLINDEX;
    static const QString GETCUTOOLINFO;

    QByteArray A_lginspect;
    QByteArray C_cc_07;
    QByteArray R_pr;
    QByteArray A_loinspect;
    QByteArray A_lgdnc;
    QByteArray A_lgplcdebug;
    QByteArray A_loplcdebug;

    QByteArray GetNcVer;
    QByteArray GetMtPos;
    QByteArray GetCuPos;
    QByteArray GetFeed;
    QByteArray GetToolIndex;
    QByteArray GetCuToolInfo;
};

#endif // ETH_HEIDENHAINCOMMANDS_H

