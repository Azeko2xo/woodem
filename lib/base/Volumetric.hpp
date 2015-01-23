#pragma once

#include<woo/lib/base/Math.hpp>

namespace woo{
	struct Volumetric{
		//! Compute tetrahedron volume; the volume is signed, positive for points in canonical ordering (TODO: check)
		static Real tetraVolume(const Vector3r& A, const Vector3r& B, const Vector3r& C, const Vector3r& D);
		//! Compute tetrahedron inertia
		static Matrix3r tetraInertia(const Vector3r& A, const Vector3r& B, const Vector3r& C, const Vector3r& D);
		static Matrix3r tetraInertia_cov(const Vector3r v[4], Real& vol, bool fixSign=true);
		static Matrix3r tetraInertia_grid(const Vector3r v[4],int div=100);



		//! Compute triangle inertia; zero thickness is assumed for inertia as such;
		//! the density to multiply with at the end is per unit area;
		//! volumetric density should therefore be multiplied by thickness.
		static Matrix3r triangleInertia(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2);
		//! Unsigned (always positive) triangle area given its vertices
		static Real triangleArea(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2);

		//! Recalculates inertia tensor of a body after translation away from (default) or towards its centroid.
		static Matrix3r inertiaTensorTranslate(const Matrix3r& I,const Real m, const Vector3r& off);
		//! Recalculate body's inertia tensor in rotated coordinates.
		static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Matrix3r& T);
		//! Recalculate body's inertia tensor in rotated coordinates.
		static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Quaternionr& rot);

		// Compute pos, ori, inertia, given mass, first and second-order momenum.
		static void computePrincipalAxes(const Real& m, const Vector3r& Sg, const Matrix3r& Ig, Vector3r& pos, Quaternionr& ori, Vector3r& inertia);
	};
};
