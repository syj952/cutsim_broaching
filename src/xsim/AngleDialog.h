#pragma once

#ifndef ANGLEDIALOG_H
#define ANGLEDIALOG_H


#include <iostream>
#include <qdialog.h>
#include <QLabel>
#include "OcctView.h"
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <vector>
#include <array>
#include <utility>
using namespace std;

class AngleDialog : public QDialog {
    Q_OBJECT
public:
     AngleDialog(QWidget* parent, OcctView* view);
     ~AngleDialog();
public slots:
    void onSelectRakeFace();
    void onSelectClearanceFace();
    void onSetCuttingVelocity();
    void onSetCuttingDepth();
    void onRakeFaceSelected(const TopoDS_Face& face);
    void onClearanceFaceSelected(const TopoDS_Face& face);
    void onEdgeSelected(const TopoDS_Edge& edge); //
    //void onEdgeOffset();
	//void computeAngles();//<计算前后角
public:
     static vector<TopoDS_Face> rakeFaces;//<前刀面
     static vector<TopoDS_Face> clearanceFaces;//<后刀面
     static std::vector<vector<double>> pointsAndVec; //存储离散点的坐标和切向
     static std::vector<std::vector<std::vector<double>>> groupedPointsAndVec; //按切削刃分组存储离散点
     static gp_Vec m_cuttingVec, m_cuttingDep;  //切削方向向量
     static gp_Vec m_cuttingDepDir1; //齿高方向，注意
     static gp_Vec m_cuttingDepDir2; // 齿宽方向
     static gp_Vec m_offsetDir; // 偏移方向
     static double m_offsetDistance; // 偏移距离
     static int m_offsetNumber; // 偏移数量
     static array<int, 7> m_depthoffset; // 齿升和偏移设置
private:
    OcctView* m_view;
	QWidget* m_parent;
    
    QLabel* lblRakeStatus;
    QLabel* lblClearanceStatus;
    QPushButton* btnCuttingDir;
    QPushButton* btnCuttingDep;
	QPushButton* btnComputeAngles;
    QPushButton* btnEdgeOffset;
    QPushButton* btnTestSelectedFacesEdge;
signals:
    void selRakeFaceSignal();
	void selClearanceFaceSignal();

};

#endif