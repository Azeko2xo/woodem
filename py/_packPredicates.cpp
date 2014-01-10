// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#include<woo/lib/base/Logging.hpp>
#include<woo/lib/base/Math.hpp>
#include<woo/lib/base/Types.hpp>
#include<woo/lib/pyutil/doc_opts.hpp>
#include<woo/lib/sphere-pack/SpherePack.hpp>
#include<woo/core/Master.hpp>

namespace py=boost::python;
#ifdef WOO_LOG4CXX
	static log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("woo.pack.predicates");
#endif

/*
This file contains various predicates that say whether a given point is within the solid,
or, not closer than "pad" to its boundary, if pad is nonzero
Besides the (point,pad) operator, each predicate defines aabb() method that returns
(min,max) tuple defining minimum and maximum point of axis-aligned bounding box 
for the predicate.

These classes are primarily used for woo.pack.* functions creating packings.
See examples/regular-sphere-pack/regular-sphere-pack.py for an example.

*/

struct Predicate{
	public:
		virtual bool operator() (const Vector3r& pt,Real pad=0.) const = 0;
		virtual AlignedBox3r aabb() const = 0;
		Vector3r dim() const { return aabb().sizes(); }
		Vector3r center() const { return aabb().center(); }
};

/* Since we want to make Predicate::operator() and Predicate::aabb() callable from c++ on py::object
with the right virtual method resolution, we have to wrap the class in the following way. See 
http://www.boost.org/doc/libs/1_38_0/libs/python/doc/tutorial/doc/html/python/exposing.html for documentation
on exposing virtual methods.

This makes it possible to derive a python class from Predicate, override its aabb() method, for instance,
and use it in PredicateUnion, which will call the python implementation of aabb() as it should. This
approach is used in the inGtsSurface class defined in pack.py.

See scripts/test/gts-operators.py for an example.

NOTE: you still have to call base class ctor in your class' ctor derived in python, e.g.
super(inGtsSurface,self).__init__() so that virtual methods work as expected.
*/
struct PredicateWrap: Predicate, py::wrapper<Predicate>{
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE { return this->get_override("__call__")(pt,pad);}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { return this->get_override("aabb")(); }
};

/*********************************************************************************
****************** Boolean operations on predicates ******************************
*********************************************************************************/

//const Predicate& obj2pred(py::object obj){ return py::extract<const Predicate&>(obj)();}

class PredicateBoolean: public Predicate{
	protected:
		const shared_ptr<Predicate> A,B;
	public:
		PredicateBoolean(const shared_ptr<Predicate>& _A, const shared_ptr<Predicate>& _B): A(_A), B(_B){}
		const shared_ptr<Predicate> getA(){ return A;}
		const shared_ptr<Predicate> getB(){ return B;}
};

// http://www.linuxtopia.org/online_books/programming_books/python_programming/python_ch16s03.html
class PredicateUnion: public PredicateBoolean{
	public:
		PredicateUnion(const shared_ptr<Predicate>& _A, const shared_ptr<Predicate>& _B): PredicateBoolean(_A,_B){}
		bool operator()(const Vector3r& pt,Real pad) const WOO_CXX11_OVERRIDE {return (*A)(pt,pad)||(*B)(pt,pad);}
		AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { return A->aabb().merged(B->aabb()); }
};
PredicateUnion makeUnion(const shared_ptr<Predicate>& A, const shared_ptr<Predicate>& B){ return PredicateUnion(A,B);}

class PredicateIntersection: public PredicateBoolean{
	public:
		PredicateIntersection(const shared_ptr<Predicate>& _A, const shared_ptr<Predicate>& _B): PredicateBoolean(_A,_B){}
		bool operator()(const Vector3r& pt,Real pad) const WOO_CXX11_OVERRIDE {return (*A)(pt,pad) && (*B)(pt,pad);}
		AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { return AlignedBox3r(A->aabb()).intersection(B->aabb()); }
};
PredicateIntersection makeIntersection(const shared_ptr<Predicate>& A, const shared_ptr<Predicate>& B){ return PredicateIntersection(A,B);}

