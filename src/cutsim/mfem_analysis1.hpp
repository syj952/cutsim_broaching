
//#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include "../cutsim/volume.hpp"
#include "cutsim.hpp"
#include "octree.hpp"
///added by syj

using namespace cutsim;
using namespace std;
//using namespace mfem;

void MeshIDExport(Octree* octree,const std::string& meshFile,std::vector<GLVertex*>& normalvertices);
void runEx12p(const std::string& meshFile, std::vector<double>& materialprops, std::vector<double>& eigenvalues, std::vector<std::vector<double>>& eigenvectors);
static QByteArray extractJsonSegment(const QByteArray &raw);
 // MFEM_ANALYSIS1_HPP

