#ifdef WOO_CLDEM

#include<woo/pkg/clDem/CLDemField.hpp>

#include<woo/core/Scene.hpp>


#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Wall.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/Gravity.hpp>
#include<woo/pkg/dem/IdealElPl.hpp>
#include<woo/pkg/dem/Leapfrog.hpp>
#include<woo/pkg/dem/InsertionSortCollider.hpp>

#ifdef WOO_OPENGL
	#include<woo/lib/opengl/GLUtils.hpp>
	#include<woo/pkg/gl/Renderer.hpp>
	#include<woo/lib/opengl/OpenGLWrapper.hpp>
#endif

// after all other includes, since it ambiguates many class in woo includes otherwise! 
#include<cl-dem0/cl/Simulation.hpp>

WOO_PLUGIN(cld,(CLDemData)(CLDemField)(CLDemRun));

#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_CLDemField));
#endif

Vector3r v2v(const Vec3& v){ return Vector3r(v[0],v[1],v[2]); }
Quaternionr q2q(const Quat& q){ return Quaternionr(q[3],q[0],q[1],q[2]); }
Matrix3r m2m(const Mat3& m){ Matrix3r ret; ret<<m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8]; return ret; }

shared_ptr<clDem::Simulation> CLDemField::getSimulation(){
	shared_ptr<CLDemField> clf;
	for(const auto& f: Master::instance().getScene()->fields){
		clf=dynamic_pointer_cast<CLDemField>(f);
		if(clf) break;
	}
	if(!clf) throw std::runtime_error("No CLDemField in O.Scene.fields");
	if(!clf->sim) throw std::runtime_error("CLDemField.sim==None.");
	return clf->sim;
}


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
	if(py::len(args)>1) woo::TypeError("CLDemField takes at most 1 non-keyword arguments ("+lexical_cast<string>(py::len(args))+" given)");
	if(py::len(args)==0) return;
	py::extract<shared_ptr<clDem::Simulation>> ex(args[0]);
	if(!ex.check()) woo::TypeError("CLDemField: non-keyword arg must be a clDem.Simulation instance.");
	sim=ex();
	args=py::tuple();
}

void CLDemRun::run(){
	CLDemField& cldem=field->cast<CLDemField>();
	sim=cldem.sim;
	if(!sim->context || sim->context!=scene->context){
		scene->ensureCl();
		sim->platform=scene->platform;
		sim->device=scene->device;
		sim->context=scene->context;
		sim->queue=scene->queue;
	}
	if(!sim) throw std::runtime_error("No CLDemField.sim! (beware: it is not saved automatically)");
	// at the very first run, run one step exactly
	// so that the modular arithmetics works
	// when we compare with woo at the end of the step (below)
	// (at the end of 0th step, 1 step has been run already, etc)
	if(scene->step==0){
		LOG_WARN("Running first step");
		bool setDt=sim->scene.dt<0;
		sim->run(1);
		if(setDt){
			scene->dt=sim->scene.dt;
			LOG_WARN("Setting O.scene.dt according to clDem "<<scene->dt);
		}
	}
	else{
		// in case timestep was adjusted in woo
		if(sim->scene.dt!=scene->dt){
			LOG_WARN("Setting clDem Δt="<<sim->scene.dt);
			sim->scene.dt=scene->dt;
		}
		sim->run(steps>0?steps:stepPeriod);
	}
	// for this reason, CLDemRun should always be at the end of the engine sequence!
	if(relTol>0.) doCompare();
}


#define _THROW_ERROR(a){ std::ostringstream o; o<<a; LOG_ERROR(a); throw std::runtime_error(o.str()); }
#define _CHK_ERR(where,err,cexpr,yexpr){ if(err > relTol) LOG_ERROR(where<<": "<<#err<<"="<<err<<">"<<relTol<<": "<<cexpr<<"/"<<yexpr); if(err>relTol*raiseLimit) _THROW_ERROR(where<<": "<<#err<<"="<<err<<">"<<relTol<<"*"<<raiseLimit<<": "<<cexpr<<"/"<<yexpr); }


void CLDemRun::compareParticleNodeDyn(const string& pId, const clDem::Particle& cp, const shared_ptr<Node>& yn, const Real kgU, const Real mU, const Real sU){
	assert(yn->hasData<DemData>());
	// check that positions, orientations, velocities etc match
	const DemData& dyn(yn->getData<DemData>());

	Real posErr=(v2v(cp.pos)-yn->pos).norm()/(mU);
	AngleAxisr cAa(q2q(cp.ori)), yAa(yn->ori);
	Real oriErr=(cAa.axis()*cAa.angle()-yAa.axis()*yAa.angle()).norm();
	Real velErr=(v2v(cp.vel)-dyn.vel).norm()/(mU/sU);
	Real angVelErr=(v2v(cp.angVel)-dyn.angVel).norm()/(1/sU);
	Real angMomErr=(v2v(cp.angMom)-dyn.angMom).norm()/(1/(kgU*pow(mU,2)/sU)); // unit: Nms=kgm²/s
	Real forceErr=(v2v(cp.force)-dyn.force).norm()/(kgU*mU/pow(sU,2));
	Real torqueErr=(v2v(cp.torque)-dyn.torque).norm()/(kgU*pow(mU,2)/pow(sU,2));
	Real massErr=(cp.mass-dyn.mass)/kgU;
	Real inertiaErr=(v2v(cp.inertia)-dyn.inertia).norm()/(kgU*pow(mU,2));

	_CHK_ERR(pId,velErr,cp.vel,dyn.vel);
	_CHK_ERR(pId,angVelErr,cp.angVel,dyn.angVel);
	_CHK_ERR(pId,posErr,cp.pos,yn->pos);
	_CHK_ERR(pId,oriErr,cp.ori,yn->ori);
	_CHK_ERR(pId,angMomErr,cp.angMom,dyn.angMom);
	_CHK_ERR(pId,forceErr,cp.force,dyn.force);
	_CHK_ERR(pId,torqueErr,cp.torque,dyn.torque);
	_CHK_ERR(pId,massErr,cp.mass,dyn.mass);
	_CHK_ERR(pId,inertiaErr,cp.inertia,dyn.inertia);
}


