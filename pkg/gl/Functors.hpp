// © 2004 Olivier Galizzi <olivier.galizzi@imag.fr>
// © 2006 Janek Kozicki <cosurgi@berlios.de>
#pragma once

#ifdef YADE_OPENGL

#include<yade/lib/multimethods/FunctorWrapper.hpp>
#include<yade/core/Dispatcher.hpp>


#define RENDERS(name) public: virtual string renders() const { return #name;}; FUNCTOR1D(name);

struct scene;
struct Renderer;

struct GLViewInfo{
	GLViewInfo(): sceneCenter(Vector3r::Zero()), sceneRadius(1.){}
	Vector3r sceneCenter;
	Real sceneRadius;
	Scene* scene;
	Renderer* renderer;
};


#define GL_FUNCTOR(Klass,typelist,renderedType) class Klass: public Functor1D<renderedType,void,typelist>{public:\
	virtual ~Klass(){};\
	virtual string renders() const { throw std::runtime_error(#Klass ": unregistered gldraw class.\n"); };\
	virtual void initgl(){/*WARNING: it must deal with static members, because it is called from another instance!*/};\
	YADE_CLASS_BASE_DOC(Klass,Functor,"Abstract functor for rendering :yref:`" #renderedType "` objects."); \
	}; REGISTER_SERIALIZABLE(Klass); 
#define GL_DISPATCHER(Klass,Functor) class Klass: public Dispatcher1D<Functor>{public:\
	YADE_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(Klass,Functor,/*optional doc*/,/*attrs*/,/*ctor*/,/*py*/); \
	}; REGISTER_SERIALIZABLE(Klass);

#include<yade/pkg/dem/Particle.hpp>

GL_FUNCTOR(GlShapeFunctor,TYPELIST_4(const shared_ptr<Shape>&, /*shift*/ const Vector3r&, /*wire*/bool,const GLViewInfo&),Shape);
GL_DISPATCHER(GlShapeDispatcher,GlShapeFunctor);

//GL_FUNCTOR(GlCGeomFunctor,TYPELIST_3(const shared_ptr<CGeom>&, const shared_ptr<Contact>& c, /*wire*/ bool),CGeom);
//GL_DISPATCHER(GlCGeomDispatcher,GlCGeomFunctor);

GL_FUNCTOR(GlBoundFunctor,TYPELIST_1(const shared_ptr<Bound>&),Bound);
GL_DISPATCHER(GlBoundDispatcher,GlBoundFunctor);

GL_FUNCTOR(GlNodeFunctor,TYPELIST_2(const shared_ptr<Node>&, const GLViewInfo&),Node);
GL_DISPATCHER(GlNodeDispatcher,GlNodeFunctor);

GL_FUNCTOR(GlCPhysFunctor,TYPELIST_3(const shared_ptr<CPhys>&, const shared_ptr<Contact>&, const GLViewInfo&),CPhys);
GL_DISPATCHER(GlCPhysDispatcher,GlCPhysFunctor);

GL_FUNCTOR(GlFieldFunctor,TYPELIST_2(const shared_ptr<Field>&, GLViewInfo*),Field);
GL_DISPATCHER(GlFieldDispatcher,GlFieldFunctor);

#if 0
#include<yade/core/Bound.hpp>
#include<yade/core/State.hpp>
#include<yade/core/Shape.hpp>
#include<yade/core/Field.hpp>
#include<yade/core/Functor.hpp>
#include<yade/core/Dispatcher.hpp>
#include<yade/core/Body.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/core/Interaction.hpp>

GL_FUNCTOR(GlBoundFunctor,TYPELIST_1(const shared_ptr<Bound>&),Bound);
GL_FUNCTOR(GlCGeomFunctor,TYPELIST_5(const shared_ptr<IGeom>&, const shared_ptr<Interaction>&, const shared_ptr<Body>&, const shared_ptr<Body>&, bool),IGeom);
GL_FUNCTOR(GlCPhysFunctor,TYPELIST_5(const shared_ptr<IPhys>&, const shared_ptr<Interaction>&, const shared_ptr<Body>&, const shared_ptr<Body>&, bool),IPhys);
GL_FUNCTOR(GlStateFunctor,TYPELIST_1(const shared_ptr<State>&),State);
GL_FUNCTOR(GlFieldFunctor,TYPELIST_1(const shared_ptr<Field>&),Field);

GL_DISPATCHER(GlBoundDispatcher,GlBoundFunctor);
GL_DISPATCHER(GlShapeDispatcher,GlShapeFunctor);
GL_DISPATCHER(GlCGeomDispatcher,GlCGeomFunctor);
GL_DISPATCHER(GlCPhysDispatcher,GlCPhysFunctor);
GL_DISPATCHER(GlStateDispatcher,GlStateFunctor);
GL_DISPATCHER(GlFieldDispatcher,GlFieldFunctor);
#undef GL_FUNCTOR
#undef GL_DISPATCHER

#endif

#endif /* YADE_OPENGL */
