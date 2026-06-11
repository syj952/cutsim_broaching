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
#include <QDebug>
#include <QEventLoop>
#include <QProcess>
#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QSaveFile>
#include <QTimer>
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
    QByteArray best;
    int start = raw.indexOf('{');
    while (start != -1) {
        int depth = 0;
        int end = -1;
        bool inString = false;
        bool escaped = false;
        for (int i = start; i < raw.size(); ++i) {
            const char ch = raw.at(i);
            if (inString) {
                if (escaped) {
                    escaped = false;
                }
                else if (ch == '\\') {
                    escaped = true;
                }
                else if (ch == '"') {
                    inString = false;
                }
                continue;
            }

            if (ch == '"') {
                inString = true;
            }
            else if (ch == '{') {
                ++depth;
            }
            else if (ch == '}') {
                --depth;
                if (depth == 0) {
                    end = i;
                    break;
                }
            }
        }

        if (end == -1) {
            start = raw.indexOf('{', start + 1);
            continue;
        }

        const QByteArray candidate = raw.mid(start, end - start + 1);
        if (candidate.contains("\"eigenvalues\"") && candidate.contains("\"eigenvectors\"")) {
            best = candidate;
        }

        start = raw.indexOf('{', end + 1);
    }

    return best;
}

static QString outputTailForLog(const QByteArray& data)
{
    QByteArray tail = data.right(2048);
    tail.replace('\r', ' ');
    tail.replace('\n', ' ');
    return QString::fromLocal8Bit(tail);
}

static bool isProjectRoot(const QDir& dir)
{
    return QFileInfo::exists(dir.filePath("data")) &&
           QFileInfo::exists(dir.filePath("mfem/fem.exe"));
}

static QString findProjectRoot()
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
            if (isProjectRoot(dir)) {
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

static QString resolveMeshPathForRead(const QString& path,
                                      const QString& projectRoot)
{
    const QFileInfo pathInfo(path);
    if (pathInfo.isAbsolute()) {
        return QDir::cleanPath(pathInfo.absoluteFilePath());
    }

    const QStringList candidates = {
        QDir(projectRoot).filePath(path),
        QDir(QDir(projectRoot).filePath("data")).filePath(path)
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(QFileInfo(candidate).absoluteFilePath());
        }
    }

    return QDir::cleanPath(QDir(projectRoot).filePath(path));
}

static bool saveJsonFile(const QString& path, const QByteArray& jsonBytes)
{
    QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        qDebug() << "无法创建 JSON 输出目录:" << info.absolutePath();
        return false;
    }

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) {
        qDebug() << "无法写入 JSON 文件:" << path << out.errorString();
        return false;
    }

    const qint64 written = out.write(jsonBytes);
    if (written != jsonBytes.size()) {
        qDebug() << "JSON 写入不完整:" << path << "written:" << written << "expected:" << jsonBytes.size();
        out.cancelWriting();
        return false;
    }

    if (!out.commit()) {
        qDebug() << "无法写入 JSON 文件:" << path << out.errorString();
        return false;
    }

    qDebug() << "JSON 已保存到文件:" << path << ", 字节数:" << jsonBytes.size();
    return true;
}

static QByteArray buildCombinedOutput(const QByteArray& stdoutBytes, const QByteArray& stderrBytes)
{
    QByteArray combined = stdoutBytes;
    if (!stderrBytes.isEmpty()) {
        combined.append('\n');
        combined.append(stderrBytes);
    }
    return combined;
}

static bool parseEigenJson(const QByteArray& jsonBytes,
                           std::vector<double>& eigenvalues,
                           std::vector<std::vector<double>>& eigenvectors)
{
    Reader reader;
    StringStream ss(jsonBytes.constData());
    Ex12pJsonHandler handler(eigenvalues, eigenvectors);
    ParseResult result = reader.Parse<kParseNanAndInfFlag>(ss, handler);

    if (!result) {
        qDebug() << "JSON 解析失败 (rapidjson): " << GetParseError_En(result.Code())
            << " Offset:" << result.Offset();
        return false;
    }

    qDebug() << "JSON 解析成功 (rapidjson流式解析)。eigenvalues:" << eigenvalues.size()
        << "eigenvectors:" << eigenvectors.size();
    if (!eigenvectors.empty()) {
        qDebug() << "eigenvectors dims: " << eigenvectors.size() << "x" << eigenvectors[0].size();
    }
    return true;
}

