#pragma once
#include<woo/pkg/dem/Particle.hpp>

struct RawShape: public Object{
	// extra ctor straight from shape
	RawShape(const shared_ptr<Shape>& sh);
	string pyStr() const WOO_CXX11_OVERRIDE {	return "<"+getClassName()+" @ "+boost::lexical_cast<string>(this)+": "+className+">"; }
	// if density is given, create DemData (and GlData) so that it is ready to be inserted into the simulation
	shared_ptr<Shape> toShape(Real density=NaN) const;
	#define woo_dem_RawShape__CLASS_BASE_DOC_ATTRS \
		RawShape,Object,"", \
		((string,className,"",,"Name of the Shape subclass.")) \
		((Vector3r,center,Vector3r::Zero(),,"Center of the bounding sphere.")) \
		((Real,radius,0.,,"Radius of the bounding sphere.")) \
		((vector<Real>,raw,,,"Raw data for shape reconstruction; the size of the array is shape-specific."))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_RawShape__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(RawShape);

#if 0
struct RawShapeClump: public Object{
	DECLARE_LOGGER;
	void postLoad(ClumpGeomBase&,void*);
	// may fail when called from postLoad, but not from ensureOk()
	void recompute(int div, bool failOk=false, bool fastOnly=false);
	void makeInvalid(){ volume=equivRad=NaN; inertia=Vector3r(NaN,NaN,NaN); pos=Vector3r::Zero(); ori=Quaternionr::Identity(); }
	bool isOk() const { return !isnan(volume); }
	void ensureOk() { if(!isOk()) recompute(div,/*failOk*/false); }
	std::tuple<shared_ptr<Node>,vector<shared_ptr<Particle>>> makeClump(const shared_ptr<Material>&, const Vector3r& pos, const Quaternionr& ori, int mask, Real scale=1.);
	py::tuple pyMakeClump(const shared_ptr<Material>& m, const Vector3r& p, const Quaternionr& o=Quaternionr::Identity(), Real scale=1., int mask=0){
		const auto& tup=makeClump(m,p,o,mask,scale); return py::make_tuple(std::get<0>(tup),std::get<1>(tup));
	}
	// static vector<shared_ptr<ClumpGeomBase>> fromSpherePack(const shared_ptr<SpherePack>& sp, int div=5);
	
	WOO_CLASS_BASE_DOC_ATTRS_PY(ClumpGeomBase,Object,"Defines geometry of spherical clumps. Each clump is described by spheres it is made of (position and radius).",
		((vector<Vector3r>,centers,,AttrTrait<Attr::triggerPostLoad>(),"Centers of constituent spheres, in clump-local coordinates."))
		((vector<Real>,radii,,AttrTrait<Attr::triggerPostLoad>(),"Radii of constituent spheres"))
		((vector<Vector2r>,scaleProb,,,"Used by particle generators: piecewise-linear function probability(equivRad) given as a sequence of x,y coordinates. If not given, constant function $p(d)=1$ is assumed. See the documentation of :obj:`woo.dem.PsdClumpGenerator` for details."))
		((Vector3r,pos,Vector3r::Zero(),AttrTrait<>().readonly().noDump(),"Centroid position (computed automatically)"))
		((Quaternionr,ori,Quaternionr::Identity(),AttrTrait<>().readonly().noDump(),"Principal axes orientation (computed automatically)"))
		((Real,volume,NaN,AttrTrait<>().readonly().noDump().volUnit(),"Volume (computed automatically)"))
		((Real,equivRad,NaN,AttrTrait<>().readonly().noDump().lenUnit(),"Equivalent radius of the clump (computed automatically) -- mean of radii of gyration, i.e. $\\frac{1}{3}\\sum \\sqrt{I_{ii}/V}$."))
		((Vector3r,inertia,Vector3r(NaN,NaN,NaN),AttrTrait<>().readonly().noDump(),"Geometrical inertia (computed with unit density)"))
		((int,div,5,AttrTrait<Attr::triggerPostLoad>().noDump(),"Sampling grid fineness, when computing volume and other properties, relative to the smallest sphere's radius. When zero or negative, assume spheres don't intersect and use a different algorithm (Steiner's theorem)."))
		, /* py*/
		.def("recompute",&ClumpGeomBase::recompute,(py::arg("div")=5,py::arg("failOk")=false,py::arg("fastOnly")=false),"Recompute principal axes of the clump, using *div* for subdivision (see :obj:`div` for the semantics). *failOk* (silently return in case of invalid data) and *fastOnly* (return if there is lots of cells in subdivision) are only to be used internally.")
		.def("makeClump",&ClumpGeomBase::pyMakeClump,(py::arg("mat"),py::arg("pos"),py::arg("ori")=Quaternionr::Identity(),py::arg("scale")=1.,py::arg("mask")=0),"Create particles as described by this clump geometry, positioned in *pos* and rotated with *ori*. Geometry will be scaled by *scale*. Returns tuple (Node,[Particle]).")
		.def("fromSpherePack",&ClumpGeomBase::fromSpherePack,(py::arg("pack"),py::arg("div")=5),"Return [ :obj:`ClumpGeomBase` ] which contain all clumps and spheres from given :obj:`SpherePack`.").staticmethod("fromSpherePack")
	);
};
WOO_REGISTER_OBJECT(ClumpGeomBase);
#endif

struct ShapePack: public Object{
	// sort the array according to the coordinate along *ax*
	// if *trimVol* is positive, discard all shapes after trimVol has been reached, in the order of increasing *ax*
	// void sort(const int& ax, const Real& trimVol=NaN);
	void fromDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, int mask);
	void toDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, const shared_ptr<Material>& mat, int mask=0, Real color=NaN);
	void saveTxt(const string& out) const;
	void loadTxt(const string& in);
	// mostly for testing only
	void add(vector<shared_ptr<Shape>>& ss);

	void postLoad(ShapePack&,void*);

	#define woo_dem_ShapePack__CLASS_BASE_DOC_ATTRS_PY \
		ShapePack,Object,"Representation of geometry of many particles, with the ability of text I/O. It is meant as a replacement for :obj:`woo.pack.SpherePack`, which only handles spherical particles.", \
		((Vector3r,cellSize,Vector3r::Zero(),,"Positive components signify periodic boundary along the respective axis.")) \
		((bool,movable,false,,"Whether the packing is movable, i.e. should be automatically recentered after filtered with a predicate.")) \
		((vector<vector<shared_ptr<RawShape>>>,rawShapes,,,"Shape objects of particles")) \
		((string,userData,,,"String of arbitrary user data to be loaded/saved with the ShapePack.")) \
		((string,loadFrom,,AttrTrait<Attr::triggerPostLoad>(),"If given when constructing the instance, the file is loaded immediately and the attribute is reset.")) \
		,/*py*/ .def("loadTxt",&ShapePack::loadTxt).def("saveTxt",&ShapePack::saveTxt).def("add",&ShapePack::add) \
		.def("fromDem",&ShapePack::fromDem,(py::arg("scene"),py::arg("dem"),py::arg("mask"))) \
		.def("toDem",&ShapePack::toDem,(py::arg("scene"),py::arg("dem"),py::arg("mat"),py::arg("mask")=0,py::arg("color")=NaN))

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapePack__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(ShapePack);
