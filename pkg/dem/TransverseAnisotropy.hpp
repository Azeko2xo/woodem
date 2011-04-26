#include<yade/pkg/common/Dispatching.hpp>
#include<yade/pkg/common/MatchMaker.hpp>
#include<yade/pkg/common/ElastMat.hpp>
#include<yade/pkg/dem/Ip2_FrictMat_FrictMat_FrictPhys.hpp>

struct Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic: IPhysFunctor {
	virtual void go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& interaction);
	FUNCTOR2D(FrictMat,FrictMat);
	void postLoad(Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic&);
	YADE_CLASS_BASE_DOC_ATTRS(Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic,IPhysFunctor,"Call :yref:`Ip2_FrictMat_FrictMat_FrictPhys` to create a new :yref:`FrictPhys`, but multiply resulting :yref:`normal<NormPhys.kn>` and `shear<NormShearPhys.ks>` by smooth dimensionless anisotropy distribution given by :yref:`rot<Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic.rot>` and :yref:`scale<Ip2_FrictMat_FrictMat_FrictPhys.scale>`.",
		((Real,nu1,.4,Attr::triggerPostLoad,"Major Poisson's ratio."))
		((Vector2r,nu1_range,Vector2r(-1.,1.),Attr::noGui,"Meaningful range for :yref:`nu1<Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic.nu1>`."))
		((Real,E1,1e6,Attr::triggerPostLoad,"In-plane normal modulus"))
		((Real,E2,1e5,,"Out-of-plane normal modulus"))
		((Real,G1,1e4,Attr::readonly,"In-planne shear modulus; computed automatically from :yref:`E1<Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic.E1>` and :yref:`nu1<Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic.nu1` as $\\frac{E_1}{2(1+\\nu_1)}$."))
		((Real,G2,1e4,,"Out-of-plane shear modulus"))

		((Real,alpha,0,Attr::triggerPostLoad,"Strike angle for the local axes"))
		((Vector2r,alpha_range,Vector2r(0,180),Attr::noGui,"Range for alpha (adjusted automatically depending on degrees)"))
		((Real,beta,0,Attr::triggerPostLoad,"Dip angle for the local axes"))
		((Vector2r,beta_range,Vector2r(0,90),Attr::noGui,"Range for beta (adjusted automatically depending on degrees)"))
		((bool,deg,true,Attr::triggerPostLoad,"True is alpha/beta are given in degrees rather than radians."))

		((Quaternionr,rot,Quaternionr::Identity(),Attr::readonly,"Rotation of principal axes of anisotropy. (Automatically orthonormalized)"))
		// ((Vector3r,scale,Vector3r::Ones(),,"Scaling coefficients for computes stiffnesses along principal axes (colums of :yref:`rot<Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic.rot>`."))
		((int,recomputeIter,-1,,"Flag to keep track of updates of rot/scale, so that stiffnesses of existing contacts are forced to be updated."))
	);
};
REGISTER_SERIALIZABLE(Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic);


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