void runEx12p(const std::string& meshFile,
              std::vector<double>& materialprops,
              std::vector<double>& eigenvalues,
              std::vector<std::vector<double>>& eigenvectors)
{
    if (materialprops.size() < 3) {
        qDebug() << "runEx12p materialprops size invalid:" << materialprops.size();
        return;
    }

    qDebug() << "Starting mfem modal solver...";

    QProcess* ex12pProcess = new QProcess(nullptr);
    ex12pProcess->setProcessChannelMode(QProcess::SeparateChannels);

    const QString projectRoot = findProjectRoot();
    const QString program = QDir(projectRoot).filePath("mfem/fem.exe");
    const QString mfemDir = QDir(projectRoot).filePath("mfem");
    const QString meshPath =
        resolveMeshPathForRead(QString::fromStdString(meshFile), projectRoot);

    ex12pProcess->setWorkingDirectory(projectRoot);

    qDebug() << "ex12p projectRoot:" << projectRoot;
    qDebug() << "ex12p program:" << program;
    qDebug() << "ex12p mesh path:" << meshPath
             << "exists:" << QFileInfo::exists(meshPath);

    if (!QFileInfo::exists(meshPath)) {
        qDebug() << "ex12p mesh file not found, abort:" << meshPath;
        ex12pProcess->deleteLater();
        return;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", mfemDir + QDir::listSeparator() + env.value("PATH"));
    ex12pProcess->setProcessEnvironment(env);

    QStringList arguments;
    arguments << "-m" << QDir::toNativeSeparators(meshPath);
    arguments << "--rho" << QString::number(materialprops[0], 'f', 12);
    arguments << "--young" << QString::number(materialprops[1], 'f', 2);
    arguments << "--nu" << QString::number(materialprops[2], 'f', 2);
    arguments << "-no-vis";

    QByteArray allOutput;
    QByteArray allError;

    QObject::connect(ex12pProcess, &QProcess::readyReadStandardOutput,
                     ex12pProcess, [ex12pProcess, &allOutput]() {
        allOutput.append(ex12pProcess->readAllStandardOutput());
    });

    QObject::connect(ex12pProcess, &QProcess::readyReadStandardError,
                     ex12pProcess, [ex12pProcess, &allError]() {
        allError.append(ex12pProcess->readAllStandardError());
    });

    QObject::connect(ex12pProcess, &QProcess::errorOccurred,
                     ex12pProcess, [ex12pProcess](QProcess::ProcessError e) {
        qDebug() << "ex12p error:" << e << ex12pProcess->errorString();
        qDebug() << "ex12p program:" << ex12pProcess->program();
        qDebug() << "ex12p workingDirectory:" << ex12pProcess->workingDirectory();
        qDebug() << "ex12p arguments:" << ex12pProcess->arguments();
    });

    qDebug() << "ex12p arguments:" << arguments;

    ex12pProcess->start(program, arguments);
    if (!ex12pProcess->waitForStarted(5000)) {
        qDebug() << "ex12p FailedToStart:" << ex12pProcess->errorString();
        qDebug() << "ex12p fem.exe exists:" << QFileInfo::exists(program);
        ex12pProcess->deleteLater();
        return;
    }

    ex12pProcess->closeWriteChannel();

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QObject::connect(ex12pProcess,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop,
                     &QEventLoop::quit);

    QObject::connect(&timeout, &QTimer::timeout, &timeout,
                     [ex12pProcess, &loop]() {
        qDebug() << "ex12p timeout, killing process.";
        if (ex12pProcess->state() == QProcess::Running) {
            ex12pProcess->kill();
        }
        loop.quit();
    });

    timeout.start(6000000);
    loop.exec();

    allOutput.append(ex12pProcess->readAllStandardOutput());
    allError.append(ex12pProcess->readAllStandardError());

    const QByteArray combinedOutput = buildCombinedOutput(allOutput, allError);
    const QByteArray jsonBytes = extractJsonSegment(combinedOutput);
    const QString jsonOutPath =
        QDir(QDir(projectRoot).filePath("data")).filePath("eigen_results.json");

    if (jsonBytes.isEmpty()) {
        qDebug() << "No valid eigen JSON extracted.";
        qDebug() << "ex12p stdout bytes:" << allOutput.size()
                 << "stderr bytes:" << allError.size();
        if (!allError.isEmpty()) {
            qDebug().noquote() << "ex12p stderr tail:" << outputTailForLog(allError);
        }
        if (!allOutput.isEmpty()) {
            qDebug().noquote() << "ex12p stdout tail:" << outputTailForLog(allOutput);
        }
    } else {
        saveJsonFile(jsonOutPath, jsonBytes);
        parseEigenJson(jsonBytes, eigenvalues, eigenvectors);
    }

    qDebug() << "ex12p finished. exitCode:" << ex12pProcess->exitCode();

    ex12pProcess->deleteLater();
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

static QString findGLVisProgram(const QString& projectRoot)
{
    const QString overridePath =
        QString::fromLocal8Bit(qgetenv("XCUTSIM_GLVIS")).trimmed();
    if (!overridePath.isEmpty() && QFileInfo::exists(overridePath)) {
        return QDir::cleanPath(QFileInfo(overridePath).absoluteFilePath());
    }

    const QStringList candidates = {
        QDir(projectRoot).filePath("mfem/glvis/glvis.exe"),
        QDir(projectRoot).filePath("mfem/glvis.exe"),
        QDir(projectRoot).filePath("glvis/glvis.exe"),
        QDir(projectRoot).filePath("glvis.exe"),
        QStringLiteral("E:/MFEM/glvis-windows/glvis.exe")
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }

    return findProgramOnPath(QStringLiteral("glvis.exe"));
}

static bool launchGLVisDetached(const QString& projectRoot,
                                const QString& meshFile,
                                const QString& gridFunctionFile)
{
    if (!QFileInfo::exists(meshFile)) {
        qDebug() << "GLVis launch skipped; mesh file not found:" << meshFile;
        return false;
    }
    if (!QFileInfo::exists(gridFunctionFile)) {
        qDebug() << "GLVis launch skipped; gf file not found:" << gridFunctionFile;
        return false;
    }

    const QString glvisProgram = findGLVisProgram(projectRoot);
    if (glvisProgram.isEmpty()) {
        qDebug() << "GLVis launch skipped; glvis.exe was not found.";
        return false;
    }

    const QStringList arguments = {
        QStringLiteral("-m"),
        QDir::toNativeSeparators(meshFile),
        QStringLiteral("-g"),
        QDir::toNativeSeparators(gridFunctionFile)
    };
    const QString workingDirectory = QFileInfo(glvisProgram).absolutePath();
    const bool started =
        QProcess::startDetached(glvisProgram, arguments, workingDirectory);
    if (!started) {
        qDebug() << "Failed to launch GLVis:" << glvisProgram << arguments;
        return false;
    }

    qDebug() << "GLVis launched detached:" << glvisProgram << arguments;
    return true;
}

static int residualReleaseMpiProcessCount()
{
    bool ok = false;
    const int requested =
        QString::fromLocal8Bit(qgetenv("XCUTSIM_RESIDUAL_MPI_NP")).toInt(&ok);
    if (ok && requested > 0) {
        return requested;
    }
    return 12;
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
    solverArguments << "-no-vis";

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
                         const QByteArray chunk = process->readAllStandardOutput();
                         stdoutData.append(chunk);
                         if (!chunk.isEmpty()) {
                             qDebug().noquote() << "[residual_release stdout]"
                                                << QString::fromLocal8Bit(chunk).trimmed();
                         }
                     });
    QObject::connect(process, &QProcess::readyReadStandardError, process,
                     [process, &stderrData]() {
                         const QByteArray chunk = process->readAllStandardError();
                         stderrData.append(chunk);
                         if (!chunk.isEmpty()) {
                             qDebug().noquote() << "[residual_release stderr]"
                                                << QString::fromLocal8Bit(chunk).trimmed();
                         }
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

    const QString solutionMeshPath = QFileInfo::exists(outputPrefixQt + "_mesh.mesh")
        ? outputPrefixQt + "_mesh.mesh"
        : meshPath;
    launchGLVisDetached(projectRoot,
                        solutionMeshPath,
                        outputPrefixQt + "_disp.gf");

    if (summary) {
        *summary = localSummary;
    }
    return 1;
}



