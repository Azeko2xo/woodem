// © 2004 Olivier Galizzi <olivier.galizzi@imag.fr>
// © 2008 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include<yade/lib/multimethods/DynLibDispatcher.hpp>
#include<yade/core/Dispatcher.hpp>
#include<yade/lib/opengl/OpenGLWrapper.hpp>

#include<yade/pkg/gl/Functors.hpp>
#include<yade/pkg/gl/NodeGlRep.hpp>

struct GlExtraDrawer: public Serializable{
	Scene* scene;
	virtual void render();
	YADE_CLASS_BASE_DOC_ATTRS(GlExtraDrawer,Serializable,"Performing arbitrary OpenGL drawing commands; called from :yref:`Renderer` (see :yref:`Renderer.extraDrawers`) once regular rendering routines will have finished.\n\nThis class itself does not render anything, derived classes should override the *render* method.",
		((bool,dead,false,,"Deactivate the object (on error/exception)."))
	);
};
REGISTER_SERIALIZABLE(GlExtraDrawer);


class SparcField;

class Renderer: public Serializable{
	static Renderer* self;
	public:
		static const int numClipPlanes=3;

		bool pointClipped(const Vector3r& p);
		vector<Vector3r> clipPlaneNormals;
		// void setBodiesDispInfo();
		static bool initDone;
		Vector3r viewDirection; // updated from GLViewer regularly
		GLViewInfo viewInfo; // update from GLView regularly
		Vector3r highlightEmission0;
		Vector3r highlightEmission1;

		// normalized saw signal with given periodicity, with values ∈ 〈0,1〉 */
		Real normSaw(Real t, Real period){ Real xi=(t-period*((int)(t/period)))/period; /* normalized value, (0-1〉 */ return (xi<.5?2*xi:2-2*xi); }
		Real normSquare(Real t, Real period){ Real xi=(t-period*((int)(t/period)))/period; /* normalized value, (0-1〉 */ return (xi<.5?0:1); }

		void drawPeriodicCell();

		// void setBodiesRefSe3();

	#if 0
		struct BodyDisp{
			Vector3r pos;
			Quaternionr ori;
			bool isDisplayed;
		};
		//! display data for individual bodies
		vector<BodyDisp> bodyDisp;
	#endif
	private:
		void resetSpecularEmission();
		void setLighting();
		void setClippingPlanes();
		GlShapeDispatcher shapeDispatcher;
		GlBoundDispatcher boundDispatcher;
		GlNodeDispatcher nodeDispatcher;
	#if 0
		GlIPhysDispatcher physDispatcher;
		GlFieldDispatcher fieldDispatcher;
		// GlStateDispatcher stateDispatcher;
	#endif
		DECLARE_LOGGER;

	public :
		// updated after every call to render
		shared_ptr<Scene> scene;
		shared_ptr<DemField> dem;
		shared_ptr<SparcField> sparc;

		// GL selection amangement
		// in selection mode, put rendered obejcts one after another into an array
		// which can be then used to look that object up

		// static so that glScopedName can access it
		// selection-related things
		bool withNames;
		vector<shared_ptr<Serializable> > glNamedObjects;
		vector<shared_ptr<Node> > glNamedNodes;

		// passing >=0 to highLev causes the object the to be highlighted, regardless of whether it is selected or not
		struct glScopedName{
			bool highlighted;
			glScopedName(const shared_ptr<Serializable>& s, const shared_ptr<Node>& n, int highLev=-1): highlighted(false){ init(s,n, highLev); }
			glScopedName(const shared_ptr<Node>& n, int highLev=-1): highlighted(false){ init(n,n, highLev); }
			void init(const shared_ptr<Serializable>& s, const shared_ptr<Node>& n, int highLev){
				Renderer* r=Renderer::self; // ugly hack, but we want to have that at all costs
				if(!r->withNames){
					if(highLev>=0 || s==r->selObj){ r->setLightHighlighted(highLev); highlighted=true; }
					else { r->setLightUnhighlighted(); highlighted=false; }
				}
				r->glNamedObjects.push_back(s);
				r->glNamedNodes.push_back(n);
				::glPushName(r->glNamedObjects.size()-1);
			};
			~glScopedName(){ glPopName(); }
		};

		void setLightHighlighted(int highLev=0);
		void setLightUnhighlighted();

		void init();
		void initgl();
		void render(const shared_ptr<Scene>& scene, bool withNames);

		void renderNodes();
		void renderRawNode(shared_ptr<Node>);
		void renderCNodes();
		void renderShape();
		void renderBound();

		void renderSparc();
		//void renderCGeom();

		void pyInitError(py::tuple, py::dict){ throw std::runtime_error("yade.gl.Renderer() may not be instantiated directly, use yade.qt.Renderer() to get the current instance."); }

#if 0
		void pyRender(){render(Omega::instance().getScene());}

