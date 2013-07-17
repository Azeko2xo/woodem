// © 2010 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#if 0 // broken, do not use
// optimize as much as possible even in the debug mode (effective?)
#if defined(__GNUG__) && __GNUC__ >= 4 && __GNUC_MINOR__ >=4
	#pragma GCC push_options
	#pragma GCC optimize "2"
#endif
#endif

#ifdef QUAD_PRECISION
	typedef long double quad;
	typedef quad Real;
#else
	typedef double Real;
#endif

#include<limits>
#include<cstdlib>
#include<boost/lexical_cast.hpp>

/*
 * use Eigen http://eigen.tuxfamily.org, version at least 3
 */

// IMPORTANT!!
#define EIGEN_DONT_ALIGN



// BEGIN workaround for
// * http://eigen.tuxfamily.org/bz/show_bug.cgi?id=528
// * https://sourceforge.net/tracker/index.php?func=detail&aid=3584127&group_id=202880&atid=983354
// (only needed with gcc <= 4.7)
// must come before Eigen/Core is included
#include<stdlib.h>
#include<sys/stat.h>
// END workaround

// require eigen3
//#define EIGEN_NO_DEBUG
#include<Eigen/Core>
#include<Eigen/Geometry>
#include<Eigen/Eigenvalues>
#include<Eigen/QR>
#include<Eigen/LU>
#include<Eigen/SVD>
#include<float.h>

// mimick expectation macros that linux has (see e.g. http://kerneltrap.org/node/4705)
#ifndef likely
	#define likely(x) __builtin_expect(!!(x),1)
#endif
#ifndef unlikely
	#define unlikely(x) __builtin_expect(!!(x),1)
#endif

// templates of those types with single parameter are not possible, use macros for now
#define VECTOR2_TEMPLATE(Scalar) Eigen::Matrix<Scalar,2,1>
#define VECTOR3_TEMPLATE(Scalar) Eigen::Matrix<Scalar,3,1>
#define VECTOR6_TEMPLATE(Scalar) Eigen::Matrix<Scalar,6,1>
#define MATRIX3_TEMPLATE(Scalar) Eigen::Matrix<Scalar,3,3>
#define MATRIX6_TEMPLATE(Scalar) Eigen::Matrix<Scalar,6,6>

// this would be the proper way, but only works in c++-0x (not yet supported by gcc (4.5))
#if 0
	template<typename Scalar> using Vector2=Eigen::Matrix<Scalar,2,1>;
	template<typename Scalar> using Vector3=Eigen::Matrix<Scalar,3,1>;
	template<typename Scalar> using Matrix3=Eigen::Matrix<Scalar,3,3>;
	typedef Vector2<int> Vector2i;
	typedef Vector2<Real> Vector2r;
	// etc
#endif

typedef VECTOR2_TEMPLATE(int) Vector2i;
typedef VECTOR2_TEMPLATE(Real) Vector2r;
typedef VECTOR3_TEMPLATE(int) Vector3i;
typedef VECTOR3_TEMPLATE(Real) Vector3r;
typedef VECTOR6_TEMPLATE(Real) Vector6r;
typedef VECTOR6_TEMPLATE(int) Vector6i;
typedef MATRIX3_TEMPLATE(Real) Matrix3r;
typedef MATRIX6_TEMPLATE(Real) Matrix6r;

typedef Eigen::Matrix<Real,Eigen::Dynamic,Eigen::Dynamic> MatrixXr;
typedef Eigen::Matrix<Real,Eigen::Dynamic,1> VectorXr;

typedef Eigen::Quaternion<Real> Quaternionr;
typedef Eigen::AngleAxis<Real> AngleAxisr;
typedef Eigen::AlignedBox<Real,2> AlignedBox2r;
typedef Eigen::AlignedBox<Real,3> AlignedBox3r;
using Eigen::AngleAxis; using Eigen::Quaternion;

// in some cases, we want to initialize types that have no default constructor (OpenMPAccumulator, for instance)
// template specialization will help us here
template<typename EigenMatrix> EigenMatrix ZeroInitializer(){ return EigenMatrix::Zero(); };
template<> int ZeroInitializer<int>();
template<> long ZeroInitializer<long>();
template<> unsigned long long ZeroInitializer<unsigned long long>();
template<> Real ZeroInitializer<Real>();


