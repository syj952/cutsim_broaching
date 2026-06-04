/*
 *  Copyright 2010 Anders Wallin (anders.e.e.wallin "at" gmail.com)
 *  Copyright 2015      Kazuyasu Hamada (k-hamada@gifu-u.ac.jp)
 *
 *  This file is part of OpenCAMlib.
 *
 *  OpenCAMlib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCAMlib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenCAMlib.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <cassert>
#include <cmath>

#include <src/cutsim/cutsim_def.hpp>

#include <QFile>
#include <QTextStream>

#include "volume.hpp"

#pragma comment(lib, "vcruntime.lib")
#pragma comment(lib, "ucrt.lib")
#pragma comment(lib, "legacy_stdio_definitions.lib")



namespace cutsim {

    //************* Sphere **************/

    /// sphere at center
    SphereVolume::SphereVolume() {
        type = SPHERE_VOLUME;
        center = GLVertex(0, 0, 0);
        radius = 1.0;
        calcBB();
    }

    double SphereVolume::dist(const GLVertex& p) const {
        double d = (center - p).norm();
        return radius - d; // positive inside. negative outside.
    }

    /// set the bounding box values
    void SphereVolume::calcBB() {
        bb.clear();
        GLVertex maxpt = GLVertex(center.x + radius, center.y + radius, center.z + radius);
        GLVertex minpt = GLVertex(center.x - radius, center.y - radius, center.z - radius);
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
    }

    //************* Rectangle **************/

    RectVolume::RectVolume() {
        type = RECTANGLE_VOLUME;
    }

    double RectVolume::dist(const GLVertex& p) const {
        return 0;
    }

    void RectVolume::calcBB() {
        bb.clear();
        // 计算包围盒的最小/最大点（考虑容差）
        GLVertex minpt = center - GLVertex(lengthX / 2, lengthY / 2, lengthZ / 2) - GLVertex(TOLERANCE, TOLERANCE, TOLERANCE);
        GLVertex maxpt = center + GLVertex(lengthX / 2, lengthY / 2, lengthZ / 2) + GLVertex(TOLERANCE, TOLERANCE, TOLERANCE);

        // 将点添加到包围盒
        bb.addPoint(minpt);
        bb.addPoint(maxpt);
    }


    RectVolume2::RectVolume2() {
        type = RECTANGLE_VOLUME;
        corner = GLVertex(0, 0, 0);
        width = 1.0;
        length = 1.0;
        hight = 1.0;
        center = GLVertex(corner.x + width * 0.5, corner.y + length * 0.5, corner.z + hight * 0.5);	// center is located at the center of box
        rotationCenter = GLVertex(0, 0, 0);
        angle = GLVertex(0, 0, 0);
    }

    void RectVolume2::calcBB() {
        GLVertex p[8] = { center + GLVertex(width * 0.5,  length * 0.5,    0.0),
                          center + GLVertex(-width * 0.5,  length * 0.5,    0.0),
                          center + GLVertex(-width * 0.5, -length * 0.5,    0.0),
                          center + GLVertex(width * 0.5, -length * 0.5,    0.0),
                          center + GLVertex(width * 0.5,  length * 0.5,  hight),
                          center + GLVertex(-width * 0.5,  length * 0.5,  hight),
                          center + GLVertex(-width * 0.5, -length * 0.5,  hight),
                          center + GLVertex(width * 0.5, -length * 0.5,  hight) };

        for (int n = 0; n < 8; n++) {
            GLVertex rotated_p = p[n] - rotationCenter;
            p[n] = rotated_p.rotateABC(angle.x, angle.y, angle.z) + rotationCenter;
        }

        GLVertex maxpt;
        GLVertex minpt;
        maxpt.x = fmax(fmax(fmax(fmax(fmax(fmax(fmax(p[0].x, p[1].x), p[2].x), p[3].x), p[4].x), p[5].x), p[6].x), p[7].x) + TOLERANCE;
        maxpt.y = fmax(fmax(fmax(fmax(fmax(fmax(fmax(p[0].y, p[1].y), p[2].y), p[3].y), p[4].y), p[5].y), p[6].y), p[7].y) + TOLERANCE;
        maxpt.z = fmax(fmax(fmax(fmax(fmax(fmax(fmax(p[0].z, p[1].z), p[2].z), p[3].z), p[4].z), p[5].z), p[6].z), p[7].z) + TOLERANCE;
        minpt.x = fmin(fmin(fmin(fmin(fmin(fmin(fmin(p[0].x, p[1].x), p[2].x), p[3].x), p[4].x), p[5].x), p[6].x), p[7].x) - TOLERANCE;
        minpt.y = fmin(fmin(fmin(fmin(fmin(fmin(fmin(p[0].y, p[1].y), p[2].y), p[3].y), p[4].y), p[5].y), p[6].y), p[7].y) - TOLERANCE;
        minpt.z = fmin(fmin(fmin(fmin(fmin(fmin(fmin(p[0].z, p[1].z), p[2].z), p[3].z), p[4].z), p[5].z), p[6].z), p[7].z) - TOLERANCE;
        bb.clear();
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
    }

    double RectVolume2::dist(const GLVertex& p) const {
        GLVertex rotated_p = p - rotationCenter;
        rotated_p = rotated_p.rotateCBA(-angle.x, -angle.y, -angle.z) + rotationCenter;
        double max_x = corner.x + width;
        double min_x = corner.x;
        double max_y = corner.y + length;
        double min_y = corner.y;
        double max_z = corner.z + hight;
        double min_z = corner.z;
        double dOut = 0.0;

        if ((min_x <= rotated_p.x) && (rotated_p.x <= max_x) && (min_y <= rotated_p.y) && (rotated_p.y <= max_y) && (min_z <= rotated_p.z) && (rotated_p.z <= max_z)) {
            double xdist, ydist, zdist;
            if ((rotated_p.x - min_x) > (max_x - rotated_p.x))
                xdist = max_x - rotated_p.x;
            else
                xdist = rotated_p.x - min_x;

            if ((rotated_p.y - min_y) > (max_y - rotated_p.y))
                ydist = max_y - rotated_p.y;
            else
                ydist = rotated_p.y - min_y;

            if ((rotated_p.z - min_z) > (max_z - rotated_p.z))
                zdist = max_z - rotated_p.z;
            else
                zdist = rotated_p.z - min_z;

            if (xdist <= ydist && xdist <= zdist)
                dOut = -xdist;
            else if (ydist < xdist && ydist < zdist)
                dOut = -ydist;
            else if (zdist < xdist && zdist < xdist)
                dOut = -zdist;
            else {
                assert(0);
                return -1;
            }
        }
        else if ((min_y <= rotated_p.y) && (rotated_p.y <= max_y) && (min_z <= rotated_p.z) && (rotated_p.z <= max_z)) {
            if (rotated_p.x < min_x) {
                dOut = min_x - rotated_p.x;
            }
            else if (rotated_p.x > max_x) {
                dOut = rotated_p.x - max_x;
            }
        }
        else if ((min_x <= rotated_p.x) && (rotated_p.x <= max_x) && (min_z <= rotated_p.z) && (rotated_p.z <= max_z)) {
            if (rotated_p.y < min_y) {
                dOut = min_y - rotated_p.y;
            }
            else if (rotated_p.y > max_y) {
                dOut = rotated_p.y - max_y;
            }
        }
        else if ((min_x <= rotated_p.x) && (rotated_p.x <= max_x) && (min_y <= rotated_p.y) && (rotated_p.y <= max_y)) {
            if (rotated_p.z < min_z) {
                dOut = min_z - rotated_p.z;
            }
            else if (rotated_p.z > max_z) {
                dOut = rotated_p.z - max_z;
            }
        }
        else if ((rotated_p.x > max_x) && (rotated_p.y > max_y))
            dOut = sqrt((rotated_p.x - max_x) * (rotated_p.x - max_x) + (rotated_p.y - max_y) * (rotated_p.y - max_y));
        else if ((rotated_p.x > max_x) && (rotated_p.z < min_z))
            dOut = sqrt((rotated_p.x - max_x) * (rotated_p.x - max_x) + (min_z - rotated_p.z) * (min_z - rotated_p.z));
        else if ((rotated_p.x < min_x) && (rotated_p.y > max_y))
            dOut = sqrt((min_x - rotated_p.x) * (min_x - rotated_p.x) + (rotated_p.y - max_y) * (rotated_p.y - max_y));
        else if ((rotated_p.y > max_y) && (rotated_p.z > max_z))
            dOut = sqrt((rotated_p.y - max_y) * (rotated_p.y - max_y) + (rotated_p.z - max_z) * (rotated_p.z - max_z));
        else if ((rotated_p.x > max_x) && (rotated_p.z > max_z))
            dOut = sqrt((rotated_p.x - max_x) * (rotated_p.x - max_x) + (rotated_p.z - max_z) * (rotated_p.z - max_z));
        else if ((rotated_p.x > max_x) && (rotated_p.y < min_y))
            dOut = sqrt((rotated_p.x - max_x) * (rotated_p.x - max_x) + (min_y - rotated_p.y) * (min_y - rotated_p.y));
        else if ((rotated_p.x < min_x) && (rotated_p.y < min_y))
            dOut = sqrt((min_x - rotated_p.x) * (min_x - rotated_p.x) + (rotated_p.y - max_y) * (rotated_p.y - max_y));
        else if ((rotated_p.y < min_y) && (rotated_p.z > max_z))
            dOut = sqrt((min_y - rotated_p.y) * (min_y - rotated_p.y) + (rotated_p.z - max_z) * (rotated_p.z - max_z));
        else if ((rotated_p.x < min_x) && (rotated_p.z < min_z))
            dOut = sqrt((rotated_p.x - max_x) * (rotated_p.x - max_x) + (min_z - rotated_p.z) * (min_z - rotated_p.z));
        else if ((rotated_p.y > max_y) && (rotated_p.z < min_z))
            dOut = sqrt((rotated_p.y - max_y) * (rotated_p.y - max_y) + (min_y - rotated_p.y) * (min_y - rotated_p.y));
        else if ((rotated_p.x < min_x) && (rotated_p.z > max_z))
            dOut = sqrt((min_x - rotated_p.x) * (min_x - rotated_p.x) + (rotated_p.z - max_z) * (rotated_p.z - max_z));
        else if ((rotated_p.y < min_y) && (rotated_p.z < min_z))
            dOut = sqrt((min_y - rotated_p.y) * (min_y - rotated_p.y) + (min_z - rotated_p.z) * (min_z - rotated_p.z));

        return -dOut;
    }

    //************* Cylinder **************/

    CylinderVolume::CylinderVolume() {
        type = CYLINDER_VOLUME;
        radius = 1.0;
        length = 1.0;
        center = GLVertex(0, 0, 0);	// center is located at the bottom of cylinder
        rotationCenter = GLVertex(0, 0, 0);
        angle = GLVertex(0, 0, 0);
    }

    void CylinderVolume::calcBB() {
        GLVertex p[8] = { center + GLVertex(radius,  radius,    0.0),
                          center + GLVertex(-radius,  radius,    0.0),
                          center + GLVertex(-radius, -radius,    0.0),
                          center + GLVertex(radius, -radius,    0.0),
                          center + GLVertex(radius,  radius, length),
                          center + GLVertex(-radius,  radius, length),
                          center + GLVertex(-radius, -radius, length),
                          center + GLVertex(radius, -radius, length) };

        for (int n = 0; n < 8; n++) {
            GLVertex rotated_p = p[n] - rotationCenter;
            p[n] = rotated_p.rotateABC(angle.x, angle.y, angle.z) + rotationCenter;
        }

        GLVertex maxpt;
        GLVertex minpt;
        maxpt.x = fmax(fmax(fmax(fmax(fmax(fmax(fmax(p[0].x, p[1].x), p[2].x), p[3].x), p[4].x), p[5].x), p[6].x), p[7].x) + TOLERANCE;
        maxpt.y = fmax(fmax(fmax(fmax(fmax(fmax(fmax(p[0].y, p[1].y), p[2].y), p[3].y), p[4].y), p[5].y), p[6].y), p[7].y) + TOLERANCE;
        maxpt.z = fmax(fmax(fmax(fmax(fmax(fmax(fmax(p[0].z, p[1].z), p[2].z), p[3].z), p[4].z), p[5].z), p[6].z), p[7].z) + TOLERANCE;
        minpt.x = fmin(fmin(fmin(fmin(fmin(fmin(fmin(p[0].x, p[1].x), p[2].x), p[3].x), p[4].x), p[5].x), p[6].x), p[7].x) - TOLERANCE;
        minpt.y = fmin(fmin(fmin(fmin(fmin(fmin(fmin(p[0].y, p[1].y), p[2].y), p[3].y), p[4].y), p[5].y), p[6].y), p[7].y) - TOLERANCE;
        minpt.z = fmin(fmin(fmin(fmin(fmin(fmin(fmin(p[0].z, p[1].z), p[2].z), p[3].z), p[4].z), p[5].z), p[6].z), p[7].z) - TOLERANCE;
        bb.clear();
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
    }

    double CylinderVolume::dist(const GLVertex& p) const {
        GLVertex rotated_p = p - rotationCenter;
        rotated_p = rotated_p.rotateCBA(-angle.x, -angle.y, -angle.z) + rotationCenter;
        GLVertex tb = rotated_p - center;
        GLVertex tt = rotated_p - (center + GLVertex(0.0, 0.0, length));
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();

        if (tb.z >= 0.0 && tt.z <= 0.0)
            return ((radius - d < tb.z) && (radius - d < -tt.z)) ? radius - d : (tb.z < -tt.z) ? tb.z : -tt.z;  // positive inside. negative outside.
        else if (tb.z < 0.0) {
            // if we are under the cylinder, then return distance to flat cylinder bottom
            if (d < radius)
                return tb.z;
            else {
                // outside the cylinder, return a distance to the outer lower "ring" of the cylinder
                GLVertex n = GLVertex(tb.x, tb.y, 0.0);
                n = n * (radius / d);  // 1/d means normalization
                return -((tb - n).norm());
            }
        }
        else {
            // if we are above the cylinder, then return distance to flat cylinder top
            if (d < radius)
                return -tt.z;
            else {
                // outside the cylinder, return a distance to the outer upper "ring" of the cylinder
                GLVertex n = GLVertex(tt.x, tt.y, 0.0);
                n = n * (radius / d);  // 1/d means normalization
                return -((tt - n).norm());
            }
        }
    }

    //************* STL **************/

    StlVolume::StlVolume() {
        type = STL_VOLUME;
        center = GLVertex(0, 0, 0);	// center is treated as the origin's offset of STL
        rotationCenter = GLVertex(0, 0, 0);
        angle = GLVertex(0, 0, 0);
        cube_resolution = 0.0;
    }

    void StlVolume::calcBB() {
        for (int i = 0; i < (int)facets.size(); i++) {
            facets[i]->v1 += center; facets[i]->v2 += center; facets[i]->v3 += center;
            facets[i]->normal = facets[i]->normal.rotateABC(angle.x, angle.y, angle.z);
            std::cout << "normal x: " << facets[i]->normal.x << " y: " << facets[i]->normal.y << " z: " << facets[i]->normal.z << "\n";
            GLVertex v1p = facets[i]->v1 - rotationCenter;
            facets[i]->v1 = v1p.rotateABC(angle.x, angle.y, angle.z) + rotationCenter;
            std::cout << "vertex v1: " << facets[i]->v1.x << " y: " << facets[i]->v1.y << " z: " << facets[i]->v1.z << "\n";
            GLVertex v2p = facets[i]->v2 - rotationCenter;
            facets[i]->v2 = v2p.rotateABC(angle.x, angle.y, angle.z) + rotationCenter;
            std::cout << "vertex v2: " << facets[i]->v2.x << " y: " << facets[i]->v2.y << " z: " << facets[i]->v2.z << "\n";
            GLVertex v3p = facets[i]->v3 - rotationCenter;
            facets[i]->v3 = v3p.rotateABC(angle.x, angle.y, angle.z) + rotationCenter;
            std::cout << "vertex v3: " << facets[i]->v3.x << " y: " << facets[i]->v3.y << " z: " << facets[i]->v3.z << "\n";

            GLVertex normal = (facets[i]->v2 - facets[i]->v1).cross(facets[i]->v3 - facets[i]->v1);
            normal.normalize();
            //assert ((facets[i]->normal - normal).norm() < CALC_TOLERANCE);
            facets[i]->normal = normal;
        }
        if (facets.size()) {
            maxpt.x = fmax(fmax(facets[0]->v1.x, facets[0]->v2.x), facets[0]->v3.x);
            maxpt.y = fmax(fmax(facets[0]->v1.y, facets[0]->v2.y), facets[0]->v3.y);
            maxpt.z = fmax(fmax(facets[0]->v1.z, facets[0]->v2.z), facets[0]->v3.z);
            minpt.x = fmin(fmin(facets[0]->v1.x, facets[0]->v2.x), facets[0]->v3.x);
            minpt.y = fmin(fmin(facets[0]->v1.y, facets[0]->v2.y), facets[0]->v3.y);
            minpt.z = fmin(fmin(facets[0]->v1.z, facets[0]->v2.z), facets[0]->v3.z);
        }
        for (int i = 0; i < (int)facets.size(); i++) {
            maxpt.x = fmax(fmax(fmax(facets[i]->v1.x, facets[i]->v2.x), facets[i]->v3.x), maxpt.x);
            maxpt.y = fmax(fmax(fmax(facets[i]->v1.y, facets[i]->v2.y), facets[i]->v3.y), maxpt.y);
            maxpt.z = fmax(fmax(fmax(facets[i]->v1.z, facets[i]->v2.z), facets[i]->v3.z), maxpt.z);
            minpt.x = fmin(fmin(fmin(facets[i]->v1.x, facets[i]->v2.x), facets[i]->v3.x), minpt.x);
            minpt.y = fmin(fmin(fmin(facets[i]->v1.y, facets[i]->v2.y), facets[i]->v3.y), minpt.y);
            minpt.z = fmin(fmin(fmin(facets[i]->v1.z, facets[i]->v2.z), facets[i]->v3.z), minpt.z);
            V21.push_back(facets[i]->v2 - facets[i]->v1);
            V21invV21dotV21.push_back((facets[i]->v2 - facets[i]->v1) * (1.0 / (facets[i]->v2 - facets[i]->v1).dot(facets[i]->v2 - facets[i]->v1)));
            V32.push_back(facets[i]->v3 - facets[i]->v2);
            V32invV32dotV32.push_back((facets[i]->v3 - facets[i]->v2) * (1.0 / (facets[i]->v3 - facets[i]->v2).dot(facets[i]->v3 - facets[i]->v2)));
            V13.push_back(facets[i]->v1 - facets[i]->v3);
            V13invV13dotV13.push_back((facets[i]->v1 - facets[i]->v3) * (1.0 / (facets[i]->v1 - facets[i]->v3).dot(facets[i]->v1 - facets[i]->v3)));
        }
        bb.clear();
        maxpt += GLVertex(cube_resolution, cube_resolution, cube_resolution) * 2.0;
        minpt -= GLVertex(cube_resolution, cube_resolution, cube_resolution) * 2.0;
        std::cout << "STL maxpt x:" << maxpt.x << " y: " << maxpt.y << " z:" << maxpt.z << "\n";
        std::cout << "STL minpt x:" << minpt.x << " y: " << minpt.y << " z:" << minpt.z << "\n";
        bb.addPoint(maxpt);
        bb.addPoint(minpt);

        maxlength = fmax(fmax(maxpt.x - minpt.x, maxpt.y - minpt.y), maxpt.z - minpt.z);
        std::cout << "maxlength:" << maxlength << "\n";

        std::cout << "cube_resolution:" << cube_resolution << "\n";

        typedef struct cube {
            double upper_limit;
            double indexcubesize;
            int max_x, max_y, max_z;
            int ratio = 2;
            bool from_facets = false;
        } CUBE;

        std::vector<CUBE> cubes;
        CUBE cube;

        int start_ratio = ROUGH_RATIO;
        assert(start_ratio >= 0); assert((start_ratio % 2) == 0);
        int end_ratio = 0;
        assert(end_ratio >= 0); assert((end_ratio == 1) || (end_ratio % 2) == 0);
        assert(start_ratio > end_ratio);
        int devide_ratio = 2;
        assert((devide_ratio % 2) == 0);
        for (int i = start_ratio; i > end_ratio; i /= devide_ratio) {
            cube.upper_limit = 0.0;
            for (int j = i; j > end_ratio; j /= devide_ratio) {
                cube.upper_limit += maxlength / (MAX_INDEX / j);
            }
            cube.upper_limit += cube_resolution * 2.0;
            cube.upper_limit *= 2.0 + TOLERANCE;
            cube.indexcubesize = maxlength / (MAX_INDEX / i);
            cube.ratio = devide_ratio;
            cube.max_x = MAX_X / i; cube.max_y = MAX_Y / i; cube.max_z = MAX_Z / i;
            assert(cube.max_x > 0); assert(cube.max_y > 0); assert(cube.max_z > 0);
            cube.from_facets = false;
            cubes.push_back(cube);
        }
        cubes[0].from_facets = true;

#ifdef MULTI_THREAD_STL_NEIGHBOR
        int threadNum = QThreadPool::globalInstance()->maxThreadCount();
        std::cout << "Thread Num: " << threadNum << "\n";
        QFuture<void> future[threadNum];
#endif

        for (int i = 0; i < (int)cubes.size(); i++) {
            double upper_limit = cubes[i].upper_limit;
            indexcubesize = cubes[i].indexcubesize;
            ratio = cubes[i].ratio;
            int max_x = cubes[i].max_x, max_y = cubes[i].max_y, max_z = cubes[i].max_z;
            bool from_facets = cubes[i].from_facets;

            std::cout << "STL Facets Searching Level " << i + 1 << "/" << (int)cubes.size();
            swapIndex();
            emit signalProgressFeature(QString("STL Facets Searching Level ") + QString::number(i + 1) + QString("/") + QString::number((int)cubes.size()), 0, max_x);
            setProgress(0);
#ifdef MULTI_THREAD_STL_NEIGHBOR
            for (int index_x = 0; index_x < max_x; index_x++) {
                for (int index_y = 0; index_y < max_y; index_y++) {
                    for (int index_z = 0; index_z < max_z;) {
                        for (int i = 0; i < threadNum; i++)
                        {
                            future[i] = QtConcurrent::run(this, &StlVolume::calcNeighborhoodIndex, index_x, index_y, index_z++, upper_limit, from_facets);
                        }
                        for (int i = 0; i < threadNum; i++)
                        {
                            future[i].waitForFinished();
                        }
#else
            for (int index_x = 0; index_x < max_x; index_x++) {
                for (int index_y = 0; index_y < max_y; index_y++) {
                    for (int index_z = 0; index_z < max_z; index_z++) {
                        calcNeighborhoodIndex(index_x, index_y, index_z, upper_limit, from_facets);
#endif
                    }
                }
                std::cout << "." << std::flush;
                accumlateProgress(1);
                sendProgress();
            }
            std::cout << "done.\n" << std::flush;
                    }

        invcubesize = 1.0 / indexcubesize;

        for (int index_x = 0; index_x < MAX_X; index_x++)
            for (int index_y = 0; index_y < MAX_Y; index_y++)
                for (int index_z = 0; index_z < MAX_Z; index_z++)
                    neighborhoodIndex[src_index][index_x][index_y][index_z].resize(0);

#ifdef MULTI_THREAD_STL_NEIGHBOR
        delete[] future;
#endif
                }

    void StlVolume::calcNeighborhoodIndex(int index_x, int index_y, int index_z, double upper_limit, bool from_facets)
    {
        double x = minpt.x + indexcubesize * index_x + indexcubesize / 2.0;
        if (x > maxpt.x + indexcubesize / 2.0) return;
        double y = minpt.y + indexcubesize * index_y + indexcubesize / 2.0;
        if (y > maxpt.y + indexcubesize / 2.0) return;
        double z = minpt.z + indexcubesize * index_z + indexcubesize / 2.0;
        if (z > maxpt.z + indexcubesize / 2.0) return;

        bool find = false;
        double min = FLT_MAX;
        double ret;
        GLVertex p = GLVertex(x, y, z);
        typedef struct { int index; double dist; } CANDIDATE;
        std::vector<CANDIDATE> candidate;

        if (from_facets == true) {
            for (int i = 0; i < (int)facets.size(); i++) {
                ret = distance(p, i);
                if (ret <= upper_limit) {
                    find = true;
                    neighborhoodIndex[dst_index][index_x][index_y][index_z].push_back(i);
                }
                else {
                    if (ret <= min) {
                        min = ret;
                    }
                    if (ret < (min + indexcubesize)) {
                        CANDIDATE data = { i, ret };
                        candidate.push_back(data);
                    }
                }
            }
        }
        else {
            neighborhoodIndex[dst_index][index_x][index_y][index_z].resize(0);

            int index_size = (int)neighborhoodIndex[src_index][index_x / ratio][index_y / ratio][index_z / ratio].size();
            if (index_size == 0) return;
            for (int ic = 0; ic < index_size; ic++) {
                int i = neighborhoodIndex[src_index][index_x / ratio][index_y / ratio][index_z / ratio][ic];
                ret = distance(p, i);
                if (ret <= upper_limit) {
                    find = true;
                    neighborhoodIndex[dst_index][index_x][index_y][index_z].push_back(i);
                }
                else {
                    if (ret <= min) {
                        min = ret;
                    }
                    if (ret < (min + indexcubesize)) {
                        CANDIDATE data = { i, ret };
                        candidate.push_back(data);
                    }
                }
            }
        }
        if (find == false) {
            double search_distance = sqrt(min * min + indexcubesize * indexcubesize) + TOLERANCE;
            for (int ic = 0; ic < (int)candidate.size(); ic++) {
                int i = candidate[ic].index;
                if (candidate[ic].dist <= search_distance) {
                    neighborhoodIndex[dst_index][index_x][index_y][index_z].push_back(i);
                }
            }
        }
    }

    double StlVolume::distance(const GLVertex & p, int index) {
        GLVertex q, r;
        GLVertex n1, n2, n3;
        double s12, s23, s31;
        double d, u, abs_d;

        u = (p - facets[index]->v1).dot(V21invV21dotV21[index]);
        q = facets[index]->v1 + V21[index] * u;
        d = (q - p).dot(facets[index]->normal);
        r = p + facets[index]->normal * d;
        n1 = (r - facets[index]->v1).cross(V13[index]);
        n2 = (r - facets[index]->v2).cross(V21[index]);
        n3 = (r - facets[index]->v3).cross(V32[index]);
        s12 = n1.dot(n2); s23 = n2.dot(n3); s31 = n3.dot(n1);

        if ((s12 > 0.0) && (s23 > 0.0) && (s31 > 0.0)) {
            return fabs(d);
        }

        double abs_d12, abs_d13, abs_d32;
        GLVertex q12, q13, q32;

        if (u <= 0.0)
            q12 = facets[index]->v1;
        else if (u >= 1.0)
            q12 = facets[index]->v2;
        else
            /* q = facets[index]->v1 + V21[index] * u */
            q12 = q;
        abs_d12 = (q12 - p).norm();

        u = (p - facets[index]->v3).dot(V13invV13dotV13[index]);
        if (u <= 0.0)
            q13 = facets[index]->v3;
        else if (u >= 1.0)
            q13 = facets[index]->v1;
        else
            q13 = facets[index]->v3 + V13[index] * u;
        abs_d13 = (q13 - p).norm();

        u = (p - facets[index]->v2).dot(V32invV32dotV32[index]);
        if (u <= 0.0)
            q32 = facets[index]->v2;
        else if (u >= 1.0)
            q32 = facets[index]->v3;
        else
            q32 = facets[index]->v2 + V32[index] * u;
        abs_d32 = (q32 - p).norm();

        if ((abs_d12 <= abs_d13) && (abs_d12 <= abs_d32)) {
            abs_d = abs_d12;
        }
        else if ((abs_d13 < abs_d12) && (abs_d13 <= abs_d32)) {
            abs_d = abs_d13;
        }
        else {
            abs_d = abs_d32;
        }

        return abs_d;
    }

    double StlVolume::dist(const GLVertex & p) const {
        int index_x, index_y, index_z;
        double ret = -FLT_MAX;

        index_x = (int)((p.x - minpt.x) * invcubesize);
        index_y = (int)((p.y - minpt.y) * invcubesize);
        index_z = (int)((p.z - minpt.z) * invcubesize);

        if ((index_x < 0 || index_x > MAX_X - 1) || (index_y < 0 || index_y > MAX_Y - 1) || (index_z < 0 || index_z > MAX_Z - 1))
            return ret;

        int index_size = (int)neighborhoodIndex[dst_index][index_x][index_y][index_z].size();
        if (index_size == 0) return ret;

        GLVertex q, r;
        GLVertex n1, n2, n3;
        double s12, s23, s31;
        double min = FLT_MAX, dir, u, abs_d;

        double abs_d12, abs_d13, abs_d32;
        GLVertex q12, q13, q32;
        bool correction = false;

        enum StlSide { INSIDE, OUTSIDE, UNDECIDED };
        StlSide side = UNDECIDED;
        enum Property { ON_VERTEX = 0x4, ON_EDGE = 0x8, ON_INNER = 0x10 };
        typedef struct { int index; StlSide side; double abs_d; GLVertex q; int property; } CANDIDATE;
        CANDIDATE selected = { 0, UNDECIDED, FLT_MAX, };
        CANDIDATE second = { 0, UNDECIDED, FLT_MAX, };
        CANDIDATE third = { 0, UNDECIDED, FLT_MAX, };
        std::vector<CANDIDATE> more;
        int normal_vec_calc_count = 0;

        //struct { double x, y, z; } target = {-24.6268, 28.3726, 7.4243};
        //struct { double x, y, z; } target = {1.54902, 28.3839, 29.4307};
        //struct { double x, y, z; } target = {-10.8187, -1.39877, -19.1763};
        struct { double x, y, z; } target = { -17.9277, 1.39365, 29.8095 };
        GLVertex test_vector;
        bool hit = false;

        //#define DETAIL
        //#define GRID    (5.0)
#define GRID    (1.0)
#ifdef DETAIL
        if ((target.x - TOLERANCE < p.x) && (p.x < target.x + TOLERANCE) && (target.y - TOLERANCE < p.y) && (p.y < target.y + TOLERANCE) && (target.z - TOLERANCE < p.z) && (p.z < target.z + TOLERANCE)) {
#else
        if ((target.x - GRID < p.x) && (p.x < target.x + GRID) && (target.y - GRID < p.y) && (p.y < target.y + GRID) && (target.z - GRID < p.z) && (p.z < target.z + GRID)) {
#endif
            {
                std::cout << "target p(" << p.x << "," << p.y << "," << p.z << ") index_size: " << index_size << " " << std::flush;
                for (int ic = 0; ic < index_size; ic++) {
                    int i = neighborhoodIndex[dst_index][index_x][index_y][index_z][ic];
                    std::cout << "(" << i << ")";
                }
                std::cout << " " << std::flush;
                hit = true;
            }
        }

        for (int ic = 0; ic < index_size; ic++) {
            int i = neighborhoodIndex[dst_index][index_x][index_y][index_z][ic];
            u = (p - facets[i]->v1).dot(V21invV21dotV21[i]);
            q = facets[i]->v1 + V21[i] * u;
            dir = (q - p).dot(facets[i]->normal);
            if ((abs_d = fabs(dir)) > min) continue;
            r = p + facets[i]->normal * dir;
            n1 = (r - facets[i]->v1).cross(V13[i]);
            n2 = (r - facets[i]->v2).cross(V21[i]);
            n3 = (r - facets[i]->v3).cross(V32[i]);
            s12 = n1.dot(n2); s23 = n2.dot(n3); s31 = n3.dot(n1);

            if ((s12 > 0.0) && (s23 > 0.0) && (s31 > 0.0)) {
                double candidate_min = abs_d - CALC_TOLERANCE;
                {
                    double d1 = (facets[i]->v1 - p).norm(); double d2 = (facets[i]->v2 - p).norm(); double d3 = (facets[i]->v3 - p).norm();
                    if ((candidate_min < d1) && (candidate_min < d2) && (candidate_min < d3)) { // sanity check..
                        min = candidate_min;
                        ret = dir;
                        selected.index = i;
                        if (ret > 0.0)
                            selected.side = INSIDE;
                        else
                            selected.side = OUTSIDE;
                        selected.q = q;
                        selected.property = ON_INNER;
                        correction = false;
                        continue;
                    }
                }
            }

            int q12_property, q13_property, q32_property;

            if (u <= 0.0) {
                q12 = facets[i]->v1;
                q12_property = ON_VERTEX + 1;
            }
            else if (u >= 1.0) {
                q12 = facets[i]->v2;
                q12_property = ON_VERTEX + 2;
            }
            else {
                /* q = facets[i]->v1 + V21[i] * u */
                q12 = q;
                q12_property = ON_EDGE + 1;
            }
            abs_d12 = (q12 - p).norm();

            u = (p - facets[i]->v3).dot(V13invV13dotV13[i]);
            if (u <= 0.0) {
                q13 = facets[i]->v3;
                q13_property = ON_VERTEX + 3;
            }
            else if (u >= 1.0) {
                q13 = facets[i]->v1;
                q13_property = ON_VERTEX + 1;
            }
            else {
                q13 = facets[i]->v3 + V13[i] * u;
                q13_property = ON_EDGE + 3;
            }
            abs_d13 = (q13 - p).norm();

            u = (p - facets[i]->v2).dot(V32invV32dotV32[i]);
            if (u <= 0.0) {
                q32 = facets[i]->v2;
                q32_property = ON_VERTEX + 2;
            }
            else if (u >= 1.0) {
                q32 = facets[i]->v3;
                q32_property = ON_VERTEX + 3;
            }
            else {
                q32 = facets[i]->v2 + V32[i] * u;
                q32_property = ON_EDGE + 2;
            }
            abs_d32 = (q32 - p).norm();

            int property;

            if ((abs_d12 <= abs_d13) && (abs_d12 <= abs_d32)) {
                q = q12;
                abs_d = abs_d12;
                property = q12_property;
            }
            else if ((abs_d13 < abs_d12) && (abs_d13 <= abs_d32)) {
                q = q13;
                abs_d = abs_d13;
                property = q13_property;
            }
            else {
                q = q32;
                abs_d = abs_d32;
                property = q32_property;
            }

            if (abs_d >= min && abs_d > second.abs_d)
                continue;

            dir = (q - p).dot(facets[i]->normal);
            if (dir > 0.0)
                side = INSIDE;
            else
                side = OUTSIDE;

            if (abs_d < min) {
                min = abs_d;
                if (side == INSIDE) {
                    ret = abs_d;
                }
                else {
                    ret = -abs_d;
                }

                if (third.side != UNDECIDED)
                    more.push_back(third);
                third = second;
                second = selected;

                selected.index = i;
                selected.side = side;
                selected.abs_d = abs_d;
                selected.q = q;
                selected.property = property;
                correction = true;
            }
            else {
                if (third.side != UNDECIDED)
                    more.push_back(third);
                third = second;
                second.index = i;
                second.side = side;
                second.abs_d = abs_d;
                second.q = q;
                second.property = property;
            }
        }

        if ((second.side != UNDECIDED) && (third.side != UNDECIDED)) {
            if ((selected.q - third.q).norm() < (selected.q - second.q).norm()) {
                std::swap(second, third);
            }
            if (!more.empty()) {
                sort(more.begin(), more.end(), [selected](const CANDIDATE& e1, const CANDIDATE& e2) { return (selected.q - e1.q).norm() < (selected.q - e2.q).norm(); });
                if ((selected.q - more[0].q).norm() < (selected.q - third.q).norm()) {
                    std::swap(third, more[0]);
                }
            }
        }

        if (correction == true) {
            if (((second.side != UNDECIDED) && (second.side != selected.side)) || ((third.side != UNDECIDED) && (third.side != selected.side)) || !more.empty()) {
                if ((selected.q - second.q).norm() < CALC_TOLERANCE * 100.0) {
                    //				std::cout << "correction ";
                    GLVertex outer_vector = ((selected.q - Facet::facetCenter(facets[selected.index])).normalize() + (selected.q - Facet::facetCenter(facets[second.index])).normalize()).normalize();
                    GLVertex normal_avg_vector = facets[selected.index]->normal + facets[second.index]->normal;
                    normal_vec_calc_count++;
                    if ((third.side != UNDECIDED) && ((selected.q - third.q).norm() < CALC_TOLERANCE * 100.0)) {
                        outer_vector += (selected.q - Facet::facetCenter(facets[third.index])).normalize();
                        outer_vector.normalize();
                        normal_avg_vector += facets[third.index]->normal;
                        normal_vec_calc_count++;
                        if (more.empty()) {
                            //						std::cout << "triple\n";
                        }
                        else {
                            //                        std::cout << "more triple...(" << more.size() << " facets)\n";
                            for (int i = 0; i < (int)more.size(); i++) {
                                if ((selected.q - more[i].q).norm() < CALC_TOLERANCE * 100.0) {
                                    outer_vector += (selected.q - Facet::facetCenter(facets[more[i].index])).normalize();
                                    outer_vector.normalize();
                                    normal_avg_vector += facets[more[i].index]->normal;
                                    normal_vec_calc_count++;
                                }
                            }
                        }
                    }
                    else {
                        //                    std::cout << "double\n" << std::flush;
                    }
                    normal_avg_vector.normalize();
                    if (normal_avg_vector.dot(outer_vector) < 0.0) {
                        outer_vector *= -1.0;
                        if (hit == true) {
                            std::cout << "negate outer_vector\n" << std::flush;
                            std::cout << " normal_avg_vector :(" << normal_avg_vector.x << "," << normal_avg_vector.y << "," << normal_avg_vector.z << ")\n" << std::flush;
                        }
                    }
                    test_vector = outer_vector;
                    if (!(normal_vec_calc_count == 1 && (second.side == selected.side))) {
                        if (outer_vector.dot(selected.q - p) < 0.0) {
                            if (selected.side == INSIDE) {
                                //          				std::cout << "SIDE WRONG!! INSIDE -> OUTSIDE p(" << p.x <<"," << p.y << "," << p.z << ")\n" <<  std::flush;
                                selected.side = OUTSIDE;
                                ret = -selected.abs_d;
                            }
                        }
                        else {
                            if (selected.side == OUTSIDE) {
                                //                          std::cout << "SIDE WRONG!! OUTSIDE -> INSIDE p(" << p.x <<"," << p.y << "," << p.z << ")\n" <<  std::flush;
                                selected.side = INSIDE;
                                ret = selected.abs_d;
                            }
                        }
                    }
                }
                else {
                    if ((second.side != selected.side) && ((selected.property & ON_EDGE) || (second.property & ON_EDGE))) {
                        int do_correct = false;
                        if ((selected.property & ON_EDGE) && (second.property & ON_EDGE)) {
                            GLVertex edge1, edge2, p1, p2, p;
                            if ((selected.property & ~ON_EDGE) == 1) {
                                edge1 = V21[selected.index];
                                p1 = facets[selected.index]->v1;
                            }
                            else if ((selected.property & ~ON_EDGE) == 2) {
                                edge1 = V32[selected.index];
                                p1 = facets[selected.index]->v2;
                            }
                            else {
                                edge1 = V32[selected.index];
                                p1 = facets[selected.index]->v3;
                            }
                            if ((second.property & ~ON_EDGE) == 1) {
                                edge2 = V21[second.index];
                                p2 = facets[second.index]->v1;
                            }
                            else if ((second.property & ~ON_EDGE) == 2) {
                                edge2 = V32[second.index];
                                p2 = facets[second.index]->v2;
                            }
                            else {
                                edge2 = V32[second.index];
                                p2 = facets[second.index]->v3;
                            }
                            edge1.normalize();
                            edge2.normalize();
                            p = (p1 - p2).normalize();
                            if ((edge1.cross(p).norm() < TOLERANCE) && (edge1.cross(edge2).norm() < TOLERANCE)) {
                                do_correct = true;
                            }
                        }
                        else if ((selected.property & ON_EDGE)) {
                            GLVertex edge, ev, p;
                            if ((selected.property & ~ON_EDGE) == 1) {
                                edge = V21[selected.index];
                                ev = facets[selected.index]->v1;
                            }
                            else if ((selected.property & ~ON_EDGE) == 2) {
                                edge = V32[selected.index];
                                ev = facets[selected.index]->v2;
                            }
                            else {
                                edge = V32[selected.index];
                                ev = facets[selected.index]->v3;
                            }
                            if ((second.property & ~ON_VERTEX) == 1) {
                                p = facets[second.index]->v1 - ev;
                            }
                            else if ((second.property & ~ON_VERTEX) == 2) {
                                p = facets[second.index]->v2 - ev;
                            }
                            else {
                                p = facets[second.index]->v3 - ev;
                            }
                            edge.normalize();
                            p.normalize();
                            if (edge.cross(p).norm() < TOLERANCE) {
                                do_correct = true;
                            }
                        }
                        else if ((second.property & ON_EDGE)) {
                            GLVertex edge, ev, p;
                            if ((second.property & ~ON_EDGE) == 1) {
                                edge = V21[second.index];
                                ev = facets[second.index]->v1;
                            }
                            else if ((second.property & ~ON_EDGE) == 2) {
                                edge = V32[second.index];
                                ev = facets[second.index]->v2;
                            }
                            else {
                                edge = V32[second.index];
                                ev = facets[second.index]->v3;
                            }
                            if ((selected.property & ~ON_VERTEX) == 1) {
                                p = facets[selected.index]->v1 - ev;
                            }
                            else if ((selected.property & ~ON_VERTEX) == 2) {
                                p = facets[selected.index]->v2 - ev;
                            }
                            else {
                                p = facets[selected.index]->v3 - ev;
                            }
                            edge.normalize();
                            p.normalize();
                            if (edge.cross(p).norm() < TOLERANCE) {
                                do_correct = true;
                            }
                        }
                        if (do_correct == true) {
                            GLVertex outer_vector = ((selected.q - Facet::facetCenter(facets[selected.index])).normalize() + (selected.q - Facet::facetCenter(facets[second.index])).normalize()).normalize();
                            GLVertex normal_avg_vector = facets[selected.index]->normal + facets[second.index]->normal;
                            if (normal_avg_vector.dot(outer_vector) < 0.0) {
                                outer_vector *= -1.0;
                            }
                            if (outer_vector.dot(selected.q - p) < 0.0) {
                                if (selected.side == INSIDE) {
                                    //                              std::cout << "SIDE WRONG!! INSIDE -> OUTSIDE p(" << p.x <<"," << p.y << "," << p.z << ")\n" <<  std::flush;
                                    selected.side = OUTSIDE;
                                    ret = -selected.abs_d;
                                }
                            }
                            else {
                                if (selected.side == OUTSIDE) {
                                    //                              std::cout << "SIDE WRONG!! OUTSIDE -> INSIDE p(" << p.x <<"," << p.y << "," << p.z << ")\n" <<  std::flush;
                                    selected.side = INSIDE;
                                    ret = selected.abs_d;
                                }
                            }
                        }
                    }
                }
            }
            // sanity check
            GLVertex v = (Facet::facetCenter(facets[selected.index]) - p).normalize();
            double distance = (Facet::facetCenter(facets[selected.index]) - p).norm();
            for (int ic = 0; ic < index_size; ic++) {
                int i = neighborhoodIndex[dst_index][index_x][index_y][index_z][ic];
                if (i == selected.index) continue;
                double v_n = v.dot(facets[i]->normal);
                if (fabs(v_n) < CALC_TOLERANCE) continue;
                double t = (facets[i]->v1 - p).dot(facets[i]->normal) / v_n;
                if (t < 0.0) continue;
                GLVertex r = p + v * t;
                n1 = (r - facets[i]->v1).cross(V13[i]);
                n2 = (r - facets[i]->v2).cross(V21[i]);
                n3 = (r - facets[i]->v3).cross(V32[i]);
                s12 = n1.dot(n2); s23 = n2.dot(n3); s31 = n3.dot(n1);
                if ((s12 > 0.0) && (s23 > 0.0) && (s31 > 0.0)) {
                    //std::cout << "sanity check\n" << std::flush;
                    if (t < distance) {
                        distance = t;
                        if ((selected.side == INSIDE) && (v_n < 0.0)) {
                            std::cout << "??? p:(" << p.x << "," << p.y << "," << p.z << ")\n" << std::flush;
                            selected.side = OUTSIDE;
                            ret = -selected.abs_d;
                        }
                        else if ((selected.side == OUTSIDE) && (v_n > 0.0)) {
                            std::cout << "??? p:(" << p.x << "," << p.y << "," << p.z << ")\n" << std::flush;
                            selected.side = INSIDE;
                            ret = selected.abs_d;
                        }
                    }
                }
            }
        }

#ifdef DETAIL
        if ((target.x - TOLERANCE < p.x) && (p.x < target.x + TOLERANCE) && (target.y - TOLERANCE < p.y) && (p.y < target.y + TOLERANCE) && (target.z - TOLERANCE < p.z) && (p.z < target.z + TOLERANCE))
#else
        if ((target.x - GRID < p.x) && (p.x < target.x + GRID) && (target.y - GRID < p.y) && (p.y < target.y + GRID) && (target.z - GRID < p.z) && (p.z < target.z + GRID))
#endif
        {
            std::cout << "normal_vec_calc_count :" << normal_vec_calc_count << "\n" << std::flush;

            if (third.side != UNDECIDED) {
                std::cout << "side(" << ((ret > 0) ? "INSIDE)" : "OUTSIDE):") << " facets:(" << selected.index << ")(" << second.index << ")(" << third.index << ")\n" << std::flush;
                std::cout << "(selected.q - second.q).norm :" << (selected.q - second.q).norm() << " selected.ads_d :" << selected.abs_d << " second.abs_d :" << second.abs_d << "\n" << std::flush;
                std::cout << "(selected.q - third.q).norm :" << (selected.q - third.q).norm() << " selected.ads_d :" << selected.abs_d << " third.abs_d :" << third.abs_d << "\n" << std::flush;
                std::cout << " selected.q:(" << selected.q.x << "," << selected.q.y << "," << selected.q.z << ")\n" << std::flush;
                std::cout << " second.q:  (" << second.q.x << "," << second.q.y << "," << second.q.z << ")\n" << std::flush;
                std::cout << " third.q:   (" << third.q.x << "," << third.q.y << "," << third.q.z << ")\n" << std::flush;
                std::cout << " test_vector:(" << test_vector.x << "," << test_vector.y << "," << test_vector.z << ")\n" << std::flush;
                std::cout << " facets normal:(" << selected.index << ")(" << facets[selected.index]->normal.x << "," << facets[selected.index]->normal.y << "," << facets[selected.index]->normal.z << ")\n" << std::flush;
                std::cout << " facets normal:(" << second.index << ")(" << facets[second.index]->normal.x << "," << facets[second.index]->normal.y << "," << facets[second.index]->normal.z << ")\n" << std::flush;
                std::cout << " facets normal:(" << third.index << ")(" << facets[third.index]->normal.x << "," << facets[third.index]->normal.y << "," << facets[third.index]->normal.z << ")\n" << std::flush;
                std::cout << " facets:(" << selected.index << ")(" << facets[selected.index]->v1.x << "," << facets[selected.index]->v1.y << "," << facets[selected.index]->v1.z << ")(" << facets[selected.index]->v2.x << "," << facets[selected.index]->v2.y << "," << facets[selected.index]->v2.z << ")(" << facets[selected.index]->v3.x << "," << facets[selected.index]->v3.y << "," << facets[selected.index]->v3.z << ")\n" << std::flush;
                std::cout << " facets:(" << second.index << ")(" << facets[second.index]->v1.x << "," << facets[second.index]->v1.y << "," << facets[second.index]->v1.z << ")(" << facets[second.index]->v2.x << "," << facets[second.index]->v2.y << "," << facets[second.index]->v2.z << ")(" << facets[second.index]->v3.x << "," << facets[second.index]->v3.y << "," << facets[second.index]->v3.z << ")\n" << std::flush;
                std::cout << " facets:(" << third.index << ")(" << facets[third.index]->v1.x << "," << facets[third.index]->v1.y << "," << facets[third.index]->v1.z << ")(" << facets[third.index]->v2.x << "," << facets[third.index]->v2.y << "," << facets[third.index]->v2.z << ")(" << facets[third.index]->v3.x << "," << facets[third.index]->v3.y << "," << facets[third.index]->v3.z << ")\n" << std::flush;
                if (!more.empty()) {
                    for (unsigned int i = 0; i < more.size(); i++) {
                        std::cout << "  (selected.q - more[" << i << "].q).norm :" << (selected.q - more[i].q).norm() << " selected.ads_d :" << selected.abs_d << " more[" << i << "].abd_d :" << more[i].abs_d << "\n" << std::flush;
                        std::cout << "   facets normal: more[" << i << "](" << more[i].index << ")(" << facets[more[i].index]->normal.x << "," << facets[more[i].index]->normal.y << "," << facets[more[i].index]->normal.z << ")\n" << std::flush;
                        std::cout << "   facets: more[" << i << "](" << more[i].index << ")(" << facets[more[i].index]->v1.x << "," << facets[more[i].index]->v1.y << "," << facets[more[i].index]->v1.z << ")(" << facets[more[i].index]->v2.x << "," << facets[more[i].index]->v2.y << "," << facets[more[i].index]->v2.z << ")(" << facets[more[i].index]->v3.x << "," << facets[more[i].index]->v3.y << "," << facets[more[i].index]->v3.z << ")\n" << std::flush;
                    }
                }
            }
            else {
                std::cout << "side(" << ((ret > 0) ? "INSIDE)" : "OUTSIDE):") << " facets:(" << selected.index << ")(" << second.index << ")\n" << std::flush;
                std::cout << "(selected.q - second.q).norm :" << (selected.q - second.q).norm() << " selected.ads :" << selected.abs_d << " second.abs_d :" << second.abs_d << "\n" << std::flush;
                std::cout << " selected.q:(" << selected.q.x << "," << selected.q.y << "," << selected.q.z << ")\n" << std::flush;
                std::cout << " second.q:  (" << second.q.x << "," << second.q.y << "," << second.q.z << ")\n" << std::flush;
                std::cout << " test_vector:(" << test_vector.x << "," << test_vector.y << "," << test_vector.z << ")\n" << std::flush;
                std::cout << " facets normal:(" << selected.index << ")(" << facets[selected.index]->normal.x << "," << facets[selected.index]->normal.y << "," << facets[selected.index]->normal.z << ")\n" << std::flush;
                std::cout << " facets normal:(" << second.index << ")(" << facets[second.index]->normal.x << "," << facets[second.index]->normal.y << "," << facets[second.index]->normal.z << ")\n" << std::flush;
                std::cout << " facets:(" << selected.index << ")(" << facets[selected.index]->v1.x << "," << facets[selected.index]->v1.y << "," << facets[selected.index]->v1.z << ")(" << facets[selected.index]->v2.x << "," << facets[selected.index]->v2.y << "," << facets[selected.index]->v2.z << ")(" << facets[selected.index]->v3.x << "," << facets[selected.index]->v3.y << "," << facets[selected.index]->v3.z << ")\n" << std::flush;
                std::cout << " facets:(" << second.index << ")(" << facets[second.index]->v1.x << "," << facets[second.index]->v1.y << "," << facets[second.index]->v1.z << ")(" << facets[second.index]->v2.x << "," << facets[second.index]->v2.y << "," << facets[second.index]->v2.z << ")(" << facets[second.index]->v3.x << "," << facets[second.index]->v3.y << "," << facets[second.index]->v3.z << ")\n" << std::flush;
            }
        }
        return ret;		// positive inside. negative outside.
        }


    //************* CutterVolume **************/

    CutterVolume::CutterVolume() {
        radius = 0.0;
        length = 0.0;
        enableholder = false;
        holderradius = 0.0;
        holderlength = 0.0;
    }

    void CutterVolume::calcBBHolder() {
        bbHolder.clear();
        GLVertex maxpt = GLVertex(center.x + holderradius + TOLERANCE, center.y + holderradius + TOLERANCE, center.z + length + holderlength + TOLERANCE);
        GLVertex minpt = GLVertex(center.x - holderradius - TOLERANCE, center.y - holderradius - TOLERANCE, center.z + length - TOLERANCE);
        bbHolder.addPoint(maxpt);
        bbHolder.addPoint(minpt);
    }

    //************* CylCutterVolume **************/

    CylCutterVolume::CylCutterVolume() {
        radius = 0.0;
        length = 0.0;
    }

    void CylCutterVolume::calcBB() {
        bb.clear();
        GLVertex maxpt = GLVertex(center.x + maxradius + TOLERANCE, center.y + maxradius + TOLERANCE, center.z + length + TOLERANCE);
        GLVertex minpt = GLVertex(center.x - maxradius - TOLERANCE, center.y - maxradius - TOLERANCE, center.z - TOLERANCE);
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
        if (enableholder)
            calcBBHolder();
    }

    double CylCutterVolume::dist(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif

        if (t.z >= 0.0) {
            return t.z > length ? holderradius - d : radius - d;  // positive inside. negative outside.
        }
        else {
            // if we are under the cutter, then return distance to flat cutter bottom
            if (d < radius)
                return t.z;
            else {
                // outside the cutter, return a distance to the outer "ring" of the cutter
                GLVertex n = GLVertex(t.x, t.y, 0.0);
                n = n * (radius / d);  // 1/d means normalization
                return -((t - n).norm());
            }
        }
    }

    Cutting CylCutterVolume::dist_cd(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
        Cutting result = { t.z, NO_COLLISION, 1 };
        double rdiff = radius - d;

        if (t.z >= 0.0) {
            result.collision |= ((t.z > flutelength) && ((rdiff = neckradius - d) > COLLISION_TOLERANCE)) ? NECK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > reachlength) && ((rdiff = shankradius - d) > COLLISION_TOLERANCE)) ? SHANK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > length) && ((rdiff = holderradius - d) > COLLISION_TOLERANCE)) ? HOLDER_COLLISION : NO_COLLISION;
            result.f = rdiff < t.z ? rdiff : t.z;  // positive inside. negative outside.
            if (rdiff > effective_radius) result.count = 4;
            return result;
        }
        else {
            // if we are under the cutter, then return distance to flat cutter bottom
            if (d < radius)
                return result;
            else {
                // outside the cutter, return a distance to the outer "ring" of the cutter
                GLVertex n = GLVertex(t.x, t.y, 0.0);
                n = n * (radius / d);  // 1/d means normalization
                result.f = -((t - n).norm());
                return result;
            }
        }
    }

    //************* BallCutterVolume **************/

    BallCutterVolume::BallCutterVolume() {
        radius = 0.0;
        length = 0.0;
    }

    void BallCutterVolume::calcBB() {
        bb.clear();
        GLVertex maxpt = GLVertex(center.x + maxradius + TOLERANCE, center.y + maxradius + TOLERANCE, center.z + length + TOLERANCE);
        GLVertex minpt = GLVertex(center.x - maxradius - TOLERANCE, center.y - maxradius - TOLERANCE, center.z - radius - TOLERANCE);
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
        if (enableholder)
            calcBBHolder();
    }

    double BallCutterVolume::dist(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
#else
        GLVertex t = p - center;
#endif

        if (t.z < 0.0)
#ifdef MULTI_AXIS
            return radius - (rotated_p - GLVertex(center.x, center.y, center.z)).norm(); // positive inside. negative outside.
#else
            return radius - (center - p).norm(); // positive inside. negative outside.
#endif
        else
#ifdef MULTI_AXIS
            return radius - (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
            return radius - (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
    }

    Cutting BallCutterVolume::dist_cd(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
#else
        GLVertex t = p - center;
#endif
        Cutting result = { 0.0, NO_COLLISION, 1 };

        if (t.z < 0) {
            result.f = radius - t.norm();
            return result;
        }
        else {
#ifdef MULTI_AXIS
            double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
            double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
            result.f = radius - d;
            result.collision |= ((t.z > flutelength) && ((result.f = neckradius - d) > COLLISION_TOLERANCE)) ? NECK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > reachlength) && ((result.f = shankradius - d) > COLLISION_TOLERANCE)) ? SHANK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > length) && ((result.f = holderradius - d) > COLLISION_TOLERANCE)) ? HOLDER_COLLISION : NO_COLLISION;
            return result;
        }
    }

    //************* BullCutterVolume **************/

    BullCutterVolume::BullCutterVolume() {
        radius = 0.0;
        length = 0.0;
        r1 = r2 = 0.0;
    }

    void BullCutterVolume::calcBB() {
        bb.clear();
        GLVertex maxpt = GLVertex(center.x + maxradius + TOLERANCE, center.y + maxradius + TOLERANCE, center.z + length + TOLERANCE);
        GLVertex minpt = GLVertex(center.x - maxradius - TOLERANCE, center.y - maxradius - TOLERANCE, center.z - TOLERANCE);
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
        if (enableholder)
            calcBBHolder();
    }

    double BullCutterVolume::dist(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif

        if (t.z >= r2) {  // cylindrical part, above toroid
            return t.z > length ? holderradius - d : radius - d;  // positive inside. negative outside.
        }
        else {
            // if we are under the cutter, then return distance to flat cutter bottom
            if (d < r1)  // cylindrical part, inside toroid
                return t.z;
            else {
                // toroid
                GLVertex n = GLVertex(t.x, t.y, 0.0);
                n = n * (r1 / d);  // 1/d means normalization
                n += GLVertex(0.0, 0.0, r2);
                return -((t - n).norm()) + r2;
            }
        }
    }

    Cutting BullCutterVolume::dist_cd(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
        Cutting result = { t.z, NO_COLLISION, 1 };
        double rdiff = radius - d;

        if (t.z >= r2) {  // cylindrical part, above toroid
            result.collision |= ((t.z > flutelength) && ((rdiff = neckradius - d) > COLLISION_TOLERANCE)) ? NECK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > reachlength) && ((rdiff = shankradius - d) > COLLISION_TOLERANCE)) ? SHANK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > length) && ((rdiff = holderradius - d) > COLLISION_TOLERANCE)) ? HOLDER_COLLISION : NO_COLLISION;
            result.f = rdiff < t.z ? rdiff : t.z;  // positive inside. negative outside.
            return result;
        }
        else {
            // if we are under the cutter, then return distance to flat cutter bottom
            if (d < r1)  // cylindrical part, inside toroid
                return result;
            else {
                // toroid
                GLVertex n = GLVertex(t.x, t.y, 0.0);
                n = n * (r1 / d);  // 1/d means normalization
                n += GLVertex(0.0, 0.0, r2);
                result.f = -((t - n).norm()) + r2;
                return result;
            }
        }
    }

    //************* ConeCutterVolume **************/

    ConeCutterVolume::ConeCutterVolume() {
        radius = 0.0;
        length = 0.0;
        flutelength = 0.0;
        r1 = r2 = 0.0;
        incline_coff = 0.0;
    }

    void ConeCutterVolume::calcBB() {
        bb.clear();
        GLVertex maxpt = GLVertex(center.x + maxradius + TOLERANCE, center.y + maxradius + TOLERANCE, center.z + length + TOLERANCE);
        GLVertex minpt = GLVertex(center.x - maxradius - TOLERANCE, center.y - maxradius - TOLERANCE, center.z - TOLERANCE);
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
        if (enableholder)
            calcBBHolder();
    }

    double ConeCutterVolume::dist(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
        double rdiff;

        if (0.0 <= t.z && t.z <= flutelength)
            rdiff = (incline_coff * t.z + r1) - d;
        else
            rdiff = radius - d;

        if (t.z >= 0.0) {
            return t.z > length ? holderradius - d : rdiff;  // positive inside. negative outside.
        }
        else {
            // if we are under the cutter, then return distance to flat cutter bottom
            if (d < r1)
                return t.z;
            else {
                // toroid
                GLVertex n = GLVertex(t.x, t.y, 0.0);
                n = n * (r1 / d);  // 1/d means normalization
                n += GLVertex(0.0, 0.0, r2);
                return -((t - n).norm());
            }
        }
    }

    Cutting ConeCutterVolume::dist_cd(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
        Cutting result = { t.z, NO_COLLISION, 1 };
        double rdiff;

        if (0.0 <= t.z && t.z <= flutelength)
            rdiff = (incline_coff * t.z + r1) - d;
        else
            rdiff = radius - d;

        if (t.z >= 0.0) {
            result.collision |= ((t.z > flutelength) && ((rdiff = neckradius - d) > COLLISION_TOLERANCE)) ? NECK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > reachlength) && ((rdiff = shankradius - d) > COLLISION_TOLERANCE)) ? SHANK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > length) && ((rdiff = holderradius - d) > COLLISION_TOLERANCE)) ? HOLDER_COLLISION : NO_COLLISION;
            result.f = rdiff < t.z ? rdiff : t.z;  // positive inside. negative outside.
            return result;
        }
        else {
            // if we are under the cutter, then return distance to flat cutter bottom
            if (d < r1)
                return result;
            else {
                // outside the cutter, return a distance to the outer "ring" of the cutter
                GLVertex n = GLVertex(t.x, t.y, 0.0);
                n = n * (r1 / d);  // 1/d means normalization
                result.f = -((t - n).norm());
                return result;
            }
        }
    }

    //************* DrillVolume **************/

    DrillVolume::DrillVolume() {
        radius = 0.0;
        length = 0.0;
        flutelength = 0.0;
        tip_hight = 0.0;
        tip_angle = 118.0;
        incline_coff = 0.0;
    }

    void DrillVolume::calcBB() {
        bb.clear();
        GLVertex maxpt = GLVertex(center.x + maxradius + TOLERANCE, center.y + maxradius + TOLERANCE, center.z + length + TOLERANCE);
        GLVertex minpt = GLVertex(center.x - maxradius - TOLERANCE, center.y - maxradius - TOLERANCE, center.z - TOLERANCE);
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
        if (enableholder)
            calcBBHolder();
    }

    double DrillVolume::dist(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
        double rdiff;

        if (0.0 <= t.z && t.z <= tip_hight)
            rdiff = incline_coff * t.z - d;
        else
            rdiff = radius - d;

        if (t.z >= 0.0) {
            return t.z > length ? holderradius - d : rdiff;  // positive inside. negative outside.
        }
        else {
            return t.z;
        }
    }

    Cutting DrillVolume::dist_cd(const GLVertex & p) const {
#ifdef MULTI_AXIS
        GLVertex rotated_p = p;
        rotated_p = rotated_p.rotateAC(angle.x, angle.z);
        GLVertex t = rotated_p - center;
        double d = (rotated_p - GLVertex(center.x, center.y, rotated_p.z)).norm();
#else
        GLVertex t = p - center;
        double d = (p - GLVertex(center.x, center.y, p.z)).norm();
#endif
        Cutting result = { t.z, NO_COLLISION, 1 };
        double rdiff = radius - d;

        if (0.0 <= t.z && t.z <= tip_hight)
            rdiff = incline_coff * t.z - d;

        if (t.z >= 0.0) {
            result.collision |= ((t.z > flutelength) && ((rdiff = neckradius - d) > COLLISION_TOLERANCE)) ? NECK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > reachlength) && ((rdiff = shankradius - d) > COLLISION_TOLERANCE)) ? SHANK_COLLISION : NO_COLLISION;
            result.collision |= ((t.z > length) && ((rdiff = holderradius - d) > COLLISION_TOLERANCE)) ? HOLDER_COLLISION : NO_COLLISION;
            result.f = rdiff < t.z ? rdiff : t.z;  // positive inside. negative outside.
            //if (rdiff > effective_radius) result.count = 4;
            return result;
        }
        else {
            // if we are under the cutter, then return distance to flat cutter bottom
            if (d < tip_radius)
                return result;
            else {
                // outside the cutter, return a distance to the outer "ring" of the cutter
                GLVertex n = GLVertex(t.x, t.y, 0.0);
                n = n * (tip_radius / d);  // 1/d means normalization
                result.f = -((t - n).norm());
                return result;
            }
        }
    }

    ///added hust

    //************* broaching_AptCutterVolume **************/
