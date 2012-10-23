#include<woo/pkg/dem/CrossAnisotropy.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
// #include<woo/pkg/dem/ScGeom.hpp>

WOO_PLUGIN(dem,(Cp2_FrictMat_FrictPhys_CrossAnisotropic));

CREATE_LOGGER(Cp2_FrictMat_FrictPhys_CrossAnisotropic);

void Cp2_FrictMat_FrictPhys_CrossAnisotropic::postLoad(Cp2_FrictMat_FrictPhys_CrossAnisotropic&){
	Real a=alpha*(deg?M_PI/180.:1.), b=beta*(deg?M_PI/180.:1.);
	xisoAxis=Vector3r(cos(a)*sin(b),-sin(a)*sin(b),cos(b));
	recomputeIter=scene->step+1; // recompute everything at the next step
	alpha_range=Vector2r(0.,deg?360.:2*M_PI);
	beta_range=Vector2r(0.,deg?90.:.5*M_PI);

	// recompute dependent poisson's ratio
	//G1=E1/(2*(1+nu1));
	nu1=E1/(2*G1)-1;
}

void Cp2_FrictMat_FrictPhys_CrossAnisotropic::go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Contact>& C){
	if(C->phys && recomputeIter!=scene->step) return;
	// if(C->phys) cerr<<"Cp2_..._CrossAnisotropic: Recreating ##"<<C->pA->id<<"+"<<C->pB->id<<endl;
	C->phys=shared_ptr<CPhys>(new FrictPhys); // this deletes the old CPhys, if it was there
	FrictPhys& ph=C->phys->cast<FrictPhys>();
	L6Geom& g=C->geom->cast<L6Geom>();

	#if 0
		// this is fucked up, since we rely on data passed from the Cg2 functors
		Real A=M_PI*pow(g.getMinRefLen(),2); // contact "area"
		Real l=g.lens[0]+g.lens[1]; // contact length
	#else
		if(!dynamic_cast<Sphere*>(C->pA->shape.get()) || !dynamic_cast<Sphere*>(C->pB->shape.get())){
			LOG_FATAL("Cp2_FrictMat_FrictPhys_CrossAnisotropic: can be only used on spherical particles!");
			throw std::runtime_error("Cp2_FrictMat_FrictPhys_CrossAnisotropic: can be only used on spherical particles!");
		}
		Real r1=C->pA->shape->cast<Sphere>().radius, r2=C->pB->shape->cast<Sphere>().radius;
		Real A=M_PI*pow(min(r1,r2),2);
		Real l=C->dPos(scene).norm(); // handles periodicity gracefully
	#endif

	// angle between pole (i.e. anisotropy normal) and contact normal
	Real sinTheta=(/*xiso axis in global coords*/(xisoAxis).cross(/*contact axis in global coords*/g.node->ori.conjugate()*Vector3r::UnitX())).norm();
	// cerr<<"x-aniso normal "<<Vector3r(rot.conjugate()*Vector3r::UnitZ())<<", contact axis "<<Vector3r(g.node->ori.conjugate()*Vector3r::UnitX())<<", angle "<<asin(sinTheta)*180/M_PI<<" (sin="<<sinTheta<<")"<<endl;
	Real weight=pow(sinTheta,2);
	ph.kn=(A/l)*(weight*E1+(1-weight)*E2);
	ph.kt=(A/l)*(weight*G1+(1-weight)*G2);
	ph.tanPhi=min(b1->cast<FrictMat>().tanPhi,b2->cast<FrictMat>().tanPhi);
}

#if 0
#if WOO_OPENGL
WOO_PLUGIN(gl,(GlExtra_LocalAxes));
#endif	
#ifdef WOO_OPENGL
#include<woo/lib/opengl/GLUtils.hpp>

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
