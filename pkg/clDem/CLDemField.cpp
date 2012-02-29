#include<yade/pkg/clDem/CLDemField.hpp>
#include<yade/core/Scene.hpp>

#ifdef YADE_OPENGL
	#include<yade/lib/opengl/GLUtils.hpp>
	#include<yade/pkg/gl/Renderer.hpp>
	#include<yade/lib/opengl/OpenGLWrapper.hpp>
#endif

#include<cl-dem0/cl/Simulation.hpp>

#ifdef YADE_CLDEM

YADE_PLUGIN(cld,(CLDemData)(CLDemField)(CLDemRun));

#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,(Gl1_CLDemField));
#endif

Vector3r v2v(const Vec3& v){ return Vector3r(v[0],v[1],v[2]); }
Quaternionr q2q(const Quat& q){ return Quaternionr(q[3],q[0],q[1],q[2]); }
Matrix3r m2m(const Mat3& m){ Matrix3r ret; ret<<m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8]; return ret; }

bool CLDemField::renderingBbox(Vector3r& mn, Vector3r& mx){
	if(!sim) return false;
	Real inf=std::numeric_limits<Real>::infinity();
	Vector3r a(inf,inf,inf), b(-inf,-inf,-inf);
	for(size_t i=0; i<sim->par.size(); i++){
		if(clDem::par_shapeT_get(&(sim->par[i]))==0) continue;
		a=a.array().min((v2v(sim->par[i].pos)).array()).matrix();
		b=b.array().max((v2v(sim->par[i].pos)).array()).matrix();
	}
	if(a[0]==inf) return false;
	cerr<<"bbox: "<<a.transpose()<<","<<b.transpose()<<endl;
	mn=a; mx=b;
	return true;
}

#if 0
void CLDemRun::pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){
	if(py::len(args)==0) return;
	if(py::len(args)>1) yade::TypeError("CLDemRun takes 0 or 1 non-keybowrd arguments ("+lexical_cast<string>(py::len(args))+" given)");
	py::extract<long> ex(args[0]);
	if(!ex.check()) yade::TypeError("CLDemRun: non-keyword argument must be an integer (stepPeriod)");
	stepPeriod=ex();
	args=py::tuple();
}
#endif

void CLDemRun::run(){
	CLDemField& cldem=field->cast<CLDemField>();
	if(!cldem.sim) throw std::runtime_error("No CLDemField.sim! (beware: it is not saved automatically)");
	cldem.sim->run(steps>0?steps:stepPeriod);
}


#ifdef YADE_OPENGL

void Gl1_CLDemField::renderBboxes(){
	glColor3v(Vector3r(1,1,0));
	for(size_t i=0; i<sim->par.size(); i++){
		if(clDem::par_shapeT_get(&(sim->par[i]))==0) continue; // invalid particle
		if((6*i+5)>=sim->bboxes.size()) continue; // not enough bboxes
		Vector3r mn(sim->bboxes[6*i],sim->bboxes[6*i+1],sim->bboxes[6*i+2]), mx(sim->bboxes[6*i+3],sim->bboxes[6*i+4],sim->bboxes[6*i+5]);
		glPushMatrix();
			glTranslatev(Vector3r(.5*(mn+mx)));
			glScalev(Vector3r(mx-mn));
			glDisable(GL_LINE_SMOOTH);
			glutWireCube(1);
			glEnable(GL_LINE_SMOOTH);
		glPopMatrix();
	}
}

void Gl1_CLDemField::renderParticles(){
	glColor3v(Vector3r(0,1,0));
	for(size_t i=0; i<sim->par.size(); i++){
		int shapeT=clDem::par_shapeT_get(&(sim->par[i]));
		if(shapeT==0) continue; // invalid particle
		glPushMatrix();
			Vector3r pos=v2v(sim->par[i].pos);
			Quaternionr ori=q2q(sim->par[i].ori);
			GLUtils::setLocalCoords(pos,ori);
			switch(shapeT){
				case clDem::Shape_Sphere:{
					glutWireSphere(sim->par[i].shape.sphere.radius,6,6);
					//glutSolidSphere(sim->par[i].shape.sphere.radius,6,6);
				}
			}
		glPopMatrix();
	}
}

void Gl1_CLDemField::go(const shared_ptr<Field>& field, GLViewInfo* _viewInfo){
	//rrr=_viewInfo->renderer;
	cldem=static_pointer_cast<CLDemField>(field);
	sim=cldem->sim;
	viewInfo=_viewInfo;
	// render bboxes
	renderBboxes();
	// render particles
	renderParticles();
	// render potential contacts
	// render contacts
};

#endif

#endif

