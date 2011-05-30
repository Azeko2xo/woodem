#include<yade/pkg/dem/CrossAnisotropy.hpp>
#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/pkg/dem/L6Geom.hpp>
// #include<yade/pkg/dem/ScGeom.hpp>

YADE_PLUGIN(dem,(Cp2_FrictMat_FrictPhys_CrossAnisotropic));

void Cp2_FrictMat_FrictPhys_CrossAnisotropic::postLoad(Cp2_FrictMat_FrictPhys_CrossAnisotropic&){
	Real a=alpha*(deg?Mathr::PI/180.:1.), b=beta*(deg?Mathr::PI/180.:1.);
	Vector3r anisoNormal=Vector3r(cos(a)*sin(b),-sin(a)*sin(b),cos(b));
	rot.setFromTwoVectors(Vector3r::UnitZ(),anisoNormal);
	recomputeIter=scene->step+1; // recompute everything at the next step
	alpha_range=Vector2r(0.,deg?180.:Mathr::PI);
	beta_range=Vector2r(0.,deg?90.:.5*Mathr::PI);

	// recompute dependent poisson's ratio
	//G1=E1/(2*(1+nu1));
	nu1=E1/(2*G1)-1;
}

void Cp2_FrictMat_FrictPhys_CrossAnisotropic::go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Contact>& I){
	if(I->phys && recomputeIter!=scene->step) return;
	I->phys=shared_ptr<CPhys>(new FrictPhys); // this deletes the old CPhys, if it was there
	FrictPhys& ph=I->phys->cast<FrictPhys>();
	L6Geom& g=I->geom->cast<L6Geom>();

	Real A=Mathr::PI*pow(g.getMinRefLen(),2); // contact "area"
	Real l=g.lens[0]+g.lens[1]; // contact length

	// angle between pole (i.e. anisotropy normal) and contact normal
	Real sinTheta=/*aniso z-axis in global coords*/((rot.conjugate()*Vector3r::UnitZ()).cross(/*normal in global coords*/g.node->ori.conjugate()*Vector3r::UnitX())).norm();
	// cerr<<"x-aniso normal "<<Vector3r(rot.conjugate()*Vector3r::UnitZ())<<", contact axis "<<Vector3r(g.node->ori.conjugate()*Vector3r::UnitX())<<", angle "<<asin(sinTheta)*180/Mathr::PI<<" (sin="<<sinTheta<<")"<<endl;
	//Real theta=asin(theta);
	Real weight=pow(sinTheta,2);
	ph.kn=(A/l)*(weight*E1+(1-weight)*E2);
	ph.kt=(A/l)*(weight*G1+(1-weight)*G2);
	#if 0
		Vector3r n=rot.conjugate()*(g.node->ori*Vector3r::UnitX()); // normal in anisotropy coords
		ph.kn=(A/l)/sqrt(pow(n[0]/E1,2)+pow(n[1]/E1,2)+pow(n[2]/E2,2));
		if(G2>0) ph.kt=(A/l)/sqrt(pow(n[0]/G1,2)+pow(n[1]/G2,2)+pow(n[2]/G2,2));
		else ph.kt=0;
	#endif
	ph.tanPhi=min(b1->cast<FrictMat>().tanPhi,b2->cast<FrictMat>().tanPhi);
}

#if 0
#if YADE_OPENGL
YADE_PLUGIN(gl,(GlExtra_LocalAxes));
#endif	
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
#endif
