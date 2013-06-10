#pragma once
#ifdef WOO_OPENGL

#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/gl/Functors.hpp>

class GLUquadric;

class Gl1_CPhys: public GlCPhysFunctor{	
		static GLUquadric* gluQuadric; // needed for gluCylinder, initialized by ::go if no initialized yet
	public:
		virtual void go(const shared_ptr<CPhys>&,const shared_ptr<Contact>&, const GLViewInfo&);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_CPhys,GlCPhysFunctor,"Renders :ref:`CPhys` objects as cylinders of which diameter and color depends on :ref:`CPhys:force` norma (0th) component.",
		((shared_ptr<ScalarRange>,range,make_shared<ScalarRange>(),,"Range for normal force"))
		((shared_ptr<ScalarRange>,shearRange,make_shared<ScalarRange>(),,"Range for absolute value of shear force"))
		((bool,shearColor,false,,"Set color by shear force rather than by normal force. (Radius still depends on normal force)"))
		((int,signFilter,0,,"If non-zero, only display contacts with negative (-1) or positive (+1) normal forces; if zero, all contacts will be displayed."))
		((Real,relMaxRad,.01,,"Relative radius for maximum forces"))
		((int,slices,6,,"Number of cylinder slices"))
		((Vector2i,slices_range,Vector2i(4,16),AttrTrait<>().noGui(),"Range for slices"))
	);
	RENDERS(CPhys);
};
WOO_REGISTER_OBJECT(Gl1_CPhys);
#endif
