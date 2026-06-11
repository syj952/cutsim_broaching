#include "mfem_analysis1.hpp"
#include "src/cutsim/cutsim_def.hpp"
#include "mfem_mesh_writer.hpp"
#include "octree.hpp"
#include "octnode.hpp"
#include "glvertex.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"
using namespace std;
using namespace rapidjson;
//using namespace mfem;
using namespace cutsim;
struct Ex12pJsonHandler : public BaseReaderHandler<UTF8<>, Ex12pJsonHandler> {
    std::vector<double>& eigenvalues;
    std::vector<std::vector<double>>& eigenvectors;
    
    enum State {
        NONE,
        EXPECTING_EIGENVALUES,
        READING_EIGENVALUES,
        EXPECTING_EIGENVECTORS,
        READING_EIGENVECTORS,
        READING_EIGENVECTOR_ROW
    } state = NONE;
    
    std::vector<double> currentRow;

    Ex12pJsonHandler(std::vector<double>& evals, std::vector<std::vector<double>>& evecs) 
        : eigenvalues(evals), eigenvectors(evecs) {}

    bool Key(const char* str, SizeType length, bool copy) {
        std::string key(str, length);
        if (key == "eigenvalues") {
            state = EXPECTING_EIGENVALUES;
        } else if (key == "eigenvectors") {
            state = EXPECTING_EIGENVECTORS;
        }
        return true;
    }

    bool StartArray() {
        if (state == EXPECTING_EIGENVALUES) {
            state = READING_EIGENVALUES;
            eigenvalues.clear();
        } else if (state == EXPECTING_EIGENVECTORS) {
            state = READING_EIGENVECTORS;
            eigenvectors.clear();
        } else if (state == READING_EIGENVECTORS) {
            state = READING_EIGENVECTOR_ROW;
            currentRow.clear();
        }
        return true;
    }

    bool EndArray(SizeType elementCount) {
        if (state == READING_EIGENVALUES) {
            state = NONE;
        } else if (state == READING_EIGENVECTOR_ROW) {
            eigenvectors.push_back(std::move(currentRow));
            state = READING_EIGENVECTORS;
        } else if (state == READING_EIGENVECTORS) {
            state = NONE;
        }
        return true;
    }

    bool Double(double d) {
        if (state == READING_EIGENVALUES) {
            eigenvalues.push_back(d);
        } else if (state == READING_EIGENVECTOR_ROW) {
            currentRow.push_back(d);
        }
        return true;
    }
    
    bool Int(int i) { return Double(i); }
    bool Uint(unsigned u) { return Double(u); }
    bool Int64(int64_t i) { return Double(i); }
    bool Uint64(uint64_t u) { return Double(u); }
};
void MeshIDExport(Octree* tree,const std::string& meshFile,std::vector<GLVertex*>& normalvertices)
{
    MfemMeshWriter writer(*tree);
    writer.exportMesh(meshFile, normalvertices);
}

static QByteArray extractJsonSegment(const QByteArray &raw)
{
    // 优先匹配我们输出的键，找最后一个对象起始位置
    int start = raw.lastIndexOf("{\"eigenvalues\"");
    if (start == -1) { start = raw.lastIndexOf("{\"eigenvectors\""); }
    if (start == -1) { start = raw.lastIndexOf('{'); }
    if (start == -1) { return QByteArray(); }

    // 顶层花括号配对查找结束位置
    int depth = 0, end = -1;
    for (int i = start; i < raw.size(); ++i)
    {
        const char ch = raw.at(i);
        if (ch == '{') { ++depth; }
        else if (ch == '}')
        {
            --depth;
            if (depth == 0) { end = i; break; }
        }
    }
    if (end == -1) { return QByteArray(); }
    return raw.mid(start, end - start + 1);
}

