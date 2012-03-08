#ifdef YADE_CLDEM

#include<yade/pkg/clDem/CLDemField.hpp>
#include<yade/core/Scene.hpp>


#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/Sphere.hpp>
#include<yade/pkg/dem/Wall.hpp>
#include<yade/pkg/dem/L6Geom.hpp>
#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/pkg/dem/Gravity.hpp>
#include<yade/pkg/dem/IdealElPl.hpp>
#include<yade/pkg/dem/Leapfrog.hpp>
#include<yade/pkg/dem/InsertionSortCollider.hpp>

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

void CLDemField::pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){
	if(py::len(args)>1) yade::TypeError("CLDemField takes at most 1 non-keyword arguments ("+lexical_cast<string>(py::len(args))+" given)");
	if(py::len(args)==0) return;
	py::extract<shared_ptr<clDem::Simulation>> ex(args[0]);
	if(!ex.check()) yade::TypeError("CLDemField: non-keyword arg must be a clDem.Simulation instance.");
	sim=ex();
	args=py::tuple();
}

void CLDemRun::run(){
	CLDemField& cldem=field->cast<CLDemField>();
	sim=cldem.sim;
	sim->scene.dt=scene->dt;
	if(!sim) throw std::runtime_error("No CLDemField.sim! (beware: it is not saved automatically)");
	// in case timestep was adjusted in yade
	sim->run(steps>0?steps:stepPeriod);
	if(compare) doCompare();
}


