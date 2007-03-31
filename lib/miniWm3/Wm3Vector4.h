// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3VECTOR4_H
#define WM3VECTOR4_H

#include "Wm3FoundationLIB.h"
#include "Wm3Math.h"

namespace Wm3
{

template <class Real>
class Vector4
{
public:
    // construction
    Vector4 ();  // uninitialized
    Vector4 (Real fX, Real fY, Real fZ, Real fW);
    Vector4 (const Real* afTuple);
    Vector4 (const Vector4& rkV);

    // coordinate access
    operator const Real* () const;
    operator Real* ();
    Real operator[] (int i) const;
    Real& operator[] (int i);
    Real X () const;
    Real& X ();
    Real Y () const;
    Real& Y ();
    Real Z () const;
    Real& Z ();
    Real W () const;
    Real& W ();

    // assignment
    Vector4& operator= (const Vector4& rkV);

    // comparison
    bool operator== (const Vector4& rkV) const;
    bool operator!= (const Vector4& rkV) const;
    bool operator<  (const Vector4& rkV) const;
    bool operator<= (const Vector4& rkV) const;
    bool operator>  (const Vector4& rkV) const;
    bool operator>= (const Vector4& rkV) const;

    // arithmetic operations
    Vector4 operator+ (const Vector4& rkV) const;
    Vector4 operator- (const Vector4& rkV) const;
    Vector4 operator* (Real fScalar) const;
    Vector4 operator/ (Real fScalar) const;
    Vector4 operator- () const;

    // arithmetic updates
    Vector4& operator+= (const Vector4& rkV);
    Vector4& operator-= (const Vector4& rkV);
    Vector4& operator*= (Real fScalar);
    Vector4& operator/= (Real fScalar);

    // vector operations
    Real Length () const;
    Real SquaredLength () const;
    Real Dot (const Vector4& rkV) const;
    Real Normalize ();

    // special vectors
    WM3_ITEM static const Vector4 ZERO;    // (0,0,0,0)
    WM3_ITEM static const Vector4 UNIT_X;  // (1,0,0,0)
    WM3_ITEM static const Vector4 UNIT_Y;  // (0,1,0,0)
    WM3_ITEM static const Vector4 UNIT_Z;  // (0,0,1,0)
    WM3_ITEM static const Vector4 UNIT_W;  // (0,0,0,1)
    WM3_ITEM static const Vector4 ONE;     // (1,1,1,1)

private:
    // support for comparisons
    int CompareArrays (const Vector4& rkV) const;

    Real m_afTuple[4];
};

// arithmetic operations
template <class Real>
Vector4<Real> operator* (Real fScalar, const Vector4<Real>& rkV);

// debugging output
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector4<Real>& rkV);

#include "Wm3Vector4.inl"

typedef Vector4<float> Vector4f;
typedef Vector4<double> Vector4d;

}

#endif
