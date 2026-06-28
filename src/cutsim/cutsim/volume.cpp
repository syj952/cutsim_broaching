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
#include <algorithm>
#include <cmath>
#include <limits>

#include <src/cutsim/cutsim_def.hpp>

#include <QFile>
#include <QDir>
#include <QFileInfo>
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
        // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀岄柛娑卞幗缁绢垶鏌ｉ埡鍌氱婵炲牊鍨垮鐢稿焵椤掑倷鐒?闂佸搫鐗冮崑鎾愁熆閸棗鎳庢禒顖炴煥濞戞澧ｉ柍褜鍓欓崯鍐差瀶閼姐倐鍋撻崷顓熷殌婵☆垰锕弫?
        GLVertex minpt = center - GLVertex(lengthX / 2, lengthY / 2, lengthZ / 2) - GLVertex(TOLERANCE, TOLERANCE, TOLERANCE);
        GLVertex maxpt = center + GLVertex(lengthX / 2, lengthY / 2, lengthZ / 2) + GLVertex(TOLERANCE, TOLERANCE, TOLERANCE);

        // 闁诲繐绻愬Λ娑㈠磻閿濆惓搴ｆ嫚閹绘帩娼遍梺鍛婂笚濠㈡鈧灚锕㈠畷鍓佲偓闈涙啞绾?
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
        V21.clear(); V21invV21dotV21.clear();
        V32.clear(); V32invV32dotV32.clear();
        V13.clear(); V13invV13dotV13.clear();

        std::vector<Facet*> valid_facets;
        valid_facets.reserve(facets.size());
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
            if (normal.norm() <= CALC_TOLERANCE) {
                continue;
            }
            normal.normalize();
            //assert ((facets[i]->normal - normal).norm() < CALC_TOLERANCE);
            facets[i]->normal = normal;
            valid_facets.push_back(facets[i]);
        }

        facets.swap(valid_facets);
        if (facets.empty()) {
            bb.clear();
            maxpt = GLVertex(0, 0, 0);
            minpt = GLVertex(0, 0, 0);
            maxlength = 0.0;
            invcubesize = 0.0;
            return;
        }

        maxpt.x = fmax(fmax(facets[0]->v1.x, facets[0]->v2.x), facets[0]->v3.x);
        maxpt.y = fmax(fmax(facets[0]->v1.y, facets[0]->v2.y), facets[0]->v3.y);
        maxpt.z = fmax(fmax(facets[0]->v1.z, facets[0]->v2.z), facets[0]->v3.z);
        minpt.x = fmin(fmin(facets[0]->v1.x, facets[0]->v2.x), facets[0]->v3.x);
        minpt.y = fmin(fmin(facets[0]->v1.y, facets[0]->v2.y), facets[0]->v3.y);
        minpt.z = fmin(fmin(facets[0]->v1.z, facets[0]->v2.z), facets[0]->v3.z);
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
        //闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥庡墰缁犮儵鏌涢弬璇插妞ゆ帞鍠栧畷锝夊磼濞戞瑦顔?
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
        modal_displacement_limit_mm = 0.05;
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
        if (enabled && !machining_residual_events_enabled) {
            machining_contact_events.clear();
        }
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
        // 婵炴垶鎸哥粔鏉戯耿椤忓懐顩烽悹浣哥－缁夊潡鏌涢幒鎴烆棡妞ゆ柨鐭傚畷姗€宕崘顏嗩槷闁哄鏅滈弻銊ッ洪弽顐ｅ闁绘柨鐨濋崑鎾舵兜妞嬪海顦╂繝銏ｅ煐閻楃娀宕曢幘顔芥櫖?
        return t.z;
    }

    Cutting broaching_AptCutterVolume::dist_cd(const GLVertex & p) const {
        Cutting result = { 0.0, NO_COLLISION, 1 };
        result.f = dist(p);
        // 闂佸憡鐟崹閬嶆偋閹绢喖绠叉い鏇楀亾婵炴挸澧庨幉鐗堟媴閻熸壋鎸呴柣蹇曞仦濞叉粓锝為锕€绠婚柣鎰祷椤箓鏌涢妸銉剰闁搞劎鏅埀?
        return result;
    }

    std::vector<std::vector<std::vector<double>>>
        broaching_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            const std::vector<size_t>&modes_to_read) {  // 婵烇絽娴傞崰妤呭极婵傜鐭楅柛灞剧⊕濞堣泛鈽夐幘璺哄妺閼垛晠鏌熼鑳厡闁稿被鍔岄锝夊即閻愯尙浠氶柣?
        if (pre_vibration_vectors.empty()) {
            return {};
        }

        // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕闁哄棛鍠栭獮鎴︻敊閼测晜娈梺杞扮閻°劑鍩€?
        size_t total_dofs = pre_vibration_vectors[0].size();
        if (total_dofs % num_dofs != 0) {
            throw std::invalid_argument("total dofs is not divisible by node dofs");
        }

        // 闂佸憡甯楃粙鎴犵磽閹惧鈻斿璺烘湰濡﹪鏌涘顓炵伌闁革絾妞介弫宥夊醇閻斿搫顥戦梺纭咁嚃閸犳鈧灚姘ㄩ埀顒冾潐绾板秷鍟梺璇″厸閻掞箓寮抽悢鍏肩厒闊洢鍎崇粈?
        size_t num_nodes = total_dofs / num_dofs;
        std::vector<std::vector<std::vector<double>>> vibration_vectors(
            modes_to_read.size(),  // 闂佸搫绉烽～澶婄暤娓氣偓楠炴劙宕惰閺嗙増淇婇妤€澧查柍褜鍏涢悞锕傚汲閻斿吋鐓傞煫鍥ㄦ尭閻忥紕鈧?
            std::vector<std::vector<double>>(num_dofs, std::vector<double>(num_nodes))
        );

        // 闁哄鍎愰崜姘暦閺屻儱绠伴柛銉戝懏姣庡┑鈽嗗灙閸撴繈鍩€椤戣法鍔嶉柡鍡欏枛楠?
        for (size_t i = 0; i < modes_to_read.size(); ++i) {
            size_t mode = modes_to_read[i];
            if (mode >= pre_vibration_vectors.size()) {
                throw std::invalid_argument("requested mode index is out of range");
            }

            // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕閼垛晠鏌熼璺ㄥ妽闁哄棛鍠栭獮鎴︻敊閼姐値浼囬梺鐓庡槻閻°劑鍩€?
            if (pre_vibration_vectors[mode].size() != total_dofs) {
                throw std::invalid_argument("mode vector length is inconsistent");
            }

            // 婵犻潧顦介崑鍕储閺嶎厼鏋侀柣妤€鐗嗙粊?
            for (int dof = 0; dof < num_dofs; ++dof) {
                for (size_t node = 0; node < num_nodes; ++node) {
                    size_t index = node * num_dofs + dof;
                    vibration_vectors[i][dof][node] = -pre_vibration_vectors[mode][index];
                }
            }
        }

        return vibration_vectors;
    }

    // 婵烇絽娲︾换鍕汲閳ь剟鏌涘Ο鐓庢瀻闁搞倝浜跺顐﹀箥椤旇姤娈㈡繛瀛樼矊妤犳悂骞冨鍫濊Е閹肩补鈧櫕鍊柣?
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

    // 闁荤姳绶ょ槐鏇㈡偩婵犳碍鍊烽柣鐔告緲閻撳倻绱掗悙顒€顕滄い鏂跨焸瀹曠兘濡搁妷銉р偓濂告煕閹剧韬柍褜鍓涢崰搴ｂ偓瑙勫▕瀹曞綊宕掑鍕嚱濡ょ姷鍋炴繛濠傤焽閻楀牏鈻斿┑鐘插閻ｉ亶鏌熼懜鍨濠靛倹鐗滈幑鍕攽閸偆鈧?
    double broaching_AptCutterVolume::calculateCutThickness(const GLVertex & current_point, const GLVertex & prev_point1, const GLVertex & prev_point2,
        const GLVertex & velocity_vector) {
        // 闂佸憡甯掑ú銈嗘櫠濞戙垺鐒婚柣鏂垮槻椤斿﹪鏌涘顓炵伌闁?
        GLVertex v = velocity_vector;

        // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柛銉戝喚鏉搁梻渚囧亞閸犲海鈧濞婂畷褰掑磼濠婂嫮鍑藉Δ鐘靛仦婵炲﹤顭囨导瀛樺剭闁告洦鍓涢妴濠囨煕濮橆厼鐏撮柛锝嗘そ閺佸秹宕煎┑鍡欌偓濂告煕閹剧韬柍褜鍓涢崰搴ｂ偓瑙勫▕瀹曘儵骞嬮敃鈧▍銈夋煛閸偄澧查梻濞炬櫊閺?
        GLVertex plane_normal = v;
        plane_normal.normalize();

        // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀堢€广儱瀚鍗炩槈閹垮啩绨婚柛銊ョ箻瀹曟粍绻濋崒姘扁偓鎾煟閵娿儱顏柛銈呴閳绘捇妫冨☉娆忊偓濠氭⒑?
        GLVertex segment_vector = prev_point2 - prev_point1;

        // 闁荤姳绶ょ槐鏇㈡偩閺勫繈浜归柟鎯у暱椤ゅ懘鏌ｉ幇顔藉殌闁糕晛鐬奸惀顏堝箰鎼搭喖鏂€闁荤姍鍥舵闁稿缍侀幆鍐礋椤愶絽鈧姊?
        GLVertex point_to_start = current_point - prev_point1;

        // 闁荤姳绶ょ槐鏇㈡偩閼姐倗妫柟绋垮瘨閸炰粙鏌涘顓炵伌闁革絾妞藉畷鐑藉Ω閵夈儳鈧ジ鏌涢幘绛硅含闁逞屽墰閸犲海鈧濞婂畷褰掑磼濠婂嫮鍑藉Δ鐘靛仦婵炲﹤顭囬悧鍫⑩枖濠电姴瀚悾閬嶆煙閼稿灚绀€濠?
        GLVertex segment_projection = segment_vector - plane_normal * (segment_vector.dot(plane_normal));

        // 闁荤姳绶ょ槐鏇㈡偩閺勫繈浜归柟鎯у暱椤ゅ懘鏌ｉ幇顔藉殌闁糕晛鐬奸惀顏堝箰鎼搭喖鏂€闁荤姍鍥舵闁稿缍侀幆鍐礋椤愶絽鈧姊洪幓鎺旂婵犫偓椤忓牆绀嗛柛銉戝喚鏉搁梻渚囧亞閸犲海鈧濞婂畷褰掑磼濠婂嫮鍑藉Δ鐘靛仦婵炲﹤顭囬悧鍫⑩枖濠电姴瀚悾閬嶆煙閼稿灚绀€濠?
        GLVertex point_projection = point_to_start - plane_normal * (point_to_start.dot(plane_normal));

        // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绠柡鍥╁仜椤ㄦ盯鏌ｉ幇顔藉殌婵犫偓椤忓棛妫柟绋垮瘨閸炰粙鏌熼懜鍨濠靛倹鐗楃粙澶嬬節閸曨剛鏆犻梺鍛婄懃閸婂綊寮抽悞?
        double t = 0.0;
        double segment_projection_length_sq = segment_projection.dot(segment_projection);

        if (segment_projection_length_sq > 1e-12) {
            t = point_projection.dot(segment_projection) / segment_projection_length_sq;
            t = std::max(0.0, std::min(1.0, t)); // 闂傚倸瀚崝鏇㈠春濮娾晠鏌涢敂瑙勬0,1]闂佽偐鍘ч崯顐⒚洪崸妤€绀?
        }

        // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绠柡鍥╁仜椤ㄦ盯鏌?
        GLVertex projected_point = prev_point1 + segment_vector * t;

        // 闁荤姳绶ょ槐鏇㈡偩閺勫繈浜归柟鎯у暱椤ゅ懘鏌ｉ幇顔藉殌闁糕晛鐭傞獮搴ㄥ即閻愬樊娈梺缁樺姇濠€鍗炩枔閹达箑瑙﹂柟杈剧畱濞呫倝鏌涢敂鍝勫闁搞劌绻樺畷婊勭節閸ワ絽浜鹃柣鏂垮槻椤斿﹪鏌涢妸銉モ偓璇裁洪崸妤冨祦闁规儼濮ゅ銊モ槈閹剧鍔熸繛鍫熷灴楠炲酣寮撮悙宸
        GLVertex distance_vector = current_point - projected_point;
        GLVertex projected_distance = distance_vector - plane_normal * (distance_vector.dot(plane_normal));

        // 闁哄鏅滈弻銊ッ洪弽顓炵闁哄洨鍋涢〃娑㈡偣閻戞绠樻い?
        return projected_distance.norm();
    }


    int broaching_AptCutterVolume::readBladeAnglesFromFile(const std::string & filename, int blade_id) {
        // 缂佺虎鍙庨崰鏇犳崲濮濇笓ade_id闂侀潻璐熼崝宥咃耿娓氣偓瀵偊宕奸敐鍛Ш闂佹悶鍎插娆撳船?
        if (blade_id < 0) {
            std::cerr << "Invalid blade_id: " << blade_id << std::endl;
            return 0;
        }

        // 闁荤姴顑呴崯顖炲汲閿濆洠鍋撻崷顓熷殌婵炲懏甯楀鍕槻闁活煈鍓氱粋鎺楀Ψ閵夘喖鏅ｇ紓浣瑰礃濞呮洟寮绘繝鍥ㄥ剭闁告洦鍋勯悗濂告煕閹剧宸ラ柛銊ヮ樀瀵偊鎮ч崼婵堛偊
        while (original_blade_points.size() <= static_cast<size_t>(blade_id)) {
            original_blade_points.emplace_back();
            blade_angles_gamma.emplace_back();
            blade_angles_alpha.emplace_back();
            blade_cut_h.emplace_back();
            blade_rakeface_id.emplace_back();
            blade_rakeface_vertex.emplace_back();
        }

        // 濠电偞鎸搁幊鎰板煘閺嶎兙浜归柟鎯у暱椤ゅ懘鏌涢幒鎴炴悙濠⒀勭洴瀹曟岸宕橀鍛殸闂佽桨鑳舵晶妤€鐣?
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

        // 婵炴垶鎸搁悺銊ヮ渻閸屾壕鍋撳☉娅亪宕戝澶婄闁逞屽墴瀵灚寰勬惔顔兼闂佸憡鐟﹂悧婊冣枔閹达附鍊?
        std::vector<GLVertex> temp_points;
        std::vector<double> temp_gamma;
        std::vector<double> temp_alpha;
        std::vector<int> temp_rakeface_id;
        std::vector<GLVertex> temp_rakeface_vertex;

        QTextStream in(&file);
        while (!in.atEnd()) {
            std::string line = in.readLine().toStdString();
            std::istringstream iss(line);
            std::vector<double> values;  // 婵炴垶鎸搁悺銊ヮ渻閸屾壕鍋撳☉娅亪宕戝鍫涗汗闁规儳鍟块·鍛存偠濞戞鐏辨繛鍫熷灴楠炲秹鍩€椤掑嫬瀚夊璺侯儐濞堝爼鏌?
            double val;

            // 闁荤姴娲╅褑銇愰崶顏備汗闁规儳鍟块·鍛存偠濞戞鐏辨繛鍫熷灴楠炲秹鍩€椤掑嫬瀚夊璺侯儐濞堝爼鏌涙繝鍕付闁糕晛鐦塭ctor婵?
            while (iss >> val) {
                values.push_back(val);
            }

            // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕婵″弶鎮傚畷銉╂晜閼恒儳鐣抽梺鐓庡槻閸熷潡鎯?闂佸憡甯楅〃鍡涘汲閻旂厧绠叉い鏇炴缁€鍕磼娓氬灝鐏╃紒?~5闂?
            if (values.size() >= 12) {
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

        // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柛銉戝喚鏉搁梺鍛娚戦懝鍓р偓瑙勫▕閻涱喚鎹勯搹瑙勬喖闂佺琚崝宥夊汲閻旂厧绠?
        for (size_t i = 0; i < temp_points.size(); ++i) {
            original_blade_points[blade_id].push_back(temp_points[i]);
            blade_angles_gamma[blade_id].push_back(temp_gamma[i]);
            blade_angles_alpha[blade_id].push_back(temp_alpha[i]);
            blade_rakeface_id[blade_id].push_back(temp_rakeface_id[i]);
            blade_rakeface_vertex[blade_id].push_back(temp_rakeface_vertex[i]);

            // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柛銉戝喚鏉搁梺鍛娚戦懝鍓р偓?
            double cut_thickness = 0.0;

            if (blade_id > 0 && original_blade_points.size() > static_cast<size_t>(blade_id - 1) &&
                !original_blade_points[blade_id - 1].empty()) {

                // 闂佸憡甯掑ú銈嗘櫠濞戙垺鐒婚柣鏂垮槻椤斿﹪鏌涘顓炵伌闁?
                GLVertex velocity_vector(dx, dy, dz);

                // 闂備緡鍓欑粔鏉戭啅婵犳艾绀堢€广儱瀚鍗炩槈閹垮啩绨婚柛銊ョ箻瀹曟粍绻濋崒姘扁偓鎾煟閵娿儱顏褍绉瑰鍨緞鐏炲墽銈查梻渚囧弨瀹曠敻宕戦敐鍥ｅ亾?
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
                // 婵犵鈧啿鈧綊鎮樻径鎰強妞ゆ牗绮犻崕鎴濃槈閹绢垰浜炬繛鎴炴惄娴滄粓宕硅箛娑樼濠电姴鍊搁悗鎾煙鐎涙澧褏濮电粙澶愬焵椤掍胶鈻旀い蹇撳閻庡ジ鏌涢幘绛瑰伐闁搞劌顦埢浠嬪焺閸愨晝鐣抽梺杞拌兌婢ф鐣垫笟鈧弫宥囦沪閻撳簶鏋忛梺娲绘娇閸旀垹鍒掗婊勫闁靛牆鎳夐崑?
                cut_thickness = (temp_points.size() > i && temp_gamma.size() > i && temp_alpha.size() > i) ?
                    ((i < temp_gamma.size() && i < temp_alpha.size()) ? 0.0 : 0.0) : 0.0;
            }

            blade_cut_h[blade_id].push_back(cut_thickness);
        }
        return 1;

    }

    void broaching_AptCutterVolume::calculatePosition_balde() {

        int blade_id = blade_num;

        // 1. 婵☆偓绲鹃悧鏇㈠储濞戞矮鐒婇柛鈩冾殘缁夊ジ鏌涢幘宕囥€恖ade_id闂佹眹鍔岀€氼噣宕戦敐澶嬫櫖闁割偅绻傞弬褍鈹戦纰卞剳闁稿缍佸畷顏嗕沪閹存帞鍓ㄧ紓浣割槼瀹曠敻宕戦敐澶嬫櫖濠㈣泛顑嗛弳顏堟⒒閸℃顥滈柛鈺佹湰缁嬪鍩€椤掍胶鈻旀い蹇撳暙椤︽煡鏌￠崘顓熺【妞ゆ梹鍔欏畷鎶藉Ω閵堝牆骞€
        std::vector<GLVertex> all_points;

        // 缂佺虎鍙庨崰鏇犳崲濮濇笓ade_id闂侀潻璐熼崝宥咃耿娓氣偓瀵偊宕奸敐鍛Ш闂佹悶鍎插娆撳船?
        if (blade_id < 0 || blade_id >= static_cast<int>(original_blade_points.size())) {
            return; // 闂佸搫鍟版慨鐢稿疾閵夆晜鍎嶉柛鎺濇懍ade_id闂佹寧绋戦惉鐓幟洪崸妤€绠抽柕澶堝妿缁犳煡鏌?
        }

        // 缂佺虎鍙庨崰鏇犳崲濮濇笓ade_points闂佹眹鍔岀€氼剟濡甸崶鈺€鐒婇煫鍥ㄨ壘閸犳洖顭块崜浣告瀻妞ゆ梹鍔楅惀顏堝礃閼碱剛歇闂佸憡鎸哥涵绶嘺de_id
        if (blade_points.size() <= static_cast<size_t>(blade_id)) {
            blade_points.resize(blade_id + 1);
        }

        // 婵犮垼娉涚€氼噣骞冩繝鍛汗闁规儳鍟块·鍛存煕閹烘垶鎼愬褎鐩畷?
        const auto& original_blade = original_blade_points[blade_id];
        auto& current_blade = blade_points[blade_id];

        current_blade.clear(); // 濠电偞鎸搁幊鎰板煘閺嶎兙浜归柟鎯у暱椤ゅ崑lade闂佹眹鍔岀€氼噣宕?
        current_blade.reserve(original_blade.size());

        for (const auto& point : original_blade) {
            GLVertex offset_blade_point;
            offset_blade_point.x = point.x + center.x; // 缂備線纭搁崹鐗堟叏?x 闂佸憡甯掑Λ婵嬪闯?
            offset_blade_point.y = point.y + center.y; // 缂備線纭搁崹鐗堟叏?y 闂佸憡甯掑Λ婵嬪闯?
            offset_blade_point.z = point.z + center.z; // 缂備線纭搁崹鐗堟叏?z 闂佸憡甯掑Λ婵嬪闯?
            current_blade.push_back(offset_blade_point);
        }

        // 濠电儑缍€椤曆勬叏閻愯翰浜归柟鎯у暱椤ゅ崑lade闂佹眹鍔岀€氼剛鏁锝嗗弿閻庯綆鍓欐禒顖炴煥濞戞澧曢柛鎴節瀹曟繈鎮╂潏鈺冩啰婵炴垶鎼╅崢鑲╃紦妤ｅ啫纾婚煫鍥ㄦ长閳哄懏鏅?
        all_points.reserve(original_blade.size() * 2); // 婵☆偅婢樼€氼剟宕规惔銊︾厐鐎广儱娲犻弫鍕⒒?
        for (const auto& point : current_blade) {
            GLVertex original_point;
            original_point.x = point.x;
            original_point.y = point.y;
            original_point.z = point.z;
            all_points.push_back(original_point);

            // 濠电儑缍€椤曆勬叏閻愬搫纾婚煫鍥ㄦ长閳哄懏鍊?
            GLVertex offset_point;
            offset_point.x = original_point.x - dx;
            offset_point.y = original_point.y - dy;
            offset_point.z = original_point.z - dz;
            all_points.push_back(offset_point);
        }

        // 婵☆偓绲鹃悧鏇㈠储濞戙垹绠ラ柟顖嗗啰鍘掗悷婊呭閹稿憡鏅堕悤绲de闂佸湱顣介崑鎾绘煛閸繍妲洪柛瀣剁秮閹啴宕熼浣风帛闁诲繐绻愮换鎰板箯娴兼潙瀚夐柍褜鍓氬鍕槻婵炲吋顨婂浠嬪炊瑜夐崑?
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

        //闂佺绻嬪ù鍥敊韫囨梻鈻旈柍褜鍓涢埀顒冾潐濮樸劌鈻撻幋鐘冲珰妞ゆ牗纰嶉埢?
        min_coord.x = min_coord.x - 0.1 * (max_coord.x - min_coord.x);
        min_coord.y = min_coord.y - 1 * (max_coord.y - min_coord.y);
        min_coord.z = min_coord.z - 0.1 * (max_coord.z - min_coord.z);

        max_coord.x = max_coord.x + 0.1 * (max_coord.x - min_coord.x);
        max_coord.y = max_coord.y + 1 * (max_coord.y - min_coord.y);
        max_coord.z = max_coord.z + 0.1 * (max_coord.z - min_coord.z);

        // 闂佸搫顑呯€氫即鍩€椤掑倸孝閻庡灚锕㈠畷鍓佲偓闈涙啞绾绢亪鏌?婵炴垶鎼╂禍顏堝Υ婵犲洦鍊?
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


        // 婵炶揪缍€濞夋洟寮ˇ鎻硃闂佸搫顦崕閬嶆偤閵娾晛纾奸柕濠忓濡层劌鈽夐幙鍐ч偗婵炶偐妾篋闁诲海鏁搁幊鎾惰姳閺屻儲鍎嶉柛鏇ㄥ灡濡椼劌菐閸ワ絽澧插ù?
        std::map<int, PlaneInfo> face_map;

        planes.clear();

        const auto& face_ids = blade_rakeface_id[blade_id];
        const auto& face_normals = blade_rakeface_vertex[blade_id];
        const auto& face_points = original_blade_points[blade_id];

        // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鐏炴垝鍖栭梺鎸庣☉閺堫剟寮ぐ鎺撯挅闁糕剝绋掑銊デ庨崶锝呭⒉濞?
        for (size_t i = 0; i < face_ids.size(); ++i) {
            int face_id = face_ids[i];
            if (face_map.find(face_id) == face_map.end()) {
                // 闂佺懓鐏氶崕鎶藉春鐏炲墽鈻旈柍褜鍓氱粙澶愵敂閸涱喚鍘愰梺姹囧妼鐎氭澘顭囨导瀛樻櫖閻忕偠妫勯悘锛勨偓鐐瑰€栧钘夘焽閻楀牏鈹嶉柍鈺佸暕缁?
                PlaneInfo plane;
                plane.normal = face_normals[i];
                // 婵炶揪缍€濞夋洟寮妶鍥ｅ亾閻㈠灚鍤€缂併劍鐓￠幆鍐礋椤愩垻鈧剟鏌涢幒鎴濇殶闁稿绲鹃幏鍛煥閳ь剛鎷归悢鍏碱棃妞ゎ偒鍘鹃悷鎰版煟閵娿儱顏柛?
                plane.point = face_points[i];
                face_map[face_id] = plane;
            }
        }

        // 闁诲繐绻愰弨鐬恜婵炴垶鎼╅崢鎯р枔閹达附顥堟い顐幒缁诲棝鏌熼褍鐏ユ繛鏉戞楠炴垿锝為锛勵槹vector
        for (const auto& entry : face_map) {
            planes.push_back(entry.second);
        }

    }

    void broaching_AptCutterVolume::calculateForceData() {
        // 濠电偞鎸搁幊鎰板煘閺嶎厼绠ラ柍褜鍓熷鍨緞婵犲偆娼濋梺杞拌兌婢ф鐣垫笟鈧浼存偐閼碱剚顔?
        force_map.clear();

        int blade_id = blade_num;

        // 闂佸吋鍎抽崲鑼躲亹閸ヮ剙绠伴柛銉戝懏姣巄lade_id闂佹眹鍔岀€氼剟宕硅箛娑樼濠电姴鍊搁悗鎾煛娴ｅ搫顣肩€?
        const auto& current_blade = blade_points[blade_id];
        const auto& current_gamma = blade_angles_gamma[blade_id];
        const auto& current_alpha = blade_angles_alpha[blade_id];
        const auto& current_cut_h = blade_cut_h[blade_id];

        // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕闁哄棛鍠栭獮鎴︻敊閼恒儛锕傛煕濮樺墽绱扮紒鏃€鎸抽幊?
        if (current_blade.size() != current_gamma.size() ||
            current_blade.size() != current_alpha.size() ||
            current_blade.size() != current_cut_h.size()) {
            std::cerr << "Inconsistent data size for blade_id " << blade_id << std::endl;
            return;
        }

        const int max_valid_index = current_blade.size() - 3;
        if (max_valid_index < 1) { // 闂佺厧鍢查崯鍧楁儍椤栫偞顥嗛柍褜鍓涢幉?婵炴垶鎼╂禍鐐哄磻閿濆绠ョ€广儱鐗嗛崢鎾偣娓氬﹦纾块柣锝咁煼瀹?
            std::cerr << "Not enough points for blade_id " << blade_id << std::endl;
            return;
        }

        for (const auto& pair : cut_h) {
            int inside_index = pair.first;    // 闂佸吋鍎抽崲鑼躲亹閸ｅ埖side_index
            double force_cut_h = pair.second.avg_dmin; // 婵炲濮村ù椋庡垝閵娾晛鍑犻柛鏇ㄤ簽缁夌厧鈽夐幙鍐ㄥ绩妤犵偛绻樺畷锝夊冀瑜旈幐顒勬煕瑜嶅ú銈夊垂韫囨稑绀堝┑鐘插暟缁犳帡骞?
            int force_cut_positionID = pair.second.node_id; // 婵炲濮村ù椋庡垝閵娾晛鍑犻柛鏇ㄤ簽缁夌厧鈽夐幙鍐ㄥ绩妤犵偛绻樺畷锝夊冀閵婏缚绮柣蹇撶箰缁绘绮╅悢鐑樺磯婵犻潧锕﹂悰鈺冪磼閸屾繍鍤欐い鏇ㄥ枟閹棃寮崶顬繈鏌ｉ幇顕呭劋D

            if (inside_index == 0 || inside_index > max_valid_index) continue; // inside_index=0闂佸搫鍟抽鎰濠靛绀嗛柛銉戝喚鏉搁梺鍛娚戦懝鍓р偓鐟扮－閳ь剚绋掗敋婵犫偓椤忓牊鈷掓い鏇楀亾妞?

            GLVertex blade_1 = current_blade[inside_index];
            GLVertex blade_2 = current_blade[inside_index + 1];
            float point_dx = blade_1.x - blade_2.x;
            float point_dy = blade_1.y - blade_2.y;
            float point_dz = blade_1.z - blade_2.z;
            float force_cut_w = std::sqrt(point_dx * point_dx + point_dy * point_dy + point_dz * point_dz);

            ForceData fd;
            fd.force_position_id = force_cut_positionID;
            fd.force_cuth = force_cut_h;

            // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柛銉戝嫬鈧鏌?(force_t)
            fd.force_t = GLVertex(dx, dy, dz);
            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓本袩闂?
            double length = fd.force_t.norm();
            if (length > 0) {
                fd.force_t = fd.force_t * (1.0 / length);
            }

            // 闁荤姳绶ょ槐鏇㈡偩缂佹ɑ濮滈柡澶嬪灦閸婂鏌?(force_f)
            fd.force_f = GLVertex(point_dx, point_dy, point_dz).cross(fd.force_t);
            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓本袩闂?
            length = fd.force_f.norm();
            if (length > 0) {
                fd.force_f = fd.force_f * (1.0 / length);
            }

            // 闁荤姳绶ょ槐鏇㈡偩鐠囧樊鍤楅柛鏇ㄥ亝閸婂鏌?(force_r)
            fd.force_r = GLVertex(0.0, 0.0, 0.0);
            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓本袩闂?
            length = fd.force_r.norm();
            if (length > 0) {
                fd.force_r = fd.force_r * (1.0 / length);
            }

            // 缂佺虎鍙庨崰鏇犳崲濮濓箯side_index闂侀潻璐熼崝宥咃耿娓氣偓瀵偊宕奸敐鍛Ш闂佹悶鍎插娆撳船?
            if (inside_index < static_cast<int>(current_gamma.size()) &&
                inside_index < static_cast<int>(current_alpha.size()) &&
                inside_index < static_cast<int>(current_cut_h.size())) {
                // 闁荤姳绶ょ槐鏇㈡偩婵犳艾瑙﹂柛顐ｇ箓椤?(force_value)
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

            // 闁荤姳鐒﹀妯肩礊瀹ュ棙濯寸€广儱娲ㄩ弸?
            fd.force_position = blade_1;

            // 闁诲孩绋掗敋闁告瑥妫滈妵鎰板箻閸愬樊鏋€闂佸憡甯掑ú銈嗘櫠濞戙垹绀嗛柛鎰典簼閻ｉ亶鏌涢弮鈧粙鎺楀汲閻旂厧绠叉い鏃囧Г琛奸柣?
            force_map[inside_index] = fd;
        }

        // 闁诲繐绻愬Λ妤冪礊鐎ｎ喖绀堢€广儱鎳庨悗濂告煕閹剧宸ラ柛銊ヮ樀閹啴宕熼銏╂綕闂佽桨鑳舵晶妤€鐣垫笟鈧浼存偐閼碱剚顔忛柣搴㈢⊕閿氶柛娆忔懗ngle_force_map
        angle_force_map[blade_id] = force_map;

        // 闁诲繐绻愰幗纭乬le_force_map闁诲孩绋掗敋闁告瑥鎽秛tnum_angle_force_map
        cutnum_angle_force_map[tool_angle] = angle_force_map;
    }

    void broaching_AptCutterVolume::outputForceData(const std::string & output_dir) {
        // 闂佸搫顑呯€氫即鍩€椤掑倸鞋缂侀鍙冨畷娆撴倻濡崵鈧喖霉閻樺啿鍔堕柣顓熷劤椤曘儵宕熼崜浣侯槱闂佸搫绉堕崢褏妲愰敓鐘虫櫖婵繄娈焧put_dir/force_data_[tool_angle].txt闂?
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << tool_angle;  // 婵烇絽娲︾换鍕汲閳?婵炶揪绲界粔鎾儍椤掑嫬鏋佸ù鑲╃節缂傚鏌涜箛鎾缎ｉ柡瀣暞缁傛帞鎹勯悜妯衡偓鎶藉级閳轰焦鍠橀柡?
        std::string file_path = output_dir + "/force_data_" + oss.str() + ".txt";
        std::string debug_file_path = output_dir + "/force_direction_debug_" + oss.str() + ".txt";

        // 闂佸憡甯楃粙鎴犵磽閹捐崵宓侀柤鎼佹涧閳數鈧鍠掗崑鎾绘煛閸屾碍鐭楁繛鍡愬灲閺佸秹宕奸敐搴㈣埞闂佺儵鏅滈悧妤勫暞閻庢鍠栨蹇曟?
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "闂佸搫鍟版慨鐢垫兜閸洖绠ラ柟鎯х－绾惧寮堕崼鐔稿碍闁搞値鍙冨顒勫炊閿旂瓔鍋? " << file_path << std::endl;
            return;
        }

        // 闂佸搫琚崕鍙夌珶濡￥浜归柟鎯у暱椤ゅ懐鈧鎮堕崕閬嶅矗鐠恒劍鍠嗛柟鐑樺灥椤斿﹪鎮楅悽鍨殌缂併劍鐓￠幆鍐礋椤愩埄娼濋梺杞拌兌婢ф鐣?
        out_file << "tool_angle\tblade_id\tpoint_index\tforce_position_id"
            << "\tx\ty\tz"
            << "\tfx\tfy\tfz\tforce_magnitude"
            << "\tforce_cuth\tk_fc\tk_fcn"
            << "\tforce_t_x\tforce_t_y\tforce_t_z"
            << "\tforce_f_x\tforce_f_y\tforce_f_z";
        for (size_t mode = 0; mode < vibration_vectors.size(); ++mode) {
            for (size_t dof = 0; dof < vibration_vectors[mode].size(); ++dof) {
                out_file << "\tmode_" << mode << "_dof_" << dof;
            }
        }
        out_file << std::endl;

        auto angle_it = cutnum_angle_force_map.find(tool_angle);
        if (angle_it != cutnum_angle_force_map.end()) {
            // 闂備緡鍓欑粔鏉戭啅閺勫繈浜归柟鎯у暱椤ゅ懘鎮峰▎鎰瑨閻庣娅曠粙澶屸偓锝庡亜椤ｆ煡鏌￠崼婵愭Ц闁搞劋绶氶幃褔宕查幙鐘绘倵閻㈠灚鍤€缂併劍鐓￠幆鍐礋椤愩埄娼濋梺杞拌兌婢ф鐣?
            for (const auto& blade_pair : angle_it->second) {
                const int& blade_id = blade_pair.first;
                // 闂備緡鍓欑粔鏉戭啅閺勫繈浜归柟鎯у暱椤ゅ懘鏌涢幒鍡椾壕闂佺粯顨呭Σ妤ф繛鎴炴尭椤戝棙鏅跺澶婂珘濠㈣泛锕ら～鏃堟煛娴ｅ搫顣肩€?
                for (const auto& force_pair : blade_pair.second) {
                    const ForceData& fd = force_pair.second;
                    const int point_index = force_pair.first;
                    const double force_magnitude = fd.force_value.norm();
                    // 闂佸憡鍔栭悷銉╁矗閸℃稒鏅慨姗嗗亞缁夊绱撻崘顏呮珴闂侀潧妫旂花婊堟煏閸℃洜鐨鹃梺鎸庣☉閼活垱鎱ㄥ婊堟煏閸℃洜鐨介梺闈涙缁ㄦ繈鏌ㄥ☉妯垮闁搞劌绻樺畷婊勭節閸屾俺鈷堥柟鑹版彧鐠侊絿妲愬┑鍥╊浄闁靛鍎遍幐銈夋煕濮橆剙顏俊顖欑窔瀵?闂佺厧顨庢禍鐐哄极鏉堛劍鍎熼柨鏃傚亾閻ｉ亶鏌熼崜鎻掔仩濠殿喒鏅犲畷銉╁箣閿曗偓濞?
                    out_file << tool_angle << "\t"
                        << blade_id << "\t"
                        << point_index << "\t"
                        << fd.force_position_id << "\t"
                        << fd.force_position.x << "\t"
                        << fd.force_position.y << "\t"
                        << fd.force_position.z << "\t"
                        << fd.force_value.x << "\t"
                        << fd.force_value.y << "\t"
                        << fd.force_value.z << "\t"
                        << force_magnitude << "\t"
                        << fd.force_cuth << "\t"
                        << fd.k_fc << "\t"
                        << fd.k_fcn << "\t"
                        << fd.force_t.x << "\t"
                        << fd.force_t.y << "\t"
                        << fd.force_t.z << "\t"
                        << fd.force_f.x << "\t"
                        << fd.force_f.y << "\t"
                        << fd.force_f.z << "\t";

                    // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞閹邦厸鏋嗛梺杞扮劍濠㈡﹢骞忔导瀛樺殜妞ゅ繐妫欓弳鐘诲箹鏉堟崘顓虹紒杈ㄧ箖濞煎繘骞橀崘鍙夌様闂佺懓澹婇崹鐗堟叏閳哄懎瑙﹂柟杈剧畱濞呫倝鏌℃担鍝勵暭鐎?
                    for (size_t mode = 0; mode < vibration_vectors.size(); ++mode) {
                        for (size_t dof = 0; dof < vibration_vectors[mode].size(); ++dof) {
                            if (fd.force_position_id >= 0 &&
                                static_cast<size_t>(fd.force_position_id) < vibration_vectors[mode][dof].size()) {
                                out_file << vibration_vectors[mode][dof][fd.force_position_id] << "\t";
                            }
                            else {
                                out_file << "0.0\t";  // 闂佸搫鍟版慨鐢稿疾閵壯勵潟闁靛繒濮风粚鍧楁煛閸愵厽纭鹃柨婵堝仱瀹?
                            }
                        }
                    }
                    out_file << std::endl;
                }
            }
        }

        out_file.close();
    }

    void broaching_AptCutterVolume::outputDeformedBladePointsData(const std::string& output_dir) {
        if (real_blade_points_map.empty()) {
            return;
        }

        const double angle_tolerance = 1e-6;
        auto angle_it = real_blade_points_map.lower_bound(new_angle - angle_tolerance);
        if (angle_it == real_blade_points_map.end() ||
            std::abs(angle_it->first - new_angle) > angle_tolerance) {
            qDebug() << "No deformed blade points for new_angle:" << new_angle;
            return;
        }

        QDir dir;
        if (!dir.mkpath(QString::fromStdString(output_dir))) {
            std::cerr << "Failed to create output directory: " << output_dir << std::endl;
            return;
        }

        std::ostringstream tool_angle_stream;
        tool_angle_stream << std::fixed << std::setprecision(3) << tool_angle;
        std::ostringstream map_angle_stream;
        map_angle_stream << std::fixed << std::setprecision(3) << angle_it->first;

        for (const auto& blade_pair : angle_it->second) {
            const int blade_id = blade_pair.first;
            std::ostringstream file_path_stream;
            file_path_stream << output_dir
                << "/deformed_blade_points_tool_"
                << tool_angle_stream.str()
                << "_map_"
                << map_angle_stream.str()
                << "_blade_"
                << blade_id
                << ".txt";

            std::ofstream out_file(file_path_stream.str());
            if (!out_file.is_open()) {
                std::cerr << "Failed to open deformed blade points file: "
                    << file_path_stream.str() << std::endl;
                continue;
            }

            out_file << "tool_angle\tmap_angle\tblade_id\tpoint_index\tsample_index"
                << "\toriginal_x\toriginal_y\toriginal_z"
                << "\tx\ty\tz"
                << "\tdx\tdy\tdz\tdxz" << std::endl;
            out_file << std::fixed << std::setprecision(9);

            const auto& point_groups = blade_pair.second;
            for (size_t point_index = 0; point_index < point_groups.size(); ++point_index) {
                const auto& samples = point_groups[point_index];
                for (size_t sample_index = 0; sample_index < samples.size(); ++sample_index) {
                    const GLVertex& point = samples[sample_index];
                    GLVertex original(0.0, 0.0, 0.0);
                    const bool has_original =
                        blade_id >= 0 &&
                        static_cast<size_t>(blade_id) < blade_points.size() &&
                        point_index < blade_points[blade_id].size();
                    if (has_original) {
                        original = blade_points[blade_id][point_index];
                    }

                    const double deform_x = point.x - original.x;
                    const double deform_y = point.y - original.y;
                    const double deform_z = point.z - original.z;
                    const double deform_xz = std::hypot(deform_x, deform_z);

                    out_file << tool_angle << "\t"
                        << angle_it->first << "\t"
                        << blade_id << "\t"
                        << point_index << "\t"
                        << sample_index << "\t"
                        << original.x << "\t"
                        << original.y << "\t"
                        << original.z << "\t"
                        << point.x << "\t"
                        << point.y << "\t"
                        << point.z << "\t"
                        << deform_x << "\t"
                        << deform_y << "\t"
                        << deform_z << "\t"
                        << deform_xz << std::endl;
                }
            }
        }
    }

    void broaching_AptCutterVolume::calculateTotalForce() {

        temp_total_force = GLVertex(0, 0, 0);

        auto it = cutnum_angle_force_map.find(tool_angle);
        if (it != cutnum_angle_force_map.end()) {
            const auto& angle_force_map = it->second;
            for (const auto& inner_pair : angle_force_map) {
                int  force_cutnum = inner_pair.first;  // 缂備焦顨忛崗娑氳姳閳哄啩娌柛宀€鍋為弳娑㈡煥濞戞ɑ缍抣oat闂?
                const auto& force_map_2 = inner_pair.second;  // ForceData 闂佽桨鑳舵晶妤€鐣?
                // 闂備緡鍓欑粔鏉戭啅缁尦rce_map闁荤姳绶ょ槐鏇㈡偩婵犳艾瑙﹂柛顐ｇ箓椤?
                for (const auto& force_pair : force_map_2) {
                    const ForceData& fd = force_pair.second;
                    int inside_index = force_pair.first;
                    //qDebug() << "fd.force_value.x:" << fd.force_value.x;
                    temp_total_force.x += fd.force_value.x;
                    temp_total_force.y += fd.force_value.y;
                    temp_total_force.z += fd.force_value.z;
                }

            }
            // 闁诲孩绋掗敋闁稿绉磋闊洤閰ｉ崵瀣偡濞嗘劕绗掗悗瑙勫▕閹啴宕熼锝呪偓銈夋煕?
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
        // 闂佸搫顑呯€氫即鍩€椤掑倸鞋缂侀鍙冨畷娆撴倻濡崵鈧喖霉閻樺啿鍔堕柣顓熷劤椤曘儵宕熼崜浣侯槱闂佸搫绉堕崢褏妲愰敓鐘虫櫖婵繄娈焧put_dir/total_force_data.txt闂?
        std::string file_path = output_dir + "/total_force_data.txt";

        // 闂佸憡甯楃粙鎴犵磽閹捐崵宓侀柤鎼佹涧閳數鈧鍠掗崑鎾绘煛閸屾碍鐭楁繛鍡愬灲閺佸秹宕奸敐搴㈣埞闂佺儵鏅滈悧妤勫暞閻庢鍠栨蹇曟?
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "闂佸搫鍟版慨鐢垫兜閸洖绠ラ柟鎯х－绾惧寮堕崼鐔稿碍闁搞値鍙冨顒勫炊閿旂瓔鍋? " << file_path << std::endl;
            return;
        }

        // 闂備緡鍓欑粔鏉戭啅缁櫞gle_total_force闂佹眹鍔岀€氫即寮銏犵９闁绘挸瀵掗崵鐘绘煥濞戞ɑ绶無ol_angle闂佸憡绮岄懟顖烆敋椤旇姤鍎熼柡鍐ㄥ€归悾閬嶆煕濮橆剛澧曞┑顔肩箻閺?
        for (const auto& pair : angle_total_force) {
            double tool_angle = pair.first;
            const GLVertex& total_force = pair.second;

            // 闂佸憡鍔栭悷銉╁矗閸℃稒鏅慨婵堫殞ol_angle闂侀潧妫旂花娑㈡煏閸℃洜鐨介梺闈涙缁ㄦ繈鏌涢幒鎴烆棦闁革絾妞介弫宥夊醇閵忊剝娈㈤梺鍛婂笚閸庡ジ濡撮崘顏嗙當闁挎洍鍋撻柛銊ラ叄濮婇箖寮幘鍓侇槴
            out_file << tool_angle << "\t"
                << total_force.x << "\t"
                << total_force.y << "\t"
                << total_force.z << std::endl;
        }

        out_file.close();
        std::cout << "total_force闂佽桨鑳舵晶妤€鐣垫担鍓插晠闁肩厧澧庣紙濠氭煕閹达妇绱伴柛? " << file_path << std::endl;
        angle_total_force.clear();
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

        // 婵烇絽娴傞崰妤呭极閸忚偐鈻旈柛婵嗗閸炪劌霉濠婂啫顒㈤懚鈺呮煙椤戣儻鍏屾繛鍫熷灴瀹曪綁宕掑☉娆愵啀闁荤姳绶ょ槐鏇㈡偩?

        temp_vibration_vectors = vibration_vectors;
        vibr_k_eff.resize(vibration_values.size());
        vibr_stock_c.resize(vibration_values.size());
        vibration_mode_max_abs.assign(vibration_vectors.size(), 0.0);
        for (size_t mode_index = 0; mode_index < vibration_vectors.size(); ++mode_index) {
            double max_abs = 0.0;
            for (const auto& dof_values : vibration_vectors[mode_index]) {
                for (double value : dof_values) {
                    const double abs_value = std::abs(value);
                    if (abs_value > max_abs) {
                        max_abs = abs_value;
                    }
                }
            }
            vibration_mode_max_abs[mode_index] = max_abs;
        }
        for (size_t mode = 0; mode < vibration_values.size(); ++mode) {
            double lambda = vibration_values[mode]; // 闂佺粯顨堥幊鎾舵濞戙垹纾?
            double omega = sqrt(lambda);           // 闂佹悶鍎抽崕銈咃耿娴ｇ櫢绱ｉ柟瀵稿Т閼?
            vibr_stock_c[mode] = 2 * stock_vibr_damping_ratio * omega;   // 濠碘槅鍨崜婵嬪焵椤戣法绐旀俊顖氭娴?

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
            //vibr_k_eff[mode] = lambda + vibr_a0 * 1.0 + vibr_a1 * vibr_stock_c[mode]; // 闁荤姵鍔戦崝鎴﹀闯濞差亝鍎楅柍鍝勬噺閳诲牓鏌涘▎鎯疯偐鎷?
        }
    }

    void broaching_AptCutterVolume::calculatestockVibration() {
        // 闂佸吋鍎抽崲鑼躲亹閸ヮ剚鍤婃い蹇撴閺嗙娀骞栨潏楣冩闁哄棛鍠栭弻宀冪疀閹炬潙顏┑鈽嗗灙閸撴繈鍩€椤戣法鍔嶉柡鍡欏枛閺?
        if (temp_vibration_vectors.empty()) {
            temp_vibration_vectors = vibration_vectors;
        }
        if (temp_vibration_vectors.empty() || temp_vibration_vectors[0].empty()) {
            return;
        }
        const size_t num_dofs = temp_vibration_vectors[0].size();
        const size_t num_modes = temp_vibration_vectors.size();

        // 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宓懐歇闂佸憡鎸哥粔鐑斤綖濡や焦鍎熼柨鏃囨硶閻熸捇鏌ｉ妸銉ヮ仾閻忓繒鍠栧畷婵嬪Ω閵夈儲顥濋梺?
        vibration_q[new_angle].resize(num_modes, 0.0);
        vibration_dq[new_angle].resize(num_modes, 0.0);
        vibration_ddq[new_angle].resize(num_modes, 0.0);

        // 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥冨妼閻╀線鏌涢弬璇插鐎殿噮鍓熷顐﹀级鐠恒劍鎲奸梺绋胯閸斿海鍒掗妸鈺佸嚑闁告帗鍔曡灒闁斥晛鍟犻崑鎾寸▕?
        current_vibration_q.resize(num_modes);
        next_vibration_q.resize(num_modes);

        QDir().mkpath("data/Modal");
        const QString modal_response_path = QDir("data/Modal").filePath("modal_response_debug.tsv");
        const QFileInfo modal_response_info(modal_response_path);
        const bool write_modal_response_header =
            !modal_response_info.exists() || modal_response_info.size() == 0;
        QFile modal_response_file(modal_response_path);
        const bool modal_response_writable =
            modal_response_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        QTextStream modal_response_out(&modal_response_file);
        if (modal_response_writable) {
            modal_response_out.setRealNumberNotation(QTextStream::FixedNotation);
            modal_response_out.setRealNumberPrecision(12);
            if (write_modal_response_header) {
                modal_response_out << "tool_angle\tnew_angle\tmode\tlambda\tvibr_k_eff"
                    << "\tf_total\tf_eff_total\tq_previous\tq_raw\tq_final"
                    << "\tq_limit\tphi_max_abs\tclipped\n";
            }
        }

        // 缂佺虎鍙庨崰鏇犳崲濮橆厾鈻斿┑鐘辫兌椤忛亶鏌￠崘銊у煟婵☆偄鐏濋～銏ゅΨ閵夈儺娼濋梺鍛婄閸ㄥ潡宕抽悜妯虹窞鐟滃秹鎯冮鈧～銏ゆ晲閸ワ絺鍋?
        if (vibration_f_prev.size() != num_modes) {
            vibration_f_prev.resize(num_modes, 0.0);
        }

        // 婵炴垶鎸搁悺銊ヮ渻閸屾壕鍋撳☉娅亪宕戝鍫涗汗闁规儳鍟块·鍛存煛閸愩劎鍩ｆ俊顐㈢仢椤垽濡烽敂鐐栨繛鎴炴惄娴滄繆鍟梺璇″厸閼宠泛鈻撻幋锕€绀?
        std::vector<double> f_current(num_modes, 0.0);

        // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鐎ｅ棔绶氶獮鈧?
        for (size_t mode = 0; mode < num_modes; ++mode) {
            double f_total = 0.0;
            double f_eff_total = 0.0;

            // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鎼粹剝顔囬梺姹囧灩瀹曨剛鈧濞婇悰顕€宕滄担鐑樼劸闂佸憡姊绘刊瀵告濞嗘挸缁╅梺顐ｇ缁€瀣⒑椤掆偓缁夋潙顔忕猾顧磄le_force_map婵炴垶鎼╅崢鎯р枔閹达箑绠ラ柍褜鍓熷鍨緞婵犲偆娼濋梺杞拌兌婢ф鐣垫笟鈧弫?
            for (size_t dof = 0; dof < num_dofs; ++dof) {
                // 闂備緡鍓欑粔鏉戭啅缁櫞gle_force_map婵炴垶鎼╅崢鎯р枔閹存粳鎺曠疀鎼淬劌娈漛lade_num闁诲海鏁搁幊鎾惰姳閺屻儲鍎嶉柛鎺濇磯rce_map
                for (const auto& [blade_num, inner_force_map] : angle_force_map) {
                    // 闂備緡鍓欑粔鏉戭啅閺勫繈浜归柟鎯у暱椤ゅ崑lade_num闁诲海鏁搁幊鎾惰姳閺屻儲鍎嶉柛鏇ㄥ墮椤ｆ煡鏌￠崼婵愭Ц濠殿喖绻樺顐︽偋閸繄銈?
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

            // 闁荤姳绶ょ槐鏇㈡偩閼姐倗椹冲璺侯儐濞呭繘鏌?
    //                f_eff_total = f_total + (vibr_a0 * vibration_q[mode] +
    //                                       vibr_a2 * vibration_dq[mode] +
    //                                       vibr_a3 * vibration_ddq[mode])
    //                             + vibr_stock_c[mode] * (vibr_a1 * vibration_q[mode] +
    //                                                   vibr_a4 * vibration_dq[mode] +
    //                                                   vibr_a5 * vibration_ddq[mode]);
            // 闁荤姳绶ょ槐鏇㈡偩閼姐倗椹冲璺侯儐濞呭繘鏌?
            f_eff_total = f_total;

            double relaxation_factor = 0.7; // 闂佸搫顦伴崕宕囨闁秴鐐婇柣妯垮皺閹藉秹鏌?-1婵炴垶鏌ㄩ澶娢?
            // 闂佸搫娲ら悺銊╁蓟婵犲洤绠版い鏍ㄨ壘琚熼梺鍛婄懃閸婂綊寮抽悢鍏兼櫖闁割偅绻勯幗鐘绘煕鐏炶濡奸柟顔兼喘閹虫盯顢旈崱妯绘闁硅壈鎻紓姘辩不閿濆妞界€光偓鐎ｎ剛顦?
            current_vibration_q[mode] = next_vibration_q[mode];
            const double q_raw = current_vibration_q[mode] * (1 - relaxation_factor) +
                (f_eff_total / vibr_k_eff[mode]) * relaxation_factor;
            next_vibration_q[mode] = q_raw;

            if (!std::isfinite(next_vibration_q[mode])) {
                next_vibration_q[mode] = current_vibration_q[mode];
            }

            double q_limit = std::numeric_limits<double>::infinity();
            double phi_max_abs = 0.0;
            bool clipped = false;
            if (mode < vibration_mode_max_abs.size() && vibration_mode_max_abs[mode] > 1e-12) {
                phi_max_abs = vibration_mode_max_abs[mode];
                const double max_displacement = std::max(1e-6, std::abs(modal_displacement_limit_mm));
                q_limit = max_displacement / vibration_mode_max_abs[mode];
                if (next_vibration_q[mode] > q_limit) {
                    next_vibration_q[mode] = q_limit;
                    clipped = true;
                }
                else if (next_vibration_q[mode] < -q_limit) {
                    next_vibration_q[mode] = -q_limit;
                    clipped = true;
                }
            }



            vibration_q[new_angle][mode] = next_vibration_q[mode];
            if (modal_response_writable) {
                modal_response_out << tool_angle << '\t'
                    << new_angle << '\t'
                    << mode << '\t'
                    << vibration_values[mode] << '\t'
                    << vibr_k_eff[mode] << '\t'
                    << f_total << '\t'
                    << f_eff_total << '\t'
                    << current_vibration_q[mode] << '\t'
                    << q_raw << '\t'
                    << next_vibration_q[mode] << '\t'
                    << q_limit << '\t'
                    << phi_max_abs << '\t'
                    << (clipped ? 1 : 0) << '\n';
            }

            //std::cerr << "Mode:" << mode
            //    << " f_eff=" << f_eff_total
            //    << " f_total=" << f_total
            //    << " vibration_q=" << next_vibration_q[mode]
            //    << std::endl;
        }
        temp_vibration_vectors = vibration_vectors;
    }

    // 闂佸搫鍊瑰姗€路閸愵亝濯奸柨娑樺閺嗩剟鏌￠崼姘壕婵犮垹鐖㈤崟顑跨帛闁诲繐绻愮换鎰板焵椤掑倸甯堕柛銈変憾瀵?
    void broaching_AptCutterVolume::calculateStraightness() {
        // 闂備緡鍓欑粔鏉戭啅缂佹ê绶為柡宓懏鍕鹃梺鍝勫婵挳鎯冮悩缁樻櫖闁割偓缍嗗锟犲箹鏉堟崘顓虹紒?
        for (const auto& angle_pair : cut_h_map) {
            double angle = angle_pair.first;  // 闁荤喐鐟︾敮鎺斺偓?
            const auto& blade_map = angle_pair.second;  // 闂佸憡宸婚崑鎾绘煟濡も偓濡叉ェ闂佸搫瀚慨鎾儍?

            // 闂備緡鍓欑粔鏉戭啅缂佹鈻旀い鎾跺仧濠€鎾煛閸曨厼孝闁汇劎濞€閺佸秹宕煎┑鍡欌偓顒勬煟濡も偓濡叉ェ闂?
            for (const auto& blade_pair : blade_map) {
                int blade_id = blade_pair.first;  // 闂佸憡宸婚崑鎾绘煟濡も偓濡叉ェ
                const auto& point_map = blade_pair.second;  // 闂佺粯鍔曞﹢閬嶅磼閵娿儺鍤曢柡鍥╁枑琛奸柣?

                // 闂備緡鍓欑粔鏉戭啅婵犳艾绀冮柛娑卞幘濠€鎾煛閸曨厼孝闁汇劎濞€閺佸秹宕奸姀鐘卞寲缂備椒绌堕崹鍦閳哄懏鏅?
                for (const auto& point_pair : point_map) {
                    int point_index = point_pair.first;  // 闂佺粯鍔曞﹢閬嶅磼閵娿儺鍤?
                    double dmin_value = point_pair.second;  // dmin闂?

                    // 闁诲孩绋掗敋闁稿绉瑰畷?straightness_map
                    // 濠电偛顦崝宥夊礈娴煎瓨鏅慨妯虹－缁犲綊姊洪幓鎺戭殭缂佺粯宀搁獮鎰媴閻戞銈查梺鍛婅壘閻厧鈻撻幋鐐碘枖濠㈣泛锕﹀﹢瀵哥磽娴ｈ灏伴柣?
                    straightness_map[angle][blade_id][point_index] = dmin_value;
                }
            }
        }
    }

    // 婵烇絽娴傞崰妤呭极婵傜瑙﹂幖杈剧稻閻ｉ亶鏌涢幋锝呅撻柡鍡欏枛閺佸秴顫濆畷鍥╃倳闂佸憡鍨归崕銈嗘櫠瀹ュ瀚夊┑澶屾箯side_index闁诲海鏁搁幊鎾惰姳閺屻儲鍎嶉柛鎾虫晢raightness闂佽桨鑳舵晶妤€鐣垫笟鈧顒勫炊閿旂瓔鍋?
    void broaching_AptCutterVolume::outputStraightnessData(const std::string & output_dir) {
        // 闂佽　鍋撻柛顐ｆ礃閼茬娀鏌熺喊妯轰壕闂佸搫鐗嗛ˇ顖炴偤閵娾晛鎹堕柕濞у嫮鏆犻梺鍛婂坊閸嬫捇鏌涜箛鏆风嵍闂佸憡绮岄惉濂稿磻閿濆洦顫曢柕蹇曞Х缁屽潡鏌ㄥ☉妯煎妤犵偞鎹囬弻灞筋吋韫囨洜顦?
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

        std::cout << "straightness blade ids: " << all_blade_ids.size() << std::endl;
        std::cout << "straightness point indices: " << all_point_indices.size() << std::endl;

        // 婵炴垶鎸鹃崕銈夋儊閳╁啰鈻旀い蹇撳閻庮剟鏌涜箛鏆风嵍闂佸憡绮岄張顒勬儊閳╁啰鈻旀い蹇撴娴狀垳绱掓笟鍨仼缂佹墎鏅濈槐鎺楀礋椤愶絽鈧倝鏌ｉ姀銏犳瀾闁搞劍宀稿顒勫炊閿旂瓔鍋?
        for (int blade_id : all_blade_ids) {
            for (int point_index : all_point_indices) {
                // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姤缂佺粯鐗楃粙澶愵敂閸℃瑧鐓侀梺鍛婅壘閻楀﹤危閹间礁瑙﹂柨鏃囧Г缁犳帡鏌℃担鍝勵暭鐎?
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
                    continue;  // 闁荤姴鎼悿鍥╂崲閸愩劉鏌﹂柍鈺佸暞缁犳帡鏌℃担鍝勵暭鐎规挷绶氶幆鍐礋椤撶姷鐓侀梺?
                }

                // 闂佸搫顑呯€氫即鍩€椤掑倸鞋缂侀鍙冨畷娆撴倻濡崵鈧喖霉閻樺啿鍔堕柣顓熷劤椤曘儵宕熼崜浣侯槱闂佸搫绉堕崢褏妲愰敓鐘虫櫖婵繄娈焧put_dir/blade_[blade_id]_point_[point_index].txt闂?
                std::string file_path = output_dir + "/blade_" + std::to_string(blade_id) +
                    "_point_" + std::to_string(point_index) + ".txt";

                // 闂佸憡甯楃粙鎴犵磽閹捐崵宓侀柤鎼佹涧閳數鈧鍠掗崑鎾绘煛閸屾碍鐭楁繛鍡愬灲閺佸秹宕奸敐搴㈣埞闂佺儵鏅滈悧妤勫暞閻庢鍠栨蹇曟?
                std::ofstream out_file(file_path);
                if (!out_file.is_open()) {
                    std::cerr << "闂佸搫鍟版慨鐢垫兜閸洖绠ラ柟鎯х－绾惧寮堕崼鐔稿碍闁搞値鍙冨顒勫炊閿旂瓔鍋? " << file_path << std::endl;
                    continue;  // 闁荤姴鎼悿鍥╂崲閸愵厹浜归柟鎯у暱椤ゅ懐绱撴担绋款仼闁诡喖閰ｉ弫宥呯暆閳ь剟骞嬫搴ｇ＜妞ゆ挾鍎愬Σ閬嶆煟閻愬弶顥欑紒妤€鎳忕粙澶愬焵椤掍胶鈻?
                }

                // 闂佸憡鍔栭悷銉╁矗閸℃稑妫橀柛銉檮椤愯棄顭?
                out_file << "# blade_id: " << blade_id << " point_index: " << point_index << std::endl;
                out_file << "# angle\tstraightness" << std::endl;

                // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鎼搭喗鍋ラ柟鑹版彧鐠侊絿妲愬┑瀣哗闁割偅娲橀懖鐘绘偣閸ャ儱鍔氶柛銊ょ窔瀹曟绮欓妴銉╂煕濠婂啰鐏遍柛瀣剁悼濡叉劙濮€閻樼數鈹涢梺姹囧妼鐎氼參寮抽悢鐓庣?
                for (const auto& angle_pair : straightness_map) {
                    double angle = angle_pair.first;
                    const auto& blade_map = angle_pair.second;

                    auto blade_it = blade_map.find(blade_id);
                    if (blade_it != blade_map.end()) {
                        const auto& point_map = blade_it->second;
                        auto point_it = point_map.find(point_index);
                        if (point_it != point_map.end()) {
                            // 闂佸憡鍔栭悷銉╁矗閸℃稒鏅慨妯诲墯濞硷繝骞栨潏鍓х暠闁归攱澹嗛埀顒傛暩閹虫挾鑺遍弻銉﹀剭闁告洦鍘界痪顖滅磼閹呬虎閻庤濞婂畷?
                            out_file << angle << "\t" << point_it->second << std::endl;
                        }
                    }
                }

                out_file.close();
                std::cout << "闂佽桨鑳舵晶妤€鐣垫担鍓插晠闁肩厧澧庣紙濠氭煕閹达妇绱伴柛? " << file_path << std::endl;
            }
        }
        straightness_map.clear();
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
        //闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥庡墰缁犮儵鏌涢弬璇插妞ゆ帞鍠栧畷锝夊磼濞戞瑦顔?
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
        // 婵炴垶鎸哥粔鏉戯耿椤忓懐顩烽悹浣哥－缁夊潡鏌涢幒鎴烆棡妞ゆ柨鐭傚畷姗€宕崘顏嗩槷闁哄鏅滈弻銊ッ洪弽顐ｅ闁绘柨鐨濋崑鎾舵兜妞嬪海顦╂繝銏ｅ煐閻楃娀宕曢幘顔芥櫖?
        return t.z;
    }

    Cutting milling_AptCutterVolume::dist_cd(const GLVertex & p) const {
        Cutting result = { 0.0, NO_COLLISION, 1 };
        result.f = dist(p);
        // 闂佸憡鐟崹閬嶆偋閹绢喖绠叉い鏇楀亾婵炴挸澧庨幉鐗堟媴閻熸壋鎸呴柣蹇曞仦濞叉粓锝為锕€绠婚柣鎰祷椤箓鏌涢妸銉剰闁搞劎鏅埀?
        return result;
    }

    std::vector<std::vector<std::vector<double>>>
        milling_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            const std::vector<size_t>&modes_to_read) {  // 婵烇絽娴傞崰妤呭极婵傜鐭楅柛灞剧⊕濞堣泛鈽夐幘璺哄妺閼垛晠鏌熼鑳厡闁稿被鍔岄锝夊即閻愯尙浠氶柣?
        if (pre_vibration_vectors.empty()) {
            return {};
        }

        // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕闁哄棛鍠栭獮鎴︻敊閼测晜娈梺杞扮閻°劑鍩€?
        size_t total_dofs = pre_vibration_vectors[0].size();
        if (total_dofs % num_dofs != 0) {
            throw std::invalid_argument("total dofs is not divisible by node dofs");
        }

        // 闂佸憡甯楃粙鎴犵磽閹惧鈻斿璺烘湰濡﹪鏌涘顓炵伌闁革絾妞介弫宥夊醇閻斿搫顥戦梺纭咁嚃閸犳鈧灚姘ㄩ埀顒冾潐绾板秷鍟梺璇″厸閻掞箓寮抽悢鍏肩厒闊洢鍎崇粈?
        size_t num_nodes = total_dofs / num_dofs;
        std::vector<std::vector<std::vector<double>>> vibration_vectors(
            modes_to_read.size(),  // 闂佸搫绉烽～澶婄暤娓氣偓楠炴劙宕惰閺嗙増淇婇妤€澧查柍褜鍏涢悞锕傚汲閻斿吋鐓傞煫鍥ㄦ尭閻忥紕鈧?
            std::vector<std::vector<double>>(num_dofs, std::vector<double>(num_nodes))
        );

        // 闁哄鍎愰崜姘暦閺屻儱绠伴柛銉戝懏姣庡┑鈽嗗灙閸撴繈鍩€椤戣法鍔嶉柡鍡欏枛楠?
        for (size_t i = 0; i < modes_to_read.size(); ++i) {
            size_t mode = modes_to_read[i];
            if (mode >= pre_vibration_vectors.size()) {
                throw std::invalid_argument("requested mode index is out of range");
            }

            // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕閼垛晠鏌熼璺ㄥ妽闁哄棛鍠栭獮鎴︻敊閼姐値浼囬梺鐓庡槻閻°劑鍩€?
            if (pre_vibration_vectors[mode].size() != total_dofs) {
                throw std::invalid_argument("mode vector length is inconsistent");
            }

            // 婵犻潧顦介崑鍕储閺嶎厼鏋侀柣妤€鐗嗙粊?
            for (int dof = 0; dof < num_dofs; ++dof) {
                for (size_t node = 0; node < num_nodes; ++node) {
                    size_t index = node * num_dofs + dof;
                    vibration_vectors[i][dof][node] = pre_vibration_vectors[mode][index];
                }
            }
        }

        return vibration_vectors;
    }

    // 婵烇絽娲︾换鍕汲閳ь剟鏌涘Ο鐓庢瀻闁搞倝浜跺顐﹀箥椤旇姤娈㈡繛瀛樼矊妤犳悂骞冨鍫濊Е閹肩补鈧櫕鍊柣?
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


    //闁荤姴娲╅褑銇愰崶顒€绀嗛柛銉戝喚鏉搁梺鍛婂笒閸熶即宕戦敐澶嬧挅?
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
        // 闁荤姳绶ょ槐鏇㈡偩缂佹鈻斿璺侯樀閸ゅ寮堕埡鍐ㄤ户闁汇垹顭烽幆鍐礋椤斿墽褰鹃梺鍛婄閸ㄥ潡宕冲ú顏勭煑濠㈣泛鐫楀┑鍫㈢當闁挎洍鍋撶憸?
        GLVertex ab = b - a;
        GLVertex bc = c - b;
        GLVertex cd = d - c;
        GLVertex da = a - d;

        // 闁荤姳绶ょ槐鏇㈡偩鐠囨祴鏋栭柡鍥╁Т濞堢娀鏌￠崒婊勫殌闁诡喗绮撻幆鍐礆韫囨稑绀嗛柛鈩冪☉濞?
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

#ifdef CUTSIM_PROFILE_POSITION_BLADE
        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();
#endif

        // 1. 婵☆偓绲鹃悧鏇㈠储濞戞矮鐒婇柛鈩兠。鏌ユ煛閸繍妲洪柛瀣剁秮閺佸秹宕煎┑鍡樻灳婵犳鍠栭鍥磻閿濆妞介悘鐐村灊閻掑﹦绱掓径搴″惞闁稿缍侀弫宥嗗緞鐎ｎ偅姣嗛梻鍌氭濡宕虹仦鍓р枖闁逞屽墯缁嬪顢旈崘顭戞Щ闂佸搫鍟冲▔娑㈩敊閹版澘闂柕濞垮€涢崢?
        blade_points.clear();  // 濠电偞鎸搁幊鎰板煘閺嶎偀鍋撻崷顓熷殌婵?
        blade_points.reserve(original_blade_points.size() * 4); // 婵☆偅婢樼€氼剟宕规惔銊︾厐鐎广儱娲犻弫鍕⒒?

        const GLfloat center_x = center.x;
        const GLfloat center_y = center.y;
        const GLfloat center_z = center.z;
        const GLfloat dx_f = static_cast<GLfloat>(dx);
        const GLfloat dy_f = static_cast<GLfloat>(dy);
        const GLfloat dz_f = static_cast<GLfloat>(dz);
        const GLfloat blade_angle_f = static_cast<GLfloat>(blade_angle);
        const GLfloat offset_angle_f = static_cast<GLfloat>(blade_angle - step);
        const GLfloat blade_cos = static_cast<GLfloat>(std::cos(blade_angle_f));
        const GLfloat blade_sin = static_cast<GLfloat>(std::sin(blade_angle_f));
        const GLfloat offset_cos = static_cast<GLfloat>(std::cos(offset_angle_f));
        const GLfloat offset_sin = static_cast<GLfloat>(std::sin(offset_angle_f));
        auto check_convexity_xy = [](const GLVertex& a, const GLVertex& b,
            const GLVertex& c, const GLVertex& d) {
            const double abx = b.x - a.x;
            const double aby = b.y - a.y;
            const double bcx = c.x - b.x;
            const double bcy = c.y - b.y;
            const double cdx = d.x - c.x;
            const double cdy = d.y - c.y;
            const double dax = a.x - d.x;
            const double day = a.y - d.y;

            const double cross1 = abx * bcy - aby * bcx;
            const double cross2 = bcx * cdy - bcy * cdx;
            const double cross3 = cdx * day - cdy * dax;
            const double cross4 = dax * aby - day * abx;

            if ((cross1 * cross2 >= 0) &&
                (cross2 * cross3 >= 0) &&
                (cross3 * cross4 >= 0)) {
                return 1;
            }
            if ((cross1 * cross2 >= 0) &&
                (cross2 * cross3 < 0) &&
                (cross3 * cross4 >= 0)) {
                return 2;
            }
            if ((cross1 * cross2 > 0) &&
                (cross2 * cross3 < 0) &&
                (cross3 * cross4 < 0)) {
                return 3;
            }
            if ((cross1 * cross2 >= 0) &&
                (cross2 * cross3 >= 0) &&
                (cross3 * cross4 < 0)) {
                return 4;
            }
            return 5;
            };

        for (const auto& point : original_blade_points) {
            const GLVertex center_orign(center_x, center_y, point.z + center_z); // a
            const GLVertex point_orign(
                point.x * blade_cos - point.y * blade_sin + center_x,
                point.x * blade_sin + point.y * blade_cos + center_y,
                point.z + center_z); // b
            const GLVertex point_offset(
                point.x * offset_cos - point.y * offset_sin + center_x - dx_f,
                point.x * offset_sin + point.y * offset_cos + center_y - dy_f,
                point.z + center_z - dz_f); // c
            const GLVertex center_offset(center_x - dx_f, center_y - dy_f, point.z + center_z - dz_f); // d

            // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姎婵炲弶鎸诲蹇涙偡閹峰苯鐓傞梺鍛婂灥閹诧繝鍩€椤戭剙绉剁粈澶愭煠濮瑰洤鍔欓悹鎰枛瀹曟瑩鎮烽弶璺ㄤ户婵炲瓨鍤庨崐鏍х暦閻捇鏌涘鍐殸闂佹眹鍔岀€氫即濡存惔銏″劅?
            const int is_convex = check_convexity_xy(center_orign, point_orign, point_offset, center_offset);
            if (is_convex == 2) {
                blade_points.push_back(center_offset);  //d
                blade_points.push_back(point_orign);  //b
                blade_points.push_back(point_offset);  //c
                blade_points.push_back(center_orign);  //a
            }
            else {
                blade_points.push_back(center_orign);  //a
                blade_points.push_back(point_orign);  //b
                blade_points.push_back(point_offset);  //c
                blade_points.push_back(center_offset);  //d
            }
        }

        // 6. 婵炴潙鍚嬮敋閻庡灚鐓″畷鐘诲川椤撶喓鍑介梺鐑╂櫆鐢繝銆傞埡鍐╁闁挎稑瀚弳?
        blade_bboxes.clear();
        const size_t total_points = blade_points.size();
        const size_t bbox_count = (total_points >= 8) ? ((total_points - 4) / 4) : 0;
        blade_bboxes.reserve(bbox_count); // 婵☆偅婢樼€氼剟宕规惔銊︾厐鐎广儱鎳庨惁鍫曟煕閵夈儺鍤熸繛鎻掓噽缁艾煤椤忓拑绱?
        const double expand_factor = 0.1;

        auto include_bbox_point = [](BoundingBox& bbox, const GLVertex& p) {
            bbox.min.x = std::min(bbox.min.x, p.x);
            bbox.min.y = std::min(bbox.min.y, p.y);
            bbox.min.z = std::min(bbox.min.z, p.z);
            bbox.max.x = std::max(bbox.max.x, p.x);
            bbox.max.y = std::max(bbox.max.y, p.y);
            bbox.max.z = std::max(bbox.max.z, p.z);
            };

        // 7. 婵犮垼娉涚€氼噣骞冩繝鍌傛帟绠涙惔銊ユ疂闂佸憡鐗曢幊搴∶洪崸妤佸剮?
        for (size_t i = 0; i + 7 < total_points; i += 4) {
            BoundingBox bbox;

            // 闂佸吋鍎抽崲鑼躲亹?婵炴垶鎼╂禍鐐哄磻閿濆鍎嶉柛鏇ㄥ亞缁屽潡鏌ｉ姀鈺冨帨缂佽鲸绻堥弻鍡涘垂椤旂厧璧嬮梻浣瑰絻缁夋挳藝閼碱剚顫曢柕蹇曞Х缁?
            const GLVertex& p1 = blade_points[i];
            const GLVertex& p2 = blade_points[i + 1];
            const GLVertex& p3 = blade_points[i + 2];
            const GLVertex& p4 = blade_points[i + 3];
            const GLVertex& p5 = blade_points[i + 4];
            const GLVertex& p6 = blade_points[i + 5];
            const GLVertex& p7 = blade_points[i + 6];
            const GLVertex& p8 = blade_points[i + 7];

            // 闂佸綊娼ч鍛叏閳哄啯濯奸柨娑樺閺嗩剟寮堕崼婵囪础闁哄拋鍋勯々濂稿幢椤撶姷顦┑顕嗙稻閺佸兗d::min/max闂佸搫娲﹂幐鎶芥偟椤曗偓瀵?
            // 闁荤姳绶ょ槐鏇㈡偩婵犳艾瀚夐柍褜鍓涙禍姝岀疀閹绢垰浜?
            bbox.min.x = p1.x;
            bbox.min.y = p1.y;
            bbox.min.z = p1.z;
            bbox.max.x = p1.x;
            bbox.max.y = p1.y;
            bbox.max.z = p1.z;

            // 闂佸搫鐗冮崑鎾绘倶韫囨挾绠伴柍?
            include_bbox_point(bbox, p2);
            include_bbox_point(bbox, p3);
            include_bbox_point(bbox, p4);
            include_bbox_point(bbox, p5);
            include_bbox_point(bbox, p6);
            include_bbox_point(bbox, p7);
            include_bbox_point(bbox, p8);

            // 闂佸搫鐗冮崑鎾愁熆閸棗鍟犻崑?
            // 闂佸湱顣介弲娑㈡儓瀹ュ棙缍囬柛锔诲幗濞呮洘淇婂Δ鈧Λ瀵告濠靛绀傚ù锝囩摂閸熷懎鈽夐幘顖氫壕闁诲氦顫夊銊モ枔閹寸姵瀚氭い鏍ㄧ閳?
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

#ifdef CUTSIM_PROFILE_POSITION_BLADE
        stop = std::chrono::system_clock::now();
        qDebug() << "calculatePosition_balde():" << std::chrono::duration<double>(stop - start).count() << "sec.";
#endif

    }

    void milling_AptCutterVolume::calculateForceData() {

        //std::chrono::system_clock::time_point start, stop;
        //start = std::chrono::system_clock::now();

        force_map.clear();

        auto dotVertex = [](const GLVertex& a, const GLVertex& b) {
            return static_cast<double>(a.x) * b.x +
                static_cast<double>(a.y) * b.y +
                static_cast<double>(a.z) * b.z;
            };
        auto vibrationValue = [this](size_t mode, size_t dof, int node_id) {
            if (mode >= temp_vibration_vectors.size() ||
                dof >= temp_vibration_vectors[mode].size() ||
                node_id < 0 ||
                static_cast<size_t>(node_id) >= temp_vibration_vectors[mode][dof].size()) {
                return 0.0;
            }
            return temp_vibration_vectors[mode][dof][node_id];
            };

        const int mechanics_angle_index = (std::abs(step) > 1e-12)
            ? static_cast<int>(std::llround(tool_angle / step))
            : 0;
        if (enable_mechanics_map_capture) {
            mechanics_map_library.removeForAngleBlade(mechanics_angle_index, blade_num);
        }

        for (const auto& pair : cut_h) {
            int inside_index = pair.first;    // 闂佸吋鍎抽崲鑼躲亹閸ｅ埖side_index
            double force_cut_h = pair.second.avg_dmin; // 婵炲濮村ù椋庡垝閵娾晛鍑犻柛鏇ㄤ簽缁夌厧鈽夐幙鍐ㄥ绩妤犵偛绻樺畷锝夊冀瑜旈幐顒勬煕瑜嶅ú銈夊垂韫囨稑绀堝┑鐘插暟缁犳帡骞?
            int force_cut_positionID = pair.second.node_id; // 婵炲濮村ù椋庡垝閵娾晛鍑犻柛鏇ㄤ簽缁夌厧鈽夐幙鍐ㄥ绩妤犵偛绻樺畷锝夊冀閵婏缚绮柣蹇撶箰缁绘绮╅悢鐑樺磯婵犻潧锕﹂悰鈺冪磼閸屾繍鍤欐い鏇ㄥ枟閹棃寮崶顬繈鏌ｉ幇顕呭劋D


            if (inside_index == 0)continue;//inside_index=0闂佸搫鍟抽鎰濠靛绀嗛柛銉戝喚鏉搁梺鍛娚戦懝鍓р偓鐟扮－閳ь剚绋掗敋婵犫偓椤忓牊鈷掓い鏇楀亾妞?


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

            // 缂備焦顨忛崗娑氱博閺夋埈娼伴柕澶樺灣缁愭鎮规笟濠勭？闁伙絽顭峰畷姘攽閸♀晜缍忛梺鍛婃⒐缁嬫垿宕规惔銊︾厒?
            double initial_Fr = (force_coefs[3] + force_coefs[0] * force_cut_h) * force_cut_w;
            double initial_Ft = (force_coefs[4] + force_coefs[1] * force_cut_h) * force_cut_w;
            double initial_Fa = (force_coefs[5] + force_coefs[2] * force_cut_h) * force_cut_w;

            // 闂佸搫鍟鍫澝归崱娆愬枂闁圭儤鍨甸濠囨偣娓氬﹦纾块柣?
            GLVertex blade_point = blade_points[inside_index * 4 + 1];
            GLVertex blade_center = blade_points[inside_index * 4];

            // 闁荤姳绶ょ槐鏇㈡偩鐠囧樊鍤楅柛鏇ㄥ亝閸婂鏌?(force_vr)
            GLVertex force_vr = blade_point - blade_center;
            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓本袩闂?
            double length = force_vr.norm();
            if (length > 0) {
                force_vr = force_vr * (1.0 / length);
            }

            // 闁荤姳绶ょ槐鏇㈡偩缂佹ɑ濮滈柡澶嬪灦閸婂鏌?(force_va)
            GLVertex force_va = GLVertex(0.0, 0.0, -1.0).rotateCBA(angle.x, angle.y, angle.z);
            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓本袩闂?
            length = force_va.norm();
            if (length > 0) {
                force_va = force_va * (1.0 / length);
            }

            // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柛銉戝嫬鈧鏌?(force_vt)
            GLVertex force_vt = force_va.cross(force_vr);
            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓本袩闂?
            length = force_vt.norm();
            if (length > 0) {
                force_vt = force_vt * (1.0 / length);
            }

            // 闂佸憡姊圭粙鎴﹀垂鎼淬劍鐓傞煫鍥ㄦ⒐椤ュ寮?
            double rotated_Fx = initial_Fr * force_vr.x + initial_Ft * force_vt.x + initial_Fa * force_va.x;
            double rotated_Fy = initial_Fr * force_vr.y + initial_Ft * force_vt.y + initial_Fa * force_va.y;
            double rotated_Fz = initial_Fr * force_vr.z + initial_Ft * force_vt.z + initial_Fa * force_va.z;

            // 缂備焦顨忛崜娆徝洪幘鎰佹桨闁靛鍨崇粣妤呮偣瑜嶉鍛村焵?
            fd.force_value = GLVertex(rotated_Fx, rotated_Fy, rotated_Fz);

            // 闁荤姳鐒﹀妯肩礊瀹ュ棙濯寸€广儱娲ㄩ弸?
            fd.force_position = blade_bottom;

            if (enable_mechanics_map_capture) {
                MechanicsMapRecord record;
                record.tool_angle = tool_angle;
                record.angle_index = mechanics_angle_index;
                record.blade_id = blade_num;
                record.inside_index = inside_index;
                record.active = force_cut_h > 0.0 && force_cut_positionID >= 0;
                record.h0 = force_cut_h;
                record.h_online = force_cut_h;
                record.w = force_cut_w;
                record.force_position_id = force_cut_positionID;
                record.node_id_bottom = pair.second.node_id;
                record.node_id_up = pair.second.node_id_up >= 0 ? pair.second.node_id_up : pair.second.node_id;
                record.P0 = pair.second.P0;
                record.P1 = pair.second.P1;
                if ((record.P1 - record.P0).norm() <= 1e-12) {
                    record.P0 = blade_bottom;
                    record.P1 = blade_up;
                }
                record.Q = pair.second.P_min;
                record.vr = force_vr;
                record.vt = force_vt;
                record.va = force_va;
                record.nominal_force = fd.force_value;

                const GLVertex edge = record.P1 - record.P0;
                const double edge_norm2 = dotVertex(edge, edge);
                double tau = 0.0;
                if (edge_norm2 > 1e-18) {
                    tau = dotVertex(record.Q - record.P0, edge) / edge_norm2;
                    tau = std::max(0.0, std::min(1.0, tau));
                }
                GLVertex projected = record.P0 + edge * tau;
                record.n_h = record.Q - projected;
                if (record.n_h.norm() > 1e-12) {
                    record.n_h.normalize();
                }
                else {
                    record.n_h = force_vr;
                }

                const GLVertex directions[3] = { force_vr, force_vt, force_va };
                for (int row = 0; row < 3; ++row) {
                    const double components[3] = {
                        row == 0 ? directions[0].x : (row == 1 ? directions[0].y : directions[0].z),
                        row == 0 ? directions[1].x : (row == 1 ? directions[1].y : directions[1].z),
                        row == 0 ? directions[2].x : (row == 1 ? directions[2].y : directions[2].z)
                    };
                    const int base = row * 6;
                    record.Phi0[base + 0] = force_cut_h * force_cut_w * components[0];
                    record.Phi0[base + 1] = force_cut_h * force_cut_w * components[1];
                    record.Phi0[base + 2] = force_cut_h * force_cut_w * components[2];
                    record.Phi0[base + 3] = force_cut_w * components[0];
                    record.Phi0[base + 4] = force_cut_w * components[1];
                    record.Phi0[base + 5] = force_cut_w * components[2];
                    record.dPhi_dh[base + 0] = force_cut_w * components[0];
                    record.dPhi_dh[base + 1] = force_cut_w * components[1];
                    record.dPhi_dh[base + 2] = force_cut_w * components[2];
                    record.dPhi_dh[base + 3] = 0.0;
                    record.dPhi_dh[base + 4] = 0.0;
                    record.dPhi_dh[base + 5] = 0.0;
                }
                record.Phi_online = record.Phi0;

                const size_t mode_count = temp_vibration_vectors.size();
                record.S_q.resize(mode_count, 0.0);
                record.B_force.resize(mode_count * 3, 0.0);
                for (size_t mode = 0; mode < mode_count; ++mode) {
                    GLVertex phi_bottom(
                        vibrationValue(mode, 0, record.node_id_bottom),
                        vibrationValue(mode, 1, record.node_id_bottom),
                        vibrationValue(mode, 2, record.node_id_bottom));
                    GLVertex phi_up(
                        vibrationValue(mode, 0, record.node_id_up),
                        vibrationValue(mode, 1, record.node_id_up),
                        vibrationValue(mode, 2, record.node_id_up));
                    GLVertex phi_interp = phi_bottom * (1.0 - tau) + phi_up * tau;
                    record.S_q[mode] = dotVertex(record.n_h, phi_interp);
                    record.B_force[mode * 3 + 0] = vibrationValue(mode, 0, record.force_position_id);
                    record.B_force[mode * 3 + 1] = vibrationValue(mode, 1, record.force_position_id);
                    record.B_force[mode * 3 + 2] = vibrationValue(mode, 2, record.force_position_id);
                }

                mechanics_map_library.angle_step = step;
                mechanics_map_library.modal_count = mode_count;
                mechanics_map_library.theta_f = force_coefs;
                mechanics_map_library.addOrReplace(record);
            }

            // 闁诲孩绋掗敋闁告瑥鏀rce_map
            force_map[inside_index] = fd;
        }
        angle_force_map[blade_num] = force_map;

        //stop = std::chrono::system_clock::now();
        //qDebug() << "calculateForceData():" << std::chrono::duration<double>(stop - start).count() << "sec.";

    }

    void milling_AptCutterVolume::outputForceData(const std::string & output_dir) {

        std::chrono::system_clock::time_point start, stop;
        start = std::chrono::system_clock::now();

        // 闂佸搫顑呯€氫即鍩€椤掑倸鞋缂侀鍙冨畷娆撴倻濡崵鈧喖霉閻樺啿鍔堕柣顓熷劤椤曘儵宕熼崜浣侯槱闂佸搫绉堕崢褏妲愰敓鐘虫櫖婵繄娈焧put_dir/force_data_[tool_angle].txt闂?
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << tool_angle;  // 婵烇絽娲︾换鍕汲閳?婵炶揪绲界粔鎾儍椤掑嫬鏋佸ù鑲╃節缂傚鏌涜箛鎾缎ｉ柡瀣暞缁傛帞鎹勯悜妯衡偓鎶藉级閳轰焦鍠橀柡?
        std::string file_path = output_dir + "/force_data_" + oss.str() + ".txt";

        // 闂佸憡甯楃粙鎴犵磽閹捐崵宓侀柤鎼佹涧閳數鈧鍠掗崑鎾绘煛閸屾碍鐭楁繛鍡愬灲閺佸秹宕奸敐搴㈣埞闂佺儵鏅滈悧妤勫暞閻庢鍠栨蹇曟?
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "闂佸搫鍟版慨鐢垫兜閸洖绠ラ柟鎯х－绾惧寮堕崼鐔稿碍闁搞値鍙冨顒勫炊閿旂瓔鍋? " << file_path << std::endl;
            return;
        }

        for (const auto& inner_pair : angle_force_map) {
            int force_cutnum = inner_pair.first;  // 闂佸憡宸婚崑鎾绘煕閹烘垵鏆為柡?
            const auto& force_map_2 = inner_pair.second;  // ForceData 闂佽桨鑳舵晶妤€鐣?

            // 闂備緡鍓欑粔鏉戭啅缁尦rce_map_2闂佹寧绋戦懟顖炲疮閹捐绀傞柕澶涘濡层劌鈽夐幙鍐х盎闁搞劋绶氬畷姘跺礃椤忓嫪鍖栭梺姹囧妼鐎氼亞绱為崨顖滅＞妞ゆ柨鍚嬬€氭煡鏌涢弮鈧粙鎺楀汲閻旂厧绠?
            for (const auto& force_pair : force_map_2) {
                int inside_index = force_pair.first;
                const ForceData& fd = force_pair.second;
                // 闂佸憡鍔栭悷銉╁矗閸℃稒鏅慨姗嗗墮閻庮剟鏌涢幒鎴濇殲闁哄棛鍠栨俊瀛樻媴缁嬫寧鏆ラ梻渚囧枔閸斿酣宕掗妸銉殨闁哄洦菤閸嬫挻鎷呮搴Ｐ㈢紓鍌氬暟閺呮娊鏌曢崱鏇犵毥闂侀潧妫旂花婵嬫煥濞戞瀚板┑顔肩床闂侀潧妫旂花婊堟煏閸℃洜鐨鹃梺鎸庣☉閼活垶宕硅箛娑樼濠电姴鍊哥悮閬嶅箹鏉堟崘顓虹紒杈ㄧ箖缁傛帡濡烽妷銉﹀皨闂佸憡鑹剧€氭澘螣娓氣偓瀵?闂佺厧顨庢禍鐐哄极鏉堛劍鍎熼柨鏃傚亾閻ｉ亶鏌熼崜鎻掔仩濠殿喒鏅犲畷銉╁箣閿曗偓濞?
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

                // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞閹邦厸鏋嗛梺杞扮劍濠㈡﹢骞忔导瀛樺殜妞ゅ繐妫欓弳鐘诲箹鏉堟崘顓虹紒杈ㄧ箖濞煎繘骞橀崘鍙夌様闂佺懓澹婇崹鐗堟叏閳哄懎瑙﹂柟杈剧畱濞呫倝鏌℃担鍝勵暭鐎?
                for (size_t mode = 0; mode < temp_vibration_vectors.size(); ++mode) {
                    for (size_t dof = 0; dof < temp_vibration_vectors[mode].size(); ++dof) {
                        if (fd.force_position_id < temp_vibration_vectors[mode][dof].size()) {
                            out_file << temp_vibration_vectors[mode][dof][fd.force_position_id] << "\t";
                        }
                        else {
                            out_file << "0.0\t";  // 闂佸搫鍟版慨鐢稿疾閵壯勵潟闁靛繒濮风粚鍧楁煛閸愵厽纭鹃柨婵堝仱瀹?
                        }
                    }
                }
                out_file << std::endl;
            }
        }

        out_file.close();
        //std::cout << "闂佸憡姊圭粙鎺楀汲閻旂厧绠叉い鏃傚帶閸ゆ帡寮堕崼鐔稿碍闁搞値鍙冮幊? " << file_path << std::endl;

        stop = std::chrono::system_clock::now();
        qDebug() << "outputForceData():" << std::chrono::duration<double>(stop - start).count() << "sec.";

    }

    void milling_AptCutterVolume::calculateTotalForce() {

        //std::chrono::system_clock::time_point start, stop;
        //start = std::chrono::system_clock::now();

        temp_total_force = GLVertex(0, 0, 0);
        for (const auto& inner_pair : angle_force_map) {
            int  force_cutnum = inner_pair.first;  // 缂備焦顨忛崗娑氳姳閳哄啩娌柛宀€鍋為弳娑㈡煥濞戞ɑ缍抣oat闂?
            const auto& force_map_2 = inner_pair.second;  // ForceData 闂佽桨鑳舵晶妤€鐣?
            // 闂備緡鍓欑粔鏉戭啅缁尦rce_map闁荤姳绶ょ槐鏇㈡偩婵犳艾瑙﹂柛顐ｇ箓椤?
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
        // 闁诲孩绋掗敋闁稿绉磋闊洤閰ｉ崵瀣偡濞嗘劕绗掗悗瑙勫▕閹啴宕熼锝呪偓銈夋煕?
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
        // 闂佸搫顑呯€氫即鍩€椤掑倸鞋缂侀鍙冨畷娆撴倻濡崵鈧喖霉閻樺啿鍔堕柣顓熷劤椤曘儵宕熼崜浣侯槱闂佸搫绉堕崢褏妲愰敓鐘虫櫖婵繄娈焧put_dir/total_force_data.txt闂?
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << tool_angle;  // 婵烇絽娲︾换鍕汲閳?婵炶揪绲界粔鎾儍椤掑嫬鏋佸ù鑲╃節缂傚鏌涜箛鎾缎ｉ柡瀣暞缁傛帞鎹勯悜妯衡偓鎶藉级閳轰焦鍠橀柡?
        std::string file_path = output_dir + "/total_force_data_" + oss.str() + ".txt";

        // 闂佸憡甯楃粙鎴犵磽閹捐崵宓侀柤鎼佹涧閳數鈧鍠掗崑鎾绘煛閸屾碍鐭楁繛鍡愬灲閺佸秹宕奸敐搴㈣埞闂佺儵鏅滈悧妤勫暞閻庢鍠栨蹇曟?
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            std::cerr << "闂佸搫鍟版慨鐢垫兜閸洖绠ラ柟鎯х－绾惧寮堕崼鐔稿碍闁搞値鍙冨顒勫炊閿旂瓔鍋? " << file_path << std::endl;
            return;
        }

        // 闂備緡鍓欑粔鏉戭啅缁櫞gle_total_force闂佹眹鍔岀€氫即寮銏犵９闁绘挸瀵掗崵鐘绘煥濞戞ɑ绶無ol_angle闂佸憡绮岄懟顖烆敋椤旇姤鍎熼柡鍐ㄥ€归悾閬嶆煕濮橆剛澧曞┑顔肩箻閺?
        for (const auto& pair : angle_total_force) {
            double tool_angle = pair.first;
            const GLVertex& total_force = pair.second;

            // 闂佸憡鍔栭悷銉╁矗閸℃稒鏅慨婵堫殞ol_angle闂侀潧妫旂花娑㈡煏閸℃洜鐨介梺闈涙缁ㄦ繈鏌涢幒鎴烆棦闁革絾妞介弫宥夊醇閵忊剝娈㈤梺鍛婂笚閸庡ジ濡撮崘顏嗙當闁挎洍鍋撻柛銊ラ叄濮婇箖寮幘鍓侇槴
            out_file << tool_angle << "\t"
                << total_force.x << "\t"
                << total_force.y << "\t"
                << total_force.z << std::endl;
        }

        out_file.close();
        std::cout << "total_force闂佽桨鑳舵晶妤€鐣垫担鍓插晠闁肩厧澧庣紙濠氭煕閹达妇绱伴柛? " << file_path << std::endl;

        //濠电偞鎸搁幊妯衡枍鎼淬劌鏋侀柣妤€鐗嗙粊?
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
        // 濠碘槅鍋€閸嬫捇鏌″畝濠冾仯ibration_values闂佸搫瀚烽崹浼村箚娴ｅ湱鈻旈柧蹇撳帨閺?
        if (vibration_values.empty()) {
            std::cout << "Warning: vibration_values is empty, using default parameters" << std::endl;

            // 婵炶揪缍€濞夋洟寮妶鍡╂付婵☆垱顑欓崥鍥煕濞嗗繐鈧綊寮抽悢鐓庣婵犻潧妫妤呮煕?
            vibr_k_eff.resize(1);
            vibr_stock_c.resize(1);

            // 闁荤姳绀佹晶浠嬫偪閸℃﹩娓舵俊顖涱儥閸氬洭鏌?
            vibr_k_eff[0] = 1.0;
            vibr_stock_c[0] = 2 * stock_vibr_damping_ratio * 1.0;

            // 闁荤姳绀佹晶浠嬫偪閸℃﹩娓舵俊顖涱儥閸氬洭鏌ｉ妸銉ユewmark闂佸憡鐟ラ崐褰掑汲?
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

        // 婵烇絽娴傞崰妤呭极閸忚偐鈻旈柛婵嗗閸炪劌霉濠婂啫顒㈤懚鈺呮煙椤戣儻鍏屾繛鍫熷灴瀹曪綁宕掑☉娆愵啀闁荤姳绶ょ槐鏇㈡偩?
        vibr_k_eff.resize(vibration_values.size());
        vibr_stock_c.resize(vibration_values.size());
        for (size_t mode = 0; mode < vibration_values.size(); ++mode) {
            double lambda = vibration_values[mode]; // 闂佺粯顨堥幊鎾舵濞戙垹纾?
            double omega = sqrt(lambda);           // 闂佹悶鍎抽崕銈咃耿娴ｇ櫢绱ｉ柟瀵稿Т閼?
            vibr_stock_c[mode] = 2 * stock_vibr_damping_ratio * omega;   // 濠碘槅鍨崜婵嬪焵椤戣法绐旀俊顖氭娴?

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
            //vibr_k_eff[mode] = lambda + vibr_a0 * 1.0 + vibr_a1 * vibr_stock_c[mode]; // 闁荤姵鍔戦崝鎴﹀闯濞差亝鍎楅柍鍝勬噺閳诲牓鏌涘▎鎯疯偐鎷?
        }
    }

    void milling_AptCutterVolume::initstockVibration() {
        // 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥冨妼閻╀線鏌涢弬璇插鐎殿噮鍓熷顐﹀级鐠恒劍鎲奸梺绋胯閸斿海鍒掗妸鈺佸嚑闁告帗鍔曡灒闁斥晛鍟犻崑鎾寸▕?
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
        // 闂佸吋鍎抽崲鑼躲亹閸ヮ剚鍤婃い蹇撴閺嗙娀骞栨潏楣冩闁哄棛鍠栭弻宀冪疀閹炬潙顏┑鈽嗗灙閸撴繈鍩€椤戣法鍔嶉柡鍡欏枛閺?
        const size_t num_dofs = temp_vibration_vectors[0].size();
        const size_t num_modes = temp_vibration_vectors.size();

        // 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥冨妼閻╀線鏌涢弬璇插鐎殿噮鍓熷顐﹀级鐠恒劍鎲奸梺绋胯閸斿海鍒掗妸鈺佸嚑闁告帗鍔曡灒闁斥晛鍟犻崑鎾寸▕?
        vibration_q.resize(num_modes);
        vibration_dq.resize(num_modes);
        vibration_ddq.resize(num_modes);
        next_vibration_q.resize(num_modes);
        next_vibration_dq.resize(num_modes);
        next_vibration_ddq.resize(num_modes);

        // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鐎ｅ棔绶氶獮鈧?
        for (size_t mode = 0; mode < num_modes; ++mode) {
            double f_total = 0.0;
            double f_eff_total = 0.0;

            // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鎼粹剝顔囬梺姹囧灩瀹曨剛鈧濞婇悰顕€宕滄担鐑樼劸闂佸憡姊绘刊瀵告濞嗘挸缁╅梺顐ｇ缁€瀣⒑椤掆偓缁夋潙顔忕猾顧磄le_force_map婵炴垶鎼╅崢鎯р枔閹达箑绠ラ柍褜鍓熷鍨緞婵犲偆娼濋梺杞拌兌婢ф鐣垫笟鈧弫?
            for (size_t dof = 0; dof < num_dofs; ++dof) {
                // 闂備緡鍓欑粔鏉戭啅缁櫞gle_force_map婵炴垶鎼╅崢鎯р枔閹存粳鎺曠疀鎼淬劌娈漛lade_num闁诲海鏁搁幊鎾惰姳閺屻儲鍎嶉柛鎺濇磯rce_map
                for (const auto& [blade_num, inner_force_map] : angle_force_map) {
                    // 闂備緡鍓欑粔鏉戭啅閺勫繈浜归柟鎯у暱椤ゅ崑lade_num闁诲海鏁搁幊鎾惰姳閺屻儲鍎嶉柛鏇ㄥ墮椤ｆ煡鏌￠崼婵愭Ц濠殿喖绻樺顐︽偋閸繄銈?
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

            // 闁荤姳绶ょ槐鏇㈡偩閼姐倗椹冲璺侯儐濞呭繘鏌?
            f_eff_total = f_total + (vibr_a0 * vibration_q[mode] +
                vibr_a2 * vibration_dq[mode] +
                vibr_a3 * vibration_ddq[mode])
                + vibr_stock_c[mode] * (vibr_a1 * vibration_q[mode] +
                    vibr_a4 * vibration_dq[mode] +
                    vibr_a5 * vibration_ddq[mode]);
            // 闁荤姳绶ょ槐鏇㈡偩閼姐倗椹冲璺侯儐濞呭繘鏌?
            f_eff_total = f_total;

            double relaxation_factor = 0.7; // 闂佸搫顦伴崕宕囨闁秴鐐婇柣妯垮皺閹藉秹鏌?-1婵炴垶鏌ㄩ澶娢?

            // 闂佸搫娲ら悺銊╁蓟婵犲洤绠版い鏍ㄨ壘琚熼梺鍛婄懃閸婂綊寮抽悢鍏兼櫖闁割偅绻勯幗鐘绘煕鐏炶濡奸柟顔兼喘閹虫盯顢旈崱妯绘闁硅壈鎻紓姘辩不閿濆妞界€光偓鐎ｎ剛顦?
            vibration_q[mode] = next_vibration_q[mode];
            next_vibration_q[mode] = vibration_q[mode] * (1 - relaxation_factor) +
                (f_eff_total / vibr_k_eff[mode]) * relaxation_factor;
            next_vibration_ddq[mode] = vibr_a0 * (next_vibration_q[mode] - vibration_q[mode])
                - vibr_a2 * vibration_dq[mode]
                - vibr_a3 * vibration_ddq[mode];
            next_vibration_dq[mode] = vibration_dq[mode]
                + vibr_a6 * vibration_ddq[mode]
                + vibr_a7 * next_vibration_ddq[mode];

            // 濠电儑缍€椤曆勬叏閻愬瓨缍囬柛锔诲幗濞呮洟姊婚崟顒€濮囬柛?
            next_vibration_q[mode] = std::min(next_vibration_q[mode], deform_color_max);

            std::cerr << "Mode:" << mode
                << " f_eff=" << f_eff_total
                << " vibr_k_eff=" << vibr_k_eff[mode]
                << " vibration_q=" << next_vibration_q[mode]
                << std::endl;
        }
        temp_vibration_vectors = vibration_vectors;//闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥囨暩缁€澶愭⒑椤掆偓閻忔繈宕㈤妶澶婄闁绘ê鐏氶弳蹇涙煕閹邦剚鍣归柣?
    }

    void milling_AptCutterVolume::outputSurfaceData(const std::string & output_dir) {
        // 濠碘槅鍋€閸嬫捇鏌″畝濠冪彺ap闂佸搫瀚烽崹浼村箚娴ｅ湱鈻旈柧蹇撳帨閺?
        if (surface_map.empty()) {
            std::cout << "surface_map is empty, skip export." << std::endl;
            return;
        }

        int success_files = 0;
        int total_points = 0;

        // 闂備緡鍓欑粔鏉戭啅缁浛p闂佹寧绋戞總鏃傛嫻閻旀哺鎺曠疀鎼淬劌娈濋梻浣诡儥閸犳牠宕归崡鐑嗗殘閺夌偞澹嗛鍗炩槈閹垮啩绨奸柡瀣暞缁?
        for (const auto& [key, vertices] : surface_map) {
            // 婵犵鈧啿鈧綊鎮樻径瀣氦婵炲棗閰ｉ崵瀣⒑濞嗘儳鏋涙い鏇ㄥ枟閹棃寮崒娑氭殸vector婵炴垶鎹佸▍锝夊煘閺嶎厽鏅€光偓閸愵亜鍔滈柡?
            if (vertices.empty()) {
                continue;
            }

            // 闂佸搫顑呯€氫即鍩€椤掑倸鞋缂侀鍙冨畷娆撴倻濡崵鈧喖霉閻樺啿鍔堕柣顓熷劤椤?
            std::string file_path = output_dir + "/" + std::to_string(key) + ".txt";

            // 闂佺懓鐏氶幐鍝ユ閹达箑妫橀柛銉檮椤?
            std::ofstream out_file(file_path);
            if (!out_file.is_open()) {
                std::cerr << "闂佸搫鍟版慨鐢垫兜閸洖绠ラ柟鎯х－绾惧鏌￠崒姘煑婵? " << file_path << std::endl;
                continue;
            }

            // 闁荤姳绀佹晶浠嬫偪閸℃ɑ缍囬柟鎯у暱濮ｅ绱掗钘夊姢閻?
            out_file << std::fixed << std::setprecision(15);

            // 闂佸憡鍔栭悷銉╁矗閸℃瑦瀚氶柕澶嗘櫆閺嗘盯鎮楅悽鍨殌缂併劍鐓￠幆鍐礋椤掆偓椤ｆ煡鏌￠崼婵愭█闁靛棗锕幃?
            for (const auto& vertex : vertices) {
                out_file << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
            }

            out_file.close();

            success_files++;
            total_points += vertices.size();

            std::cout << "surface file exported: " << file_path
                << " (points: " << vertices.size() << ")" << std::endl;
        }

        std::cout << "surface export finished: files=" << success_files
            << " points=" << total_points << std::endl;

        surface_map.clear();
    }

    void milling_AptCutterVolume::outputVibrationData(const std::string & output_file) {
        // 闂佹眹鍨婚崰鎰板垂濮樿鲸鏆滈柨鏃囧Г椤ρ囨⒒閸屾氨鎽犻柛鈺傚灴閹啴宕熼浣衡偓顔济归悩鐑樼【闁?
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
            std::cout << "闂佸憡甯楃粙鎴犵磽閹捐绠版い鏍ㄨ壘琚熼梺杞拌兌婢ф鐣垫笟鈧顒勫炊閿旂瓔鍋? " << timestamped_file << std::endl;
        }

        // 闂佺懓鐏氶幐鍝ユ閹寸偞缍囬柟鎯у暱濮ｅ鏌￠崒姘煑婵?
        std::ofstream out_file(timestamped_file, std::ios::app); // 婵炶揪缍€濞夋洟寮妶鍡樹氦闁芥ê顦～锝嗕繆椤栨せ鍋撳畷鍥╊攨
        if (!out_file.is_open()) {
            std::cerr << "闂佸搫鍟版慨鐢垫兜閸洖绠ラ柟鎯х－绾惧鏌￠崒姘煑婵? " << timestamped_file << std::endl;
            return;
        }

        // 闁荤姳绀佹晶浠嬫偪閸℃ɑ缍囬柟鎯у暱濮ｅ绱掗钘夊姢閻?
        out_file << std::fixed << std::setprecision(15);

        // 闂佸憡鍔栭悷銉╁矗閸炴ol_angle
        out_file << tool_angle;

        // 闂佸憡鍔栭悷銉╁矗閸℃稑绠ラ柍褜鍓熷鍨箙缁夊€則_vibration_q[mode]
        for (size_t mode = 0; mode < next_vibration_q.size(); ++mode) {
            out_file << " " << next_vibration_q[mode];
        }

        // 闂佺懓绠嶉崹濂搞€?
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
        //闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥庡墰缁犮儵鏌涢弬璇插妞ゆ帞鍠栧畷锝夊磼濞戞瑦顔?
        q_x[0.0] = 0.0;
        dot_q_x[0.0] = 0.0;
        ddot_q_x[0.0] = 0.0;
        q_y[0.0] = 0.0;
        dot_q_y[0.0] = 0.0;
        ddot_q_y[0.0] = 0.0;
        calcBB();
    }

    // 缂傚倷鐒﹀鎶藉级閻愯尙鎽犳俊顐㈡濞碱亪顢楅崒姘寲
    GLVertex rotate_x(const GLVertex & p, double angle) {
        double cos_theta = cos(angle);
        double sin_theta = sin(angle);
        GLVertex rotated;
        rotated.x = p.x;
        rotated.y = static_cast<GLfloat>(p.y * cos_theta - p.z * sin_theta);
        rotated.z = static_cast<GLfloat>(p.y * sin_theta + p.z * cos_theta);
        return rotated;
    }

    // 缂傚倷鐒﹀鎾级閻愯尙鎽犳俊顐㈡濞碱亪顢楅崒姘寲
    GLVertex rotate_y(const GLVertex & p, double angle) {
        double cos_theta = cos(angle);
        double sin_theta = sin(angle);
        GLVertex rotated;
        rotated.x = static_cast<GLfloat>(p.x * cos_theta + p.z * sin_theta);
        rotated.y = p.y;
        rotated.z = static_cast<GLfloat>(-p.x * sin_theta + p.z * cos_theta);
        return rotated;
    }

    // 缂傚倷鐒﹀鐑藉级閻愯尙鎽犳俊顐㈡濞碱亪顢楅崒姘寲
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

        // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绫嶉悗锝庡幗缁侇噣鏌涘顒佹崳婵炲牊鍨跺蹇涘捶椤撶喐鐝梺?
        double max_x = std::numeric_limits<double>::lowest();
        double max_y = std::numeric_limits<double>::lowest();
        double max_z = std::numeric_limits<double>::lowest();
        double min_x = std::numeric_limits<double>::max();
        double min_y = std::numeric_limits<double>::max();
        double min_z = std::numeric_limits<double>::max();

        // 闂佸吋婢橀崯鍐差瀶婵犳艾绠ラ柍褜鍓熷鍨緞婵犲倻鈧剟鏌涜箛娑欐暠妞ゆ柨鐭傞幆鍐礋椤掍椒绮繝銏犵垻閸愩劎锛欓悗鍨緲鐎氼剟骞忛惃鏇㈡煠閻撳骸鏆欐繛?
        double max_radius = 0.0;
        for (const auto& seg : segments) {
            max_radius = std::max(max_radius, std::max(seg.radius1, seg.radius2));
            min_z = std::min(min_z, seg.z_start);
            max_z = std::max(max_z, seg.z_end);
        }

        // 闂佹眹鍨婚崰鎰板垂濮橆厽缍囬柛锔诲幗濞呮洟鏌ｉ埡鍌氱婵?婵炴垶鎼╂禍顏堝Υ婵犲洦鍊?
        std::vector<GLVertex> vertices;
        vertices.emplace_back(max_radius, max_radius, min_z);
        vertices.emplace_back(max_radius, max_radius, max_z);
        vertices.emplace_back(max_radius, -max_radius, min_z);
        vertices.emplace_back(max_radius, -max_radius, max_z);
        vertices.emplace_back(-max_radius, max_radius, min_z);
        vertices.emplace_back(-max_radius, max_radius, max_z);
        vertices.emplace_back(-max_radius, -max_radius, min_z);
        vertices.emplace_back(-max_radius, -max_radius, max_z);

        // 闂佸搫鍟鍫澝归崱娑樼闁逞屽墴瀵灚寰勯幇鈹惧亾婵犲洦鍊烽柣鐔稿鐎氭瑩鏌熼崹顐㈠姢闁糕晛鐭傚鍛婃媴缁洖浜?
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

        // 濠电儑缍€椤曆勬叏閻愬搫绀嗛柍褜鍓熷畷妤呮憥閸屾繂骞€闂婎偄娲ら崯顐ｇ閸濄儳鐭?
        max_x += center.x + TOLERANCE;
        max_y += center.y + TOLERANCE;
        max_z += center.z + TOLERANCE;
        min_x += center.x - TOLERANCE;
        min_y += center.y - TOLERANCE;
        min_z += center.z - TOLERANCE;

        // 闂佸搫娲ら悺銊╁蓟婵犲啯缍囬柛锔诲幗濞呮洟鏌?
        //std::cout << "[DEBUG] Bounding Box - max_x: " << max_x << ", max_y: " << max_y << ", max_z: " << max_z << std::endl;
        //std::cout << "[DEBUG] Bounding Box - min_x: " << min_x << ", min_y: " << min_y << ", min_z: " << min_z << std::endl;
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
        // 婵炴垶鎸哥粔鏉戯耿椤忓懐顩烽悹浣哥－缁夊潡鏌涢幒鎴烆棡妞ゆ柨鐭傚畷姗€宕崘顏嗩槷闁哄鏅滈弻銊ッ洪弽顐ｅ闁绘柨鐨濋崑鎾舵兜妞嬪海顦╂繝銏ｅ煐閻楃娀宕曢幘顔芥櫖?
        return t.z;
    }

    Cutting digitaltwin_AptCutterVolume::dist_cd(const GLVertex & p) const {
        Cutting result = { 0.0, NO_COLLISION, 1 };
        result.f = dist(p);
        // 闂佸憡鐟崹閬嶆偋閹绢喖绠叉い鏇楀亾婵炴挸澧庨幉鐗堟媴閻熸壋鎸呴柣蹇曞仦濞叉粓锝為锕€绠婚柣鎰祷椤箓鏌涢妸銉剰闁搞劎鏅埀?
        return result;
    }

    std::vector<std::vector<std::vector<double>>>
        digitaltwin_AptCutterVolume::convertTo3DVibrationVectors(const std::vector<std::vector<double>>&pre_vibration_vectors,
            int num_dofs,
            const std::vector<size_t>&modes_to_read) {  // 婵烇絽娴傞崰妤呭极婵傜鐭楅柛灞剧⊕濞堣泛鈽夐幘璺哄妺閼垛晠鏌熼鑳厡闁稿被鍔岄锝夊即閻愯尙浠氶柣?
        if (pre_vibration_vectors.empty()) {
            return {};
        }

        // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕闁哄棛鍠栭獮鎴︻敊閼测晜娈梺杞扮閻°劑鍩€?
        size_t total_dofs = pre_vibration_vectors[0].size();
        if (total_dofs % num_dofs != 0) {
            throw std::invalid_argument("total dofs is not divisible by node dofs");
        }

        // 闂佸憡甯楃粙鎴犵磽閹惧鈻斿璺烘湰濡﹪鏌涘顓炵伌闁革絾妞介弫宥夊醇閻斿搫顥戦梺纭咁嚃閸犳鈧灚姘ㄩ埀顒冾潐绾板秷鍟梺璇″厸閻掞箓寮抽悢鍏肩厒闊洢鍎崇粈?
        size_t num_nodes = total_dofs / num_dofs;
        std::vector<std::vector<std::vector<double>>> vibration_vectors(
            modes_to_read.size(),  // 闂佸搫绉烽～澶婄暤娓氣偓楠炴劙宕惰閺嗙増淇婇妤€澧查柍褜鍏涢悞锕傚汲閻斿吋鐓傞煫鍥ㄦ尭閻忥紕鈧?
            std::vector<std::vector<double>>(num_dofs, std::vector<double>(num_nodes))
        );

        // 闁哄鍎愰崜姘暦閺屻儱绠伴柛銉戝懏姣庡┑鈽嗗灙閸撴繈鍩€椤戣法鍔嶉柡鍡欏枛楠?
        for (size_t i = 0; i < modes_to_read.size(); ++i) {
            size_t mode = modes_to_read[i];
            if (mode >= pre_vibration_vectors.size()) {
                throw std::invalid_argument("requested mode index is out of range");
            }

            // 濠碘槅鍋€閸嬫捇鏌＄仦璇插姕閼垛晠鏌熼璺ㄥ妽闁哄棛鍠栭獮鎴︻敊閼姐値浼囬梺鐓庡槻閻°劑鍩€?
            if (pre_vibration_vectors[mode].size() != total_dofs) {
                throw std::invalid_argument("mode vector length is inconsistent");
            }

            // 婵犻潧顦介崑鍕储閺嶎厼鏋侀柣妤€鐗嗙粊?
            for (int dof = 0; dof < num_dofs; ++dof) {
                for (size_t node = 0; node < num_nodes; ++node) {
                    size_t index = node * num_dofs + dof;
                    vibration_vectors[i][dof][node] = pre_vibration_vectors[mode][index];
                }
            }
        }

        return vibration_vectors;
    }

    // 婵烇絽娲︾换鍕汲閳ь剟鏌涘Ο鐓庢瀻闁搞倝浜跺顐﹀箥椤旇姤娈㈡繛瀛樼矊妤犳悂骞冨鍫濊Е閹肩补鈧櫕鍊柣?
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


    //闁荤姴娲╅褑銇愰崶顒€绀嗛柛銉戝喚鏉搁梺鍛婂笒閸熶即宕戦敐澶嬧挅?
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
        // 闁荤姳绶ょ槐鏇㈡偩缂佹鈻斿璺侯樀閸ゅ寮堕埡鍐ㄤ户闁汇垹顭烽幆鍐礋椤斿墽褰鹃梺鍛婄閸ㄥ潡宕冲ú顏勭煑濠㈣泛鐫楀┑鍫㈢當闁挎洍鍋撶憸?
        GLVertex ab = b - a;
        GLVertex bc = c - b;
        GLVertex cd = d - c;
        GLVertex da = a - d;

        // 闁荤姳绶ょ槐鏇㈡偩鐠囨祴鏋栭柡鍥╁Т濞堢娀鏌￠崒婊勫殌闁诡喗绮撻幆鍐礆韫囨稑绀嗛柛鈩冪☉濞?
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

        // 婵炶揪缍€濞夋洟寮妶鍥ㄥ仒闁靛ň鏅滃銊モ槈閹垮啫骞栫紒娲畺閹瑩鎮烽悧鍫濐伅闂佸憡宸婚崑鎾绘煕韫囨挸妲荤€殿噮鍓熷顐﹀箯瀹€濠傛倎缂備胶濮甸〃鍛村垂韫囨稑绀堝┑鐘插€归崐銈夋煕閺冣偓缁嬫帡寮婚悢鐓庤Е?
        // 闂佺顑呭ú鈺咁敊閺呭増rfaceCenter閻庤鐡曠亸娆戝垝閿熺姵鐒绘慨妯虹－缁犳牕菐閸ワ絺鍋撻崘鎻掆枏婵炵鍋愭繛鈧柍褜鍓氱敮鎺楀春瀹€鍐︿汗闁规儳鍟块·鍛存倵閻㈡鏀伴柦鏍у缁?

        // 闂佸吋鍎抽崲鑼躲亹閸ヮ剙绀嗛柍褜鍓熷畷妤呮憥閸屾繂骞€闂婎偄娲ら崯浼村磻閿濆妞介悘鐐靛亾椤ュ寮堕悜鍡楀⒉妞ゎ偅顨嗛幆?
        GLVertex cutter_center = this->center;
        GLVertex cutter_angle = this->angle;

        // 闁荤姳绶ょ槐鏇㈡偩缂佹顩烽幖绮光偓宕団偓顒勬煕韫囨碍鑵归柤鍨灱缁犳盯宕橀妸銉у帓闁荤偞绋忛崝鎴濐焽閻楀牏鈻旀い鎾跺仧婵″洭鏌ｉ妸銉ヮ仾闁哄瞼鍠栧畷銉╁箣濠靛洤鈧姊?
        double dx = surfaceCenter.x - cutter_center.x;
        double dy = surfaceCenter.y - cutter_center.y;
        double dz = surfaceCenter.z - cutter_center.z;

        // 闁荤姳绶ょ槐鏇㈡偩閼姐倖宕夋繝闈涚墱閻?
        double distance = sqrt(dx * dx + dy * dy + dz * dz);

        if (distance > 0) {
            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宥冨妽閻撴瑩鏌涘顓炵仴闁诡喗绮撻弻?
            double dir_x = dx / distance;
            double dir_y = dy / distance;
            double dir_z = dz / distance;

            // 闂佸吋婢橀崯鍐差瀶婵犳艾绀嗛柍褜鍓熷畷妤呮煥鐎ｎ偒妫嗛柡澶屽剱閸撴繈锝炲Δ浣瑰劅闁挎梻鍋撻悾杈╂喐閺夊灝鑸归柟?
            // 缂傚倷鐒﹀鎶藉级閻愯尙鎽犳俊顐㈡濞碱亪顢楅崒婊冨絹闂?
            double cos_ax = cos(cutter_angle.x);
            double sin_ax = sin(cutter_angle.x);
            // 缂傚倷鐒﹀鎾级閻愯尙鎽犳俊顐㈡濞碱亪顢楅崒婊冨絹闂?
            double cos_ay = cos(cutter_angle.y);
            double sin_ay = sin(cutter_angle.y);

            // 闁圭厧鐡ㄥ濠氬极閵堝绫嶉悗锝庡幗缁侇噣鏌涘▎鎯挎垵鐣烽弻銉ョ闁绘鐗婇悡娆撴煕濮橆厼鐏ラ柟顔界矒閺?
            double rotated_y = dir_y * cos_ay - dir_z * sin_ay;
            double rotated_z = dir_y * sin_ay + dir_z * cos_ay;

            double rotated_x = dir_x * cos_ax + rotated_z * sin_ax;
            double final_z = -dir_x * sin_ax + rotated_z * cos_ax;

            // 闂佸搫娲ら悺銊╁蓟婵犲洤绫嶉悗锝庡幗缁侇噣鏌涘顒佹崳婵炲牊鍨垮顒勬偡閻楀牆鈧鏌涘顓炵伌闁?
            dir_x = rotated_x;
            dir_y = rotated_y;
            dir_z = final_z;

            // 闂備焦褰冪粔鐢稿蓟婵犲懌浜归柟鍝勭Ф椤忛亶鏌?
            double norm = sqrt(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
            if (norm > 0) {
                dir_x /= norm;
                dir_y /= norm;
                dir_z /= norm;
            }

            // force_data闂佸憡鐗曢幊搴ㄥ箚閸儲鐓ｉ柨婵嗘噹閻庮剟鏌ｉ妸銉ヮ仼闁搞劌绻樺畷銉╁箣濠靛棭娼濋梺闈涙缁€浣烘閻愬搫瑙﹂柟瀛樼箓椤棃鏌涘鍐劮闂佷即浜跺畷銉╁箣濠靛棭娼?
            // force_data[0]: 闂佸憡甯掑ú銈夊箖濠婂牆绀?(tangential force)
            // force_data[1]: 閻庡灚婢樼€氼剟骞冨鍫濈?(radial force)  
            // force_data[2]: 闁哄鍋炲娆撳箖濠婂牆绀?(axial force)

            // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柍褜鍓熷畷妤€鐣￠悧鍫㈢▌闂佸搫绉村ú銊╂焾鐎涙鈻旈悗锝庡墯閻ｉ亶鏌涢弮鈧粙鎴﹀箖濠婂牊鐓?
            // 闂侀潻璐熼崝宀勫垂娓氣偓瀹曟鐣￠悧鍫㈢▌闂佸搫绉村ú銊╂焾鐎涙鈻旀い鎾卞妿缁?
            // 闂佸憡甯掑ú銈夊箖濠婂牆绀夋繛鎴濈－缁愭鏌涢妸銉モ偓璇裁洪幐搴ｎ洸閹肩补鈧磭鈧剟鏌涜箛鏇熺効闂佹媽浜惀顏堝礌閿涘嫮顦┑鐐茶嫰閻忔繈宕硅箛娑樼濠电姴鍟悡娆撴煕?
            // 閻庡灚婢樼€氼剟骞冨鍫濈婵炴垵纾粣妤呮煕閵娿儱鈧煤閹稿海顩查幖绮光偓宕団偓顒勬煕韫囨洘鐒块梺鎷屼含閻ヮ亪宕犻敍鍕槷闂佸湱顭堝ú銈夊箖濠婂牆绀嗛柍褜鍓熷畷妤呮憥閸屾繂骞€闂?
            // 闁哄鍋炲娆撳箖濠婂牆绀夋繛鎴濈－缁愭绻涚仦鐣屼虎闁搞劋绶氬畷妤呮偪椤栫偛褰欑紓浣哄亾鐎笛囧蓟閻旂厧瑙?

            // 婵☆偓绲鹃悧鏇㈠储濞戞矮鐒婇柛鈩冾殔椤柨霉閻樻煡顎楅柛銊ょ窔瀹曟鐣￠悧鍫㈢▌闂佸搫绉村ú銊╂焾鐎涙ɑ濮滄い鎺嶇鎼村﹪鏌涢幒妤婃殥缂佹锕㈤幃鍓т沪閼恒儳绋勯梺鍝勭Т濞层劑鏌?
            // 闂佺硶鏅炲銊ц姳椤掑倹鍋橀柕濞炬櫆濡椼劌鈽夐幙鍐ㄥ箹缂佹椽绠栭幃娆戞喆閸曨剛鍘甸梺鍛婄閸ㄥ綊鍨惧Ο灏栧亾鐟欏嫯澹樺┑顔肩箻閹啴宕熼銏⑩偓濠氭偡?

            // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柍褜鍓熷畷妤呮偪椤栫偛褰欑紓浣哄亾鐎笛囧蓟閻旂厧瑙﹂柟鎼灣缁€鍕煕鐎ｎ亝绌挎い鏃€妾烽柡澶屽仧缁绘繄鎷归悢鐓庣闁逞屽墴瀹曟鎮块鐐插綑缂備焦鍎崇亸鍛?
            double tool_axis_x = 0.0;
            double tool_axis_y = 0.0;
            double tool_axis_z = 1.0;

            // 闁圭厧鐡ㄥ濠氬极閵堝绀嗛柍褜鍓熷畷妤呮煥鐎ｎ偒妫嗛柡澶屽剱閸撴繈锝炲Δ浣瑰劅闁挎洍鍋撻柛鈺佺焸瀹曟岸鍩€椤掑嫬绀傞柣銈庡灦閸欙紕绱掗幆褍鐨＄紒杈ㄧ懄閹峰懐鎹勯妸锔芥闂佸憡鎸哥粔鐟邦焽閺夋鍟呴柤纰卞墰閻ュ懘鎮楃憴鍕叝缂佺姷鍠栭幆鍐礋椤愩垻绉梻浣瑰絻妤犲繒妲?

            // 闂佸搫鍟鍫澝归崱娑樼闁逞屽墴瀹曟鎮块鐐插綑缂?
            double rotated_axis_y = tool_axis_y * cos_ay - tool_axis_z * sin_ay;
            double rotated_axis_z = tool_axis_y * sin_ay + tool_axis_z * cos_ay;
            double rotated_axis_x = tool_axis_x * cos_ax + rotated_axis_z * sin_ax;
            double final_axis_z = -tool_axis_x * sin_ax + rotated_axis_z * cos_ax;

            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓啰鈧剟鏌涜箛鏇熺効闂佹媽浜惀?
            double axis_norm = sqrt(rotated_axis_x * rotated_axis_x + rotated_axis_y * rotated_axis_y + final_axis_z * final_axis_z);
            if (axis_norm > 0) {
                rotated_axis_x /= axis_norm;
                rotated_axis_y /= axis_norm;
                final_axis_z /= axis_norm;
            }

            // 闁荤姳绶ょ槐鏇㈡偩婵犳艾绀嗛柛銉戝嫬鈧鏌￠崒婊勫殌闁诡喗绮撻弫宥夊醇濠靛牃鍋撻銏″剮缂傚牏濮烽懝楣冩煕閹哄棗浜鹃梺绋跨箳閺屽鏌婃潏鈺冩／闁告牭绱曠粈澶愭煙缁嬫寧鎼愰柟顔界矌閹即濡搁埡鍌涖€冩繛鎴炴惄閸樿偐缂撴ィ鍐╁€烽悷娆忓閻撴瑩鏌涘顓炵仴婵犫偓椤忓牆鍨傞柛灞剧矋缁绢垰霉濠婂啯鍞夐梺鎷屼含閻ヮ亪宕归鈧幐顒勬⒒閸偅绶氱紒妤€鍊块幆鍐礋椤掆偓椤瞼鎲搁悧鍫濇暰缂?
            double projection_dot = dir_x * rotated_axis_x + dir_y * rotated_axis_y + dir_z * final_axis_z;
            double proj_x = dir_x - projection_dot * rotated_axis_x;
            double proj_y = dir_y - projection_dot * rotated_axis_y;
            double proj_z = dir_z - projection_dot * final_axis_z;

            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓啰鈧ジ鏌涘顓炵仸闁哄瞼鍠栧畷?
            double tangential_norm = sqrt(proj_x * proj_x + proj_y * proj_y + proj_z * proj_z);
            if (tangential_norm > 0) {
                proj_x /= tangential_norm;
                proj_y /= tangential_norm;
                proj_z /= tangential_norm;
            }

            // 闁荤姳绶ょ槐鏇㈡偩鐠囧樊鍤楅柛鏇ㄥ亝閸婂鏌￠崒婊勫殌闁诡喗绮撻弫宥夊醇濠靛牃鍋撻銏″剮缂傚牏濮烽懝楣冩煕閹哄棗浜鹃梺绋跨箳閺屽鏌婃潏鈺冩／闁割煈鍠楃€氭煡鏌涢幒鎴炴悙闁诡喗绮撳顒勬偡閻楀牆鈧鏌?
            double radial_x = rotated_axis_y * proj_z - rotated_axis_z * proj_y;
            double radial_y = rotated_axis_z * proj_x - rotated_axis_x * proj_z;
            double radial_z = rotated_axis_x * proj_y - rotated_axis_y * proj_x;

            // 閻熸粎澧楃敮濠勭博閹绢喖绀岄柡宓懐鐛梺鍛婄閸ㄥ爼寮婚悢鐓庤Е?
            double radial_norm = sqrt(radial_x * radial_x + radial_y * radial_y + radial_z * radial_z);
            if (radial_norm > 0) {
                radial_x /= radial_norm;
                radial_y /= radial_norm;
                radial_z /= radial_norm;
            }

            // 闁诲繐绻愬Λ妤呭垂韫囨稑瑙﹂柟瀛樼箓椤棃鏌曢崱鏇狀槮缂佸墎鍋ゅ畷銉╁箣濠靛棭娼濋梺鍛婄矊閻線鏌婇柆宥呰Е闁瑰瓨绻傞～鏃堟煕閹烘垶顥＄悮娆撴煕閹烘鏆掔紒妤侊耿閹墽浠﹂懞銉х▌闂佸搫绉村ú銊╂焾?
            force_xyz[0] = force_data[0] * proj_x + force_data[1] * radial_x + force_data[2] * rotated_axis_x;  // Fx
            force_xyz[1] = force_data[0] * proj_y + force_data[1] * radial_y + force_data[2] * rotated_axis_y;  // Fy
            force_xyz[2] = force_data[0] * proj_z + force_data[1] * radial_z + force_data[2] * final_axis_z;    // Fz

            // 闁哄鐗婇幐鎼佸吹椤撶姵瀚柛鎰靛幘濡茬菐閸ワ絽澧插ù?
            //qDebug() << "Force Position: (" << surfaceCenter.x << ", " << surfaceCenter.y << ", " << surfaceCenter.z << ")";
            //qDebug() << "Force Direction: (" << dir_x << ", " << dir_y << ", " << dir_z << ")";
            //qDebug() << "Total Force: (" << force_xyz[0] << ", " << force_xyz[1] << ", " << force_xyz[2] << ")";
            //qDebug() << "Tangential Direction: (" << proj_x << ", " << proj_y << ", " << proj_z << ")";
            //qDebug() << "Radial Direction: (" << radial_x << ", " << radial_y << ", " << radial_z << ")";
            //qDebug() << "Tool Axis Direction: (" << rotated_axis_x << ", " << rotated_axis_y << ", " << final_axis_z << ")";



        }
        else {
            // 閻熸粎澧楅幐鎼佸垂娓氣偓瀹曟鎽庨崒婵嗗箑闂婎偄娲ら崯鈺冪箔瀹€鈧幃浼村Ω閳哄倹銆冩繛鎴炴惄閸樿偐缂撴ィ鍐╃厒鐎广儱鎳忛崐銈夋煛閸愵収鍟囩紒杈ㄧ箖閹峰懐鎹勯妸锔芥婵帗绋掗…鍫ヮ敇婵犳艾绀夋繛鎴炵懄閻撴瑩鏌?
            force_xyz[0] = force_data[0];
            force_xyz[1] = force_data[1];
            force_xyz[2] = force_data[2];

            qDebug() << "Warning: Cutter center coincides with surface center, using default force direction";
            qDebug() << "Default Force: (" << force_xyz[0] << ", " << force_xyz[1] << ", " << force_xyz[2] << ")";
        }

    }

    void digitaltwin_AptCutterVolume::updatestockVibrParams() {
        // 濠碘槅鍋€閸嬫捇鏌″畝濠冾仯ibration_values闂佸搫瀚烽崹浼村箚娴ｅ湱鈻旈柧蹇撳帨閺?
        if (vibration_values.empty()) {
            std::cout << "Warning: vibration_values is empty, using default parameters" << std::endl;

            // 婵炶揪缍€濞夋洟寮妶鍡╂付婵☆垱顑欓崥鍥煕濞嗗繐鈧綊寮抽悢鐓庣婵犻潧妫妤呮煕?
            vibr_k_eff.resize(1);
            vibr_stock_c.resize(1);

            // 闁荤姳绀佹晶浠嬫偪閸℃﹩娓舵俊顖涱儥閸氬洭鏌?
            vibr_k_eff[0] = 1.0;
            vibr_stock_c[0] = 2 * stock_vibr_damping_ratio * 1.0;

            // 闁荤姳绀佹晶浠嬫偪閸℃﹩娓舵俊顖涱儥閸氬洭鏌ｉ妸銉ユewmark闂佸憡鐟ラ崐褰掑汲?
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

        // 婵烇絽娴傞崰妤呭极閸忚偐鈻旈柛婵嗗閸炪劌霉濠婂啫顒㈤懚鈺呮煙椤戣儻鍏屾繛鍫熷灴瀹曪綁宕掑☉娆愵啀闁荤姳绶ょ槐鏇㈡偩?
        vibr_k_eff.resize(vibration_values.size());
        vibr_stock_c.resize(vibration_values.size());
        for (size_t mode = 0; mode < vibration_values.size(); ++mode) {
            double lambda = vibration_values[mode]; // 闂佺粯顨堥幊鎾舵濞戙垹纾?
            double omega = sqrt(lambda);           // 闂佹悶鍎抽崕銈咃耿娴ｇ櫢绱ｉ柟瀵稿Т閼?
            vibr_stock_c[mode] = 2 * stock_vibr_damping_ratio * omega;   // 濠碘槅鍨崜婵嬪焵椤戣法绐旀俊顖氭娴?

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
            //vibr_k_eff[mode] = lambda + vibr_a0 * 1.0 + vibr_a1 * vibr_stock_c[mode]; // 闁荤姵鍔戦崝鎴﹀闯濞差亝鍎楅柍鍝勬噺閳诲牓鏌涘▎鎯疯偐鎷?
        }
    }

    void digitaltwin_AptCutterVolume::initstockVibration() {
        // 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥冨妼閻╀線鏌涢弬璇插鐎殿噮鍓熷顐﹀级鐠恒劍鎲奸梺绋胯閸斿海鍒掗妸鈺佸嚑闁告帗鍔曡灒闁斥晛鍟犻崑鎾寸▕?
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

        if (!has_surface) {
            qDebug() << "Skip stock vibration: no surface contact point.";
            temp_vibration_vectors = vibration_vectors;
            return;
        }

        if (temp_vibration_vectors.empty() || temp_vibration_vectors[0].empty()) {
            qDebug() << "Skip stock vibration: vibration vectors are empty.";
            temp_vibration_vectors = vibration_vectors;
            return;
        }

        int node_id;
        if (surfaceCenter_id <= 0)
            node_id = -surfaceCenter_id; // 濠殿喗绻愮徊浠嬫偉閸洘鍤嶉柛灞剧矊娴狀垳绱撻崒娑氬ⅹ鐟?
        else
            node_id = surfaceCenter_id + normalvertices_size - 1; // 闂佽鍣崜姘扁偓鍨礋閹崇偤宕掑鍐у寲缂傚倸鍊归悧鏇°亹?

        // 闂佸吋鍎抽崲鑼躲亹閸ヮ剚鍤婃い蹇撴閺嗙娀骞栨潏楣冩闁哄棛鍠栭弻宀冪疀閹炬潙顏┑鈽嗗灙閸撴繈鍩€椤戣法鍔嶉柡鍡欏枛閺?
        const size_t num_dofs = temp_vibration_vectors[0].size();
        const size_t num_modes = temp_vibration_vectors.size();

        if (node_id < 0 || temp_vibration_vectors[0][0].empty()
            || static_cast<size_t>(node_id) >= temp_vibration_vectors[0][0].size()) {
            qDebug() << "Skip stock vibration: invalid surface node id."
                << "surfaceCenter_id:" << surfaceCenter_id
                << "node_id:" << node_id
                << "node_count:" << (temp_vibration_vectors[0][0].empty() ? 0 : temp_vibration_vectors[0][0].size());
            temp_vibration_vectors = vibration_vectors;
            return;
        }

        if (vibr_k_eff.size() < num_modes || vibr_stock_c.size() < num_modes) {
            qDebug() << "Skip stock vibration: vibration parameters are not initialized."
                << "modes:" << num_modes
                << "k_eff:" << vibr_k_eff.size()
                << "stock_c:" << vibr_stock_c.size();
            temp_vibration_vectors = vibration_vectors;
            return;
        }

        // 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥冨妼閻╀線鏌涢弬璇插鐎殿噮鍓熷顐﹀级鐠恒劍鎲奸梺绋胯閸斿海鍒掗妸鈺佸嚑闁告帗鍔曡灒闁斥晛鍟犻崑鎾寸▕?
        vibration_q.resize(num_modes);
        vibration_dq.resize(num_modes);
        vibration_ddq.resize(num_modes);
        next_vibration_q.resize(num_modes);
        next_vibration_dq.resize(num_modes);
        next_vibration_ddq.resize(num_modes);
        // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鐎ｅ棔绶氶獮鈧?
        for (size_t mode = 0; mode < num_modes; ++mode) {
            double f_total = 0.0;
            double f_eff_total = 0.0;

            // 闂備緡鍓欑粔鏉戭啅婵犳艾绠ラ柍褜鍓熷鍨緞鎼粹剝顔囬梺姹囧灩瀹曨剛鈧濞婇悰顕€宕滄担鐑樼劸闂佸憡姊绘刊瀵告濞嗘挸缁╅梺顐ｇ缁€瀣⒑椤掆偓缁夋潙顔忕猾顧磄le_force_map婵炴垶鎼╅崢鎯р枔閹达箑绠ラ柍褜鍓熷鍨緞婵犲偆娼濋梺杞拌兌婢ф鐣垫笟鈧弫?
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

            // 闁荤姳绶ょ槐鏇㈡偩閼姐倗椹冲璺侯儐濞呭繘鏌?
            f_eff_total = f_total + (vibr_a0 * vibration_q[mode] +
                vibr_a2 * vibration_dq[mode] +
                vibr_a3 * vibration_ddq[mode])
                + vibr_stock_c[mode] * (vibr_a1 * vibration_q[mode] +
                    vibr_a4 * vibration_dq[mode] +
                    vibr_a5 * vibration_ddq[mode]);
            // 闁荤姳绶ょ槐鏇㈡偩閼姐倗椹冲璺侯儐濞呭繘鏌?
            f_eff_total = f_total;

            double relaxation_factor = 0.7; // 闂佸搫顦伴崕宕囨闁秴鐐婇柣妯垮皺閹藉秹鏌?-1婵炴垶鏌ㄩ澶娢?

            // 闂佸搫娲ら悺銊╁蓟婵犲洤绠版い鏍ㄨ壘琚熼梺鍛婄懃閸婂綊寮抽悢鍏兼櫖闁割偅绻勯幗鐘绘煕鐏炶濡奸柟顔兼喘閹虫盯顢旈崱妯绘闁硅壈鎻紓姘辩不閿濆妞界€光偓鐎ｎ剛顦?
            vibration_q[mode] = next_vibration_q[mode];
            next_vibration_q[mode] = vibration_q[mode] * (1 - relaxation_factor) +
                (f_eff_total / vibr_k_eff[mode]) * relaxation_factor;
            next_vibration_ddq[mode] = vibr_a0 * (next_vibration_q[mode] - vibration_q[mode])
                - vibr_a2 * vibration_dq[mode]
                - vibr_a3 * vibration_ddq[mode];
            next_vibration_dq[mode] = vibration_dq[mode]
                + vibr_a6 * vibration_ddq[mode]
                + vibr_a7 * next_vibration_ddq[mode];

            // 濠电儑缍€椤曆勬叏閻愬瓨缍囬柛锔诲幗濞呮洟姊婚崟顒€濮囬柛?
            next_vibration_q[mode] = std::min(next_vibration_q[mode], deform_color_max);

            //std::cerr << "Mode:" << mode
            //    << " f_eff=" << f_eff_total
            //    << " vibr_k_eff=" << vibr_k_eff[mode]
            //    << " vibration_q=" << next_vibration_q[mode]
            //    << std::endl;
        }
        temp_vibration_vectors = vibration_vectors;//闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥囨暩缁€澶愭⒑椤掆偓閻忔繈宕㈤妶澶婄闁绘ê鐏氶弳蹇涙煕閹邦剚鍣归柣?
    }
#pragma endregion

    } // end namespace
    // end of file volume.cpp
