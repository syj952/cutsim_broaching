/*
 *  Copyright 2010-2011 Anders Wallin (anders.e.e.wallin "at" gmail.com)
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
#ifndef VOLUME_H
#define VOLUME_H

///added hust
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPainter>
#include <map>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>

#include <iostream>
#include <list>
#include <cassert>
#include <float.h>
#include <array>

#include "bbox.hpp"
#include "facet.hpp"
#include "glvertex.hpp"
#include "gldata.hpp"
#include "mechanics_map.hpp"
#include "stl.hpp"

#ifdef MULTI_THREAD
#include <QtCore>
#include <QFuture>
#endif

#include <QObject>
#include <QtConcurrent>

#include <algorithm>

#include <fstream>
#include <sstream>
#include <string>

#include <cuda_runtime.h>
#include <iomanip>
//#include "curve_widget.hpp"


namespace cutsim {

    /// base-class for defining implicit volumes from which to build octrees
    /// an implicit volume is defined as a function dist(Point p)
    /// which returns a positive value inside the volume and a negative value outside.
    ///
    /// the "positive inside, negative outside" sign-convetion means that boolean operations can be done with:
    ///
    ///  A U B ('union' or 'sum') =  max( d(A),  d(B) )
    ///  A \ B ('diff' or 'minus') = min( d(A), -d(B) )
    ///  A int B ('intersection') =  min( d(A),  d(B) )
    ///
    /// reference: Frisken et al. "Designing with Distance Fields"
    /// http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.69.9025
    ///
    /// iso-surface extraction using standard marching-cubes requires just the distance
    /// field to be stored at each corner vertex of an octree leaf-node.
    ///
    /// advanced iso-surface extraction using extended-marching-cubes or dual-contouring may require
    /// more information such as normals of the distance field or exact
    /// intersection points and normals. This is similar to a tri-dexel representation.
    /// In multi-material simulation a material-index can be stored.
    /// Each cutter may also cut the material with a color of its own (new vertices have the color of the cutter).

    typedef enum {
        NO_VOLUME = 0,
        RECTANGLE_VOLUME = 1,
        CYLINDER_VOLUME = 2,
        SPHERE_VOLUME = 3,
        STL_VOLUME = 4,
        APT_VOLUME = 5,
    } VolumeType;

    class Volume : public QObject {
        Q_OBJECT
    public:

        /// default constructor
        Volume() {}
        virtual ~Volume() {}

        VolumeType type;

        /// return signed distance from volume surface to Point p
        /// Points p inside the volume should return positive values.
        /// Points p outside the volume should return negative values.
        virtual double dist(const GLVertex& p) const = 0;
        /// update the bounding-box
        virtual void calcBB() {}
        /// bounding-box. This holds the maximum(minimum) points along the X,Y, and Z-coordinates
        /// of the volume (i.e. the volume where dist(p) returns negative values)
        Bbox bb;
        /// the color of this Volume
        Color color;
        /// set the color
        void setColor(GLfloat r, GLfloat g, GLfloat b) {
            color.r = r; color.g = g; color.b = b;
        }


        void setProgress(int value) { progress = value; };
        void accumlateProgress(int value) { progress += value; };
        void sendProgress() { emit signalProgressValue(progress); }
        int Progress() { return progress; }
    signals:
        void signalProgressValue(int progress);
        void signalProgressFeature(QString aMessage, int mim_value, int max_value);

    private:
        int progress;
    };

    /// sphere centered at center
    class SphereVolume : public Volume {

    public:
        /// default constructor
        SphereVolume();
        virtual ~SphereVolume() {}
        /// set radius of sphere
        void setRadius(double r) {
            radius = r;
            calcBB();
        }
        /// set the centerpoint of the sphere
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }
        /// update the Bbox
        void calcBB();
        double dist(const GLVertex& p) const;

        /// center Point of sphere
        GLVertex center;
        /// radius of sphere
        double radius;
    };

    /// box-volume
    /// from corner, go out in the three directions v1,v2,v3
    /// where a, b, c are in [0,1]
    class RectVolume : public Volume {

    public:
        ///added hust

        double max_depth;

        /// default constructor
        RectVolume();
        virtual ~RectVolume() {}
        /// set the length of box
        void setlengthX(double l) {
            lengthX = l;
            calcBB();
        }
        /// set the width of box
        void setlengthY(double w) {
            lengthY = w;
            calcBB();
        }
        /// set the hight of box
        void setlengthZ(double h) {
            lengthZ = h;
            calcBB();
        }
        /// set the center of box
        void setCenter(GLVertex c) {
            center = c;
            calcBB();
        }
        /// set the rotation center of box
        void setRotationCenter(GLVertex c) {
            rotationCenter = c;
        }
        /// set the angle of box
        void setAngle(GLVertex a) {
            angle = a;
        }

        double dist(const GLVertex& p) const override;
        void calcBB();
        GLVertex getCenter() const { return center; }
        double getLengthX() const { return lengthX; }
        double getLengthY() const { return lengthY; }
        double getLengthZ() const { return lengthZ; }

    private:
        /// width of the box
        double lengthX;
        /// length of the box
        double lengthY;
        /// hight of the box
        double lengthZ;
        /// box center of the bottom
        GLVertex center;
        /// center of rotation
        GLVertex rotationCenter;
        /// box angle
        GLVertex angle;
    };

    /// box-volume2
    /// corner is located at the left bottom one.
    /// from corner, width(x) length(y) hight(z)
    class RectVolume2 : public Volume {

    public:
        /// default constructor
        RectVolume2();
        virtual ~RectVolume2() {}
        /// set the corner of box
        void setCorner(GLVertex c) {
            corner = c;
            center = GLVertex(corner.x + width * 0.5, corner.y + length * 0.5, corner.z);
            calcBB();
        }
        /// set the length of box
        void setLength(double l) {
            length = l;
            center = GLVertex(corner.x + width * 0.5, corner.y + length * 0.5, corner.z);
            calcBB();
        }
        /// set the width of box
        void setWidth(double w) {
            width = w;
            center = GLVertex(corner.x + width * 0.5, corner.y + length * 0.5, corner.z);
            calcBB();
        }
        /// set the hight of box
        void setHight(double h) {
            hight = h;
            calcBB();
        }
        /// set the center of box
        void setCenter(GLVertex c) {
            center = c;
            corner = GLVertex(center.x - width * 0.5, center.y - length * 0.5, center.z);
            calcBB();
        }
        /// set the rotation center of box
        void setRotationCenter(GLVertex c) {
            rotationCenter = c;
        }
        /// set the angle of box
        void setAngle(GLVertex a) {
            angle = a;
        }
        /// update the bounding-box
        void calcBB();
        double dist(const GLVertex& p) const;

    private:
        /// one corner of the left bottom of box
        GLVertex corner;
        /// width of the box
        double width;
        /// length of the box
        double length;
        /// hight of the box
        double hight;
        /// box center of the bottom
        GLVertex center;
        /// center of rotation
        GLVertex rotationCenter;
        /// box angle
        GLVertex angle;
    };

    /// cylinder volume
    class CylinderVolume : public Volume {
    public:
        ///added hust
        double max_depth;
        // 在公有部分添加访问方法
        GLVertex getCenter() const { return center; }
        double getRadius() const { return radius; }
        double getLength() const { return length; }
        GLVertex getAngle() const { return angle; }
        GLVertex getRotationCenter() const { return rotationCenter; }
        CylinderVolume();
        virtual ~CylinderVolume() {}
        /// set radius of cylinder
        void setRadius(double r) {
            radius = r;
            calcBB();
        }
        /// set the length of cylinder
        void setLength(double l) {
            length = l;
            calcBB();
        }
        /// set the center of cylinder
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }
        /// set the rotation center of clynder
        void setRotationCenter(GLVertex c) {
            rotationCenter = c;
        }
        /// set the angle of cylinder
        void setAngle(GLVertex a) {
            angle = a;
        }
        /// update the bounding-box of cylinder
        void calcBB();
        double dist(const GLVertex& p) const;

    private:
        /// cylinder radius
        double radius;
        /// cylinder length
        double length;
        /// cylinder center
        GLVertex center;
        /// center of rotation
        GLVertex rotationCenter;
        /// cylinder angle
        GLVertex angle;
    };

    /// STL volume
    class StlVolume : public Stl, public Volume {

    public:
        StlVolume();
        virtual ~StlVolume() {
            V21.resize(0); V21invV21dotV21.resize(0);
            V32.resize(0); V32invV32dotV32.resize(0);
            V13.resize(0); V13invV13dotV13.resize(0);
        }

        /// set the center of STL
        void setCenter(GLVertex v) {
            center = v;
        }
        /// set the rotation center of STL
        void setRotationCenter(GLVertex c) {
            rotationCenter = c;
        }
        /// set the angle of STL
        void setAngle(GLVertex a) {
            angle = a;
        }
        /// set the resplution
        void setCubeResolution(double resolution) {
            cube_resolution = resolution;
        }
        /// update the bounding-box of STL
        void calcBB();
        double dist(const GLVertex& p) const;

        int readStlFile(QString file) { int retval = Stl::readStlFile(file); /*calcBB();*/ return retval; }

        // Getter methods for CUDA access
        const std::vector<GLVertex>& getV21() const { return V21; }
        const std::vector<GLVertex>& getV21invV21dotV21() const { return V21invV21dotV21; }
        const std::vector<GLVertex>& getV32() const { return V32; }
        const std::vector<GLVertex>& getV32invV32dotV32() const { return V32invV32dotV32; }
        const std::vector<GLVertex>& getV13() const { return V13; }
        const std::vector<GLVertex>& getV13invV13dotV13() const { return V13invV13dotV13; }
        const GLVertex& getMinPt() const { return minpt; }
        const GLVertex& getMaxPt() const { return maxpt; }
        double getInvCubeSize() const { return invcubesize; }

    private:
        // V21[i] = facets[i]->v2 - facets[i]->v1
        std::vector<GLVertex> V21;
        // V21[i]/<V21[i], V21[i]>
        std::vector<GLVertex> V21invV21dotV21;
        // V32[i] = facets[i]->v3 - facets[i]->v2
        std::vector<GLVertex> V32;
        // V32[i]/<V32[i], V32[i]>
        std::vector<GLVertex> V32invV32dotV32;
        // V13[i] = facets[i]->v1 - facets[i]->v3
        std::vector<GLVertex> V13;
        // V13[i]/<V13[i], V13[i]>
        std::vector<GLVertex> V13invV13dotV13;
        /// STL center
        GLVertex center;
        /// center of rotation
        GLVertex rotationCenter;
        /// STL angle
        GLVertex angle;

        enum { MAX_INDEX = 128, MAX_X = MAX_INDEX, MAX_Y = MAX_INDEX, MAX_Z = MAX_INDEX, ROUGH_RATIO = 16 };

        GLVertex maxpt;
        GLVertex minpt;
        double maxlength;
        int src_index = 0, dst_index = 1;
        void swapIndex() { int tmp = src_index; src_index = dst_index; dst_index = tmp; assert(src_index ^ dst_index); }
        std::vector<int>  neighborhoodIndex[2][MAX_X][MAX_Y][MAX_Z];
        int  ratio = 2;
        double indexcubesize;
        double invcubesize;
        void calcNeighborhoodIndex(int index_x, int index_y, int index_z, double upper_limit, bool from_facets);
        double distance(const GLVertex& p, int index);
        double cube_resolution;
    };


    typedef enum {
        NO_TOOL = 0,
        CYLINDER = 1,
        BALL = 2,
        BULL = 3,
        CONE = 4,
        APT = 5,
        DRILL = 10,
    } CutterType;

    typedef enum {
        NO_COLLISION = 0x0,
        NECK_COLLISION = 0x10000,
        SHANK_COLLISION = 0x20000,
        HOLDER_COLLISION = 0x40000,
        PARTS_COLLISION = 0x80000,
    } CollisionType;

    typedef struct cutting {
        double	f;
        int		collision;
        int		count;
    } Cutting;

    /// cylindrical cutter volume

    class CutterVolume : public Volume {

    public:
        CutterVolume();
        /// cutter type
        CutterType cuttertype;
        /// cutter radius
        double radius;
        /// cutter length
        double length;
        /// flute length
        double flutelength;
        /// neck radius
        double neckradius;
        /// reach length
        double reachlength;
        /// shank radius
        double shankradius;
        /// max radius of cutter, neck and shank radius
        double maxradius;
        /// cutter center
        GLVertex center;
        /// cutter angle
        GLVertex angle;

        /// Holder variables
        bool enableholder;
        double holderradius;
        double holderlength;
        void enableHolder(bool flag) {
            enableholder = flag;
            if (enableholder && holderradius <= 0.0)
                holderradius = DEFAULT_HOLDER_RADIUS;
            if (enableholder && holderlength <= 0.0)
                holderlength = DEFAULT_HOLDER_LENGTH;
        }
        void setHolderRadius(double r) {
            holderradius = r;
            if (holderradius > 0.0)
                enableholder = true;
            else
                enableholder = false;
            calcBBHolder();
        }
        void setHolderLength(double l) {
            holderlength = l;
            calcBBHolder();
        }

        Bbox bbHolder;
        /// update the Bbox
        void calcBBHolder();

        virtual void setRadius(double r) {}
        virtual void setAngle(GLVertex a) {}
        virtual void setCenter(GLVertex v) {}
        virtual void setTool(CutterVolume* cutter) {}
        virtual GLVertex getCenter() { return center; }
        virtual GLVertex getAngle() { return angle; }
        virtual double dist(const GLVertex& p) const { return 0.0; }
        virtual Cutting dist_cd(const GLVertex& p) const { Cutting r = { 0.0, NO_COLLISION, 0 }; return r; }
    };

    /// cylindrical cutter volume
    class CylCutterVolume : public CutterVolume {

    public:
        CylCutterVolume();
        /// set the radius of Cylindrical Cutter
        void setRadius(double r) {
            radius = r;
            neckradius = r;
            shankradius = r;
            maxradius = r;
            effective_radius = 0.25 * radius;
            calcBB();
        }
        /// set the length of Cylindrical Cutter
        void setLength(double l) {
            length = l;
            flutelength = l;
            reachlength = l;
            calcBB();
        }
        /// set the centerpoint of Cylindrical Cutter
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }        /// cutter position
        /// set the angle of Cylindrical Cutter
        void setAngle(GLVertex a) {
#ifdef MULTI_AXIS
            angle = a;
            bb.angle = a;
            if (enableholder)
                bbHolder.angle = a;
#endif
        }
        /// set the flute length of Cylindrical Cutter
        void setFluteLength(double fl) {
            flutelength = fl;
        }
        /// set the neck radius of Cylindrical Cutter
        void setNeckRadius(double nr) {
            neckradius = nr;
        }
        /// set the reach length of Cylindrical Cutter
        void setReachLength(double rl) {
            reachlength = rl;
        }
        /// set the shank radius of Cylindrical Cutter
        void setShankRadius(double sr) {
            shankradius = sr;
            if (shankradius > radius) {
                maxradius = sr;
                calcBB();
            }
        }
        /// get the centerpoint of Cylindrical Cutter
        GLVertex getCenter() { return center; }
        /// get the angle of Cylindrical Cutter
        GLVertex getAngle() { return angle; }
        /// update the Bbox
        void calcBB();
        double dist(const GLVertex& p) const;
        Cutting dist_cd(const GLVertex& p) const;

    private:
        double effective_radius;
    };

    /// ball-nose cutter volume

    class BallCutterVolume : public CutterVolume {

    public:
        BallCutterVolume();
        /// set the radius of Ball Cutter
        void setRadius(double r) {
            radius = r;
            neckradius = r;
            shankradius = r;
            maxradius = r;
            calcBB();
        }
        /// set the length of Ball Cutter
        void setLength(double l) {
            length = l - radius;
            flutelength = length;
            reachlength = length;
            calcBB();
        }
        /// set the centerpoint of Ball Cutter
        void setCenter(GLVertex v) {
            center = v + GLVertex(0.0, 0.0, radius);
            calcBB();
        }
        /// set the angle of Ball Cutter
        void setAngle(GLVertex a) {
#ifdef MULTI_AXIS
            angle = a;
            bb.angle = a;
            if (enableholder)
                bbHolder.angle = a;
#endif
        }
        /// set the flute length of Ball Cutter
        void setFluteLength(double fl) {
            flutelength = fl - radius;
        }
        /// set the neck radius of Ball Cutter
        void setNeckRadius(double nr) {
            neckradius = nr;
        }
        /// set the reach length of Ball Cutter
        void setReachLength(double rl) {
            reachlength = rl - radius;
        }
        /// set the shank radius of Ball Cutter
        void setShankRadius(double sr) {
            shankradius = sr;
            if (shankradius > radius) {
                maxradius = sr;
                calcBB();
            }
        }
        /// get the centerpoint of Ball Cutter
        GLVertex getCenter() { return center; }
        /// get the angle of Ball Cutter
        GLVertex getAngle() { return angle; }
        /// update bounding box
        void calcBB();
        double dist(const GLVertex& p) const;
        Cutting dist_cd(const GLVertex& p) const;

    private:
        double effective_hight;
    };

    /// bull-nose cutter volume

    class BullCutterVolume : public CutterVolume {

    public:
        BullCutterVolume();

        /// radius of cylinder-part
        double r1;
        /// radius of torus
        double r2;

        /// set the radius of Bull Cutter
        void setRadius(double r) {
            radius = r;
            neckradius = r;
            shankradius = r;
            maxradius = r;
            calcBB();
        }
        /// set the nose radius of Bull Cutter
        void setBullRadius(double r) {
            if (radius - r > 0.0) {
                r1 = radius - r;
                r2 = r;
            }
        }
        /// set the length of Bull Cutter
        void setLength(double l) {
            length = l;
            flutelength = l;
            reachlength = l;
            calcBB();
        }
        /// set the centerpoint of Bull Cutter
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }        /// cutter position
        /// set the angle of Bull Cutter
        void setAngle(GLVertex a) {
#ifdef MULTI_AXIS
            angle = a;
            bb.angle = a;
            if (enableholder)
                bbHolder.angle = a;
#endif
        }
        /// set the flute length of Bull Cutter
        void setFluteLength(double fl) {
            flutelength = fl;
        }
        /// set the neck radius of Bull Cutter
        void setNeckRadius(double nr) {
            neckradius = nr;
        }
        /// set the reach length of Bull Cutter
        void setReachLength(double rl) {
            reachlength = rl;
        }
        /// set the shank radius of Bull Cutter
        void setShankRadius(double sr) {
            shankradius = sr;
            if (shankradius > radius) {
                maxradius = sr;
                calcBB();
            }
        }
        /// get the centerpoint of Bull Cutter
        GLVertex getCenter() { return center; }
        /// get the angle of Bull Cutter
        GLVertex getAngle() { return angle; }
        /// update bounding box
        void calcBB();
        double dist(const GLVertex& p) const;
        Cutting dist_cd(const GLVertex& p) const;

    private:
        double effective_radius;
    };

    /// cone-nose cutter volume

    class ConeCutterVolume : public CutterVolume {

    public:
        ConeCutterVolume();

        /// radius of tip-part
        double r1;
        /// radius of base-part
        double r2;

        /// set the radius of Cone Cutter
        void setRadius(double r) {
            radius = r;
            neckradius = r;
            shankradius = r;
            maxradius = r;
            r1 = r2 = r;
            calcBB();
        }
        /// set the nose tip radius of Cone Cutter
        void setTipRadius(double r) {
            r1 = r;
            if (maxradius < r1) {
                maxradius = r1;
                calcBB();
            }
            if (flutelength > 0.0)
                incline_coff = (r2 - r1) / flutelength;
        }
        /// set the nose base radius of Cone Cutter
        void setBaseRadius(double r) {
            r2 = r;
            if (maxradius < r2) {
                maxradius = r2;
                calcBB();
            }
            if (flutelength > 0.0)
                incline_coff = (r2 - r1) / flutelength;
        }
        /// set the length of Cone Cutter
        void setLength(double l) {
            length = l;
            flutelength = l;
            reachlength = l;
            calcBB();
            if (flutelength > 0.0)
                incline_coff = (r2 - r1) / flutelength;
        }
        /// set the centerpoint of Cone Cutter
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }        /// cutter position
        /// set the angle of Cone Cutter
        void setAngle(GLVertex a) {
#ifdef MULTI_AXIS
            angle = a;
            bb.angle = a;
            if (enableholder)
                bbHolder.angle = a;
#endif
        }
        /// set the flute length of Cone Cutter
        void setFluteLength(double fl) {
            flutelength = fl;
            if (flutelength > 0.0)
                incline_coff = (r2 - r1) / flutelength;
        }
        /// set the neck radius of Cone Cutter
        void setNeckRadius(double nr) {
            neckradius = nr;
        }
        /// set the reach length of Cone Cutter
        void setReachLength(double rl) {
            reachlength = rl;
        }
        /// set the shank radius of Cone Cutter
        void setShankRadius(double sr) {
            shankradius = sr;
            if (shankradius > radius) {
                maxradius = sr;
                calcBB();
            }
        }
        /// get the centerpoint of Cone Cutter
        GLVertex getCenter() { return center; }
        /// get the angle of Cone Cutter
        GLVertex getAngle() { return angle; }
        /// update bounding box
        void calcBB();
        double dist(const GLVertex& p) const;
        Cutting dist_cd(const GLVertex& p) const;

    private:
        double incline_coff;
        double effective_radius;
    };

    /// drill volume

    class DrillVolume : public CutterVolume {

    public:
        DrillVolume();

        /// hight of tip-part
        double tip_hight;

        /// set the radius of Drill
        void setRadius(double r) {
            radius = r;
            neckradius = r;
            shankradius = r;
            maxradius = r;
            assert(tip_angle > 0.0);
            tip_hight = radius / tan(tip_angle / (2.0 * 180.0) * CUTSIM_PI);
            if (tip_hight > 0.0)
                incline_coff = radius / tip_hight;
            tip_radius = 0.1 * radius;
            calcBB();
        }
        /// set the nose tip angle of Drill
        void setTipAngle(double a) {
            if (a > 0.0) {
                tip_angle = a;
                tip_hight = radius / tan(tip_angle / (2.0 * 180.0) * CUTSIM_PI);
                if (tip_hight > 0.0)
                    incline_coff = radius / tip_hight;
            }
        }
        /// set the length of Drill
        void setLength(double l) {
            length = l;
            flutelength = l;
            reachlength = l;
            calcBB();
        }
        /// set the centerpoint of Drill
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }        /// cutter position
        /// set the angle of Drill
        void setAngle(GLVertex a) {
#ifdef MULTI_AXIS
            angle = a;
            bb.angle = a;
            if (enableholder)
                bbHolder.angle = a;
#endif
        }
        /// set the flute length of Drill
        void setFluteLength(double fl) {
            flutelength = fl;
        }
        /// set the neck radius of Drill
        void setNeckRadius(double nr) {
            neckradius = nr;
        }
        /// set the reach length of Drill
        void setReachLength(double rl) {
            reachlength = rl;
        }
        /// set the shank radius of Drill
        void setShankRadius(double sr) {
            shankradius = sr;
            if (shankradius > radius) {
                maxradius = sr;
                calcBB();
            }
        }
        /// get the centerpoint of Drill
        GLVertex getCenter() { return center; }
        /// get the angle of Drill
        GLVertex getAngle() { return angle; }
        /// update bounding box
        void calcBB();
        double dist(const GLVertex& p) const;
        Cutting dist_cd(const GLVertex& p) const;

    private:
        double tip_angle;
        double tip_radius;
        double incline_coff;
        double effective_hight;
    };

    ///added hust

    class broaching_AptCutterVolume : public CutterVolume {
    public:
        broaching_AptCutterVolume();

        double tool_angle, blade_angle, new_angle, tool_angle_T;//刀具当前角度,刀刃角度
        double rotation_angle;//旋转角度
        double z_start;//刀具初始位置
        double node_z_start;//节点起始z坐标
        double a_1, b_1, H_1, r_1;//螺旋角，锥角，锥高度，半径
        double a_2, b_2, H_2, r_2;//螺旋角，锥角，锥高度，半径
        double step;
        int blade_num;
        double max_depth_1;
        double max_depth_2;
        double get_blade_angle_1(double z)const;
        double get_blade_angle_2(double z)const;
        double v_x, v_y, v_z, dx, dy, dz;
        double cube_resolution_1;
        double cube_resolution_2;
        double inv_cube_resolution_2;
        double spindle_speed;
        double dt;
        int blade_sum;
        double v_c;

        std::unordered_map<double, double> r_blade;//刀刃半径
        std::unordered_map<double, double> angle_blade;//刀刃半径
        struct BB_position {
            GLVertex a;
            GLVertex b;
            GLVertex c;
            GLVertex d;
        };
        double bb_xmin, bb_xmax, bb_ymin, bb_ymax;
        std::tuple<GLVertex, GLVertex, GLVertex, GLVertex, GLVertex, GLVertex, GLVertex, GLVertex> bb_points;//包围盒位置
        std::vector<std::vector<GLVertex>> original_blade_points;//刀刃离散点相对坐标集
        std::vector<std::vector<GLVertex>> blade_points;//刀刃离散点绝对坐标集

        // 添加前角数据存储容器
        std::vector<std::vector<double>> blade_angles_gamma;  // 存储刀具前角数据
        std::vector<std::vector<double>> blade_angles_alpha;  // 存储刀具后角数据
        std::vector<std::vector<double>> blade_cut_h;  // 存储刀具后角数据
        std::vector<std::vector<int>> blade_rakeface_id;  // 存储刀具面id
        std::vector<std::vector<GLVertex>> blade_rakeface_vertex;  // 存储刀具面向量
        // 添加前角数据读取方法声明
        int readBladeAnglesFromFile(const std::string& filename, int blade_id);
        double calculateCutThickness(const GLVertex& current_point, const GLVertex& prev_point1, const GLVertex& prev_point2,
            const GLVertex& velocity_vector);

        // 平面信息结构体
        struct PlaneInfo {
            GLVertex normal;  // 平面法向量
            GLVertex point;   // 平面上的点
        };
        std::vector<PlaneInfo> planes;

        struct Segment {
            int type;
            double radius1, radius2, length;
            GLVertex center;
            std::function<double(const GLVertex&)> edgeFunc;
            double z_start, z_end;

            // 添加构造函数
            Segment(int t, double r1, double r2, double len, std::function<double(const GLVertex&)> func, double zs, double ze)
                : type(t), radius1(r1), radius2(r2), length(len), center(), edgeFunc(func), z_start(zs), z_end(ze) {
            }
        };
        std::vector<Segment> segments;
        GLVertex getCenter() { return center; }
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }
        void setangle(double theata) {
            tool_angle = theata;
        }
        void addCylinder(double z_start, double z_end, double radius) {
            segments.push_back(Segment(0, radius, radius, z_end - z_start, nullptr, z_start, z_end));
        }
        void addBall(double z_start, double z_end, double radius) {
            segments.push_back(Segment(1, radius, radius, z_end - z_start, nullptr, z_start, z_end));
        }
        void addCone(double z_start, double z_end, double r1, double r2) {
            segments.push_back(Segment(2, r1, r2, z_end - z_start, nullptr, z_start, z_end));
        }
        void addEdgeCurve(double z_start, double z_end, std::function<double(const GLVertex&)> func) {
            segments.push_back(Segment(3, 0, 0, z_end - z_start, func, z_start, z_end));
        }
        virtual double dist(const GLVertex& p) const override;
        virtual Cutting dist_cd(const GLVertex& p) const override;
        void calcBB() override;
        // 用于记录z值对的刀刃角度和刀具半径
        std::unordered_map<double, double> z_blade_angle;
        void addz_blade_angle(double key, double value) {
            z_blade_angle[key] = value;
        }
        std::unordered_map<double, double> z_r;
        void addz_r(double key, double value) {
            z_r[key] = value;
        }

        struct CutData {
            float avg_dmin;      // 平均切削深度
            float min_d2edge;    // 最小边距距离
            int node_id;         // 最小边距距离对应节点ID

            CutData(float avg, float min_d, int id)
                : avg_dmin(avg), min_d2edge(min_d), node_id(id) {
            }
        };

        std::unordered_map<int, CutData> cut_h;
        void addcut_h(int key, float cuth, float min_d2edge, int node_id) {
            cut_h.emplace(key, CutData(cuth, min_d2edge, node_id));
        }

        struct MachiningContactEvent {
            int step_id = 0;
            double stroke_mm = 0.0;
            double tool_angle = 0.0;
            int blade_id = -1;
            int inside_index = -1;
            GLVertex surface_p0_mm;
            GLVertex surface_p1_mm;
            GLVertex local_x;
            GLVertex local_y;
            GLVertex local_z;
            double sweep_width_mm = 0.0;
            double layer_depth_mm = 0.30;
        };

        double machining_residual_layer_depth_mm;
        int machining_residual_step_id;
        double machining_residual_stroke_mm;
        bool machining_residual_events_enabled;
        std::vector<MachiningContactEvent> machining_contact_events;
        void setMachiningResidualContext(int stepId, double strokeMm);
        void setMachiningResidualEventsEnabled(bool enabled);
        void clearMachiningContactEvents();
        void addMachiningContactEvent(int bladeId, int insideIndex,
            const GLVertex& p0, const GLVertex& p1,
            const GLVertex& materialPoint, double sweepWidthMm);
        // 定义force的数据结构
        struct ForceData {
            GLVertex force_r;
            GLVertex force_t;
            GLVertex force_f;
            GLVertex force_value;
            GLVertex force_position;
            int force_position_id;
            double force_cuth;
            double k_fc, k_fcn;
        };
        std::array<std::array<double, 11>, 4> force_coefs;
        // 添加用于记录力数据的map
        std::unordered_map<int, ForceData> force_map;//刀刃点
        std::unordered_map<int, std::unordered_map<int, ForceData>> angle_force_map;//刀刃数
        std::unordered_map<double, std::unordered_map<int, std::unordered_map<int, ForceData>>> cutnum_angle_force_map;//角度
        // 存储每个tool_angle对应的合力
        std::map<double, GLVertex> angle_total_force;
        GLVertex temp_total_force;

        int checkConvexity(const GLVertex& a, const GLVertex& b, const GLVertex& c, const GLVertex& d) const;
        void calculatePosition_balde();
        void calculateForceData();
        void calculateTotalForce();
        void getCurrentTotalForce(double* Fx, double* Fy, double* Fz);
        void outputTotalForceData(const std::string& output_dir);
        void outputForceData(const std::string& output_dir);
        void outputDeformedBladePointsData(const std::string& output_dir);
        double K_fc_correct, K_fcn_correct, k_fc, k_fcn;//径向、切向和轴向的剪切力系数,径向、切向和轴向的犁耕力系数,修正系数

        //刀具振动参数
        double vibr_c;//阻尼系数
        double vibr_k;//刚度系数
        double vibr_m;//刀具质量
        std::unordered_map<double, double> q_x;
        std::unordered_map<double, double> dot_q_x;
        std::unordered_map<double, double> ddot_q_x;
        std::unordered_map<double, double> q_y;
        std::unordered_map<double, double> dot_q_y;
        std::unordered_map<double, double> ddot_q_y;
        void calculateVibration();

        //工件振动参数
        double vibr_beta, vibr_gamma;
        double vibr_a0, vibr_a1, vibr_a2, vibr_a3, vibr_a4, vibr_a5, vibr_a6, vibr_a7;
        double stock_vibr_damping_ratio;
        std::vector<double> vibr_k_eff, vibr_stock_c;
        std::map<double, std::vector<double>> vibration_q;//<time><mode>
        std::map<double, std::vector<double>> vibration_dq;
        std::map<double, std::vector<double>> vibration_ddq;
        std::vector<double> current_vibration_q;//<mode>
        std::vector<double> next_vibration_q;//<mode>
        void calculatestockVibration();
        void updatestockVibrParams();
        int normalvertices_size;
        std::vector<double>vibration_values;//每个模态对应的特征值
        std::vector<std::vector<double>>pre_vibration_vectors;//每一行代表每个模态对应的特征向量
        std::vector<std::vector<std::vector<double>>>vibration_vectors;//每一行代表每个模态对应的特征向量<模态阶数><自由度方向>
        std::vector<std::vector<std::vector<double>>>temp_vibration_vectors;//每一行代表每个模态对应的特征向量<模态阶数><自由度方向>
        std::vector<double> vibration_mode_max_abs;
        double modal_displacement_limit_mm;
        std::vector<std::vector<std::vector<double>>>
            convertTo3DVibrationVectors(const std::vector<std::vector<double>>& pre_vibration_vectors,
                int num_dofs,
                const std::vector<size_t>& modes_to_read);
        std::vector<std::vector<std::vector<double>>>
            convertTo3DVibrationVectors(const std::vector<std::vector<double>>& pre_vibration_vectors,
                int num_dofs,
                int num_modes_to_read);


        // 新增：广义-α法参数
        double alpha_m;   // 质量参数
        double alpha_f;   // 力参数
        double rho_inf;   // 高频谱半径 (0.0 ≤ rho_inf ≤ 1.0, 推荐0.8-0.9)

        // 新增：存储每个模态的上一时间步力
        std::vector<double> vibration_f_prev;

        int dt_numb;
        bool dt_flag = false;

        //表面质量
        double f_length;
        std::map<double, std::map<int, std::vector<std::vector<GLVertex>>>> real_blade_points_map;  // 存储z值与对应dmin数组的映射
        std::map<double, std::map< int, std::map<int, double>>> cut_h_map;  // 存储z值与对应dmin数组的映射
        // 修改add_cut_h_map函数实现
        void add_cut_h_map(double outer_key, int blade_id, int inside_index, double dmin_value) {
            cut_h_map[outer_key][blade_id][inside_index] = dmin_value;
        }
        std::map<double, std::map< int, std::map<int, double>>> straightness_map;//<角度><刀刃点><value>
        std::map<double, std::map<int, double>> ra_map;//<角度><刀刃点><value>
        void calculateStraightness();
        void calculateSurfaceRoughness();
        void outputStraightnessData(const std::string& output_dir);
        double deform_color_max, deform_color_min;//表面质量可视化
        int deform_color_var; //表面质量可视化变量索引, U magnitude 0, Ux 1, Uy 2, Uz 3;

    };

    class milling_AptCutterVolume : public CutterVolume {
    public:
        milling_AptCutterVolume();

        double tool_angle, blade_angle;//刀具当前角度,刀刃角度
        double rotation_angle;//旋转角度
        double z_start;//刀具初始位置
        double node_z_start;//节点起始z坐标
        double a_1, b_1, H_1, r_1;//螺旋角，锥角，锥高度，半径
        double a_2, b_2, H_2, r_2;//螺旋角，锥角，锥高度，半径
        double step;
        int blade_num;
        double max_depth_1;
        double max_depth_2;
        double get_blade_angle_1(double z)const;
        double get_blade_angle_2(double z)const;
        double v_x, v_y, v_z, dx, dy, dz;
        double cube_resolution_1;
        double cube_resolution_2;
        double inv_cube_resolution_2;
        double spindle_speed;
        double dt;
        double blade_sum;

        //刀具振动参数
        double vibr_c;//阻尼系数
        double vibr_k;//刚度系数
        double vibr_m;//刀具质量
        std::unordered_map<double, double> q_x;
        std::unordered_map<double, double> dot_q_x;
        std::unordered_map<double, double> ddot_q_x;
        std::unordered_map<double, double> q_y;
        std::unordered_map<double, double> dot_q_y;
        std::unordered_map<double, double> ddot_q_y;
        void calculateVibration();


        std::unordered_map<double, double> r_blade;//刀刃半径
        std::unordered_map<double, double> angle_blade;//刀刃半径
        struct BB_position {
            GLVertex a;
            GLVertex b;
            GLVertex c;
            GLVertex d;
        };
        std::vector<GLVertex> original_blade_points;
        std::vector<GLVertex> blade_points;
        struct BoundingBox {
            float3 min;
            float3 max;
        };
        std::vector<BoundingBox> blade_bboxes; // 存储预计算的包围盒
        std::vector<BoundingBox> total_blade_bboxes;

        void readTestPointsFromFile(const std::string& filename);

        GLVertex angle;//刀具旋转角度
        double angle_dx, angle_dy, angle_dz;
        void setAngle(GLVertex a) {
            angle = a;
        }

        struct Segment {
            enum Type { CYLINDER, BALL, CONE, CUSTOM_EDGE } type;
            double radius1, radius2, length;
            GLVertex center;
            std::function<double(const GLVertex&)> edgeFunc;
            double z_start, z_end;

            // 添加构造函数
            Segment(Type t, double r1, double r2, double len, std::function<double(const GLVertex&)> func, double zs, double ze)
                : type(t), radius1(r1), radius2(r2), length(len), center(), edgeFunc(func), z_start(zs), z_end(ze) {
            }
        };
        std::vector<Segment> segments;
        GLVertex getCenter() { return center; }
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }
        void addCylinder(double z_start, double z_end, double radius) {
            segments.push_back(Segment(Segment::CYLINDER, radius, radius, z_end - z_start, nullptr, z_start, z_end));
        }
        void addBall(double z_start, double z_end, double radius) {
            segments.push_back(Segment(Segment::BALL, radius, radius, z_end - z_start, nullptr, z_start, z_end));
        }
        void addCone(double z_start, double z_end, double r1, double r2) {
            segments.push_back(Segment(Segment::CONE, r1, r2, z_end - z_start, nullptr, z_start, z_end));
        }
        void addEdgeCurve(double z_start, double z_end, std::function<double(const GLVertex&)> func) {
            segments.push_back(Segment(Segment::CUSTOM_EDGE, 0, 0, z_end - z_start, func, z_start, z_end));
        }
        virtual double dist(const GLVertex& p) const override;
        virtual Cutting dist_cd(const GLVertex& p) const override;
        void calcBB() override;
        // 用于记录z值对的刀刃角度和刀具半径
        std::unordered_map<double, double> z_blade_angle;
        void addz_blade_angle(double key, double value) {
            z_blade_angle[key] = value;
        }
        std::unordered_map<double, double> z_r;
        void addz_r(double key, double value) {
            z_r[key] = value;
        }
        struct CutData {
            double avg_dmin;      // 平均切削深度
            double min_d2edge;    // 最小边距距离
            int node_id;         // 最小边距距离对应节点ID
            int node_id_up;
            GLVertex P_min;
            GLVertex P0;
            GLVertex P1;

            CutData(double avg, double min_d, int id, GLVertex P, int up_id = -1,
                GLVertex p0 = GLVertex(), GLVertex p1 = GLVertex())
                : avg_dmin(avg), min_d2edge(min_d), node_id(id), node_id_up(up_id),
                P_min(P), P0(p0), P1(p1) {
            }
        };

        std::unordered_map<int, CutData> cut_h;
        void addcut_h(int key, double cuth, double min_d2edge, int node_id, GLVertex P_min,
            int node_id_up = -1, GLVertex P0 = GLVertex(), GLVertex P1 = GLVertex()) {
            cut_h.emplace(key, CutData(cuth, min_d2edge, node_id, P_min, node_id_up, P0, P1));
        }
        // 定义force的数据结构
        struct ForceData {
            GLVertex force_value;
            GLVertex force_position;
            int force_position_id;
            double force_cuth;
            GLVertex P_min;
        };
        std::array<double, 6> force_coefs;
        MechanicsMapLibrary mechanics_map_library;
        bool enable_mechanics_map_capture = true;
        // 添加用于记录力数据的map
        std::unordered_map<int, ForceData> force_map;//刀刃点
        std::unordered_map<int, std::unordered_map<int, ForceData>> angle_force_map;//刀刃数
        std::unordered_map<double, std::unordered_map<int, std::unordered_map<int, ForceData>>> cutnum_angle_force_map;//角度
        // 存储每个tool_angle对应的合力
        std::map<double, GLVertex> angle_total_force;
        GLVertex temp_total_force;

        int checkConvexity(const GLVertex& a, const GLVertex& b, const GLVertex& c, const GLVertex& d) const;

        void calculatePosition_balde();
        void calculateForceData();
        void outputForceData(const std::string& output_dir);
        void calculateTotalForce();
        void getCurrentTotalForce(double* Fx, double* Fy, double* Fz);
        void outputTotalForceData(const std::string& output_dir);
        double K_rc, K_tc, K_ac, K_re, K_te, K_ae;//径向、切向和轴向的剪切力系数,径向、切向和轴向的犁耕力系数
        void PlotForces(double cube_resolution);
        void plotForceCurves(const std::map<double, GLVertex>& angle_total_force);
        void plotVibrationCurves();

        //工件振动参数
        int mfem_depth;
        double vibr_beta, vibr_gamma;
        double vibr_a0, vibr_a1, vibr_a2, vibr_a3, vibr_a4, vibr_a5, vibr_a6, vibr_a7;
        double stock_vibr_damping_ratio;
        std::vector<double> vibr_k_eff, vibr_stock_c;
        std::vector<double> vibration_q;//<mode>
        std::vector<double> vibration_dq;
        std::vector<double> vibration_ddq;
        std::vector<double> next_vibration_q;//<mode>
        std::vector<double> next_vibration_dq;
        std::vector<double> next_vibration_ddq;
        void calculatestockVibration();
        void initstockVibration();
        void updatestockVibrParams();
        int normalvertices_size;
        std::vector<double>vibration_values;//每个模态对应的特征值
        std::vector<std::vector<double>>pre_vibration_vectors;//每一行代表每个模态对应的特征向量
        std::vector<std::vector<std::vector<double>>>vibration_vectors;//每一行代表每个模态对应的特征向量<模态阶数><自由度方向>
        std::vector<std::vector<std::vector<double>>>temp_vibration_vectors;//每一行代表每个模态对应的特征向量<模态阶数><自由度方向>
        std::vector<std::vector<std::vector<double>>>
            convertTo3DVibrationVectors(const std::vector<std::vector<double>>& pre_vibration_vectors,
                int num_dofs,
                const std::vector<size_t>& modes_to_read);
        std::vector<std::vector<std::vector<double>>>
            convertTo3DVibrationVectors(const std::vector<std::vector<double>>& pre_vibration_vectors,
                int num_dofs,
                int num_modes_to_read);

        int dt_numb;

        // 新增：广义-α法参数
        double alpha_m;   // 质量参数
        double alpha_f;   // 力参数
        double rho_inf;   // 高频谱半径 (0.0 ≤ rho_inf ≤ 1.0, 推荐0.8-0.9)

        //表面质量
        std::map<double, std::map<int, std::vector<GLVertex>>> real_blade_points_map;  // 存储z值与对应dmin数组的映射
        std::map<int, std::vector<GLVertex>> surface_map;
        // 修改add_surface_map函数实现
        void add_surface_map(int inside_index, GLVertex point) {
            surface_map[inside_index].push_back(point);  // 直接赋值到双层映射中
        }
        void calculateSurfaceData();
        void outputSurfaceData(const std::string& output_dir);
        void outputVibrationData(const std::string& output_file);
        double deform_color_max, deform_color_min;//表面质量可视化
        int deform_color_var; //表面质量可视化变量索引, U magnitude 0, Ux 1, Uy 2, Uz 3;
    };

    class digitaltwin_AptCutterVolume : public CutterVolume {
    public:
        digitaltwin_AptCutterVolume();

        double tool_angle, blade_angle;//刀具当前角度,刀刃角度
        double rotation_angle;//旋转角度
        double step;
        int blade_num;
        double max_depth_1;
        double max_depth_2;
        double v_x, v_y, v_z, dx, dy, dz;
        double cube_resolution_1;
        double cube_resolution_2;
        double inv_cube_resolution_2;
        double spindle_speed, spindle_feed;
        double dt;
        double blade_sum;
        double now_x, now_y, now_z, pre_x, pre_y, pre_z;
        double A, B, C, pre_A, pre_B, pre_C;

        //刀具振动参数
        double vibr_c;//阻尼系数
        double vibr_k;//刚度系数
        double vibr_m;//刀具质量
        std::unordered_map<double, double> q_x;
        std::unordered_map<double, double> dot_q_x;
        std::unordered_map<double, double> ddot_q_x;
        std::unordered_map<double, double> q_y;
        std::unordered_map<double, double> dot_q_y;
        std::unordered_map<double, double> ddot_q_y;
        void calculateVibration();
        GLVertex surfaceCenter;
        int surfaceCenter_id;
        bool has_surface;


        std::unordered_map<double, double> r_blade;//刀刃半径
        std::unordered_map<double, double> angle_blade;//刀刃半径
        struct BB_position {
            GLVertex a;
            GLVertex b;
            GLVertex c;
            GLVertex d;
        };
        std::vector<GLVertex> original_blade_points;
        std::vector<GLVertex> blade_points;
        struct BoundingBox {
            float3 min;
            float3 max;
        };
        std::vector<BoundingBox> blade_bboxes; // 存储预计算的包围盒
        std::vector<BoundingBox> total_blade_bboxes;

        void readTestPointsFromFile(const std::string& filename);

        GLVertex angle;//刀具旋转角度
        double angle_dx, angle_dy, angle_dz;
        void setAngle(GLVertex a) {
            angle = a;
            calcBB();
        }

        struct Segment {
            enum Type { CYLINDER, BALL, CONE, CUSTOM_EDGE } type;
            double radius1, radius2, length;
            GLVertex center, angle;
            double z_start, z_end;

            // 添加构造函数
            Segment(Type t, double r1, double r2, double len, double zs, double ze)
                : type(t), radius1(r1), radius2(r2), length(len), center(), z_start(zs), z_end(ze) {
            }
        };
        std::vector<Segment> segments;
        GLVertex getCenter() { return center; }
        void setCenter(GLVertex v) {
            center = v;
            calcBB();
        }
        void addCylinder(double z_start, double z_end, double radius) {
            segments.push_back(Segment(Segment::CYLINDER, radius, radius, z_end - z_start, z_start, z_end));
        }
        void addBall(double z_start, double z_end, double radius) {
            segments.push_back(Segment(Segment::BALL, radius, radius, z_end - z_start, z_start, z_end));
        }
        void addCone(double z_start, double z_end, double r1, double r2) {
            segments.push_back(Segment(Segment::CONE, r1, r2, z_end - z_start, z_start, z_end));
        }
        virtual double dist(const GLVertex& p) const override;
        virtual Cutting dist_cd(const GLVertex& p) const override;
        void calcBB() override;
        // 用于记录z值对的刀刃角度和刀具半径
        std::unordered_map<double, double> z_blade_angle;
        void addz_blade_angle(double key, double value) {
            z_blade_angle[key] = value;
        }
        std::unordered_map<double, double> z_r;
        void addz_r(double key, double value) {
            z_r[key] = value;
        }
        struct CutData {
            double avg_dmin;      // 平均切削深度
            double min_d2edge;    // 最小边距距离
            int node_id;         // 最小边距距离对应节点ID
            GLVertex P_min;

            CutData(double avg, double min_d, int id, GLVertex P)
                : avg_dmin(avg), min_d2edge(min_d), node_id(id), P_min(P) {
            }
        };

        std::unordered_map<int, CutData> cut_h;
        void addcut_h(int key, double cuth, double min_d2edge, int node_id, GLVertex P_min) {
            cut_h.emplace(key, CutData(cuth, min_d2edge, node_id, P_min));
        }
        // 定义force的数据结构
        struct ForceData {
            GLVertex force_value;
            GLVertex force_position;
            int force_position_id;
            double force_cuth;
            GLVertex P_min;
        };

        struct ZData {
            double min_d2edge = FLT_MAX;
            int min_node_id = -1;
        };
        std::map<int, ZData> z2sum_count;
        int totalforce_key, totalforce_id;
        std::vector<std::array<double, 3>>force_data_vector;
        MechanicsMapLibrary mechanics_map_library;
        bool mechanics_map_loaded = false;
        std::array<double, 6> theta_f{};
        std::array<double, 3>force_data, force_xyz;//分别为切向、径向和轴向

        int checkConvexity(const GLVertex& a, const GLVertex& b, const GLVertex& c, const GLVertex& d) const;

        void calculatePosition_balde();
        void calculateForceData();
        void outputForceData(const std::string& output_dir);
        void calculateTotalForce();
        void getCurrentTotalForce(double* Fx, double* Fy, double* Fz);
        void outputTotalForceData(const std::string& output_dir);
        double K_rc, K_tc, K_ac, K_re, K_te, K_ae;//径向、切向和轴向的剪切力系数,径向、切向和轴向的犁耕力系数
        void PlotForces(double cube_resolution);
        void plotForceCurves(const std::map<double, GLVertex>& angle_total_force);
        void plotVibrationCurves();

        //工件振动参数
        int mfem_depth;
        double vibr_beta, vibr_gamma;
        double vibr_a0, vibr_a1, vibr_a2, vibr_a3, vibr_a4, vibr_a5, vibr_a6, vibr_a7;
        double stock_vibr_damping_ratio;
        std::vector<double> vibr_k_eff, vibr_stock_c;
        std::vector<double> vibration_q;//<mode>
        std::vector<double> vibration_dq;
        std::vector<double> vibration_ddq;
        std::vector<double> next_vibration_q;//<mode>
        std::vector<double> next_vibration_dq;
        std::vector<double> next_vibration_ddq;
        void calculatestockVibration();
        void initstockVibration();
        void updatestockVibrParams();
        int normalvertices_size;
        std::vector<double>vibration_values;//每个模态对应的特征值
        std::vector<std::vector<double>>pre_vibration_vectors;//每一行代表每个模态对应的特征向量
        std::vector<std::vector<std::vector<double>>>vibration_vectors;//每一行代表每个模态对应的特征向量<模态阶数><自由度方向>
        std::vector<std::vector<std::vector<double>>>temp_vibration_vectors;//每一行代表每个模态对应的特征向量<模态阶数><自由度方向>
        std::vector<std::vector<std::vector<double>>>
            convertTo3DVibrationVectors(const std::vector<std::vector<double>>& pre_vibration_vectors,
                int num_dofs,
                const std::vector<size_t>& modes_to_read);
        std::vector<std::vector<std::vector<double>>>
            convertTo3DVibrationVectors(const std::vector<std::vector<double>>& pre_vibration_vectors,
                int num_dofs,
                int num_modes_to_read);

        int dt_numb;

        // 新增：广义-α法参数
        double alpha_m;   // 质量参数
        double alpha_f;   // 力参数
        double rho_inf;   // 高频谱半径 (0.0 ≤ rho_inf ≤ 1.0, 推荐0.8-0.9)

        //表面质量
        std::map<double, std::map<int, std::vector<GLVertex>>> real_blade_points_map;  // 存储z值与对应dmin数组的映射
        std::map<int, std::vector<GLVertex>> surface_map;
        // 修改add_surface_map函数实现
        void add_surface_map(int inside_index, GLVertex point) {
            surface_map[inside_index].push_back(point);  // 直接赋值到双层映射中
        }
        void calculateSurfaceData();
        void outputSurfaceData(const std::string& output_dir);
        void outputVibrationData(const std::string& output_file);
        double deform_color_max, deform_color_min;//表面质量可视化
        int deform_color_var; //表面质量可视化变量索引, U magnitude 0, Ux 1, Uy 2, Uz 3;
    };
} // end namespace
#endif
// end file volume.hpp
