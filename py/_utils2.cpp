#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/pkg/dem/Sphere.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/pkg/dem/L6Geom.hpp>

#include<yade/pkg/dem/Funcs.hpp>

#include<yade/lib/base/CompUtils.hpp>

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

Real pWaveDt(shared_ptr<Scene> _scene=shared_ptr<Scene>()){
	Scene* scene=(_scene?_scene.get():Omega::instance().getScene().get());
	DemField* field=getDemField(scene).get();
	Real dt=std::numeric_limits<Real>::infinity();
	FOREACH(const shared_ptr<Particle>& b, *field->particles){
		if(!b || !b->material || !b->shape || b->shape->nodes.size()!=1 || !b->shape->nodes[0]->hasData<DemData>()) continue;
		const shared_ptr<ElastMat>& elMat=dynamic_pointer_cast<ElastMat>(b->material);
		const shared_ptr<Sphere>& s=dynamic_pointer_cast<Sphere>(b->shape);
		if(!elMat || !s) continue;
		dt=min(dt,s->radius/sqrt(elMat->young/elMat->density));
	}
	return dt;
}

Real pWaveTimeStep(){
	cerr<<"DeprecationWarning: utils.pWaveTimeStep is deprecated, use use utils.pWaveDt instead."<<endl;
	return pWaveDt();
}

py::object boxPsd(const AlignedBox3r& box, bool mass, int num, int mask, Vector2r dRange, bool zip){
	Scene* scene=Omega::instance().getScene().get(); DemField* field=getDemField(scene).get();
	vector<Vector2r> psd0=DemFuncs::boxPsd(scene,field,box,mass,num,mask,dRange);
	if(zip){
		py::list ret;
		for(const auto& dp: psd0) ret.append(py::make_tuple(dp[0],dp[1]));
		return ret;
	} else {
		py::list diameters,percentage;
		for(const auto& dp: psd0){ diameters.append(dp[0]); percentage.append(dp[1]); }
		return py::make_tuple(diameters,percentage);
	}
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
		const shared_ptr<Particle>& b1=dem->particles->safeGet(id1); const shared_ptr<Particle>& b2=dem->particles->safeGet(id2);
		if(b1->contacts.find(id2)!=b1->contacts.end()) yade::ValueError("Contact ##"+lexical_cast<string>(id1)+"+"+lexical_cast<string>(id2)+" already exists.");
		shared_ptr<Contact> C=gDisp->explicitAction(scene,b1,b2,/*force*/force);
		if(force && !C) throw std::logic_error("CGeomFunctor did not create contact, although force==true");
		if(!C) continue;
		pDisp->explicitAction(scene,b1->material,b2->material,C);
		C->stepMadeReal=scene->step;
		dem->contacts->add(C);
		ret.push_back(C);
	}
	return ret;
}


Real unbalancedForce(bool useMaxForce=false){
	Scene* scene=Omega::instance().getScene().get(); DemField* field=getDemField(scene).get();
	return DemFuncs::unbalancedForce(scene,field,useMaxForce);
}




py::tuple stressStiffnessWork(Real volume=0, bool skipMultinodal=false, const Vector6r& prevStress=(Vector6r()<<NaN,NaN,NaN,NaN,NaN,NaN).finished()){
	Matrix6r K(Matrix6r::Zero());
	Matrix3r stress(Matrix3r::Zero());
	Scene* scene=Omega::instance().getScene().get(); DemField* dem=getDemField(scene).get();
	
	std::tie(stress,K)=DemFuncs::stressStiffness(scene,dem,skipMultinodal,volume);

	// prune numerical noise from the stiffness matrix
	Real maxElem=K.maxCoeff();
	for(int p=0; p<6; p++) for(int q=0; q<6; q++) if(K(p,q)<1e-12*maxElem) K(p,q)=0;
	// compute velGrad work in periodic simulations
	Real work=NaN;
	if(scene->isPeriodic){
		Matrix3r midStress=!isnan(prevStress[0])?.5*(voigt_toSymmTensor(prevStress)+stress):stress;
		Real midVolume=(scene->cell->hSize-.5*scene->dt*scene->cell->gradV).determinant();
		work=-(scene->cell->gradV*midStress).trace()*scene->dt*midVolume;
	}
	return py::make_tuple(tensor_toVoigt(stress),K,work);
}