class PredicateDifference: public PredicateBoolean{
	public:
		PredicateDifference(const shared_ptr<Predicate>& _A, const shared_ptr<Predicate>& _B): PredicateBoolean(_A,_B){}
		bool operator()(const Vector3r& pt,Real pad) const WOO_CXX11_OVERRIDE {return (*A)(pt,pad) && !(*B)(pt,-pad);}
		AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { return A->aabb(); }
};
PredicateDifference makeDifference(const shared_ptr<Predicate>& A, const shared_ptr<Predicate>& B){ return PredicateDifference(A,B);}

class PredicateSymmetricDifference: public PredicateBoolean{
	public:
		PredicateSymmetricDifference(const shared_ptr<Predicate>& _A, const shared_ptr<Predicate>& _B): PredicateBoolean(_A,_B){}
		bool operator()(const Vector3r& pt,Real pad) const WOO_CXX11_OVERRIDE {bool inA=(*A)(pt,pad), inB=(*B)(pt,pad); return (inA && !inB) || (!inA && inB);}
		AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { return AlignedBox3r(A->aabb()).extend(B->aabb()); }
};
PredicateSymmetricDifference makeSymmetricDifference(const shared_ptr<Predicate>& A, const shared_ptr<Predicate>& B){ return PredicateSymmetricDifference(A,B);}

/*********************************************************************************
****************************** Primitive predicates ******************************
*********************************************************************************/


/*! Sphere predicate */
class inSphere: public Predicate {
	Vector3r center; Real radius;
public:
	inSphere(const Vector3r& _center, Real _radius){center=_center; radius=_radius;}
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE { return ((pt-center).norm()-pad<=radius-pad); }
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE {return AlignedBox3r(Vector3r(center[0]-radius,center[1]-radius,center[2]-radius),Vector3r(center[0]+radius,center[1]+radius,center[2]+radius));}
};

class inAlignedHalfspace: public Predicate{
	short axis; Real coord; bool lower;
public:
	inAlignedHalfspace(int _axis, const Real& _coord, bool _lower=true): axis(_axis), coord(_coord), lower(_lower){
		if(!(axis==0 || axis==1 || axis==2)) throw std::runtime_error("inAlignedHalfSpace.axis: must be in {0,1,2} (not "+to_string(axis)+")");
	}
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE {
		if(lower){
			return pt[axis]+pad<coord;
		} else {
			return pt[axis]-pad>coord;
		}
	}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { AlignedBox3r ret(Vector3r(-Inf,-Inf,-Inf),Vector3r(Inf,Inf,Inf)); if(lower) ret.max()[axis]=coord; else ret.min()[axis]=coord; return ret; }
};

class inAxisRange: public Predicate{
	short axis; Vector2r range;
	public:
		inAxisRange(int _axis, const Vector2r& _range): axis(_axis), range(_range ){
			if(!(axis==0 || axis==1 || axis==2)) throw std::runtime_error("inAxisRange.axis: must be in {0,1,2} (not "+to_string(axis)+")");
		}
		bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE {
			return (pt[axis]-pad>=range[0] && pt[axis]+pad<=range[1]);
		}
		AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { AlignedBox3r ret(Vector3r(-Inf,-Inf,-Inf),Vector3r(Inf,Inf,Inf)); ret.min()[axis]=range[0]; ret.max()[axis]=range[1]; return ret; }
};

/*! Axis-aligned box predicate */
class inAlignedBox: public Predicate{
	Vector3r mn, mx;
public:
	inAlignedBox(const Vector3r& _mn, const Vector3r& _mx): mn(_mn), mx(_mx) {}
	inAlignedBox(const AlignedBox3r& box): mn(box.min()), mx(box.max()) {}
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE {
		return
			mn[0]+pad<=pt[0] && mx[0]-pad>=pt[0] &&
			mn[1]+pad<=pt[1] && mx[1]-pad>=pt[1] &&
			mn[2]+pad<=pt[2] && mx[2]-pad>=pt[2];
	}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { return AlignedBox3r(mn,mx); }
};

