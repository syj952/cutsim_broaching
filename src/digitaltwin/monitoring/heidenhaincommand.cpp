#include "heidenhaincommand.h"
#include <QDebug>

const QString ETH_HeiDenHainCommands::A_LGINSPECT = "00000008415f4c47494e535045435400";
const QString ETH_HeiDenHainCommands::C_CC_07 = "00000002435f43430007";
const QString ETH_HeiDenHainCommands::R_PR = "00000000525f5052";
const QString ETH_HeiDenHainCommands::A_LOINSPECT = "00000008415f4c4f494e535045435400";
const QString ETH_HeiDenHainCommands::A_LGDNC = "00000004415f4c47444e4300";
const QString ETH_HeiDenHainCommands::A_LGPLCDEBUG = "00000009415f4c47504c43444542554700";
const QString ETH_HeiDenHainCommands::A_LOPLCDEBUG = "00000009415f4c4f504c43444542554700";

const QString ETH_HeiDenHainCommands::GETNCVER = "00000000525f5652";
const QString ETH_HeiDenHainCommands::GETMTPOS = "00000002525f52490015";
const QString ETH_HeiDenHainCommands::GETCUPOS = "00000002525f52490016";
const QString ETH_HeiDenHainCommands::GETFEED = "00000002525f52490019";
const QString ETH_HeiDenHainCommands::GETTOOLINDEX = "00000020525f544c00000000544e433a5c544f4f4c5f502e5443480057484552452054203d203000";
const QString ETH_HeiDenHainCommands::GETCUTOOLINFO = "00000002525f52490033";

ETH_HeiDenHainCommands::ETH_HeiDenHainCommands()
{
    A_lginspect = QByteArray::fromHex(A_LGINSPECT.toLatin1());
    C_cc_07 = QByteArray::fromHex(C_CC_07.toLatin1());
    R_pr = QByteArray::fromHex(R_PR.toLatin1());
    A_loinspect = QByteArray::fromHex(A_LOINSPECT.toLatin1());
    A_lgdnc = QByteArray::fromHex(A_LGDNC.toLatin1());
    A_lgplcdebug = QByteArray::fromHex(A_LGPLCDEBUG.toLatin1());
    A_loplcdebug = QByteArray::fromHex(A_LOPLCDEBUG.toLatin1());

    GetNcVer = QByteArray::fromHex(GETNCVER.toLatin1());
    GetMtPos = QByteArray::fromHex(GETMTPOS.toLatin1());
    GetCuPos = QByteArray::fromHex(GETCUPOS.toLatin1());
    GetFeed = QByteArray::fromHex(GETFEED.toLatin1());
    GetToolIndex = QByteArray::fromHex(GETTOOLINDEX.toLatin1());
    GetCuToolInfo = QByteArray::fromHex(GETCUTOOLINFO.toLatin1());
}
