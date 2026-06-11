#include "mdichild.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <QFileDialog>
using namespace std;

int MdiChild::generateAbaqusINP(bool twoD_threeD, double V, double t, double lambda, double gamma, double alpha, double r, double miu)
{
    bool planestrain = true; 
    if (twoD_threeD) planestrain = false;    
    int LAGorALEorCEL = 2; // lag: 0, ALE: 1, CEL: 2
    switch (LAGorALEorCEL)
    {
    case 0:
        planestrain = true;
    case 1:
        planestrain = true;
    }
    if (planestrain) lambda = 0;
    if (t <=0 ) return -1;
    //double V, t, lambda, gamma, alpha, r;
    // Input parameters
    //gamma = -30; // rake angle(deg)
    //alpha = 5; // flank angle(deg)
    //lambda = 45; // inclination angle(deg)
    //r = 0; // edge radius(um)

    //V = 6; // cutting speed(m/min)
    //t = 150; // undeformed chip thickness(um)
    if(miu<0 || miu >1) miu = 0.15; // friction coefficient
    double Dt_VB = 0.0; // VB in mm
    double alpha_VB = alpha; // VB angle in degree, similar to the flank angle
    // Simulation setting
    int f = 100; // output frequency
    int s = 5; // mesh size (1-coarse to 10-fine)
    int b = 20; // mesh bias(1 - 10)
    if (b < 1) b = 0;
    int g = 2; // void height which is above the uncut chip

    int length2height_ratio = 2;
    int thickness2height_ratio = 2.5;
    if (lambda > 90) lambda = 180 - lambda;
    // Workpiece material (FGH96)
    vector<vector<double>> K_wp = { {9.3, 25}, {11.2, 200}, {14, 400}, {17.2, 600}, {20.1, 800} };
    vector<vector<double>> Cp_wp = { {390, 25}, {420, 200}, {455, 400}, {487, 600}, {520, 800} };
    vector<vector<double>> rho_wp = { {8320} }; // density (kg/m3)
    vector<vector<double>> E_wp = { {220e3, 0.33} }; // Young's modulus(MPa)-poisson's ratio
    vector<vector<double>> alpha_wp = { {1.11e-05, 25}, {1.19e-5, 200}, {1.33e-5, 400}, {1.44e-5, 600}, {1.51e-5, 800} };
    double eta_wp = 0.9; // inelastic heat fraction
    double A_wp = 1030; // JC parameters (MPa)
    double B_wp = 1220; // (MPa)
    double n_wp = 0.6372;
    double C_wp = 0.027;
    double epsilon0_wp = 0.001; // (1/s)
    double m_wp = 0.8244;
    double T0_wp = 25; // (℃)
    double Tm_wp = 1270; // (℃)

    // Cutting tool material (HSS)
    vector<vector<double>> K_t = { {19.21} }; // conductivity(W/m/℃)
    vector<vector<double>> Cp_t = { {420} }; // specific heat (W/m/℃)
    vector<vector<double>> rho_t = { {8200} }; // density (kg/m3)
    vector<vector<double>> E_t = { {245e3, 0.22} }; // Young's modulus(MPa)-poisson's ratio
    vector<vector<double>> alpha_t = { {1.1e-5} }; // thermal expansion (1/℃)
    double eta_t = 0.9; // inelastic heat fraction

    // Contact condition    
    vector<vector<double>> h = { {5000, 0}, {18000, 30}, {87000, 180}, {222000, 300}, {410000, 420}, {500000, 600} };


    /*string FileName = "gamma" + convertDoubleToString(gamma,0) + "_lambda" + convertDoubleToString(lambda, 0) + "_r" + convertDoubleToString(r, 0) + "_t" + convertDoubleToString(t, 0) + "_s" + std::to_string(s);*/

    QString defaultFileName = QString("Abaqus_V%1_t%2_lambda%3_gamma%4_alpha%5_r%6_s%7")
        .arg(int(V))
        .arg(int(t * 1000))
        .arg(int(lambda))     // 自动调用 QString::number
        .arg(int(gamma))  
        .arg(int(alpha))
        .arg(int(r * 1000))
        .arg(int(s));

    QString filepathName = QFileDialog::getSaveFileName(
        nullptr,
        tr("输出abaqus inp文件"),
        defaultFileName,  // 使用转换后的 QString
        tr("inp文件 (*.inp);;All Files (*.*)")
    );

    if (filepathName.isEmpty()) return 1;
    if (!filepathName.endsWith(".inp", Qt::CaseInsensitive)) {
        filepathName += ".inp";
    }

    // Open output file
    ofstream fid(filepathName.toUtf8().constData());
    if (!fid.is_open()) {
        cerr << "Error opening file!" << std::endl;
        return 1;
    }

    // Preprocess
    for (auto& val : Cp_wp) val[0] *= 1e6;
    for (auto& val : Cp_t) val[0] *= 1e6;
    for (auto& val : rho_wp) val[0] *= 1e-12;
    for (auto& val : rho_t) val[0] *= 1e-12;

    lambda = abs(lambda);
    //V = V / 60 * 1000;
    //t = t / 1000;
    //r = r / 1000;

    // workpiece defined by length*width*height (x*z*y)
    double W = 10 * t; // workpiece width(mm)
    double Lp = 4 * t;
    double Lc = (40 + 4) * t;
    double L = Lc + 2 * Lp; // workpiece length as well as cutting length(mm)
    double D = (15 + 6) * t; // workpiece height(mm)

    double Dv = 2 * t; // void gap(mm) (x+ and x- each have a void gap)
    //% "切削过程中，刀具-切屑接触长度与未变形切屑厚度的解析关系，给出公式"
    // 点击查看元宝的回答 https ://yb.tencent.com/s/GAoQBF92e4sp
    double Lf = 1.6 * tan(M_PI / 4 + atan(miu) - gamma * M_PI/ 180);// tool - chip contact length, should be later multiplied by the uncut chip thickness
    double Dc;
    if (g < Lf && Lf > 1) // the void above the chip should be enough for the tool - chip contact, bu smaller to restrict the chip curved to tontact the workpiece or the forming chip
    {
        Dc = (ceil(Lf) + 0.5) * t; //  chip flow zone(mm), avoid the formed chip reflow back to the forming chip. 
    }
    else
    {
        Dc = (g + 0.5) * t;
    }
    
    double Dl = Lc * tan(lambda * M_PI / 180); // chip deflection zone(mm)

    double Va = 1; // velocity(mm/s)
    if (V > Va) Va = V;
    double time_scale = 1;
    double mass_scale = 1;
    //if (V <= 1667) {
    //    time_scale = 1667 / V;
    //}
    Va = time_scale * Va;
    double delta1 = t / s; // workpiece mesh size
    double delta2;
    if (r <= 0 || r >= t) {
        delta2 = t / ceil(s / 3.0);
    }
    else if (r <= t / 10) {
        delta2 = t / 10 / ceil(s / 3.0);
    }
    else {
        delta2 = r / ceil(s / 3.0);
    }

    string step1 = "pre-cutting";
    double steptime1 = (Lp + 0.5 * W * tan(lambda * M_PI / 180)) / Va;
    string step2 = "cutting";
    double steptime2 = L / V;
    string step3 = "post-cutting";
    double steptime3 = (Lp + 0.5 * W * tan(lambda * M_PI / 180)) / Va;
    string step4 = "release";
    double steptime4 = 0.2 + 0.05 / steptime2 * 0.02;

    int f1 = ceil(f / steptime2 * steptime1); int f2 = f;
    int f3 = ceil(f / steptime2 * steptime3); int f4 = 50;
    double t1 = steptime1 / f1; double t2 = steptime2 / f2;
    double t3 = steptime3 / f3; double t4 = steptime4 / f4;


    // Write header
    fid << "*Heading\n";
    fid << "*Preprint, echo=NO, model=NO, history=NO, contact=NO\n";

    // Eulerian domain
    fid << "*Part, name=euler\n";
    int n_work, n_uncutchip; 
    double q_work, q_uncutchip;
    if (b == 0) {
        n_uncutchip = Dc / delta1;
        n_work = ceil(D / delta1);
    }
    else {
        q_uncutchip = (Dc - g * t - delta1 + b * delta1) / (Dc - g * t);
        n_uncutchip = ceil(log10(b) / log10(q_uncutchip));
        q_work = (D - delta1 + b * delta1) / D;
        n_work = ceil(log10(b) / log10(q_work));
    }
    vector<double> cv = linspace(0, 1, ceil(Dv / (delta1 * 4.0)) + 1);
    // first void mesh's length equals workspiece
    for (int i = 0; i < cv.size(); i++) cv[i] = pow(cv[i], (log(length2height_ratio * delta1 / Dv) / log(1 / ceil(Dv / (delta1 * 4.0)))));

    vector<double> ct = linspace(0, 1, ceil(Dc / delta1) + 1);
    vector<double> cl = linspace(0, 1, ceil(Dl / (delta1 * thickness2height_ratio)) + 1);
    cl.pop_back();
    vector<double> cx = linspace(0, 1, ceil(L / (delta1 * length2height_ratio)) + 1);
    cx.erase(cx.begin());
    cx.pop_back();
    vector<double> cy;
    for (int i = n_work; i >= 1; i--) cy.push_back((pow(q_work, n_work) - pow(q_work, i)) / (pow(q_work, n_work) - q_work));
    cy.pop_back();
    vector<double> cz;
    if (planestrain)
    {
        cz = linspace(0, 1, 2);
    }
    else {
        cz = linspace(0, 1, ceil(W / (delta1 * 2.5)) + 1);
        cz.erase(cz.begin());
        cz.pop_back();
    }

    vector<double> x, y, z;
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        for (int i = 0; i < cx.size(); i++) x.push_back(-L + L * cx[i]);
        for (int i = 0; i < cy.size(); i++) y.push_back(-D + D * cy[i]);
        for (int i = 0; i < ct.size(); i++) y.push_back(delta1 * i);
        break;
    case 2:

        for (int i = 0; i < cv.size(); i++) x.push_back(-L - Dv + Dv * cv[i]);
        for (int i = 0; i < cx.size(); i++) x.push_back(-L + L * cx[i]);
        for (int i = 0; i < cv.size(); i++) x.push_back(Dv * cv[i]);

        for (int i = 0; i < cy.size(); i++) y.push_back(-D + D * cy[i]);
        for (int i = 0; i < ct.size(); i++) y.push_back(Dc * ct[i]);
    }
    if (planestrain)
    {
        for (int i = 0; i < cz.size(); i++) z.push_back(W * cz[i]);
    }
    else {
        for (int i = 0; i < cv.size(); i++) z.push_back(-Dv + Dv * cv[i]);
        for (int i = 0; i < cz.size(); i++) z.push_back(W * cz[i]);
        for (int i = 0; i < cl.size(); i++) z.push_back(W + Dl * cl[i]);
        for (int i = 0; i < cv.size(); i++) z.push_back(W + Dl + Dv * cv[i]);
    }
    vector<int> Node_wp, Node_void, Node_bottom, Node_side;
    int l = x.size(), m = y.size(), n = z.size();
    fid << "*Node\n";
    int Node_index;
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        for (int i = 1; i <= l; i++) {
            for (int j = 1; j <= m; j++) {
                Node_index = j + (i - 1) * m;
                fid << setw(7) << Node_index << ", "
                    << fixed << setprecision(12) << setw(13) << x[i - 1] << ", "
                    << fixed << setprecision(12) << setw(13) << y[j - 1] << "\n";
                Node_wp.push_back(Node_index);
                if (y[j - 1] == -D || ((x[i - 1] == -L || x[i - 1] == 0) && y[j - 1] <= -D)) {
                    Node_bottom.push_back(Node_index);
                }
            }
        }
        break;
    case 2:
        for (int i = 1; i <= l; i++) {
            for (int j = 1; j <= m; j++) {
                for (int k = 1; k <= n; k++) {
                    Node_index = k + (j - 1) * n + (i - 1) * m * n;
                    fid << setw(7) << Node_index << ", "
                        << fixed << setprecision(12) << setw(13) << x[i - 1] << ", "
                        << fixed << setprecision(12) << setw(13) << y[j - 1] << ", "
                        << fixed << setprecision(12) << setw(13) << z[k - 1] << "\n";

                    if (x_in_range(-L, 0, x[i - 1]) && y[j - 1] <= t &&
                        z_in_range(0, W, z[k - 1])) {
                        Node_wp.push_back(Node_index);
                        if (y[j - 1] == -D || ((x[i - 1] == -L || x[i - 1] == 0 || z[k - 1] == 0 || z[k - 1] == W) && y[j - 1] <= -D)) {
                            Node_bottom.push_back(Node_index);
                        }
                        if (z[k - 1] == 0 || z[k - 1] == W) {
                            Node_side.push_back(Node_index);
                        }
                    }
                    else {
                        Node_void.push_back(Node_index);
                    }
                }
            }
        }
    }
    vector<int> Element_wp, Element_void;
    fid << "*Element, type=EC3D8RT\n";
    int Element_index;
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        for (int i = 1; i <= l - 1; i++) {
            for (int j = 1; j <= m - 1; j++) {
                Element_index = j + (i - 1) * (m - 1);
                int V1 = j + 1 + i * m;
                int V2 = j + 1 + (i - 1) * m;
                int V3 = j + (i - 1) * m;
                int V4 = j + i * m;
                fid << setw(7) << Element_index << ", " << setw(7) << V1 << ", " << setw(7) << V2 << ", "
                    << setw(7) << V3 << ", " << setw(7) << V4 << "\n";
                Element_wp.push_back(Element_index);
            }
        }
        break;
    case 2:
        for (int i = 1; i <= l - 1; i++) {
            for (int j = 1; j <= m - 1; j++) {
                for (int k = 1; k <= n - 1; k++) {
                    Element_index = k + (j - 1) * (n - 1) + (i - 1) * (m - 1) * (n - 1);
                    int V1 = k + 1 + j * n + i * m * n;
                    int V2 = k + 1 + j * n + (i - 1) * m * n;
                    int V3 = k + j * n + (i - 1) * m * n;
                    int V4 = k + j * n + i * m * n;
                    int V5 = k + 1 + (j - 1) * n + i * m * n;
                    int V6 = k + 1 + (j - 1) * n + (i - 1) * m * n;
                    int V7 = k + (j - 1) * n + (i - 1) * m * n;
                    int V8 = k + (j - 1) * n + i * m * n;
                    fid << setw(7) << Element_index << ", " << setw(7) << V1 << ", " << setw(7) << V2 << ", "
                        << setw(7) << V3 << ", " << setw(7) << V4 << ", " << setw(7) << V5 << ", "
                        << setw(7) << V6 << ", " << setw(7) << V7 << ", " << setw(7) << V8 << "\n";

                    if (x[i - 1] >= -L && x[i] <= 0 && y[j] <= t && z[k - 1] >= 0 && z[k] <= W) {
                        Element_wp.push_back(Element_index);
                    }
                    else {
                        Element_void.push_back(Element_index);
                    }
                }
            }
        }
        break;
    }
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        fid << "*Nset, nset=workpiece, generate\n";
        fid << "1, " << setw(7) << Node_index << ", 1\n";
        fid << "*Elset, elset=workpiece, generate\n";
        fid << "1, " << setw(7) << Element_index << ", 1\n";

        fid << "*Solid Section, elset=workpiece, material=material_wp\n";
        fid << "1\n";
        break;
    case 2:
        // Write node sets
        fid << "*Nset, nset=euler, generate\n";
        fid << "1, " << setw(7) << Node_index << ", 1\n";
        fid << "*Elset, elset=euler, generate\n";
        fid << "1, " << setw(7) << Element_index << ", 1\n";
        fid << "*Nset, nset=workpiece\n";
        for (size_t i = 0; i < Node_wp.size(); i++) {
            fid << setw(7) << Node_wp[i] << ",";
            if ((i + 1) % 15 == 0 || i == Node_wp.size() - 1) fid << "\n";
        }
        fid << "*Nset, nset=void\n";
        for (size_t i = 0; i < Node_void.size(); i++) {
            fid << setw(7) << Node_void[i] << ",";
            if ((i + 1) % 15 == 0 || i == Node_void.size() - 1) fid << "\n";
        }
        fid << "*Elset, elset=workpiece\n";
        for (size_t i = 0; i < Element_wp.size(); i++) {
            fid << setw(7) << Element_wp[i] << ",";
            if ((i + 1) % 15 == 0 || i == Element_wp.size() - 1) fid << "\n";
        }
        fid << "*Elset, elset=void\n";
        for (size_t i = 0; i < Element_void.size(); i++) {
            fid << setw(7) << Element_void[i] << ",";
            if ((i + 1) % 15 == 0 || i == Element_void.size() - 1) fid << "\n";
        }

        fid << "*Eulerian Section, elset=euler\n";
        fid << "material_wp, material_wp\n";
        fid << "*Surface, type=EULERIAN MATERIAL, name=material_wp\n";
        fid << "material_wp\n";
    }
    fid << "*End Part\n";

    // Tool
    //cx和 cy：这两行定义了在 x 和 y 方向上的归一化网格坐标，范围从 0 到 1。步长由 Dt / delta2和 Dc / delta2计算并向上取整（ceil函数）决定，这通常是为了控制网格的密度或分辨率 。cz：z 方向的网格定义取决于布尔变量 plane_strain（可能表示是否为平面应变问题）：如果 plane_strain为真，cz简单地取[0, 1]，可能表示在 z 方向只考虑一个单位厚度或无需细分。如果 plane_strain为假，则 z 方向的网格点根据(W + 2 * Dv) / (delta1 * 4)的计算结果确定步长，从而生成从 0 到 1 的序列

    fid << "*Part, name=tool\n";
    Dc = Dc + 2 * t;
    /////////////////////////////////////////////////////////////
    // tool thickness(mm)//////////
    /////////////////////////////////////////////////////////////
    double Dt = Dc * 1; 
    if (Dt < Dt_VB * 2) Dt = 2 * Dt_VB;
    // Calculate grid parameters
    cx.clear(); cy.clear(); cz.clear();
    if (b == 0) {
        cx = linspace(0, 1, ceil(Dt / delta2) + 1);
        cy = linspace(0, 1, ceil(Dc / delta2) + 1);
    }
    else
    {
        double q; int nn;
        q = (Dt - delta2 + b * delta2) / Dt;
        nn = ceil(log10(b) / log10(q) * 1.2);
        for (int i = 1; i <= nn; i++) cx.push_back((pow(q, i) - q) / (pow(q, nn) - q)); // = linspace(0, 1, ceil(Dt / delta2) + 1);
        q = (Dc - delta2 + b * delta2) / Dc;
        nn = ceil(log10(b) / log10(q) * 1.2);
        for (int i = 1; i <= nn; i++) cy.push_back((pow(q, i) - q) / (pow(q, nn) - q));
    }

    if (planestrain) {
        cz = linspace(0, 1, 2);
    }
    else {
        cz = linspace(0, 1, ceil((W + 2 * Dv) / (delta1 * 4)) + 1);
    }

    // Tool geometry calculations
    //这部分代码定义了四个点的坐标(a_i, b_i)，可能代表一个特定几何形状（如梯形或特定边界）的角点
    //(a1, b1)是原点(0, 0)。
    //(a2, b2)的 x 坐标由 Dc和角度 gamma的正切值计算，y 坐标为 Dc。
    //(a3, b3)和(a4, b4)的 x 坐标基于 a1、a2中的较大值加上 Dt，y 坐标分别为 Dc和由 a2 + Dt与角度 alpha的正切计算的值。
    double a1 = 0; double b1 = 0;
    double a2 = Dc * tan(gamma * M_PI / 180); double b2 = Dc;
    double a3 = max(a1, a2) + Dt_VB + Dc * tan(gamma * M_PI / 180); double b3 = Dc;
    double a4 = max(a1, a2) + Dt_VB; double b4 = (a2 + Dt_VB) * tan(alpha_VB * M_PI / 180);
    //这部分计算了一个圆的圆心(ao, bo)以及该圆与从原点出发的两条射线（斜率可能为 b2 / a2和 b4 / a4）的切点坐标(a5, b5)和(a6, b6)
    //计算中使用了半径 r和勾股定理等几何关系。
    double ao = r * (sqrt(a2 * a2 + b2 * b2) / a2 + sqrt(a4 * a4 + b4 * b4) / a4) / (b2 / a2 - b4 / a4);
    double bo = b2 / a2 * ao - r * sqrt(a2 * a2 + b2 * b2) / a2;
    double a5 = (ao + b4 / a4 * bo) / (1 + (b4 / a4) * (b4 / a4));
    double b5 = b4 / a4 * a5;
    double a6 = (ao + b2 / a2 * bo) / (1 + (b2 / a2) * (b2 / a2));
    double b6 = b2 / a2 * a6;
    //最后这部分计算了一系列系数 h1到 h6。这些系数很可能用于后续构建一个几何变换矩阵，以便将不规则区域映射到规则网格上进行计算
    double h6 = -(a2 + a4 - a3 - (a4 - a3) / (b4 - b3) * (b2 - b3 + b4)) /
        (a2 - a3 - (a4 - a3) / (b4 - b3) * (b2 - b3));
    double h5 = (-(b2 - b3 + b4) - (b2 - b3) * h6) / (b4 - b3);
    double h4 = -(-(b3 - b4) - (b3 - b4) * h5 - b3 * h6);
    double h3 = -(-b4 - b4 * h5);
    double h2 = -(a4 - a3) - (a4 - a3) * h5 + a3 * h6;
    double h1 = -(-a3 + h2 - a3 * h5 - a3 * h6);

    double cx_Vb_x = Dt_VB / (h1 - Dt_VB * h5);
    int index_Vb_x = 0; double temp = 1e9;
    for (int i = 0; i < cx.size(); i++) {
        if (temp > abs(cx[i] - cx_Vb_x)) {
            temp = abs(cx[i] - cx_Vb_x);
            index_Vb_x = i;
        }
    }
    //[~, index_Vb_x] = min(abs(cx - cx_Vb_x));
    double w = h5 * cx[index_Vb_x] + 1;
    double x_VB = h1 * cx[index_Vb_x] / w;
    double y_VB = h3 * cx[index_Vb_x] / w;

    double a12 = 0; double b12 = 0;
    double a22 = Dc * tan(gamma * M_PI / 180);
    double b22 = Dc - y_VB;
    double a32 = max(a12, a22) + Dt;
    double b32 = Dc - y_VB;
    double a42 = max(a12, a22) + Dt;
    double b42 = (a2 + Dt) * tan(alpha * M_PI / 180);

    double h62 = -(a22 + a42 - a32 - (a42 - a32) / (b42 - b32) * (b22 - b32 + b42)) / (a22 - a32 - (a42 - a32) / (b42 - b32) * (b22 - b32));
    double h52 = (-(b22 - b32 + b42) - (b22 - b32) * h62) / (b42 - b32);
    double h42 = -(-(b32 - b42) - (b32 - b42) * h52 - b32 * h62);
    double h32 = -(-b42 - b42 * h52);
    double h22 = -(a42 - a32) - (a42 - a32) * h52 + a32 * h62;
    double h12 = -(-a32 + h22 - a3 * h52 - a32 * h62);

    double tooloffset_y = bo - r;
    double tooloffset_z;
    if (t < r * (1 + sin(gamma * M_PI / 180))) {
        tooloffset_z = ao - sqrt(r * r - (r - min(r, t)) * (r - min(r, t)));
    }
    else {
        tooloffset_z = min(ao - r, t * tan(gamma * M_PI / 180));
    }

    // Generate nodes
    vector<int> Node_boundary;
    l = cx.size(), m = cy.size(), n = cz.size();
    int Element_rakeface = 0, Element_flankface = 0;

    fid << "*Node\n";
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        for (int i = 1; i <= l; i++) {
            for (int j = 1; j <= m; j++) {
                Node_index = j + (i - 1) * m;  // Adjusting for 1-based indexing
                double w = h5 * cx[i - 1] + h6 * cy[j - 1] + 1;
                double xx = (h1 * cx[i - 1] + h2 * cy[j - 1]) / w;
                double yy = (h3 * cx[i - 1] + h4 * cy[j - 1]) / w;

                if (i - 1 > index_Vb_x) {
                    w = h52 * (cx[i - 1] - cx[index_Vb_x]) + h62 * cy[j - 1] + 1;
                    xx = (h12 * (cx[i - 1] - cx[index_Vb_x]) + h22 * cy[j - 1]) / w + x_VB;
                    yy = (h32 * (cx[i - 1] - cx[index_Vb_x]) + h42 * cy[j - 1]) / w + y_VB;
                }

                if (cx[i - 1] == 1 || cy[j - 1] == 1) {
                    Node_boundary.push_back(Node_index);
                }

                if (r != 0) {
                    if (inradius({ a1, b1 }, { a5, b5 }, { ao, bo }, { a6, b6 }, { xx, yy })) {
                        double l0 = sqrt((xx - ao) * (xx - ao) + (yy - bo) * (yy - bo));
                        double l1 = (b2 * (ao - xx) - a2 * (bo - yy)) / sqrt(a2 * a2 + b2 * b2);
                        double l2 = (a4 * (bo - yy) - b4 * (ao - xx)) / sqrt(a4 * a4 + b4 * b4);
                        double lm = max(l1, l2);
                        xx = ao - (ao - xx) * lm / l0;
                        yy = bo - (bo - yy) * lm / l0;
                    }
                }
                xx = xx - tooloffset_z;
                yy = yy - tooloffset_y;

                fid << setw(7) << Node_index << ", "
                    << fixed << setprecision(12) << setw(13) << xx << ", "
                    << setw(13) << yy << "\n";
            }
        }
        break;
    case 2:
        for (int i = 1; i <= l; i++) {
            for (int j = 1; j <= m; j++) {
                for (int k = 1; k <= n; k++) {
                    Node_index = k + (j - 1) * n + (i - 1) * m * n;  // Adjusting for 1-based indexing
                    double w = h5 * cx[i - 1] + h6 * cy[j - 1] + 1;
                    double xx = (h1 * cx[i - 1] + h2 * cy[j - 1]) / w;
                    double yy = (h3 * cx[i - 1] + h4 * cy[j - 1]) / w;

                    if (i - 1 > index_Vb_x) {
                        w = h52 * (cx[i - 1] - cx[index_Vb_x]) + h62 * cy[j - 1] + 1;
                        xx = (h12 * (cx[i - 1] - cx[index_Vb_x]) + h22 * cy[j - 1]) / w + x_VB;
                        yy = (h32 * (cx[i - 1] - cx[index_Vb_x]) + h42 * cy[j - 1]) / w + y_VB;
                    }

                    if (cx[i - 1] == 1 || cy[j - 1] == 1) {
                        Node_boundary.push_back(Node_index);
                    }

                    if (r != 0) {
                        if (inradius({ a1, b1 }, { a5, b5 }, { ao, bo }, { a6, b6 }, { xx, yy })) {
                            double l0 = sqrt((xx - ao) * (xx - ao) + (yy - bo) * (yy - bo));
                            double l1 = (b2 * (ao - xx) - a2 * (bo - yy)) / sqrt(a2 * a2 + b2 * b2);
                            double l2 = (a4 * (bo - yy) - b4 * (ao - xx)) / sqrt(a4 * a4 + b4 * b4);
                            double lm = max(l1, l2);
                            xx = ao - (ao - xx) * lm / l0;
                            yy = bo - (bo - yy) * lm / l0;
                        }
                    }

                    double zz = (W + 2 * Dv) * cz[k - 1] - Dv;
                    xx = xx + zz * tan(lambda * M_PI / 180) - tooloffset_z;
                    yy = yy - tooloffset_y;

                    fid << setw(7) << Node_index << ", "
                        << fixed << setprecision(12) << setw(13) << xx << ", "
                        << setw(13) << yy << ", " << setw(13) << zz << "\n";
                }
            }
        }
        break;
    }
    // Generate elements
    vector<int> Element_rakeface_vec, Element_flankface_vec;
    fid << "*Element, type=C3D8T\n";
    int node_index = 1;  // Assuming nodes start at 1
    int element_index = 1;
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        for (int i = 1; i <= l - 1; i++) {
            for (int j = 1; j <= m - 1; j++) {
                Element_index = j + (i - 1) * (m - 1);  // 1-based

                int V1 = j + 1 + i * m;
                int V2 = j + 1 + (i - 1) * m;
                int V3 = j + (i - 1) * m;
                int V4 = j + i * m;

                if (cx[i - 1] == 0) {
                    Element_rakeface_vec.push_back(Element_index);
                }

                if (cy[j - 1] == 0) {
                    Element_flankface_vec.push_back(Element_index);
                }

                // Adjusting node numbers to be sequential and correct
                // This is a placeholder - actual node numbering needs to match your node generation
                fid << setw(7) << Element_index << ", "
                    << setw(7) << V1 << ", " << setw(7) << V2 << ", " << setw(7) << V3 << ", " << setw(7) << V4 << "\n";
            }
        }
        break;
    case 2:
        for (int i = 1; i <= l - 1; i++) {
            for (int j = 1; j <= m - 1; j++) {
                for (int k = 1; k <= n - 1; k++) {
                    Element_index = k + (j - 1) * (n - 1) + (i - 1) * (m - 1) * (n - 1);  // 1-based

                    int V1 = k + 1 + j * n + i * m * n;
                    int V2 = k + 1 + j * n + (i - 1) * m * n;
                    int V3 = k + j * n + (i - 1) * m * n;
                    int V4 = k + j * n + i * m * n;
                    int V5 = k + 1 + (j - 1) * n + i * m * n;
                    int V6 = k + 1 + (j - 1) * n + (i - 1) * m * n;
                    int V7 = k + (j - 1) * n + (i - 1) * m * n;
                    int V8 = k + (j - 1) * n + i * m * n;

                    if (cx[i - 1] == 0) {
                        Element_rakeface_vec.push_back(Element_index);
                    }

                    if (cy[j - 1] == 0) {
                        Element_flankface_vec.push_back(Element_index);
                    }

                    // Adjusting node numbers to be sequential and correct
                    // This is a placeholder - actual node numbering needs to match your node generation
                    fid << setw(7) << Element_index << ", "
                        << setw(7) << V1 << ", " << setw(7) << V2 << ", " << setw(7) << V3 << ", " << setw(7) << V4 << ", "
                        << setw(7) << V5 << ", " << setw(7) << V6 << ", " << setw(7) << V7 << ", " << setw(7) << V8 << "\n";
                }
            }
        }
        break;
    }

    // Generate node and element sets
    fid << "*Nset, nset=tool, generate\n";
    fid << "1, " << setw(7) << Node_index << ", 1\n";
    fid << "*Elset, elset=tool, generate\n";
    fid << "1, " << setw(7) << Element_index << ", 1\n";
    fid << "*Solid Section, elset=tool, material=material_t\n";
    fid << ",\n";
    fid << "*End Part\n";

    // Assembly section
    fid << "*Assembly, name=Assembly\n";
    fid << "*Instance, name=Euler, part=euler\n";
    fid << "*End Instance\n";
    fid << "*Instance, name=Tool, part=tool\n";
    fid << "0., 0., 0\n";
    fid << "*End Instance\n";

    // Reference node
    fid << "*Node\n";
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        fid << "1,  " << fixed << setprecision(12) << setw(13) << (a3 + 0.5 * W * tan(lambda * M_PI / 180) - tooloffset_z) << ", "
            << setw(13) << (b3 - tooloffset_y) << "\n";
        break;
    case 2:
        fid << "1,  " << fixed << setprecision(12) << setw(13) << (a3 + 0.5 * W * tan(lambda * M_PI / 180) - tooloffset_z) << ", "
            << setw(13) << (b3 - tooloffset_y) << ", " << setw(13) << (0.5 * W) << "\n";
    }
    fid << "*Nset, nset=RP\n";
    fid << "1,\n";

    // Bottom nodes
    fid << "*Nset, nset=bottom, instance=Euler\n";
    for (size_t i = 0; i < Node_bottom.size(); i++) {
        fid << setw(7) << Node_bottom[i] << ",";
        if ((i + 1) % 15 == 0 || i == Node_bottom.size() - 1) {
            fid << "\n";
        }
    }
    switch (LAGorALEorCEL)
    {
    case 2:
        // Side nodes
        fid << "*Nset, nset=side, instance=Euler\n";
        for (size_t i = 0; i < Node_side.size(); i++) {
            fid << setw(7) << Node_side[i] << ",";
            if ((i + 1) % 15 == 0 || i == Node_side.size() - 1) {
                fid << "\n";
            }
        }
    }
    // Boundary nodes
    fid << "*Nset, nset=boundary, instance=Tool\n";
    for (size_t i = 0; i < Node_boundary.size(); i++) {
        fid << setw(7) << Node_boundary[i] << ",";;
        if ((i + 1) % 15 == 0 || i == Node_boundary.size() - 1) {
            fid << "\n";
        }
    }

    // Rake and flank faces
    fid << "*Nset, nset=RP\n";
    fid << "1,\n";

    fid << "*Elset, elset=rakeface, internal, instance=Tool\n";
    for (size_t i = 0; i < Element_rakeface_vec.size(); i++) {
        fid << setw(7) << Element_rakeface_vec[i] << ",";;
        if ((i + 1) % 15 == 0 || i == Element_rakeface_vec.size() - 1) {
            fid << "\n";
        }
    }

    fid << "*Elset, elset=flankface, internal, instance=Tool\n";
    for (size_t i = 0; i < Element_flankface_vec.size(); i++) {
        fid << setw(7) << Element_flankface_vec[i] << ",";;
        if ((i + 1) % 15 == 0 || i == Element_flankface_vec.size() - 1) {
            fid << "\n";
        }
    }

    // Interface surface
    fid << "*Surface, type=ELEMENT, name=interface\n";
    fid << "rakeface, S4\n";
    fid << "flankface, S2\n";

    // Rigid body
    fid << "*Rigid Body, ref node=Rp, elset=Tool.tool\n";
    fid << "*End Assembly\n";


    // Material properties section
    fid << "*Material, name=material_wp\n"; // workpiece material

    fid << "*Conductivity\n";
    for (int i = 0; i < K_wp.size(); i++) {
        for (int j = 0; j < K_wp[i].size(); j++) {
            fid << setprecision(12) << K_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Specific Heat\n";
    for (int i = 0; i < Cp_wp.size(); i++) {
        for (int j = 0; j < Cp_wp[i].size(); j++) {
            fid << setprecision(12) << Cp_wp[i][j] / time_scale / mass_scale << ",";
        }
        fid << "\n";
    }

    fid << "*Density\n";
    for (int i = 0; i < rho_wp.size(); i++) {
        for (int j = 0; j < rho_wp[i].size(); j++) {
            fid << setprecision(12) << rho_wp[i][j] * mass_scale << ",";
        }
        fid << "\n";
    }

    fid << "*Elastic\n";
    for (int i = 0; i < E_wp.size(); i++) {
        for (int j = 0; j < E_wp[i].size(); j++) {
            fid << setprecision(12) << E_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Expansion\n";
    for (int i = 0; i < alpha_wp.size(); i++) {
        for (int j = 0; j < alpha_wp[i].size(); j++) {
            fid << setprecision(12) << alpha_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Inelastic Heat Fraction\n";
    fid << setprecision(12) << eta_wp << "\n";

    fid << "*Plastic, hardening=JOHNSON COOK\n";
    fid << setprecision(12) << A_wp << "," << B_wp << "," << n_wp << "," << m_wp << "," << Tm_wp << "," << T0_wp << "\n";
    fid << "*Rate Dependent, type=JOHNSON COOK\n";
    fid << setprecision(12) << C_wp << "," << epsilon0_wp * time_scale << "\n";

    fid << "*Material, name=material_t\n"; // cutting tool material

    fid << "*Conductivity\n";
    for (int i = 0; i < K_t.size(); i++) {
        for (int j = 0; j < K_t[i].size(); j++) {
            fid << setprecision(12) << K_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Specific Heat\n";
    for (int i = 0; i < Cp_t.size(); i++) {
        for (int j = 0; j < Cp_t[i].size(); j++) {
            fid << setprecision(12) << Cp_t[i][j] / time_scale / mass_scale << ",";
        }
        fid << "\n";
    }

    fid << "*Density\n";
    for (int i = 0; i < rho_t.size(); i++) {
        for (int j = 0; j < rho_t[i].size(); j++) {
            fid << setprecision(12) << rho_t[i][j] * mass_scale << ",";
        }
        fid << "\n";
    }

    fid << "*Elastic\n";
    for (int i = 0; i < E_t.size(); i++) {
        for (int j = 0; j < E_t[i].size(); j++) {
            fid << setprecision(12) << E_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Expansion\n";
    for (int i = 0; i < alpha_t.size(); i++) {
        for (int j = 0; j < alpha_t[i].size(); j++) {
            fid << setprecision(12) << alpha_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Inelastic Heat Fraction\n";
    fid << setprecision(12) << eta_t << "\n";

    fid << "*Surface Interaction, name=wp_t\n"; // tool-chip contact

    fid << "*Friction\n";
    fid << setprecision(12) << miu << "\n";

    fid << "*Surface Behavior, pressure-overclosure=HARD\n";

    fid << "*Gap Conductance, pressure\n";
    for (int i = 0; i < h.size(); i++) {
        fid << setprecision(12) << h[i][0] << "," << h[i][1] << "\n";
    }

    fid << "*Gap Heat Generation\n";
    fid << "1., 0.5\n";
    switch (LAGorALEorCEL)
    {
    case 2:
        fid << "*Initial Conditions, type=VOLUME FRACTION\n";
        fid << "Euler.workpiece, Euler.material_wp, 1.\n";
    }
    fid << "*Initial Conditions, type=TEMPERATURE\n";
    fid << "Tool.tool, " << T0_wp << "\n";
    switch (LAGorALEorCEL)
    {
    case 0:
    case 1:
        fid << "*Initial Conditions, type=TEMPERATURE\n";
        fid << "Euler.Workpiece," << T0_wp << "\n";
        break;
    case 2:
        fid << "*Initial Conditions, type=TEMPERATURE\n";
        fid << "Euler.euler," << T0_wp << "\n";
    }
    // Step 1 section
    fid << "*Step, name=" << step1 << ", nlgeom = YES\n"; // step1 name from input

    fid << "*Dynamic Temperature-displacement, Explicit\n";
    fid << ", " << setprecision(12) << steptime1 << "\n"; // time increment for step1

    fid << "*Bulk Viscosity\n";
    fid << "0.06, 1.2\n"; // bulk viscosity parameters
    switch (LAGorALEorCEL)
    {
    case 2:
        if (planestrain) {
            // Velocity boundary conditions for side set
            fid << "*Boundary, type=VELOCITY\n";
            fid << "side, 3, 3\n"; // z-direction velocity fixed
            fid << "side, 4, 4\n"; // rotational velocity fixed
            fid << "side, 5, 5\n"; // rotational velocity fixed
        }
    }
    // Velocity boundary conditions for bottom set
    fid << "*Boundary, type=VELOCITY\n";
    fid << "bottom, 1, 1\n"; // x-direction velocity fixed
    fid << "bottom, 2, 2\n"; // y-direction velocity fixed
    fid << "bottom, 3, 3\n"; // z-direction velocity fixed
    fid << "bottom, 4, 4\n"; // rotational velocity fixed
    fid << "bottom, 5, 5\n"; // rotational velocity fixed
    fid << "bottom, 6, 6\n"; // rotational velocity fixed

    // Velocity boundary conditions for RP (reference point)
    fid << "*Boundary, type=VELOCITY\n";
    fid << "RP, 1, 1, " << -Va << "\n"; // x-direction velocity set to -Va
    fid << "RP, 2, 2\n"; // y-direction velocity free
    fid << "RP, 3, 3\n"; // z-direction velocity free
    fid << "RP, 4, 4\n"; // rotational velocity free
    fid << "RP, 5, 5\n"; // rotational velocity free
    fid << "RP, 6, 6\n"; // rotational velocity free

    // Temperature boundary conditions
    fid << "*Boundary\n";
    fid << "bottom, 11, 11, " << setprecision(12) << T0_wp << "\n"; // bottom set temperature fixed at T0_wp

    fid << "*Boundary\n";
    fid << "boundary, 11, 11, " << setprecision(12) << T0_wp << "\n"; // other boundary temperature fixed at T0_wp

    switch (LAGorALEorCEL)
    {
    case 1:
        fid << "*Adaptive Mesh Controls, name=Ada-1, curvature refinement=5.\n";
        fid << "0.5, 0., 0.5\n";
        fid << "*Adaptive Mesh, elset=EULER.WORKPIECE, controls=Ada-1, frequency=1, mesh sweeps=10, op=NEW\n";
    }

    // Contact definition
    fid << "*Contact, op=NEW\n"; // new contact definition
    fid << "*Contact Inclusions\n";
    fid << "interface , Euler.material_wp\n"; // contact between interface and Euler.material_wp

    fid << "*Contact Property Assignment\n";
    fid << " ,  , wp_t\n"; // assign wp_t contact property

    // Restart settings
    fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

    // Field output settings
    fid << "*Output, field, number interval=" << f1 << "\n"; // field output frequency
    fid << "*Node Output\n";
    fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement

    fid << "*Element Output, directions=YES\n";
    fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables

    fid << "*Contact Output\n";
    fid << "CSTRESS, \n"; // contact stress output

    // History output settings
    fid << "*Output, history, time interval=" << setprecision(12) << t1 << "\n"; // history output time interval
    fid << "*Node Output, nset=RP\n";
    fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP

    fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << t1 << "\n"; // preselected history variables

    fid << "*End Step\n"; // end of step definition

    // Step 2 section
    fid << "*Step, name=" << step2 << ", nlgeom = YES\n"; // step2 name from input
    fid << "*Dynamic Temperature-displacement, Explicit\n";
    fid << "," << setprecision(12) << steptime2 << "\n"; // time increment for step2
    fid << "*Bulk Viscosity\n";
    fid << "0.06, 1.2\n"; // bulk viscosity parameters

    // Velocity boundary condition for RP (reference point)
    fid << "*Boundary, type=VELOCITY\n";
    fid << "RP, 1, 1, " << setprecision(12) << -V << "\n"; // x-direction velocity set to -V

    switch (LAGorALEorCEL)
    {
    case 1:
        fid << "*Adaptive Mesh, elset=EULER.WORKPIECE, controls=Ada-1, frequency=1, mesh sweeps=10, op=NEW\n";
    }

    // Restart settings
    fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

    // Field output settings
    fid << "*Output, field, number interval=" << f2 << "\n"; // field output frequency
    fid << "*Node Output\n";
    fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement
    fid << "*Element Output, directions=YES\n";
    fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables
    fid << "*Contact Output\n";
    fid << "CSTRESS, \n"; // contact stress output

    // History output settings
    fid << "*Output, history, time interval=" << setprecision(12) << t2 << "\n"; // history output time interval
    fid << "*Node Output, nset=RP\n";
    fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP
    fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << t2 << "\n"; // preselected history variables

    fid << "*End Step\n"; // end of step 2 definition

    // Step 3 section
    //fid << "*Step, name=" << step3 << ", nlgeom = YES\n"; // step3 name from input
    //fid << "*Dynamic Temperature-displacement, Explicit\n";
    //fid << "," << setprecision(12) << steptime3 << "\n"; // time increment for step3
    //fid << "*Bulk Viscosity\n";
    //fid << "0.06, 1.2\n"; // bulk viscosity parameters

    //// Velocity boundary condition for RP (reference point)
    //fid << "*Boundary, type=VELOCITY\n";
    //fid << "RP, 1, 1, " << -V << "\n"; // x-direction velocity set to -V

    //switch (LAGorALEorCEL)
    //{
    //case 1:
    //    fid << "*Adaptive Mesh, elset=EULER.WORKPIECE, controls=Ada-1, frequency=1, mesh sweeps=10, op=NEW\n";
    //}

    //// Restart settings
    //fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

    //// Field output settings
    //fid << "*Output, field, number interval=" << f3 << "\n"; // field output frequency
    //fid << "*Node Output\n";
    //fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement
    //fid << "*Element Output, directions=YES\n";
    //fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables
    //fid << "*Contact Output\n";
    //fid << "CSTRESS, \n"; // contact stress output

    //// History output settings
    //fid << "*Output, history, time interval=" << setprecision(12) << t3 << "\n"; // history output time interval
    //fid << "*Node Output, nset=RP\n";
    //fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP
    //fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << t3 << "\n"; // preselected history variables

    //fid << "*End Step\n"; // end of step 3 definition

    // Step 4 section
    fid << "*Step, name=" << step4 << ", nlgeom = YES\n"; // step4 name from input
    fid << "*Dynamic Temperature-displacement, Explicit\n";
    fid << "," << steptime4 << "\n"; // time increment for step4
    fid << "*Bulk Viscosity\n";
    fid << "0.06, 1.2\n"; // bulk viscosity parameters

    // Velocity boundary condition for RP (reference point)
    fid << "*Boundary, type=VELOCITY\n";
    fid << "RP, 1, 1, 0\n"; // x-direction velocity set to 0 (stationary)

    switch (LAGorALEorCEL)
    {
    case 1:
        fid << "*Adaptive Mesh, op=NEW\n";
    }

    // Contact definition
    fid << "** Interaction: general_contact\n"; // new contact definition
    fid << "*Contact, op=NEW\n"; // new contact definition

    // Restart settings
    fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

    // Field output settings
    fid << "*Output, field, number interval=" << f4 << "\n"; // field output frequency
    fid << "*Node Output\n";
    fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement
    fid << "*Element Output, directions=YES\n";
    fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables
    fid << "*Contact Output\n";
    fid << "CSTRESS, \n"; // contact stress output

    // History output settings
    fid << "*Output, history, time interval=" << setprecision(12) << t4 << "\n"; // history output time interval
    fid << "*Node Output, nset=RP\n";
    fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP
    fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << t4 << "\n", t4; // preselected history variables

    fid << "*End Step\n"; // end of step 4 definition

    // Close the output file
    fid.close();
    return 0;
};

int MdiChild::generateAbaqusMultiCutsINP(QString filepathName, int DimensionType, int SimulationType,
    double DistanceBetweenTool, std::vector<double> VelocityVector,
    std::vector<double> UCTVector, std::vector<double> LambdaVector,
    std::vector<double> GammaVector, std::vector<double> AlphaVector,
    std::vector<double> HoneRadiusVector, std::vector<double> MiuVector,
    std::vector<double> VBVector, std::vector<double> AlphaVBVector)
{
    // SimulationType, lag: 0, ALE: 1, CEL: 2
    bool PlaneStrain = true;
    if (DimensionType == 3) PlaneStrain = false;
    switch (SimulationType)
    {
    case 0:
        PlaneStrain = true;
        break;
    case 1:
        PlaneStrain = true;
    }

    // Workpiece material (FGH96)
    vector<vector<double>> K_wp = { {9.3, 25}, {11.2, 200}, {14, 400}, {17.2, 600}, {20.1, 800} };
    vector<vector<double>> Cp_wp = { {390, 25}, {420, 200}, {455, 400}, {487, 600}, {520, 800} };
    vector<vector<double>> rho_wp = { {8320} }; // density (kg/m3)
    vector<vector<double>> E_wp = { {220e3, 0.33} }; // Young's modulus(MPa)-poisson's ratio
    vector<vector<double>> alpha_wp = { {1.11e-05, 25}, {1.19e-5, 200}, {1.33e-5, 400}, {1.44e-5, 600}, {1.51e-5, 800} };
    double eta_wp = 0.9; // inelastic heat fraction
    double A_wp = 1030; // JC parameters (MPa)
    double B_wp = 1220; // (MPa)
    double n_wp = 0.6372;
    double C_wp = 0.027;
    double epsilon0_wp = 0.001; // (1/s)
    double m_wp = 0.8244;
    double T0_wp = 25; // (℃)
    double Tm_wp = 1270; // (℃)

    // Cutting tool material (HSS)
    vector<vector<double>> K_t = { {19.21} }; // conductivity(W/m/℃)
    vector<vector<double>> Cp_t = { {420} }; // specific heat (W/m/℃)
    vector<vector<double>> rho_t = { {8200} }; // density (kg/m3)
    vector<vector<double>> E_t = { {245e3, 0.22} }; // Young's modulus(MPa)-poisson's ratio
    vector<vector<double>> alpha_t = { {1.1e-5} }; // thermal expansion (1/℃)
    double eta_t = 0.9; // inelastic heat fraction

    // Contact condition    
    vector<vector<double>> h = { {5000, 0}, {18000, 30}, {87000, 180}, {222000, 300}, {410000, 420}, {500000, 600} };

    // Preprocess
    for (auto& val : Cp_wp) val[0] *= 1e6;
    for (auto& val : Cp_t) val[0] *= 1e6;
    for (auto& val : rho_wp) val[0] *= 1e-12;
    for (auto& val : rho_t) val[0] *= 1e-12;


    // Simulation setting
    int FieldOutputFrequency = 100; // output frequency
    int UCTElementYDirCount = 5; // Uncut Chip Element count in Y direction (1-coarse to 10-fine) 
    int MeshBias = 5; // mesh bias(1 - 10)
    if (MeshBias < 1) MeshBias = 0;


    // workpiece defined by length*width*height (x*z*y)
    double MaxUCT = *(std::max_element(UCTVector.begin(), UCTVector.end()));
    double MinUCT = *(std::min_element(UCTVector.begin(), UCTVector.end()));
    double MaxLambda = *(std::max_element(LambdaVector.begin(), LambdaVector.end()));
    double WorkThickness = 10 * MaxUCT; // workpiece width(mm) in z direction
    double InitialGapBetweenToolWork = 1 * MaxUCT;
    double WorkLength = (40 + 4) * MaxUCT; // workpiece length as well as cutting length(mm)
    double OtherUCTSum = std::accumulate(UCTVector.begin(), UCTVector.end(), 0.0) - UCTVector[0];
    double BulkHeight = (15 + 6) * MaxUCT; // bulk  height(mm)

    double VoidGapLength = 2 * UCTVector[0]; // void gap(mm) (x+ and x- each have a void gap)

    double ChipDeflectionThickness = WorkLength * tan(MaxLambda * M_PI / 180); // chip deflection zone(mm)

    double ChipElementCharactorLength = UCTVector.back() / UCTElementYDirCount; // chip mesh size
    double CutterElementCharactorLength = ChipElementCharactorLength;
    if (HoneRadiusVector.back() >= UCTVector.back()) {
        ChipElementCharactorLength = UCTVector.back() / UCTElementYDirCount;
    }
    else if (HoneRadiusVector.back() <= UCTVector.back() / UCTElementYDirCount) {
        ChipElementCharactorLength = HoneRadiusVector.back() / 4;
    }
    else {
        ChipElementCharactorLength = UCTVector.back() / UCTElementYDirCount;
    }

    double ElementLength2HeightRatio = 2;
    double ElementThickness2HeightRatio = 2.5; //Hexelement ratio of thickness(y) to height (y) 
    double ChipFlowHeight; // chip flow zone(mm), avoid the formed chip reflow back to the forming chip.
    double ToolChipContactLengthNormalized = 1.6 * tan(M_PI / 4 + atan(MiuVector[0]) - GammaVector[0] * M_PI / 180);// tool - chip contact length, should be later multiplied by the uncut chip thickness
    ChipFlowHeight = (ceil(ToolChipContactLengthNormalized) - 0.5) * UCTVector[0]; //  

    int ChipFlowElementYDirCount, FirstUCTElementYDirCount, OtherUCTsElementYDirCount, BulkElementYDirCount;
    double ChipFlowElementYDirExponent, FirstUCTElementYDirExponent, OtherUCTsElementYDirExponent, BulkElementYDirExponent;
    if (MeshBias == 0) {
        ChipFlowElementYDirCount = ceil(ChipFlowHeight / ChipElementCharactorLength);
        FirstUCTElementYDirCount = ceil(UCTVector[0] / ChipElementCharactorLength);
        OtherUCTsElementYDirCount = ceil(OtherUCTSum / ChipElementCharactorLength);
        BulkElementYDirCount = ceil(BulkHeight / ChipElementCharactorLength);
    }
    else {
        //ChipFlowElementYDirCount = ceil(ChipFlowHeight / ChipElementCharactorLength);
        //FirstUCTElementYDirCount = ceil(UCTVector[0] / ChipElementCharactorLength);
        //OtherUCTsElementYDirCount = ceil(OtherUCTSum / ChipElementCharactorLength);
        //BulkElementYDirCount = ceil(BulkHeight / ChipElementCharactorLength);

        ChipFlowElementYDirExponent = (ChipFlowHeight - ChipElementCharactorLength + MeshBias * ChipElementCharactorLength) / ChipFlowHeight;
        FirstUCTElementYDirExponent = (UCTVector[0] - ChipElementCharactorLength + MeshBias * ChipElementCharactorLength) / UCTVector[0];
        OtherUCTsElementYDirExponent = (OtherUCTSum - ChipElementCharactorLength + MeshBias * ChipElementCharactorLength) / OtherUCTSum;
        BulkElementYDirExponent = (BulkHeight - ChipElementCharactorLength + MeshBias * ChipElementCharactorLength) / BulkHeight;

        ChipFlowElementYDirCount = ceil(log10(MeshBias) / log10(ChipFlowElementYDirExponent));
        FirstUCTElementYDirCount = ceil(log10(MeshBias) / log10(FirstUCTElementYDirExponent));
        OtherUCTsElementYDirCount = ceil(log10(MeshBias) / log10(OtherUCTsElementYDirExponent));
        BulkElementYDirCount = ceil(log10(MeshBias) / log10(BulkElementYDirExponent));
    }

    // first workpiece's right(+x) and left(-x) void mesh
    vector<double> NormlizedDistanceVoid = linspace(0, 1, ceil(VoidGapLength / (ChipElementCharactorLength * 4.0)) + 1);
    for (int i = 0; i < NormlizedDistanceVoid.size(); i++) NormlizedDistanceVoid[i] = pow(NormlizedDistanceVoid[i], (log(ChipElementCharactorLength * ElementLength2HeightRatio / VoidGapLength) / log(1 / ceil(VoidGapLength / (ChipElementCharactorLength * 4.0)))));

    vector<double> NormlizedDistanceWorkpieceLength = linspace(0, 1, ceil(WorkLength / (ChipElementCharactorLength * ElementLength2HeightRatio)) + 1);
    NormlizedDistanceWorkpieceLength.erase(NormlizedDistanceWorkpieceLength.begin());
    NormlizedDistanceWorkpieceLength.pop_back();

    vector<double> NormlizedDistanceBulkHeight;
    for (int i = BulkElementYDirCount; i > 1; i--)
        NormlizedDistanceBulkHeight.push_back((pow(BulkElementYDirExponent, BulkElementYDirCount) - pow(BulkElementYDirExponent, i)) / (pow(BulkElementYDirExponent, BulkElementYDirCount) - BulkElementYDirExponent));
    //NormlizedDistanceBulkHeight.pop_back();

    vector<double> NormlizedDistanceOtherUCTs;
    for (int i = OtherUCTsElementYDirCount; i > 1; i--)
        NormlizedDistanceOtherUCTs.push_back((pow(OtherUCTsElementYDirExponent, OtherUCTsElementYDirCount) - pow(OtherUCTsElementYDirExponent, i)) / (pow(OtherUCTsElementYDirExponent, OtherUCTsElementYDirCount) - OtherUCTsElementYDirExponent));
    //NormlizedDistanceOtherUCTs.pop_back();

    vector<double> NormlizedDistanceFirstUCTHeight;
    for (int i = FirstUCTElementYDirCount; i >= 1; i--)
        NormlizedDistanceFirstUCTHeight.push_back((pow(FirstUCTElementYDirExponent, FirstUCTElementYDirCount) - pow(FirstUCTElementYDirExponent, i)) / (pow(FirstUCTElementYDirExponent, FirstUCTElementYDirCount) - FirstUCTElementYDirExponent));
    //NormlizedDistanceFirstUCTHeight.pop_back();

    vector<double> NormlizedDistanceChipFlow; // = linspace(0, 1, ceil(ChipFlowHeight / ChipElementCharactorLength) + 1);
    for (int i = ChipFlowElementYDirCount - 1; i >= 1; i--)
        NormlizedDistanceChipFlow.push_back((pow(ChipFlowElementYDirExponent, ChipFlowElementYDirCount) - pow(ChipFlowElementYDirExponent, i)) / (pow(ChipFlowElementYDirExponent, ChipFlowElementYDirCount) - ChipFlowElementYDirExponent));

    vector<double> NormlizedDistanceChipDeflection = linspace(0, 1, ceil(ChipDeflectionThickness / (ChipElementCharactorLength * ElementThickness2HeightRatio)) + 1);
    NormlizedDistanceChipDeflection.pop_back();

    vector<double> NormlizedDistanceWorkThickness;
    if (PlaneStrain)
    {
        NormlizedDistanceWorkThickness = linspace(0, 1, 2);
    }
    else {
        NormlizedDistanceWorkThickness = linspace(0, 1, ceil(WorkThickness / (ChipFlowElementYDirExponent * 2.5)) + 1);
        NormlizedDistanceWorkThickness.erase(NormlizedDistanceWorkThickness.begin());
        NormlizedDistanceWorkThickness.pop_back();
    }

    vector<double> x, y, z;
    switch (SimulationType)
    {
    case 0:
    case 1:
        for (int i = 0; i < NormlizedDistanceWorkpieceLength.size(); i++) x.push_back(-WorkLength + WorkLength * NormlizedDistanceWorkpieceLength[i]);

        for (int i = 0; i < NormlizedDistanceBulkHeight.size(); i++) y.push_back(-BulkHeight + BulkHeight * NormlizedDistanceBulkHeight[i] - OtherUCTSum);
        for (int i = 0; i < NormlizedDistanceOtherUCTs.size(); i++) y.push_back(-OtherUCTSum + OtherUCTSum * NormlizedDistanceOtherUCTs[i]);
        for (int i = 0; i < NormlizedDistanceFirstUCTHeight.size(); i++) y.push_back(UCTVector[0] * NormlizedDistanceFirstUCTHeight[i]);
        //for (int i = 0; i < NormlizedDistanceChipFlow.size(); i++) y.push_back(ChipFlowElementYDirExponent * i);
        break;
    case 2:

        for (int i = 0; i < NormlizedDistanceVoid.size(); i++) x.push_back(-WorkLength - VoidGapLength + VoidGapLength * NormlizedDistanceVoid[i]);
        for (int i = 0; i < NormlizedDistanceWorkpieceLength.size(); i++) x.push_back(-WorkLength + WorkLength * NormlizedDistanceWorkpieceLength[i]);
        for (int i = 0; i < NormlizedDistanceVoid.size(); i++) x.push_back(VoidGapLength * NormlizedDistanceVoid[i]);

        for (int i = 0; i < NormlizedDistanceBulkHeight.size(); i++) y.push_back(-BulkHeight + BulkHeight * NormlizedDistanceBulkHeight[i] - OtherUCTSum);
        for (int i = 0; i < NormlizedDistanceOtherUCTs.size(); i++) y.push_back(-OtherUCTSum + OtherUCTSum * NormlizedDistanceOtherUCTs[i]);
        for (int i = 0; i < NormlizedDistanceFirstUCTHeight.size(); i++) y.push_back(UCTVector[0] * NormlizedDistanceFirstUCTHeight[i]);
        for (int i = 0; i < NormlizedDistanceChipFlow.size(); i++) y.push_back(UCTVector[0] + NormlizedDistanceChipFlow[i] * ChipFlowHeight);
    }
    if (PlaneStrain)
    {
        for (int i = 0; i < NormlizedDistanceWorkThickness.size(); i++) z.push_back(WorkThickness * NormlizedDistanceWorkThickness[i]);
    }
    else {
        for (int i = 0; i < NormlizedDistanceVoid.size(); i++) z.push_back(-VoidGapLength + VoidGapLength * NormlizedDistanceVoid[i]);
        for (int i = 0; i < NormlizedDistanceWorkThickness.size(); i++) z.push_back(WorkThickness * NormlizedDistanceWorkThickness[i]);
        for (int i = 0; i < NormlizedDistanceChipDeflection.size(); i++) z.push_back(WorkThickness + ChipDeflectionThickness * NormlizedDistanceChipDeflection[i]);
        for (int i = 0; i < NormlizedDistanceVoid.size(); i++) z.push_back(WorkThickness + ChipDeflectionThickness + VoidGapLength * NormlizedDistanceVoid[i]);
    }
    // Open output file
    ofstream fid(filepathName.toUtf8().constData());
    if (!fid.is_open()) {
        cerr << "Error opening file!" << std::endl;
        return 1;
    }
    // Write header
    fid << "*Heading\n";
    fid << "*Preprint, echo=NO, model=NO, history=NO, contact=NO\n";

    // Eulerian domain
    fid << "*Part, name=euler\n";
    vector<int> Node_wp, Node_void, Node_bottom, Node_side;
    int l = x.size(), m = y.size(), n = z.size();
    fid << "*Node\n";
    int Node_index;
    switch (SimulationType)
    {
    case 0:
    case 1:
        for (int i = 1; i <= l; i++) {
            for (int j = 1; j <= m; j++) {
                Node_index = j + (i - 1) * m;
                fid << setw(7) << Node_index << ", "
                    << fixed << setprecision(12) << setw(13) << x[i - 1] << ", "
                    << fixed << setprecision(12) << setw(13) << y[j - 1] << "\n";
                Node_wp.push_back(Node_index);
                if (y[j - 1] == -BulkHeight - OtherUCTSum || ((x[i - 1] == -WorkLength || x[i - 1] == 0) && y[j - 1] <= -BulkHeight - OtherUCTSum)) {
                    Node_bottom.push_back(Node_index);
                }
            }
        }
        break;
    case 2:
        for (int i = 1; i <= l; i++) {
            for (int j = 1; j <= m; j++) {
                for (int k = 1; k <= n; k++) {
                    Node_index = k + (j - 1) * n + (i - 1) * m * n;
                    fid << setw(7) << Node_index << ", "
                        << fixed << setprecision(12) << setw(13) << x[i - 1] << ", "
                        << fixed << setprecision(12) << setw(13) << y[j - 1] << ", "
                        << fixed << setprecision(12) << setw(13) << z[k - 1] << "\n";

                    if (x_in_range(-WorkLength, 0, x[i - 1]) && y[j - 1] <= UCTVector[0] &&
                        z_in_range(0, WorkThickness, z[k - 1])) {
                        Node_wp.push_back(Node_index);
                        if (y[j - 1] == -BulkHeight - OtherUCTSum || ((x[i - 1] == -WorkLength || x[i - 1] == 0 || z[k - 1] == 0 || z[k - 1] == WorkThickness) && y[j - 1] <= -BulkHeight - OtherUCTSum)) {
                            Node_bottom.push_back(Node_index);
                        }
                        if (z[k - 1] == 0 || z[k - 1] == WorkThickness) {
                            Node_side.push_back(Node_index);
                        }
                    }
                    else {
                        Node_void.push_back(Node_index);
                    }
                }
            }
        }
    }
    vector<int> Element_wp, Element_void;
    fid << "*Element, type=EC3D8RT\n";
    int Element_index;
    switch (SimulationType)
    {
    case 0:
    case 1:
        for (int i = 1; i <= l - 1; i++) {
            for (int j = 1; j <= m - 1; j++) {
                Element_index = j + (i - 1) * (m - 1);
                int V1 = j + 1 + i * m;
                int V2 = j + 1 + (i - 1) * m;
                int V3 = j + (i - 1) * m;
                int V4 = j + i * m;
                fid << setw(7) << Element_index << ", " << setw(7) << V1 << ", " << setw(7) << V2 << ", "
                    << setw(7) << V3 << ", " << setw(7) << V4 << "\n";
                Element_wp.push_back(Element_index);
            }
        }
        break;
    case 2:
        for (int i = 1; i <= l - 1; i++) {
            for (int j = 1; j <= m - 1; j++) {
                for (int k = 1; k <= n - 1; k++) {
                    Element_index = k + (j - 1) * (n - 1) + (i - 1) * (m - 1) * (n - 1);
                    int V1 = k + 1 + j * n + i * m * n;
                    int V2 = k + 1 + j * n + (i - 1) * m * n;
                    int V3 = k + j * n + (i - 1) * m * n;
                    int V4 = k + j * n + i * m * n;
                    int V5 = k + 1 + (j - 1) * n + i * m * n;
                    int V6 = k + 1 + (j - 1) * n + (i - 1) * m * n;
                    int V7 = k + (j - 1) * n + (i - 1) * m * n;
                    int V8 = k + (j - 1) * n + i * m * n;
                    fid << setw(7) << Element_index << ", " << setw(7) << V1 << ", " << setw(7) << V2 << ", "
                        << setw(7) << V3 << ", " << setw(7) << V4 << ", " << setw(7) << V5 << ", "
                        << setw(7) << V6 << ", " << setw(7) << V7 << ", " << setw(7) << V8 << "\n";

                    if (x[i - 1] >= -WorkLength && x[i] <= 0 && y[j] <= UCTVector[0] && z[k - 1] >= 0 && z[k] <= WorkThickness) {
                        Element_wp.push_back(Element_index);
                    }
                    else {
                        Element_void.push_back(Element_index);
                    }
                }
            }
        }
        break;
    }
    switch (SimulationType)
    {
    case 0:
    case 1:
        fid << "*Nset, nset=workpiece, generate\n";
        fid << "1, " << setw(7) << Node_index << ", 1\n";
        fid << "*Elset, elset=workpiece, generate\n";
        fid << "1, " << setw(7) << Element_index << ", 1\n";

        fid << "*Solid Section, elset=workpiece, material=material_wp\n";
        fid << "1\n";
        break;
    case 2:
        // Write node sets
        fid << "*Nset, nset=euler, generate\n";
        fid << "1, " << setw(7) << Node_index << ", 1\n";
        fid << "*Elset, elset=euler, generate\n";
        fid << "1, " << setw(7) << Element_index << ", 1\n";
        fid << "*Nset, nset=workpiece\n";
        for (size_t i = 0; i < Node_wp.size(); i++) {
            fid << setw(7) << Node_wp[i] << ",";
            if ((i + 1) % 15 == 0 || i == Node_wp.size() - 1) fid << "\n";
        }
        fid << "*Nset, nset=void\n";
        for (size_t i = 0; i < Node_void.size(); i++) {
            fid << setw(7) << Node_void[i] << ",";
            if ((i + 1) % 15 == 0 || i == Node_void.size() - 1) fid << "\n";
        }
        fid << "*Elset, elset=workpiece\n";
        for (size_t i = 0; i < Element_wp.size(); i++) {
            fid << setw(7) << Element_wp[i] << ",";
            if ((i + 1) % 15 == 0 || i == Element_wp.size() - 1) fid << "\n";
        }
        fid << "*Elset, elset=void\n";
        for (size_t i = 0; i < Element_void.size(); i++) {
            fid << setw(7) << Element_void[i] << ",";
            if ((i + 1) % 15 == 0 || i == Element_void.size() - 1) fid << "\n";
        }

        fid << "*Eulerian Section, elset=euler\n";
        fid << "material_wp, material_wp\n";
        fid << "*Surface, type=EULERIAN MATERIAL, name=material_wp\n";
        fid << "material_wp\n";
    }
    fid << "*End Part\n";


    // Tool part
    vector<vector<int>> Element_rakeface_vecs, Element_flankface_vecs;
    vector <double> a3Vector, b3Vector, tooloffset_xVector, tooloffset_yVector, tooloffset_zVector;
    vector<vector<int>> ToolsNode_boundary;
    double PreviousChipFlowHeight = ChipFlowHeight;
    vector<double> WorkYOffsetVector, ToolYOffsetVector;
    WorkYOffsetVector.push_back(0); ToolYOffsetVector.push_back(0);
    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++)
    {
        vector<double> cx, cy, cz;
        double Dt_VB = VBVector[index_cutter]; // VB in mm
        double alpha_VB = AlphaVBVector[index_cutter]; // VB angle in degree, similar to the flank angle
        double r = HoneRadiusVector[index_cutter];
        double t = UCTVector[index_cutter];
        double lambda = LambdaVector[index_cutter];
        ToolChipContactLengthNormalized = 1.6 * tan(M_PI / 4 + atan(MiuVector[index_cutter]) - GammaVector[index_cutter] * M_PI / 180);// tool - chip contact length, should be later multiplied by the uncut chip thickness
        ChipFlowHeight = (ceil(ToolChipContactLengthNormalized) - 0.5) * UCTVector[index_cutter]; //  
        if (index_cutter > 0)
        {
            WorkYOffsetVector.push_back(PreviousChipFlowHeight + UCTVector[index_cutter - 1] - ChipFlowHeight);
            ToolYOffsetVector.push_back(WorkYOffsetVector[index_cutter] - UCTVector[index_cutter]);
        }
        PreviousChipFlowHeight = ChipFlowHeight;
        // Tool
        //cx和 cy：这两行定义了在 x 和 y 方向上的归一化网格坐标，范围从 0 到 1。步长由 Dt / CutterElementCharactorLength和 Dc / CutterElementCharactorLength计算并向上取整（ceil函数）决定，这通常是为了控制网格的密度或分辨率 。NormlizedDistanceWorkThickness：z 方向的网格定义取决于布尔变量 plane_strain（可能表示是否为平面应变问题）：如果 plane_strain为真，NormlizedDistanceWorkThickness简单地取[0, 1]，可能表示在 z 方向只考虑一个单位厚度或无需细分。如果 plane_strain为假，则 z 方向的网格点根据(W + 2 * Dv) / (ChipFlowElementYDirExponent * 4)的计算结果确定步长，从而生成从 0 到 1 的序列

        fid << "*Part, name=tool_" << index_cutter << "\n";
        double ToolHeight = ChipFlowHeight + 3 * UCTVector[index_cutter];
        /////////////////////////////////////////////////////////////
        // tool thickness(mm):Dt+Dt_VB
        /////////////////////////////////////////////////////////////
        double Dt = ToolHeight * 1;
        if (Dt < Dt_VB * 2) Dt = 2 * Dt_VB;

        // Calculate grid parameters        
        if (MeshBias == 0) {
            cx = linspace(0, 1, ceil(Dt / CutterElementCharactorLength) + 1);
            cy = linspace(0, 1, ceil(ToolHeight / CutterElementCharactorLength) + 1);
        }
        else
        {
            double q; int nn;
            q = (Dt - CutterElementCharactorLength + MeshBias * CutterElementCharactorLength) / Dt;
            nn = ceil(log10(MeshBias) / log10(q) * 1.2);
            for (int i = 1; i <= nn; i++) cx.push_back((pow(q, i) - q) / (pow(q, nn) - q)); // = linspace(0, 1, ceil(Dt / CutterElementCharactorLength) + 1);
            q = (ToolHeight - CutterElementCharactorLength + MeshBias * CutterElementCharactorLength) / ToolHeight;
            nn = ceil(log10(MeshBias) / log10(q) * 1.2);
            for (int i = 1; i <= nn; i++) cy.push_back((pow(q, i) - q) / (pow(q, nn) - q));
        }

        if (PlaneStrain) {
            cz = linspace(0, 1, 2);
        }
        else {
            cz = linspace(0, 1, ceil((WorkThickness + 2 * VoidGapLength) / (ChipFlowElementYDirExponent * 4)) + 1);
        }

        // Tool geometry calculations
        //这部分代码定义了四个点的坐标(a_i, b_i)，可能代表一个特定几何形状（如梯形或特定边界）的角点
        //(a1, b1)是原点(0, 0)。
        //(a2, b2)的 x 坐标由 ToolHeight和角度 gamma的正切值计算，y 坐标为 ToolHeight。
        //(a3, b3)和(a4, b4)的 x 坐标基于 a1、a2中的较大值加上 Dt，y 坐标分别为 Dc和由 a2 + Dt与角度 alpha的正切计算的值。
        double a1 = 0; double b1 = 0;
        double a2 = ToolHeight * tan(GammaVector[index_cutter] * M_PI / 180); double b2 = ToolHeight;
        double a3 = max(a1, a2) + Dt_VB + ToolHeight * tan(GammaVector[index_cutter] * M_PI / 180); double b3 = ToolHeight;
        double a4 = max(a1, a2) + Dt_VB; double b4 = (a2 + Dt_VB) * tan(alpha_VB * M_PI / 180);
        //这部分计算了一个圆的圆心(ao, bo)以及该圆与从原点出发的两条射线（斜率可能为 b2 / a2和 b4 / a4）的切点坐标(a5, b5)和(a6, b6)
        //计算中使用了半径 r和勾股定理等几何关系。
        double ao = r * (sqrt(a2 * a2 + b2 * b2) / a2 + sqrt(a4 * a4 + b4 * b4) / a4) / (b2 / a2 - b4 / a4);
        double bo = b2 / a2 * ao - r * sqrt(a2 * a2 + b2 * b2) / a2;
        double a5 = (ao + b4 / a4 * bo) / (1 + (b4 / a4) * (b4 / a4));
        double b5 = b4 / a4 * a5;
        double a6 = (ao + b2 / a2 * bo) / (1 + (b2 / a2) * (b2 / a2));
        double b6 = b2 / a2 * a6;
        //最后这部分计算了一系列系数 h1到 h6。这些系数很可能用于后续构建一个几何变换矩阵，以便将不规则区域映射到规则网格上进行计算
        double h6 = -(a2 + a4 - a3 - (a4 - a3) / (b4 - b3) * (b2 - b3 + b4)) /
            (a2 - a3 - (a4 - a3) / (b4 - b3) * (b2 - b3));
        double h5 = (-(b2 - b3 + b4) - (b2 - b3) * h6) / (b4 - b3);
        double h4 = -(-(b3 - b4) - (b3 - b4) * h5 - b3 * h6);
        double h3 = -(-b4 - b4 * h5);
        double h2 = -(a4 - a3) - (a4 - a3) * h5 + a3 * h6;
        double h1 = -(-a3 + h2 - a3 * h5 - a3 * h6);

        double cx_Vb_x = Dt_VB / (h1 - Dt_VB * h5);
        int index_Vb_x = 0; double temp = 1e9;
        for (int i = 0; i < cx.size(); i++) {
            if (temp > abs(cx[i] - cx_Vb_x)) {
                temp = abs(cx[i] - cx_Vb_x);
                index_Vb_x = i;
            }
        }
        //[~, index_Vb_x] = min(abs(cx - cx_Vb_x));
        double w = h5 * cx[index_Vb_x] + 1;
        double x_VB = h1 * cx[index_Vb_x] / w;
        double y_VB = h3 * cx[index_Vb_x] / w;

        double a12 = 0; double b12 = 0;
        double a22 = ToolHeight * tan(GammaVector[index_cutter] * M_PI / 180);
        double b22 = ToolHeight - y_VB;
        double a32 = max(a12, a22) + Dt;
        double b32 = ToolHeight - y_VB;
        double a42 = max(a12, a22) + Dt;
        double b42 = (a2 + Dt) * tan(AlphaVector[index_cutter] * M_PI / 180);

        double h62 = -(a22 + a42 - a32 - (a42 - a32) / (b42 - b32) * (b22 - b32 + b42)) / (a22 - a32 - (a42 - a32) / (b42 - b32) * (b22 - b32));
        double h52 = (-(b22 - b32 + b42) - (b22 - b32) * h62) / (b42 - b32);
        double h42 = -(-(b32 - b42) - (b32 - b42) * h52 - b32 * h62);
        double h32 = -(-b42 - b42 * h52);
        double h22 = -(a42 - a32) - (a42 - a32) * h52 + a32 * h62;
        double h12 = -(-a32 + h22 - a3 * h52 - a32 * h62);

        double tooloffset_y = bo - r;
        double tooloffset_x;
        if (t < r * (1 + sin(GammaVector[index_cutter] * M_PI / 180))) {
            tooloffset_x = ao - sqrt(r * r - (r - min(r, t)) * (r - min(r, t)));
        }
        else {
            tooloffset_x = min(ao - r, t * tan(GammaVector[index_cutter] * M_PI / 180));
        }

        // Generate nodes
        vector<int> Node_boundary;
        l = cx.size(), m = cy.size(), n = cz.size();
        int Element_rakeface = 0, Element_flankface = 0;

        fid << "*Node\n";
        switch (SimulationType)
        {
        case 0:
        case 1:
            for (int i = 1; i <= l; i++) {
                for (int j = 1; j <= m; j++) {
                    Node_index = j + (i - 1) * m;  // Adjusting for 1-based indexing
                    double w = h5 * cx[i - 1] + h6 * cy[j - 1] + 1;
                    double xx = (h1 * cx[i - 1] + h2 * cy[j - 1]) / w;
                    double yy = (h3 * cx[i - 1] + h4 * cy[j - 1]) / w;

                    if (i - 1 > index_Vb_x) {
                        w = h52 * (cx[i - 1] - cx[index_Vb_x]) + h62 * cy[j - 1] + 1;
                        xx = (h12 * (cx[i - 1] - cx[index_Vb_x]) + h22 * cy[j - 1]) / w + x_VB;
                        yy = (h32 * (cx[i - 1] - cx[index_Vb_x]) + h42 * cy[j - 1]) / w + y_VB;
                    }

                    if (cx[i - 1] == 1 || cy[j - 1] == 1) {
                        Node_boundary.push_back(Node_index);
                    }

                    if (r != 0) {
                        if (inradius({ a1, b1 }, { a5, b5 }, { ao, bo }, { a6, b6 }, { xx, yy })) {
                            double l0 = sqrt((xx - ao) * (xx - ao) + (yy - bo) * (yy - bo));
                            double l1 = (b2 * (ao - xx) - a2 * (bo - yy)) / sqrt(a2 * a2 + b2 * b2);
                            double l2 = (a4 * (bo - yy) - b4 * (ao - xx)) / sqrt(a4 * a4 + b4 * b4);
                            double lm = max(l1, l2);
                            xx = ao - (ao - xx) * lm / l0;
                            yy = bo - (bo - yy) * lm / l0;
                        }
                    }
                    xx = xx - tooloffset_x;
                    yy = yy - tooloffset_y;

                    fid << setw(7) << Node_index << ", "
                        << fixed << setprecision(12) << setw(13) << xx << ", "
                        << setw(13) << yy << "\n";
                }
            }
            break;
        case 2:
            for (int i = 1; i <= l; i++) {
                for (int j = 1; j <= m; j++) {
                    for (int k = 1; k <= n; k++) {
                        Node_index = k + (j - 1) * n + (i - 1) * m * n;  // Adjusting for 1-based indexing
                        double w = h5 * cx[i - 1] + h6 * cy[j - 1] + 1;
                        double xx = (h1 * cx[i - 1] + h2 * cy[j - 1]) / w;
                        double yy = (h3 * cx[i - 1] + h4 * cy[j - 1]) / w;

                        if (i - 1 > index_Vb_x) {
                            w = h52 * (cx[i - 1] - cx[index_Vb_x]) + h62 * cy[j - 1] + 1;
                            xx = (h12 * (cx[i - 1] - cx[index_Vb_x]) + h22 * cy[j - 1]) / w + x_VB;
                            yy = (h32 * (cx[i - 1] - cx[index_Vb_x]) + h42 * cy[j - 1]) / w + y_VB;
                        }

                        if (cx[i - 1] == 1 || cy[j - 1] == 1) {
                            Node_boundary.push_back(Node_index);
                        }

                        if (r != 0) {
                            if (inradius({ a1, b1 }, { a5, b5 }, { ao, bo }, { a6, b6 }, { xx, yy })) {
                                double l0 = sqrt((xx - ao) * (xx - ao) + (yy - bo) * (yy - bo));
                                double l1 = (b2 * (ao - xx) - a2 * (bo - yy)) / sqrt(a2 * a2 + b2 * b2);
                                double l2 = (a4 * (bo - yy) - b4 * (ao - xx)) / sqrt(a4 * a4 + b4 * b4);
                                double lm = max(l1, l2);
                                xx = ao - (ao - xx) * lm / l0;
                                yy = bo - (bo - yy) * lm / l0;
                            }
                        }

                        double zz = (WorkThickness + 2 * VoidGapLength) * cz[k - 1] - VoidGapLength;
                        xx = xx + zz * tan(lambda * M_PI / 180) - tooloffset_x;
                        yy = yy - tooloffset_y;

                        fid << setw(7) << Node_index << ", "
                            << fixed << setprecision(12) << setw(13) << xx << ", "
                            << setw(13) << yy << ", " << setw(13) << zz << "\n";
                    }
                }
            }
            break;
        }
        // Generate elements
        vector<int> Element_rakeface_vec, Element_flankface_vec;
        fid << "*Element, type=C3D8T\n";
        int node_index = 1;  // Assuming nodes start at 1
        int element_index = 1;
        switch (SimulationType)
        {
        case 0:
        case 1:
            for (int i = 1; i <= l - 1; i++) {
                for (int j = 1; j <= m - 1; j++) {
                    Element_index = j + (i - 1) * (m - 1);  // 1-based

                    int V1 = j + 1 + i * m;
                    int V2 = j + 1 + (i - 1) * m;
                    int V3 = j + (i - 1) * m;
                    int V4 = j + i * m;

                    if (cx[i - 1] == 0) {
                        Element_rakeface_vec.push_back(Element_index);
                    }

                    if (cy[j - 1] == 0) {
                        Element_flankface_vec.push_back(Element_index);
                    }

                    // Adjusting node numbers to be sequential and correct
                    // This is a placeholder - actual node numbering needs to match your node generation
                    fid << setw(7) << Element_index << ", "
                        << setw(7) << V1 << ", " << setw(7) << V2 << ", " << setw(7) << V3 << ", " << setw(7) << V4 << "\n";
                }
            }
            break;
        case 2:
            for (int i = 1; i <= l - 1; i++) {
                for (int j = 1; j <= m - 1; j++) {
                    for (int k = 1; k <= n - 1; k++) {
                        Element_index = k + (j - 1) * (n - 1) + (i - 1) * (m - 1) * (n - 1);  // 1-based

                        int V1 = k + 1 + j * n + i * m * n;
                        int V2 = k + 1 + j * n + (i - 1) * m * n;
                        int V3 = k + j * n + (i - 1) * m * n;
                        int V4 = k + j * n + i * m * n;
                        int V5 = k + 1 + (j - 1) * n + i * m * n;
                        int V6 = k + 1 + (j - 1) * n + (i - 1) * m * n;
                        int V7 = k + (j - 1) * n + (i - 1) * m * n;
                        int V8 = k + (j - 1) * n + i * m * n;

                        if (cx[i - 1] == 0) {
                            Element_rakeface_vec.push_back(Element_index);
                        }

                        if (cy[j - 1] == 0) {
                            Element_flankface_vec.push_back(Element_index);
                        }

                        // Adjusting node numbers to be sequential and correct
                        // This is a placeholder - actual node numbering needs to match your node generation
                        fid << setw(7) << Element_index << ", "
                            << setw(7) << V1 << ", " << setw(7) << V2 << ", " << setw(7) << V3 << ", " << setw(7) << V4 << ", "
                            << setw(7) << V5 << ", " << setw(7) << V6 << ", " << setw(7) << V7 << ", " << setw(7) << V8 << "\n";
                    }
                }
            }
            break;
        }

        // Generate node and element sets
        fid << "*Nset, nset=tool_" << index_cutter << ", generate\n";
        fid << "1, " << setw(7) << Node_index << ", 1\n";
        fid << "*Elset, elset=tool_" << index_cutter << ", generate\n";
        fid << "1, " << setw(7) << Element_index << ", 1\n";
        fid << "*Solid Section, elset=tool_" << index_cutter << ", material=material_t\n";
        fid << ",\n";
        fid << "*End Part\n";

        Element_rakeface_vecs.push_back(Element_rakeface_vec);
        Element_flankface_vecs.push_back(Element_flankface_vec);
        a3Vector.push_back(a3);
        b3Vector.push_back(b3);
        tooloffset_xVector.push_back(tooloffset_x);
        tooloffset_yVector.push_back(tooloffset_y);
        //tooloffset_zVector.push_back(tooloffset_z);
        ToolsNode_boundary.push_back(Node_boundary);
    }

    // Assembly section
    fid << "*Assembly, name=Assembly\n";
    fid << "*Instance, name=Euler, part=euler\n";
    fid << "*End Instance\n";
    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++) {
        fid << "*Instance, name=Tool_" << index_cutter << ", part=tool_" << index_cutter << "\n";
        fid << "0," << ToolYOffsetVector[index_cutter] << ", 0\n";
        fid << "*End Instance\n";
    }

    // Reference node
    fid << "*Node\n";
    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++)
    {
        switch (SimulationType)
        {
        case 0:
        case 1:
            fid << index_cutter + 1 << ", " << fixed << setprecision(12) << setw(13) << (a3Vector[index_cutter] + 0.5 * WorkThickness * tan(LambdaVector[index_cutter] * M_PI / 180)) << ", "
                << setw(13) << b3Vector[index_cutter] + tooloffset_yVector[index_cutter] + ToolYOffsetVector[index_cutter] << "\n";
            break;
        case 2:
            fid << index_cutter + 1 << ",  " << fixed << setprecision(12) << setw(13) << (a3Vector[index_cutter] + 0.5 * WorkThickness * tan(LambdaVector[index_cutter] * M_PI / 180)) << ", "
                << setw(13) << b3Vector[index_cutter] + tooloffset_yVector[index_cutter] + ToolYOffsetVector[index_cutter] << ", " << setw(13) << (0.5 * WorkThickness) << "\n";
        }

    }
    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++)
    {
        fid << "*Nset, nset=RP-" << index_cutter << "\n";
        fid << index_cutter + 1 << ",\n";
    }
    // Bottom nodes
    fid << "*Nset, nset=bottom, instance=Euler\n";
    for (size_t i = 0; i < Node_bottom.size(); i++) {
        fid << setw(7) << Node_bottom[i] << ",";
        if ((i + 1) % 15 == 0 || i == Node_bottom.size() - 1) {
            fid << "\n";
        }
    }
    switch (SimulationType)
    {
    case 2:
        // Side nodes
        fid << "*Nset, nset=side, instance=Euler\n";
        for (size_t i = 0; i < Node_side.size(); i++) {
            fid << setw(7) << Node_side[i] << ",";
            if ((i + 1) % 15 == 0 || i == Node_side.size() - 1) {
                fid << "\n";
            }
        }
    }
    // Tool Boundary nodes
    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++)
    {
        fid << "*Nset, nset=boundary_" << index_cutter << ", instance=Tool_" << index_cutter << "\n";
        for (size_t i = 0; i < ToolsNode_boundary[index_cutter].size(); i++) {
            fid << setw(7) << ToolsNode_boundary[index_cutter][i] << ",";;
            if ((i + 1) % 15 == 0 || i == ToolsNode_boundary[index_cutter].size() - 1) {
                fid << "\n";
            }
        }

        fid << "*Elset, elset=rakeface_" << index_cutter << ", internal, instance = Tool_" << index_cutter << "\n";
        for (size_t i = 0; i < Element_rakeface_vecs[index_cutter].size(); i++) {
            fid << setw(7) << Element_rakeface_vecs[index_cutter][i] << ",";;
            if ((i + 1) % 15 == 0 || i == Element_rakeface_vecs[index_cutter].size() - 1) {
                fid << "\n";
            }
        }

        fid << "*Elset, elset=flankface_" << index_cutter << ", internal, instance=Tool_" << index_cutter << "\n";
        for (size_t i = 0; i < Element_flankface_vecs[index_cutter].size(); i++) {
            fid << setw(7) << Element_flankface_vecs[index_cutter][i] << ",";;
            if ((i + 1) % 15 == 0 || i == Element_flankface_vecs[index_cutter].size() - 1) {
                fid << "\n";
            }
        }

        // Interface surface
        fid << "*Surface, type=ELEMENT, name=interface_" << index_cutter << "\n";
        fid << "rakeface_" << index_cutter << ", S4\n";
        fid << "flankface_" << index_cutter << ", S2\n";

        // Rigid body
        fid << "*Rigid Body, ref node=Rp-" << index_cutter << ", elset=Tool_" << index_cutter << ".tool_" << index_cutter << ", position=CENTER OF MASS\n";
    }
    fid << "*End Assembly\n";


    // Material properties section
    fid << "*Material, name=material_wp\n"; // workpiece material

    fid << "*Conductivity\n";
    for (int i = 0; i < K_wp.size(); i++) {
        for (int j = 0; j < K_wp[i].size(); j++) {
            fid << setprecision(12) << K_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Specific Heat\n";
    for (int i = 0; i < Cp_wp.size(); i++) {
        for (int j = 0; j < Cp_wp[i].size(); j++) {
            fid << setprecision(12) << Cp_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Density\n";
    for (int i = 0; i < rho_wp.size(); i++) {
        for (int j = 0; j < rho_wp[i].size(); j++) {
            fid << setprecision(12) << rho_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Elastic\n";
    for (int i = 0; i < E_wp.size(); i++) {
        for (int j = 0; j < E_wp[i].size(); j++) {
            fid << setprecision(12) << E_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Expansion\n";
    for (int i = 0; i < alpha_wp.size(); i++) {
        for (int j = 0; j < alpha_wp[i].size(); j++) {
            fid << setprecision(12) << alpha_wp[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Inelastic Heat Fraction\n";
    fid << setprecision(12) << eta_wp << "\n";

    fid << "*Plastic, hardening=JOHNSON COOK\n";
    fid << setprecision(12) << A_wp << "," << B_wp << "," << n_wp << "," << m_wp << "," << Tm_wp << "," << T0_wp << "\n";
    fid << "*Rate Dependent, type=JOHNSON COOK\n";
    fid << setprecision(12) << C_wp << "," << epsilon0_wp << "\n";

    fid << "*Material, name=material_t\n"; // cutting tool material

    fid << "*Conductivity\n";
    for (int i = 0; i < K_t.size(); i++) {
        for (int j = 0; j < K_t[i].size(); j++) {
            fid << setprecision(12) << K_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Specific Heat\n";
    for (int i = 0; i < Cp_t.size(); i++) {
        for (int j = 0; j < Cp_t[i].size(); j++) {
            fid << setprecision(12) << Cp_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Density\n";
    for (int i = 0; i < rho_t.size(); i++) {
        for (int j = 0; j < rho_t[i].size(); j++) {
            fid << setprecision(12) << rho_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Elastic\n";
    for (int i = 0; i < E_t.size(); i++) {
        for (int j = 0; j < E_t[i].size(); j++) {
            fid << setprecision(12) << E_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Expansion\n";
    for (int i = 0; i < alpha_t.size(); i++) {
        for (int j = 0; j < alpha_t[i].size(); j++) {
            fid << setprecision(12) << alpha_t[i][j] << ",";
        }
        fid << "\n";
    }

    fid << "*Inelastic Heat Fraction\n";
    fid << setprecision(12) << eta_t << "\n";
    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++)
    {
        fid << "*Surface Interaction, name=wp_t_" << index_cutter << "\n"; // tool-chip contact

        fid << "*Friction\n";
        fid << setprecision(12) << MiuVector[index_cutter] << "\n";

        fid << "*Surface Behavior, pressure-overclosure=HARD\n";

        fid << "*Gap Conductance, pressure\n";
        for (int i = 0; i < h.size(); i++) {
            fid << setprecision(12) << h[i][0] << "," << h[i][1] << "\n";
        }

        fid << "*Gap Heat Generation\n";
        fid << "1., 0.5\n";
    }
    switch (SimulationType)
    {
    case 2:
        fid << "*Initial Conditions, type=VOLUME FRACTION\n";
        fid << "Euler.workpiece, Euler.material_wp, 1.\n";
    }
    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++)
    {
        fid << "**Name: Field-Temp-Tool-" << index_cutter << " Type: Temperature\n";
        fid << "*Initial Conditions, type=TEMPERATURE\n";
        fid << "Tool_" << index_cutter << ".Tool_" << index_cutter << ", " << T0_wp << "\n";
    }
    switch (SimulationType)
    {
    case 0:
    case 1:
        fid << "**Name: Field-Temp-Work Type: Temperature\n";
        fid << "*Initial Conditions, type=TEMPERATURE\n";
        fid << "Euler.Workpiece," << T0_wp << "\n";
        break;
    case 2:
        fid << "**Name: Field-Temp-Work Type: Temperature\n";
        fid << "*Initial Conditions, type=TEMPERATURE\n";
        fid << "Euler.euler," << T0_wp << "\n";
    }

    for (int index_cutter = 0; index_cutter < UCTVector.size(); index_cutter++)
    {
        double Va = VelocityVector[index_cutter];

        string CuttingStep = "cutting";
        double CuttingStepTime = (InitialGapBetweenToolWork + WorkLength + 0.5 * WorkThickness * tan(LambdaVector[index_cutter] * M_PI / 180)) / Va;
        string WaitStep = "wait";
        double WaitStepTime = DistanceBetweenTool / Va;

        string FeedStep = "Feed";
        //double FeedSpeed = 10 * Va;
        //double FeedStepTime = WorkYOffsetVector[index_cutter] / FeedSpeed;

        double FeedStepTime = DistanceBetweenTool / Va;
        double FeedSpeed = Va;

        int CuttingStepOutputFrequency = FieldOutputFrequency;
        int WaitStepOutputFrequency = ceil(FieldOutputFrequency / CuttingStepTime * WaitStepTime);
        int FeedStepOutputFrequency = ceil(FieldOutputFrequency / CuttingStepTime * FeedStepTime);
        double CuttingStepOutputInterval = CuttingStepTime / CuttingStepOutputFrequency;
        double WaitStepOutputInterval = WaitStepTime / WaitStepOutputFrequency;
        double FeedStepOutputInterval = FeedStepTime / WaitStepOutputFrequency;


        //////////////////////////////////////////////////////////////////////////////////////
        // 1st Cutting Step section
        //////////////////////////////////////////////////////////////////////////////////////
        fid << "**\n";
        fid << "**STEP: " << CuttingStep << "_" << index_cutter << "\n";
        fid << "**\n"; // step1 name from input\n";
        fid << "*Step, name=" << CuttingStep << "_" << index_cutter << ", nlgeom = YES\n"; // step1 name from input

        fid << "*Dynamic Temperature-displacement, Explicit\n";
        fid << ", " << setprecision(12) << CuttingStepTime << "\n"; // time increment for step1
        fid << "*Bulk Viscosity\n";
        fid << "0.06, 1.2\n"; // bulk viscosity parameters

        fid << "**\n";
        fid << "**BOUNDARY CONDITIONS\n";
        fid << "**\n";
        if (index_cutter == 0) {
            // Temperature boundary conditions
            fid << "**Name: Temp-BC-Bottom Type: Temperature\n";
            fid << "*Boundary\n";
            fid << "bottom, 11, 11, " << setprecision(12) << T0_wp << "\n"; // bottom set temperature fixed at T0_wp 
        }
        fid << "**Name: Temp-BC-Tool_" << index_cutter << " Type: Temperature\n";
        fid << "*Boundary\n";
        fid << "boundary_" << index_cutter << ", 11, 11, " << setprecision(12) << T0_wp << "\n"; // other boundary temperature fixed at T0_wp

        switch (SimulationType)
        {
        case 2:
            if (PlaneStrain) {
                if (index_cutter == 0) {
                    // Velocity boundary conditions for side set
                    fid << "**Name: Vel-BC-Side-V3 Type: Velocity/Angular velocity\n";
                    fid << "*Boundary, type=VELOCITY\n";
                    fid << "side, 3, 3\n"; // z-direction velocity fixed
                    fid << "**Name: Vel-BC-Side-VR1 Type: Velocity/Angular velocity\n";
                    fid << "*Boundary, type=VELOCITY\n";
                    fid << "side, 4, 4\n"; // rotational velocity fixed
                    fid << "**Name: Vel-BC-Side-VR2 Type: Velocity/Angular velocity\n";
                    fid << "*Boundary, type=VELOCITY\n";
                    fid << "side, 5, 5\n"; // rotational velocity fixed
                }
            }
        }
        if (index_cutter == 0)
        {
            // Velocity boundary conditions for bottom set
            fid << "**Name: Vel-BC-Bottom-V1 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 1, 1\n"; // x-direction velocity fixed
            fid << "**Name: Vel-BC-Bottom-V2 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 2, 2\n"; // y-direction velocity fixed
            fid << "**Name: Vel-BC-Bottom-V3 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 3, 3\n"; // z-direction velocity fixed
            fid << "**Name: Vel-BC-Bottom-VR1 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 4, 4\n"; // rotational velocity fixed
            fid << "**Name: Vel-BC-Bottom-VR2 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 5, 5\n"; // rotational velocity fixed
            fid << "**Name: Vel-BC-Bottom-VR3 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 6, 6\n"; // rotational velocity fixed
        }
        else {
            fid << "**Name: Vel-BC-Bottom-V2 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 2, 2\n"; // y-direction velocity fixed
        }

        // Velocity boundary conditions for RP (reference point)
        fid << "**Name: Vel-BC-Rp-" << index_cutter << "-V1 Type: Velocity/Angular velocity\n";
        fid << "*Boundary, type=VELOCITY\n";
        fid << "Rp-" << index_cutter << ", 1, 1, " << -Va << "\n"; // x-direction velocity set to -Va
        fid << "**Name: Vel-BC-Rp-" << index_cutter << "-V2 Type: Velocity/Angular velocity\n";
        fid << "*Boundary, type=VELOCITY\n";
        fid << "Rp-" << index_cutter << ", 2, 2\n"; // y-direction velocity free
        fid << "**Name: Vel-BC-Rp-" << index_cutter << "-V3 Type: Velocity/Angular velocity\n";
        fid << "*Boundary, type=VELOCITY\n";
        fid << "Rp-" << index_cutter << ", 3, 3\n"; // z-direction velocity free
        fid << "**Name: Vel-BC-Rp-" << index_cutter << "-VR1 Type: Velocity/Angular velocity\n";
        fid << "*Boundary, type=VELOCITY\n";
        fid << "Rp-" << index_cutter << ", 4, 4\n"; // rotational velocity free
        fid << "**Name: Vel-BC-Rp-" << index_cutter << "-VR2 Type: Velocity/Angular velocity\n";
        fid << "*Boundary, type=VELOCITY\n";
        fid << "Rp-" << index_cutter << ", 5, 5\n"; // rotational velocity free
        fid << "**Name: Vel-BC-Rp-" << index_cutter << "-VR3 Type: Velocity/Angular velocity\n";
        fid << "*Boundary, type=VELOCITY\n";
        fid << "Rp-" << index_cutter << ", 6, 6\n"; // rotational velocity free

        switch (SimulationType)
        {
        case 1:
            fid << "*Adaptive Mesh Controls, name=Ada-1, curvature refinement=5.\n";
            fid << "0.5, 0., 0.5\n";
            fid << "*Adaptive Mesh, elset=EULER.WORKPIECE, controls=Ada-1, frequency=1, mesh sweeps=10, op=NEW\n";
        }

        // Contact definition
        fid << "**\n"; // new contact definition
        fid << "** INTERACTIONS\n"; // new contact definition
        fid << "**\n"; // new contact definition
        fid << "** Interaction: general_contact_cutting_" << index_cutter << "\n"; // new contact definition
        fid << "*Contact, op=NEW\n"; // new contact definition
        fid << "*Contact Inclusions\n";
        fid << "interface_" << index_cutter << ", EULER.MATERIAL_WP\n"; // new contact definition
        fid << "*Contact Property Assignment\n"; // new contact definition
        fid << " , , WP_T_" << index_cutter << "\n"; // new contact definition

        fid << "**\n";
        fid << "**OUTPUT REQUESTS\n";
        fid << "**\n";
        // Restart settings
        fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

        // Field output settings
        fid << "*Output, field, number interval=" << CuttingStepOutputFrequency << "\n"; // field output frequency
        fid << "*Node Output\n";
        fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement

        fid << "*Element Output, directions=YES\n";
        fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables

        fid << "*Contact Output\n";
        fid << "CSTRESS, \n"; // contact stress output

        // History output settings
        fid << "*Output, history, time interval=" << setprecision(12) << CuttingStepOutputInterval << "\n"; // history output time interval
        fid << "*Node Output, nset=Rp-" << index_cutter << "\n";
        fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP

        fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << CuttingStepOutputInterval << "\n"; // preselected history variables

        fid << "*End Step\n"; // end of step definition

        ////////////////////////////////////////////////////////////////////////////////////////
        //// 2nd WaitStep section
        ////////////////////////////////////////////////////////////////////////////////////////
        //fid << "*Step, name=" << WaitStep << "_" << index_cutter << ", nlgeom = YES\n"; // step2 name from input
        //fid << "*Dynamic Temperature-displacement, Explicit\n";
        //fid << "," << setprecision(12) << WaitStepTime << "\n"; // time increment for step2
        //fid << "*Bulk Viscosity\n";
        //fid << "0.06, 1.2\n"; // bulk viscosity parameters

        //// Velocity boundary condition for RP (reference point)
        //fid << "**Name: Vel-BC-Rp-" << index_cutter << "-V1 Type: Velocity/Angular velocity\n";
        //fid << "*Boundary, type=VELOCITY\n";
        //fid << "Rp-" << index_cutter << ", 1, 1, " << setprecision(12) << 0 << "\n"; // x-direction velocity set to -V

        //switch (SimulationType)
        //{
        //case 1:
        //    fid << "*Adaptive Mesh, elset=EULER.WORKPIECE, controls=Ada-1, frequency=1, mesh sweeps=10, op=NEW\n";
        //}

        //// Restart settings
        //fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

        //// Field output settings
        //fid << "*Output, field, number interval=" << CuttingStepOutputFrequency << "\n"; // field output frequency
        //fid << "*Node Output\n";
        //fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement
        //fid << "*Element Output, directions=YES\n";
        //fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables
        //fid << "*Contact Output\n";
        //fid << "CSTRESS, \n"; // contact stress output

        //// History output settings
        //fid << "*Output, history, time interval=" << setprecision(12) << CuttingStepOutputInterval << "\n"; // history output time interval
        //fid << "*Node Output, nset=Rp-" << index_cutter << "\n";
        //fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP
        //fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << CuttingStepOutputInterval << "\n"; // preselected history variables

        //fid << "*End Step\n"; // end of step 2 definition

        //////////////////////////////////////////////////////////////////////////////////////
        // Feed Step section
        //////////////////////////////////////////////////////////////////////////////////////
        if (index_cutter + 1 < UCTVector.size())
        {
            fid << "**\n";
            fid << "**STEP: " << FeedStep << "_" << index_cutter << "\n";
            fid << "**\n"; // step1 name from input\n";
            fid << "*Step, name=" << FeedStep << "_" << index_cutter << ", nlgeom = YES\n"; // step4 name from input
            fid << "*Dynamic Temperature-displacement, Explicit\n";
            fid << "," << FeedStepTime << "\n"; // time increment for step4
            fid << "*Bulk Viscosity\n";
            fid << "0.06, 1.2\n"; // bulk viscosity parameters

            fid << "**\n";
            fid << "**BOUNDARY CONDITIONS\n";
            fid << "**\n";
            // Velocity boundary conditions for bottom set
            fid << "**Name: Vel-BC-Bottom-V2 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "bottom, 2, 2, " << FeedSpeed << "\n"; // y-direction velocity fixed  

            fid << "**Name: Vel-BC-Rp-" << index_cutter << "-V1 Type: Velocity/Angular velocity\n";
            fid << "*Boundary, type=VELOCITY\n";
            fid << "Rp-" << index_cutter << ", 1, 1\n"; // x-direction velocity set to -0


            switch (SimulationType)
            {
            case 1:
                fid << "*Adaptive Mesh, op=NEW\n";
            }

            // Contact definition
            fid << "**\n"; // new contact definition
            fid << "** INTERACTIONS\n"; // new contact definition
            fid << "**\n"; // new contact definition
            fid << "** Interaction: general_contact_cutting_" << index_cutter << "\n"; // new contact definition
            fid << "*Contact, op=NEW\n"; // new contact definition


            fid << "**\n";
            fid << "**OUTPUT REQUESTS\n";
            fid << "**\n";
            // Restart settings
            fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

            // Field output settings
            fid << "*Output, field, number interval=" << FeedStepOutputFrequency << "\n"; // field output frequency
            fid << "*Node Output\n";
            fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement
            fid << "*Element Output, directions=YES\n";
            fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables
            fid << "*Contact Output\n";
            fid << "CSTRESS, \n"; // contact stress output

            // History output settings
            fid << "*Output, history, time interval=" << setprecision(12) << FeedStepOutputInterval << "\n"; // history output time interval
            fid << "*Node Output, nset=Rp-" << index_cutter << "\n";
            fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP
            fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << FeedStepOutputInterval << "\n", FeedStepOutputInterval; // preselected history variables

            fid << "*End Step\n"; // end of step 4 definition
        }
    }

    // Release Step section
    string ReleaseStep = "Release";
    double ReleaseStepTime = 0.2;

    int ReleaseStepOutputFrequency = 50; double ReleaseStepOutputInterval = ReleaseStepTime / ReleaseStepOutputFrequency;

    fid << "**\n";
    fid << "**STEP: " << ReleaseStep << "\n";
    fid << "**\n"; // step1 name from input\n";
    fid << "*Step, name=" << ReleaseStep << ", nlgeom = YES\n"; // step4 name from input
    fid << "*Dynamic Temperature-displacement, Explicit\n";
    fid << "," << ReleaseStepTime << "\n"; // time increment for step4
    fid << "*Bulk Viscosity\n";
    fid << "0.06, 1.2\n"; // bulk viscosity parameters

    fid << "**\n";
    fid << "**BOUNDARY CONDITIONS\n";
    fid << "**\n";
    // Velocity boundary condition for RP (reference point)
    fid << "**Name: Vel-BC-Rp-" << UCTVector.size() - 1 << "-V1 Type: Velocity/Angular velocity\n";
    fid << "*Boundary, type=VELOCITY\n";
    fid << "Rp-" << UCTVector.size() - 1 << ", 1, 1\n"; // x-direction velocity set to 0 (stationary)

    switch (SimulationType)
    {
    case 1:
        fid << "*Adaptive Mesh, op=NEW\n";
    }

    // Contact definition
    fid << "**\n"; // new contact definition
    fid << "** INTERACTIONS\n"; // new contact definition
    fid << "**\n"; // new contact definition
    fid << "** Interaction: general_contact_cutting_" << UCTVector.size() - 1 << "\n"; // new contact definition
    fid << "*Contact, op=NEW\n"; // new contact definition

    fid << "**\n";
    fid << "**OUTPUT REQUESTS\n";
    fid << "**\n";
    // Restart settings
    fid << "*Restart, write, number interval=1, time marks=NO\n"; // write restart files every step

    // Field output settings
    fid << "*Output, field, number interval=" << ReleaseStepOutputFrequency << "\n"; // field output frequency
    fid << "*Node Output\n";
    fid << "A, NT, RFL, U, V\n"; // output variables: acceleration, temperature, reaction force, displacement
    fid << "*Element Output, directions=YES\n";
    fid << "CFAILURE, DMICRT, EVF, HFL, LE, PE, PEEQ, PEEQVAVG, PEVAVG, S, SDEG, STATUS, SVAVG, TEMP\n"; // element output variables
    fid << "*Contact Output\n";
    fid << "CSTRESS, \n"; // contact stress output

    //// History output settings
    //fid << "*Output, history, time interval=" << setprecision(12) << ReleaseStepOutputInterval << "\n"; // history output time interval
    //fid << "*Node Output, nset=RP\n";
    //fid << "RF1, RF2, RF3, RM1, RM2, RM3\n"; // output reaction forces/moment at RP
    //fid << "*Output, history, variable=PRESELECT, time interval=" << setprecision(12) << ReleaseStepOutputInterval << "\n", ReleaseStepOutputInterval; // preselected history variables

    fid << "*End Step\n"; // end of step 4 definition
    // Close the output file
    fid.close();
    return 0;
};
