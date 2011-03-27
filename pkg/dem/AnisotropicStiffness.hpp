#include<yade/pkg/common/Dispatching.hpp>
#include<yade/pkg/common/MatchMaker.hpp>
#include<yade/pkg/common/ElastMat.hpp>
#include<yade/pkg/dem/Ip2_FrictMat_FrictMat_FrictPhys.hpp>

struct Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness: public Ip2_FrictMat_FrictMat_FrictPhys {
	virtual void go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& interaction);
	FUNCTOR2D(FrictMat,FrictMat);
	void postLoad(Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness&);
	YADE_CLASS_BASE_DOC_ATTRS(Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness,Ip2_FrictMat_FrictMat_FrictPhys,"Call :yref:`Ip2_FrictMat_FrictMat_FrictPhys` to create a new :yref:`FrictPhys`, but multiply resulting :yref:`normal<NormPhys.kn>` and `shear<NormShearPhys.ks>` by smooth dimensionless anisotropy distribution given by :yref:`rot<Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness.rot>` and :yref:`scale<Ip2_FrictMat_FrictMat_FrictPhys.scale>`.",
		((Real,alpha,0,Attr::triggerPostLoad,"Strike angle for the local axes"))
		((Real,beta,0,Attr::triggerPostLoad,"Dip angle for the local axes"))
		((bool,deg,true,Attr::triggerPostLoad,"True is alpha/beta are given in degrees rather than radians."))
		((Quaternionr,rot,Quaternionr::Identity(),Attr::readonly,"Rotation of principal axes of anisotropy. (Automatically orthonormalized)"))
		((Vector3r,scale,Vector3r::Ones(),,"Scaling coefficients for computes stiffnesses along principal axes (colums of :yref:`rot<Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness.rot>`."))
		((int,recomputeIter,-1,,"Flag to keep track of updates of rot/scale, so that stiffnesses of existing contacts are forced to be updated."))
	);
};
REGISTER_SERIALIZABLE(Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness);


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
