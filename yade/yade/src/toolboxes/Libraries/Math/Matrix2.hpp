// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 1998-2005.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement http://www.wild-magic.com/License/WildMagic3.pdf and
// may not be copied or disclosed except in accordance with the terms of that
// agreement.

#ifndef WM3MATRIX2_H
#define WM3MATRIX2_H

// Matrix operations are applied on the left.  For example, given a matrix M
// and a vector V, matrix-times-vector is M*V.  That is, V is treated as a
// column vector.  Some graphics APIs use V*M where V is treated as a row
// vector.  In this context the "M" matrix is really a transpose of the M as
// represented in Wild Magic.  Similarly, to apply two matrix operations M0
// and M1, in that order, you compute M1*M0 so that the transform of a vector
// is (M1*M0)*V = M1*(M0*V).  Some graphics APIs use M0*M1, but again these
// matrices are the transpose of those as represented in Wild Magic.  You
// must therefore be careful about how you interface the transformation code
// with graphics APIS.
//
// For memory organization it might seem natural to chose RealType[N][N] for the
// matrix storage, but this can be a problem on a platform/console that
// chooses to store the data in column-major rather than row-major format.
// To avoid potential portability problems, the matrix is stored as RealType[N*N]
// and organized in row-major order.  That is, the entry of the matrix in row
// r (0 <= r < N) and column c (0 <= c < N) is stored at index i = c+N*r
// (0 <= i < N*N).

// Rotation matrices are of the form
//   R = cos(t) -sin(t)
//       sin(t)  cos(t)
// where t > 0 indicates a counterclockwise rotation in the xy-plane.

#include "Vector2.hpp"

//namespace Wm3
//{

template <class RealType>
class Matrix2
{
public:
    // If bZero is true, create the zero matrix.  Otherwise, create the
    // identity matrix.
    Matrix2 (bool bZero = true);

    // copy constructor
    Matrix2 (const Matrix2& rkM);

    // input Mrc is in row r, column c.
    Matrix2 (RealType fM00, RealType fM01, RealType fM10, RealType fM11);

    // Create a matrix from an array of numbers.  The input array is
    // interpreted based on the Boolean input as
    //   true:  entry[0..3] = {m00,m01,m10,m11}  [row major]
    //   false: entry[0..3] = {m00,m10,m01,m11}  [column major]
    Matrix2 (const RealType afEntry[4], bool bRowMajor);

    // Create matrices based on vector input.  The Boolean is interpreted as
    //   true: vectors are columns of the matrix
    //   false: vectors are rows of the matrix
    Matrix2 (const Vector2<RealType>& rkU, const Vector2<RealType>& rkV,
        bool bColumns);
    Matrix2 (const Vector2<RealType>* akV, bool bColumns);

    // create a diagonal matrix
    Matrix2 (RealType fM00, RealType fM11);

    // create a rotation matrix (positive angle - counterclockwise)
    Matrix2 (RealType fAngle);

    // create a tensor product U*V^T
    Matrix2 (const Vector2<RealType>& rkU, const Vector2<RealType>& rkV);

    // create various matrices
    void makeZero ();
    void makeIdentity ();
    void makeDiagonal (RealType fM00, RealType fM11);
    void fromAngle (RealType fAngle);
    void makeTensorProduct (const Vector2<RealType>& rkU,
        const Vector2<RealType>& rkV);

    // member access
    operator const RealType* () const;
    operator RealType* ();
    const RealType* operator[] (int iRow) const;
    RealType* operator[] (int iRow);
    RealType operator() (int iRow, int iCol) const;
    RealType& operator() (int iRow, int iCol);
    void setRow (int iRow, const Vector2<RealType>& rkV);
    Vector2<RealType> getRow (int iRow) const;
    void setColumn (int iCol, const Vector2<RealType>& rkV);
    Vector2<RealType> getColumn (int iCol) const;
    void getColumnMajor (RealType* afCMajor) const;

    // assignment
    Matrix2& operator= (const Matrix2& rkM);

    // comparison
    bool operator== (const Matrix2& rkM) const;
    bool operator!= (const Matrix2& rkM) const;
    bool operator<  (const Matrix2& rkM) const;
    bool operator<= (const Matrix2& rkM) const;
    bool operator>  (const Matrix2& rkM) const;
    bool operator>= (const Matrix2& rkM) const;

    // arithmetic operations
    Matrix2 operator+ (const Matrix2& rkM) const;
    Matrix2 operator- (const Matrix2& rkM) const;
    Matrix2 operator* (const Matrix2& rkM) const;
    Matrix2 operator* (RealType fScalar) const;
    Matrix2 operator/ (RealType fScalar) const;
    Matrix2 operator- () const;

    // arithmetic updates
    Matrix2& operator+= (const Matrix2& rkM);
    Matrix2& operator-= (const Matrix2& rkM);
    Matrix2& operator*= (RealType fScalar);
    Matrix2& operator/= (RealType fScalar);

    // matrix times vector
    Vector2<RealType> operator* (const Vector2<RealType>& rkV) const;  // M * v

    // other operations
    Matrix2 transpose () const;  // M^T
    Matrix2 transposeTimes (const Matrix2& rkM) const;  // this^T * M
    Matrix2 timesTranspose (const Matrix2& rkM) const;  // this * M^T
    Matrix2 inverse () const;
    Matrix2 adjoint () const;
    RealType determinant () const;
    RealType qForm (const Vector2<RealType>& rkU,
        const Vector2<RealType>& rkV) const;  // u^T*M*v

    // The matrix must be a rotation for these functions to be valid.  The
    // last function uses Gram-Schmidt orthonormalization applied to the
    // columns of the rotation matrix.  The angle must be in radians, not
    // degrees.
    void toAngle (RealType& rfAngle) const;
    void orthonormalize ();

    // The matrix must be symmetric.  Factor M = R * D * R^T where
    // R = [u0|u1] is a rotation matrix with columns u0 and u1 and
    // D = diag(d0,d1) is a diagonal matrix whose diagonal entries are d0 and
    // d1.  The eigenvector u[i] corresponds to eigenvector d[i].  The
    // eigenvalues are ordered as d0 <= d1.
    void eigenDecomposition (Matrix2& rkRot, Matrix2& rkDiag) const;

    static const Matrix2 ZERO;
    static const Matrix2 IDENTITY;

private:
    // for indexing into the 1D array of the matrix, iCol+N*iRow
    static int I (int iRow, int iCol);

    // support for comparisons
    int compareArrays (const Matrix2& rkM) const;
    
    // matrix stored in row-major order
    RealType m_afEntry[4];
};

// c * M
template <class RealType>
Matrix2<RealType> operator* (RealType fScalar, const Matrix2<RealType>& rkM);

// v^T * M
template <class RealType>
Vector2<RealType> operator* (const Vector2<RealType>& rkV, const Matrix2<RealType>& rkM);

#include "Matrix2.ipp"

typedef Matrix2<float> Matrix2f;
typedef Matrix2<double> Matrix2d;
typedef Matrix2<Real> Matrix2r;

//}

#endif