void runEx12p(const std::string& meshFile,
              std::vector<double>& materialprops,
              std::vector<double>& eigenvalues,
              std::vector<std::vector<double>>& eigenvectors)
{
    qDebug() << "开始调用 mfem 程序...";
    QProcess *ex12pProcess = new QProcess(nullptr);
    //ex12pProcess->setProcessChannelMode(QProcess::MergedChannels);
    ex12pProcess->setProcessChannelMode(QProcess::SeparateChannels);
    //QString program = "mpirun"; // linux
    //QString program = "mpiexec"; // windows
    //QStringList arguments;
    //arguments << "-np" << "1";
    //arguments << "mfem/fem.exe";
    QString program = "mfem/fem.exe"; // windows
    QStringList arguments;
    // 改为使用传入的 meshFile
    arguments << "-m" << QString::fromStdString(meshFile);
    arguments << "--rho" << QString::number(materialprops[0], 'f', 12);
    arguments << "--young" << QString::number(materialprops[1], 'f', 2);
    arguments << "--nu" << QString::number(materialprops[2], 'f', 2);    
    arguments << "-no-vis";

    QByteArray allOutput;
    QObject::connect(ex12pProcess, &QProcess::readyReadStandardOutput, ex12pProcess, [ex12pProcess, &allOutput]() {
        const QByteArray chunk = ex12pProcess->readAllStandardOutput();
        allOutput.append(chunk); //直接累积到 allOutput        
        //qDebug().noquote() << "[ex12p]" << chunk; //// 不在终端打印 JSON 内容，
    });

    QObject::connect(ex12pProcess, &QProcess::errorOccurred, ex12pProcess, [](QProcess::ProcessError e) {
        qDebug() << "ex12p error:" << e;
    });
    QObject::connect(ex12pProcess, &QProcess::stateChanged, ex12pProcess, [](QProcess::ProcessState s) {
        qDebug() << "ex12p state:" << s;
    });

    ex12pProcess->start(program, arguments);
    ex12pProcess->closeWriteChannel();

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QObject::connect(ex12pProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &timeout, [ex12pProcess, &loop]() {
        qDebug() << "ex12p 超时，尝试终止进程";
        if (ex12pProcess->state() == QProcess::Running) {
            ex12pProcess->kill();
        }
        loop.quit();
    });

    timeout.start(6000000);
    loop.exec();

    allOutput.append(ex12pProcess->readAllStandardOutput());

    {
        const QByteArray jsonBytes = extractJsonSegment(allOutput);

        // 将 JSON 保存到文本文件，不在终端显示
        const QString jsonOutPath = QStringLiteral("data/eigen_results.json");
        QFile out(jsonOutPath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(jsonBytes);
            out.close();
            qDebug() << "JSON 已保存到文件:" << jsonOutPath << ", 字节数:" << jsonBytes.size();
        } else {
            qDebug() << "无法写入 JSON 文件:" << jsonOutPath;
        }
        if (jsonBytes.isEmpty()) {
            qDebug() << "未能提取到有效的 JSON 数据段";
        }
        else {
            Reader reader;
            StringStream ss(jsonBytes.constData());
            Ex12pJsonHandler handler(eigenvalues, eigenvectors);
            // kParseNumbersAsStringsFlag is not needed, we parse doubles directly
            // 启用 kParseNanAndInfFlag 以支持 NaN 和 Infinity，这在科学计算中很常见
            ParseResult result = reader.Parse<kParseNanAndInfFlag>(ss, handler);

            if (!result) {
                qDebug() << "JSON 解析失败 (rapidjson): " << GetParseError_En(result.Code())
                    << " Offset:" << result.Offset();
            }
            else {
                qDebug() << "JSON 解析成功 (rapidjson流式解析)。eigenvalues:" << eigenvalues.size()
                    << "eigenvectors:" << eigenvectors.size();
                if (!eigenvectors.empty()) {
                    qDebug() << "eigenvectors dims: " << eigenvectors.size() << "x" << eigenvectors[0].size();
                }
            }
        }
    }

    int exitCode = ex12pProcess->exitCode();
    //qDebug() << "ex12p 程序执行完成，退出代码:" << exitCode;

    ex12pProcess->deleteLater();
    qDebug() << "ex12p 程序调用完成，主程序继续执行...";

}

static bool isResidualReleaseRoot(const QDir& dir)
{
    return QFileInfo::exists(dir.filePath("data")) &&
           QFileInfo::exists(dir.filePath("mfem/residual_release.exe"));
}

static QString findResidualReleaseProjectRoot()
{
    const QStringList starts = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };

    QString dataOnlyRoot;
    for (const QString& start : starts) {
        QDir dir(start);
        dir.makeAbsolute();
        for (int i = 0; i < 5; ++i) {
            if (isResidualReleaseRoot(dir)) {
                return QDir::cleanPath(dir.absolutePath());
            }
            if (dataOnlyRoot.isEmpty() &&
                QFileInfo::exists(dir.filePath("data"))) {
                dataOnlyRoot = QDir::cleanPath(dir.absolutePath());
            }
            if (!dir.cdUp()) {
                break;
            }
        }
    }

    if (!dataOnlyRoot.isEmpty()) {
        return dataOnlyRoot;
    }

    return QDir::cleanPath(QDir::currentPath());
}

