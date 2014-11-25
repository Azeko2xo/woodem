#include<woo/lib/base/Volumetric.hpp>
/*! Calculates tetrahedron inertia relative to the origin (0,0,0), with unit density (scales linearly).

See article F. Tonon, "Explicit Exact Formulas for the 3-D Tetrahedron Inertia Tensor in Terms of its Vertex Coordinates", http://www.scipub.org/fulltext/jms2/jms2118-11.pdf

Numerical example to check:

vertices:
	(8.33220, 11.86875, 0.93355)
	(0.75523 ,5.00000, 16.37072)
	(52.61236, 5.00000, 5.38580)
	(2.00000, 5.00000, 3.00000)
centroid:
	(15.92492, 0.78281, 3.72962)
intertia/density WRT centroid:
	a/μ = 43520.33257 m⁵
	b/μ = 194711.28938 m⁵
	c/μ = 191168.76173 m⁵
	a’/μ= 4417.66150 m⁵
	b’/μ=-46343.16662 m⁵
	c’/μ= 11996.20119 m⁵

The numerical testcase (in TetraTestGen::generate) is exact as in the article for inertia (as well as centroid):

43520.3
194711
191169
4417.66
-46343.2
11996.2

*/
Matrix3r woo::Volumetric::tetraInertia(const Vector3r& A, const Vector3r& B, const Vector3r& C, const Vector3r& D){
	#define x1 A[0]
	#define y1 A[1]
	#define z1 A[2]
	#define x2 B[0]
	#define y2 B[1]
	#define z2 B[2]
	#define x3 C[0]
	#define y3 C[1]
	#define z3 C[2]
	#define x4 D[0]
	#define y4 D[1]
	#define z4 D[2]

	// Jacobian of transformation to the reference 4hedron
	Real detJ=(x2-x1)*(y3-y1)*(z4-z1)+(x3-x1)*(y4-y1)*(z2-z1)+(x4-x1)*(y2-y1)*(z3-z1)
		-(x2-x1)*(y4-y1)*(z3-z1)-(x3-x1)*(y2-y1)*(z4-z1)-(x4-x1)*(y3-y1)*(z2-z1);
	detJ=std::abs(detJ);
	Real a=detJ*(y1*y1+y1*y2+y2*y2+y1*y3+y2*y3+
		y3*y3+y1*y4+y2*y4+y3*y4+y4*y4+z1*z1+z1*z2+
		z2*z2+z1*z3+z2*z3+z3*z3+z1*z4+z2*z4+z3*z4+z4*z4)/60.;
	Real b=detJ*(x1*x1+x1*x2+x2*x2+x1*x3+x2*x3+x3*x3+
		x1*x4+x2*x4+x3*x4+x4*x4+z1*z1+z1*z2+z2*z2+z1*z3+
		z2*z3+z3*z3+z1*z4+z2*z4+z3*z4+z4*z4)/60.;
	Real c=detJ*(x1*x1+x1*x2+x2*x2+x1*x3+x2*x3+x3*x3+x1*x4+
		x2*x4+x3*x4+x4*x4+y1*y1+y1*y2+y2*y2+y1*y3+
		y2*y3+y3*y3+y1*y4+y2*y4+y3*y4+y4*y4)/60.;
	// a' in the article etc.
	Real a__=detJ*(2*y1*z1+y2*z1+y3*z1+y4*z1+y1*z2+
		2*y2*z2+y3*z2+y4*z2+y1*z3+y2*z3+2*y3*z3+
		y4*z3+y1*z4+y2*z4+y3*z4+2*y4*z4)/120.;
	Real b__=detJ*(2*x1*z1+x2*z1+x3*z1+x4*z1+x1*z2+
		2*x2*z2+x3*z2+x4*z2+x1*z3+x2*z3+2*x3*z3+
		x4*z3+x1*z4+x2*z4+x3*z4+2*x4*z4)/120.;
	Real c__=detJ*(2*x1*y1+x2*y1+x3*y1+x4*y1+x1*y2+
		2*x2*y2+x3*y2+x4*y2+x1*y3+x2*y3+2*x3*y3+
		x4*y3+x1*y4+x2*y4+x3*y4+2*x4*y4)/120.;

	Matrix3r ret; ret<<
		 a  , -b__, -c__,
		-b__,  b  , -a__,
		-c__, -a__,  c  ;
	return ret;

	#undef x1
	#undef y1
	#undef z1
	#undef x2
	#undef y2
	#undef z2
	#undef x3
	#undef y3
	#undef z3
	#undef x4
	#undef y4
	#undef z4

};