#pragma region
    broaching_AptCutterVolume::broaching_AptCutterVolume() {
        type = APT_VOLUME;
        cuttertype = APT;
        radius = 0.0;
        length = 0.0;
        center = GLVertex(0, 0, 0);
        flutelength = 0.0;
        //初始化运动学参数
        q_x[0.0] = 0.0;
        dot_q_x[0.0] = 0.0;
        ddot_q_x[0.0] = 0.0;
        q_y[0.0] = 0.0;
        dot_q_y[0.0] = 0.0;
        ddot_q_y[0.0] = 0.0;
        machining_residual_layer_depth_mm = 0.30;
        machining_residual_step_id = 0;
        machining_residual_stroke_mm = 0.0;
        machining_residual_events_enabled = false;
    }

    namespace {
        GLVertex normalizedOrZero(const GLVertex& value) {
            const double length = value.norm();
            if (length <= 1e-12 || !std::isfinite(length)) {
                return GLVertex(0, 0, 0);
            }
            return value * (1.0 / length);
        }
    }

    void broaching_AptCutterVolume::setMachiningResidualContext(int stepId, double strokeMm) {
        machining_residual_step_id = stepId;
        machining_residual_stroke_mm = strokeMm;
    }

    void broaching_AptCutterVolume::setMachiningResidualEventsEnabled(bool enabled) {
        machining_residual_events_enabled = enabled;
    }

    void broaching_AptCutterVolume::clearMachiningContactEvents() {
        machining_contact_events.clear();
    }

    void broaching_AptCutterVolume::addMachiningContactEvent(int bladeId, int insideIndex,
        const GLVertex& p0, const GLVertex& p1,
        const GLVertex& materialPoint, double sweepWidthMm) {
        if (!machining_residual_events_enabled) {
            return;
        }
        if (bladeId < 0 || insideIndex < 0) {
            return;
        }

        const GLVertex segment = p1 - p0;
        if (segment.norm() <= 1e-12) {
            return;
        }

        GLVertex local_x = normalizedOrZero(GLVertex(v_x, v_y, v_z));
        if (local_x.norm() <= 1e-12) {
            local_x = normalizedOrZero(GLVertex(dx, dy, dz));
        }
        if (local_x.norm() <= 1e-12) {
            return;
        }

        GLVertex local_z = normalizedOrZero(segment.cross(local_x));
        if (local_z.norm() <= 1e-12) {
            return;
        }

        if ((materialPoint - p0).dot(local_z) < 0.0) {
            local_z *= -1.0;
        }

        GLVertex local_y = normalizedOrZero(local_z.cross(local_x));
        if (local_y.norm() <= 1e-12) {
            return;
        }

        const double displacement = std::sqrt(dx * dx + dy * dy + dz * dz);
        double event_sweep_width = sweepWidthMm;
        if (event_sweep_width <= 0.0 || !std::isfinite(event_sweep_width)) {
            event_sweep_width = std::max(displacement, static_cast<double>(cube_resolution_1));
        }

        MachiningContactEvent event;
        event.step_id = machining_residual_step_id;
        event.stroke_mm = machining_residual_stroke_mm;
        event.tool_angle = tool_angle;
        event.blade_id = bladeId;
        event.inside_index = insideIndex;
        event.surface_p0_mm = p0;
        event.surface_p1_mm = p1;
        event.local_x = local_x;
        event.local_y = local_y;
        event.local_z = local_z;
        event.sweep_width_mm = event_sweep_width;
        event.layer_depth_mm = machining_residual_layer_depth_mm;
        machining_contact_events.push_back(event);
    }

    void broaching_AptCutterVolume::calcBB() {
        bb.clear();

    }

    double broaching_AptCutterVolume::get_blade_angle_1(double z)const {
        //std::cout << "blade_angle_1: " << tool_angle+tan(a_1)*z/(tan(b_1)+r_1-H_1*tan(b_1));
        return tool_angle + tan(a_1) * (z - z_start) / (tan(b_1) + r_1 - H_1 * tan(b_1));
    }

    double broaching_AptCutterVolume::get_blade_angle_2(double z)const {
        return tool_angle + tan(a_1) * H_1 / (tan(b_1) + r_1 - H_1 * tan(b_1)) + tan(a_2) * (z - H_1 - z_start) / (tan(b_2) + r_2 - H_2 * tan(b_2));
    }

    double broaching_AptCutterVolume::dist(const GLVertex & p) const {
        GLVertex t = p - center;
        //std::cout << "AptCutterVol
        //std::cout << "p: (" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
        //std::cout << "center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
        //std::cout << "t: (" << t.x << ", " << t.y << ", " << t.z << ")" << std::endl;
        for (const auto& seg : segments) {
            //std::cout << "seg.z_start: " << seg.z_start << "; "<< "seg.z_end: " << seg.z_end << std::endl;
            //std::cout << "p.z: " << p.z;
            if (p.z >= seg.z_start && p.z <= seg.z_end) {
                //std::cout << "seg.type: " << seg.type<< std::endl;
                switch (seg.type) {
                case 0:
                    //std::cout << "dist: " << -seg.radius1 + sqrt(t.x * t.x + t.y * t.y)<< std::endl;
                    return seg.radius1 - sqrt(t.x * t.x + t.y * t.y);
                case 1: {
                    return seg.radius1 - t.norm();
                }
                case 2: {
                    double r = seg.radius1 + (seg.radius2 - seg.radius1) * (p.z - seg.z_start) / (seg.z_end - seg.z_start);
                    //std::cout << "dist: " << -r + sqrt(t.x * t.x + t.y * t.y)<< std::endl;
                    return r - sqrt(t.x * t.x + t.y * t.y);
                }
                }
            }
        }
        // 不在任何分段内，返回负值（外部）
        return t.z;
    }

    Cutting broaching_AptCutterVolume::dist_cd(const GLVertex & p) const {
        Cutting result = { 0.0, NO_COLLISION, 1 };
        result.f = dist(p);
        // 可根据需要扩展碰撞类型判定
        return result;
    }

    std::vector<std::vector<std::vector<double>>>
        broaching_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            const std::vector<size_t>&modes_to_read) {  // 修改参数为模态索引列表
        if (pre_vibration_vectors.empty()) {
            return {};
        }

        // 检查数据完整性
        size_t total_dofs = pre_vibration_vectors[0].size();
        if (total_dofs % num_dofs != 0) {
            throw std::invalid_argument("总自由度数量不是每个节点自由度数的整数倍");
        }

        // 创建三维向量（根据指定模态数量）
        size_t num_nodes = total_dofs / num_dofs;
        std::vector<std::vector<std::vector<double>>> vibration_vectors(
            modes_to_read.size(),  // 根据指定模态数量创建
            std::vector<std::vector<double>>(num_dofs, std::vector<double>(num_nodes))
        );

        // 转换指定模态数据
        for (size_t i = 0; i < modes_to_read.size(); ++i) {
            size_t mode = modes_to_read[i];
            if (mode >= pre_vibration_vectors.size()) {
                throw std::invalid_argument("请求的模态阶数超过数据范围");
            }

            // 检查模态数据一致性
            if (pre_vibration_vectors[mode].size() != total_dofs) {
                throw std::invalid_argument("不同模态的特征向量长度不一致");
            }

            // 填充数据
            for (int dof = 0; dof < num_dofs; ++dof) {
                for (size_t node = 0; node < num_nodes; ++node) {
                    size_t index = node * num_dofs + dof;
                    vibration_vectors[i][dof][node] = -pre_vibration_vectors[mode][index];
                }
            }
        }

        return vibration_vectors;
    }

    // 保留原函数用于向后兼容
    std::vector<std::vector<std::vector<double>>>
        broaching_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            int num_modes_to_read) {
        std::vector<size_t> modes;
        for (size_t i = 0; i < static_cast<size_t>(num_modes_to_read); ++i) {
            modes.push_back(i);
        }
        return convertTo3DVibrationVectors(pre_vibration_vectors, num_dofs, modes);
    }

    // 计算点到线段在切削速度垂直平面上的投影距离
    double broaching_AptCutterVolume::calculateCutThickness(const GLVertex & current_point, const GLVertex & prev_point1, const GLVertex & prev_point2,
        const GLVertex & velocity_vector) {
        // 切削速度向量
        GLVertex v = velocity_vector;

        // 计算切削速度垂直平面的法向量（切削速度向量本身）
        GLVertex plane_normal = v;
        plane_normal.normalize();

        // 计算前一个切削刃的线段向量
        GLVertex segment_vector = prev_point2 - prev_point1;

        // 计算当前点到线段起点的向量
        GLVertex point_to_start = current_point - prev_point1;

        // 计算线段向量在切削速度垂直平面上的投影
        GLVertex segment_projection = segment_vector - plane_normal * (segment_vector.dot(plane_normal));

        // 计算当前点到线段起点的向量在切削速度垂直平面上的投影
        GLVertex point_projection = point_to_start - plane_normal * (point_to_start.dot(plane_normal));

        // 计算投影点在线段投影上的参数t
        double t = 0.0;
        double segment_projection_length_sq = segment_projection.dot(segment_projection);

        if (segment_projection_length_sq > 1e-12) {
            t = point_projection.dot(segment_projection) / segment_projection_length_sq;
            t = std::max(0.0, std::min(1.0, t)); // 限制t在[0,1]范围内
        }

        // 计算投影点
        GLVertex projected_point = prev_point1 + segment_vector * t;

        // 计算当前点到投影点的向量在切削速度垂直平面上的投影
        GLVertex distance_vector = current_point - projected_point;
        GLVertex projected_distance = distance_vector - plane_normal * (distance_vector.dot(plane_normal));

        // 返回投影距离
        return projected_distance.norm();
    }


    int broaching_AptCutterVolume::readBladeAnglesFromFile(const std::string & filename, int blade_id) {
        // 确保blade_id在有效范围内
        if (blade_id < 0) {
            std::cerr << "Invalid blade_id: " << blade_id << std::endl;
            return 0;
        }

        // 调整容器大小以容纳新的切削刃数据
        while (original_blade_points.size() <= static_cast<size_t>(blade_id)) {
            original_blade_points.emplace_back();
            blade_angles_gamma.emplace_back();
            blade_angles_alpha.emplace_back();
            blade_cut_h.emplace_back();
            blade_rakeface_id.emplace_back();
            blade_rakeface_vertex.emplace_back();
        }

        // 清空当前切削刃的数据
        original_blade_points[blade_id].clear();
        blade_angles_gamma[blade_id].clear();
        blade_angles_alpha[blade_id].clear();
        blade_cut_h[blade_id].clear();
        blade_rakeface_id[blade_id].clear();
        blade_rakeface_vertex[blade_id].clear();

        const QString qFilename = QString::fromUtf8(filename.c_str());
        QFile file(qFilename);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error opening blade file:" << qFilename << file.errorString();
            return 0;
        }

        // 临时存储所有读取的点
        std::vector<GLVertex> temp_points;
        std::vector<double> temp_gamma;
        std::vector<double> temp_alpha;
        std::vector<int> temp_rakeface_id;
        std::vector<GLVertex> temp_rakeface_vertex;

        QTextStream in(&file);
        while (!in.atEnd()) {
            std::string line = in.readLine().toStdString();
            std::istringstream iss(line);
            std::vector<double> values;  // 临时存储当前行的所有数值
            double val;

            // 读取当前行的所有数值到vector中
            while (iss >> val) {
                values.push_back(val);
            }

            // 检查是否有至少6列数据（索引0~5）
            if (values.size() >= 6) {
                temp_points.emplace_back(values[0], values[1], values[2]);
                temp_gamma.push_back(values[4]);
                temp_alpha.push_back(values[5]);
                temp_rakeface_id.push_back(static_cast<int>(values[8]));
                temp_rakeface_vertex.emplace_back(values[9], values[10], values[11]);
            }
            else {
                std::cerr << "Invalid line format: " << line << std::endl;
            }
        }
        file.close();

        // 计算切削厚度并存储数据
        for (size_t i = 0; i < temp_points.size(); ++i) {
            original_blade_points[blade_id].push_back(temp_points[i]);
            blade_angles_gamma[blade_id].push_back(temp_gamma[i]);
            blade_angles_alpha[blade_id].push_back(temp_alpha[i]);
            blade_rakeface_id[blade_id].push_back(temp_rakeface_id[i]);
            blade_rakeface_vertex[blade_id].push_back(temp_rakeface_vertex[i]);

            // 计算切削厚度
            double cut_thickness = 0.0;

            if (blade_id > 0 && original_blade_points.size() > static_cast<size_t>(blade_id - 1) &&
                !original_blade_points[blade_id - 1].empty()) {

                // 切削速度向量
                GLVertex velocity_vector(dx, dy, dz);

                // 遍历前一个切削刃的所有相邻点对
                const auto& prev_blade_points = original_blade_points[blade_id - 1];
                double min_distance = std::numeric_limits<double>::max();

                for (size_t j = 0; j < prev_blade_points.size() - 1; ++j) {
                    double distance = calculateCutThickness(temp_points[i], prev_blade_points[j],
                        prev_blade_points[j + 1], velocity_vector);
                    min_distance = std::min(min_distance, distance);
                }

                cut_thickness = min_distance;
            }
            else {
                // 如果是第一个切削刃或前一个切削刃没有数据，使用默认值
                cut_thickness = (temp_points.size() > i && temp_gamma.size() > i && temp_alpha.size() > i) ?
                    ((i < temp_gamma.size() && i < temp_alpha.size()) ? 0.0 : 0.0) : 0.0;
            }

            blade_cut_h[blade_id].push_back(cut_thickness);
        }
        return 1;

    }

    void broaching_AptCutterVolume::calculatePosition_balde() {

        int blade_id = blade_num;

        // 1. 首先将当前blade_id的点（原始点和偏移点）收集到一个临时容器中
        std::vector<GLVertex> all_points;

        // 确保blade_id在有效范围内
        if (blade_id < 0 || blade_id >= static_cast<int>(original_blade_points.size())) {
            return; // 无效的blade_id，直接返回
        }

        // 确保blade_points的大小足够容纳当前blade_id
        if (blade_points.size() <= static_cast<size_t>(blade_id)) {
            blade_points.resize(blade_id + 1);
        }

        // 处理当前切削刃
        const auto& original_blade = original_blade_points[blade_id];
        auto& current_blade = blade_points[blade_id];

        current_blade.clear(); // 清空当前blade的点
        current_blade.reserve(original_blade.size());

        for (const auto& point : original_blade) {
            GLVertex offset_blade_point;
            offset_blade_point.x = point.x + center.x; // 累加 x 分量
            offset_blade_point.y = point.y + center.y; // 累加 y 分量
            offset_blade_point.z = point.z + center.z; // 累加 z 分量
            current_blade.push_back(offset_blade_point);
        }

        // 添加当前blade的原始点（已加上中心偏移）
        all_points.reserve(original_blade.size() * 2); // 预分配空间
        for (const auto& point : current_blade) {
            GLVertex original_point;
            original_point.x = point.x;
            original_point.y = point.y;
            original_point.z = point.z;
            all_points.push_back(original_point);

            // 添加偏移点
            GLVertex offset_point;
            offset_point.x = original_point.x - dx;
            offset_point.y = original_point.y - dy;
            offset_point.z = original_point.z - dz;
            all_points.push_back(offset_point);
        }

        // 首先找到当前blade所有点的最小和最大坐标值
        GLVertex min_coord = all_points.empty() ? GLVertex() : all_points[0];
        GLVertex max_coord = all_points.empty() ? GLVertex() : all_points[0];

        for (const auto& point : all_points) {
            min_coord.x = std::min(min_coord.x, point.x);
            min_coord.y = std::min(min_coord.y, point.y);
            min_coord.z = std::min(min_coord.z, point.z);

            max_coord.x = std::max(max_coord.x, point.x);
            max_coord.y = std::max(max_coord.y, point.y);
            max_coord.z = std::max(max_coord.z, point.z);
        }

        //允许一定的误差
        min_coord.x = min_coord.x - 0.1 * (max_coord.x - min_coord.x);
        min_coord.y = min_coord.y - 1 * (max_coord.y - min_coord.y);
        min_coord.z = min_coord.z - 0.1 * (max_coord.z - min_coord.z);

        max_coord.x = max_coord.x + 0.1 * (max_coord.x - min_coord.x);
        max_coord.y = max_coord.y + 1 * (max_coord.y - min_coord.y);
        max_coord.z = max_coord.z + 0.1 * (max_coord.z - min_coord.z);

        // 构造包围盒的8个顶点
        bb_points = std::make_tuple(
            GLVertex{ min_coord.x, min_coord.y, min_coord.z }, // 0: min, min, min
            GLVertex{ max_coord.x, min_coord.y, min_coord.z }, // 1: max, min, min
            GLVertex{ max_coord.x, max_coord.y, min_coord.z }, // 2: max, max, min
            GLVertex{ min_coord.x, max_coord.y, min_coord.z }, // 3: min, max, min
            GLVertex{ min_coord.x, min_coord.y, max_coord.z }, // 4: min, min, max
            GLVertex{ max_coord.x, min_coord.y, max_coord.z }, // 5: max, min, max
            GLVertex{ max_coord.x, max_coord.y, max_coord.z }, // 6: max, max, max
            GLVertex{ min_coord.x, max_coord.y, max_coord.z }  // 7: min, max, max
        );


        // 使用map来存储每个面ID对应的面信息
        std::map<int, PlaneInfo> face_map;

        planes.clear();

        const auto& face_ids = blade_rakeface_id[blade_id];
        const auto& face_normals = blade_rakeface_vertex[blade_id];
        const auto& face_points = original_blade_points[blade_id];

        // 遍历所有点，收集面信息
        for (size_t i = 0; i < face_ids.size(); ++i) {
            int face_id = face_ids[i];
            if (face_map.find(face_id) == face_map.end()) {
                // 找到一个新的面，创建面信息
                PlaneInfo plane;
                plane.normal = face_normals[i];
                // 使用对应的刀刃点作为面上的点
                plane.point = face_points[i];
                face_map[face_id] = plane;
            }
        }

        // 将map中的面信息转换为vector
        for (const auto& entry : face_map) {
            planes.push_back(entry.second);
        }

    }

    void broaching_AptCutterVolume::calculateForceData() {
        // 清空所有力数据映射
        force_map.clear();

        int blade_id = blade_num;

        // 获取指定blade_id的切削刃数据
        const auto& current_blade = blade_points[blade_id];
        const auto& current_gamma = blade_angles_gamma[blade_id];
        const auto& current_alpha = blade_angles_alpha[blade_id];
        const auto& current_cut_h = blade_cut_h[blade_id];

        // 检查数据是否一致
        if (current_blade.size() != current_gamma.size() ||
            current_blade.size() != current_alpha.size() ||
            current_blade.size() != current_cut_h.size()) {
            std::cerr << "Inconsistent data size for blade_id " << blade_id << std::endl;
            return;
        }

        const int max_valid_index = current_blade.size() - 3;
        if (max_valid_index < 1) { // 至少需要3个点才能计算力
            std::cerr << "Not enough points for blade_id " << blade_id << std::endl;
            return;
        }

        for (const auto& pair : cut_h) {
            int inside_index = pair.first;    // 获取inside_index
            double force_cut_h = pair.second.avg_dmin; // 从结构体中获取平均切削深度
            int force_cut_positionID = pair.second.node_id; // 从结构体中获取最小边距距离对应节点ID

            if (inside_index == 0 || inside_index > max_valid_index) continue; // inside_index=0时，切削厚度存在问题

            GLVertex blade_1 = current_blade[inside_index];
            GLVertex blade_2 = current_blade[inside_index + 1];
            float point_dx = blade_1.x - blade_2.x;
            float point_dy = blade_1.y - blade_2.y;
            float point_dz = blade_1.z - blade_2.z;
            float force_cut_w = std::sqrt(point_dx * point_dx + point_dy * point_dy + point_dz * point_dz);

            ForceData fd;
            fd.force_position_id = force_cut_positionID;
            fd.force_cuth = force_cut_h;

            // 计算切向力 (force_t)
            fd.force_t = GLVertex(dx, dy, dz);
            // 归一化处理
            double length = fd.force_t.norm();
            if (length > 0) {
                fd.force_t = fd.force_t * (1.0 / length);
            }

            // 计算轴向力 (force_f)
            fd.force_f = GLVertex(point_dx, point_dy, point_dz).cross(fd.force_t);
            // 归一化处理
            length = fd.force_f.norm();
            if (length > 0) {
                fd.force_f = fd.force_f * (1.0 / length);
            }

            // 计算径向力 (force_r)
            fd.force_r = GLVertex(0.0, 0.0, 0.0);
            // 归一化处理
            length = fd.force_r.norm();
            if (length > 0) {
                fd.force_r = fd.force_r * (1.0 / length);
            }

            // 确保inside_index在有效范围内
            if (inside_index < static_cast<int>(current_gamma.size()) &&
                inside_index < static_cast<int>(current_alpha.size()) &&
                inside_index < static_cast<int>(current_cut_h.size())) {
                // 计算合力 (force_value)
                //k_fc = 17587 - 29.54 * v_c - 199.7 * current_gamma[inside_index] - 572.45 * current_alpha[inside_index]
                //    - 469307 * current_cut_h[inside_index] + 6411 * current_gamma[inside_index] * current_cut_h[inside_index] + 21542 * current_alpha[inside_index] * current_cut_h[inside_index];
                //k_fcn = 16476 + 65 * v_c - 331.1 * current_gamma[inside_index] - 192.68 * current_alpha[inside_index]
                //    - 445119 * current_cut_h[inside_index] + 8808.7 * current_gamma[inside_index] * current_cut_h[inside_index];
                //K_fc_correct = 0.0024 * current_gamma[inside_index] * current_gamma[inside_index] - 0.068 * current_gamma[inside_index] + 1.5;
                //K_fcn_correct = 0.002 * current_gamma[inside_index] * current_gamma[inside_index] - 0.0572 * current_gamma[inside_index] + 1.439;
                //
                k_fc = force_coefs[0][0] + force_coefs[0][1] * v_c
                    + force_coefs[0][2] * current_gamma[inside_index] + force_coefs[0][3] * current_alpha[inside_index]
                    + force_coefs[0][4] * current_cut_h[inside_index]
                    + force_coefs[0][9] * current_gamma[inside_index] * current_cut_h[inside_index]
                    + force_coefs[0][10] * current_alpha[inside_index] * current_cut_h[inside_index];
                k_fcn = force_coefs[1][0] + force_coefs[1][1] * v_c
                    + force_coefs[1][2] * current_gamma[inside_index] + force_coefs[1][3] * current_alpha[inside_index]
                    + force_coefs[1][4] * current_cut_h[inside_index]
                    + force_coefs[1][9] * current_gamma[inside_index] * current_cut_h[inside_index]
                    + force_coefs[1][10] * current_alpha[inside_index] * current_cut_h[inside_index];
                K_fc_correct = force_coefs[2][2] * current_gamma[inside_index] * current_gamma[inside_index]
                    + force_coefs[2][1] * current_gamma[inside_index] + force_coefs[2][0];
                K_fcn_correct = force_coefs[3][2] * current_gamma[inside_index] * current_gamma[inside_index]
                    + force_coefs[3][1] * current_gamma[inside_index] + force_coefs[3][0];
                fd.k_fc = k_fc * K_fc_correct * force_cut_h * force_cut_w;
                fd.k_fcn = k_fcn * K_fcn_correct * force_cut_h * force_cut_w;

                fd.force_value.x = (fd.k_fc * fd.force_t.x + fd.k_fcn * fd.force_f.x);
                fd.force_value.y = (fd.k_fc * fd.force_t.y + fd.k_fcn * fd.force_f.y);
                fd.force_value.z = (fd.k_fc * fd.force_t.z + fd.k_fcn * fd.force_f.z);
            }

            // 记录位置
            fd.force_position = blade_1;

            // 存入当前切削刃的力数据映射
            force_map[inside_index] = fd;
        }

        // 将当前切削刃的力数据映射存入angle_force_map
        angle_force_map[blade_id] = force_map;

        // 将angle_force_map存入cutnum_angle_force_map
        cutnum_angle_force_map[tool_angle] = angle_force_map;
    }

    void broaching_AptCutterVolume::outputForceData(const std::string & output_dir) {
        // 构造输出文件路径（格式：output_dir/force_data_[tool_angle].txt）
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << tool_angle;  // 保留2位小数避免文件名过长
        std::string file_path = output_dir + "/force_data_" + oss.str() + ".txt";

        // 创建并打开文件（覆盖模式）
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "无法打开输出文件: " << file_path << std::endl;
            return;
        }

        // 查找当前工具角度对应的力数据
        auto angle_it = cutnum_angle_force_map.find(tool_angle);
        if (angle_it != cutnum_angle_force_map.end()) {
            // 遍历当前角度下所有刀片ID对应的力数据
            for (const auto& blade_pair : angle_it->second) {
                const int& blade_id = blade_pair.first;
                // 遍历当前刀片ID下所有力数据
                for (const auto& force_pair : blade_pair.second) {
                    const ForceData& fd = force_pair.second;
                    // 写入：位置x、y、z，力x、y、z，切削厚度，以及各阶数/自由度的振动向量
                    out_file << fd.force_position.x << "\t"
                        << fd.force_position.y << "\t"
                        << fd.force_position.z << "\t"
                        << fd.force_value.x << "\t"
                        << fd.force_value.y << "\t"
                        << fd.force_value.z << "\t"
                        << fd.force_cuth << "\t"
                        << fd.k_fc << "\t"
                        << fd.k_fcn << "\t";

                    // 遍历所有阶数和自由度，输出振动向量数据
                    for (size_t mode = 0; mode < vibration_vectors.size(); ++mode) {
                        for (size_t dof = 0; dof < vibration_vectors[mode].size(); ++dof) {
                            if (fd.force_position_id < vibration_vectors[mode][dof].size()) {
                                out_file << vibration_vectors[mode][dof][fd.force_position_id] << "\t";
                            }
                            else {
                                out_file << "0.0\t";  // 无效索引时填充0
                            }
                        }
                    }
                    out_file << std::endl;
                }
            }
        }

        out_file.close();
    }

    void broaching_AptCutterVolume::calculateTotalForce() {

        temp_total_force = GLVertex(0, 0, 0);

        auto it = cutnum_angle_force_map.find(tool_angle);
        if (it != cutnum_angle_force_map.end()) {
            const auto& angle_force_map = it->second;
            for (const auto& inner_pair : angle_force_map) {
                int  force_cutnum = inner_pair.first;  // 第二层键（float）
                const auto& force_map_2 = inner_pair.second;  // ForceData 数据
                // 遍历force_map计算合力
                for (const auto& force_pair : force_map_2) {
                    const ForceData& fd = force_pair.second;
                    int inside_index = force_pair.first;
                    //qDebug() << "fd.force_value.x:" << fd.force_value.x;
                    temp_total_force.x += fd.force_value.x;
                    temp_total_force.y += fd.force_value.y;
                    temp_total_force.z += fd.force_value.z;
                }

            }
            // 存储每个角度的合力
            angle_total_force[tool_angle] = temp_total_force;
            qDebug() << "temp_total_force.z:" << temp_total_force.z;
        }
        else {
            std::cout << "error:calculateTotalForce()" << std::endl;
            return;
        }
    }
    void broaching_AptCutterVolume::getCurrentTotalForce(double* Fx, double* Fy, double* Fz) {

        auto it = angle_total_force.find(tool_angle);
        if (it != angle_total_force.end()) {
            *Fx = it->second.x;
            *Fy = it->second.y;
            *Fz = it->second.z;
        }
        else {
            std::cout << "error:getCurrentTotalForce()" << std::endl;
            return;
        }
    }
    void broaching_AptCutterVolume::outputTotalForceData(const std::string & output_dir) {
        // 构造输出文件路径（格式：output_dir/total_force_data.txt）
        std::string file_path = output_dir + "/total_force_data.txt";

        // 创建并打开文件（覆盖模式）
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "无法打开输出文件: " << file_path << std::endl;
            return;
        }

        // 遍历angle_total_force的键值对（tool_angle和对应的合力）
        for (const auto& pair : angle_total_force) {
            double tool_angle = pair.first;
            const GLVertex& total_force = pair.second;

            // 写入：tool_angle、x、y、z分量（用制表符分隔）
            out_file << tool_angle << "\t"
                << total_force.x << "\t"
                << total_force.y << "\t"
                << total_force.z << std::endl;
        }

        out_file.close();
        std::cout << "total_force数据已输出至: " << file_path << std::endl;
    }

    void broaching_AptCutterVolume::calculateVibration() {

        auto it = angle_total_force.find(tool_angle);
        if (it != angle_total_force.end()) {
            double force_x = it->second.x;
            double force_y = it->second.y;

            qDebug() << "force_x:" << force_x;
            qDebug() << "force_y:" << force_y;

            ddot_q_x[tool_angle] = (force_x - vibr_c * dot_q_x[tool_angle] - vibr_k * q_x[tool_angle]) / vibr_m * 1e6;
            ddot_q_y[tool_angle] = (force_y - vibr_c * dot_q_y[tool_angle] - vibr_k * q_y[tool_angle]) / vibr_m * 1e6;

            q_x[tool_angle + step] = q_x[tool_angle] + dot_q_x[tool_angle] * dt;
            dot_q_x[tool_angle + step] = dot_q_x[tool_angle] + ddot_q_x[tool_angle] * dt;
            q_y[tool_angle + step] = q_y[tool_angle] + dot_q_y[tool_angle] * dt;
            dot_q_y[tool_angle + step] = dot_q_y[tool_angle] + ddot_q_y[tool_angle] * dt;
        }

    }

    void broaching_AptCutterVolume::updatestockVibrParams() {
        // 修改为基于模态的参数计算
        vibr_k_eff.resize(vibration_values.size());
        vibr_stock_c.resize(vibration_values.size());
        for (size_t mode = 0; mode < vibration_values.size(); ++mode) {
            double lambda = vibration_values[mode]; // 特征值
            double omega = sqrt(lambda);           // 固有频率
            vibr_stock_c[mode] = 2 * stock_vibr_damping_ratio * omega;   // 模态阻尼

            //std::cerr <<"mode:"<<mode<< ";  lambda=" << lambda<<";  "<< std::endl;

            vibr_a0 = 1 / (vibr_beta * dt * dt);
            vibr_a1 = vibr_gamma / (vibr_beta * dt);
            vibr_a2 = 1.0 / (vibr_beta * dt);
            vibr_a3 = 1.0 / (2.0 * vibr_beta) - 1.0;
            vibr_a4 = vibr_gamma / vibr_beta - 1.0;
            vibr_a5 = dt * (vibr_gamma / (2.0 * vibr_beta) - 1.0);
            vibr_a6 = dt * (1.0 - vibr_gamma);
            vibr_a7 = vibr_gamma * dt;
            vibr_k_eff[mode] = lambda;
            //vibr_k_eff[mode] = lambda + vibr_a0 * 1.0 + vibr_a1 * vibr_stock_c[mode]; // 质量矩阵变为1
        }
    }

    void broaching_AptCutterVolume::calculatestockVibration() {
        // 获取自由度数量和模态数量
        const size_t num_dofs = vibration_vectors[0].size();
        const size_t num_modes = vibration_vectors.size();

        // 初始化当前角度下的振动参数
        vibration_q[new_angle].resize(num_modes, 0.0);
        vibration_dq[new_angle].resize(num_modes, 0.0);
        vibration_ddq[new_angle].resize(num_modes, 0.0);

        // 初始化振动参数存储结构[模态]
        current_vibration_q.resize(num_modes);
        next_vibration_q.resize(num_modes);

        // 确保上一时间步力向量大小正确
        if (vibration_f_prev.size() != num_modes) {
            vibration_f_prev.resize(num_modes, 0.0);
        }

        // 临时存储当前时间步每个模态的力
        std::vector<double> f_current(num_modes, 0.0);

        // 遍历所有模态
        for (size_t mode = 0; mode < num_modes; ++mode) {
            double f_total = 0.0;
            double f_eff_total = 0.0;

            // 遍历所有自由度并累加（改为遍历angle_force_map中的所有力数据）
            for (size_t dof = 0; dof < num_dofs; ++dof) {
                // 遍历angle_force_map中的每个blade_num对应的force_map
                for (const auto& [blade_num, inner_force_map] : angle_force_map) {
                    // 遍历当前blade_num对应的所有力数据
                    for (const auto& [id, fd] : inner_force_map) {
                        const double force_component = [&] {
                            switch (dof) {
                            case 0: return fd.force_value.x;
                            case 1: return -fd.force_value.y;
                            case 2: return fd.force_value.z;
                            }
                            }();

                        f_total += force_component * temp_vibration_vectors[mode][dof][fd.force_position_id];
                    }
                }
            }

            // 计算等效力
    //                f_eff_total = f_total + (vibr_a0 * vibration_q[mode] +
    //                                       vibr_a2 * vibration_dq[mode] +
    //                                       vibr_a3 * vibration_ddq[mode])
    //                             + vibr_stock_c[mode] * (vibr_a1 * vibration_q[mode] +
    //                                                   vibr_a4 * vibration_dq[mode] +
    //                                                   vibr_a5 * vibration_ddq[mode]);
            // 计算等效力
            f_eff_total = f_total;

            double relaxation_factor = 0.7; // 松弛因子，0-1之间
            // 更新振动参数（存储各自由度之和）
            current_vibration_q[mode] = next_vibration_q[mode];
            next_vibration_q[mode] = current_vibration_q[mode] * (1 - relaxation_factor) +
                (f_eff_total / vibr_k_eff[mode]) * relaxation_factor;



            vibration_q[new_angle][mode] = next_vibration_q[mode];

            //std::cerr << "Mode:" << mode
            //    << " f_eff=" << f_eff_total
            //    << " f_total=" << f_total
            //    << " vibration_q=" << next_vibration_q[mode]
            //    << std::endl;
        }
        temp_vibration_vectors = vibration_vectors;
    }

    // 新增计算最大最小值函数
    void broaching_AptCutterVolume::calculateStraightness() {
        // 遍历外层映射（角度）
        for (const auto& angle_pair : cut_h_map) {
            double angle = angle_pair.first;  // 角度
            const auto& blade_map = angle_pair.second;  // 刀片ID映射

            // 遍历中层映射（刀片ID）
            for (const auto& blade_pair : blade_map) {
                int blade_id = blade_pair.first;  // 刀片ID
                const auto& point_map = blade_pair.second;  // 点索引映射

                // 遍历内层映射（点索引）
                for (const auto& point_pair : point_map) {
                    int point_index = point_pair.first;  // 点索引
                    double dmin_value = point_pair.second;  // dmin值

                    // 存储到 straightness_map
                    // 注意：这里保持相同的三层结构
                    straightness_map[angle][blade_id][point_index] = dmin_value;
                }
            }
        }
    }

    // 修改后的函数：输出所有inside_index对应的straightness数据文件
    void broaching_AptCutterVolume::outputStraightnessData(const std::string & output_dir) {
        // 收集所有存在的刀具ID和点索引（去重）
        std::set<int> all_blade_ids;
        std::set<int> all_point_indices;

        for (const auto& angle_pair : straightness_map) {
            const auto& blade_map = angle_pair.second;
            for (const auto& blade_pair : blade_map) {
                int blade_id = blade_pair.first;
                all_blade_ids.insert(blade_id);

                const auto& point_map = blade_pair.second;
                for (const auto& point_pair : point_map) {
                    int point_index = point_pair.first;
                    all_point_indices.insert(point_index);
                }
            }
        }

        std::cout << "发现 " << all_blade_ids.size() << " 个刀具ID" << std::endl;
        std::cout << "发现 " << all_point_indices.size() << " 个点索引" << std::endl;

        // 为每个刀具ID和每个点索引组合生成文件
        for (int blade_id : all_blade_ids) {
            for (int point_index : all_point_indices) {
                // 检查这个组合是否有数据
                bool has_data = false;
                for (const auto& angle_pair : straightness_map) {
                    double angle = angle_pair.first;
                    const auto& blade_map = angle_pair.second;

                    auto blade_it = blade_map.find(blade_id);
                    if (blade_it != blade_map.end()) {
                        const auto& point_map = blade_it->second;
                        if (point_map.find(point_index) != point_map.end()) {
                            has_data = true;
                            break;
                        }
                    }
                }

                if (!has_data) {
                    continue;  // 跳过没有数据的组合
                }

                // 构造输出文件路径（格式：output_dir/blade_[blade_id]_point_[point_index].txt）
                std::string file_path = output_dir + "/blade_" + std::to_string(blade_id) +
                    "_point_" + std::to_string(point_index) + ".txt";

                // 创建并打开文件（覆盖模式）
                std::ofstream out_file(file_path);
                if (!out_file.is_open()) {
                    std::cerr << "无法打开输出文件: " << file_path << std::endl;
                    continue;  // 跳过当前组合，继续处理下一个
                }

                // 写入文件头
                out_file << "# 刀具ID: " << blade_id << " 点索引: " << point_index << std::endl;
                out_file << "# 角度\t直线度值" << std::endl;

                // 遍历所有角度，收集该刀具ID和点索引的数据
                for (const auto& angle_pair : straightness_map) {
                    double angle = angle_pair.first;
                    const auto& blade_map = angle_pair.second;

                    auto blade_it = blade_map.find(blade_id);
                    if (blade_it != blade_map.end()) {
                        const auto& point_map = blade_it->second;
                        auto point_it = point_map.find(point_index);
                        if (point_it != point_map.end()) {
                            // 写入：角度和对应的直线度值
                            out_file << angle << "\t" << point_it->second << std::endl;
                        }
                    }
                }

                out_file.close();
                std::cout << "数据已输出至: " << file_path << std::endl;
            }
        }
    }
