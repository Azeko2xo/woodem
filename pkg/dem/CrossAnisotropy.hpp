#pragma once
#include<woo/pkg/dem/ContactLoop.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
// #include<woo/pkg/dem/Cp2_FrictMat_FrictPhys.hpp>

struct Cp2_FrictMat_FrictPhys_CrossAnisotropic: CPhysFunctor {
	virtual void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(FrictMat,FrictMat);
	void postLoad(Cp2_FrictMat_FrictPhys_CrossAnisotropic&,void*);
	DECLARE_LOGGER;
	WOO_CLASS_BASE_DOC_ATTRS(Cp2_FrictMat_FrictPhys_CrossAnisotropic,CPhysFunctor,"Call :obj:`Cp2_FrictMat_FrictPhys` to create a new :obj:`FrictPhys`, but multiply resulting :obj:`normal <woo.dem.FrictPhys.kn>` and :obj:`shear <woo.dem.FrictPhys.kt>` stiffnesses by smooth dimensionless anisotropy distribution given by :obj:`alpha` and :obj:`beta`. The functionality is demonstrated in the following movie: \n\n.. youtube:: KYCvi6SNOc0\n\n",
		//((Real,nu2,.4,AttrTrait<Attr::readonly>(),"Minor Poisson's ratio (not really used)."))
		// ((Vector2r,nu1_range,Vector2r(-1.,1.),,"Meaningful range for :obj:`nu1<Cp2_FrictMat_FrictPhys_CrossAnisotropic.nu1>`."))
		((Real,E1,1e6,AttrTrait<Attr::triggerPostLoad>(),"In-plane normal modulus"))
		((Real,E2,1e5,,"Out-of-plane normal modulus"))
		//((Real,G1,1e4,,"In-plane shear modulus; computed automatically from :obj:`E1` and :obj:`nu1` as $\\frac{E_1}{2(1+\\nu_1)}$."))
		((Real,G1,1e4,,"In-plane shear modulus"))
		//  computed automatically from :obj:`E1` and :obj:`nu1` as $\\frac{E_1}{2(1+\\nu_1)}$."))
		((Real,G2,1e4,,"Out-of-plane shear modulus"))
		((Real,nu1,.4,AttrTrait<Attr::readonly>(),"Major Poisson's ratio; dependent value computed as $\\frac{E_1}{2G_1}-1$."))

		((Real,alpha,0,AttrTrait<Attr::triggerPostLoad>(),"Strike angle for the local axes"))
		((Vector2r,alpha_range,Vector2r(0,360),,"Range for alpha (adjusted automatically depending on degrees)"))
		((Real,beta,0,AttrTrait<Attr::triggerPostLoad>(),"Dip angle for the local axes"))
		((Vector2r,beta_range,Vector2r(0,90),,"Range for beta (adjusted automatically depending on degrees)"))
		((bool,deg,true,AttrTrait<Attr::triggerPostLoad>(),"True is alpha/beta are given in degrees rather than radians."))

		((Vector3r,xisoAxis,Vector3r::UnitX(),AttrTrait<Attr::readonly>(),"Axis (normal) of the cross-anisotropy in global coordinates; computed from :obj:`alpha` and :obj:`beta` as :math:`\\vec{n}=(\\cos\\alpha\\sin\\beta,-\\sin\\alpha\\sin\\beta,\\cos\\beta)`."))
		((int,recomputeIter,-1,,"Flag to keep track of updates to :obj:`alpha` and :obj:`beta`, so that stiffnesses of existing contacts are forced to be updated."))
	);
};
WOO_REGISTER_OBJECT(Cp2_FrictMat_FrictPhys_CrossAnisotropic);

#if 0
#ifdef WOO_OPENGL
#include<woo/pkg/common/OpenGLRenderer.hpp>

class GlExtra_LocalAxes: public GlExtraDrawer{
	public:
	//void postLoad(GlExtra_OctreeCubes&,void*);
	virtual void render();
	WOO_CLASS_BASE_DOC_ATTRS(GlExtra_LocalAxes,GlExtraDrawer,"Render local coordinate system axes.",
		((Vector3r,pos,Vector3r::Zero(),,"System position in global coordinates"))
		((Quaternionr,ori,Quaternionr::Identity(),,"System orientation"))
		//((Vector3r,color,Vector3r(1,1,1),,"Arrow and plane colors"))
		//((int,grid,7,,"Bitmask for showing planes perpendicular to x(1), y(2) and z(4)."))
		//((Real,length,1,,"Length of axes arrows."))
	);
};
WOO_REGISTER_OBJECT(GlExtra_LocalAxes);
#endif

#endif
