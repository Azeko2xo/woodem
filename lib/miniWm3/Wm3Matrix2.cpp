// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#include "Wm3FoundationPCH.h"
#include "Wm3Matrix2.h"
using namespace Wm3;

template<> const Matrix2<float> Matrix2<float>::ZERO(
    0.0f,0.0f,
    0.0f,0.0f);
template<> const Matrix2<float> Matrix2<float>::IDENTITY(
    1.0f,0.0f,
    0.0f,1.0f);

template<> const Matrix2<double> Matrix2<double>::ZERO(
    0.0,0.0,
    0.0,0.0);
template<> const Matrix2<double> Matrix2<double>::IDENTITY(
    1.0,0.0,
    0.0,1.0);