#pragma endregion
    //************* milling_AptCutterVolume **************/
#pragma region
    milling_AptCutterVolume::milling_AptCutterVolume() {
        type = APT_VOLUME;
        cuttertype = APT;
        radius = 0.0;
        length = 0.0;
        center = GLVertex(0, 0, 0);
        flutelength = 0.0;
        //初始化运动学参数
        q_x[0.0] = 0.0;
        dot_q_x[0.0] = 0.0;
        ddot_q_x[0.0] = 0.0;
        q_y[0.0] = 0.0;
        dot_q_y[0.0] = 0.0;
        ddot_q_y[0.0] = 0.0;
    }

    void milling_AptCutterVolume::calcBB() {
        bb.clear();

    }

    double milling_AptCutterVolume::get_blade_angle_1(double z)const {
        //std::cout << "blade_angle_1: " << tool_angle+tan(a_1)*z/(tan(b_1)+r_1-H_1*tan(b_1));
        return tool_angle + tan(a_1) * (z - z_start) / (tan(b_1) + r_1 - H_1 * tan(b_1));
    }

    double milling_AptCutterVolume::get_blade_angle_2(double z)const {
        return tool_angle + tan(a_1) * H_1 / (tan(b_1) + r_1 - H_1 * tan(b_1)) + tan(a_2) * (z - H_1 - z_start) / (tan(b_2) + r_2 - H_2 * tan(b_2));
    }

    double milling_AptCutterVolume::dist(const GLVertex & p) const {
        GLVertex t = p - center;
        //std::cout << "AptCutterVol
        //std::cout << "p: (" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
        //std::cout << "center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
        //std::cout << "t: (" << t.x << ", " << t.y << ", " << t.z << ")" << std::endl;
        for (const auto& seg : segments) {
            //std::cout << "seg.z_start: " << seg.z_start << "; "<< "seg.z_end: " << seg.z_end << std::endl;
            //std::cout << "p.z: " << p.z;
            if (p.z >= seg.z_start && p.z <= seg.z_end) {
                //std::cout << "seg.type: " << seg.type<< std::endl;
                switch (seg.type) {
                case 0:
                    //std::cout << "dist: " << -seg.radius1 + sqrt(t.x * t.x + t.y * t.y)<< std::endl;
                    return seg.radius1 - sqrt(t.x * t.x + t.y * t.y);
                case 1: {
                    return seg.radius1 - t.norm();
                }
                case 2: {
                    double r = seg.radius1 + (seg.radius2 - seg.radius1) * (p.z - seg.z_start) / (seg.z_end - seg.z_start);
                    //std::cout << "dist: " << -r + sqrt(t.x * t.x + t.y * t.y)<< std::endl;
                    return r - sqrt(t.x * t.x + t.y * t.y);
                }
                }
            }
        }
        // 不在任何分段内，返回负值（外部）
        return t.z;
    }

    Cutting milling_AptCutterVolume::dist_cd(const GLVertex & p) const {
        Cutting result = { 0.0, NO_COLLISION, 1 };
        result.f = dist(p);
        // 可根据需要扩展碰撞类型判定
        return result;
    }

    std::vector<std::vector<std::vector<double>>>
        milling_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            const std::vector<size_t>&modes_to_read) {  // 修改参数为模态索引列表
        if (pre_vibration_vectors.empty()) {
            return {};
        }

        // 检查数据完整性
        size_t total_dofs = pre_vibration_vectors[0].size();
        if (total_dofs % num_dofs != 0) {
            throw std::invalid_argument("总自由度数量不是每个节点自由度数的整数倍");
        }

        // 创建三维向量（根据指定模态数量）
        size_t num_nodes = total_dofs / num_dofs;
        std::vector<std::vector<std::vector<double>>> vibration_vectors(
            modes_to_read.size(),  // 根据指定模态数量创建
            std::vector<std::vector<double>>(num_dofs, std::vector<double>(num_nodes))
        );

        // 转换指定模态数据
        for (size_t i = 0; i < modes_to_read.size(); ++i) {
            size_t mode = modes_to_read[i];
            if (mode >= pre_vibration_vectors.size()) {
                throw std::invalid_argument("请求的模态阶数超过数据范围");
            }

            // 检查模态数据一致性
            if (pre_vibration_vectors[mode].size() != total_dofs) {
                throw std::invalid_argument("不同模态的特征向量长度不一致");
            }

            // 填充数据
            for (int dof = 0; dof < num_dofs; ++dof) {
                for (size_t node = 0; node < num_nodes; ++node) {
                    size_t index = node * num_dofs + dof;
                    vibration_vectors[i][dof][node] = pre_vibration_vectors[mode][index];
                }
            }
        }

        return vibration_vectors;
    }

    // 保留原函数用于向后兼容
    std::vector<std::vector<std::vector<double>>>
        milling_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            int num_modes_to_read) {
        std::vector<size_t> modes;
        for (size_t i = 0; i < static_cast<size_t>(num_modes_to_read); ++i) {
            modes.push_back(i);
        }
        return convertTo3DVibrationVectors(pre_vibration_vectors, num_dofs, modes);
    }


    //读取切削刃点集
    void milling_AptCutterVolume::readTestPointsFromFile(const std::string & filename) {
        original_blade_points.clear();
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            float x, y, z;
            if (iss >> x >> y >> z) {
                original_blade_points.emplace_back(x, y, z);
            }
        }
        file.close();
    }

    int milling_AptCutterVolume::checkConvexity(const GLVertex & a, const GLVertex & b,
        const GLVertex & c, const GLVertex & d) const {
        // 计算三个连续的边向量叉积符号
        GLVertex ab = b - a;
        GLVertex bc = c - b;
        GLVertex cd = d - c;
        GLVertex da = a - d;

        // 计算法线方向的Z分量
        double cross1 = ab.cross(bc).z;
        double cross2 = bc.cross(cd).z;
        double cross3 = cd.cross(da).z;
        double cross4 = da.cross(ab).z;

        if ((cross1 * cross2 >= 0) &&
            (cross2 * cross3 >= 0) &&
            (cross3 * cross4 >= 0)) {
            return(1);
        }
        else if ((cross1 * cross2 >= 0) &&
            (cross2 * cross3 < 0) &&
            (cross3 * cross4 >= 0)) {
            return(2);
        }
        else if ((cross1 * cross2 > 0) &&
            (cross2 * cross3 < 0) &&
            (cross3 * cross4 < 0)) {
            return(3);
        }
        else if ((cross1 * cross2 >= 0) &&
            (cross2 * cross3 >= 0) &&
            (cross3 * cross4 < 0)) {
            return(4);
        }
        else {
            return(5);
        }
    }

    void milling_AptCutterVolume::calculatePosition_balde() {

        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();

        // 1. 首先将所有点（原始点和偏移点）收集到一个临时容器中
        blade_points.clear();  // 清空容器
        blade_points.reserve(original_blade_points.size() * 4); // 预分配空间

        for (auto& point : original_blade_points) {
            GLVertex center_orign = GLVertex(center.x, center.y, point.z + center.z); // a
            GLVertex point_orign = point.rotateABC(0.0, 0.0, blade_angle) + center; // b
            GLVertex point_offset = point.rotateABC(0.0, 0.0, -step + blade_angle) + center - GLVertex(dx, dy, dz); // c
            GLVertex center_offset = center_orign - GLVertex(dx, dy, dz); // d

            // 检查四边形凸性，若为凹则交换b和c的顺序
            int is_convex = checkConvexity(center_orign, point_orign, point_offset, center_offset);
            double alpha = 1.0;
            double beta = 1.0 - alpha;
            if (is_convex == 2) {
                blade_points.push_back(GLVertex(beta * point_orign.x + alpha * center_offset.x, beta * point_orign.y + alpha * center_offset.y, beta * point_orign.z + alpha * center_offset.z));  //d
                blade_points.push_back(point_orign);  //b
                blade_points.push_back(point_offset);  //c
                blade_points.push_back(GLVertex(beta * point_offset.x + alpha * center_orign.x, beta * point_offset.y + alpha * center_orign.y, beta * point_offset.z + alpha * center_orign.z));  //a
            }
            else {
                blade_points.push_back(GLVertex(beta * point_orign.x + alpha * center_orign.x, beta * point_orign.y + alpha * center_orign.y, beta * point_orign.z + alpha * center_orign.z));  //a
                blade_points.push_back(point_orign);  //b
                blade_points.push_back(point_offset);  //c
                blade_points.push_back(GLVertex(beta * point_offset.x + alpha * center_offset.x, beta * point_offset.y + alpha * center_offset.y, beta * point_offset.z + alpha * center_offset.z));  //d
            }
        }

        // 6. 优化包围盒预计算
        blade_bboxes.clear();
        const size_t total_points = blade_points.size();
        const size_t bbox_count = (total_points - 8) / 4;
        blade_bboxes.reserve(bbox_count); // 预分配包围盒空间
        const double expand_factor = 0.1;

        // 7. 处理每个包围盒
        for (size_t i = 0; i <= total_points - 8; i += 4) {
            BoundingBox bbox;

            // 获取8个点的引用，避免重复索引
            const GLVertex& p1 = blade_points[i];
            const GLVertex& p2 = blade_points[i + 1];
            const GLVertex& p3 = blade_points[i + 2];
            const GLVertex& p4 = blade_points[i + 3];
            const GLVertex& p5 = blade_points[i + 4];
            const GLVertex& p6 = blade_points[i + 5];
            const GLVertex& p7 = blade_points[i + 6];
            const GLVertex& p8 = blade_points[i + 7];

            // 手动计算边界框，比std::min/max更高效
            // 计算最小值
            bbox.min.x = p1.x;
            bbox.min.y = p1.y;
            bbox.min.z = p1.z;

            // 最小值
            bbox.min.x = std::min({ bbox.min.x, p2.x, p3.x, p4.x, p5.x, p6.x, p7.x, p8.x });
            bbox.min.y = std::min({ bbox.min.y, p2.y, p3.y, p4.y, p5.y, p6.y, p7.y, p8.y });
            bbox.min.z = std::min({ bbox.min.z, p2.z, p3.z, p4.z, p5.z, p6.z, p7.z, p8.z });

            // 最大值
            bbox.max.x = std::max({ p1.x, p2.x, p3.x, p4.x, p5.x, p6.x, p7.x, p8.x });
            bbox.max.y = std::max({ p1.y, p2.y, p3.y, p4.y, p5.y, p6.y, p7.y, p8.y });
            bbox.max.z = std::max({ p1.z, p2.z, p3.z, p4.z, p5.z, p6.z, p7.z, p8.z });

            // 扩展边界框，允许一定的误差
            const double dx = bbox.max.x - bbox.min.x;
            const double dy = bbox.max.y - bbox.min.y;
            const double dz = bbox.max.z - bbox.min.z;

            const double expand_x = dx * expand_factor;
            const double expand_y = dy * expand_factor;
            const double expand_z = dz * expand_factor;

            bbox.min.x -= expand_x;
            bbox.min.y -= expand_y;
            bbox.min.z -= expand_z;

            bbox.max.x += expand_x;
            bbox.max.y += expand_y;
            bbox.max.z += expand_z;

            blade_bboxes.push_back(bbox);
        }

        stop = std::chrono::system_clock::now();
        qDebug() << "calculatePosition_balde():" << std::chrono::duration<double>(stop - start).count() << "sec.";

    }

    void milling_AptCutterVolume::calculateForceData() {

        //std::chrono::system_clock::time_point start, stop;
        //start = std::chrono::system_clock::now();

        force_map.clear();


        for (const auto& pair : cut_h) {
            int inside_index = pair.first;    // 获取inside_index
            double force_cut_h = pair.second.avg_dmin; // 从结构体中获取平均切削深度
            int force_cut_positionID = pair.second.node_id; // 从结构体中获取最小边距距离对应节点ID


            if (inside_index == 0)continue;//inside_index=0时，切削厚度存在问题


            GLVertex blade_bottom = blade_points[inside_index * 4 + 1];
            GLVertex blade_up = blade_points[inside_index * 4 + 5];
            double dx = blade_up.x - blade_bottom.x;
            double dy = blade_up.y - blade_bottom.y;
            double dz = blade_up.z - blade_bottom.z;
            float force_cut_w = std::abs(std::sqrt(dx * dx + dy * dy + dz * dz));

            //std::cout << "inside_index: " << pair.first << ", force_cut_h: " << pair.second.avg_dmin <<", force_cut_w: " << force_cut_w << std::endl;

            ForceData fd;

            fd.P_min = GLVertex(pair.second.P_min.x, pair.second.P_min.y, pair.second.P_min.z);

            fd.force_position_id = force_cut_positionID;
            fd.force_cuth = force_cut_h;

            // 第一步：计算初始力分量
            double initial_Fr = (force_coefs[3] + force_coefs[0] * force_cut_h) * force_cut_w;
            double initial_Ft = (force_coefs[4] + force_coefs[1] * force_cut_h) * force_cut_w;
            double initial_Fa = (force_coefs[5] + force_coefs[2] * force_cut_h) * force_cut_w;

            // 旋转角度计算
            GLVertex blade_point = blade_points[inside_index * 4 + 1];
            GLVertex blade_center = blade_points[inside_index * 4];

            // 计算径向力 (force_vr)
            GLVertex force_vr = blade_point - blade_center;
            // 归一化处理
            double length = force_vr.norm();
            if (length > 0) {
                force_vr = force_vr * (1.0 / length);
            }

            // 计算轴向力 (force_va)
            GLVertex force_va = GLVertex(0.0, 0.0, -1.0).rotateCBA(angle.x, angle.y, angle.z);
            // 归一化处理
            length = force_va.norm();
            if (length > 0) {
                force_va = force_va * (1.0 / length);
            }

            // 计算切向力 (force_vt)
            GLVertex force_vt = force_va.cross(force_vr);
            // 归一化处理
            length = force_vt.norm();
            if (length > 0) {
                force_vt = force_vt * (1.0 / length);
            }

            // 力分量旋转
            double rotated_Fx = initial_Fr * force_vr.x + initial_Ft * force_vt.x + initial_Fa * force_va.x;
            double rotated_Fy = initial_Fr * force_vr.y + initial_Ft * force_vt.y + initial_Fa * force_va.y;
            double rotated_Fz = initial_Fr * force_vr.z + initial_Ft * force_vt.z + initial_Fa * force_va.z;

            // 第四步：赋值
            fd.force_value = GLVertex(rotated_Fx, rotated_Fy, rotated_Fz);

            // 记录位置
            fd.force_position = blade_bottom;

            // 存入force_map
            force_map[inside_index] = fd;
        }
        angle_force_map[blade_num] = force_map;

        //stop = std::chrono::system_clock::now();
        //qDebug() << "calculateForceData():" << std::chrono::duration<double>(stop - start).count() << "sec.";

    }

    void milling_AptCutterVolume::outputForceData(const std::string & output_dir) {

        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();

        // 构造输出文件路径（格式：output_dir/force_data_[tool_angle].txt）
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << tool_angle;  // 保留2位小数避免文件名过长
        std::string file_path = output_dir + "/force_data_" + oss.str() + ".txt";

        // 创建并打开文件（覆盖模式）
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "无法打开输出文件: " << file_path << std::endl;
            return;
        }

        for (const auto& inner_pair : angle_force_map) {
            int force_cutnum = inner_pair.first;  // 刀刃数
            const auto& force_map_2 = inner_pair.second;  // ForceData 数据

            // 遍历force_map_2，写入每个刀刃点的位置和力数据
            for (const auto& force_pair : force_map_2) {
                int inside_index = force_pair.first;
                const ForceData& fd = force_pair.second;
                // 写入：刀刃数、内部索引、位置x、y、z，力x、y、z，切削厚度，以及各阶数/自由度的振动向量
                out_file << fd.force_position.x << "\t"
                    << fd.force_position.y << "\t"
                    << fd.force_position.z << "\t"
                    << fd.P_min.x << "\t"
                    << fd.P_min.y << "\t"
                    << fd.P_min.z << "\t"
                    << fd.force_value.x << "\t"
                    << fd.force_value.y << "\t"
                    << fd.force_value.z << "\t"
                    << fd.force_cuth << "\t";

                // 遍历所有阶数和自由度，输出振动向量数据
                for (size_t mode = 0; mode < temp_vibration_vectors.size(); ++mode) {
                    for (size_t dof = 0; dof < temp_vibration_vectors[mode].size(); ++dof) {
                        if (fd.force_position_id < temp_vibration_vectors[mode][dof].size()) {
                            out_file << temp_vibration_vectors[mode][dof][fd.force_position_id] << "\t";
                        }
                        else {
                            out_file << "0.0\t";  // 无效索引时填充0
                        }
                    }
                }
                out_file << std::endl;
            }
        }

        out_file.close();
        //std::cout << "力数据已输出至: " << file_path << std::endl;

        stop = std::chrono::system_clock::now();
        qDebug() << "outputForceData():" << std::chrono::duration<double>(stop - start).count() << "sec.";

    }

    void milling_AptCutterVolume::calculateTotalForce() {

        //std::chrono::system_clock::time_point start, stop;
        //start = std::chrono::system_clock::now();

        temp_total_force = GLVertex(0, 0, 0);
        for (const auto& inner_pair : angle_force_map) {
            int  force_cutnum = inner_pair.first;  // 第二层键（float）
            const auto& force_map_2 = inner_pair.second;  // ForceData 数据
            // 遍历force_map计算合力
            for (const auto& force_pair : force_map_2) {
                const ForceData& fd = force_pair.second;
                // collected_force_data.push_back(fd);
                int inside_index = force_pair.first;
                //qDebug() << "fd.force_value.x:" << fd.force_value.x;
                temp_total_force.x += fd.force_value.x;
                temp_total_force.y += fd.force_value.y;
                temp_total_force.z += fd.force_value.z;
            }
        }
        // 存储每个角度的合力
        angle_total_force[tool_angle] = temp_total_force;

        //stop = std::chrono::system_clock::now();
        //qDebug() << "calculateTotalForce():" << std::chrono::duration<double>(stop - start).count() << "sec.";

    }

    void milling_AptCutterVolume::getCurrentTotalForce(double* Fx, double* Fy, double* Fz) {

        auto it = angle_total_force.find(tool_angle);
        if (it != angle_total_force.end()) {
            *Fx = it->second.x;
            *Fy = it->second.y;
            *Fz = it->second.z;
        }
        else {
            std::cout << "error:getCurrentTotalForce()" << std::endl;
            return;
        }
    }

    void milling_AptCutterVolume::outputTotalForceData(const std::string & output_dir) {
        // 构造输出文件路径（格式：output_dir/total_force_data.txt）
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << tool_angle;  // 保留2位小数避免文件名过长
        std::string file_path = output_dir + "/total_force_data_" + oss.str() + ".txt";

        // 创建并打开文件（覆盖模式）
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "无法打开输出文件: " << file_path << std::endl;
            return;
        }

        // 遍历angle_total_force的键值对（tool_angle和对应的合力）
        for (const auto& pair : angle_total_force) {
            double tool_angle = pair.first;
            const GLVertex& total_force = pair.second;

            // 写入：tool_angle、x、y、z分量（用制表符分隔）
            out_file << tool_angle << "\t"
                << total_force.x << "\t"
                << total_force.y << "\t"
                << total_force.z << std::endl;
        }

        out_file.close();
        std::cout << "total_force数据已输出至: " << file_path << std::endl;

        //清除数据
        angle_total_force.clear();
    }

    void milling_AptCutterVolume::calculateVibration() {

        auto it = angle_total_force.find(tool_angle);
        if (it != angle_total_force.end()) {
            double force_x = it->second.x;
            double force_y = it->second.y;

            qDebug() << "force_x:" << force_x;
            qDebug() << "force_y:" << force_y;

            ddot_q_x[tool_angle] = (force_x - vibr_c * dot_q_x[tool_angle] - vibr_k * q_x[tool_angle]) / vibr_m * 1e6;
            ddot_q_y[tool_angle] = (force_y - vibr_c * dot_q_y[tool_angle] - vibr_k * q_y[tool_angle]) / vibr_m * 1e6;

            q_x[tool_angle + step] = q_x[tool_angle] + dot_q_x[tool_angle] * dt;
            dot_q_x[tool_angle + step] = dot_q_x[tool_angle] + ddot_q_x[tool_angle] * dt;
            q_y[tool_angle + step] = q_y[tool_angle] + dot_q_y[tool_angle] * dt;
            dot_q_y[tool_angle + step] = dot_q_y[tool_angle] + ddot_q_y[tool_angle] * dt;
        }

    }

    void milling_AptCutterVolume::updatestockVibrParams() {
        // 检查vibration_values是否为空
        if (vibration_values.empty()) {
            std::cout << "Warning: vibration_values is empty, using default parameters" << std::endl;

            // 使用默认参数初始化
            vibr_k_eff.resize(1);
            vibr_stock_c.resize(1);

            // 设置默认值
            vibr_k_eff[0] = 1.0;
            vibr_stock_c[0] = 2 * stock_vibr_damping_ratio * 1.0;

            // 设置默认的Newmark参数
            vibr_a0 = 1 / (vibr_beta * dt * dt);
            vibr_a1 = vibr_gamma / (vibr_beta * dt);
            vibr_a2 = 1.0 / (vibr_beta * dt);
            vibr_a3 = 1.0 / (2.0 * vibr_beta) - 1.0;
            vibr_a4 = vibr_gamma / vibr_beta - 1.0;
            vibr_a5 = dt * (vibr_gamma / (2.0 * vibr_beta) - 1.0);
            vibr_a6 = dt * (1.0 - vibr_gamma);
            vibr_a7 = vibr_gamma * dt;

            return;
        }

        // 修改为基于模态的参数计算
        vibr_k_eff.resize(vibration_values.size());
        vibr_stock_c.resize(vibration_values.size());
        for (size_t mode = 0; mode < vibration_values.size(); ++mode) {
            double lambda = vibration_values[mode]; // 特征值
            double omega = sqrt(lambda);           // 固有频率
            vibr_stock_c[mode] = 2 * stock_vibr_damping_ratio * omega;   // 模态阻尼

            //std::cerr <<"mode:"<<mode<< ";  lambda=" << lambda<<";  "<< std::endl;

            vibr_a0 = 1 / (vibr_beta * dt * dt);
            vibr_a1 = vibr_gamma / (vibr_beta * dt);
            vibr_a2 = 1.0 / (vibr_beta * dt);
            vibr_a3 = 1.0 / (2.0 * vibr_beta) - 1.0;
            vibr_a4 = vibr_gamma / vibr_beta - 1.0;
            vibr_a5 = dt * (vibr_gamma / (2.0 * vibr_beta) - 1.0);
            vibr_a6 = dt * (1.0 - vibr_gamma);
            vibr_a7 = vibr_gamma * dt;
            vibr_k_eff[mode] = lambda;
            //vibr_k_eff[mode] = lambda + vibr_a0 * 1.0 + vibr_a1 * vibr_stock_c[mode]; // 质量矩阵变为1
        }
    }

    void milling_AptCutterVolume::initstockVibration() {
        // 初始化振动参数存储结构[模态]
        temp_vibration_vectors = vibration_vectors;
        const size_t num_modes = vibration_vectors.size();
        vibration_q.clear();
        vibration_dq.clear();
        vibration_ddq.clear();
        next_vibration_q.clear();
        next_vibration_dq.clear();
        next_vibration_ddq.clear();
        vibration_q.resize(num_modes, 0.0);
        vibration_dq.resize(num_modes, 0.0);
        vibration_ddq.resize(num_modes, 0.0);
        next_vibration_q.resize(num_modes, 0.0);
        next_vibration_dq.resize(num_modes, 0.0);
        next_vibration_ddq.resize(num_modes, 0.0);
    }

    void milling_AptCutterVolume::calculatestockVibration() {
        // 获取自由度数量和模态数量
        const size_t num_dofs = temp_vibration_vectors[0].size();
        const size_t num_modes = temp_vibration_vectors.size();

        // 初始化振动参数存储结构[模态]
        vibration_q.resize(num_modes);
        vibration_dq.resize(num_modes);
        vibration_ddq.resize(num_modes);
        next_vibration_q.resize(num_modes);
        next_vibration_dq.resize(num_modes);
        next_vibration_ddq.resize(num_modes);

        // 遍历所有模态
        for (size_t mode = 0; mode < num_modes; ++mode) {
            double f_total = 0.0;
            double f_eff_total = 0.0;

            // 遍历所有自由度并累加（改为遍历angle_force_map中的所有力数据）
            for (size_t dof = 0; dof < num_dofs; ++dof) {
                // 遍历angle_force_map中的每个blade_num对应的force_map
                for (const auto& [blade_num, inner_force_map] : angle_force_map) {
                    // 遍历当前blade_num对应的所有力数据
                    for (const auto& [id, fd] : inner_force_map) {
                        const double force_component = [&] {
                            switch (dof) {
                            case 0: return fd.force_value.x;
                            case 1: return fd.force_value.y;
                            case 2: return fd.force_value.z;
                            }
                            }();

                        f_total += force_component * temp_vibration_vectors[mode][dof][fd.force_position_id];
                    }
                }
            }

            // 计算等效力
            f_eff_total = f_total + (vibr_a0 * vibration_q[mode] +
                vibr_a2 * vibration_dq[mode] +
                vibr_a3 * vibration_ddq[mode])
                + vibr_stock_c[mode] * (vibr_a1 * vibration_q[mode] +
                    vibr_a4 * vibration_dq[mode] +
                    vibr_a5 * vibration_ddq[mode]);
            // 计算等效力
            f_eff_total = f_total;

            double relaxation_factor = 0.7; // 松弛因子，0-1之间

            // 更新振动参数（存储各自由度之和）
            vibration_q[mode] = next_vibration_q[mode];
            next_vibration_q[mode] = vibration_q[mode] * (1 - relaxation_factor) +
                (f_eff_total / vibr_k_eff[mode]) * relaxation_factor;
            next_vibration_ddq[mode] = vibr_a0 * (next_vibration_q[mode] - vibration_q[mode])
                - vibr_a2 * vibration_dq[mode]
                - vibr_a3 * vibration_ddq[mode];
            next_vibration_dq[mode] = vibration_dq[mode]
                + vibr_a6 * vibration_ddq[mode]
                + vibr_a7 * next_vibration_ddq[mode];

            // 添加边界限制
            next_vibration_q[mode] = std::min(next_vibration_q[mode], deform_color_max);

            std::cerr << "Mode:" << mode
                << " f_eff=" << f_eff_total
                << " vibr_k_eff=" << vibr_k_eff[mode]
                << " vibration_q=" << next_vibration_q[mode]
                << std::endl;
        }
        temp_vibration_vectors = vibration_vectors;//初始化，避免占用内存
    }

    void milling_AptCutterVolume::outputSurfaceData(const std::string & output_dir) {
        // 检查map是否为空
        if (surface_map.empty()) {
            std::cout << "surface_map 为空，无需输出" << std::endl;
            return;
        }

        int success_files = 0;
        int total_points = 0;

        // 遍历map，为每个键创建一个文件
        for (const auto& [key, vertices] : surface_map) {
            // 如果这个键对应的vector为空，跳过
            if (vertices.empty()) {
                continue;
            }

            // 构造输出文件路径
            std::string file_path = output_dir + "/" + std::to_string(key) + ".txt";

            // 打开文件
            std::ofstream out_file(file_path);
            if (!out_file.is_open()) {
                std::cerr << "无法打开文件: " << file_path << std::endl;
                continue;
            }

            // 设置输出精度
            out_file << std::fixed << std::setprecision(15);

            // 写入该键对应的所有顶点
            for (const auto& vertex : vertices) {
                out_file << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
            }

            out_file.close();

            success_files++;
            total_points += vertices.size();

            std::cout << "已输出: " << file_path
                << " (包含 " << vertices.size() << " 个点)" << std::endl;
        }

        std::cout << "输出完成，共 " << success_files << " 个文件，"
            << total_points << " 个顶点" << std::endl;

        surface_map.clear();
    }

    void milling_AptCutterVolume::outputVibrationData(const std::string & output_file) {
        // 生成带时间戳的文件名
        static std::string timestamped_file;
        if (timestamped_file.empty()) {
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            std::tm local_time;
#ifdef _WIN32
            localtime_s(&local_time, &time_t_now);
#else
            localtime_r(&time_t_now, &local_time);
#endif

            std::ostringstream oss;
            oss << "vibration_data_"
                << std::setfill('0') << std::setw(4) << (local_time.tm_year + 1900)
                << std::setfill('0') << std::setw(2) << (local_time.tm_mon + 1)
                << std::setfill('0') << std::setw(2) << local_time.tm_mday
                << "_"
                << std::setfill('0') << std::setw(2) << local_time.tm_hour
                << std::setfill('0') << std::setw(2) << local_time.tm_min
                << std::setfill('0') << std::setw(2) << local_time.tm_sec
                << ".txt";
            timestamped_file = oss.str();
            std::cout << "创建振动数据文件: " << timestamped_file << std::endl;
        }

        // 打开输出文件
        std::ofstream out_file(timestamped_file, std::ios::app); // 使用追加模式
        if (!out_file.is_open()) {
            std::cerr << "无法打开文件: " << timestamped_file << std::endl;
            return;
        }

        // 设置输出精度
        out_file << std::fixed << std::setprecision(15);

        // 写入tool_angle
        out_file << tool_angle;

        // 写入所有next_vibration_q[mode]
        for (size_t mode = 0; mode < next_vibration_q.size(); ++mode) {
            out_file << " " << next_vibration_q[mode];
        }

        // 换行
        out_file << std::endl;

        out_file.close();
    }
