#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/pkg/dem/Sphere.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>

shared_ptr<DemField> getDemField(Scene* scene){
	shared_ptr<DemField> ret;
	FOREACH(const shared_ptr<Field>& f, scene->fields){
		if(dynamic_pointer_cast<DemField>(f)){
			if(ret) yade::RuntimeError("Ambiguous: more than one DemField in Scene.fields.");
			ret=static_pointer_cast<DemField>(f);
		}
	}
	if(!ret) yade::RuntimeError("No DemField in Scene.fields.");
	return ret;
}

Real pWaveTimeStep(){
	Scene* scene=Omega::instance().getScene().get(); DemField* field=getDemField(scene).get();
	Real dt=std::numeric_limits<Real>::infinity();
	FOREACH(const shared_ptr<Particle>& b, field->particles){
		if(!b || !b->material || !b->shape || b->shape->nodes.size()!=1 || !b->shape->nodes[0]->hasData<DemData>()) continue;
		const shared_ptr<ElastMat>& elMat=dynamic_pointer_cast<ElastMat>(b->material);
		const shared_ptr<Sphere>& s=dynamic_pointer_cast<Sphere>(b->shape);
		if(!elMat || !s) continue;
		dt=min(dt,s->radius/sqrt(elMat->young/elMat->density));
	}
	return dt;
}


vector<shared_ptr<Contact> > createContacts(const vector<Particle::id_t>& ids1, const vector<Particle::id_t>& ids2, const vector<shared_ptr<CGeomFunctor> >& cgff, const vector<shared_ptr<CPhysFunctor> >& cpff, bool force){
	if(ids1.size()!=ids2.size()) yade::ValueError("id1 and id2 arguments must have same length.");
	if(cgff.size()+cpff.size()>0 && cgff.size()*cpff.size()==0) yade::ValueError("Either both CGeomFunctors and CPhysFunctors must be specified, or neither of them (now: "+lexical_cast<string>(cgff.size())+", "+lexical_cast<string>(cpff.size())+")");
	Scene* scene=Omega::instance().getScene().get(); shared_ptr<DemField> dem=getDemField(scene);
	shared_ptr<CGeomDispatcher> gDisp; shared_ptr<CPhysDispatcher> pDisp;
	if(cgff.empty()){ // find dispatchers in current engines
		FOREACH(const shared_ptr<Engine>& e, scene->engines){
			ContactLoop* cl(dynamic_cast<ContactLoop*>(e.get()));
			if(!cl) continue;
			gDisp=cl->geoDisp; pDisp=cl->phyDisp;
			break;
		}
		if(!gDisp) yade::RuntimeError("No ContactLoop in Scene.engines; supply CGeomFunctors and CPhysFunctors explicitly.");
	} else { // create dispatcher objects from supplied functors
		gDisp=shared_ptr<CGeomDispatcher>(new CGeomDispatcher);
		pDisp=shared_ptr<CPhysDispatcher>(new CPhysDispatcher);
		FOREACH(const shared_ptr<CGeomFunctor>& cgf, cgff) gDisp->add(cgf);
		FOREACH(const shared_ptr<CPhysFunctor>& cpf, cpff) pDisp->add(cpf);
	}
	assert(gDisp && pDisp);
	gDisp->scene=pDisp->scene=scene; gDisp->field=pDisp->field=dem; gDisp->updateScenePtr(); pDisp->updateScenePtr();
	vector<shared_ptr<Contact> > ret;
	for(int k=0; k<(int)ids1.size(); k++){
		Particle::id_t id1=ids1[k], id2=ids2[k];
		const shared_ptr<Particle>& b1=dem->particles.safeGet(id1); const shared_ptr<Particle>& b2=dem->particles.safeGet(id2);
		if(b1->contacts.find(id2)!=b1->contacts.end()) yade::ValueError("Contact ##"+lexical_cast<string>(id1)+"+"+lexical_cast<string>(id2)+" already exists.");
		shared_ptr<Contact> C=gDisp->explicitAction(scene,b1,b2,/*force*/force);
		if(force && !C) throw std::logic_error("CGeomFunctor did not create contact, although force==true");
		if(!C) continue;
		pDisp->explicitAction(scene,b1->material,b2->material,C);
		C->stepMadeReal=scene->step;
		dem->contacts.add(C);
		ret.push_back(C);
	}
	return ret;
}