void CLDemRun::doCompare(){
	// get DEM field to compare with
	shared_ptr<DemField> dem;
	FOREACH(const auto& f, scene->fields){
		dem=dynamic_pointer_cast<DemField>(f);
		if(dem) break;
	}
	if(!dem) _THROW_ERROR("No DEM field to compare clDem with. Set CLDemRun.compare=False.");
	if(sim->scene.step!=scene->step) _THROW_ERROR("Step differs: "<<sim->scene.step<<"/"<<scene->step);
	if(sim->scene.dt!=scene->dt) _THROW_ERROR("Δt differs: "<<sim->scene.dt<<"/"<<scene->dt);

	// find contact loop
	shared_ptr<ContactLoop> loop;
	FOREACH(const auto& e, scene->engines){
		loop=dynamic_pointer_cast<ContactLoop>(e);
		if(loop) break;
	}
	if(!loop) _THROW_ERROR("No ContactLoop in scene.engines.");

	shared_ptr<Cp2_FrictMat_FrictPhys> cp2Frict;
	FOREACH(const auto& f, loop->phyDisp->functors){
		cp2Frict=dynamic_pointer_cast<Cp2_FrictMat_FrictPhys>(f);
		if(cp2Frict) break;
	}
	if(!cp2Frict) _THROW_ERROR("No Cp2_FrictMat_FrictPhys in ContactLoop.physDisp.");

	// units so that errors are scaled properly
	Real kgU=NaN, mU=NaN; // taken from particles
	Real sU=scene->dt;

	/* compare particles */
	FOREACH(const shared_ptr< ::Particle> yp, *dem->particles){
		// no particles in woo and clDem
		::Particle::id_t yId=yp->id;
		if(!yp->shape || yp->shape->nodes.empty() || !yp->shape->nodes[0]->hasData<CLDemData>()) _THROW_ERROR("#"<<yId<<": no CLDemData with clDem id information.");
		clDem::par_id_t clId=yp->shape->nodes[0]->getData<CLDemData>().clIx;
		string pId="#"+lexical_cast<string>(clId)+"/"+lexical_cast<string>(yId);
		const clDem::Particle& cp(sim->par[clId]);
		int shapeT=clDem::par_shapeT_get(&cp);
		if(shapeT==Shape_None) _THROW_ERROR(pId<<": not in clDem");
		if(shapeT==Shape_Clump) _THROW_ERROR(pId<<": woo::Particle associated with clump clDem::Particle");

		shared_ptr<Node> yn; /* must be set by shapeT handlers! */

		// set units for relative errors
		if(shapeT==Shape_Sphere) mU=cp.shape.sphere.radius;
		if(cp.mass>0) kgU=cp.mass;
		bool kgFaked=false, mFaked=false; // use unit units, but reset back to NaN afterwards
		if(isnan(kgU)){ kgU=1.; kgFaked=true; }
		if(isnan(mU)){ mU=1.; mFaked=true; }

		// check that materials match
		int matId=clDem::par_matId_get(&cp);
		// check that materials match
		int matT=clDem::mat_matT_get(&(sim->scene.materials[matId]));
		switch(matT){
			case clDem::Mat_ElastMat:{
				if(!dynamic_pointer_cast< ::FrictMat>(yp->material)) _THROW_ERROR(pId<<": material mismatch ElastMat/"<<typeid(*(yp->material)).name());
				const ::FrictMat& ym(yp->material->cast< ::FrictMat>());
				if(!isinf(ym.tanPhi)) _THROW_ERROR(pId<<": woo::FrictMat::tanPhi="<<ym.tanPhi<<" should be infinity to represent frictionless clDem::ElastMat");
				const clDem::ElastMat& cm(sim->scene.materials[matId].mat.elast);
				if(ym.density!=cm.density) _THROW_ERROR(pId<<": density differs "<<cm.density<<"/"<<ym.density);
				if(ym.young!=cm.young) _THROW_ERROR(pId<<": young differes "<<cm.young<<"/"<<ym.young);
				break;
			}
			case clDem::Mat_FrictMat:{
				if(!dynamic_pointer_cast< ::FrictMat>(yp->material)) _THROW_ERROR(pId<<": material mismatch FrictMat/"<<typeid(*(yp->material)).name());
				const ::FrictMat& ym(yp->material->cast< ::FrictMat>());
				const clDem::FrictMat& cm(sim->scene.materials[matId].mat.frict);
				if(ym.density!=cm.density) _THROW_ERROR(pId<<": density differs "<<cm.density<<"/"<<ym.density);
				if(ym.young!=cm.young) _THROW_ERROR(pId<<": young differs "<<cm.young<<"/"<<ym.young);
				if(ym.tanPhi!=cm.tanPhi) _THROW_ERROR(pId<<": young differs "<<cm.tanPhi<<"/"<<ym.tanPhi);
				if(ym.ktDivKn!=cm.ktDivKn) _THROW_ERROR(pId<<": ktDivKn differs "<<cm.ktDivKn<<"/"<<ym.ktDivKn);
				break;
			}
			default:
				_THROW_ERROR(pId<<": mat index "<<matT<<" not handled by the comparator"); continue;
		}

		// check that shapes match
		switch(shapeT){
			case Shape_Sphere:{
				yn=yp->shape->nodes[0];
				if(!dynamic_pointer_cast<woo::Sphere>(yp->shape)) _THROW_ERROR(pId<<": shape mismatch Sphere/"<<typeid(*(yp->shape)).name());
				const woo::Sphere& ys(yp->shape->cast<woo::Sphere>());
				if(ys.radius!=cp.shape.sphere.radius) _THROW_ERROR(pId<<": spheres radius "<<cp.shape.sphere.radius<<"/"<<ys.radius);
				break;
			}
			case Shape_Wall:{
				yn=yp->shape->nodes[0];
				if(!dynamic_pointer_cast< ::Wall>(yp->shape)) _THROW_ERROR(pId<<": shape mismatch Wall/"<<typeid(*(yp->shape)).name());
				const ::Wall& yw(yp->shape->cast< ::Wall>());
				if(yw.axis!=cp.shape.wall.axis) _THROW_ERROR(pId<<": wall axis "<<cp.shape.wall.axis<<"/"<<yw.axis);
				if(yw.sense!=cp.shape.wall.sense) _THROW_ERROR(pId<<": wall sense "<<cp.shape.wall.sense<<"/"<<yw.sense);
				break;
			}
			case Shape_Clump: /* was handled above */ LOG_FATAL("Impossible error: clump here?"); abort();  break;
			default:
				_THROW_ERROR(pId<<": shape index "<<shapeT<<" not handled by the comparator.");
		}
		assert(yn);

		// compare dynamic params
		compareParticleNodeDyn(pId,cp,yn,kgU,mU,sU);

		// reset back to NaN so that another particle might define it right
		if(mFaked) mU=NaN;
		if(kgFaked) kgU=NaN;
	}

	// in case we got no good units, use stupid values here
	if(isnan(kgU)) kgU=1.;
	if(isnan(mU)) mU=1.;
	Real NU=kgU*mU/pow(sU,2); // force unit
	
	// check clumps
	FOREACH(const shared_ptr< ::Node> yn, dem->clumps){
		if(yn->hasData<CLDemData>()) _THROW_ERROR("clump@"<<yn<<": no CLDemData with clDem id information.");
		clDem::par_id_t clId=yn->getData<CLDemData>().clIx;
		string pId="clump#"+lexical_cast<string>(clId)+"/@"+lexical_cast<string>(yn);
		const clDem::Particle& cp(sim->par[clId]);
		int shapeT=clDem::par_shapeT_get(&cp);
		if(shapeT==Shape_None) _THROW_ERROR(pId<<": not in clDem");
		if(shapeT!=Shape_Clump) _THROW_ERROR(pId<<": woo's clump associated with a regular clDem::Particle (shapeT="<<shapeT<<")");

		if(!yn->hasData<DemData>() || !dynamic_pointer_cast<ClumpData>(yn->getDataPtr<DemData>())) _THROW_ERROR(pId<<": does not have associated ClumpData instance");
		const ::ClumpData& ycd(yn->getData<DemData>().cast<ClumpData>());
		long ix=cp.shape.clump.ix;
		if(ix<0 || ix>=(long)sim->clumps.size()) _THROW_ERROR(pId<<": clump index "<<ix<<" is out of range for sim.clumps 〈0.."<<sim->clumps.size()<<")");
		size_t cLen;
		for(cLen=0; sim->clumps[ix+cLen].id>=0; cLen++){
			// last element must sentinel with id<0, therefore we should never come here
			if(ix+cLen==sim->clumps.size()-1) _THROW_ERROR(pId<<": clumps["<<ix+cLen<<"] is last element and should have id<0 (id="<<sim->clumps[ix+cLen].id<<")");
		}
		if(cLen!=ycd.nodes.size()) _THROW_ERROR(pId<<": clumps have different number of members "<<cLen<<"/"<<ycd.nodes.size());
		// assume nodes/particles are in the same order
		for(size_t i=0; i<cLen; i++){
			const ClumpMember& cm=sim->clumps[ix+i];
			if(!ycd.nodes[i]->hasData<CLDemData>()) _THROW_ERROR(pId<<"/"<<i<<" references node without CLDemData");
			// get CLDemData::clIx
			clDem::par_id_t clMemberId=ycd.nodes[i]->getData<CLDemData>().clIx;
			// check that the node referenced from woo is the one of the particle referenced by clDem
			if(clMemberId!=cm.id) _THROW_ERROR(pId<<"/"<<i<<": woo thinks the referenced clDem member should be "<<clMemberId<<", but clDem stores the value of "<<cm.id<<" (are clumps in the same order?)");
			// check relative positions and orientations
			Real relPosErr=(v2v(cm.relPos)-ycd.relPos[i]).norm()/mU;
			AngleAxisr caa(q2q(cm.relOri)), yaa(ycd.relOri[i]);
			Real relOriErr=(caa.axis()*caa.angle()-yaa.axis()*yaa.angle()).norm()/mU;
			_CHK_ERR(pId<<"/"<<i,relPosErr,cm.relPos,ycd.relPos[i]);
			_CHK_ERR(pId<<"/"<<i,relOriErr,cm.relOri,ycd.relOri[i]);
		}

		// compare dynamic params
		compareParticleNodeDyn(pId,cp,yn,kgU,mU,sU);
	}


	/* compare contacts */
	for(const clDem::Contact& cc: sim->con){
		if(cc.ids.s0<0) continue; // invalid contact
		string cId="##"+lexical_cast<string>(cc.ids.s0)+"+"+lexical_cast<string>(cc.ids.s1);
		const shared_ptr< ::Contact>& yc(dem->contacts->find(cc.ids.s0,cc.ids.s1));
		if(!yc){ _THROW_ERROR(cId<<": not in woo"); continue; }
		int geomT=clDem::con_geomT_get(&cc);
		int physT=clDem::con_physT_get(&cc);
		if(!yc->geom && geomT!=clDem::Geom_None) _THROW_ERROR(cId<<": only has geom in clDem");
		if( yc->geom && geomT==clDem::Geom_None) _THROW_ERROR(cId<<": only has geom in woo"); 
		if(!yc->phys && physT!=clDem::Phys_None) _THROW_ERROR(cId<<": only has phys in clDem");
		if( yc->phys && physT==clDem::Phys_None) _THROW_ERROR(cId<<": only has phys in woo"); 
		if(!yc->geom || !yc->phys) continue;
		Real posErr=(v2v(cc.pos)-yc->geom->node->pos).norm();
		_CHK_ERR(cId,posErr,cc.pos,yc->geom->node->pos);
		// force+torque
		Real forceErr=(v2v(cc.force)-yc->phys->force).norm()/(NU);
		Real torqueErr=(v2v(cc.torque)-yc->phys->torque).norm()/(NU*mU);
		_CHK_ERR(cId,forceErr,cc.force,yc->phys->force);
		_CHK_ERR(cId,torqueErr,cc.torque,yc->phys->torque);

		switch(geomT){
			case(clDem::Geom_L6Geom):{
				if(!dynamic_pointer_cast< ::L6Geom>(yc->geom)){ _THROW_ERROR(cId<<": geom mismatch L6Geom/"<<typeid(*(yc->geom)).name()); continue; }
				const ::L6Geom& yg(yc->geom->cast< ::L6Geom>());
				Real oriErr=(m2m(cc.ori)-yg.trsf).norm();
				Real velErr=(v2v(cc.geom.l6g.vel)-yg.vel).norm()/(mU/sU);
				Real angVelErr=(v2v(cc.geom.l6g.angVel)-yg.angVel).norm()/(1/sU);
				Real uNErr=abs(cc.geom.l6g.uN-yg.uN)/(mU);
				_CHK_ERR(cId,oriErr,cc.ori,yg.trsf);
				_CHK_ERR(cId,velErr,cc.geom.l6g.vel,yg.vel);
				_CHK_ERR(cId,angVelErr,cc.geom.l6g.angVel,yg.angVel);
				_CHK_ERR(cId,uNErr,cc.geom.l6g.uN,yg.uN);
				break;
			}
			default: _THROW_ERROR(cId<<": geomT "<<geomT<<" not handled by the comparator"); continue;
		}

		switch(physT){
			case(clDem::Phys_NormPhys):{
				if(!dynamic_pointer_cast< ::FrictPhys>(yc->phys)) _THROW_ERROR(cId<<": phys mismatch NormPhys/"<<typeid(*(yc->phys)).name());
				const ::FrictPhys& yp(yc->phys->cast< ::FrictPhys>());
				Real kNErr=abs(cc.phys.norm.kN-yp.kn)/(NU/mU);
				_CHK_ERR(cId,kNErr,cc.phys.norm.kN,yp.kn);
				// kT not checked
				if(!isinf(yp.tanPhi)) _THROW_ERROR(cId<<": woo::FrictMat::tanPhi="<<yp.tanPhi<<" should be infinity to represent frictionless clDem::NormPhys");
				break;
			}
			case(clDem::Phys_FrictPhys):{
				if(!dynamic_pointer_cast< ::FrictPhys>(yc->phys)) _THROW_ERROR(cId<<": phys mismatch FrictPhys/"<<typeid(*(yc->phys)).name());
				const ::FrictPhys& yp(yc->phys->cast< ::FrictPhys>());
				Real kNErr=abs(cc.phys.frict.kN-yp.kn)/(NU/mU);
				Real kTErr=abs(cc.phys.frict.kT-yp.kt)/(NU/mU);
				Real tanPhiErr=abs(cc.phys.frict.tanPhi-yp.tanPhi)/(NU/mU);
				_CHK_ERR(cId,kNErr,cc.phys.frict.kN,yp.kn);
				_CHK_ERR(cId,kTErr,cc.phys.frict.kT,yp.kt);
				_CHK_ERR(cId,tanPhiErr,cc.phys.frict.tanPhi,yp.tanPhi);
				break;
			}
			default: _THROW_ERROR(cId<<": physT "<<physT<<" not handled by the comparator"); continue;
		}
	}
	/* check the other way */
	if(sim->cpuCollider){
		FOREACH(const auto& C, *dem->contacts){
			Vector2i ids(C->leakPA()->id,C->leakPB()->id);
			string cId="##"+lexical_cast<string>(ids[0])+"+"+lexical_cast<string>(ids[1]);
			clDem::CpuCollider::ConLoc* cl=sim->cpuCollider->find(ids[0],ids[1]);
			if(!cl) LOG_ERROR(cId<<": not in clDem");
		}
	}

	/* compare potential contacts: does not make sense, since verletDist is different */


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
				case ENERGY_frict: name="frict"; break;
				case ENERGY_broken: name="broken"; break;
				default: _THROW_ERROR("Energy number "<<i<<" not handled by the comparator.");
			}
			int yId=-1; scene->energy->findId(name,yId,/*flags*/0,/*newIfNotFound*/false);
			Real ye=0.;
			if(yId>=0) ye=scene->energy->energies.get(yId);
			Real ce=sim->scene.energy[i];
			// NU*mU = N*m = Joule
			//if(fabs(ce)/(NU*mU)<relTol && fabs(ye)/(NU*mU)<relTol) continue;
			Real eErr=abs(ye-ce)/(NU*mU);
			_CHK_ERR("Energy "<<clDem::energyDefinitions[i].name<<"/"<<name,eErr,ce,ye);
			//if(eErr>relTol) LOG_ERROR("Energy: "<<clDem::energyDefinitions[i].name<<"/"<<name<<" differs "<<eErr<<">"<<relTol<<": "<<ce<<"/"<<ye);
		}
	}
}
#undef _CHK_ERR
#undef _THROW_ERROR


