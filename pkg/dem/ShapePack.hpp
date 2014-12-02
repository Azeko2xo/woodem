#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/lib/sphere-pack/SpherePack.hpp> // for Predicate
#include<woo/lib/pyutil/converters.hpp>


struct SphereClumpGeom;

struct RawShape: public Object{
	// extra ctor straight from shape
	RawShape(const shared_ptr<Shape>& sh);
	string pyStr() const WOO_CXX11_OVERRIDE {	return "<"+getClassName()+" @ "+boost::lexical_cast<string>(this)+": "+className+">"; }
	// if density is given, create DemData (and GlData) so that it is ready to be inserted into the simulation
	shared_ptr<Shape> toShape(Real density=NaN, Real scale=1.) const;
	void translate(const Vector3r& offset);
	// void rotate(const Quaternionr& rot);
	shared_ptr<RawShape> copy() const;
	#define woo_dem_RawShape__CLASS_BASE_DOC_ATTRS_PY \
		RawShape,Object,"Object holding raw geometry data for one :obj:`Shape` in a uniform manner: center and radius of bounding sphere, plus an array of raw data. It is used as an intermediary shape-neutral storage format.", \
		((string,className,"",,"Name of the Shape subclass.")) \
		((Vector3r,center,Vector3r::Zero(),,"Center of the bounding sphere.")) \
		((Real,radius,0.,,"Radius of the bounding sphere.")) \
		((vector<Real>,raw,,,"Raw data for shape reconstruction; the size of the array is shape-specific.")) \
		,/*py*/ ; woo::converters_cxxVector_pyList_2way<shared_ptr<RawShape>>();


	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_RawShape__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(RawShape);

struct ShapeClump: public Object{
	// may fail when called from postLoad, but not from ensureOk()
	virtual void recompute(int div, bool failOk=false, bool fastOnly=false){ throw std::runtime_error("ShapeClump.recompute: this class is not to be used directly, derived classes should override recompute."); }
	void makeInvalid(){ volume=equivRad=NaN; inertia=Vector3r(NaN,NaN,NaN); pos=Vector3r::Zero(); ori=Quaternionr::Identity(); }
	bool isOk() const { return !isnan(volume); }
	void ensureOk() { if(!isOk()) recompute(div,/*failOk*/false); }
	// fill *pos* as average bounding sphere position, without calling compute
	// virtual void ensureApproxPos(){ throw std::runtime_error("ShapeClump.ensureApproxPos: this class is not to be used directly, derived classes should override ensureApproxPos."); }
	// if pos contains NaN, RawShape natural position and orientation are used
	virtual std::tuple<vector<shared_ptr<Node>>,vector<shared_ptr<Particle>>> makeParticles(const shared_ptr<Material>&, const Vector3r& pos, const Quaternionr& ori, int mask=0, Real scale=1.){  throw std::runtime_error("ShapeClump.makeParticles: this class is not be used directly, derived classes should override makeParticles."); }
	py::tuple pyMakeParticles(const shared_ptr<Material>& m, const Vector3r& p, const Quaternionr& o=Quaternionr::Identity(), Real scale=1., int mask=0){ const auto& tup=makeParticles(m,p,o,mask,scale); return py::make_tuple(std::get<0>(tup),std::get<1>(tup)); }
	virtual void translate(const Vector3r& offset){  throw std::runtime_error("ShapeClump.translate: this class is not be used directly, derived classes should override translate."); }
	virtual shared_ptr<ShapeClump> copy() const{  throw std::runtime_error("ShapeClump.copy: this class is not be used directly, derived classes should override copy."); }
	
	#define woo_dem_ShapeClump__CLASS_BASE_DOC_ATTRS_PY \
		ShapeClump,Object,"Defines pure geometry of clumps. This is a base class, not to be used as-is.", \
		((vector<Vector2r>,scaleProb,,,"Used by particle generators: piecewise-linear function probability(equivRad) given as a sequence of x,y coordinates. If not given, constant function $p(d)=1$ is assumed. See the documentation of :obj:`woo.dem.PsdClumpGenerator` for details.")) \
		((Vector3r,pos,Vector3r::Zero(),AttrTrait<>().readonly().noDump(),"Centroid position (computed automatically)")) \
		((Quaternionr,ori,Quaternionr::Identity(),AttrTrait<>().readonly().noDump(),"Principal axes orientation (computed automatically)")) \
		((Real,volume,NaN,AttrTrait<>().readonly().noDump().volUnit(),"Volume (computed automatically)")) \
		((Real,equivRad,NaN,AttrTrait<>().readonly().noDump().lenUnit(),"Equivalent radius of the clump (computed automatically) -- mean of radii of gyration, i.e. $\\frac{1}{3}\\sum \\sqrt{I_{ii}/V}$.")) \
		((Vector3r,inertia,Vector3r(NaN,NaN,NaN),AttrTrait<>().readonly().noDump(),"Geometrical inertia (computed with unit density)")) \
		((int,div,5,AttrTrait<Attr::triggerPostLoad>().noDump(),"Sampling grid fineness, when computing volume and other properties, relative to the smallest sphere's radius. When zero or negative, assume spheres don't intersect and use a different algorithm (Steiner's theorem)."))  \
		((bool,clumped,true,AttrTrait<>().readonly(),"Whether nodes of this shape are clumped together when the particle is created (so far, clumped shapes *only* are produced).")) \
		, /* py */ \
		.def("recompute",&ShapeClump::recompute,(py::arg("div")=5,py::arg("failOk")=false,py::arg("fastOnly")=false),"Recompute principal axes of the clump, using *div* for subdivision (see :obj:`div` for the semantics). *failOk* (silently return in case of invalid data) and *fastOnly* (return if there is lots of cells in subdivision) are only to be used internally.") \
		.def("makeParticles",&ShapeClump::pyMakeParticles,(py::arg("mat"),py::arg("pos"),py::arg("ori")=Quaternionr::Identity(),py::arg("scale")=1.,py::arg("mask")=0),"Create particle(s) as described by this geometry, positioned in *pos* and rotated with *ori*. Geometry will be scaled by *scale*. Returns tuple ([Node,...],[Particle,..]).") \
		; woo::converters_cxxVector_pyList_2way<shared_ptr<ShapeClump>>();
	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapeClump__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(ShapeClump);

struct RawShapeClump: public ShapeClump {
	WOO_DECL_LOGGER;
	void recompute(int div, bool failOk=false, bool fastOnly=false) WOO_CXX11_OVERRIDE;
	std::tuple<vector<shared_ptr<Node>>,vector<shared_ptr<Particle>>> makeParticles(const shared_ptr<Material>&, const Vector3r& pos, const Quaternionr& ori, int mask, Real scale=1.) WOO_CXX11_OVERRIDE;

