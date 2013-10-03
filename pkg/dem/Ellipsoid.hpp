#pragma once
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Sphere.hpp>

namespace woo{
	struct Ellipsoid: public Shape{
		void selfTest(const shared_ptr<Particle>&) WOO_CXX11_OVERRIDE;
		bool numNodesOk() const { return nodes.size()==1; }
		// update dynamic properties (mass, intertia) of the sphere based on current radius
		void updateDyn(const Real& density) const;
		WOO_CLASS_BASE_DOC_ATTRS_CTOR(Ellipsoid,Shape,"Ellipsoidal particle.",
			((Vector3r,semiAxes,Vector3r(NaN,NaN,NaN),AttrTrait<>().lenUnit(),"Semi-principal axes.")),
			createIndex(); /*ctor*/
		);
		REGISTER_CLASS_INDEX(Ellipsoid,Shape);
	};
};
WOO_REGISTER_OBJECT(Ellipsoid);


#ifdef WOO_OPENGL
#include<woo/pkg/gl/Functors.hpp>
struct Gl1_Ellipsoid: public Gl1_Sphere{
	virtual void go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2,const GLViewInfo& glInfo){
		Gl1_Sphere::renderScaledSphere(shape,shift,wire2,glInfo,/*radius*/1.0,shape->cast<Ellipsoid>().semiAxes);
	}
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_Ellipsoid,Gl1_Sphere,"Renders :obj:`woo.dem.Ellipsoid` object",);
	RENDERS(Ellipsoid);
};
WOO_REGISTER_OBJECT(Gl1_Ellipsoid);
#endif