// io
template<class Scalar> std::ostream & operator<<(std::ostream &os, const VECTOR2_TEMPLATE(Scalar)& v){ os << v.x()<<" "<<v.y(); return os; };
template<class Scalar> std::ostream & operator<<(std::ostream &os, const VECTOR3_TEMPLATE(Scalar)& v){ os << v.x()<<" "<<v.y()<<" "<<v.z(); return os; };
template<class Scalar> std::ostream & operator<<(std::ostream &os, const VECTOR6_TEMPLATE(Scalar)& v){ os << v[0]<<" "<<v[1]<<" "<<v[2]<<" "<<v[3]<<" "<<v[4]<<" "<<v[5]; return os; };
template<class Scalar> std::ostream & operator<<(std::ostream &os, const Eigen::Quaternion<Scalar>& q){ os<<q.w()<<" "<<q.x()<<" "<<q.y()<<" "<<q.z(); return os; };
// operators
//template<class Scalar> VECTOR3_TEMPLATE(Scalar) operator*(Scalar s, const VECTOR3_TEMPLATE(Scalar)& v) {return v*s;}
//template<class Scalar> MATRIX3_TEMPLATE(Scalar) operator*(Scalar s, const MATRIX3_TEMPLATE(Scalar)& m) { return m*s; }
//template<class Scalar> Quaternion<Scalar> operator*(Scalar s, const Quaternion<Scalar>& q) { return q*s; }
template<typename Scalar> void matrixEigenDecomposition(const MATRIX3_TEMPLATE(Scalar) m, MATRIX3_TEMPLATE(Scalar)& mRot, MATRIX3_TEMPLATE(Scalar)& mDiag){ Eigen::SelfAdjointEigenSolver<MATRIX3_TEMPLATE(Scalar)> a(m); mRot=a.eigenvectors(); mDiag=a.eigenvalues().asDiagonal(); }
// http://eigen.tuxfamily.org/dox/TutorialGeometry.html
template<typename Scalar> MATRIX3_TEMPLATE(Scalar) matrixFromEulerAnglesXYZ(Scalar x, Scalar y, Scalar z){ MATRIX3_TEMPLATE(Scalar) m; m=AngleAxis<Scalar>(x,VECTOR3_TEMPLATE(Scalar)::UnitX())*AngleAxis<Scalar>(y,VECTOR3_TEMPLATE(Scalar)::UnitY())*AngleAxis<Scalar>(z,VECTOR3_TEMPLATE(Scalar)::UnitZ()); return m;}
template<typename Scalar> bool operator==(const Quaternion<Scalar>& u, const Quaternion<Scalar>& v){ return u.x()==v.x() && u.y()==v.y() && u.z()==v.z() && u.w()==v.w(); }
template<typename Scalar> bool operator!=(const Quaternion<Scalar>& u, const Quaternion<Scalar>& v){ return !(u==v); }
template<typename Scalar> bool operator==(const MATRIX3_TEMPLATE(Scalar)& m, const MATRIX3_TEMPLATE(Scalar)& n){ for(int i=0;i<3;i++)for(int j=0;j<3;j++)if(m(i,j)!=n(i,j)) return false; return true; }
template<typename Scalar> bool operator!=(const MATRIX3_TEMPLATE(Scalar)& m, const MATRIX3_TEMPLATE(Scalar)& n){ return !(m==n); }
template<typename Scalar> bool operator==(const MATRIX6_TEMPLATE(Scalar)& m, const MATRIX6_TEMPLATE(Scalar)& n){ for(int i=0;i<6;i++)for(int j=0;j<6;j++)if(m(i,j)!=n(i,j)) return false; return true; }
template<typename Scalar> bool operator!=(const MATRIX6_TEMPLATE(Scalar)& m, const MATRIX6_TEMPLATE(Scalar)& n){ return !(m==n); }
template<typename Scalar> bool operator==(const VECTOR6_TEMPLATE(Scalar)& u, const VECTOR6_TEMPLATE(Scalar)& v){ return u[0]==v[0] && u[1]==v[1] && u[2]==v[2] && u[3]==v[3] && u[4]==v[4] && u[5]==v[5]; }
template<typename Scalar> bool operator!=(const VECTOR6_TEMPLATE(Scalar)& u, const VECTOR6_TEMPLATE(Scalar)& v){ return !(u==v); }
template<typename Scalar> bool operator==(const VECTOR3_TEMPLATE(Scalar)& u, const VECTOR3_TEMPLATE(Scalar)& v){ return u.x()==v.x() && u.y()==v.y() && u.z()==v.z(); }
template<typename Scalar> bool operator!=(const VECTOR3_TEMPLATE(Scalar)& u, const VECTOR3_TEMPLATE(Scalar)& v){ return !(u==v); }
template<typename Scalar> bool operator==(const VECTOR2_TEMPLATE(Scalar)& u, const VECTOR2_TEMPLATE(Scalar)& v){ return u.x()==v.x() && u.y()==v.y(); }
template<typename Scalar> bool operator!=(const VECTOR2_TEMPLATE(Scalar)& u, const VECTOR2_TEMPLATE(Scalar)& v){ return !(u==v); }
template<typename Scalar> Quaternion<Scalar> operator*(Scalar f, const Quaternion<Scalar>& q){ return Quaternion<Scalar>(q.coeffs()*f); }
template<typename Scalar> Quaternion<Scalar> operator+(Quaternion<Scalar> q1, const Quaternion<Scalar>& q2){ return Quaternion<Scalar>(q1.coeffs()+q2.coeffs()); }	/* replace all those by standard math functions
	this is a non-templated version, to avoid compilation because of static constants;
*/
template<typename Scalar>
struct Math{
	static Scalar Sign(Scalar f){ if(f<0) return -1; if(f>0) return 1; return 0; }
	static Scalar UnitRandom(){ return ((double)rand()/((double)(RAND_MAX))); }
	static Scalar SymmetricRandom(){ return 2.*(((double)rand())/((double)(RAND_MAX)))-1.; }
	static Scalar FastInvCos0(Scalar fValue){ Scalar fRoot = sqrt(((Scalar)1.0)-fValue); Scalar fResult = -(Scalar)0.0187293; fResult *= fValue; fResult += (Scalar)0.0742610; fResult *= fValue; fResult -= (Scalar)0.2121144; fResult *= fValue; fResult += (Scalar)1.5707288; fResult *= fRoot; return fResult; }
};
typedef Math<Real> Mathr;