	void translate(const Vector3r& offset) WOO_CXX11_OVERRIDE;
	shared_ptr<ShapeClump> copy() const WOO_CXX11_OVERRIDE;

	// void ensureApproxPos() WOO_CXX11_OVERRIDE;
	bool isSphereOnly() const;
	shared_ptr<SphereClumpGeom> asSphereClumpGeom() const;
	#define woo_dem_RawShapeClump__CLASS_BASE_DOC_ATTRS \
		RawShapeClump,ShapeClump,"Clump consisting of generic shapes (:obj:`RawShape`).", \
		((vector<shared_ptr<RawShape>>,rawShapes,,,"Data for creating primitive shapes"))

	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_RawShapeClump__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(RawShapeClump);

struct ShapePack: public Object{
	// sort the array according to the coordinate along *ax*
	// if *trimVol* is positive, discard all shapes after trimVol has been reached, in the order of increasing *ax*
	void sort(const int& ax, const Real& trimVol=NaN);
	// recompute all clumps with this precision
	void recomputeAll();
	bool shapeSupported(const shared_ptr<Shape>&) const;
	void fromDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, int mask, bool skipUnsupported=true);
	void toDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, const shared_ptr<Material>& mat, int mask=0, Real color=NaN);
	void saveTxt(const string& out) const;
	void loadTxt(const string& in);
	// mostly for testing only
	void add(const vector<shared_ptr<Shape>>& ss, bool clumped=true);

	shared_ptr<ShapePack> filtered(const shared_ptr<Predicate>& p, int recenter=-1); // recenter is for compat with SpherePack
	void filter(const shared_ptr<Predicate>& p, int recenter=-1);
	// total volume of all particles
	Real solidVolume(); 
	void translate(const Vector3r& offset);
	void canonicalize();
	void cellRepeat(const Vector3i& count);

	void postLoad(ShapePack&,void*);
	WOO_DECL_LOGGER;

	#define woo_dem_ShapePack__CLASS_BASE_DOC_ATTRS_PY \
		ShapePack,Object,ClassTrait().doc("Representation of geometry of many particles, with the ability of text I/O. It is meant as a replacement for :obj:`woo.pack.SpherePack`, which only handles spherical particles.").section("Packings","TODO",{"RawShape","ShapeClump"}), \
		((Vector3r,cellSize,Vector3r::Zero(),,"Positive components signify periodic boundary along the respective axis.")) \
		((bool,movable,false,,"Whether the packing is movable, i.e. should be automatically recentered after filtered with a predicate.")) \
		((int,div,5,,"Default value for recomputing properties of clumps (relative to the smallest equivalent radius)")) \
		((vector<shared_ptr<ShapeClump>>,raws,,,"Raw shapes of particles/clumps.")) \
		((string,userData,,,"String of arbitrary user data to be loaded/saved with the ShapePack.")) \
		((string,loadFrom,,AttrTrait<Attr::triggerPostLoad>(),"If given when constructing the instance, the file is loaded immediately and the attribute is reset.")) \
		,/*py*/ .def("loadTxt",&ShapePack::loadTxt).def("saveTxt",&ShapePack::saveTxt) \
			.def("load",&ShapePack::loadTxt).def("save",&ShapePack::saveTxt) /* SpherePack compat names */ \
		.def("add",&ShapePack::add,(py::arg("shapes"),py::arg("clumped")=true)) \
		.def("fromDem",&ShapePack::fromDem,(py::arg("scene"),py::arg("dem"),py::arg("mask")=0,py::arg("skipUnsupported")=true)) \
		.def("toDem",&ShapePack::toDem,(py::arg("scene"),py::arg("dem"),py::arg("mat"),py::arg("mask")=0,py::arg("color")=NaN)) \
		.def("filtered",&ShapePack::filtered,(py::arg("predicate"),py::arg("recenter")=-1)) \
		.def("filter",&ShapePack::filter,(py::arg("predicate"),py::arg("recenter")=-1)) \
		.def("solidVolume",&ShapePack::solidVolume) \
		.def("translate",&ShapePack::translate) \
		.def("canonicalize",&ShapePack::canonicalize) \
		.def("cellRepeat",&ShapePack::cellRepeat)

	WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapePack__CLASS_BASE_DOC_ATTRS_PY);
};
WOO_REGISTER_OBJECT(ShapePack);