/* convert simulation from Yade to clDem, optionally add engines running the clDem simulation alongside */
shared_ptr<clDem::Simulation> CLDemField::wooToClDem(const shared_ptr< ::Scene>& scene, int stepPeriod, Real relTol){
	auto sim=make_shared<clDem::Simulation>();
	shared_ptr<DemField> dem;
	for(const auto& f: scene->fields){ dem=dynamic_pointer_cast<DemField>(f); if(dem) break; }
	if(!dem) throw std::runtime_error("No DemField found");

	scene->ensureCl();
	sim->platform=scene->platform;
	sim->device=scene->device;
	sim->context=scene->context;
	sim->queue=scene->queue;

	sim->scene.dt=scene->dt;
	sim->scene.step=scene->step-1;
	sim->trackEnergy=scene->trackEnergy;
	sim->scene.loneGroups=dem->loneMask;

	std::map< ::Material*,int> ymm; // woo materials, mapping to clDem material numbers
	for(const auto& yp: *dem->particles){
		ymm.insert(std::make_pair(yp->material.get(),ymm.size())); // this makes sure materials are numbered consecutively
	}
	if(ymm.size()>(size_t)clDem::SCENE_MAT_NUM_) throw std::runtime_error("Yade uses "+lexical_cast<string>(ymm.size())+" materials, which is more than the maximum "+lexical_cast<string>(clDem::SCENE_MAT_NUM_)+" clDem was compiled with.");
	// copy materials
	for(const auto& ymi: ymm){
		const ::Material* ym(ymi.first);
		clDem::Material cm;
		const ::FrictMat* yfm=dynamic_cast<const ::FrictMat*>(ym);
		if(yfm){
			clDem::mat_matT_set(&cm,Mat_FrictMat);
			cm.mat.frict.density=ym->density;
			cm.mat.frict.young=yfm->young;
			cm.mat.frict.ktDivKn=yfm->ktDivKn;
			cm.mat.frict.tanPhi=yfm->tanPhi;
			sim->scene.materials[ymi.second]=cm;
			continue;
		}
		// handle ElastMat here, once needed
		throw std::runtime_error(string("Unhandled material ")+typeid(*(ym)).name());
	}

	// create particles
	for(auto& yp: *dem->particles){
		string pId="#"+lexical_cast<string>(yp->id);
		if(!yp->shape) throw std::runtime_error(pId+": Particle.shape==None.");
		clDem::Particle cp;
		par_matId_set(&cp,ymm[yp->material.get()]);
		par_groups_set(&cp,yp->mask);

		auto ysphere=dynamic_pointer_cast<woo::Sphere>(yp->shape);
		auto ywall=dynamic_pointer_cast< ::Wall>(yp->shape);
		bool monoNodal=true;
		if(ysphere){
			clDem::par_shapeT_set(&cp,clDem::Shape_Sphere);
			cp.shape.sphere.radius=ysphere->radius;
		}
		else if(ywall){
			clDem::par_shapeT_set(&cp,clDem::Shape_Wall);
			cp.shape.wall.axis=ywall->axis;
			cp.shape.wall.sense=ywall->sense;
		}
		else {
			throw std::runtime_error(pId+": unhandled Particle.shape type "+typeid(*(yp->shape)).name());
		}
		if(monoNodal){
			// other cases must be taken care of by the shape handler
			if(yp->shape->nodes.size()!=1) throw std::runtime_error(pId+": should have exactly one node, has "+lexical_cast<string>(yp->shape->nodes.size()));
			auto& node(yp->shape->nodes[0]);
			auto& dyn(node->getData<DemData>());
			cp.pos=clDem::fromEigen(node->pos);
			cp.vel=clDem::fromEigen(dyn.vel);
			cp.mass=dyn.mass;
			cp.force=clDem::fromEigen(dyn.force);
			cp.ori=clDem::fromEigen(node->ori);
			cp.angVel=clDem::fromEigen(dyn.angVel);
			cp.angMom=clDem::fromEigen(dyn.angMom);
			cp.inertia=clDem::fromEigen(dyn.inertia);
			cp.torque=clDem::fromEigen(dyn.torque);
			int dofs=0;
			for(int i=0; i<6; i++){
				if(!dyn.isBlockedAxisDOF(i%3,i/3)) dofs|=clDem::dof_axis(i%3,i/3);
			}
			clDem::par_dofs_set(&cp,dofs);
			if(node->hasData<CLDemData>()) throw std::runtime_error(pId+": nodes[0] already has CLDemData.");
			node->setData<CLDemData>(make_shared<CLDemData>());
			node->getData<CLDemData>().clIx=sim->par.size(); // will be pushed back to this position
		}
		sim->par.push_back(cp);
		
		// if we run in parallel with cldem, make dem particles render with wire only
		if(stepPeriod>0) yp->shape->setWire(true);
	}

	// create clumps
	for(const auto& yn: dem->clumps){
		if(!yn->hasData<DemData>() || !dynamic_pointer_cast<ClumpData>(yn->getDataPtr<DemData>())) throw std::runtime_error("clump @ 0x"+lexical_cast<string>(yn.get())+": does not have associated ClumpData instance");
		const ::ClumpData& ycd(yn->getData<DemData>().cast<ClumpData>());
		vector<clDem::par_id_t> cIds;
		for(size_t i=0; i<ycd.nodes.size(); i++){
			const auto& yn=ycd.nodes[i];
			if(!yn->hasData<CLDemData>()) throw runtime_error("clump @0x"+lexical_cast<string>(yn.get())+"/"+lexical_cast<string>(i)+": member without CLDemData?");
			cIds.push_back(yn->getData<CLDemData>().clIx);
		}
		sim->makeClumped(cIds);
	}

	// copy existing contacts
	FOREACH(const shared_ptr< ::Contact>& c, *dem->contacts){
		clDem::Contact con;
		par_id2_t ids={c->leakPA()->id,c->leakPB()->id};
		con.ids=ids;
		sim->con.push_back(con);
	}
	
	// decide which contact model to use
	shared_ptr<ContactLoop> loop;
	shared_ptr<Leapfrog> leapfrog;
	shared_ptr<Gravity> gravity;
	for(const auto& e: scene->engines){
		if(!loop) loop=dynamic_pointer_cast<ContactLoop>(e);
		if(!leapfrog) leapfrog=dynamic_pointer_cast<Leapfrog>(e);
		if(!gravity) gravity=dynamic_pointer_cast<Gravity>(e);
	}
	if(!loop) throw std::runtime_error("No ContactLoop in simulation.");
	if(!loop) throw std::runtime_error("No Leapfrog in simulation.");

	if(!loop->lawDisp) throw std::runtime_error("No ContactLoop.lawDisp");
	if(loop->lawDisp->functors.size()!=1) throw std::runtime_error("Exactly one ContactLoop.lawDisp.functors is required (zero or many found)");
	shared_ptr<LawFunctor> law=loop->lawDisp->functors[0];

	auto linEl6=dynamic_pointer_cast<Law2_L6Geom_FrictPhys_LinEl6>(law);
	auto idealElPl=dynamic_pointer_cast<Law2_L6Geom_FrictPhys_IdealElPl>(law);
	if(linEl6){
		throw std::runtime_error("Law2_L6Geom_FrictPhys_LinEl6 not handled yet.");
		sim->breakTension=false;
		if(isnan(linEl6->charLen) || isinf(linEl6->charLen)) sim->charLen=NaN;
	} else if(idealElPl){
		sim->breakTension=true;
		if(idealElPl->noSlip) throw std::runtime_error("Law2_L6Geom_FrictPhys_IdealElPl.noSlip==True not handled yet.");
		// nothing to do in this case
	} else {
		throw std::runtime_error("Unhandled Law2 functor "+string(typeid(*loop).name()));
	}

	sim->scene.damping=leapfrog->damping;
	sim->scene.gravity=clDem::fromEigen(gravity?gravity->gravity:Vector3r(0,0,0));
	sim->trackEnergy=scene->trackEnergy;
	//sim->scene.step=scene->step-1; // right before
	//sim->scene.t=scene->t-
	sim->scene.loneGroups=dem->loneMask;

	// run in parallel optionally
	if(stepPeriod>=1){
		auto clField=make_shared<CLDemField>();
		clField->sim=sim;
		auto clRun=make_shared<CLDemRun>();
		clRun->stepPeriod=stepPeriod;
		clRun->relTol=relTol;
		clRun->field=clField;
		scene->fields.push_back(clField);
		scene->engines.push_back(clRun);
	}

	return sim;
}