// http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
template <typename T> int sgn(T val){ return (val>T(0))-(val<T(0)); }


/* this was removed in eigen3, see http://forum.kde.org/viewtopic.php?f=74&t=90914 */
template<typename MatrixT>
void Matrix_computeUnitaryPositive(const MatrixT& in, MatrixT* unitary, MatrixT* positive){
	assert(unitary); assert(positive); 
	Eigen::JacobiSVD<MatrixT> svd(in, Eigen::ComputeThinU | Eigen::ComputeThinV);
	MatrixT mU, mV, mS;
	mU = svd.matrixU();
   mV = svd.matrixV();
   mS = svd.singularValues().asDiagonal();
	*unitary=mU * mV.adjoint();
	*positive=mV * mS * mV.adjoint();
}


bool MatrixXr_pseudoInverse(const MatrixXr &a, MatrixXr &a_pinv, double epsilon=std::numeric_limits<MatrixXr::Scalar>::epsilon());



/*
 * Extra woo math functions and classes
 */


/* convert Vector6r in the Voigt notation to corresponding 2nd order symmetric tensor (stored as Matrix3r)
	if strain is true, then multiply non-diagonal parts by .5
*/
template<typename Scalar>
MATRIX3_TEMPLATE(Scalar) voigt_toSymmTensor(const VECTOR6_TEMPLATE(Scalar)& v, bool strain=false){
	Real k=(strain?.5:1.);
	MATRIX3_TEMPLATE(Scalar) ret; ret<<v[0],k*v[5],k*v[4], k*v[5],v[1],k*v[3], k*v[4],k*v[3],v[2]; return ret;
}
/* convert 2nd order tensor to 6-vector (Voigt notation), symmetrizing the tensor;
	if strain is true, multiply non-diagonal compoennts by 2.
*/
template<typename Scalar>
VECTOR6_TEMPLATE(Scalar) tensor_toVoigt(const MATRIX3_TEMPLATE(Scalar)& m, bool strain=false){
	int k=(strain?2:1);
	VECTOR6_TEMPLATE(Scalar) ret; ret<<m(0,0),m(1,1),m(2,2),k*.5*(m(1,2)+m(2,1)),k*.5*(m(2,0)+m(0,2)),k*.5*(m(0,1)+m(1,0)); return ret;
}