		void renderDOF_ID();
		void renderIPhys();
		void renderIGeom();
		// called also to render selectable entitites;
		void renderAllInteractionsWire();

		void renderField();
#endif
	YADE_CLASS_BASE_DOC_ATTRS_DEPREC_INIT_CTOR_PY(Renderer,Serializable,"Class responsible for rendering scene on OpenGL devices.",
		((Vector3r,dispScale,((void)"disable scaling",Vector3r::Ones()),,"Artificially enlarge (scale) dispalcements from bodies' :yref:`reference positions<State.refPos>` by this relative amount, so that they become better visible (independently in 3 dimensions). Disbled if (1,1,1)."))
		((Real,rotScale,((void)"disable scaling",1.),,"Artificially enlarge (scale) rotations of bodies relative to their :yref:`reference orientation<State.refOri>`, so the they are better visible."))
		((Vector3r,lightPos,Vector3r(75,130,0),,"Position of OpenGL light source in the scene."))
		((Vector3r,light2Pos,Vector3r(-130,75,30),,"Position of secondary OpenGL light source in the scene."))
		((Vector3r,lightColor,Vector3r(0.6,0.6,0.6),,"Per-color intensity of primary light (RGB)."))
		((Vector3r,light2Color,Vector3r(0.5,0.5,0.1),,"Per-color intensity of secondary light (RGB)."))
		((Vector3r,bgColor,Vector3r(.2,.2,.2),,"Color of the background canvas (RGB)"))
		((bool,light1,true,,"Turn light 1 on."))
		((bool,light2,true,,"Turn light 2 on."))
		((bool,nodes,true,,"Render nodes belonging to fields"))
		((bool,ghosts,true,,"Render objects crossing periodic cell edges by cloning them in multiple places (periodic simulations only)."))
		#ifdef YADE_SUBDOMAINS
			((int,subDomMask,0,,"If non-zero, render shape only of particles that are inside respective domains - -they are counted from the left, i.e. 5 (binary 101) will show subdomains 1 and 3. If zero, render everything."))
		#endif
		// ((int,mask,((void)"draw everything",~0),,"Bitmask for showing only bodies where ((mask & :yref:`Body::mask`)!=0)"))
		// ((int,selId,-1,,"Id of particle that was selected by the user."))
		((shared_ptr<Serializable>,selObj,,,"Object which was selected by the user (access only via yade.qt.selObj)."))
		((shared_ptr<Node>,selObjNode,,Attr::readonly,"Node associated to the selected object (recenters scene on that object upon selection)"))
		((vector<Se3r>,clipPlaneSe3,vector<Se3r>(numClipPlanes,Se3r(Vector3r::Zero(),Quaternionr::Identity())),,"Position and orientation of clipping planes"))
		((vector<bool>,clipPlaneActive,vector<bool>(numClipPlanes,false),,"Activate/deactivate respective clipping planes"))
		((vector<shared_ptr<GlExtraDrawer> >,extraDrawers,,,"Additional rendering components (:yref:`GlExtraDrawer`)."))

		((bool,wire,false,,"Render all bodies with wire only"))
		((bool,id,false,,"Show particle id's"))
		((bool,bound,false,,"Render particle's :yref:`Bound`"))
		((bool,shape,true,,"Render particle's :yref:`Shape`"))
		((int,cNodes,true,,"Render contact's nodes (0=no, 1=nodes only, 2=line between particles, 3=both"))
		((Vector2i,cNodes_range,Vector2i(0,3),Attr::noGui,"Range for cNodes"))
		//((bool,intrAllWire,false,,"Draw wire for all interactions, blue for potential and green for real ones (mostly for debugging)")),
		,/*deprec*/
		,/*init*/
		,/*ctor*/ if(self && initDone) throw std::runtime_error("yade.gl.Renderer() is already constructed, use yade.qt.Renderer() to retrieve the instance."); self=this;
		,/*py*/
		// .def("setRefSe3",&Renderer::setBodiesRefSe3,"Make current positions and orientation reference for scaleDisplacements and scaleRotations.")
		//.def("render",&Renderer::pyRender,"Render the scene in the current OpenGL context.")
		.def_readonly("shapeDispatcher",&Renderer::shapeDispatcher)
		.def_readonly("boundDispatcher",&Renderer::boundDispatcher)
		.def_readonly("nodeDispatcher",&Renderer::nodeDispatcher)
		// .def_readonly("cgeomDispatcher",&Renderer::nodeDispatcher)

		.def("__init__",py::make_constructor(&Renderer::pyInitError)) // does not seem to be called :-(
	);
};
REGISTER_SERIALIZABLE(Renderer);
