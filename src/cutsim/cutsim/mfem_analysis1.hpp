
#ifndef MFEM_ANALYSIS1_HPP
#define MFEM_ANALYSIS1_HPP

//#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include "../cutsim/volume.hpp"
#include "cutsim.hpp"
#include "octree.hpp"
///added by syj

using namespace cutsim;
using namespace std;
//using namespace mfem;

void MeshIDExport(Octree* octree,const std::string& meshFile,std::vector<GLVertex*>& normalvertices);
void runEx12p(const std::string& meshFile, std::vector<double>& materialprops, std::vector<double>& eigenvalues, std::vector<std::vector<double>>& eigenvectors);
std::string residualReleaseProjectRoot();

struct ResidualReleaseSummary {
    bool success = false;
    int step = 0;
    double max_disp_mm = 0.0;
    double max_ux_mm = 0.0;
    double max_uy_mm = 0.0;
    double max_uz_mm = 0.0;
    std::string displacement_file;
    std::string deformed_mesh_file;
    std::string summary_file;
};

int runResidualRelease(
    const std::string& meshFile,
    const std::string& stressConfigFile,
    double young,
    double poisson,
    int fixedBoundaryAttr,
    int stepId,
    const std::string& outputPrefix,
    ResidualReleaseSummary* summary = nullptr);
static QByteArray extractJsonSegment(const QByteArray &raw);

#endif // MFEM_ANALYSIS1_HPP
