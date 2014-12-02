// © 2004 Olivier Galizzi <olivier.galizzi@imag.fr>
// © 2006 Janek Kozicki <cosurgi@berlios.de>
#pragma once

#ifdef WOO_OPENGL

#include<woo/lib/multimethods/FunctorWrapper.hpp>
#include<woo/core/Dispatcher.hpp>
#include<woo/lib/pyutil/converters.hpp>



#define RENDERS(name) public: virtual string renders() const { return #name;}; FUNCTOR1D(name);

struct scene;
struct Renderer;

struct GLViewInfo{
	GLViewInfo(): sceneCenter(Vector3r::Zero()), sceneRadius(1.){}
	Vector3r sceneCenter;
	Real sceneRadius;
	Scene* scene;
	//Renderer* renderer;
};


#define GL_FUNCTOR(Klass,typelist,renderedType) class Klass: public Functor1D<renderedType,void,typelist>{public:\
	virtual string renders() const { throw std::runtime_error(#Klass ": unregistered gldraw class.\n"); };\
	virtual void initgl(){/*WARNING: it must deal with static members, because it is called from another instance!*/};\
	WOO_CLASS_BASE_DOC_ATTRS_PY(Klass,Functor,"Abstract functor for rendering :obj:`" #renderedType "` objects.",/*attrs*/,/*py*/ ; woo::converters_cxxVector_pyList_2way<shared_ptr<Klass>>();); \
	}; WOO_REGISTER_OBJECT(Klass); 
#define GL_DISPATCHER(Klass,Functor) class Klass: public Dispatcher1D<Functor>{public:\
	WOO_DISPATCHER1D_FUNCTOR_DOC_ATTRS_CTOR_PY(Klass,Functor,/*optional doc*/,/*attrs*/,/*ctor*/,/*py*/); \
	}; WOO_REGISTER_OBJECT(Klass);

#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Contact.hpp>

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

#endif /* WOO_OPENGL */
