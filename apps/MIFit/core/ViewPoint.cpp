#include "ViewPoint.h"

#include <cmath>
#include <cstdio>
#include <chemlib/chemlib.h>
#include <chemlib/Monomer.h>
#include <chemlib/MIMoleculeBase.h>
#include <math/mathlib.h>
#include <math/Quaternion.h>
#include <math/Vector3.h>
#include <opengl/QuatUtil.h>

#include "Cfiles.h"

using namespace chemlib;
using namespace mi::math;
using namespace mi::opengl;

ViewPoint::ViewPoint()
    : width_(200), height_(100),
      perspective_(0),
      scale_(20),
      frontClip_(5.0), backClip_(-5.0)

{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (i == j)
            {
                viewmat[i][j] = 1.0;
            }
            else
            {
                viewmat[i][j] = 0;
            }
        }
    }
    center[0] = center[1] = center[2] = 0;
    zangle = sin(perspective_ * M_PI/180.0);
}

void ViewPoint::UnDo()
{
    if (UndoList.size() > 0)
    {
        for (int i = 0; i < 3; i++)
        {
            center[i] = UndoList.back().center[i];
            for (int j = 0; j < 3; j++)
            {
                viewmat[i][j] = UndoList.back().viewmat[i][j];
            }
        }
        scale_ = UndoList.back().scale;
        frontClip_ = UndoList.back().frontClip;
        backClip_ = UndoList.back().backClip;
        //undoable = false;
        UndoList.pop_back();
    }
}

void ViewPoint::Do()
{
    ViewSave last;
    for (int i = 0; i < 3; i++)
    {
        last.center[i] = center[i];
        for (int j = 0; j < 3; j++)
        {
            last.viewmat[i][j] = viewmat[i][j];
        }
    }
    last.scale = scale_;
    last.frontClip = frontClip_;
    last.backClip = backClip_;
    UndoList.push_back(last);
}

void ViewPoint::setmatrix(float mat[3][3])
{
    viewmat[0][0] = mat[0][0];
    viewmat[0][1] = mat[0][1];
    viewmat[0][2] = mat[0][2];
    viewmat[1][0] = mat[1][0];
    viewmat[1][1] = mat[1][1];
    viewmat[1][2] = mat[1][2];
    viewmat[2][0] = mat[2][0];
    viewmat[2][1] = mat[2][1];
    viewmat[2][2] = mat[2][2];
    orthomatrix(viewmat, viewmat);
}

void ViewPoint::copymatrix(float mat[3][3])
{
    //float msf = MSF;
    mat[0][0] = viewmat[0][0];
    mat[0][1] = viewmat[0][1];
    mat[0][2] = viewmat[0][2];
    mat[1][0] = viewmat[1][0];
    mat[1][1] = viewmat[1][1];
    mat[1][2] = viewmat[1][2];
    mat[2][0] = viewmat[2][0];
    mat[2][1] = viewmat[2][1];
    mat[2][2] = viewmat[2][2];
}

void ViewPoint::setSize(int width, int height)
{
    width_ = width;
    height_ = height;
}

void ViewPoint::rotate(float rx, float ry, float rz)
{
    incmatrix(rx, ry, rz, viewmat, viewmat);
    orthomatrix(viewmat, viewmat);
}

void ViewPoint::moveto(float x, float y, float z)
{
    center[0] = x;
    center[1] = y;
    center[2] = z;
}

float ViewPoint::perspective() const
{
    return perspective_;
}

void ViewPoint::setPerspective(float p)
{
    perspective_ = std::max(0.0f, p);
    zangle = sin(perspective_*M_PI/180.0);
}

QVector3D ViewPoint::transform(const QVector3D& point) const
{
    return QVector3D(viewmat[0][0]*point.x() + viewmat[0][1]*point.y()
                     + viewmat[0][2]*point.z(),
                     viewmat[1][0]*point.x() + viewmat[1][1]*point.y()
                     + viewmat[1][2]*point.z(),
                     viewmat[2][0]*point.x() + viewmat[2][1]*point.y()
                     + viewmat[2][2]*point.z());
}


