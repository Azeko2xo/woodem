// © 2007 Janek Kozicki <cosurgi@mail.berlios.de>
// © 2008 Václav Šmilauer <eudoxos@arcig.cz>
#pragma once

#include<yade/core/IPhys.hpp>

class NormPhys:public IPhys {
	public:
		virtual ~NormPhys();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(NormPhys,IPhys,"Abstract class for interactions that have normal stiffness.",
		((Real,kn,NaN,,"Normal stiffness"))
		((Vector3r,normalForce,Vector3r::Zero(),,"Normal force after previous step (in global coordinates).")),
		createIndex();
	);
	REGISTER_CLASS_INDEX(NormPhys,IPhys);
};
REGISTER_SERIALIZABLE(NormPhys);

class NormShearPhys: public NormPhys{
	public:
		virtual ~NormShearPhys();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(NormShearPhys,NormPhys,
		"Abstract class for interactions that have shear stiffnesses, in addition to normal stiffness. This class is used in the PFC3d-style stiffness timestepper.",
		((Real,ks,NaN,,"Shear stiffness"))
		((Vector3r,shearForce,Vector3r::Zero(),,"Shear force after previous step (in global coordinates).")),
		createIndex();
	);
	REGISTER_CLASS_INDEX(NormShearPhys,NormPhys);
};
REGISTER_SERIALIZABLE(NormShearPhys);

#ifdef YADE_OPENGL
#include<yade/lib/opengl/OpenGLWrapper.hpp>
#include<yade/lib/opengl/GLUtils.hpp>
#include<yade/pkg/common/GLDrawFunctors.hpp>
#include<GL/glu.h>

class Gl1_NormPhys: public GlIPhysFunctor{	
		static GLUquadric* gluQuadric; // needed for gluCylinder, initialized by ::go if no initialized yet
	public:
		virtual void go(const shared_ptr<IPhys>&,const shared_ptr<Interaction>&,const shared_ptr<Body>&,const shared_ptr<Body>&,bool wireFrame);
	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_NormPhys,GlIPhysFunctor,"Renders :yref:`NormPhys` objects as cylinders of which diameter and color depends on :yref:`NormPhys:normForce` magnitude.",
		((Real,maxFn,0,,"Value of :yref:`NormPhys.normalForce` corresponding to :yref:`maxDiameter<Gl1_NormPhys.maxDiameter>`. This value will be increased (but *not decreased*) automatically."))
		((int,signFilter,0,,"If non-zero, only display contacts with negative (-1) or positive (+1) normal forces; if zero, all contacts will be displayed."))
		((Real,refRadius,std::numeric_limits<Real>::infinity(),,"Reference (minimum) particle radius; used only if :yref:`maxRadius<Gl1_NormPhys.maxRadius>` is negative. This value will be decreased (but *not increased*) automatically. |yupdate|"))
		((Real,maxRadius,-1,,"Cylinder radius corresponding to the maximum normal force. If negative, auto-updated :yref:`refRadius<Gl1_NormPhys.refRadius>` will be used instead."))
		((int,slices,6,,"Number of sphere slices; (see `glutCylinder reference <http://www.opengl.org/sdk/docs/man/xhtml/gluCylinder.xml>`__)"))
		((int,stacks,1,,"Number of sphere stacks; (see `glutCylinder reference <http://www.opengl.org/sdk/docs/man/xhtml/gluCylinder.xml>`__)"))
		// strong/weak fabric attributes
		((Real,maxWeakFn,NaN,,"Value that divides contacts by their normal force into the ``weak fabric'' and ``strong fabric''. This value is set as side-effect by :yref:`utils.fabricTensor`."))
		((int,weakFilter,0,,"If non-zero, only display contacts belonging to the ``weak'' (-1) or ``strong'' (+1) fabric."))
		((Real,weakScale,1.,,"If :yref:`maxWeakFn<Gl1_NormPhys.maxWeakFn>` is set, scale radius of the weak fabric by this amount (usually smaller than 1). If zero, 1 pixel line is displayed. Colors are not affected by this value."))
	);
	RENDERS(NormPhys);
};
REGISTER_SERIALIZABLE(Gl1_NormPhys);
#endif

