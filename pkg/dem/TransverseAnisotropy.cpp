#include<yade/pkg/dem/TransverseAnisotropy.hpp>
#include<yade/pkg/dem/FrictPhys.hpp>
#include<yade/pkg/dem/GenericSpheresContact.hpp>

YADE_PLUGIN(dem,(Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic));
#if YADE_OPENGL
YADE_PLUGIN(gl,(GlExtra_LocalAxes));
#endif	

void Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic::postLoad(Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic&){
	Real a=alpha*(deg?Mathr::PI/180.:1.), b=beta*(deg?Mathr::PI/180.:1.);
	Vector3r anisoNormal=Vector3r(cos(a)*sin(b),-sin(a)*sin(b),cos(b));
	rot.setFromTwoVectors(Vector3r::UnitZ(),anisoNormal);
	recomputeIter=scene->iter+1; // recompute everything at the next step
	alpha_range=Vector2r(0.,deg?180.:Mathr::PI);
	beta_range=Vector2r(0.,deg?90.:.5*Mathr::PI);

	// recompute dependent stiffnesses
	G1=E1/(2*(1+nu1));
}

void Ip2_FrictMat_FrictMat_FrictPhys_TransverseAnisotropic::go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& I){
	if(I->phys && recomputeIter!=scene->iter) return;
	I->phys=shared_ptr<IPhys>(new FrictPhys); // this deletes the old IPhys, if it was there
	FrictPhys& ph=I->phys->cast<FrictPhys>();
	GenericSpheresContact& gsc=I->geom->cast<GenericSpheresContact>();
	Real A=Mathr::PI*pow(gsc.minRefR(),2); // contact "area"
	Real l=gsc.refR1+gsc.refR2; // contact length
	Vector3r n=rot.conjugate()*gsc.normal; // normal in local coords
	ph.kn=(A/l)/sqrt(pow(n[0]/E1,2)+pow(n[1]/E1,2)+pow(n[2]/E2,2));
	if(G2>0) ph.ks=(A/l)/sqrt(pow(n[0]/G1,2)+pow(n[1]/G2,2)+pow(n[2]/G2,2));
	else ph.ks=0;
	ph.tanPhi=tan(min(b1->cast<FrictMat>().frictionAngle,b2->cast<FrictMat>().frictionAngle));
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