/* Apply Levi-Civita permutation tensor on m
	http://en.wikipedia.org/wiki/Levi-Civita_symbol
*/
template<typename Scalar>
VECTOR3_TEMPLATE(Scalar) leviCivita(const MATRIX3_TEMPLATE(Scalar)& m){
	// i,j,k: v_i=ε_ijk W_j,k
	// +: 1,2,3; 3,1,2; 2,3,1
	// -: 1,3,2; 3,2,1; 2,1,3
	return Vector3r(/*+2,3-3,2*/m(1,2)-m(2,1),/*+3,1-1,3*/m(2,0)-m(0,2),/*+1,2-2,1*/m(0,1)-m(1,0));
}

// mapping between 6x6 matrix indices and tensor indices (Voigt notation)
__attribute__((unused))
const short voigtMap[6][6][4]={
	{{0,0,0,0},{0,0,1,1},{0,0,2,2},{0,0,1,2},{0,0,2,0},{0,0,0,1}},
	{{1,1,0,0},{1,1,1,1},{1,1,2,2},{1,1,1,2},{1,1,2,0},{1,1,0,1}},
	{{2,2,0,0},{2,2,1,1},{2,2,2,2},{2,2,1,2},{2,2,2,0},{2,2,0,1}},
	{{1,2,0,0},{1,2,1,1},{1,2,2,2},{1,2,1,2},{1,2,2,0},{1,2,0,1}},
	{{2,0,0,0},{2,0,1,1},{2,0,2,2},{2,0,1,2},{2,0,2,0},{2,0,0,1}},
	{{0,1,0,0},{0,1,1,1},{0,1,2,2},{0,1,1,2},{0,1,2,0},{0,1,0,1}}
};

__attribute__((unused))
const Real NaN(std::numeric_limits<Real>::signaling_NaN());

__attribute__((unused))
const Real Inf(std::numeric_limits<Real>::infinity());

// void quaternionToEulerAngles (const Quaternionr& q, Vector3r& eulerAngles,Real threshold=1e-6f);
template<typename Scalar> void quaterniontoGLMatrix(const Quaternion<Scalar>& q, Scalar m[16]){
	Scalar w2=2.*q.w(), x2=2.*q.x(), y2=2.*q.y(), z2=2.*q.z();
	Scalar x2w=w2*q.w(), y2w=y2*q.w(), z2w=z2*q.w();
	Scalar x2x=x2*q.x(), y2x=y2*q.x(), z2x=z2*q.x();
	Scalar x2y=y2*q.y(), y2y=y2*q.y(), z2y=z2*q.y();
	Scalar x2z=z2*q.z(), y2z=y2*q.z(), z2z=z2*q.z();
	m[0]=1.-(y2y+z2z); m[4]=y2x-z2w;      m[8]=z2x+y2w;       m[12]=0;
	m[1]=y2x+z2w;      m[5]=1.-(x2x+z2z); m[9]=z2y-x2w;       m[13]=0;
	m[2]=z2x-y2w;      m[6]=z2y+x2w;      m[10]=1.-(x2x+y2y); m[14]=0;
	m[3]=0.;           m[7]=0.;           m[11]=0.;           m[15]=1.;
}


/*
 * Serialization of math classes
 */


#include<boost/serialization/nvp.hpp>
#include<boost/serialization/is_bitwise_serializable.hpp>

// fast serialization (no version infor and no tracking) for basic math types
// http://www.boost.org/doc/libs/1_42_0/libs/serialization/doc/traits.html#bitwise
BOOST_IS_BITWISE_SERIALIZABLE(Vector2r);
BOOST_IS_BITWISE_SERIALIZABLE(Vector2i);
BOOST_IS_BITWISE_SERIALIZABLE(Vector3r);
BOOST_IS_BITWISE_SERIALIZABLE(Vector3i);
BOOST_IS_BITWISE_SERIALIZABLE(Vector6r);
BOOST_IS_BITWISE_SERIALIZABLE(Vector6i);
BOOST_IS_BITWISE_SERIALIZABLE(Quaternionr);
BOOST_IS_BITWISE_SERIALIZABLE(Matrix3r);
BOOST_IS_BITWISE_SERIALIZABLE(Matrix6r);
BOOST_IS_BITWISE_SERIALIZABLE(MatrixXr);
BOOST_IS_BITWISE_SERIALIZABLE(VectorXr);
BOOST_IS_BITWISE_SERIALIZABLE(AlignedBox2r);
BOOST_IS_BITWISE_SERIALIZABLE(AlignedBox3r);

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, Vector2r & g, const unsigned int version){
	Real &x=g[0], &y=g[1];
	ar & BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y);
}

