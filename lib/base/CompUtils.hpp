#pragma once
#include<yade/lib/base/Math.hpp>

// various geometry-related trivial numerical functions
struct CompUtils{
	static Vector3r scalarOnColorScale(Real x, Real xmin, Real xmal);
	static int defaultCmap;
	static Vector3r mapColor(Real normalizedColor, int cmap=-1); // if -1, use defaultCmap
	static Vector3r mapColor_map0(Real normalizedColor);
	static Vector3r closestSegmentPt(const Vector3r& P, const Vector3r& A, const Vector3r& B, Real* normPos=NULL);
	static Vector3r inscribedCircleCenter(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2);
};
