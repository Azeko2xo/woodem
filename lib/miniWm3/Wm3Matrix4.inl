// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (bool bZero)
{
    if (bZero)
    {
        MakeZero();
    }
    else
    {
        MakeIdentity();
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (const Matrix4& rkM)
{
    size_t uiSize = 16*sizeof(Real);
    System::Memcpy(m_afEntry,uiSize,rkM.m_afEntry,uiSize);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (Real fM00, Real fM01, Real fM02, Real fM03,
    Real fM10, Real fM11, Real fM12, Real fM13, Real fM20, Real fM21,
    Real fM22, Real fM23, Real fM30, Real fM31, Real fM32, Real fM33)
{
    m_afEntry[ 0] = fM00;
    m_afEntry[ 1] = fM01;
    m_afEntry[ 2] = fM02;
    m_afEntry[ 3] = fM03;
    m_afEntry[ 4] = fM10;
    m_afEntry[ 5] = fM11;
    m_afEntry[ 6] = fM12;
    m_afEntry[ 7] = fM13;
    m_afEntry[ 8] = fM20;
    m_afEntry[ 9] = fM21;
    m_afEntry[10] = fM22;
    m_afEntry[11] = fM23;
    m_afEntry[12] = fM30;
    m_afEntry[13] = fM31;
    m_afEntry[14] = fM32;
    m_afEntry[15] = fM33;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::Matrix4 (const Real afEntry[16], bool bRowMajor)
{
    if (bRowMajor)
    {
        size_t uiSize = 16*sizeof(Real);
        System::Memcpy(m_afEntry,uiSize,afEntry,uiSize);
    }
    else
    {
        m_afEntry[ 0] = afEntry[ 0];
        m_afEntry[ 1] = afEntry[ 4];
        m_afEntry[ 2] = afEntry[ 8];
        m_afEntry[ 3] = afEntry[12];
        m_afEntry[ 4] = afEntry[ 1];
        m_afEntry[ 5] = afEntry[ 5];
        m_afEntry[ 6] = afEntry[ 9];
        m_afEntry[ 7] = afEntry[13];
        m_afEntry[ 8] = afEntry[ 2];
        m_afEntry[ 9] = afEntry[ 6];
        m_afEntry[10] = afEntry[10];
        m_afEntry[11] = afEntry[14];
        m_afEntry[12] = afEntry[ 3];
        m_afEntry[13] = afEntry[ 7];
        m_afEntry[14] = afEntry[11];
        m_afEntry[15] = afEntry[15];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::operator const Real* () const
{
    return m_afEntry;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>::operator Real* ()
{
    return m_afEntry;
}
//----------------------------------------------------------------------------
template <class Real>
const Real* Matrix4<Real>::operator[] (int iRow) const
{
    return &m_afEntry[4*iRow];
}
//----------------------------------------------------------------------------
template <class Real>
Real* Matrix4<Real>::operator[] (int iRow)
{
    return &m_afEntry[4*iRow];
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix4<Real>::operator() (int iRow, int iCol) const
{
    return m_afEntry[I(iRow,iCol)];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Matrix4<Real>::operator() (int iRow, int iCol)
{
    return m_afEntry[I(iRow,iCol)];
}
//----------------------------------------------------------------------------
template <class Real>
int Matrix4<Real>::I (int iRow, int iCol)
{
    assert(0 <= iRow && iRow < 4 && 0 <= iCol && iCol < 4);
    return iCol + 4*iRow;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::MakeZero ()
{
    memset(m_afEntry,0,16*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::MakeIdentity ()
{
    m_afEntry[ 0] = (Real)1.0;
    m_afEntry[ 1] = (Real)0.0;
    m_afEntry[ 2] = (Real)0.0;
    m_afEntry[ 3] = (Real)0.0;
    m_afEntry[ 4] = (Real)0.0;
    m_afEntry[ 5] = (Real)1.0;
    m_afEntry[ 6] = (Real)0.0;
    m_afEntry[ 7] = (Real)0.0;
    m_afEntry[ 8] = (Real)0.0;
    m_afEntry[ 9] = (Real)0.0;
    m_afEntry[10] = (Real)1.0;
    m_afEntry[11] = (Real)0.0;
    m_afEntry[12] = (Real)0.0;
    m_afEntry[13] = (Real)0.0;
    m_afEntry[14] = (Real)0.0;
    m_afEntry[15] = (Real)1.0;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::SetRow (int iRow, const Vector4<Real>& rkV)
{
    assert(0 <= iRow && iRow < 4);
    for (int iCol = 0, i = 4*iRow; iCol < 4; iCol++, i++)
    {
        m_afEntry[i] = rkV[iCol];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Vector4<Real> Matrix4<Real>::GetRow (int iRow) const
{
    assert(0 <= iRow && iRow < 4);
    Vector4<Real> kV;
    for (int iCol = 0, i = 4*iRow; iCol < 4; iCol++, i++)
    {
        kV[iCol] = m_afEntry[i];
    }
    return kV;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::SetColumn (int iCol, const Vector4<Real>& rkV)
{
    assert(0 <= iCol && iCol < 4);
    for (int iRow = 0, i = iCol; iRow < 4; iRow++, i += 4)
    {
        m_afEntry[i] = rkV[iRow];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Vector4<Real> Matrix4<Real>::GetColumn (int iCol) const
{
    assert(0 <= iCol && iCol < 4);
    Vector4<Real> kV;
    for (int iRow = 0, i = iCol; iRow < 4; iRow++, i += 4)
    {
        kV[iRow] = m_afEntry[i];
    }
    return kV;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::GetColumnMajor (Real* afCMajor) const
{
    for (int iRow = 0, i = 0; iRow < 4; iRow++)
    {
        for (int iCol = 0; iCol < 4; iCol++)
        {
            afCMajor[i++] = m_afEntry[I(iCol,iRow)];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>& Matrix4<Real>::operator= (const Matrix4& rkM)
{
    size_t uiSize = 16*sizeof(Real);
    System::Memcpy(m_afEntry,uiSize,rkM.m_afEntry,uiSize);
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
int Matrix4<Real>::CompareArrays (const Matrix4& rkM) const
{
    return memcmp(m_afEntry,rkM.m_afEntry,16*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix4<Real>::operator== (const Matrix4& rkM) const
{
    return CompareArrays(rkM) == 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix4<Real>::operator!= (const Matrix4& rkM) const
{
    return CompareArrays(rkM) != 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix4<Real>::operator<  (const Matrix4& rkM) const
{
    return CompareArrays(rkM) < 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix4<Real>::operator<= (const Matrix4& rkM) const
{
    return CompareArrays(rkM) <= 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix4<Real>::operator>  (const Matrix4& rkM) const
{
    return CompareArrays(rkM) > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix4<Real>::operator>= (const Matrix4& rkM) const
{
    return CompareArrays(rkM) >= 0;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::operator+ (const Matrix4& rkM) const
{
    Matrix4 kSum;
    for (int i = 0; i < 16; i++)
    {
        kSum.m_afEntry[i] = m_afEntry[i] + rkM.m_afEntry[i];
    }
    return kSum;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::operator- (const Matrix4& rkM) const
{
    Matrix4 kDiff;
    for (int i = 0; i < 16; i++)
    {
        kDiff.m_afEntry[i] = m_afEntry[i] - rkM.m_afEntry[i];
    }
    return kDiff;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::operator* (const Matrix4& rkM) const
{
    Matrix4 kProd;
    for (int iRow = 0; iRow < 4; iRow++)
    {
        for (int iCol = 0; iCol < 4; iCol++)
        {
            int i = I(iRow,iCol);
            kProd.m_afEntry[i] = (Real)0.0;
            for (int iMid = 0; iMid < 4; iMid++)
            {
                kProd.m_afEntry[i] +=
                    m_afEntry[I(iRow,iMid)]*rkM.m_afEntry[I(iMid,iCol)];
            }
        }
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::operator* (Real fScalar) const
{
    Matrix4 kProd;
    for (int i = 0; i < 16; i++)
    {
        kProd.m_afEntry[i] = fScalar*m_afEntry[i];
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::operator/ (Real fScalar) const
{
    Matrix4 kQuot;
    int i;

    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < 16; i++)
        {
            kQuot.m_afEntry[i] = fInvScalar*m_afEntry[i];
        }
    }
    else
    {
        for (i = 0; i < 16; i++)
        {
            kQuot.m_afEntry[i] = Math<Real>::MAX_REAL;
        }
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::operator- () const
{
    Matrix4 kNeg;
    for (int i = 0; i < 16; i++)
    {
        kNeg.m_afEntry[i] = -m_afEntry[i];
    }
    return kNeg;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>& Matrix4<Real>::operator+= (const Matrix4& rkM)
{
    for (int i = 0; i < 16; i++)
    {
        m_afEntry[i] += rkM.m_afEntry[i];
    }
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>& Matrix4<Real>::operator-= (const Matrix4& rkM)
{
    for (int i = 0; i < 16; i++)
    {
        m_afEntry[i] -= rkM.m_afEntry[i];
    }
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>& Matrix4<Real>::operator*= (Real fScalar)
{
    for (int i = 0; i < 16; i++)
    {
        m_afEntry[i] *= fScalar;
    }
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real>& Matrix4<Real>::operator/= (Real fScalar)
{
    int i;

    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < 16; i++)
        {
            m_afEntry[i] *= fInvScalar;
        }
    }
    else
    {
        for (i = 0; i < 16; i++)
        {
            m_afEntry[i] = Math<Real>::MAX_REAL;
        }
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector4<Real> Matrix4<Real>::operator* (const Vector4<Real>& rkV) const
{
    Vector4<Real> kProd;
    for (int iRow = 0; iRow < 4; iRow++)
    {
        kProd[iRow] = (Real)0.0;
        for (int iCol = 0; iCol < 4; iCol++)
        {
            kProd[iRow] += m_afEntry[I(iRow,iCol)]*rkV[iCol];
        }
            
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::Transpose () const
{
    Matrix4 kTranspose;
    for (int iRow = 0; iRow < 4; iRow++)
    {
        for (int iCol = 0; iCol < 4; iCol++)
        {
            kTranspose.m_afEntry[I(iRow,iCol)] = m_afEntry[I(iCol,iRow)];
        }
    }
    return kTranspose;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::TransposeTimes (const Matrix4& rkM) const
{
    // P = A^T*B, P[r][c] = sum_m A[m][r]*B[m][c]
    Matrix4 kProd;
    for (int iRow = 0; iRow < 4; iRow++)
    {
        for (int iCol = 0; iCol < 4; iCol++)
        {
            int i = I(iRow,iCol);
            kProd.m_afEntry[i] = (Real)0.0;
            for (int iMid = 0; iMid < 4; iMid++)
            {
                kProd.m_afEntry[i] +=
                    m_afEntry[I(iMid,iRow)]*rkM.m_afEntry[I(iMid,iCol)];
            }
        }
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::TimesTranspose (const Matrix4& rkM) const
{
    // P = A*B^T, P[r][c] = sum_m A[r][m]*B[c][m]
    Matrix4 kProd;
    for (int iRow = 0; iRow < 4; iRow++)
    {
        for (int iCol = 0; iCol < 4; iCol++)
        {
            int i = I(iRow,iCol);
            kProd.m_afEntry[i] = (Real)0.0;
            for (int iMid = 0; iMid < 4; iMid++)
            {
                kProd.m_afEntry[i] +=
                    m_afEntry[I(iRow,iMid)]*rkM.m_afEntry[I(iCol,iMid)];
            }
        }
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::Inverse () const
{
    Real fA0 = m_afEntry[ 0]*m_afEntry[ 5] - m_afEntry[ 1]*m_afEntry[ 4];
    Real fA1 = m_afEntry[ 0]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 4];
    Real fA2 = m_afEntry[ 0]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 4];
    Real fA3 = m_afEntry[ 1]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 5];
    Real fA4 = m_afEntry[ 1]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 5];
    Real fA5 = m_afEntry[ 2]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 6];
    Real fB0 = m_afEntry[ 8]*m_afEntry[13] - m_afEntry[ 9]*m_afEntry[12];
    Real fB1 = m_afEntry[ 8]*m_afEntry[14] - m_afEntry[10]*m_afEntry[12];
    Real fB2 = m_afEntry[ 8]*m_afEntry[15] - m_afEntry[11]*m_afEntry[12];
    Real fB3 = m_afEntry[ 9]*m_afEntry[14] - m_afEntry[10]*m_afEntry[13];
    Real fB4 = m_afEntry[ 9]*m_afEntry[15] - m_afEntry[11]*m_afEntry[13];
    Real fB5 = m_afEntry[10]*m_afEntry[15] - m_afEntry[11]*m_afEntry[14];

    Real fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
    if (Math<Real>::FAbs(fDet) <= Math<Real>::ZERO_TOLERANCE)
    {
        return Matrix4<Real>::ZERO;
    }

    Matrix4 kInv;
    kInv[0][0] = + m_afEntry[ 5]*fB5 - m_afEntry[ 6]*fB4 + m_afEntry[ 7]*fB3;
    kInv[1][0] = - m_afEntry[ 4]*fB5 + m_afEntry[ 6]*fB2 - m_afEntry[ 7]*fB1;
    kInv[2][0] = + m_afEntry[ 4]*fB4 - m_afEntry[ 5]*fB2 + m_afEntry[ 7]*fB0;
    kInv[3][0] = - m_afEntry[ 4]*fB3 + m_afEntry[ 5]*fB1 - m_afEntry[ 6]*fB0;
    kInv[0][1] = - m_afEntry[ 1]*fB5 + m_afEntry[ 2]*fB4 - m_afEntry[ 3]*fB3;
    kInv[1][1] = + m_afEntry[ 0]*fB5 - m_afEntry[ 2]*fB2 + m_afEntry[ 3]*fB1;
    kInv[2][1] = - m_afEntry[ 0]*fB4 + m_afEntry[ 1]*fB2 - m_afEntry[ 3]*fB0;
    kInv[3][1] = + m_afEntry[ 0]*fB3 - m_afEntry[ 1]*fB1 + m_afEntry[ 2]*fB0;
    kInv[0][2] = + m_afEntry[13]*fA5 - m_afEntry[14]*fA4 + m_afEntry[15]*fA3;
    kInv[1][2] = - m_afEntry[12]*fA5 + m_afEntry[14]*fA2 - m_afEntry[15]*fA1;
    kInv[2][2] = + m_afEntry[12]*fA4 - m_afEntry[13]*fA2 + m_afEntry[15]*fA0;
    kInv[3][2] = - m_afEntry[12]*fA3 + m_afEntry[13]*fA1 - m_afEntry[14]*fA0;
    kInv[0][3] = - m_afEntry[ 9]*fA5 + m_afEntry[10]*fA4 - m_afEntry[11]*fA3;
    kInv[1][3] = + m_afEntry[ 8]*fA5 - m_afEntry[10]*fA2 + m_afEntry[11]*fA1;
    kInv[2][3] = - m_afEntry[ 8]*fA4 + m_afEntry[ 9]*fA2 - m_afEntry[11]*fA0;
    kInv[3][3] = + m_afEntry[ 8]*fA3 - m_afEntry[ 9]*fA1 + m_afEntry[10]*fA0;

    Real fInvDet = ((Real)1.0)/fDet;
    for (int iRow = 0; iRow < 4; iRow++)
    {
        for (int iCol = 0; iCol < 4; iCol++)
        {
            kInv[iRow][iCol] *= fInvDet;
        }
    }

    return kInv;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> Matrix4<Real>::Adjoint () const
{
    Real fA0 = m_afEntry[ 0]*m_afEntry[ 5] - m_afEntry[ 1]*m_afEntry[ 4];
    Real fA1 = m_afEntry[ 0]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 4];
    Real fA2 = m_afEntry[ 0]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 4];
    Real fA3 = m_afEntry[ 1]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 5];
    Real fA4 = m_afEntry[ 1]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 5];
    Real fA5 = m_afEntry[ 2]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 6];
    Real fB0 = m_afEntry[ 8]*m_afEntry[13] - m_afEntry[ 9]*m_afEntry[12];
    Real fB1 = m_afEntry[ 8]*m_afEntry[14] - m_afEntry[10]*m_afEntry[12];
    Real fB2 = m_afEntry[ 8]*m_afEntry[15] - m_afEntry[11]*m_afEntry[12];
    Real fB3 = m_afEntry[ 9]*m_afEntry[14] - m_afEntry[10]*m_afEntry[13];
    Real fB4 = m_afEntry[ 9]*m_afEntry[15] - m_afEntry[11]*m_afEntry[13];
    Real fB5 = m_afEntry[10]*m_afEntry[15] - m_afEntry[11]*m_afEntry[14];

    Matrix4 kAdj;
    kAdj[0][0] = + m_afEntry[ 5]*fB5 - m_afEntry[ 6]*fB4 + m_afEntry[ 7]*fB3;
    kAdj[1][0] = - m_afEntry[ 4]*fB5 + m_afEntry[ 6]*fB2 - m_afEntry[ 7]*fB1;
    kAdj[2][0] = + m_afEntry[ 4]*fB4 - m_afEntry[ 5]*fB2 + m_afEntry[ 7]*fB0;
    kAdj[3][0] = - m_afEntry[ 4]*fB3 + m_afEntry[ 5]*fB1 - m_afEntry[ 6]*fB0;
    kAdj[0][1] = - m_afEntry[ 1]*fB5 + m_afEntry[ 2]*fB4 - m_afEntry[ 3]*fB3;
    kAdj[1][1] = + m_afEntry[ 0]*fB5 - m_afEntry[ 2]*fB2 + m_afEntry[ 3]*fB1;
    kAdj[2][1] = - m_afEntry[ 0]*fB4 + m_afEntry[ 1]*fB2 - m_afEntry[ 3]*fB0;
    kAdj[3][1] = + m_afEntry[ 0]*fB3 - m_afEntry[ 1]*fB1 + m_afEntry[ 2]*fB0;
    kAdj[0][2] = + m_afEntry[13]*fA5 - m_afEntry[14]*fA4 + m_afEntry[15]*fA3;
    kAdj[1][2] = - m_afEntry[12]*fA5 + m_afEntry[14]*fA2 - m_afEntry[15]*fA1;
    kAdj[2][2] = + m_afEntry[12]*fA4 - m_afEntry[13]*fA2 + m_afEntry[15]*fA0;
    kAdj[3][2] = - m_afEntry[12]*fA3 + m_afEntry[13]*fA1 - m_afEntry[14]*fA0;
    kAdj[0][3] = - m_afEntry[ 9]*fA5 + m_afEntry[10]*fA4 - m_afEntry[11]*fA3;
    kAdj[1][3] = + m_afEntry[ 8]*fA5 - m_afEntry[10]*fA2 + m_afEntry[11]*fA1;
    kAdj[2][3] = - m_afEntry[ 8]*fA4 + m_afEntry[ 9]*fA2 - m_afEntry[11]*fA0;
    kAdj[3][3] = + m_afEntry[ 8]*fA3 - m_afEntry[ 9]*fA1 + m_afEntry[10]*fA0;

    return kAdj;
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix4<Real>::Determinant () const
{
    Real fA0 = m_afEntry[ 0]*m_afEntry[ 5] - m_afEntry[ 1]*m_afEntry[ 4];
    Real fA1 = m_afEntry[ 0]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 4];
    Real fA2 = m_afEntry[ 0]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 4];
    Real fA3 = m_afEntry[ 1]*m_afEntry[ 6] - m_afEntry[ 2]*m_afEntry[ 5];
    Real fA4 = m_afEntry[ 1]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 5];
    Real fA5 = m_afEntry[ 2]*m_afEntry[ 7] - m_afEntry[ 3]*m_afEntry[ 6];
    Real fB0 = m_afEntry[ 8]*m_afEntry[13] - m_afEntry[ 9]*m_afEntry[12];
    Real fB1 = m_afEntry[ 8]*m_afEntry[14] - m_afEntry[10]*m_afEntry[12];
    Real fB2 = m_afEntry[ 8]*m_afEntry[15] - m_afEntry[11]*m_afEntry[12];
    Real fB3 = m_afEntry[ 9]*m_afEntry[14] - m_afEntry[10]*m_afEntry[13];
    Real fB4 = m_afEntry[ 9]*m_afEntry[15] - m_afEntry[11]*m_afEntry[13];
    Real fB5 = m_afEntry[10]*m_afEntry[15] - m_afEntry[11]*m_afEntry[14];
    Real fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
    return fDet;
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix4<Real>::QForm (const Vector4<Real>& rkU,
    const Vector4<Real>& rkV) const
{
    return rkU.Dot((*this)*rkV);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::MakeObliqueProjection (const Vector3<Real>& rkNormal,
    const Vector3<Real>& rkPoint, const Vector3<Real>& rkDirection)
{
    // The projection plane is Dot(N,X-P) = 0 where N is a 3-by-1 unit-length
    // normal vector and P is a 3-by-1 point on the plane.  The projection
    // is oblique to the plane, in the direction of the 3-by-1 vector D.
    // Necessarily Dot(N,D) is not zero for this projection to make sense.
    // Given a 3-by-1 point U, compute the intersection of the line U+t*D
    // with the plane to obtain t = -Dot(N,U-P)/Dot(N,D).  Then
    //
    //   projection(U) = P + [I - D*N^T/Dot(N,D)]*(U-P)
    //
    // A 4-by-4 homogeneous transformation representing the projection is
    //
    //       +-                               -+
    //   M = | D*N^T - Dot(N,D)*I   -Dot(N,P)D |
    //       |          0^T          -Dot(N,D) |
    //       +-                               -+
    //
    // where M applies to [U^T 1]^T by M*[U^T 1]^T.  The matrix is chosen so
    // that M[3][3] > 0 whenever Dot(N,D) < 0 (projection is onto the
    // "positive side" of the plane).

    Real fNdD = rkNormal.Dot(rkDirection);
    Real fNdP = rkNormal.Dot(rkPoint);
    m_afEntry[ 0] = rkDirection[0]*rkNormal[0] - fNdD;
    m_afEntry[ 1] = rkDirection[0]*rkNormal[1];
    m_afEntry[ 2] = rkDirection[0]*rkNormal[2];
    m_afEntry[ 3] = -fNdP*rkDirection[0];
    m_afEntry[ 4] = rkDirection[1]*rkNormal[0];
    m_afEntry[ 5] = rkDirection[1]*rkNormal[1] - fNdD;
    m_afEntry[ 6] = rkDirection[1]*rkNormal[2];
    m_afEntry[ 7] = -fNdP*rkDirection[1];
    m_afEntry[ 8] = rkDirection[2]*rkNormal[0];
    m_afEntry[ 9] = rkDirection[2]*rkNormal[1];
    m_afEntry[10] = rkDirection[2]*rkNormal[2] - fNdD;
    m_afEntry[11] = -fNdP*rkDirection[2];
    m_afEntry[12] = 0.0f;
    m_afEntry[13] = 0.0f;
    m_afEntry[14] = 0.0f;
    m_afEntry[15] = -fNdD;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::MakePerspectiveProjection (const Vector3<Real>& rkNormal,
    const Vector3<Real>& rkPoint, const Vector3<Real>& rkEye)
{
    //     +-                                                 -+
    // M = | Dot(N,E-P)*I - E*N^T    -(Dot(N,E-P)*I - E*N^T)*E |
    //     |        -N^t                      Dot(N,E)         |
    //     +-                                                 -+
    //
    // where E is the eye point, P is a point on the plane, and N is a
    // unit-length plane normal.

    Real fNdEmP = rkNormal.Dot(rkEye-rkPoint);

    m_afEntry[ 0] = fNdEmP - rkEye[0]*rkNormal[0];
    m_afEntry[ 1] = -rkEye[0]*rkNormal[1];
    m_afEntry[ 2] = -rkEye[0]*rkNormal[2];
    m_afEntry[ 3] = -(m_afEntry[0]*rkEye[0] + m_afEntry[1]*rkEye[1] +
        m_afEntry[2]*rkEye[2]);
    m_afEntry[ 4] = -rkEye[1]*rkNormal[0];
    m_afEntry[ 5] = fNdEmP - rkEye[1]*rkNormal[1];
    m_afEntry[ 6] = -rkEye[1]*rkNormal[2];
    m_afEntry[ 7] = -(m_afEntry[4]*rkEye[0] + m_afEntry[5]*rkEye[1] +
        m_afEntry[6]*rkEye[2]);
    m_afEntry[ 8] = -rkEye[2]*rkNormal[0];
    m_afEntry[ 9] = -rkEye[2]*rkNormal[1];
    m_afEntry[10] = fNdEmP- rkEye[2]*rkNormal[2];
    m_afEntry[11] = -(m_afEntry[8]*rkEye[0] + m_afEntry[9]*rkEye[1] +
        m_afEntry[10]*rkEye[2]);
    m_afEntry[12] = -rkNormal[0];
    m_afEntry[13] = -rkNormal[1];
    m_afEntry[14] = -rkNormal[2];
    m_afEntry[15] = rkNormal.Dot(rkEye);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix4<Real>::MakeReflection (const Vector3<Real>& rkNormal,
    const Vector3<Real>& rkPoint)
{
    //     +-                         -+
    // M = | I-2*N*N^T    2*Dot(N,P)*N |
    //     |     0^T            1      |
    //     +-                         -+
    //
    // where P is a point on the plane and N is a unit-length plane normal.

    Real fTwoNdP = ((Real)2.0)*(rkNormal.Dot(rkPoint));

    m_afEntry[ 0] = (Real)1.0 - ((Real)2.0)*rkNormal[0]*rkNormal[0];
    m_afEntry[ 1] = -((Real)2.0)*rkNormal[0]*rkNormal[1];
    m_afEntry[ 2] = -((Real)2.0)*rkNormal[0]*rkNormal[2];
    m_afEntry[ 3] = fTwoNdP*rkNormal[0];
    m_afEntry[ 4] = -((Real)2.0)*rkNormal[1]*rkNormal[0];
    m_afEntry[ 5] = (Real)1.0 - ((Real)2.0)*rkNormal[1]*rkNormal[1];
    m_afEntry[ 6] = -((Real)2.0)*rkNormal[1]*rkNormal[2];
    m_afEntry[ 7] = fTwoNdP*rkNormal[1];
    m_afEntry[ 8] = -((Real)2.0)*rkNormal[2]*rkNormal[0];
    m_afEntry[ 9] = -((Real)2.0)*rkNormal[2]*rkNormal[1];
    m_afEntry[10] = (Real)1.0 - ((Real)2.0)*rkNormal[2]*rkNormal[2];
    m_afEntry[11] = fTwoNdP*rkNormal[2];
    m_afEntry[12] = (Real)0.0;
    m_afEntry[13] = (Real)0.0;
    m_afEntry[14] = (Real)0.0;
    m_afEntry[15] = (Real)1.0;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix4<Real> operator* (Real fScalar, const Matrix4<Real>& rkM)
{
    return rkM*fScalar;
}
//----------------------------------------------------------------------------
template <class Real>
Vector4<Real> operator* (const Vector4<Real>& rkV, const Matrix4<Real>& rkM)
{
    return Vector4<Real>(
        rkV[0]*rkM[0][0]+rkV[1]*rkM[1][0]+rkV[2]*rkM[2][0]+rkV[3]*rkM[3][0],
        rkV[0]*rkM[0][1]+rkV[1]*rkM[1][1]+rkV[2]*rkM[2][1]+rkV[3]*rkM[3][1],
        rkV[0]*rkM[0][2]+rkV[1]*rkM[1][2]+rkV[2]*rkM[2][2]+rkV[3]*rkM[3][2],
        rkV[0]*rkM[0][3]+rkV[1]*rkM[1][3]+rkV[2]*rkM[2][3]+rkV[3]*rkM[3][3]);
}
//----------------------------------------------------------------------------