template<class Archive>
void serialize(Archive & ar, Vector2i & g, const unsigned int version){
	int &x=g[0], &y=g[1];
	ar & BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y);
}

template<class Archive>
void serialize(Archive & ar, Vector3r & g, const unsigned int version)
{
	Real &x=g[0], &y=g[1], &z=g[2];
	ar & BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y) & BOOST_SERIALIZATION_NVP(z);
}

template<class Archive>
void serialize(Archive & ar, Vector3i & g, const unsigned int version){
	int &x=g[0], &y=g[1], &z=g[2];
	ar & BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y) & BOOST_SERIALIZATION_NVP(z);
}

template<class Archive>
void serialize(Archive & ar, Vector6r & g, const unsigned int version){
	Real &v0=g[0], &v1=g[1], &v2=g[2], &v3=g[3], &v4=g[4], &v5=g[5];
	ar & BOOST_SERIALIZATION_NVP(v0) & BOOST_SERIALIZATION_NVP(v1) & BOOST_SERIALIZATION_NVP(v2) & BOOST_SERIALIZATION_NVP(v3) & BOOST_SERIALIZATION_NVP(v4) & BOOST_SERIALIZATION_NVP(v5);
}

template<class Archive>
void serialize(Archive & ar, Vector6i & g, const unsigned int version){
	int &v0=g[0], &v1=g[1], &v2=g[2], &v3=g[3], &v4=g[4], &v5=g[5];
	ar & BOOST_SERIALIZATION_NVP(v0) & BOOST_SERIALIZATION_NVP(v1) & BOOST_SERIALIZATION_NVP(v2) & BOOST_SERIALIZATION_NVP(v3) & BOOST_SERIALIZATION_NVP(v4) & BOOST_SERIALIZATION_NVP(v5);
}

template<class Archive>
void serialize(Archive & ar, Quaternionr & g, const unsigned int version)
{
	Real &w=g.w(), &x=g.x(), &y=g.y(), &z=g.z();
	ar & BOOST_SERIALIZATION_NVP(w) & BOOST_SERIALIZATION_NVP(x) & BOOST_SERIALIZATION_NVP(y) & BOOST_SERIALIZATION_NVP(z);
}

template<class Archive>
void serialize(Archive & ar, AlignedBox2r & b, const unsigned int version){
	Vector2r& min(b.min()); Vector2r& max(b.max());
	ar & BOOST_SERIALIZATION_NVP(min) & BOOST_SERIALIZATION_NVP(max);
}

template<class Archive>
void serialize(Archive & ar, AlignedBox3r & b, const unsigned int version){
	Vector3r& min(b.min()); Vector3r& max(b.max());
	ar & BOOST_SERIALIZATION_NVP(min) & BOOST_SERIALIZATION_NVP(max);
}

template<class Archive>
void serialize(Archive & ar, Matrix3r & m, const unsigned int version){
	Real &m00=m(0,0), &m01=m(0,1), &m02=m(0,2), &m10=m(1,0), &m11=m(1,1), &m12=m(1,2), &m20=m(2,0), &m21=m(2,1), &m22=m(2,2);
	ar & BOOST_SERIALIZATION_NVP(m00) & BOOST_SERIALIZATION_NVP(m01) & BOOST_SERIALIZATION_NVP(m02) &
		BOOST_SERIALIZATION_NVP(m10) & BOOST_SERIALIZATION_NVP(m11) & BOOST_SERIALIZATION_NVP(m12) &
		BOOST_SERIALIZATION_NVP(m20) & BOOST_SERIALIZATION_NVP(m21) & BOOST_SERIALIZATION_NVP(m22);
}