/* TODO: define tolerance when exception is thrown, rather than just showing errors */
void CLDemRun::doCompare(){
	// get DEM field to compare with
	shared_ptr<DemField> dem;
	FOREACH(const auto& f, scene->fields){
		if(dynamic_pointer_cast<DemField>(f)){ dem=static_pointer_cast<DemField>(f); break; }
	}
	if(sim->scene.step!=scene->step) LOG_ERROR("Step differs: "<<sim->scene.step<<"/"<<scene->step);
	if(sim->scene.dt!=scene->dt) LOG_ERROR("Δt differs: "<<sim->scene.dt<<"/"<<scene->dt);

	// units so that errors are scaled properly
	Real kgU=NaN, mU=NaN; // taken from particles
	Real sU=scene->dt;

	/* compare particles */
	if(dem->particles.size()!=sim->par.size()) LOG_ERROR("Differing number of particles: "<<sim->par.size()<<"/"<<dem->particles.size());
	// compare particle's positions, orientations
	for(size_t id=0; id<sim->par.size(); id++){
		// no particles in yade and clDem
		if(id>=dem->particles.size()){ LOG_ERROR("#"<<id<<" not in Yade"); continue; }
		int shapeT=clDem::par_shapeT_get(&(sim->par[id]));
		if(!dem->particles[id] && shapeT==Shape_None) continue;
		if(!dem->particles[id]){ LOG_ERROR("#"<<id<<" not in Yade"); continue; }
		if(shapeT==Shape_None){ LOG_ERROR("#"<<id<<" not in clDem"); continue; }
		const clDem::Particle& cp(sim->par[id]);
		const shared_ptr< ::Particle>& yp(dem->particles[id]);
		// check that shapes match
		switch(shapeT){
			case Shape_Sphere:{
				if(!dynamic_pointer_cast< ::Sphere>(yp->shape)){ LOG_ERROR("#"<<id<<": shape mismatch Sphere/"<<typeid(*(yp->shape)).name()); continue; }
				const ::Sphere& ys(yp->shape->cast< ::Sphere>());
				if(ys.radius!=cp.shape.sphere.radius){ LOG_ERROR("#"<<id<<": spheres radius "<<cp.shape.sphere.radius<<"/"<<ys.radius); continue; }
				break;
			}
			case Shape_Wall:{
				if(!dynamic_pointer_cast< ::Wall>(yp->shape)){ LOG_ERROR("#"<<id<<": shape mismatch Wall/"<<typeid(*(yp->shape)).name()); continue; }
				const ::Wall& yw(yp->shape->cast< ::Wall>());
				if(yw.axis!=cp.shape.wall.axis){ LOG_ERROR("#"<<id<<": wall axis "<<cp.shape.wall.axis<<"/"<<yw.axis); continue; }
				if(yw.sense!=cp.shape.wall.sense){ LOG_ERROR("#"<<id<<": wall sense "<<cp.shape.wall.sense<<"/"<<yw.sense); continue; }
				break;
			}
			default:
				LOG_ERROR("#"<<id<<": shape index "<<shapeT<<" not handled by the comparator."); continue;
		}

		// check that materials match
		int matId=clDem::par_matId_get(&(sim->par[id]));
		int matT=clDem::mat_matT_get(&(sim->scene.materials[matId]));
		switch(matT){
			case clDem::Mat_ElastMat:{
				if(!dynamic_pointer_cast< ::FrictMat>(yp->material)){ LOG_ERROR("#"<<id<<": material mismatch ElastMat/"<<typeid(*(yp->material)).name()); continue; }
				const ::FrictMat& ym(yp->material->cast<FrictMat>());
				if(!isinf(ym.tanPhi)) LOG_ERROR("#"<<id<<": yade::FrictMat::tanPhi="<<ym.tanPhi<<" should be infinity to represent frictionless clDem::ElastMat");
				const clDem::ElastMat& cm(sim->scene.materials[matId].mat.elast);
				if(ym.density!=cm.density){ LOG_ERROR("#"<<id<<": density differs "<<cm.density<<"/"<<ym.density); continue; }
				if(ym.young!=cm.young){ LOG_ERROR("#"<<id<<": young differes "<<cm.young<<"/"<<ym.young); continue; }
				break;
			}
			default:
				LOG_ERROR("#"<<id<<": mat index "<<matT<<" not handled by the comparator"); continue;
		}

		// set units for relative errors
		if(shapeT==Shape_Sphere) mU=cp.shape.sphere.radius;
		if(cp.mass>0) kgU=cp.mass;
		bool kgFaked=false, mFaked=false; // use unit units, but reset back to NaN afterwards
		if(isnan(kgU)){ kgU=1.; kgFaked=true; }
		if(isnan(mU)){ mU=1.; mFaked=true; }


		// check that positions, orientations, velocities etc match
		const auto& yn=yp->shape->nodes[0];
		const DemData& dyn(yn->getData<DemData>());
		const Vector3r &pos(yn->pos), &vel(dyn.vel), &angVel(dyn.angVel), &force(dyn.force), &torque(dyn.torque);
		const Quaternionr &ori(yn->ori);
		Real posErr=(v2v(cp.pos)-pos).norm()/(mU);
		AngleAxisr cAa(q2q(cp.ori)), yAa(ori);
		Real oriErr=(cAa.axis()*cAa.angle()-yAa.axis()*yAa.angle()).norm();
		Real velErr=(v2v(cp.vel)-vel).norm()/(mU/sU);
		Real angVelErr=(v2v(cp.angVel)-angVel).norm()/(1/sU);
		Real forceErr=(v2v(cp.force)-force).norm()/(kgU*mU/pow(sU,2));
		Real torqueErr=(v2v(cp.torque)-torque).norm()/(kgU*pow(mU,2)/pow(sU,2));
		#define _CHK_ERR(what) if(what ## Err > relTol) LOG_ERROR("#"<<id<<": "<<#what<<"Err="<<what ## Err<<">"<<relTol<<": "<<cp.what<<"/"<<what);
			_CHK_ERR(pos);
			_CHK_ERR(ori);
			_CHK_ERR(vel);
			_CHK_ERR(angVel);
			_CHK_ERR(force);
			_CHK_ERR(torque);
		#undef _CHK_ERR

		// reset back to NaN so that another particle might define it right
		if(mFaked) mU=NaN;
		if(kgFaked) kgU=NaN;
	}

	if(isnan(kgU)) kgU=1.;
	if(isnan(mU)) mU=1.;
	Real NU=kgU*mU/pow(sU,2); // force unit
	
	/* compare contacts */
	// TODO: check that all contacts in yade exist in clDem
	for(const clDem::Contact& cc: sim->con){
		if(cc.ids.s0<0) continue; // invalid contact
		string cId="##"+lexical_cast<string>(cc.ids.s0)+"+"+lexical_cast<string>(cc.ids.s1);
		const shared_ptr< ::Contact>& yc(dem->contacts.find(cc.ids.s0,cc.ids.s1));
		if(!yc){ LOG_ERROR(cId<<": not in yade"); continue; }
		if(!yc->isReal()){ LOG_ERROR(cId<<": only potential in yade"); continue; }
		#define _CHK_ERR(err,cexpr,yexpr) if(err > relTol) LOG_ERROR(cId<<": "<<#err<<"="<<err<<">"<<relTol<<": "<<cexpr<<"/"<<yexpr);
		Real posErr=(v2v(cc.pos)-yc->geom->node->pos).norm();
		_CHK_ERR(posErr,cc.pos,yc->geom->node->pos);
		// force+torque
		Real forceErr=(v2v(cc.force)-yc->phys->force).norm()/(NU);
		Real torqueErr=(v2v(cc.torque)-yc->phys->torque).norm()/(NU*mU);
		_CHK_ERR(forceErr,cc.force,yc->phys->force);
		_CHK_ERR(torqueErr,cc.torque,yc->phys->torque);

		int geomT=clDem::con_geomT_get(&cc);
		switch(geomT){
			case(clDem::Geom_L6Geom):{
				if(!dynamic_pointer_cast< ::L6Geom>(yc->geom)){ LOG_ERROR(cId<<": geom mismatch L6Geom/"<<typeid(*(yc->geom)).name()); continue; }
				const ::L6Geom yg(yc->geom->cast< ::L6Geom>());
				Real oriErr=(m2m(cc.ori)-yg.trsf).norm();
				Real velErr=(v2v(cc.geom.l6g.vel)-yg.vel).norm()/(mU/sU);
				Real angVelErr=(v2v(cc.geom.l6g.angVel)-yg.angVel).norm()/(1/sU);
				Real uNErr=abs(cc.geom.l6g.uN-yg.uN)/(mU);
				_CHK_ERR(oriErr,cc.ori,yg.trsf);
				_CHK_ERR(velErr,cc.geom.l6g.vel,yg.vel);
				_CHK_ERR(angVelErr,cc.geom.l6g.angVel,yg.angVel);
				_CHK_ERR(uNErr,cc.geom.l6g.uN,yg.uN);
				break;
			}
			default: LOG_ERROR(cId<<": geomT "<<geomT<<" not handled by the comparator"); continue;
		}

		int physT=clDem::con_physT_get(&cc);
		switch(physT){
			case(clDem::Phys_NormPhys):{
				if(!dynamic_pointer_cast< ::FrictPhys>(yc->phys)){ LOG_ERROR(cId<<": phys mismatch NormPhys/"<<typeid(*(yc->phys)).name()); continue; }
				const ::FrictPhys yp(yc->phys->cast< ::FrictPhys>());
				Real kNErr=abs(cc.phys.normPhys.kN-yp.kn)/(NU/mU);
				_CHK_ERR(kNErr,cc.phys.normPhys.kN,yp.kn);
				// kT not checked
				if(!isinf(yp.tanPhi)) LOG_ERROR(cId<<": yade::FrictMat::tanPhi="<<yp.tanPhi<<" should be infinity to represent frictionless clDem::NormPhys");
				break;
			}
			default: LOG_ERROR(cId<<": physT "<<physT<<" not handled by the comparator"); continue;
		}
		#undef _CHK_ERR
	}
	/* compare potential contacts */


	/* compare energies */
	if(scene->trackEnergy){
		// Real eAbsSum=0; for(int i=0; i<clDem::SCENE_ENERGY_NUM; i++) eAbsSum+=abs(sim->scene.energy[i]); 
		for(int i=0; i<clDem::SCENE_ENERGY_NUM; i++){
			string name;
			switch(i){
				case ENERGY_Ekt: name="kinTrans"; break;
				case ENERGY_Ekr: name="kinRot"; break;
				case ENERGY_grav: name="grav"; break;
				case ENERGY_damp: name="nonviscDamp"; break;
				case ENERGY_elast: name="elast"; break;
				default: LOG_ERROR("Energy number "<<i<<" not handled by the comparator.");
			}
			int yId=-1; scene->energy->findId(name,yId,/*flags*/0,/*newIfNotFound*/false);
			Real ye=0.;
			if(yId>=0) ye=scene->energy->energies.get(yId);
			Real ce=sim->scene.energy[i];
			// NU*mU = N*m = Joule
			//if(fabs(ce)/(NU*mU)<relTol && fabs(ye)/(NU*mU)<relTol) continue;
			Real eErr=abs(ye-ce)/(NU*mU);
			if(eErr>relTol) LOG_ERROR("Energy: "<<clDem::energyDefinitions[i].name<<"/"<<name<<" differs "<<eErr<<">"<<relTol<<": "<<ce<<"/"<<ye);
		}
	}
}