#pragma endregion
    //************* digitaltwin_AptCutterVolume **************/
#pragma region
    digitaltwin_AptCutterVolume::digitaltwin_AptCutterVolume() {
        type = APT_VOLUME;
        cuttertype = APT;
        radius = 0.0;
        length = 0.0;
        center = GLVertex(0, 0, 0);
        flutelength = 0.0;
        //初始化运动学参数
        q_x[0.0] = 0.0;
        dot_q_x[0.0] = 0.0;
        ddot_q_x[0.0] = 0.0;
        q_y[0.0] = 0.0;
        dot_q_y[0.0] = 0.0;
        ddot_q_y[0.0] = 0.0;
        calcBB();
    }

    // 绕x轴旋转点
    GLVertex rotate_x(const GLVertex & p, double angle) {
        double cos_theta = cos(angle);
        double sin_theta = sin(angle);
        GLVertex rotated;
        rotated.x = p.x;
        rotated.y = static_cast<GLfloat>(p.y * cos_theta - p.z * sin_theta);
        rotated.z = static_cast<GLfloat>(p.y * sin_theta + p.z * cos_theta);
        return rotated;
    }

    // 绕y轴旋转点
    GLVertex rotate_y(const GLVertex & p, double angle) {
        double cos_theta = cos(angle);
        double sin_theta = sin(angle);
        GLVertex rotated;
        rotated.x = static_cast<GLfloat>(p.x * cos_theta + p.z * sin_theta);
        rotated.y = p.y;
        rotated.z = static_cast<GLfloat>(-p.x * sin_theta + p.z * cos_theta);
        return rotated;
    }

    // 绕z轴旋转点
    GLVertex  rotate_z(const GLVertex & p, double angle) {
        float cos_theta = cos(static_cast<float>(angle));
        float sin_theta = sin(static_cast<float>(angle));
        GLVertex rotated;
        rotated.x = p.x * cos_theta - p.y * sin_theta;
        rotated.y = p.x * sin_theta + p.y * cos_theta;
        rotated.z = p.z;
        return rotated;
    }

    void digitaltwin_AptCutterVolume::calcBB() {
        bb.clear();
        if (segments.empty()) return;

        // 计算旋转后的边界盒
        double max_x = std::numeric_limits<double>::min();
        double max_y = std::numeric_limits<double>::min();
        double max_z = std::numeric_limits<double>::min();
        double min_x = std::numeric_limits<double>::max();
        double min_y = std::numeric_limits<double>::max();
        double min_z = std::numeric_limits<double>::max();

        // 考虑所有刀具段的最大半径和z范围
        double max_radius = 0.0;
        for (const auto& seg : segments) {
            max_radius = std::max(max_radius, std::max(seg.radius1, seg.radius2));
            min_z = std::min(min_z, seg.z_start);
            max_z = std::max(max_z, seg.z_end);
        }

        // 生成边界盒的8个顶点
        std::vector<GLVertex> vertices;
        vertices.emplace_back(max_radius, max_radius, min_z);
        vertices.emplace_back(max_radius, max_radius, max_z);
        vertices.emplace_back(max_radius, -max_radius, min_z);
        vertices.emplace_back(max_radius, -max_radius, max_z);
        vertices.emplace_back(-max_radius, max_radius, min_z);
        vertices.emplace_back(-max_radius, max_radius, max_z);
        vertices.emplace_back(-max_radius, -max_radius, min_z);
        vertices.emplace_back(-max_radius, -max_radius, max_z);

        // 旋转所有顶点并找到极值
        for (const auto& v : vertices) {
            GLVertex rotated = rotate_x(v, angle.x);
            rotated = rotate_y(rotated, angle.y);
            rotated = rotate_z(rotated, angle.z);

            max_x = std::max(max_x, static_cast<double>(rotated.x));
            max_y = std::max(max_y, static_cast<double>(rotated.y));
            max_z = std::max(max_z, static_cast<double>(rotated.z));
            min_x = std::min(min_x, static_cast<double>(rotated.x));
            min_y = std::min(min_y, static_cast<double>(rotated.y));
            min_z = std::min(min_z, static_cast<double>(rotated.z));
        }

        // 添加刀具中心偏移
        max_x += center.x + TOLERANCE;
        max_y += center.y + TOLERANCE;
        max_z += center.z + TOLERANCE;
        min_x += center.x - TOLERANCE;
        min_y += center.y - TOLERANCE;
        min_z += center.z - TOLERANCE;

        // 更新边界盒
        std::cout << "[DEBUG] Bounding Box - max_x: " << max_x << ", max_y: " << max_y << ", max_z: " << max_z << std::endl;
        std::cout << "[DEBUG] Bounding Box - min_x: " << min_x << ", min_y: " << min_y << ", min_z: " << min_z << std::endl;
        GLVertex maxpt(max_x, max_y, max_z);
        GLVertex minpt(min_x, min_y, min_z);
        bb.addPoint(maxpt);
        bb.addPoint(minpt);
    }

    double digitaltwin_AptCutterVolume::dist(const GLVertex & p) const {
        GLVertex t = p - center;
        //std::cout << "AptCutterVol
        //std::cout << "p: (" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
        //std::cout << "center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
        //std::cout << "t: (" << t.x << ", " << t.y << ", " << t.z << ")" << std::endl;
        for (const auto& seg : segments) {
            //std::cout << "seg.z_start: " << seg.z_start << "; "<< "seg.z_end: " << seg.z_end << std::endl;
            //std::cout << "p.z: " << p.z;
            if (p.z >= seg.z_start && p.z <= seg.z_end) {
                //std::cout << "seg.type: " << seg.type<< std::endl;
                switch (seg.type) {
                case 0:
                    //std::cout << "dist: " << -seg.radius1 + sqrt(t.x * t.x + t.y * t.y)<< std::endl;
                    return seg.radius1 - sqrt(t.x * t.x + t.y * t.y);
                case 1: {
                    return seg.radius1 - t.norm();
                }
                case 2: {
                    double r = seg.radius1 + (seg.radius2 - seg.radius1) * (p.z - seg.z_start) / (seg.z_end - seg.z_start);
                    //std::cout << "dist: " << -r + sqrt(t.x * t.x + t.y * t.y)<< std::endl;
                    return r - sqrt(t.x * t.x + t.y * t.y);
                }
                }
            }
        }
        // 不在任何分段内，返回负值（外部）
        return t.z;
    }

    Cutting digitaltwin_AptCutterVolume::dist_cd(const GLVertex & p) const {
        Cutting result = { 0.0, NO_COLLISION, 1 };
        result.f = dist(p);
        // 可根据需要扩展碰撞类型判定
        return result;
    }

    std::vector<std::vector<std::vector<double>>>
        digitaltwin_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            const std::vector<size_t>&modes_to_read) {  // 修改参数为模态索引列表
        if (pre_vibration_vectors.empty()) {
            return {};
        }

        // 检查数据完整性
        size_t total_dofs = pre_vibration_vectors[0].size();
        if (total_dofs % num_dofs != 0) {
            throw std::invalid_argument("总自由度数量不是每个节点自由度数的整数倍");
        }

        // 创建三维向量（根据指定模态数量）
        size_t num_nodes = total_dofs / num_dofs;
        std::vector<std::vector<std::vector<double>>> vibration_vectors(
            modes_to_read.size(),  // 根据指定模态数量创建
            std::vector<std::vector<double>>(num_dofs, std::vector<double>(num_nodes))
        );

        // 转换指定模态数据
        for (size_t i = 0; i < modes_to_read.size(); ++i) {
            size_t mode = modes_to_read[i];
            if (mode >= pre_vibration_vectors.size()) {
                throw std::invalid_argument("请求的模态阶数超过数据范围");
            }

            // 检查模态数据一致性
            if (pre_vibration_vectors[mode].size() != total_dofs) {
                throw std::invalid_argument("不同模态的特征向量长度不一致");
            }

            // 填充数据
            for (int dof = 0; dof < num_dofs; ++dof) {
                for (size_t node = 0; node < num_nodes; ++node) {
                    size_t index = node * num_dofs + dof;
                    vibration_vectors[i][dof][node] = pre_vibration_vectors[mode][index];
                }
            }
        }

        return vibration_vectors;
    }

    // 保留原函数用于向后兼容
    std::vector<std::vector<std::vector<double>>>
        digitaltwin_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            int num_modes_to_read) {
        std::vector<size_t> modes;
        for (size_t i = 0; i < static_cast<size_t>(num_modes_to_read); ++i) {
            modes.push_back(i);
        }
        return convertTo3DVibrationVectors(pre_vibration_vectors, num_dofs, modes);
    }


    //读取切削刃点集
    void digitaltwin_AptCutterVolume::readTestPointsFromFile(const std::string & filename) {
        original_blade_points.clear();
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            float x, y, z;
            if (iss >> x >> y >> z) {
                original_blade_points.emplace_back(x, y, z);
            }
        }
        file.close();
    }

    int digitaltwin_AptCutterVolume::checkConvexity(const GLVertex & a, const GLVertex & b,
        const GLVertex & c, const GLVertex & d) const {
        // 计算三个连续的边向量叉积符号
        GLVertex ab = b - a;
        GLVertex bc = c - b;
        GLVertex cd = d - c;
        GLVertex da = a - d;

        // 计算法线方向的Z分量
        double cross1 = ab.cross(bc).z;
        double cross2 = bc.cross(cd).z;
        double cross3 = cd.cross(da).z;
        double cross4 = da.cross(ab).z;

        if ((cross1 * cross2 >= 0) &&
            (cross2 * cross3 >= 0) &&
            (cross3 * cross4 >= 0)) {
            return(1);
        }
        else if ((cross1 * cross2 >= 0) &&
            (cross2 * cross3 < 0) &&
            (cross3 * cross4 >= 0)) {
            return(2);
        }
        else if ((cross1 * cross2 > 0) &&
            (cross2 * cross3 < 0) &&
            (cross3 * cross4 < 0)) {
            return(3);
        }
        else if ((cross1 * cross2 >= 0) &&
            (cross2 * cross3 >= 0) &&
            (cross3 * cross4 < 0)) {
            return(4);
        }
        else {
            return(5);
        }
    }

    void digitaltwin_AptCutterVolume::calculateTotalForce() {

        if (has_surface == false)
        {
            force_xyz[0] = 0.0;
            force_xyz[1] = 0.0;
            force_xyz[2] = 0.0;
            return;
        }

        // 使用表面中心点和刀具参数计算切削合力方向
        // 假设surfaceCenter已经通过信号传递到当前对象中

        // 获取刀具中心点和旋转角度
        GLVertex cutter_center = this->center;
        GLVertex cutter_angle = this->angle;

        // 计算从刀具中心到表面中心的方向向量
        double dx = surfaceCenter.x - cutter_center.x;
        double dy = surfaceCenter.y - cutter_center.y;
        double dz = surfaceCenter.z - cutter_center.z;

        // 计算距离
        double distance = sqrt(dx * dx + dy * dy + dz * dz);

        if (distance > 0) {
            // 归一化方向向量
            double dir_x = dx / distance;
            double dir_y = dy / distance;
            double dir_z = dz / distance;

            // 考虑刀具旋转角度的影响
            // 绕x轴旋转矩阵
            double cos_ax = cos(cutter_angle.x);
            double sin_ax = sin(cutter_angle.x);
            // 绕y轴旋转矩阵
            double cos_ay = cos(cutter_angle.y);
            double sin_ay = sin(cutter_angle.y);

            // 应用旋转变换到方向向量
            double rotated_y = dir_y * cos_ay - dir_z * sin_ay;
            double rotated_z = dir_y * sin_ay + dir_z * cos_ay;

            double rotated_x = dir_x * cos_ax + rotated_z * sin_ax;
            double final_z = -dir_x * sin_ax + rotated_z * cos_ax;

            // 更新旋转后的方向向量
            dir_x = rotated_x;
            dir_y = rotated_y;
            dir_z = final_z;

            // 重新归一化
            double norm = sqrt(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
            if (norm > 0) {
                dir_x /= norm;
                dir_y /= norm;
                dir_z /= norm;
            }

            // force_data包含铣刀的切向力、径向力和轴向力
            // force_data[0]: 切向力 (tangential force)
            // force_data[1]: 径向力 (radial force)  
            // force_data[2]: 轴向力 (axial force)

            // 计算刀具坐标系下的力向量
            // 在刀具坐标系中：
            // 切向力：垂直于刀具轴线，沿切削方向
            // 径向力：垂直于刀具轴线，指向刀具中心
            // 轴向力：沿刀具轴线方向

            // 首先将力从刀具坐标系转换到世界坐标系
            // 基于表面中心点方向确定力的分解

            // 计算刀具轴线方向（假设z轴为刀具轴线）
            double tool_axis_x = 0.0;
            double tool_axis_y = 0.0;
            double tool_axis_z = 1.0;

            // 应用刀具旋转角度到刀具轴线（使用前面已经定义的变量）

            // 旋转刀具轴线
            double rotated_axis_y = tool_axis_y * cos_ay - tool_axis_z * sin_ay;
            double rotated_axis_z = tool_axis_y * sin_ay + tool_axis_z * cos_ay;
            double rotated_axis_x = tool_axis_x * cos_ax + rotated_axis_z * sin_ax;
            double final_axis_z = -tool_axis_x * sin_ax + rotated_axis_z * cos_ax;

            // 归一化刀具轴线
            double axis_norm = sqrt(rotated_axis_x * rotated_axis_x + rotated_axis_y * rotated_axis_y + final_axis_z * final_axis_z);
            if (axis_norm > 0) {
                rotated_axis_x /= axis_norm;
                rotated_axis_y /= axis_norm;
                final_axis_z /= axis_norm;
            }

            // 计算切向方向（垂直于刀具轴线，指向表面中心点方向在垂直于轴线平面上的投影）
            double projection_dot = dir_x * rotated_axis_x + dir_y * rotated_axis_y + dir_z * final_axis_z;
            double proj_x = dir_x - projection_dot * rotated_axis_x;
            double proj_y = dir_y - projection_dot * rotated_axis_y;
            double proj_z = dir_z - projection_dot * final_axis_z;

            // 归一化切向方向
            double tangential_norm = sqrt(proj_x * proj_x + proj_y * proj_y + proj_z * proj_z);
            if (tangential_norm > 0) {
                proj_x /= tangential_norm;
                proj_y /= tangential_norm;
                proj_z /= tangential_norm;
            }

            // 计算径向方向（垂直于刀具轴线和切向方向）
            double radial_x = rotated_axis_y * proj_z - rotated_axis_z * proj_y;
            double radial_y = rotated_axis_z * proj_x - rotated_axis_x * proj_z;
            double radial_z = rotated_axis_x * proj_y - rotated_axis_y * proj_x;

            // 归一化径向方向
            double radial_norm = sqrt(radial_x * radial_x + radial_y * radial_y + radial_z * radial_z);
            if (radial_norm > 0) {
                radial_x /= radial_norm;
                radial_y /= radial_norm;
                radial_z /= radial_norm;
            }

            // 将切向力、径向力和轴向力分解到世界坐标系
            force_xyz[0] = force_data[0] * proj_x + force_data[1] * radial_x + force_data[2] * rotated_axis_x;  // Fx
            force_xyz[1] = force_data[0] * proj_y + force_data[1] * radial_y + force_data[2] * rotated_axis_y;  // Fy
            force_xyz[2] = force_data[0] * proj_z + force_data[1] * radial_z + force_data[2] * final_axis_z;    // Fz

            // 输出调试信息
            qDebug() << "Force Position: (" << surfaceCenter.x << ", " << surfaceCenter.y << ", " << surfaceCenter.z << ")";
            qDebug() << "Force Direction: (" << dir_x << ", " << dir_y << ", " << dir_z << ")";
            qDebug() << "Total Force: (" << force_xyz[0] << ", " << force_xyz[1] << ", " << force_xyz[2] << ")";
            qDebug() << "Tangential Direction: (" << proj_x << ", " << proj_y << ", " << proj_z << ")";
            qDebug() << "Radial Direction: (" << radial_x << ", " << radial_y << ", " << radial_z << ")";
            qDebug() << "Tool Axis Direction: (" << rotated_axis_x << ", " << rotated_axis_y << ", " << final_axis_z << ")";



        }
        else {
            // 当刀具中心与表面中心重合时，使用默认力方向
            force_xyz[0] = force_data[0];
            force_xyz[1] = force_data[1];
            force_xyz[2] = force_data[2];

            qDebug() << "Warning: Cutter center coincides with surface center, using default force direction";
            qDebug() << "Default Force: (" << force_xyz[0] << ", " << force_xyz[1] << ", " << force_xyz[2] << ")";
        }

    }

    void digitaltwin_AptCutterVolume::updatestockVibrParams() {
        // 检查vibration_values是否为空
        if (vibration_values.empty()) {
            std::cout << "Warning: vibration_values is empty, using default parameters" << std::endl;

            // 使用默认参数初始化
            vibr_k_eff.resize(1);
            vibr_stock_c.resize(1);

            // 设置默认值
            vibr_k_eff[0] = 1.0;
            vibr_stock_c[0] = 2 * stock_vibr_damping_ratio * 1.0;

            // 设置默认的Newmark参数
            vibr_a0 = 1 / (vibr_beta * dt * dt);
            vibr_a1 = vibr_gamma / (vibr_beta * dt);
            vibr_a2 = 1.0 / (vibr_beta * dt);
            vibr_a3 = 1.0 / (2.0 * vibr_beta) - 1.0;
            vibr_a4 = vibr_gamma / vibr_beta - 1.0;
            vibr_a5 = dt * (vibr_gamma / (2.0 * vibr_beta) - 1.0);
            vibr_a6 = dt * (1.0 - vibr_gamma);
            vibr_a7 = vibr_gamma * dt;

            return;
        }

        // 修改为基于模态的参数计算
        vibr_k_eff.resize(vibration_values.size());
        vibr_stock_c.resize(vibration_values.size());
        for (size_t mode = 0; mode < vibration_values.size(); ++mode) {
            double lambda = vibration_values[mode]; // 特征值
            double omega = sqrt(lambda);           // 固有频率
            vibr_stock_c[mode] = 2 * stock_vibr_damping_ratio * omega;   // 模态阻尼

            //std::cerr <<"mode:"<<mode<< ";  lambda=" << lambda<<";  "<< std::endl;

            vibr_a0 = 1 / (vibr_beta * dt * dt);
            vibr_a1 = vibr_gamma / (vibr_beta * dt);
            vibr_a2 = 1.0 / (vibr_beta * dt);
            vibr_a3 = 1.0 / (2.0 * vibr_beta) - 1.0;
            vibr_a4 = vibr_gamma / vibr_beta - 1.0;
            vibr_a5 = dt * (vibr_gamma / (2.0 * vibr_beta) - 1.0);
            vibr_a6 = dt * (1.0 - vibr_gamma);
            vibr_a7 = vibr_gamma * dt;
            vibr_k_eff[mode] = lambda;
            //vibr_k_eff[mode] = lambda + vibr_a0 * 1.0 + vibr_a1 * vibr_stock_c[mode]; // 质量矩阵变为1
        }
    }

    void digitaltwin_AptCutterVolume::initstockVibration() {
        // 初始化振动参数存储结构[模态]
        temp_vibration_vectors = vibration_vectors;
        const size_t num_modes = vibration_vectors.size();
        vibration_q.clear();
        vibration_dq.clear();
        vibration_ddq.clear();
        next_vibration_q.clear();
        next_vibration_dq.clear();
        next_vibration_ddq.clear();
        vibration_q.resize(num_modes, 0.0);
        vibration_dq.resize(num_modes, 0.0);
        vibration_ddq.resize(num_modes, 0.0);
        next_vibration_q.resize(num_modes, 0.0);
        next_vibration_dq.resize(num_modes, 0.0);
        next_vibration_ddq.resize(num_modes, 0.0);
    }

    void digitaltwin_AptCutterVolume::calculatestockVibration() {

        int node_id;
        if (surfaceCenter_id <= 0)
            node_id = -surfaceCenter_id; // 正常节点编号
        else
            node_id = surfaceCenter_id + normalvertices_size - 1; // 悬挂节点编号

        // 获取自由度数量和模态数量
        const size_t num_dofs = temp_vibration_vectors[0].size();
        const size_t num_modes = temp_vibration_vectors.size();

        // 初始化振动参数存储结构[模态]
        vibration_q.resize(num_modes);
        vibration_dq.resize(num_modes);
        vibration_ddq.resize(num_modes);
        next_vibration_q.resize(num_modes);
        next_vibration_dq.resize(num_modes);
        next_vibration_ddq.resize(num_modes);
        // 遍历所有模态
        for (size_t mode = 0; mode < num_modes; ++mode) {
            double f_total = 0.0;
            double f_eff_total = 0.0;

            // 遍历所有自由度并累加（改为遍历angle_force_map中的所有力数据）
            for (size_t dof = 0; dof < num_dofs; ++dof) {
                const double force_component = [&] {
                    switch (dof) {
                    case 0: return force_xyz[0];
                    case 1: return force_xyz[1];
                    case 2: return force_xyz[2];
                    }
                    }();
                double force_contribution = force_component * temp_vibration_vectors[mode][dof][node_id];
                f_total += force_contribution;
            }

            // 计算等效力
            f_eff_total = f_total + (vibr_a0 * vibration_q[mode] +
                vibr_a2 * vibration_dq[mode] +
                vibr_a3 * vibration_ddq[mode])
                + vibr_stock_c[mode] * (vibr_a1 * vibration_q[mode] +
                    vibr_a4 * vibration_dq[mode] +
                    vibr_a5 * vibration_ddq[mode]);
            // 计算等效力
            f_eff_total = f_total;

            double relaxation_factor = 0.7; // 松弛因子，0-1之间

            // 更新振动参数（存储各自由度之和）
            vibration_q[mode] = next_vibration_q[mode];
            next_vibration_q[mode] = vibration_q[mode] * (1 - relaxation_factor) +
                (f_eff_total / vibr_k_eff[mode]) * relaxation_factor;
            next_vibration_ddq[mode] = vibr_a0 * (next_vibration_q[mode] - vibration_q[mode])
                - vibr_a2 * vibration_dq[mode]
                - vibr_a3 * vibration_ddq[mode];
            next_vibration_dq[mode] = vibration_dq[mode]
                + vibr_a6 * vibration_ddq[mode]
                + vibr_a7 * next_vibration_ddq[mode];

            // 添加边界限制
            next_vibration_q[mode] = std::min(next_vibration_q[mode], deform_color_max);

            std::cerr << "Mode:" << mode
                << " f_eff=" << f_eff_total
                << " vibr_k_eff=" << vibr_k_eff[mode]
                << " vibration_q=" << next_vibration_q[mode]
                << std::endl;
        }
        temp_vibration_vectors = vibration_vectors;//初始化，避免占用内存
    }
#pragma endregion

    } // end namespace
    // end of file volume.cpp
