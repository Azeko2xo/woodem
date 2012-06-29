#pragma once
#include<woo/lib/base/Types.hpp>
#include<woo/lib/base/Math.hpp>

// various geometry-related trivial numerical functions
struct CompUtils{
	static void clamp(Real& x, Real a, Real b){ x=clamped(x,a,b);}
	static Real clamped(Real x, Real a, Real b){ return x<a?a:(x>b?b:x); }
	static Vector3r scalarOnColorScale(Real x, Real xmin, Real xmal, int cmap=-1);
	static int defaultCmap;
	static Vector3r mapColor(Real normalizedColor, int cmap=-1); // if -1, use defaultCmap
	static Vector3r mapColor_map0(Real normalizedColor);
	static Vector3r closestSegmentPt(const Vector3r& P, const Vector3r& A, const Vector3r& B, Real* normPos=NULL);
	static Vector3r inscribedCircleCenter(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2);
	// compute d0,d1 for line defined by point A and _unit_ vector u, and sphere by center C and radius r
	// return number of intersection points (0,1,2) and sets accordingly either nothing, d0, or d0 and d1 to parameters determining points on the line (A+ti*u)
	// relTol causes points to be merged if they are closer than relTol*r
	static int lineSphereIntersection(const Vector3r& A, const Vector3r& u, const Vector3r& C, const Real r, Real& t0, Real& t1, Real relTol=1e-6);
	// return barycentric coordinates of a point (must be in-plane) on a facet (triangle in space)
	static Vector3r facetBarycentrics(const Vector3r& x, const Vector3r& A, const Vector3r& B, const Vector3r& C);
	struct Colormap{
		std::string name;
		Real rgb[256*3];
	};
	static const vector<Colormap> colormaps;
};