template<class Archive>
void serialize(Archive & ar, Matrix6r & m, const unsigned int version){
	Real
		&m00=m(0,0), &m01=m(0,1), &m02=m(0,2), &m03=m(0,3), &m04=m(0,4), &m05=m(0,5),
		&m10=m(1,0), &m11=m(1,1), &m12=m(1,2), &m13=m(1,3), &m14=m(1,4), &m15=m(1,5),
		&m20=m(2,0), &m21=m(2,1), &m22=m(2,2), &m23=m(2,3), &m24=m(2,4), &m25=m(2,5),
		&m30=m(3,0), &m31=m(3,1), &m32=m(3,2), &m33=m(3,3), &m34=m(3,4), &m35=m(3,5),
		&m40=m(4,0), &m41=m(4,1), &m42=m(4,2), &m43=m(4,3), &m44=m(4,4), &m45=m(4,5),
		&m50=m(5,0), &m51=m(5,1), &m52=m(5,2), &m53=m(5,3), &m54=m(5,4), &m55=m(5,5);
	ar &
		BOOST_SERIALIZATION_NVP(m00) & BOOST_SERIALIZATION_NVP(m01) & BOOST_SERIALIZATION_NVP(m02) & BOOST_SERIALIZATION_NVP(m03) & BOOST_SERIALIZATION_NVP(m04) & BOOST_SERIALIZATION_NVP(m05) &
		BOOST_SERIALIZATION_NVP(m10) & BOOST_SERIALIZATION_NVP(m11) & BOOST_SERIALIZATION_NVP(m12) & BOOST_SERIALIZATION_NVP(m13) & BOOST_SERIALIZATION_NVP(m14) & BOOST_SERIALIZATION_NVP(m15) &
		BOOST_SERIALIZATION_NVP(m20) & BOOST_SERIALIZATION_NVP(m21) & BOOST_SERIALIZATION_NVP(m22) & BOOST_SERIALIZATION_NVP(m23) & BOOST_SERIALIZATION_NVP(m24) & BOOST_SERIALIZATION_NVP(m25) &
		BOOST_SERIALIZATION_NVP(m30) & BOOST_SERIALIZATION_NVP(m31) & BOOST_SERIALIZATION_NVP(m32) & BOOST_SERIALIZATION_NVP(m33) & BOOST_SERIALIZATION_NVP(m34) & BOOST_SERIALIZATION_NVP(m35) &
		BOOST_SERIALIZATION_NVP(m40) & BOOST_SERIALIZATION_NVP(m41) & BOOST_SERIALIZATION_NVP(m42) & BOOST_SERIALIZATION_NVP(m43) & BOOST_SERIALIZATION_NVP(m44) & BOOST_SERIALIZATION_NVP(m45) &
		BOOST_SERIALIZATION_NVP(m50) & BOOST_SERIALIZATION_NVP(m51) & BOOST_SERIALIZATION_NVP(m52) & BOOST_SERIALIZATION_NVP(m53) & BOOST_SERIALIZATION_NVP(m54) & BOOST_SERIALIZATION_NVP(m55);
}

template<class Archive>
void serialize(Archive & ar, VectorXr & v, const unsigned int version){
	int size=v.size();
	ar & BOOST_SERIALIZATION_NVP(size);
	if(Archive::is_loading::value) v.resize(size);
	for(int i=0; i<v.size(); i++) ar & boost::serialization::make_nvp(("v"+boost::lexical_cast<std::string>(i)).c_str(),v[i]);
};

template<class Archive>
void serialize(Archive & ar, MatrixXr & m, const unsigned int version){
	int rows=m.rows(), cols=m.cols();
	ar & BOOST_SERIALIZATION_NVP(rows) & BOOST_SERIALIZATION_NVP(cols);
	if(Archive::is_loading::value) m.resize(rows,cols);
	for(int r=0; r<m.rows(); r++) for(int c=0; c<m.cols(); c++) ar & boost::serialization::make_nvp(("i_"+boost::lexical_cast<std::string>(r)+"_"+boost::lexical_cast<std::string>(c)).c_str(),m(r,c));
};

} // namespace serialization
} // namespace boost

#if 0
// revert optimization options back
#if defined(__GNUG__) && __GNUC__ >= 4 && __GNUC_MINOR__ >=4
	#pragma GCC pop_options
#endif
#endif
