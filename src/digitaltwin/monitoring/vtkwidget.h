#ifndef VTKWIDGET_H
#define VTKWIDGET_H

#include <QWidget>
#include <QDebug>
#include <vtkeigen/eigen/Dense>
#include <iostream>
#include <cmath>
#include <QLabel>
#include <QRandomGenerator>
#include "fileparser.h"
#include <QGridLayout>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <array>
//#include "../xsim/mdichild_action_mchconfig.cpp"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkAxesActor.h>
#include <vtkCaptionActor2D.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkAssembly.h>
#include <vtkAssemblyNode.h>
#include <vtkSTLReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkTransform.h>
#include <vtkLine.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>

#include<vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

QT_BEGIN_NAMESPACE
namespace Ui { class VTKWidget; }
QT_END_NAMESPACE

class VTKWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VTKWidget(QWidget *parent = nullptr);
    ~VTKWidget();
    Ui::VTKWidget *ui;
    void ToolPathDisplay(vtkSmartPointer<vtkPoints> points, double *data, int index, double feed);
    void MachineMotion(double *MacPos, double *data, int index, double feed, double rpm);
    void AddTriangles(vtkRenderer* renderer, double* triangleArray, vtkIdType numTriangles, double* colorArray);

private:
    QVTKOpenGLNativeWidget* vtkWidget;
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    QList<Component> components;
    QMap<QString, vtkSmartPointer<vtkAssembly>> assemblyMap;

    static vtkSmartPointer<vtkPolyData> polyData;//ToolPath
    vtkSmartPointer<vtkActor> actor_wp;
    vtkSmartPointer<vtkPolyData> polydata_wp;

    void InitializeRenderer();
    vtkSmartPointer<vtkActor> LoadSTLFile(const QString& filename, QColor color);
    vtkSmartPointer<vtkAssembly> CreateComponentAssembly(const Component& component);
    void UpdateComponentTransform(vtkSmartPointer<vtkAssembly> assembly,
                                   double posX, double posY, double posZ,
                                   double rotA, double rotB);
    void SetCamera();
    void WorldAxesDisplay();
    void ClearSTL();
    void ClearToolPath();
    bool needClearToolpath = 0;

    int runtime = 0; ///< current render count.
    Eigen::Vector4d p_last_2wp; ///< starting point of tool path, based on workpiece coordinate system.

    QFile file2;
    QString currentTime;

protected:
    void resizeEvent(QResizeEvent* event) override;

//public slots:
public:
    void loadMchFile(const QString &filePath);
    //void linshi(double ppp[6], double act_rpm, double act_feed);
    void linshi(const std::array<double, 6>& ppp, double act_rpm, double act_feed);
    void linshi_toolindex(int index);
    void linshi_2(QStringList result);
    void onlengthToolTipUpdated(double lengthToolTip, const QVector<double>& workpieceOffset, const QVector<double>& manualOffset);
    void computeToolTip();

private:
    double force_port[3] = {0, 0, 0};
    double g_machcoor_to_vtk[3] = {-564.241, 396.641, 44.951};//z: +¦¤cnc +151.672 +µ¶³¤157.065 -600
    double toolTipLength;
    double g_workpieceOffset[3];
    double g_manualOffset[3];
    std::array<double, 8> mchData{};
    double v_rpm;

private:
    vtkSmartPointer<vtkMatrix4x4> cloneMatrix(vtkMatrix4x4* src) const;
    vtkSmartPointer<vtkMatrix4x4> multiplyMatrix(vtkMatrix4x4* left, vtkMatrix4x4* right) const;
    void printMatrix(const QString& name, vtkMatrix4x4* matrix) const;
    vtkSmartPointer<vtkMatrix4x4> getToolToWorldMatrixManual() const;
    vtkSmartPointer<vtkMatrix4x4> getWorkpieceToWorldMatrixManual() const;
    double normalizeRad(double angle);

signals:
    void componentsLoaded(const QList<Component>& components);
    void mchDatatoCutsim(std::array<double, 8> mchData);
};
#endif // VTKWIDGET_H
