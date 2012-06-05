#include<yade/pkg/dem/LawTester.hpp>
#include<yade/lib/pyutil/gil.hpp>
#include<yade/pkg/dem/L6Geom.hpp>

YADE_PLUGIN(dem,(LawTesterStage)(LawTester));

CREATE_LOGGER(LawTester);


void LawTester::run(){
	const auto& dem=static_pointer_cast<DemField>(field);
	if(ids[0]<0 || ids[0]>=(long)dem->particles->size() || !(*dem->particles)[ids[0]] || ids[1]<0 || ids[1]>=(long)dem->particles->size() || !(*dem->particles)[ids[1]]) throw std::runtime_error("LawTester.ids=("+to_string(ids[0])+","+to_string(ids[1])+"): invalid particle ids.");
	const shared_ptr<Particle>& pA=(*dem->particles)[ids[0]];
	const shared_ptr<Particle>& pB=(*dem->particles)[ids[1]];
	if(!pA->shape || pA->shape->nodes.size()!=1 || !pB->shape || pB->shape->nodes.size()!=1) throw std::runtime_error("LawTester: only mono-nodal particles are handled now.");
	Node* nodes[2]={pA->shape->nodes[0].get(),pB->shape->nodes[0].get()};
	DemData* dyns[2]={nodes[0]->getDataPtr<DemData>().get(),nodes[1]->getDataPtr<DemData>().get()};
	for(auto dyn: dyns){
		if(!dyn->impose) dyn->impose=make_shared<Local6Dofs>();
		else if(!dynamic_pointer_cast<Local6Dofs>(dyn->impose)) throw std::runtime_error("LawTester: DemData.impose is set, but it is not a Local6Dofs.");
	}
	Local6Dofs* imposes[2]={static_cast<Local6Dofs*>(dyns[0]->impose.get()),static_cast<Local6Dofs*>(dyns[1]->impose.get())};

	shared_ptr<Contact> C=dem->contacts->find(ids[0],ids[1]);
	// this is null ptr if there is no contact, or the contact is only potential
	if(C && !C->isReal()) C=shared_ptr<Contact>();
	shared_ptr<L6Geom> l6g;
	if(C){
		l6g=dynamic_pointer_cast<L6Geom>(C->geom);
		if(!l6g) throw std::runtime_error("LawTester: C.geom is not a L6Geom");
	}

	// compute "contact" orientation even if there is no contact (in that case, only normal things will be prescribed)
	Quaternionr ori;
	if(!C){
		Vector3r locX=(nodes[1]->pos-nodes[0]->pos).normalized();
		Vector3r locY=locX.cross(abs(locX[1])<abs(locX[2])?Vector3r::UnitY():Vector3r::UnitZ()); locY-=locX*locY.dot(locX); locY.normalize(); Vector3r locZ=locX.cross(locY);Matrix3r T; T.row(0)=locX; T.row(1)=locY; T.row(2)=locZ;
		ori=Quaternionr(T);
	} else {
		ori=C->geom->node->ori;
	}

	if(stage+1>(int)stages.size()) throw std::runtime_error("LawTester.stage out of range (0…"+to_string(((int)stages.size())-1));
	const auto& stg=stages[stage];
	/* we are in this stage for the first time */
	if(stageT0<0){
		stg->step=0;
		stg->time=0;
		stageT0=scene->time;
		/* prescribe values and whats */
		for(int i:{0,1}){
			int sign=(i==0?-1:1);
			Real weight=(i==0?1-abWeight:abWeight);
			DemData* dyn(dyns[i]);
			Local6Dofs* impose(imposes[i]);
			impose->ori=ori;
			impose->whats=stg->whats;
			for(int ix=0;ix<6;ix++){
				// forces are applied to both, only velocity is ditributed
				impose->values[ix]=(impose->whats[ix]==Impose::VELOCITY)?sign*weight*stg->values[ix]:sign*stg->values[ix];
			}
			for(int ix:{1,2,4,5}){
				if(impose->values[ix]!=0 && impose->whats[ix]!=0) throw std::runtime_error("Prescribing non-zero values for indices 1,2,4,5 not yet implemented (whats[i] must be 0)");
			}
		}
	} else {
		/* only update stage values */
		stg->step++;
		stg->time=scene->time-stageT0;
		// update local coords
		for(int i:{0,1}) imposes[i]->ori=ori;
	};

	/* save smooth data */
	if(smoothErr<0) smoothErr=smooth;
	if(!C){
		f=v=u=smooF=smooV=smooU=Vector6r::Constant(NaN);
		fErrRel=fErrAbs=uErrRel=uErrAbs=vErrRel=vErrAbs=Vector6r::Constant(NaN);
	} else {
		f<<C->phys->force,C->phys->torque;
		v<<l6g->vel,l6g->angVel;
		if(isnan(smooF[0])){ // the contact is new in this step (or we are new), just save unsmoothed value
			u=Vector6r::Zero(); u[0]=l6g->uN;
			smooF=f; smooV=v; smooU=u;
			fErrRel=fErrAbs=uErrRel=uErrAbs=vErrRel=vErrAbs=Vector6r::Constant(NaN);
		} else {
			u+=v*scene->dt; // cumulative in u
			u[0]=l6g->uN;
			// smooth all values here
			#define _SMOO_ERR(foo,smooFoo,fooErrRel,fooErrAbs) smooFoo=(1-smooth)*smooFoo+smooth*foo; {\
				Vector6r rErr=((foo.array()-smooFoo.array())/smooFoo.array()).abs().matrix(); Vector6r aErr=(foo.array()-smooFoo.array()).abs().matrix();\
				if(isnan(fooErrRel[0])){ fooErrRel=rErr; fooErrAbs=aErr; } \
				else{ fooErrRel=(1-smoothErr)*fooErrRel+smoothErr*rErr; fooErrAbs=(1-smoothErr)*fooErrAbs+smoothErr*aErr; } \
			}
			_SMOO_ERR(f,smooF,fErrRel,fErrAbs);
			_SMOO_ERR(u,smooU,uErrRel,uErrAbs);
			_SMOO_ERR(v,smooV,vErrRel,vErrAbs);
			#undef _SMOO_ERR
		}
	}

	/* check the result of stg->until */ 
	try{
		GilLock lock; // lock the interpreter for this block
		py::object main=py::import("__main__");
		py::object globals=main.attr("__dict__");
		py::dict locals;
		locals["C"]=C?py::object(C):py::object();
		locals["pA"]=py::object(pA);
		locals["pB"]=py::object(pB);
		locals["stage"]=py::object(stg);
		shared_ptr<LawTester> tester(this,null_deleter());
		shared_ptr<Scene> scene2(scene,null_deleter());
		locals["scene"]=py::object(scene2);
		locals["tester"]=py::object(tester);

		py::object result=py::eval(stg->until.c_str(),globals,locals);
		bool done=py::extract<bool>(result)();
		if(done){
			LOG_INFO("Stage "<<stage<<" done.");
			stageT0=-1;
			if(!stg->done.empty()) py::exec(stg->done.c_str(),globals,locals);
			if(stage<stages.size()-1) stage++;
			else{ // finished
				LOG_INFO("All stages completed, making myself dead");
				dead=true;
			};
			/* ... */
		}
	} catch (py::error_already_set& e){
		throw std::runtime_error("LawTester exception:\n"+parsePythonException());
	}
};
