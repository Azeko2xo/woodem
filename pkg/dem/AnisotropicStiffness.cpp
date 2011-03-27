#include<yade/pkg/dem/AnisotropicStiffness.hpp>

YADE_PLUGIN((Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness)
#if YADE_OPENGL
	(GlExtra_LocalAxes)
#endif	
);

void Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness::postLoad(Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness&){
	Real a=alpha*(deg?Mathr::PI/180.:1.), b=beta*(deg?Mathr::PI/180.:1.);
	Vector3r anisoNormal=Vector3r(cos(a)*sin(b),-sin(a)*sin(b),cos(b));
	rot.setFromTwoVectors(Vector3r::UnitZ(),anisoNormal);
	recomputeIter=scene->iter+1; // recompute everything at the next step
}

void Ip2_FrictMat_FrictMat_FrictPhys_AnisotropicStiffness::go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& I){
	if(I->phys && recomputeIter!=scene->iter) return;
	if(I->phys) I->phys=shared_ptr<IPhys>(); // reset so that call to Ip2_... now will not return seeing IPhys is there already
	Ip2_FrictMat_FrictMat_FrictPhys::go(b1,b2,I);
	assert(I->phys);
	FrictPhys& ph=I->phys->cast<FrictPhys>();
	const Vector3r& gn=I->geom->cast<GenericSpheresContact>().normal; // normal in global coords
	Vector3r ln=rot.conjugate()*gn; // normal in local coords
	Real gScale=(ln.cwise()*scale).norm();
	ph.kn*=gScale;
	ph.ks*=gScale;
}

#ifdef YADE_OPENGL
#include<yade/lib/opengl/GLUtils.hpp>

void GlExtra_LocalAxes::render(){
	for(int ax=0; ax<3; ax++){
		Vector3r c(Vector3r::Zero()); c[ax]=1; // color and, at the same time, local axis unit vector
		//GLUtils::GLDrawArrow(pos,pos+length*(ori*c),c);
	}
	#if 0
		Real realSize=2*length;
		int nSegments=20;
		if(grid & 1) { glColor3f(0.6,0.3,0.3); glPushMatrix(); glRotated(90.,0.,1.,0.); QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
		if(grid & 2) { glColor3f(0.3,0.6,0.3); glPushMatrix(); glRotated(90.,1.,0.,0.); QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
		if(grid & 4) { glColor3f(0.3,0.3,0.6); glPushMatrix(); /*glRotated(90.,0.,1.,0.);*/ QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
	#endif
};

#endif