void ViewPoint::transform(const QVector3D& point, int &xt, int &yt, int &zt) const
{
    float x = point.x() - center[0];
    float y = point.y() - center[1];
    float z = point.z() - center[2];

    float rz = -(viewmat[2][0]*x + viewmat[2][1]*y + viewmat[2][2]*z);

    // z is reversed to keep system righthanded
    // if x is across from left to right and y is down
    // then z points into the screen
    float zp = rz*zangle;

    float rx = (viewmat[0][0]*x + viewmat[0][1]*y + viewmat[0][2]*z);
    float t = rx*zp;

    rx += t;
    rx *= scale_;
    xt = (int)rx + width_/2;

    float ry = (viewmat[1][0]*x + viewmat[1][1]*y + viewmat[1][2]*z);
    t = ry*zp;
    ry += t;
    ry *= scale_;
    yt = (int)ry + height_/2;

    rz *= scale_;
    zt = (int)rz;
}

QVector3D ViewPoint::Invert(int sx, int sy, int sz)
{
    uinv(viewmat, umat);
    sx -= width_/2;
    sy -= height_/2;
    float rx = sx;
    float ry = sy;
    float rz = sz;
    float zp = rz*zangle;
    rx /= scale_;
    ry /= scale_;
    rz /= -scale_;
    rx -= rx*zp;
    ry -= ry*zp;
    return QVector3D((rx*umat[0][0] + ry*umat[0][1] + rz*umat[0][2]) + center[0],
                     (rx*umat[1][0] + ry*umat[1][1] + rz*umat[1][2]) + center[1],
                     (rx*umat[2][0] + ry*umat[2][1] + rz*umat[2][2]) + center[2]);
}

void ViewPoint::zoom(float ds)
{
    scale_ *= ds;
}

void ViewPoint::getdirection(int xdrag, int ydrag, float &xdir, float &ydir, float &zdir)
{
    //Scroll view
    //determine direction relative to molecule axes
    //by inverting matrix
    //int xdir, ydir, zdir;

    uinv(viewmat, umat);
    xdir = (xdrag* umat[0][0]
            + ydrag*umat[0][1]) / scale_;
    ydir = (xdrag* umat[1][0]
            + ydrag*umat[1][1]) / scale_;
    zdir = (xdrag* umat[2][0]
            + ydrag*umat[2][1]) / scale_;
}

void ViewPoint::scroll(int xdrag, int ydrag, int zdrag)
{
    //Scroll view
    //determine direction relative to molecule axes
    //by inverting matrix
    uinv(viewmat, umat);
    float xdir = (xdrag*umat[0][0]
                  + ydrag*umat[0][1]
                  + zdrag*umat[0][2]) / scale_;
    float ydir = (xdrag*umat[1][0]
                  + ydrag*umat[1][1]
                  + zdrag*umat[1][2]) / scale_;
    float zdir = (xdrag*umat[2][0]
                  + ydrag*umat[2][1]
                  + zdrag*umat[2][2]) / scale_;
    center[0] += -xdir;
    center[1] += -ydir;
    center[2] += -zdir;
}

void ViewPoint::changeSlab(float delta)
{
    frontClip_ += delta/2.0;
    backClip_ -= delta/2.0;
    if (frontClip_ < backClip_)
    {
        frontClip_ = backClip_ + 1;
    }
}

void ViewPoint::setSlab(float frontClip, float backClip)
{
    frontClip_ = frontClip;
    backClip_ = backClip;
    if (frontClip_ < backClip_)
        std::swap(frontClip_, backClip_);
}

void ViewPoint::setScale(qreal s)
{
    scale_ = s;
    if (scale_ < 1.1)
    {
        scale_ = 1.1;
    }
}

qreal ViewPoint::scale() const
{
    return scale_;
}

qreal ViewPoint::frontClip() const
{
    return frontClip_;
}

qreal ViewPoint::backClip() const
{
    return backClip_;
}

void ViewPoint::setFrontClip(qreal f)
{
    frontClip_ = f;
}

void ViewPoint::setBackClip(qreal b)
{
    backClip_ = b;
}

float ViewPoint::getcenter(int i) const
{
    if (i >= 0 && i < 3)
        return center[i];
    else
        return 0.0;
}

int ViewPoint::width() const
{
    return width_;
}

int ViewPoint::height() const
{
    return height_;
}


float ViewPoint::getwidth() const
{
    return width_ / scale_;
}

float ViewPoint::getheight() const
{
    return height_ / scale_;
}

bool ViewPoint::UnDoable() const
{
    return UndoList.size() > 0;
}