/* convert simulation from clDem to Yade, create engines such that both simulations run in parallel */
shared_ptr< ::Scene> CLDemField::clDemToYade(const shared_ptr<clDem::Simulation>& sim, int stepPeriod, Real relTol){
	auto scene=make_shared< ::Scene>();

	auto dem=make_shared< ::DemField>();
	auto cld=make_shared< ::CLDemField>();
	cld->sim=sim;
	scene->fields={dem,cld};

	// determine timestep here, if negative
	if(sim->scene.dt<0){
		sim->scene.dt=fabs(sim->scene.dt)*sim->pWaveDt();
		if(isinf(sim->scene.dt)) throw std::runtime_error("Invalid p-wave timestep; are there no spherical particles in the simulation?");
		std::cout<<"Note: setting Δt="<<sim->scene.dt<<endl;
	}
	
	// global params
	scene->dt=sim->scene.dt;
	scene->step=sim->scene.step+1;
	scene->trackEnergy=sim->trackEnergy;
	dem->loneMask=sim->scene.loneGroups;

	// materials
	int mmatT=clDem::Mat_None; // track combination of materials, which is not supported

	shared_ptr< ::Material> ymats[clDem::SCENE_MAT_NUM_];
	for(int i=0; i<clDem::SCENE_MAT_NUM_; i++){
		const clDem::Material& cmat(sim->scene.materials[i]);
		int matT=clDem::mat_matT_get(&cmat);
		switch(matT){
			case clDem::Mat_None: break;
			case clDem::Mat_ElastMat:{
				if(mmatT==clDem::Mat_None) mmatT=clDem::Mat_ElastMat;
				if(mmatT!=clDem::Mat_ElastMat) throw std::runtime_error("Combination of materials (ElastMat, FrictMat) not handled.");
				auto fm=make_shared< ::FrictMat>();
				fm->density=cmat.mat.elast.density;
				fm->young=cmat.mat.elast.young;
				fm->tanPhi=Inf; // no friction
				//fm->poisson=NaN; // meaningless
				ymats[i]=fm;
				break;
			}
			case clDem::Mat_FrictMat:{
				if(mmatT==clDem::Mat_None) mmatT=clDem::Mat_FrictMat;
				if(mmatT!=clDem::Mat_FrictMat) throw std::runtime_error("Combination of materials (ElastMat, FrictMat) not handled.");
				//if(isnan(cmat.mat.frict.ktDivKn)) throw std::runtime_error("FrictMat.ktDivKn must not be NaN");
				auto fm=make_shared< ::FrictMat>();
				fm->density=cmat.mat.frict.density;
				fm->young=cmat.mat.frict.young;
				fm->ktDivKn=cmat.mat.frict.ktDivKn;
				fm->tanPhi=cmat.mat.frict.tanPhi;
				//fm->poisson=NaN; // meaningless
				ymats[i]=fm;
				break;
			}
			default: throw std::runtime_error("materials["+lexical_cast<string>(i)+": unhandled matT "+lexical_cast<string>(matT)+".");
		}
	}
	if(mmatT==clDem::Mat_None) throw std::runtime_error("No materials defined.");

	// create engines
	// woo first
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
	if(isnan(sim->scene.verletDist)) collider->dead=true;
	auto loop=make_shared<ContactLoop>();
	loop->geoDisp->add(make_shared<Cg2_Sphere_Sphere_L6Geom>());
	loop->geoDisp->add(make_shared<Cg2_Wall_Sphere_L6Geom>());
	loop->geoDisp->add(make_shared<Cg2_Facet_Sphere_L6Geom>());
	auto frict=make_shared<Cp2_FrictMat_FrictPhys>();
	loop->phyDisp->add(frict);
	loop->applyForces=true;
	switch(mmatT){
		case clDem::Mat_ElastMat:{
			if(sim->breakTension){
				if(!isnan(sim->charLen)) throw std::runtime_error("The combination of breaking contacts and non-NaN charLen (i.e. bending stiffness) is not handled yet!");
				auto law=make_shared<Law2_L6Geom_FrictPhys_IdealElPl>();
				law->noSlip=true;
				loop->lawDisp->add(law);
			} else {
				auto law=make_shared<Law2_L6Geom_FrictPhys_LinEl6>();
				law->charLen=isnan(sim->charLen)?Inf:sim->charLen;
				loop->lawDisp->add(law);
			}
			break;
		}
		case clDem::Mat_FrictMat:{
			auto law=make_shared<Law2_L6Geom_FrictPhys_IdealElPl>();
			law->noSlip=false;
			loop->lawDisp->add(law);
			break;
		}
	}
	// our engine at the end
	auto clDemRun=make_shared<CLDemRun>();
	clDemRun->stepPeriod=stepPeriod;
	clDemRun->initRun=true; // the initial run runs merely one step
	clDemRun->relTol=relTol;

	scene->engines={
		grav,
		integrator,
		collider,
		loop,
		clDemRun,
	};

	std::vector<clDem::par_id_t> wooIds; // ids of particles in woo
	wooIds.resize(sim->par.size(),-1);
	// particles
	for(size_t i=0; i<sim->par.size(); i++){
		const clDem::Particle& cp=sim->par[i];
		auto yp=make_shared< ::Particle>();
		string pId="#"+lexical_cast<string>(i);
		// material
		int matId=par_matId_get(&cp);
		assert(matId>=0 && matId<clDem::SCENE_MAT_NUM_);
		yp->material=ymats[matId];
		assert(yp->material);
		// shape
		int shapeT=par_shapeT_get(&cp);
		// multinodal particles set this to false and copy pos & ori by themselves
		// other particles set to true and will have those copied automatically after the switch block
		bool monoNodal=true; 
		switch(shapeT){
			case Shape_None: continue;
			case Shape_Wall:{
				auto yw=make_shared< ::Wall>();
				yw->sense=cp.shape.wall.sense;
				yw->axis=cp.shape.wall.axis;
				yp->shape=yw;
				break;
			}
			case Shape_Sphere:{
				auto ys=make_shared<woo::Sphere>();
				ys->radius=cp.shape.sphere.radius;
				yp->shape=ys;
				break;
			}
			case Shape_Clump:{
				/* do everything here, not below */
				vector<shared_ptr<Node>> members;
				vector<Particle::id_t> memberIds;
				long ix=cp.shape.clump.ix;
				for(int ii=ix; sim->clumps[ii].id>=0; ii++){
					long id=sim->clumps[ii].id;
					if(id>=(long)dem->particles->size()) throw std::runtime_error(pId+": clump members mut come before the clump (references #"+lexical_cast<string>(id)+")");
					if(!(*dem->particles)[id]->shape->nodes[0]->getData<DemData>().isClumped()) throw std::runtime_error(pId+": clump members should have been marked as clumped (#"+lexical_cast<string>(id));
					// avoid check in CLlumpData::makeClump
					(*dem->particles)[id]->shape->nodes[0]->getData<DemData>().setNoClump();
					members.push_back((*dem->particles)[id]->shape->nodes[0]);
					memberIds.push_back(id);
				}
				auto clump=ClumpData::makeClump(members,memberIds);
				clump->setData<CLDemData>(make_shared<CLDemData>());
				clump->getData<CLDemData>().clIx=i;
				dem->clumps.push_back(clump);
				// void the particle, since there is no particle object for clump
				// that avoids running the block below as well
				yp.reset();
				//dem->particles.insertAt(yp,i);
				break;
			}
			default: throw std::runtime_error(pId+": unhandled shapeT "+lexical_cast<string>(shapeT)+".");
		}
		if(yp){
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
				dyn.angMom=v2v(cp.angMom);
				dyn.mass=cp.mass;
				dyn.inertia=v2v(cp.inertia);
				dyn.force=v2v(cp.force);
				dyn.torque=v2v(cp.torque);
				if(par_clumped_get(&cp)) dyn.setClumped();
				int dofs=par_dofs_get(&cp);
				for(int i=0; i<6; i++){
					// clDem defines free dofs, woo defines blocked dofs
					if(!(dofs&clDem::dof_axis(i%3,i/3))) dyn.flags|=::DemData::axisDOF(i%3,i/3);
				}
				node->setData<CLDemData>(make_shared<CLDemData>());
				node->getData<CLDemData>().clIx=i;
			}
			// not yet implemented in cl-dem0
			// if(par_stateT_get(&cp)!=clDem::State_None)
			yp->shape->color=(par_dofs_get(&cp)==0?.5:.3);
			yp->shape->setWire(true);
			yp->mask=par_groups_get(&cp);
			wooIds[i]=dem->particles->insert(yp);
		}
	}
	// real/"real" contacts
	for(const clDem::Contact& c: sim->con){
		string cId="##"+lexical_cast<string>(c.ids.s0)+"+"+lexical_cast<string>(c.ids.s1);
		if(c.ids.s0<0) continue; // invalid contact
		if(clDem::con_geomT_get(&c)!=Geom_None) throw std::runtime_error(cId+": pre-existing geom not handled yet.");
		if(clDem::con_physT_get(&c)!=Phys_None) throw std::runtime_error(cId+": pre-existing phys not handled yet.");
		auto yc=make_shared< ::Contact>();
		::Particle::id_t idA=wooIds[c.ids.s0], idB=wooIds[c.ids.s1];
		assert(idA>=0 && idA<(long)dem->particles->size());
		assert(idB>=0 && idB<(long)dem->particles->size());
		yc->pA=(*dem->particles)[idA];
		yc->pB=(*dem->particles)[idB];
		dem->contacts->add(yc);
	}

	// without collisions detection, potential contacts are given in advance
	if(isnan(sim->scene.verletDist)){
		// potential contacts
		for(const clDem::par_id2_t ids: sim->pot){
			string cId="pot##"+lexical_cast<string>(ids.s0)+"+"+lexical_cast<string>(ids.s1);
			auto yc=make_shared< ::Contact>();
			if(ids.s0<0) continue;
			::Particle::id_t idA=wooIds[ids.s0], idB=wooIds[ids.s1];
			assert(idA>=0 && idA<(long)dem->particles->size());
			assert(idB>=0 && idB<(long)dem->particles->size());
			yc->pA=(*dem->particles)[idA];
			yc->pB=(*dem->particles)[idB];
			dem->contacts->add(yc);
		}
	}

	dem->collectNodes();

	// set engine's fields and scene pointers
	scene->postLoad(*scene,NULL);
	return scene;
};