class inOrientedBox: public Predicate{
	Vector3r pos;
	Quaternionr ori;
	AlignedBox3r box;
public:
	inOrientedBox(const Vector3r& _pos, const Quaternionr& _ori, const AlignedBox3r& _box): pos(_pos),ori(_ori),box(_box){};
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE{
		// shrunk box
		AlignedBox3r box2; box2.min()=box.min()+Vector3r::Ones()*pad; box2.max()=box.max()-Vector3r::Ones()*pad;
		return box2.contains(ori.conjugate()*(pt-pos));
	}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE {
		AlignedBox3r ret;
		// box containing all corners after transformation
		for(const auto& corner: {AlignedBox3r::BottomLeftFloor, AlignedBox3r::BottomRightFloor, AlignedBox3r::TopLeftFloor, AlignedBox3r::TopRightFloor, AlignedBox3r::BottomLeftCeil, AlignedBox3r::BottomRightCeil, AlignedBox3r::TopLeftCeil, AlignedBox3r::TopRightCeil}) ret.extend(pos+ori*box.corner(corner));
		return ret;
	}
};

class inParallelepiped: public Predicate{
	Vector3r n[6]; // outer normals, for -x, +x, -y, +y, -z, +z
	Vector3r pts[6]; // points on planes
	Vector3r mn,mx;
public:
	inParallelepiped(const Vector3r& o, const Vector3r& a, const Vector3r& b, const Vector3r& c){
		Vector3r A(o), B(a), C(a+(b-o)), D(b), E(c), F(c+(a-o)), G(c+(a-o)+(b-o)), H(c+(b-o));
		Vector3r x(B-A), y(D-A), z(E-A);
		n[0]=-y.cross(z).normalized(); n[1]=-n[0]; pts[0]=A; pts[1]=B;
		n[2]=-z.cross(x).normalized(); n[3]=-n[2]; pts[2]=A; pts[3]=D;
		n[4]=-x.cross(y).normalized(); n[5]=-n[4]; pts[4]=A; pts[5]=E;
		// bounding box
		Vector3r vertices[8]={A,B,C,D,E,F,G,H};
		mn=mx=vertices[0];
		for(int i=1; i<8; i++){ mn=mn.array().min(vertices[i].array()).matrix(); mx=mx.array().max(vertices[i].array()).matrix(); }
	}
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE {
		for(int i=0; i<6; i++) if((pt-pts[i]).dot(n[i])>-pad) return false;
		return true;
	}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE { return AlignedBox3r(mn,mx); }
};

/*! Arbitrarily oriented cylinder predicate */
class inCylinder: public Predicate{
	Vector3r c1,c2,c12; Real radius,ht;
public:
	inCylinder(const Vector3r& _c1, const Vector3r& _c2, Real _radius){c1=_c1; c2=_c2; c12=c2-c1; radius=_radius; ht=c12.norm(); }
	bool operator()(const Vector3r& pt, Real pad=0.) const {
		Real u=(pt.dot(c12)-c1.dot(c12))/(ht*ht); // normalized coordinate along the c1--c2 axis
		if((u*ht<0+pad) || (u*ht>ht-pad)) return false; // out of cylinder along the axis
		Real axisDist=((pt-c1).cross(pt-c2)).norm()/ht;
		if(axisDist>radius-pad) return false;
		return true;
	}
	AlignedBox3r aabb() const {
		// see http://www.gamedev.net/community/forums/topic.asp?topic_id=338522&forum_id=20&gforum_id=0 for the algorithm
		const Vector3r& A(c1); const Vector3r& B(c2); 
		Vector3r k(
			sqrt((pow(A[1]-B[1],2)+pow(A[2]-B[2],2)))/ht,
			sqrt((pow(A[0]-B[0],2)+pow(A[2]-B[2],2)))/ht,
			sqrt((pow(A[0]-B[0],2)+pow(A[1]-B[1],2)))/ht);
		Vector3r mn=A.array().min(B.array()).matrix(), mx=A.array().max(B.array()).matrix();
		return AlignedBox3r((mn-radius*k).eval(),(mx+radius*k).eval());
	}
	string __str__() const {
		std::ostringstream oss;
		oss<<"<woo.pack.inCylinder @ "<<this<<", A="<<c1.transpose()<<", B="<<c2.transpose()<<", radius="<<radius<<">";
		return oss.str();
	}
};

