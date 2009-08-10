// © 2009 Václav Šmilauer <eudoxos@arcig.cz>

#include<vector>
#include<string>	
using namespace std; // sorry

#ifdef YADE_PYTHON
	#include<boost/python.hpp>
	#include<yade/extra/boost_python_len.hpp>
	using namespace boost;
#endif

#include<boost/foreach.hpp>
#ifndef FOREACH
	#define FOREACH BOOST_FOREACH
#endif

#include<yade/lib-base/Logging.hpp>
#include<yade/lib-base/yadeWm3Extra.hpp>

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
	struct Sph{
		Vector3r c; Real r;
		Sph(const Vector3r& _c, Real _r): c(_c), r(_r){};
		#ifdef YADE_PYTHON
			python::tuple asTuple() const { return python::make_tuple(c,r); }
		#endif
	};
	std::vector<Sph> pack;
	Vector3r cellSize;
	SpherePack(): cellSize(Vector3r::ZERO){};
	#ifdef YADE_PYTHON
		SpherePack(const python::list& l):cellSize(Vector3r::ZERO){ fromList(l); }
	#endif
	// add single sphere
	void add(const Vector3r& c, Real r){ pack.push_back(Sph(c,r)); }

	// I/O
	#ifdef YADE_PYTHON
		void fromList(const python::list& l);
		python::list toList() const;
		python::list toList_pointsAsTuples() const;
	#endif
	void fromFile(const string file);
	void toFile(const string file) const;
	void fromSimulation();

	// random generation
	long makeCloud(Vector3r min, Vector3r max, Real rMean, Real rFuzz, size_t num, bool periodic=false);

	// spatial characteristics
	Vector3r dim() const {Vector3r mn,mx; aabb(mn,mx); return mx-mn;}
	#ifdef YADE_PYTHON
		python::tuple aabb_py() const { Vector3r mn,mx; aabb(mn,mx); return python::make_tuple(mn,mx); }
	#endif
	void aabb(Vector3r& mn, Vector3r& mx) const {
		Real inf=std::numeric_limits<Real>::infinity(); mn=Vector3r(inf,inf,inf); mx=Vector3r(-inf,-inf,-inf);
		FOREACH(const Sph& s, pack){ Vector3r r(s.r,s.r,s.r); mn=componentMinVector(mn,s.c-r); mx=componentMaxVector(mx,s.c+r);}
	}
	Vector3r midPt() const {Vector3r mn,mx; aabb(mn,mx); return .5*(mn+mx);}
	Real relDensity() const {
		Real sphVol=0; Vector3r dd=dim();
		FOREACH(const Sph& s, pack) sphVol+=pow(s.r,3);
		sphVol*=(4/3.)*Mathr::PI;
		return sphVol/(dd[0]*dd[1]*dd[2]);
	}

	// transformations
	void translate(const Vector3r& shift){ FOREACH(Sph& s, pack) s.c+=shift; }
	void rotate(const Vector3r& axis, Real angle){ Vector3r mid=midPt(); Quaternionr q(axis,angle); FOREACH(Sph& s, pack) s.c=q*(s.c-mid)+mid; }
	void scale(Real scale){ Vector3r mid=midPt(); FOREACH(Sph& s, pack) {s.c=scale*(s.c-mid)+mid; s.r*=abs(scale); } }

	// iteration 
	#ifdef YADE_PYTHON
		size_t len() const{ return pack.size(); }
		python::tuple getitem(size_t idx){ if(idx<0 || idx>=pack.size()) throw runtime_error("Index "+lexical_cast<string>(idx)+" out of range 0.."+lexical_cast<string>(pack.size()-1)); return pack[idx].asTuple(); }
		struct iterator{
			const SpherePack& sPack; size_t pos;
			iterator(const SpherePack& _sPack): sPack(_sPack), pos(0){}
			iterator iter(){ return *this;}
			python::tuple next(){
				if(pos==sPack.pack.size()){ PyErr_SetNone(PyExc_StopIteration); python::throw_error_already_set(); }
				return sPack.pack[pos++].asTuple();
			}
		};
		SpherePack::iterator getIterator() const{ return SpherePack::iterator(*this);};
	#endif
	DECLARE_LOGGER;
};

