#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/IntraForce.hpp>

// NB: workaround for https://bugs.launchpad.net/woo/+bug/528509 removed
namespace woo{
	struct Sphere: public Shape{
		bool numNodesOk() const { return nodes.size()==1; }
		virtual string pyStr() const { return "<Sphere r="+to_string(radius)+" @ "+lexical_cast<string>(this)+">"; }
		WOO_CLASS_BASE_DOC_ATTRS_CTOR(Sphere,Shape,"Spherical particle.",
			((Real,radius,NaN,AttrTrait<>().lenUnit(),"Radius [m]")),
			createIndex(); /*ctor*/
		);
		REGISTER_CLASS_INDEX(Sphere,Shape);
	};
};
REGISTER_SERIALIZABLE(Sphere);

struct Bo1_Sphere_Aabb: public BoundFunctor{
	void go(const shared_ptr<Shape>&);
	FUNCTOR1D(Sphere);
	WOO_CLASS_BASE_DOC_ATTRS(Bo1_Sphere_Aabb,BoundFunctor,"Functor creating :ref:`Aabb` from :ref:`Sphere`.",
		((Real,distFactor,((void)"deactivated",-1),,"Relative enlargement of the bounding box; deactivated if negative.\n\n.. note::\n\tThis attribute is used to create distant contacts, but is only meaningful with an :ref:`CGeomFunctor` which will not simply discard such interactions: :ref:`Cg2_Sphere_Sphere_L6Geom::distFactor` should have the same value."))
	);
};
REGISTER_SERIALIZABLE(Bo1_Sphere_Aabb);

struct In2_Sphere_ElastMat: public IntraFunctor{
	void go(const shared_ptr<Shape>&, const shared_ptr<Material>&, const shared_ptr<Particle>&);
	FUNCTOR2D(Sphere,ElastMat);
	WOO_CLASS_BASE_DOC_ATTRS(In2_Sphere_ElastMat,IntraFunctor,"Apply contact forces on sphere; having one node only, Sphere generates no internal forces as such.",/*attrs*/
		// unused in the non-debugging version, but keep to not break archive compatibility
		//#ifdef WOO_DEBUG
			((Vector2i,watch,Vector2i(-1,-1),,"Print detailed information about contact having those ids (debugging only)"))
		//#endif
	);
};
REGISTER_SERIALIZABLE(In2_Sphere_ElastMat);



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
	public:
		virtual void go(const shared_ptr<Shape>&, const Vector3r&, bool,const GLViewInfo&);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Sphere,GlShapeFunctor,"Renders :ref:`Sphere` object",
		((Real,quality,1.0,AttrTrait<>().range(Vector2r(0,8)),"Change discretization level of spheres. quality>1  for better image quality, at the price of more cpu/gpu usage, 0<quality<1 for faster rendering. If mono-color sphres are displayed (:ref:`Gl1_Sphere::stripes=False), quality mutiplies :ref:`Gl1_Sphere::glutSlices` and :ref:`Gl1_Sphere::glutStacks`. If striped spheres are displayed (:ref:`Gl1_Sphere::stripes=True), only integer increments are meaningfull : quality=1 and quality=1.9 will give the same result, quality=2 will give finer result."))
		((bool,wire,false,,"Only show wireframe (controlled by ``glutSlices`` and ``glutStacks``."))
		((bool,smooth,false,,"Render lines smooth (it makes them thicker and less clear if there are many spheres.)"))
		((Real,scale,1.,,"Scale sphere radii"))
		((Vector2r,scale_range,Vector2r(.1,2.),AttrTrait<>().noGui(),"Range for scale"))
		((bool,stripes,false,,"In non-wire rendering, show stripes clearly showing particle rotation."))
		((bool,localSpecView,true,,"Compute specular light in local eye coordinate system."))
		((int,glutSlices,12,AttrTrait<Attr::noSave>().readonly(),"Base number of sphere slices, multiplied by :ref:`Gl1_Sphere::quality` before use); not used with ``stripes`` (see `glut{Solid,Wire}Sphere reference <http://www.opengl.org/documentation/specs/glut/spec3/node81.html>`_)"))
		((int,glutStacks,6,AttrTrait<Attr::noSave>().readonly(),"Base number of sphere stacks, multiplied by :ref:`Gl1_Sphere::quality` before use; not used with ``stripes`` (see `glut{Solid,Wire}Sphere reference <http://www.opengl.org/documentation/specs/glut/spec3/node81.html>`_)"))
	);
	RENDERS(Sphere);
};
REGISTER_SERIALIZABLE(Gl1_Sphere);
#endif
