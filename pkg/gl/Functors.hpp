// © 2004 Olivier Galizzi <olivier.galizzi@imag.fr>
// © 2006 Janek Kozicki <cosurgi@berlios.de>
#pragma once

#ifdef YADE_OPENGL

#include<yade/lib/multimethods/FunctorWrapper.hpp>


#define RENDERS(name) public: virtual string renders() const { return #name;}; FUNCTOR1D(name);

struct GLViewInfo{
	GLViewInfo(): sceneCenter(Vector3r::Zero()), sceneRadius(1.){}
	Vector3r sceneCenter;
	Real sceneRadius;
};

class Renderer;

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

GL_FUNCTOR(GlShapeFunctor,TYPELIST_4(const shared_ptr<Shape>&, /*shift*/ const Vector3r&,/*wire*/bool,const GLViewInfo&),Shape);
//GL_FUNCTOR(GlShapeFunctor,TYPELIST_1(const shared_ptr<Shape>&),Shape);
GL_DISPATCHER(GlShapeDispatcher,GlShapeFunctor);

GL_FUNCTOR(GlBoundFunctor,TYPELIST_1(const shared_ptr<Bound>&),Bound);
GL_DISPATCHER(GlBoundDispatcher,GlBoundFunctor);

GL_FUNCTOR(GlNodeFunctor ,TYPELIST_2(const shared_ptr<Node >&, const GLViewInfo&),Node);
GL_DISPATCHER(GlNodeDispatcher ,GlNodeFunctor );

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
GL_FUNCTOR(GlIGeomFunctor,TYPELIST_5(const shared_ptr<IGeom>&, const shared_ptr<Interaction>&, const shared_ptr<Body>&, const shared_ptr<Body>&, bool),IGeom);
GL_FUNCTOR(GlIPhysFunctor,TYPELIST_5(const shared_ptr<IPhys>&, const shared_ptr<Interaction>&, const shared_ptr<Body>&, const shared_ptr<Body>&, bool),IPhys);
GL_FUNCTOR(GlStateFunctor,TYPELIST_1(const shared_ptr<State>&),State);
GL_FUNCTOR(GlFieldFunctor,TYPELIST_1(const shared_ptr<Field>&),Field);

GL_DISPATCHER(GlBoundDispatcher,GlBoundFunctor);
GL_DISPATCHER(GlShapeDispatcher,GlShapeFunctor);
GL_DISPATCHER(GlIGeomDispatcher,GlIGeomFunctor);
GL_DISPATCHER(GlIPhysDispatcher,GlIPhysFunctor);
GL_DISPATCHER(GlStateDispatcher,GlStateFunctor);
GL_DISPATCHER(GlFieldDispatcher,GlFieldFunctor);
#undef GL_FUNCTOR
#undef GL_DISPATCHER

#endif

#endif /* YADE_OPENGL */
