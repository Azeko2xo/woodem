#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/IntraForce.hpp>
#include<woo/pkg/dem/L6Geom.hpp>

// NB: workaround for https://bugs.launchpad.net/woo/+bug/528509 removed
namespace woo{
	struct Sphere: public Shape{
		void selfTest(const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
		int numNodes() const WOO_CXX11_OVERRIDE { return 1; }
		void setFromRaw(const Vector3r& center, const Real& radius, const vector<Real>& raw) WOO_CXX11_OVERRIDE;
		void asRaw(Vector3r& center, Real& radius, vector<Real>& raw) const WOO_CXX11_OVERRIDE;
		bool isInside(const Vector3r& pt) const WOO_CXX11_OVERRIDE;
		// update dynamic properties (mass, intertia) of the sphere based on current radius
		void lumpMassInertia(const shared_ptr<Node>&, Real density, Real& mass, Matrix3r& I, bool& rotateOk) WOO_CXX11_OVERRIDE;
		virtual string pyStr() const { return "<Sphere r="+to_string(radius)+" @ "+lexical_cast<string>(this)+">"; }
		Real equivRadius() const WOO_CXX11_OVERRIDE { return radius; }
		Real volume() const WOO_CXX11_OVERRIDE;
		AlignedBox3r alignedBox() const WOO_CXX11_OVERRIDE;
		void applyScale(Real scale) WOO_CXX11_OVERRIDE;
		#define woo_dem_Sphere__CLASS_BASE_DOC_ATTRS_CTOR \
			Sphere,Shape,"Spherical particle.", \
			((Real,radius,NaN,AttrTrait<>().lenUnit(),"Radius.")), \
			createIndex(); /*ctor*/
		WOO_DECL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_Sphere__CLASS_BASE_DOC_ATTRS_CTOR);
		REGISTER_CLASS_INDEX(Sphere,Shape);
	};
};
WOO_REGISTER_OBJECT(Sphere);

