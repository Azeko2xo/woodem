#pragma once
#include<woo/lib/base/Types.hpp>
#include<woo/lib/base/Math.hpp>

// various geometry-related trivial numerical functions
struct CompUtils{
	static void clamp(Real& x, Real a, Real b){ x=clamped(x,a,b);}
	static Real clamped(Real x, Real a, Real b){ return x<a?a:(x>b?b:x); }
	/*! Wrap number to interval 0…sz */
	static Real wrapNum(const Real& x, const Real& sz) { Real norm=x/sz; return (norm-floor(norm))*sz; }
	/*! Wrap number to interval 0…sz; store how many intervals were wrapped in period */
	static Real wrapNum(const Real& x, const Real& sz, int& period) { Real norm=x/sz; period=(int)floor(norm); return (norm-period)*sz; }

	static Vector3r scalarOnColorScale(Real x, Real xmin, Real xmal, int cmap=-1, bool reversed=false);
	static int defaultCmap;
	static Vector3r mapColor(Real normalizedColor, int cmap=-1, bool reversed=false); // if -1, use defaultCmap
	static Vector3r mapColor_map0(Real normalizedColor);
	static Vector3r closestSegmentPt(const Vector3r& P, const Vector3r& A, const Vector3r& B, Real* normPos=NULL);
	static Real segmentPlaneIntersection(const Vector3r& A, const Vector3r& B, const Vector3r& pt, const Vector3r& normal);
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

	// http://stackoverflow.com/a/7918281/761090
	template <class T>
	static inline T fetch_add(T *ptr, T val) {
		#ifdef WOO_OPENMP
			#ifdef __GNUC__
				return __sync_fetch_and_add(ptr, val);
			#elif _OPENMP>=201107
				T t;
				#pragma omp atomic capture
				{ t = *ptr; *ptr += val; }
				return t;
			#else
				#error "This function requires gcc extensions (for __sync_fetch_and_add) or OpenMP 3.1 (for 'pragma omp atomic capture')"
			#endif
		#else
			// with single thread no need to synchronize at all
			T t=*ptr; *ptr+=val; return t;
		#endif
	}
};
