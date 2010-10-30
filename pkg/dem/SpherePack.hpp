// © 2009 Václav Šmilauer <eudoxos@arcig.cz>

#pragma once

#include<vector>
#include<string>	
#include<limits>
using namespace std; // sorry

#include<boost/python.hpp>
#include<yade/extra/boost_python_len.hpp>
using namespace boost;

#include<boost/foreach.hpp>
#ifndef FOREACH
	#define FOREACH BOOST_FOREACH
#endif

#include<yade/lib-base/Logging.hpp>
#include<yade/lib-base/Math.hpp>

/*! Class representing geometry of spherical packing, with some utility functions.

*/
class SpherePack{
	// return coordinate wrapped to x0…x1, relative to x0; don't care about period
	// copied from PeriodicInsertionSortCollider
	Real cellWrapRel(const Real x, const Real x0, const Real x1){
		Real xNorm=(x-x0)/(x1-x0);
		return (xNorm-floor(xNorm))*(x1-x0);
	}
public:
	enum {RDIST_RMEAN, RDIST_POROSITY, RDIST_PSD};
	struct Sph{
		Vector3r c; Real r;
		Sph(const Vector3r& _c, Real _r): c(_c), r(_r){};
		python::tuple asTuple() const { return python::make_tuple(c,r); }
	};
	std::vector<Sph> pack;
	Vector3r cellSize;
	Real psdScaleExponent;
	SpherePack(): cellSize(Vector3r::Zero()), psdScaleExponent(2.5){};
	SpherePack(const python::list& l):cellSize(Vector3r::Zero()){ fromList(l); }
	// add single sphere
	void add(const Vector3r& c, Real r){ pack.push_back(Sph(c,r)); }

	// I/O
	void fromList(const python::list& l);
	python::list toList() const;
	#if 0
		python::list toList_pointsAsTuples() const;
	#endif
	void fromFile(const string file);
	void toFile(const string file) const;
	void fromSimulation();

	// random generation; if num<0, insert as many spheres as possible; if porosity>0, recompute meanRadius (porosity>0.65 recommended) and try generating this porosity with num spheres.
	long makeCloud(Vector3r min, Vector3r max, Real rMean=-1, Real rFuzz=0, int num=-1, bool periodic=false, Real porosity=-1, const vector<Real>& psdSizes=vector<Real>(), const vector<Real>& psdCumm=vector<Real>(), bool distributeMass=false);
	// return number of piece for x in piecewise function defined by cumm with non-decreasing elements ∈(0,1)
	// norm holds normalized coordinate withing the piece
	int psdGetPiece(Real x, const vector<Real>& cumm, Real& norm);

	// function to generate the packing based on a given psd
	long particleSD(Vector3r min, Vector3r max, Real rMean, bool periodic=false, string name="", int numSph=400, const vector<Real>& radii=vector<Real>(), const vector<Real>& passing=vector<Real>(),bool passingIsNotPercentageButCount=false);

	// interpolate a variable with power distribution (exponent -3) between two margin values, given uniformly distributed x∈(0,1)
	Real pow3Interp(Real x,Real a,Real b){ return pow(x*(pow(b,-2)-pow(a,-2))+pow(a,-2),-1./2); }

	// periodic repetition
	void cellRepeat(Vector3i count);
	void cellFill(Vector3r volume);

	// spatial characteristics
	Vector3r dim() const {Vector3r mn,mx; aabb(mn,mx); return mx-mn;}
	python::tuple aabb_py() const { Vector3r mn,mx; aabb(mn,mx); return python::make_tuple(mn,mx); }
	void aabb(Vector3r& mn, Vector3r& mx) const {
		Real inf=std::numeric_limits<Real>::infinity(); mn=Vector3r(inf,inf,inf); mx=Vector3r(-inf,-inf,-inf);
		FOREACH(const Sph& s, pack){ Vector3r r(s.r,s.r,s.r); mn=mn.cwise().min(s.c-r); mx=mx.cwise().max(s.c+r);}
	}
	Vector3r midPt() const {Vector3r mn,mx; aabb(mn,mx); return .5*(mn+mx);}
	Real relDensity() const {
		Real sphVol=0; Vector3r dd=dim();
		FOREACH(const Sph& s, pack) sphVol+=pow(s.r,3);
		sphVol*=(4/3.)*Mathr::PI;
		return sphVol/(dd[0]*dd[1]*dd[2]);
	}
	python::tuple psd(int bins=10, bool mass=false) const;

	// transformations
	void translate(const Vector3r& shift){ FOREACH(Sph& s, pack) s.c+=shift; }
	void rotate(const Vector3r& axis, Real angle){
		if(cellSize!=Vector3r::Zero()) { LOG_WARN("Periodicity reset when rotating periodic packing (non-zero cellSize="<<cellSize<<")"); cellSize=Vector3r::Zero(); }
		Vector3r mid=midPt(); Quaternionr q(AngleAxisr(angle,axis)); FOREACH(Sph& s, pack) s.c=q*(s.c-mid)+mid;
	}
	void scale(Real scale){ Vector3r mid=midPt(); cellSize*=scale; FOREACH(Sph& s, pack) {s.c=scale*(s.c-mid)+mid; s.r*=abs(scale); } }
	#if 0
		void shrinkMaxRelOverlap(Real maxRelOverlap);
		Real maxRelOverlap();
	#endif

	// iteration 
	size_t len() const{ return pack.size(); }
	python::tuple getitem(size_t idx){ if(idx<0 || idx>=pack.size()) throw runtime_error("Index "+lexical_cast<string>(idx)+" out of range 0.."+lexical_cast<string>(pack.size()-1)); return pack[idx].asTuple(); }
	struct _iterator{
		const SpherePack& sPack; size_t pos;
		_iterator(const SpherePack& _sPack): sPack(_sPack), pos(0){}
		_iterator iter(){ return *this;}
		python::tuple next(){
			if(pos==sPack.pack.size()){ PyErr_SetNone(PyExc_StopIteration); python::throw_error_already_set(); }
			return sPack.pack[pos++].asTuple();
		}
	};
	SpherePack::_iterator getIterator() const{ return SpherePack::_iterator(*this);};
	DECLARE_LOGGER;
};