struct Bo1_Sphere_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	void goGeneric(const shared_ptr<Shape>& sh, Vector3r halfSize);
	FUNCTOR1D(Sphere);
	#define woo_dem_Bo1_Sphere_Aabb__CLASS_BASE_DOC_ATTRS \
		Bo1_Sphere_Aabb,BoundFunctor,"Functor creating :obj:`Aabb` from :obj:`Sphere`.", \
		((Real,distFactor,((void)"deactivated",-1),,"Relative enlargement of the bounding box; deactivated if negative.\n\n.. note::\n\tThis attribute is used to create distant contacts, but is only meaningful with an :obj:`CGeomFunctor` which will not simply discard such interactions: :obj:`Cg2_Sphere_Sphere_L6Geom::distFactor` should have the same value."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Bo1_Sphere_Aabb__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(Bo1_Sphere_Aabb);

struct In2_Sphere_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
	FUNCTOR2D(Sphere,ElastMat);
	#ifdef WOO_DEBUG
		#define woo_dem_In2_Sphere_ElastMat__watch__DEBUG ((Vector2i,watch,Vector2i(-1,-1),,"Print detailed information about contact having those ids (debugging only)"))
	#else
		#define woo_dem_In2_Sphere_ElastMat__watch__DEBUG
	#endif

	#define woo_dem_In2_Sphere_ElastMat__CLASS_BASE_DOC_ATTRS \
		In2_Sphere_ElastMat,IntraFunctor,"Apply contact forces on sphere; having one node only, Sphere generates no internal forces as such.", \
		/*attrs*/ \
			((bool,alreadyWarned_ContactLoopWithApplyForces,false,AttrTrait<>().noGui(),"Keep track of whether the warning on ContactLoop already applying forces was issued.")) \
			woo_dem_In2_Sphere_ElastMat__watch__DEBUG
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_In2_Sphere_ElastMat__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(In2_Sphere_ElastMat);


struct Cg2_Sphere_Sphere_L6Geom: public Cg2_Any_Any_L6Geom__Base{
	bool go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	void setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) WOO_CXX11_OVERRIDE;
	#define woo_dem_Cg2_Sphere_Sphere_L6Geom__CLASS_BASE_DOC_ATTRS \
		Cg2_Sphere_Sphere_L6Geom,Cg2_Any_Any_L6Geom__Base,"Incrementally compute :obj:`L6Geom` for contact of 2 spheres. Detailed documentation in py/_extraDocs.py", \
		((Real,distFactor,1,,"Create interaction if spheres are not futher than ``|distFactor|*(r1+r2)``. If negative, zero normal deformation will be set to be the initial value (otherwise, the geometrical distance is the 'zero' one)."))
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Sphere_Sphere_L6Geom__CLASS_BASE_DOC_ATTRS);
	FUNCTOR2D(Sphere,Sphere);
	DEFINE_FUNCTOR_ORDER_2D(Sphere,Sphere);
	//WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(Cg2_Sphere_Sphere_L6Geom);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
class Gl1_Sphere: public GlShapeFunctor{
		// for stripes
		static vector<Vector3r> vertices, faces;
		static int glStripedSphereList;
		static int glGlutSphereList;
		void subdivideTriangle(Vector3r& v1,Vector3r& v2,Vector3r& v3, int depth);
		//Generate GlList for GLUT sphere
		void initGlutGlList();
		//Generate GlList for sliced spheres
		void initStripedGlList();
		//for regenerating glutSphere list if needed
		static Real prevQuality;
	protected:
		// called for rendering spheres both and ellipsoids, differing in the scale parameter
		// radius is radius for sphere, and it can be set to 1.0 for ellipsoid (containing radii inside scale)
		void renderScaledSphere(const shared_ptr<Shape>&, const Vector3r&, bool,const GLViewInfo&, const Real& radius, const Vector3r& scaleAxes=Vector3r(NaN,NaN,NaN));
	public:
		virtual void go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2,const GLViewInfo& glInfo){
			renderScaledSphere(sh,shift,wire2,glInfo,sh->cast<Sphere>().radius);
		}
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Sphere,GlShapeFunctor,"Renders :obj:`Sphere` object",
		((Real,quality,1.0,AttrTrait<>().range(Vector2r(0,8)),"Change discretization level of spheres. quality>1 for better image quality, at the price of more cpu/gpu usage, 0<quality<1 for faster rendering. If mono-color sphres are displayed (:obj:`stripes` = `False`), quality mutiplies :obj:`glutSlices` and :obj:`glutStacks`. If striped spheres are displayed (:obj:`stripes = `True`), only integer increments are meaningful: quality=1 and quality=1.9 will give the same result, quality=2 will give a finer result."))
		((bool,wire,false,,"Only show wireframe (controlled by :obj:`glutSlices` and :obj:`glutStacks`."))
		((bool,smooth,false,,"Render lines smooth (it makes them thicker and less clear if there are many spheres.)"))
		((Real,scale,1.,AttrTrait<>().range(Vector2r(.1,2.)),"Scale sphere radii"))
		/*
			TODO: re-enable this
			((bool,stripes,false,,"In non-wire rendering, show stripes clearly showing particle rotation."))
			((bool,localSpecView,true,,"Compute specular light in local eye coordinate system."))
		*/
		((int,glutSlices,12,AttrTrait<Attr::noSave>().readonly(),"Base number of sphere slices, multiplied by :obj:`Gl1_Sphere.quality` before use); not used with ``stripes`` (see `glut{Solid,Wire}Sphere reference <http://www.opengl.org/documentation/specs/glut/spec3/node81.html>`_)"))
		((int,glutStacks,6,AttrTrait<Attr::noSave>().readonly(),"Base number of sphere stacks, multiplied by :obj:`Gl1_Sphere.quality` before use; not used with ``stripes`` (see `glut{Solid,Wire}Sphere reference <http://www.opengl.org/documentation/specs/glut/spec3/node81.html>`_)"))
	);
	RENDERS(Sphere);
};
WOO_REGISTER_OBJECT(Gl1_Sphere);
#endif