#ifdef WOO_OPENGL

bool Gl1_CLDemField::parWire;
Real Gl1_CLDemField::quality;
Vector2r Gl1_CLDemField::quality_range;
bool Gl1_CLDemField::bboxes;
bool Gl1_CLDemField::par;
bool Gl1_CLDemField::pot;
bool Gl1_CLDemField::con;
shared_ptr<ScalarRange> Gl1_CLDemField::parRange;
shared_ptr<ScalarRange> Gl1_CLDemField::conRange;

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
	if(!conRange) conRange=make_shared<ScalarRange>();
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
					Vector3r unit1=Vector3r::Zero(), unit2=Vector3r::Zero();
					unit1[ax1]=unit2[ax2]=step;
					glLineWidth(2);
					GLUtils::Grid(corner,unit1,unit2,Vector2i(div,div),/*edgeMask*/0);
					break;
				}
				case clDem::Shape_Clump:{
					// draw lines between the clump and its members
					GLUtils::setLocalCoords(pos,ori);
					for(int i=p.shape.clump.ix; sim->clumps[i].id<0; i++){
						GLUtils::GLDrawLine(Vector3r::Zero(),v2v(sim->clumps[i].relPos),/*color*/Vector3r(1,0,0),/*width*/2);
					}
					break;
				}
				default: cerr<<"[gl:shapeT="<<shapeT<<"?]";
			}
		glPopMatrix();
	}
}

void Gl1_CLDemField::renderPot(){
	glLineWidth(2);
	for(const par_id2_t& ids: sim->pot){
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
		GLUtils::GLDrawLine(A,B,conRange->color(c.force[0]));
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

#endif /* WOO_OPENGL */

#endif /* WOO_CLDEM */