Real woo::Volumetric::tetraVolume(const Vector3r& A, const Vector3r& B, const Vector3r& C, const Vector3r& D){
	return (A-D).dot((B-D).cross(C-D))/6.;
}



// http://en.wikipedia.org/wiki/Inertia_tensor_of_triangle
Matrix3r woo::Volumetric::triangleInertia(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2){
	Matrix3r V; V<<v0,v1,v2;
	Real a=((v1-v0).cross(v2-v0)).norm(); // twice the triangle area
	Matrix3r S; S<<2,1,1, 1,2,1, 1,1,2; S*=(1/24.);
	return (a/24.)*(v0.squaredNorm()+v1.squaredNorm()+v2.squaredNorm()+(v0+v1+v2).squaredNorm())*Matrix3r::Identity()-a*V.transpose()*S*V;
};

Real woo::Volumetric::triangleArea(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2){
	return (1/2.)*((v1-v0).cross(v2-v0)).norm();
}


void woo::Volumetric::computePrincipalAxes(const Real& M, const Vector3r& Sg, const Matrix3r& Ig, Vector3r& pos, Quaternionr& ori, Vector3r& inertia){
	assert(M>0); // LOG_TRACE("M=\n"<<M<<"\nIg=\n"<<Ig<<"\nSg=\n"<<Sg);
	// clump's centroid
	pos=Sg/M;
	// this will calculate translation only, since rotation is zero
	Matrix3r Ic_orientG=inertiaTensorTranslate(Ig, -M /* negative mass means towards centroid */, pos); // inertia at clump's centroid but with world orientation
	//LOG_TRACE("Ic_orientG=\n"<<Ic_orientG);
	Ic_orientG(1,0)=Ic_orientG(0,1); Ic_orientG(2,0)=Ic_orientG(0,2); Ic_orientG(2,1)=Ic_orientG(1,2); // symmetrize
	Eigen::SelfAdjointEigenSolver<Matrix3r> decomposed(Ic_orientG);
	const Matrix3r& R_g2c(decomposed.eigenvectors());
	// has NaNs for identity matrix??
	//LOG_TRACE("R_g2c=\n"<<R_g2c);
	// set quaternion from rotation matrix
	ori=Quaternionr(R_g2c); ori.normalize();
	inertia=decomposed.eigenvalues();
}

/*! @brief Recalculates inertia tensor of a body after translation away from (default) or towards its centroid.
 *
 * @param I inertia tensor in the original coordinates; it is assumed to be upper-triangular (elements below the diagonal are ignored).
 * @param m mass of the body; if positive, translation is away from the centroid; if negative, towards centroid.
 * @param off offset of the new origin from the original origin
 * @return inertia tensor in the new coordinate system; the matrix is symmetric.
 */
Matrix3r woo::Volumetric::inertiaTensorTranslate(const Matrix3r& I, const Real m, const Vector3r& off){
	// short eigen implementation; check it gives the same result as above
	return I+m*(off.dot(off)*Matrix3r::Identity()-off*off.transpose());
}

/*! @brief Recalculate body's inertia tensor in rotated coordinates.
 *
 * @param I inertia tensor in old coordinates
 * @param T rotation matrix from old to new coordinates
 * @return inertia tensor in new coordinates
 */
Matrix3r woo::Volumetric::inertiaTensorRotate(const Matrix3r& I,const Matrix3r& T){
	/* [http://www.kwon3d.com/theory/moi/triten.html] */
	return T.transpose()*I*T;
}

/*! @brief Recalculate body's inertia tensor in rotated coordinates.
 *
 * @param I inertia tensor in old coordinates
 * @param rot quaternion that describes rotation from old to new coordinates
 * @return inertia tensor in new coordinates
 */
Matrix3r woo::Volumetric::inertiaTensorRotate(const Matrix3r& I, const Quaternionr& rot){
	Matrix3r T=rot.toRotationMatrix();
	return inertiaTensorRotate(I,T);
}