Real muStiffnessScaling(Real piHat=Mathr::PI/2, bool skipFloaters=false, Real V=-1){
	Scene* scene=Omega::instance().getScene().get(); const auto& dem=getDemField(scene);
	if(V<=0){
		if(scene->isPeriodic) V=scene->cell->getVolume();
		else yade::RuntimeError("Positive value of volume (V) must be givne for aperiodic simulations.");
	}
	int N=0;
	Real Rr2=0; // r'*r^2
	FOREACH(const shared_ptr<Contact>& c, *dem->contacts){
		if(!(dynamic_pointer_cast<Sphere>(c->pA->shape) && dynamic_pointer_cast<Sphere>(c->pB->shape))) continue;
		if(skipFloaters && (
			count_if(c->pA->contacts.begin(),c->pA->contacts.end(),[](const Particle::MapParticleContact::value_type& v){return v.second->isReal();})<=1 || /* smaller than one would be a grave bug*/
			count_if(c->pB->contacts.begin(),c->pB->contacts.end(),[](const Particle::MapParticleContact::value_type& v){return v.second->isReal();})<=1
		)) continue;
		Real rr=.5*c->dPos(scene).norm(); // handles PBC
		Real r=min(c->pA->shape->cast<Sphere>().radius,c->pB->shape->cast<Sphere>().radius);
		Rr2+=rr*r*r;
		N++;
	}
	Rr2/=N; // averages
	return (2/3.)*(N*piHat/V)*Rr2;
}

/* Liao1997: Stress-strain relationship for granular materials based on the hypothesis of the best fit */
Matrix6r bestFitCompliance(){
	Scene* scene=Omega::instance().getScene().get(); const auto& dem=getDemField(scene);
	if(!scene->isPeriodic) yade::RuntimeError("Only implemented fro periodic simulations.");
	Real V=scene->cell->getVolume();
	// stuff data in here first
	struct ContData{ Real hn,hs; Vector3r l,n,s,t; };
	vector<ContData> CC;
	FOREACH(const shared_ptr<Contact>& c, *dem->contacts){
		if(!(dynamic_pointer_cast<Sphere>(c->pA->shape) && dynamic_pointer_cast<Sphere>(c->pB->shape))) continue;
		const Matrix3r& trsf=c->geom->cast<L6Geom>().trsf;
		ContData cd={1/c->phys->cast<FrictPhys>().kn,1/c->phys->cast<FrictPhys>().kt,c->dPos(scene),trsf.row(0),trsf.row(1),trsf.row(2)};
		CC.push_back(cd);
	}
	Matrix3r AA(Matrix3r::Zero());
	FOREACH(const ContData& c, CC){ for(int j:{0,1,2}) for(int n:{0,1,2}) AA(j,n)+=c.l[j]*c.l[n]; }
	AA*=1/V;
	AA=(Matrix3r::Ones().array()/AA.array()).matrix(); // componentwise reciprocal
	Matrix6r H(Matrix6r::Zero());
	FOREACH(const ContData& c, CC){
		for(int p=0; p<6; p++) for(int q=p;q<6;q++){
			int i=voigtMap[p][q][0], j=voigtMap[p][q][1], k=voigtMap[p][q][2], l=voigtMap[p][q][3];
			const Vector3r& n(c.n); const Vector3r& s(c.s);	const Vector3r& t(c.t);
			Real hh=c.hn*n[i]*n[k]+c.hs*(s[i]*s[k]+t[i]*t[k]);
			Real lnAjn=0, lqAlq=0;
			for(int N:{0,1,2}) lnAjn+=c.l[N]*AA(j,N);
			for(int Q:{0,1,2}) lqAlq+=c.l[Q]*AA(l,Q);
			H(p,q)=hh*lnAjn*lqAlq;
			if(q!=p) H(q,p)=H(p,q);
		}
	}
	H*=1/V;
	return H;
};