shared_ptr< ::Scene> CLDemRun::clDemToYade(const shared_ptr<clDem::Simulation>& sim, int stepPeriod){
	auto scene=make_shared< ::Scene>();

	auto dem=make_shared< ::DemField>();
	auto cld=make_shared< ::CLDemField>();
	cld->sim=sim;
	scene->fields={dem,cld};

	// global params
	scene->dt=sim->scene.dt;
	scene->step=sim->scene.step+1;
	scene->trackEnergy=true;

	// create engines
	auto grav=make_shared<Gravity>();
	grav->gravity=v2v(sim->scene.gravity);
	auto integrator=make_shared<Leapfrog>();
	integrator->damping=sim->scene.damping;
	integrator->reset=true;
	integrator->kinSplit=true;
	auto collider=make_shared<InsertionSortCollider>();
	collider->boundDispatcher->add(make_shared<Bo1_Sphere_Aabb>());
	collider->boundDispatcher->add(make_shared<Bo1_Wall_Aabb>());
	collider->boundDispatcher->add(make_shared<Bo1_Facet_Aabb>());
	auto loop=make_shared<ContactLoop>();
	loop->geoDisp->add(make_shared<Cg2_Sphere_Sphere_L6Geom>());
	loop->geoDisp->add(make_shared<Cg2_Wall_Sphere_L6Geom>());
	loop->geoDisp->add(make_shared<Cg2_Facet_Sphere_L6Geom>());
	auto frict=make_shared<Cp2_FrictMat_FrictPhys>();
	frict->ktDivKn=.8; // FIXME: use value from clDem somehow!
	loop->phyDisp->add(frict);
	auto law=make_shared<Law2_L6Geom_FrictPhys_LinEl6>();
	law->charLen=500; // FIXME: use value from clDem somehow!
	loop->lawDisp->add(law);
	auto intra=make_shared<IntraForce>();
	intra->add(make_shared<In2_Sphere_ElastMat>());
	auto clDemRun=make_shared<CLDemRun>();
	clDemRun->stepPeriod=stepPeriod;
	clDemRun->compare=true;
	clDemRun->relTol=1e-5;
	scene->engines={
		grav,
		integrator,
		collider,
		loop,
		intra,
		clDemRun
	};

	// materials
	shared_ptr< ::Material> ymats[clDem::SCENE_MAT_NUM];
	for(int i=0; i<clDem::SCENE_MAT_NUM; i++){
		const clDem::Material& cmat(sim->scene.materials[i]);
		int matT=clDem::mat_matT_get(&cmat);
		switch(matT){
			case clDem::Mat_None: break;
			case clDem::Mat_ElastMat:{
				auto fm=make_shared< ::FrictMat>();
				fm->density=cmat.mat.elast.density;
				fm->young=cmat.mat.elast.young;
				fm->tanPhi=Inf; // no friction
				fm->poisson=NaN; // meaningless
				ymats[i]=fm;
				break;
			}
			default: throw std::runtime_error("materials["+lexical_cast<string>(i)+": unhandled matT "+lexical_cast<string>(matT)+".");
		}
	}
	// particles
	for(size_t i=0; i<sim->par.size(); i++){
		const clDem::Particle& cp=sim->par[i];
		auto yp=make_shared< ::Particle>();
		string pId="#"+lexical_cast<string>(i);
		// material
		int matId=par_matId_get(&cp);
		assert(matId>=0 && matId<clDem::SCENE_MAT_NUM);
		yp->material=ymats[matId];
		assert(yp->material);
		// shape
		int shapeT=par_shapeT_get(&cp);
		// multinodal particles set this to false and copy pos & ori by themselves
		// other particles set to true and will have those copied automatically after the switch block
		bool monoNodal=false; 
		switch(shapeT){
			case Shape_None: continue;
			case Shape_Wall:{
				auto yw=make_shared< ::Wall>();
				yw->sense=cp.shape.wall.sense;
				yw->axis=cp.shape.wall.axis;
				yp->shape=yw;
				monoNodal=true;
				break;
			}
			case Shape_Sphere:{
				auto ys=make_shared< ::Sphere>();
				ys->radius=cp.shape.sphere.radius;
				yp->shape=ys;
				monoNodal=true;
				break;
			}
			default: throw std::runtime_error(pId+": unhandled shapeT "+lexical_cast<string>(shapeT)+".");
		}
		// copy pos, ori, vel, angVel & c
		if(monoNodal){
			yp->shape->nodes.resize(1);
			auto& node=yp->shape->nodes[0];
			node=make_shared<Node>();
			node->pos=v2v(cp.pos);
			node->ori=q2q(cp.ori);
			node->setData<DemData>(make_shared<DemData>());
			DemData& dyn=node->getData<DemData>();
			dyn.vel=v2v(cp.vel);
			dyn.angVel=v2v(cp.angVel);
			dyn.mass=cp.mass;
			dyn.inertia=v2v(cp.inertia);
			dyn.force=v2v(cp.force);
			dyn.torque=v2v(cp.torque);
			int dofs=par_dofs_get(&cp);
			for(int i=0; i<6; i++){
				// clDem defines free dofs, yade defines blocked dofs
				if(!(dofs&clDem::dof_axis(i%3,i/3))) dyn.flags|=::DemData::axisDOF(i%3,i/3);
			}
		}
		// not yet implemented in cl-dem0
		// if(par_stateT_get(&cp)!=clDem::State_None)
		yp->shape->color=(par_dofs_get(&cp)==0?.5:.3);
		yp->shape->setWire(true);

		dem->particles.insertAt(yp,i);
	}
	if(!sim->con.empty()) throw std::runtime_error("Copying contacts from clDem to yade is not implemented yet.");

	//

	dem->collectNodes();

	// set engine's fields and scene pointers
	scene->postLoad(*scene);
	return scene;
};


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
	glColor3v(Vector3r(.4,.7,.2));
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

