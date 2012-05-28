#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/FrictMat.hpp>
// #include<yade/pkg/dem/Cp2_FrictMat_FrictPhys.hpp>

struct Cp2_FrictMat_FrictPhys_CrossAnisotropic: CPhysFunctor {
	virtual void go(const shared_ptr<Material>&, const shared_ptr<Material>&, const shared_ptr<Contact>&);
	FUNCTOR2D(FrictMat,FrictMat);
	void postLoad(Cp2_FrictMat_FrictPhys_CrossAnisotropic&);
	DECLARE_LOGGER;
	YADE_CLASS_BASE_DOC_ATTRS(Cp2_FrictMat_FrictPhys_CrossAnisotropic,CPhysFunctor,"Call :yref:`Cp2_FrictMat_FrictPhys` to create a new :yref:`FrictPhys`, but multiply resulting :yref:`normal<NormPhys.kn>` and `shear<NormShearPhys.ks>` by smooth dimensionless anisotropy distribution given by :yref:`rot<Cp2_FrictMat_FrictPhys_CrossAnisotropic.rot>` and :yref:`scale<Cp2_FrictMat_FrictPhys.scale>`.",
		//((Real,nu2,.4,AttrTrait<Attr::readonly>(),"Minor Poisson's ratio (not really used)."))
		// ((Vector2r,nu1_range,Vector2r(-1.,1.),,"Meaningful range for :yref:`nu1<Cp2_FrictMat_FrictPhys_CrossAnisotropic.nu1>`."))
		((Real,E1,1e6,AttrTrait<Attr::triggerPostLoad>(),"In-plane normal modulus"))
		((Real,E2,1e5,,"Out-of-plane normal modulus"))
		//((Real,G1,1e4,,"In-plane shear modulus; computed automatically from :yref:`E1<Cp2_FrictMat_FrictPhys_CrossAnisotropic.E1>` and :yref:`nu1<Cp2_FrictMat_FrictPhys_CrossAnisotropic.nu1` as $\\frac{E_1}{2(1+\\nu_1)}$."))
		((Real,G1,1e4,,"In-plane shear modulus"))
		//  computed automatically from :yref:`E1<Cp2_FrictMat_FrictPhys_CrossAnisotropic.E1>` and :yref:`nu1<Cp2_FrictMat_FrictPhys_CrossAnisotropic.nu1` as $\\frac{E_1}{2(1+\\nu_1)}$."))
		((Real,G2,1e4,,"Out-of-plane shear modulus"))
		((Real,nu1,.4,AttrTrait<Attr::readonly>(),"Major Poisson's ratio; dependent value computed as $\\frac{E_1}{2G_1}-1$."))

		((Real,alpha,0,AttrTrait<Attr::triggerPostLoad>(),"Strike angle for the local axes"))
		((Vector2r,alpha_range,Vector2r(0,360),,"Range for alpha (adjusted automatically depending on degrees)"))
		((Real,beta,0,AttrTrait<Attr::triggerPostLoad>(),"Dip angle for the local axes"))
		((Vector2r,beta_range,Vector2r(0,90),,"Range for beta (adjusted automatically depending on degrees)"))
		((bool,deg,true,AttrTrait<Attr::triggerPostLoad>(),"True is alpha/beta are given in degrees rather than radians."))

		// ((Quaternionr,rot,Quaternionr::Identity(),AttrTrait<Attr::readonly>(),"Rotation of principal axes of anisotropy. (Automatically orthonormalized)"))
		((Vector3r,xisoAxis,Vector3r::UnitX(),AttrTrait<Attr::readonly>(),"Axis (normal) of the cross-anisotropy in global coordinates (computed from *alpha* and *beta* as $n=(\\cos\\alpha\\sin\\beta,-\\sin\\alpha\\sin\\beta,\\cos\\beta)$."))
		// ((Vector3r,scale,Vector3r::Ones(),,"Scaling coefficients for computes stiffnesses along principal axes (colums of :yref:`rot<Cp2_FrictMat_FrictPhys_CrossAnisotropic.rot>`."))
		((int,recomputeIter,-1,,"Flag to keep track of updates of rot/scale, so that stiffnesses of existing contacts are forced to be updated."))
	);
};
REGISTER_SERIALIZABLE(Cp2_FrictMat_FrictPhys_CrossAnisotropic);

#if 0
#ifdef YADE_OPENGL
#include<yade/pkg/common/OpenGLRenderer.hpp>

class GlExtra_LocalAxes: public GlExtraDrawer{
	public:
	//void postLoad(GlExtra_OctreeCubes&);
	virtual void render();
	YADE_CLASS_BASE_DOC_ATTRS(GlExtra_LocalAxes,GlExtraDrawer,"Render local coordinate system axes.",
		((Vector3r,pos,Vector3r::Zero(),,"System position in global coordinates"))
		((Quaternionr,ori,Quaternionr::Identity(),,"System orientation"))
		//((Vector3r,color,Vector3r(1,1,1),,"Arrow and plane colors"))
		//((int,grid,7,,"Bitmask for showing planes perpendicular to x(1), y(2) and z(4)."))
		//((Real,length,1,,"Length of axes arrows."))
	);
};
REGISTER_SERIALIZABLE(GlExtra_LocalAxes);
#endif

#endif