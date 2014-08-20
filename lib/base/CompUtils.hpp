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
	// return parameters where lines approach the most (see distSq_LineLine)
	static Vector2r closestParams_LineLine(const Vector3r& P, const Vector3r& u, const Vector3r& Q, const Vector3r& v, bool& parallel);
	// return squared distance of two lines L1: P+us, L2: Q+vt where the lines approach the most
	// u and v may not be normal, but must not be zero; *parallel* is set when lines are parallel
	static Real distSq_LineLine(const Vector3r& P, const Vector3r& u, const Vector3r& Q, const Vector3r& v, bool& parallel, Vector2r& st);
	// return squared distance between two segments, given by their center point, extents and direction vector
	static Real distSq_SegmentSegment(const Vector3r& center0, const Vector3r& direction0, const Real& halfLength0, const Vector3r& center1, const Vector3r& direction1, const Real& halfLength1, Vector2r& st, bool& parallel);

	static Vector3r closestSegmentPt(const Vector3r& P, const Vector3r& A, const Vector3r& B, Real* normPos=NULL);
	static Real segmentPlaneIntersection(const Vector3r& A, const Vector3r& B, const Vector3r& pt, const Vector3r& normal);
	static Vector3r inscribedCircleCenter(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2);
	static Vector3r circumscribedCircleCenter(const Vector3r& A, const Vector3r& B, const Vector3r& C);
	// compute d0,d1 for line defined by point A and _unit_ vector u, and sphere by center C and radius r
	// return number of intersection points (0,1,2) and sets accordingly either nothing, d0, or d0 and d1 to parameters determining points on the line (A+ti*u)
	// relTol causes points to be merged if they are closer than relTol*r
	static int lineSphereIntersection(const Vector3r& A, const Vector3r& u, const Vector3r& C, const Real r, Real& t0, Real& t1, Real relTol=1e-6);
	// return barycentric coordinates of a point (must be in-plane) on a triangle in space
	static Vector3r triangleBarycentrics(const Vector3r& x, const Vector3r& A, const Vector3r& B, const Vector3r& C);
	struct Colormap{
		std::string name;
		Real rgb[256*3];
	};
	static const vector<Colormap> colormaps;

	static constexpr int unitSphereTri20_maxTesselationLevel=3;
	// return vertices and face indices for unit sphere
	// if level is 0, return icosahedron, higher levels return tesselated icosahedron
	// the maximum level is in unitSphereTri20_maxTesselationLevel
	// if higher level is demanded, an exception is thrown
	// the tesselation is computed on-demand and cached for later use
	static std::tuple<const vector<Vector3r>&,const vector<Vector3i>&> unitSphereTri20(int level);

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