std::string residualReleaseProjectRoot()
{
    return findResidualReleaseProjectRoot().toStdString();
}

static QString resolveResidualPath(const QString& path,
                                   const QString& projectRoot)
{
    const QFileInfo pathInfo(path);
    if (pathInfo.isAbsolute()) {
        return QDir::cleanPath(pathInfo.absoluteFilePath());
    }

    return QDir::cleanPath(QDir(projectRoot).filePath(path));
}

static void appendResidualReleaseCandidates(QStringList& candidates,
                                            const QString& start)
{
    QDir dir(start);
    dir.makeAbsolute();
    for (int i = 0; i < 5; ++i) {
        candidates << dir.filePath("mfem/residual_release.exe");
        candidates << dir.filePath("residual_release.exe");
        if (!dir.cdUp()) {
            break;
        }
    }
}

static QString findResidualReleaseProgram(const QString& projectRoot)
{
    const QStringList candidates = {
        QDir(projectRoot).filePath("mfem/residual_release.exe"),
        QDir(projectRoot).filePath("residual_release.exe")
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    QStringList fallbackCandidates;
    appendResidualReleaseCandidates(fallbackCandidates, QDir::currentPath());
    appendResidualReleaseCandidates(fallbackCandidates,
                                    QCoreApplication::applicationDirPath());

    for (const QString& candidate : fallbackCandidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return candidates.front();
}

static QString findProgramOnPath(const QString& programName)
{
    const QString pathValue = QString::fromLocal8Bit(qgetenv("PATH"));
    const QStringList entries = pathValue.split(';', QString::SkipEmptyParts);
    for (const QString& entry : entries) {
        const QString candidate = QDir(entry).filePath(programName);
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }
    return QString();
}

static QString findMpiExecProgram(const QString& projectRoot)
{
    const QString overridePath =
        QString::fromLocal8Bit(qgetenv("XCUTSIM_MPIEXEC")).trimmed();
    if (!overridePath.isEmpty() && QFileInfo::exists(overridePath)) {
        return QDir::cleanPath(QFileInfo(overridePath).absoluteFilePath());
    }

    const QStringList candidates = {
        QDir(projectRoot).filePath("mfem/mpiexec.exe"),
        QDir(projectRoot).filePath("mpiexec.exe"),
        QStringLiteral("C:/Program Files/Microsoft MPI/Bin/mpiexec.exe"),
        QStringLiteral("C:/Program Files (x86)/Microsoft MPI/Bin/mpiexec.exe"),
        QStringLiteral("C:/Program Files (x86)/Microsoft SDKs/MPI/Bin/mpiexec.exe")
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }

    return findProgramOnPath(QStringLiteral("mpiexec.exe"));
}

static int residualReleaseMpiProcessCount()
{
    bool ok = false;
    const int requested =
        QString::fromLocal8Bit(qgetenv("XCUTSIM_RESIDUAL_MPI_NP")).toInt(&ok);
    if (ok && requested > 0) {
        return requested;
    }
    return 4;
}

static bool parseResidualReleaseSummary(const QString& summaryPath,
                                        ResidualReleaseSummary& summary)
{
    QFile file(summaryPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open residual release summary:" << summaryPath;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "Cannot parse residual release summary:" << summaryPath
                 << parseError.errorString();
        return false;
    }

    const QJsonObject obj = doc.object();
    summary.step = obj.value("step").toInt(summary.step);
    summary.max_disp_mm = obj.value("max_disp_mm").toDouble(summary.max_disp_mm);
    summary.max_ux_mm = obj.value("max_ux_mm").toDouble(summary.max_ux_mm);
    summary.max_uy_mm = obj.value("max_uy_mm").toDouble(summary.max_uy_mm);
    summary.max_uz_mm = obj.value("max_uz_mm").toDouble(summary.max_uz_mm);
    summary.success = true;
    return true;
}

int runResidualRelease(
    const std::string& meshFile,
    const std::string& stressConfigFile,
    double young,
    double poisson,
    int fixedBoundaryAttr,
    int stepId,
    const std::string& outputPrefix,
    ResidualReleaseSummary* summary)
{
    const QString projectRoot = findResidualReleaseProjectRoot();
    const QString meshPath =
        resolveResidualPath(QString::fromStdString(meshFile), projectRoot);
    const QString stressConfigPath =
        resolveResidualPath(QString::fromStdString(stressConfigFile), projectRoot);
    const QString outputPrefixQt =
        resolveResidualPath(QString::fromStdString(outputPrefix), projectRoot);

    const QFileInfo outputInfo(outputPrefixQt);
    if (!outputInfo.path().isEmpty()) {
        QDir().mkpath(outputInfo.path());
    }

    const QString solverProgram = findResidualReleaseProgram(projectRoot);
    QStringList solverArguments;
    solverArguments << "-m" << meshPath;
    solverArguments << "--young" << QString::number(young, 'f', 6);
    solverArguments << "--nu" << QString::number(poisson, 'f', 8);
    solverArguments << "--bc-attr" << QString::number(fixedBoundaryAttr);
    solverArguments << "--stress-config" << stressConfigPath;
    solverArguments << "--step" << QString::number(stepId);
    solverArguments << "--out" << outputPrefixQt;
    solverArguments << "-vis";

    QString program = solverProgram;
    QStringList arguments = solverArguments;
    const int mpiRanks = residualReleaseMpiProcessCount();
    const QString mpiExec = findMpiExecProgram(projectRoot);
    if (mpiRanks > 1 && !mpiExec.isEmpty()) {
        program = mpiExec;
        arguments.clear();
        arguments << "-n" << QString::number(mpiRanks) << solverProgram;
        arguments << solverArguments;
        qDebug() << "Residual release MPI enabled:"
                 << "mpiexec" << mpiExec
                 << "ranks" << mpiRanks;
    } else if (mpiRanks > 1) {
        qDebug() << "Residual release MPI launcher not found; running single process."
                 << "Set XCUTSIM_MPIEXEC to an MS-MPI mpiexec.exe path to enable"
                 << mpiRanks << "ranks.";
    }

    qDebug() << "Starting residual release solver:" << program
             << "workingDir" << projectRoot
             << arguments;

    QProcess* process = new QProcess(nullptr);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->setWorkingDirectory(projectRoot);

    QByteArray stdoutData;
    QByteArray stderrData;
    QObject::connect(process, &QProcess::readyReadStandardOutput, process,
                     [process, &stdoutData]() {
                         stdoutData.append(process->readAllStandardOutput());
                     });
    QObject::connect(process, &QProcess::readyReadStandardError, process,
                     [process, &stderrData]() {
                         stderrData.append(process->readAllStandardError());
                     });
    QObject::connect(process, &QProcess::errorOccurred, process,
                     [](QProcess::ProcessError e) {
                         qDebug() << "residual_release process error:" << e;
                     });

    process->start(program, arguments);
    process->closeWriteChannel();

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &timeout, [process, &loop]() {
        qDebug() << "residual_release timeout, killing process.";
        if (process->state() == QProcess::Running) {
            process->kill();
        }
        loop.quit();
    });

    timeout.start(6000000);
    loop.exec();

    stdoutData.append(process->readAllStandardOutput());
    stderrData.append(process->readAllStandardError());

    const int exitCode = process->exitCode();
    const QProcess::ExitStatus exitStatus = process->exitStatus();
    process->deleteLater();

    if (!stdoutData.isEmpty()) {
        qDebug().noquote() << "[residual_release stdout]" << stdoutData;
    }
    if (!stderrData.isEmpty()) {
        qDebug().noquote() << "[residual_release stderr]" << stderrData;
    }
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        qDebug() << "residual_release failed. exitCode =" << exitCode;
        return 0;
    }

    ResidualReleaseSummary localSummary;
    localSummary.step = stepId;
    localSummary.displacement_file = (outputPrefixQt + "_disp.gf").toStdString();
    localSummary.deformed_mesh_file =
        (outputPrefixQt + "_deformed.mesh").toStdString();
    localSummary.summary_file =
        (outputPrefixQt + "_summary.json").toStdString();

    const QString summaryPath = outputPrefixQt + "_summary.json";
    if (!parseResidualReleaseSummary(summaryPath, localSummary)) {
        return 0;
    }

    qDebug() << "Residual release summary:"
             << "step" << localSummary.step
             << "max_disp_mm" << localSummary.max_disp_mm
             << "max_ux_mm" << localSummary.max_ux_mm
             << "max_uy_mm" << localSummary.max_uy_mm
             << "max_uz_mm" << localSummary.max_uz_mm;

    if (summary) {
        *summary = localSummary;
    }
    return 1;
}





