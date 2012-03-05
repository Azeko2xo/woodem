#ifdef YADE_CLDEM

#include<yade/pkg/clDem/CLDemField.hpp>
#include<yade/core/Scene.hpp>

#ifdef YADE_OPENGL
	#include<yade/lib/opengl/GLUtils.hpp>
	#include<yade/pkg/gl/Renderer.hpp>
	#include<yade/lib/opengl/OpenGLWrapper.hpp>
#endif

#include<cl-dem0/cl/Simulation.hpp>


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
	// cerr<<"bbox: "<<a.transpose()<<","<<b.transpose()<<endl;
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

bool Gl1_CLDemField::parWire;
Real Gl1_CLDemField::quality;
Vector2r Gl1_CLDemField::quality_range;
bool Gl1_CLDemField::bboxes;
bool Gl1_CLDemField::par;
bool Gl1_CLDemField::pot;
bool Gl1_CLDemField::con;
shared_ptr<ScalarRange> Gl1_CLDemField::parRange;

void Gl1_CLDemField::renderBboxes(){
	glColor3v(Vector3r(1,1,0));
	for(size_t i=0; i<sim->par.size(); i++){
		if(clDem::par_shapeT_get(&(sim->par[i]))==0) continue; // invalid particle
		if((6*i+5)>=sim->bboxes.size()) continue; // not enough bboxes
		Vector3r mn(sim->bboxes[6*i],sim->bboxes[6*i+1],sim->bboxes[6*i+2]), mx(sim->bboxes[6*i+3],sim->bboxes[6*i+4],sim->bboxes[6*i+5]);
		glPushMatrix();
			glTranslatev(Vector3r(.5*(mn+mx)));
			glScalev(Vector3r(mx-mn));
			glutWireCube(1);
		glPopMatrix();
	}
}

void Gl1_CLDemField::renderPar(){
	if(!parRange) parRange=make_shared<ScalarRange>();
	for(size_t i=0; i<sim->par.size(); i++){
		int shapeT=clDem::par_shapeT_get(&(sim->par[i]));
		if(shapeT==Shape_None) continue; // invalid particle
		const clDem::Particle& p(sim->par[i]);
		glColor3v(parRange->color(v2v(p.vel).norm()));
		glPushMatrix();
			Vector3r pos=v2v(p.pos);
			// this crashes when compiled with -O3... why??!
			Quaternionr ori=q2q(p.ori);
			switch(shapeT){
				case clDem::Shape_Sphere:{
					GLUtils::setLocalCoords(pos,ori);
					if(parWire){ glDisable(GL_LIGHTING); glutWireSphere(sim->par[i].shape.sphere.radius,slices,stacks); glEnable(GL_LIGHTING); }
					else glutSolidSphere(sim->par[i].shape.sphere.radius,slices,stacks);
					break;
				}
				case clDem::Shape_Wall:{
					// copied from pkg/dem/Wall.cpp (should be moved to a separate func in GLUtils)
					const int div=20;
					int ax0=sim->par[i].shape.wall.axis; int ax1=(ax0+1)%3, ax2=(ax0+2)%3;
					Vector3r corner=viewInfo->sceneCenter-viewInfo->sceneRadius*Vector3r::Ones();
					corner[ax0]=pos[ax0];
					Real step=2*viewInfo->sceneRadius/div;
					Vector3r unit1, unit2; unit1=unit2=Vector3r::Zero();
					unit1[ax1]=step; unit2[ax2]=step;
					glDisable(GL_LINE_SMOOTH);
					glLineWidth(10);
					GLUtils::Grid(corner,unit1,unit2,Vector2i(div,div));
					//cerr<<"corner="<<corner.transpose()<<", unit1="<<unit1.transpose()<<", unit2="<<unit2.transpose()<<", div="<<2*div<<endl;
					GLUtils::GLDrawLine(corner,corner+div*unit1,Vector3r(1,1,1),5);
					GLUtils::GLDrawLine(corner,corner+div*unit2,Vector3r(1,1,1),5);
					GLUtils::GLDrawLine(corner+div*unit2,corner+div*unit1+div*unit2,Vector3r(1,1,1),5);
					GLUtils::GLDrawLine(corner+div*unit1,corner+div*unit1+div*unit2,Vector3r(1,1,1),5);
					GLUtils::GLDrawLine(corner,corner+div*unit1+div*unit2,Vector3r(1,1,1),5);
					GLUtils::GLDrawLine(corner+div*unit1,corner+div*unit2,Vector3r(1,1,1),5);
					break;
				}
				default: cerr<<"[shapeT="<<shapeT<<"?]";
			}
		glPopMatrix();
	}
}

void Gl1_CLDemField::renderPot(){
	glLineWidth(2);
	for(const cl_long2& ids: sim->pot){
		if(ids.s0<0) continue;
		#if 1
			if(ids.s0>=(cl_long)sim->par.size() || ids.s1>=(cl_long)sim->par.size()){
				cerr<<"Gl1_CLDemField::renderPot(): ##"<<ids.s0<<"+"<<ids.s1<<": Only "<<sim->par.size()<<" particles!"<<endl;
				continue;
			}
		#endif
		assert(ids.s0<(cl_long)sim->par.size()); assert(ids.s1<(cl_long)sim->par.size());
		GLUtils::GLDrawLine(v2v(sim->par[ids.s0].pos),v2v(sim->par[ids.s1].pos),Vector3r(0,0,1));
	}
	glLineWidth(1);
}

void Gl1_CLDemField::renderCon(){
	glLineWidth(4);
	for(const clDem::Contact& c: sim->con){
		if(c.ids.s0<0) continue;
		assert(c.ids.s0<(cl_long)sim->par.size()); assert(c.ids.s1<(cl_long)sim->par.size());
		const clDem::Particle& p1(sim->par[c.ids.s0]), &p2(sim->par[c.ids.s1]);
		int s1=clDem::par_shapeT_get(&p1), s2=clDem::par_shapeT_get(&p2);
		Vector3r A,B;
		if((s1==s2 && s1==clDem::Shape_Sphere) || (s1!=clDem::Shape_Sphere && s2!=clDem::Shape_Sphere)){ A=v2v(p1.pos); B=v2v(p2.pos); }
		else if(s1==clDem::Shape_Sphere){ A=v2v(p1.pos); B=v2v(c.pos); }
		else if(s2==clDem::Shape_Sphere){ A=v2v(c.pos); B=v2v(p2.pos); }
		else continue; // unreachable, but makes compiler happy
		GLUtils::GLDrawLine(A,B,Vector3r(0,1,0));
	}
	glLineWidth(1);
}

void Gl1_CLDemField::go(const shared_ptr<Field>& field, GLViewInfo* _viewInfo){
	//rrr=_viewInfo->renderer;
	sim=static_pointer_cast<CLDemField>(field)->sim;
	viewInfo=_viewInfo;
	stacks=6+quality*14;
	slices=stacks;
	//cerr<<"A";

	glDisable(GL_LINE_SMOOTH);
		// render bboxes
		if(bboxes) renderBboxes();
		// render particles
		if(par) renderPar();
		// render potential contacts
		if(pot) renderPot();
		// render contacts
		if(con) renderCon();
	glEnable(GL_LINE_SMOOTH);
};

#endif

#endif