BOOST_PYTHON_MODULE(_utils2){
	// http://numpy.scipy.org/numpydoc/numpy-13.html mentions this must be done in module init, otherwise we will crash
	//import_array();

	YADE_SET_DOCSTRING_OPTS;
	py::def("pWaveTimeStep",pWaveTimeStep,"Do not use, remaed to pWaveDt and will be removed.");
	py::def("pWaveDt",pWaveDt,(py::arg("scene")=py::object()),"Get timestep accoring to the velocity of P-Wave propagation; computed from sphere radii, rigidities and masses.");
	py::def("boxPsd",boxPsd,(py::arg("box")=AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),py::arg("num")=20,py::arg("mask")=0,py::arg("dRange")=Vector2r(0.,0.),py::arg("zip")=true),"Compute Particle size distribution in given box (min,max) or in the whole simulation, if box is not specified; list of couples (diameter,passing) is returned; with *unzip*, tuple of two sequences, diameters and passing values, are returned.");
	py::def("createContacts",createContacts,(py::arg("ids1"),py::arg("id2s"),py::arg("geomFunctors")=vector<shared_ptr<CGeomFunctor> >(),py::arg("physFunctors")=vector<shared_ptr<CPhysFunctor> >(),py::arg("force")=true),"Create contacts between given DEM particles.\n\nCurrent engines are searched for :yref:`ContactLoop`, unless *geomFunctors* and *physFunctors* are given. *force* will make :yref:`CGeomFunctors` acknowledge the contact even if particles don't touch geometrically.\n\n.. warning::\n\tThis function will very likely behave incorrectly for periodic simulations (though it could be extended it to handle it farily easily).");
	py::def("stressStiffnessWork",stressStiffnessWork,(py::arg("volume")=0,py::arg("skipMultinodal")=true,py::arg("prevStress")=(Vector6r()<<NaN,NaN,NaN,NaN,NaN,NaN).finished()),"Compute stress and stiffness tensors, and work increment of current velocity gradient (*nan* for aperiodic simulations); returns tuple (stress, stiffness, work), where stress and stiffness are in Voigt notation. *skipMultinodal* skips all contacts involving particles with multiple nodes, where stress & stiffness values can be determined only by In2 functors.");
	py::def("muStiffnessScaling",muStiffnessScaling,(py::arg("piHat")=Mathr::PI/2,py::arg("skipFloaters")=false,py::arg("V")=-1),"Compute stiffness scaling parameter relating continuum-like stiffness with packing stiffness; see 'Particle assembly with cross-anisotropic stiffness tensor' for details. With *skipFloaters*, ignore contacts where any of the two contacting particlds has only one *real* contact (thus not contributing to the assembly stability).");
	py::def("bestFitCompliance",bestFitCompliance,"Compute compliance based on best-fit hypothesis, using the paper [Liao1997], equations (30) and (28,31).");
	py::def("mapColor",CompUtils::scalarOnColorScale,(py::arg("x"),py::arg("min")=0,py::arg("max")=1,py::arg("cmap")=-1),"Map scalar to color (as 3-tuple). See O.cmap, O.cmaps to set colormap globally.");
	py::def("unbalancedForce",unbalancedForce,(py::arg("useMaxForce")=false),"Compute the ratio of mean (or maximum, if *useMaxForce*) summary force on bodies and mean force magnitude on interactions. It is an adimensional measure of staticity, which approaches zero for quasi-static states.");
}


