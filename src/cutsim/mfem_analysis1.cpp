//#include "mfem_analysis1.hpp"
#include "src/broaching/cutsim_def.hpp"
#include "octree.hpp"
#include "octnode.hpp"
#include "glvertex.hpp"
#include "rapidjson/reader.h"
#include "rapidjson/error/en.h"
#include <QProcessEnvironment>
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
        }
        else if (key == "eigenvectors") {
            state = EXPECTING_EIGENVECTORS;
        }
        return true;
    }

    bool StartArray() {
        if (state == EXPECTING_EIGENVALUES) {
            state = READING_EIGENVALUES;
            eigenvalues.clear();
        }
        else if (state == EXPECTING_EIGENVECTORS) {
            state = READING_EIGENVECTORS;
            eigenvectors.clear();
        }
        else if (state == READING_EIGENVECTORS) {
            state = READING_EIGENVECTOR_ROW;
            currentRow.clear();
        }
        return true;
    }

    bool EndArray(SizeType elementCount) {
        if (state == READING_EIGENVALUES) {
            state = NONE;
        }
        else if (state == READING_EIGENVECTOR_ROW) {
            eigenvectors.push_back(std::move(currentRow));
            state = READING_EIGENVECTORS;
        }
        else if (state == READING_EIGENVECTORS) {
            state = NONE;
        }
        return true;
    }

    bool Double(double d) {
        if (state == READING_EIGENVALUES) {
            eigenvalues.push_back(d);
        }
        else if (state == READING_EIGENVECTOR_ROW) {
            currentRow.push_back(d);
        }
        return true;
    }

    bool Int(int i) { return Double(i); }
    bool Uint(unsigned u) { return Double(u); }
    bool Int64(int64_t i) { return Double(i); }
    bool Uint64(uint64_t u) { return Double(u); }
};
void MeshIDExport(Octree* tree, const std::string& meshFile, std::vector<GLVertex*>& normalvertices)
{

    // 清除所有顶点状态
    tree->clearVertexStates();
    //tree->transtate();
    std::cout << "start numbering..." << std::endl;
    // 获取悬挂顶点和普通顶点
    std::vector<GLVertex*> hanging_vertices;
    tree->get_hangingVertex(hanging_vertices, normalvertices);
    std::cout << "hanging vertex: " << hanging_vertices.size() << std::endl;
    std::cout << "normal vertex: " << normalvertices.size() << std::endl;

    // 获取悬挂顶点的父节点信息
    tree->get_hanging_vertex_parent();
    // 获取边界面
    std::vector<std::vector<int>> boundaryFaces;
    std::vector<Octnode*> boundarynode;
    tree->boundary(boundaryFaces, normalvertices, boundarynode);
    std::cout << "boundary node: " << boundarynode.size() << std::endl;
    // 导出网格到文件
    tree->export_mesh_to_file(normalvertices, boundaryFaces, hanging_vertices, meshFile);
    auto mesh_export_end = std::chrono::high_resolution_clock::now();

}

static QByteArray extractJsonSegment(const QByteArray& raw)
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
    QProcess* ex12pProcess = new QProcess(nullptr);
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
    QByteArray errorOutput;
    QProcessEnvironment processEnv = QProcessEnvironment::systemEnvironment();
    const QString msysUcrtBin = QStringLiteral("C:\\msys64\\ucrt64\\bin");
    const QString oldPath = processEnv.value(QStringLiteral("PATH"));
    if (!oldPath.contains(msysUcrtBin, Qt::CaseInsensitive)) {
        processEnv.insert(QStringLiteral("PATH"), msysUcrtBin + QStringLiteral(";") + oldPath);
    }
    ex12pProcess->setProcessEnvironment(processEnv);
    QObject::connect(ex12pProcess, &QProcess::readyReadStandardOutput, ex12pProcess, [ex12pProcess, &allOutput]() {
        const QByteArray chunk = ex12pProcess->readAllStandardOutput();
        allOutput.append(chunk); //直接累积到 allOutput        
        //qDebug().noquote() << "[ex12p]" << chunk; //// 不在终端打印 JSON 内容，
        });

    QObject::connect(ex12pProcess, &QProcess::readyReadStandardError, ex12pProcess, [ex12pProcess, &errorOutput]() {
        const QByteArray chunk = ex12pProcess->readAllStandardError();
        errorOutput.append(chunk);
        qDebug().noquote() << "[ex12p stderr]" << QString::fromLocal8Bit(chunk);
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
    errorOutput.append(ex12pProcess->readAllStandardError());
    if (!errorOutput.isEmpty()) {
        qDebug().noquote() << "ex12p stderr full:" << QString::fromLocal8Bit(errorOutput);
    }

    {
        const QByteArray jsonBytes = extractJsonSegment(allOutput);

        // 将 JSON 保存到文本文件，不在终端显示
        const QString jsonOutPath = QStringLiteral("data/eigen_results.json");
        QFile out(jsonOutPath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(jsonBytes);
            out.close();
            qDebug() << "JSON 已保存到文件:" << jsonOutPath << ", 字节数:" << jsonBytes.size();
        }
        else {
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
                    const size_t eigenvectorLength = eigenvectors[0].size();
                    const size_t eigenNodeCount = eigenvectorLength / 3;
                    qDebug() << "eigenvector length:" << eigenvectorLength;
                    qDebug() << "eigen node count:" << eigenNodeCount;
                    qDebug() << "max valid eigen node id:" << (eigenNodeCount > 0 ? eigenNodeCount - 1 : 0);
                    if (eigenvectorLength % 3 != 0) {
                        qDebug() << "ERROR: eigenvector length is not divisible by 3.";
                    }
                }
            }
        }
    }

    int exitCode = ex12pProcess->exitCode();
    qDebug() << "ex12p exitCode:" << exitCode
        << "exitStatus:" << ex12pProcess->exitStatus()
        << "errorString:" << ex12pProcess->errorString();
    //qDebug() << "ex12p 程序执行完成，退出代码:" << exitCode;

    ex12pProcess->deleteLater();
    qDebug() << "ex12p 程序调用完成，主程序继续执行...";

}