class inCylSector: public Predicate{
	Vector3r pos; Quaternionr ori; Vector2r rrho, ttheta, zz; Quaternionr oriConj;
	bool limZ, limTh, limRho;
public:
	inCylSector(const Vector3r& _pos, const Quaternionr& _ori, const Vector2r& _rrho=Vector2r::Zero(), const Vector2r& _ttheta=Vector2r::Zero(), const Vector2r& _zz=Vector2r::Zero()): pos(_pos), ori(_ori), rrho(_rrho), ttheta(_ttheta), zz(_zz) {
		oriConj=ori.conjugate();
		for(int i:{0,1}) ttheta[i]=CompUtils::wrapNum(ttheta[i],2*M_PI);
		limRho=(rrho!=Vector2r::Zero());
		limTh=(ttheta!=Vector2r::Zero());
		limZ=(zz!=Vector2r::Zero());
	};
	bool operator()(const Vector3r& pt, Real pad=0.) const {
		Vector3r l3=oriConj*(pt-pos);
		Real z=l3[2];
		if(limZ && (z-pad<zz[0] || z+pad>zz[1])) return false;
		Real th=CompUtils::wrapNum(atan2(l3[1],l3[0]),2*M_PI);
		Real rho=l3.head<2>().norm();
		if(limRho && (rho-pad<rrho[0] || rho+pad>rrho[1])) return false;
		if(limTh){
			Real t0=CompUtils::wrapNum(ttheta[0]+(rho>0?pad/rho:0.),2*M_PI);
			Real t1=CompUtils::wrapNum(ttheta[1]-(rho>0?pad/rho:0.),2*M_PI);
			// we are wrapping around 2π
			// |===t1........t0===|
			if(t1<t0) return (th>=t1 || th<=t0);
			// |...t0========t1...|
			return (th>=t0 && th<=t1);
		};
		return true;
	}
	AlignedBox3r aabb() const {
		// this aabb is very pessimistic, but probably not really needed
		// if(!limRho || !limTh || !limZ) 
		return AlignedBox3r(Vector3r(-Inf,-Inf,-Inf),Vector3r(Inf,Inf,Inf));
		#if 0
			AlignedBox3r bb;
			auto cyl2glob=[&](const Real& rho, const Real& theta, const Real& z)->Vector3r{ return ori*Vector3r(rho*cos(theta),rho*sin(theta),z)+pos; }
			for(Real z:{zz[0],zz[1]){
				bb.extend(cyl2glob(0,0,z)); // center
				bb.extend(cyl2glob(
			}
		#endif
	};
};

/*! Oriented hyperboloid predicate (cylinder as special case).

See http://mathworld.wolfram.com/Hyperboloid.html for the parametrization and meaning of symbols
*/
class inHyperboloid: public Predicate{
	Vector3r c1,c2,c12; Real R,a,ht,c;
public:
	inHyperboloid(const Vector3r& _c1, const Vector3r& _c2, Real _R, Real _r){
		c1=_c1; c2=_c2; R=_R; a=_r;
		c12=c2-c1; ht=c12.norm();
		Real uMax=sqrt(pow(R/a,2)-1); c=ht/(2*uMax);
	}
	// WARN: this is not accurate, since padding is taken as perpendicular to the axis, not the the surface
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE {
		Real v=(pt.dot(c12)-c1.dot(c12))/(ht*ht); // normalized coordinate along the c1--c2 axis
		if((v*ht<0+pad) || (v*ht>ht-pad)) return false; // out of cylinder along the axis
		Real u=(v-.5)*ht/c; // u from the wolfram parametrization; u is 0 in the center
		Real rHere=a*sqrt(1+u*u); // pad is taken perpendicular to the axis, not to the surface (inaccurate)
		Real axisDist=((pt-c1).cross(pt-c2)).norm()/ht;
		if(axisDist>rHere-pad) return false;
		return true;
	}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE {
		// the lazy way
		return inCylinder(c1,c2,R).aabb();
	}
};

/*! Axis-aligned ellipsoid predicate */
class inEllipsoid: public Predicate{
	Vector3r c, abc;
public:
	inEllipsoid(const Vector3r& _c, const Vector3r& _abc) {c=_c; abc=_abc;}
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE {
		//Define the ellipsoid X-coordinate of given Y and Z
		Real x = sqrt((1-pow((pt[1]-c[1]),2)/((abc[1]-pad)*(abc[1]-pad))-pow((pt[2]-c[2]),2)/((abc[2]-pad)*(abc[2]-pad)))*((abc[0]-pad)*(abc[0]-pad)))+c[0]; 
		Vector3r edgeEllipsoid(x,pt[1],pt[2]); // create a vector of these 3 coordinates
		//check whether given coordinates lie inside ellipsoid or not
		if ((pt-c).norm()<=(edgeEllipsoid-c).norm()) return true;
		else return false;
	}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE {
		const Vector3r& center(c); const Vector3r& ABC(abc);
		return AlignedBox3r(Vector3r(center[0]-ABC[0],center[1]-ABC[1],center[2]-ABC[2]),Vector3r(center[0]+ABC[0],center[1]+ABC[1],center[2]+ABC[2]));
	}
};

/*! Negative notch predicate.

Use intersection (& operator) of another predicate with notInNotch to create notched solid.


		
		geometry explanation:
		
			c: the center
			normalHalfHt (in constructor): A-C
			inside: perpendicular to notch edge, points inside the notch (unit vector)
			normal: perpendicular to inside, perpendicular to both notch planes
			edge: unit vector in the direction of the edge

		          ↑ distUp        A
		-------------------------
		                        | C
		         inside(unit) ← * → distInPlane
		                        |
		-------------------------
		          ↓ distDown      B

*/
class notInNotch: public Predicate{
	Vector3r c, edge, normal, inside; Real aperture;
public:
	notInNotch(const Vector3r& _c, const Vector3r& _edge, const Vector3r& _normal, Real _aperture){
		c=_c;
		edge=_edge; edge.normalize();
		normal=_normal; normal-=edge*edge.dot(normal); normal.normalize();
		inside=edge.cross(normal);
		aperture=_aperture;
		// LOG_DEBUG("edge="<<edge<<", normal="<<normal<<", inside="<<inside<<", aperture="<<aperture);
	}
	bool operator()(const Vector3r& pt, Real pad=0.) const {
		Real distUp=normal.dot(pt-c)-aperture/2, distDown=-normal.dot(pt-c)-aperture/2, distInPlane=-inside.dot(pt-c);
		// LOG_DEBUG("pt="<<pt<<", distUp="<<distUp<<", distDown="<<distDown<<", distInPlane="<<distInPlane);
		if(distInPlane>=pad) return true;
		if(distUp     >=pad) return true;
		if(distDown   >=pad) return true;
		if(distInPlane<0) return false;
		if(distUp  >0) return sqrt(pow(distInPlane,2)+pow(distUp,2))>=pad;
		if(distDown>0) return sqrt(pow(distInPlane,2)+pow(distUp,2))>=pad;
		// between both notch planes, closer to the edge than pad (distInPlane<pad)
		return false;
	}
	// This predicate is not bounded, return infinities
	AlignedBox3r aabb() const {
		return AlignedBox3r(Vector3r(-Inf,-Inf,-Inf),Vector3r(Inf,Inf,Inf));
	}
};

shared_ptr<SpherePack> SpherePack_filtered(const shared_ptr<SpherePack>& sp, const shared_ptr<Predicate>& p, bool recenter=true){
	auto ret=make_shared<SpherePack>();
	ret->cellSize=sp->cellSize;
	size_t N=sp->pack.size();
	Vector3r off=Vector3r::Zero();
	auto sbox=sp->aabb();
	auto pbox=p->aabb();
	if(recenter && !isinf(pbox.sizes().maxCoeff())){
		off=pbox.center()-sbox.center();
		pbox.translate(off);
		sbox.translate(off);
	}
	// do not warn for inifinite predicates, which are always larger than the packing
	for(int ax:{0,1,2}){
		if(pbox.min()[ax]!=-Inf && pbox.min()[ax]<sbox.min()[ax]) LOG_WARN("SpherePack.filtered: axis="<<ax<<", packing aabb (min="<<sbox.min()[ax]<<") outside of the predicate aabb (min="<<pbox.min()[ax]<<")");
		if(pbox.max()[ax]!=Inf && pbox.max()[ax]>sbox.max()[ax]) LOG_WARN("SpherePack.filtered: axis="<<ax<<", packing aabb (max="<<sbox.max()[ax]<<") outside of the predicate aabb (max="<<pbox.max()[ax]<<").");
	}
	// if(!sbox.contains(pbox)) LOG_WARN("Packing's box does not fully contain box of the predicate");
	//#if dimP[0]>dimS[0] or dimP[1]>dimS[1] or dimP[2]>dimS[2]: warnings.warn("Packing's dimension (%s) doesn't fully contain dimension of the predicate (%s)."%(dimS,dimP))

	if(!sp->hasClumps()){
		for(size_t i=0; i<N; i++){
			if(!(*p)(sp->pack[i].c+off,sp->pack[i].r)) continue; // sphere is outside
			ret->add(sp->pack[i].c+off,sp->pack[i].r);
		}
		return ret;
	}
	// with clumps
	std::set<int> delSph, delClump;
	for(size_t i=0; i<N; i++){
		if((*p)(sp->pack[i].c+off,sp->pack[i].r)) continue; // sphere is inside
		if(sp->pack[i].clumpId>=0) delClump.insert(sp->pack[i].clumpId);
		else delSph.insert(i);
	}
	for(size_t i=0; i<N; i++){
		const int& clumpId(sp->pack[i].clumpId);
		// either the sphere as such, or the whole clump was to be removed
		if((clumpId<0 && delSph.count(i)>0) || (clumpId>=0 && delClump.count(clumpId)>0)) continue; // skip this one
		ret->add(sp->pack[i].c+off,sp->pack[i].r,clumpId);
	}
	return ret;
}

// this is only activated in the SCons build
#if defined(WOO_GTS)
// HACK
#include"3rd-party/pygts-0.3.1/pygts.h"

/* Helper function for inGtsSurface::aabb() */
static void vertex_aabb(GtsVertex *vertex, pair<Vector3r,Vector3r> *bb)
{
	GtsPoint *_p=GTS_POINT(vertex);
	Vector3r p(_p->x,_p->y,_p->z);
	bb->first=bb->first.array().min(p.array()).matrix();
	bb->second=bb->second.array().max(p.array()).matrix();
}

/*
This class plays tricks getting around pyGTS to get GTS objects and cache bb tree to speed
up point inclusion tests. For this reason, we have to link with _gts.so (see corresponding
SConscript file), which is at the same time the python module.
*/
class inGtsSurface: public Predicate{
	py::object pySurf; // to hold the reference so that surf is valid
	GtsSurface *surf;
	bool is_open, noPad, noPadWarned;
	GNode* tree;
public:
	inGtsSurface(py::object _surf, bool _noPad=false): pySurf(_surf), noPad(_noPad), noPadWarned(false) {
		if(!pygts_surface_check(_surf.ptr())) throw invalid_argument("Ctor must receive a gts.Surface() instance."); 
		surf=PYGTS_SURFACE_AS_GTS_SURFACE(PYGTS_SURFACE(_surf.ptr()));
	 	if(!gts_surface_is_closed(surf)) throw invalid_argument("Surface is not closed.");
		is_open=gts_surface_volume(surf)<0.;
		if((tree=gts_bb_tree_surface(surf))==NULL) throw runtime_error("Could not create GTree.");
	}
	~inGtsSurface(){g_node_destroy(tree);}
	AlignedBox3r aabb() const WOO_CXX11_OVERRIDE {
		Real inf=std::numeric_limits<Real>::infinity();
		pair<Vector3r,Vector3r> bb; bb.first=Vector3r(inf,inf,inf); bb.second=Vector3r(-inf,-inf,-inf);
		gts_surface_foreach_vertex(surf,(GtsFunc)vertex_aabb,&bb);
		return AlignedBox3r(bb.first,bb.second);
	}
	bool ptCheck(const Vector3r& pt) const{
		GtsPoint gp; gp.x=pt[0]; gp.y=pt[1]; gp.z=pt[2];
		return (bool)gts_point_is_inside_surface(&gp,tree,is_open);
	}
	bool operator()(const Vector3r& pt, Real pad=0.) const WOO_CXX11_OVERRIDE {
		if(noPad){
			if(pad!=0. && noPadWarned) LOG_WARN("inGtsSurface constructed with noPad; requested non-zero pad set to zero.");
			return ptCheck(pt);
		}
		return ptCheck(pt) && ptCheck(pt-Vector3r(pad,0,0)) && ptCheck(pt+Vector3r(pad,0,0)) && ptCheck(pt-Vector3r(0,pad,0))&& ptCheck(pt+Vector3r(0,pad,0)) && ptCheck(pt-Vector3r(0,0,pad))&& ptCheck(pt+Vector3r(0,0,pad));
	}
	py::object surface() const {return pySurf; }
};

#endif

WOO_PYTHON_MODULE(_packPredicates);
BOOST_PYTHON_MODULE(_packPredicates){
	py::scope().attr("__doc__")="Spatial predicates for volumes (defined analytically or by triangulation).";
	WOO_SET_DOCSTRING_OPTS;
	// base predicate class
	py::class_<PredicateWrap,shared_ptr<PredicateWrap>,/* necessary, as methods are pure virtual*/ boost::noncopyable>("Predicate")
		.def("__call__",py::pure_virtual(&Predicate::operator()),(py::arg("pt"),py::arg("pad")=0.))
		.def("aabb",py::pure_virtual(&Predicate::aabb))
		.def("dim",&Predicate::dim)
		.def("center",&Predicate::center)
		.def("__or__",makeUnion).def("__and__",makeIntersection).def("__sub__",makeDifference).def("__xor__",makeSymmetricDifference);
	// boolean operations
	py::class_<PredicateBoolean,shared_ptr<PredicateBoolean>,py::bases<Predicate>,boost::noncopyable>("PredicateBoolean","Boolean operation on 2 predicates (abstract class)",py::no_init)
		.add_property("A",&PredicateBoolean::getA).add_property("B",&PredicateBoolean::getB);
	py::class_<PredicateUnion,shared_ptr<PredicateUnion>,py::bases<PredicateBoolean> >("PredicateUnion","Union (non-exclusive disjunction) of 2 predicates. A point has to be inside any of the two predicates to be inside. Can be constructed using the ``|`` operator on predicates: ``pred1 | pred2``.",py::init<shared_ptr<Predicate>,shared_ptr<Predicate>>());
	py::class_<PredicateIntersection,shared_ptr<PredicateIntersection>,py::bases<PredicateBoolean>>("PredicateIntersection","Intersection (conjunction) of 2 predicates. A point has to be inside both predicates. Can be constructed using the ``&`` operator on predicates: ``pred1 & pred2``.",py::init<shared_ptr<Predicate>,shared_ptr<Predicate>>());
	py::class_<PredicateDifference,shared_ptr<PredicateDifference>,py::bases<PredicateBoolean> >("PredicateDifference","Difference (conjunction with negative predicate) of 2 predicates. A point has to be inside the first and outside the second predicate. Can be constructed using the ``-`` operator on predicates: ``pred1 - pred2``.",py::init<shared_ptr<Predicate>,shared_ptr<Predicate>>());
	py::class_<PredicateSymmetricDifference,shared_ptr<PredicateSymmetricDifference>,py::bases<PredicateBoolean> >("PredicateSymmetricDifference","SymmetricDifference (exclusive disjunction) of 2 predicates. A point has to be in exactly one predicate of the two. Can be constructed using the ``^`` operator on predicates: ``pred1 ^ pred2``.",py::init<shared_ptr<Predicate>,shared_ptr<Predicate>>());
	// primitive predicates
	py::class_<inSphere,shared_ptr<inSphere>,py::bases<Predicate>>("inSphere","Sphere predicate.",py::init<const Vector3r&,Real>(py::args("center","radius"),"Ctor taking center (as a 3-tuple) and radius"));
	py::class_<inAlignedBox,shared_ptr<inAlignedBox>,py::bases<Predicate>>("inAlignedBox","Axis-aligned box predicate",py::init<const Vector3r&,const Vector3r&>(py::args("minAABB","maxAABB"),"Ctor taking minumum and maximum points of the box (as 3-tuples).")).def(py::init<const AlignedBox3r&>(py::arg("box")));
	py::class_<inOrientedBox,shared_ptr<inOrientedBox>,py::bases<Predicate>>("inOrientedBox","Arbitrarily oriented box specified as local coordinates (pos,ori) and aligned box in those local coordinates.",py::init<const Vector3r&,const Quaternionr&, const AlignedBox3r&>(py::args("pos","ori","box"),"Ctor taking position and orientation of the local system, and aligned box in local coordinates."));
	py::class_<inCylSector,shared_ptr<inCylSector>,py::bases<Predicate>>("inCylSector","Sector of an arbitrarily oriented cylinder in 3d, limiting cylindrical coordinates :math:`\\rho`, :math:`\\theta`, :math:`z`; all coordinate limits are optional.",py::init<const Vector3r&, const Quaternionr&, py::optional<const Vector2r&, const Vector2r&, const Vector2r&>>(py::args("pos","ori","rrho","ttheta","zz"),"Ctor taking position and orientation of the local system, and aligned box in local coordinates."));
	py::class_<inAlignedHalfspace,shared_ptr<inAlignedHalfspace>,py::bases<Predicate>>("inAlignedHalfspace","Half-space given by coordinate at some axis.",py::init<int, const Real&, py::optional<bool>>(py::args("axis","coord","lower"),"Ctor taking axis (0,1,2 for :math:`x`, :math:`y`, :math:`z` respectively), the coordinate along that axis, and whether the *lower* half (or the upper half, if *lower* is false) is considered."));
	py::class_<inAxisRange,shared_ptr<inAxisRange>,py::bases<Predicate>>("inAxisRange","Range of coordinate along some axis, effectively defining two axis-aligned enclosing planes.",py::init<int, const Vector2r&>(py::args("axis","range"),"Ctor taking axis (0,1,2 for :math:`x`, :math:`y`, :math:`z` respectively), and coordinate range along that axis."));
	py::class_<inParallelepiped,shared_ptr<inParallelepiped>,py::bases<Predicate>>("inParallelepiped","Parallelepiped predicate",py::init<const Vector3r&,const Vector3r&, const Vector3r&, const Vector3r&>(py::args("o","a","b","c"),"Ctor taking four points: ``o`` (for origin) and then ``a``, ``b``, ``c`` which define endpoints of 3 respective edges from ``o``."));
	py::class_<inCylinder,shared_ptr<inCylinder>,py::bases<Predicate>>("inCylinder","Cylinder predicate",py::init<const Vector3r&,const Vector3r&,Real>(py::args("centerBottom","centerTop","radius"),"Ctor taking centers of the lateral walls (as 3-tuples) and radius.")).def("__str__",&inCylinder::__str__);
	py::class_<inHyperboloid,shared_ptr<inHyperboloid>,py::bases<Predicate> >("inHyperboloid","Hyperboloid predicate",py::init<const Vector3r&,const Vector3r&,Real,Real>(py::args("centerBottom","centerTop","radius","skirt"),"Ctor taking centers of the lateral walls (as 3-tuples), radius at bases and skirt (middle radius)."));
	py::class_<inEllipsoid,shared_ptr<inEllipsoid>,py::bases<Predicate> >("inEllipsoid","Ellipsoid predicate",py::init<const Vector3r&,const Vector3r&>(py::args("centerPoint","abc"),"Ctor taking center of the ellipsoid (3-tuple) and its 3 radii (3-tuple)."));
	py::class_<notInNotch,shared_ptr<notInNotch>,py::bases<Predicate> >("notInNotch","Outside of infinite, rectangle-shaped notch predicate",py::init<const Vector3r&,const Vector3r&,const Vector3r&,Real>(py::args("centerPoint","edge","normal","aperture"),"Ctor taking point in the symmetry plane, vector pointing along the edge, plane normal and aperture size.\nThe side inside the notch is edge×normal.\nNormal is made perpendicular to the edge.\nAll vectors are normalized at construction time.")); 
	#ifdef WOO_GTS
		py::class_<inGtsSurface,shared_ptr<inGtsSurface>,py::bases<Predicate> >("inGtsSurface","GTS surface predicate",py::init<py::object,py::optional<bool> >(py::args("surface","noPad"),"Ctor taking a gts.Surface() instance, which must not be modified during instance lifetime.\nThe optional noPad can disable padding (if set to True), which speeds up calls several times.\nNote: padding checks inclusion of 6 points along +- cardinal directions in the pad distance from given point, which is not exact."))
			.add_property("surf",&inGtsSurface::surface,"The associated gts.Surface object.");
	#endif
	py::def("SpherePack_filtered",SpherePack_filtered,(py::arg("sp"),py::arg("predicate"),py::arg("recenter")=true),"Return new :obj:`SpherePack` object, without any spheres which don't match *predicate*. Clumps are handled gracefully, i.e. if any of clump's spheres does not satisfy the predicate, the whole clump is taken away. If *recenter* is True (and none of the dimensions of the predicate is infinite), the packing will be translated to have center at the same point as the predicate.");
}