py::tuple stressStiffness(Real volume=0, bool skipMultinodal=false){
	Matrix6r K(Matrix6r::Zero());
	Matrix3r stress(Matrix3r::Zero());
	Scene* scene=Omega::instance().getScene().get(); DemField* dem=getDemField(scene).get();
	FOREACH(const shared_ptr<Contact>& C, dem->contacts){
		FrictPhys* phys=YADE_CAST<FrictPhys*>(C->phys.get());
		if(C->pA->shape->nodes.size()!=1 || C->pB->shape->nodes.size()!=1){
			if(skipMultinodal) continue;
			else yade::ValueError("Particle "+lexical_cast<string>(C->pA->shape->nodes.size()!=1? C->pA->id : C->pB->id)+" has more than one node; to skip contacts with such particles, say skipMultinodal=True");
		}
		// use current distance here
		const Real d0=(C->pB->shape->nodes[0]->pos-C->pA->shape->nodes[0]->pos+(scene->isPeriodic?scene->cell->intrShiftPos(C->cellDist):Vector3r::Zero())).norm();
		const Vector3r& n=C->geom->node->ori.conjugate()*Vector3r::UnitX(); // normal in global coords
		// contact force, in global coords
		Vector3r F=C->geom->node->ori.conjugate()*C->phys->force;
		Real fN=F.dot(n);
		Vector3r fT=F-n*F.dot(n);
		for(int i=0; i<3; i++) for(int j=0;j<3; j++){ stress(i,j)+=d0*(fN*n[i]*n[j]+.5*(fT[i]*n[j]+fT[j]*n[i])); }
		const Real& kN=phys->kn; const Real& kT=phys->kt;
		// mapping between 6x6 matrix indices and tensor indices (Voigt notation)
		// only upper triangle used here
		const int map[6][6][4]={
			{{0,0,0,0},{0,0,1,1},{0,0,2,2},{0,0,1,2},{0,0,2,0},{0,0,0,1}},
			{{1,1,0,0},{1,1,1,1},{1,1,2,2},{1,1,1,2},{1,1,2,0},{1,1,0,1}},
			{{2,2,0,0},{2,2,1,1},{2,2,2,2},{2,2,1,2},{2,2,2,0},{2,2,0,1}},
			{{1,2,0,0},{1,2,1,1},{1,2,2,2},{1,2,1,2},{1,2,2,0},{1,2,0,1}},
			{{2,0,0,0},{2,0,1,1},{2,0,2,2},{2,0,1,2},{2,0,2,0},{2,0,0,1}},
			{{0,1,0,0},{0,1,1,1},{0,1,2,2},{0,1,1,2},{0,1,2,0},{0,1,0,1}}};
		const int kron[3][3]={{1,0,0},{0,1,0},{0,0,1}}; // Kronecker delta
		for(int p=0; p<6; p++) for(int q=p;q<6;q++){
			int i=map[p][q][0], j=map[p][q][1], k=map[p][q][2], l=map[p][q][3];
			K(p,q)+=d0*d0*(kN*n[i]*n[j]*n[k]*n[l]+kT*(.25*(n[j]*n[k]*kron[i][l]+n[j]*n[l]*kron[i][k]+n[i]*n[k]*kron[j][l]+n[i]*n[l]*kron[j][k])-n[i]*n[j]*n[k]*n[l]));
		}
	}
	for(int p=0;p<6;p++)for(int q=p+1;q<6;q++) K(q,p)=K(p,q); // symmetrize
	if(scene->isPeriodic){ volume=scene->cell->getVolume(); }
	else if(volume<=0) yade::ValueError("stressStiffness: positive volume value must be given for aperiodic simulations.");
	stress/=volume; K/=volume;
	// prune numerical noise from the stiffness matrix
	Real maxElem=K.maxCoeff();
	for(int p=0; p<6; p++) for(int q=0; q<6; q++) if(K(p,q)<1e-12*maxElem) K(p,q)=0;
	//
	return py::make_tuple(tensor_toVoigt(stress),K);
}


BOOST_PYTHON_MODULE(_utils2){
	// http://numpy.scipy.org/numpydoc/numpy-13.html mentions this must be done in module init, otherwise we will crash
	//import_array();

	YADE_SET_DOCSTRING_OPTS;
	py::def("pWaveTimeStep",pWaveTimeStep,"Get timestep accoring to the velocity of P-Wave propagation; computed from sphere radii, rigidities and masses.");
	py::def("createContacts",createContacts,(py::arg("ids1"),py::arg("id2s"),py::arg("geomFunctors")=vector<shared_ptr<CGeomFunctor> >(),py::arg("physFunctors")=vector<shared_ptr<CPhysFunctor> >(),py::arg("force")=true),"Create contacts between given DEM particles.\n\nCurrent engines are searched for :yref:`ContactLoop`, unless *geomFunctors* and *physFunctors* are given. *force* will make :yref:`CGeomFunctors` acknowledge the contact even if particles don't touch geometrically.\n\n.. warning::\n\tThis function will very likely behave incorrectly for periodic simulations (though it could be extended it to handle it farily easily).");
	py::def("stressStiffness",stressStiffness,(py::arg("volume")=0,py::arg("skipMultinodal")=true),"Compute stress and stiffness tensors; returns tuple (stress, stiffness) in Voigt notation. *skipMultinodal* skips all contacts involving particles with multiple nodes, where stress & stiffness values can be determined only by In2 functors.");
}


