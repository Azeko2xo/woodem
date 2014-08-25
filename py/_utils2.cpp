#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/ContactLoop.hpp>
#include<woo/pkg/dem/L6Geom.hpp>

#include<woo/pkg/dem/Funcs.hpp>

#include<woo/lib/base/CompUtils.hpp>

Real pWaveDt(shared_ptr<Scene> _scene=shared_ptr<Scene>(), bool noClumps=false){
	Scene* scene=(_scene?_scene.get():Master::instance().getScene().get());
	return DemFuncs::pWaveDt(DemFuncs::getDemField(scene),noClumps);
}

py::tuple psd(vector<Vector2r> ddmm, bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) {
	vector<Vector2r> psd=DemFuncs::psd(ddmm,/*cumulative*/cumulative,/*normalize*/normalize,num,dRange,
		/*radius getter*/[](const Vector2r& diamMass) ->Real { return diamMass[0]; },
		/*weight getter*/[&](const Vector2r& diamMass) -> Real{ return mass?diamMass[1]:1.; }
	);
	py::list diameters,percentage;
	for(const auto& dp: psd){ diameters.append(dp[0]); percentage.append(dp[1]); }
	return py::make_tuple(diameters,percentage);
}


py::object boxPsd(const AlignedBox3r& box, bool mass, int num, int mask, Vector2r dRange, bool zip){
	Scene* scene=Master::instance().getScene().get(); DemField* field=DemFuncs::getDemField(scene).get();
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
	if(ids1.size()!=ids2.size()) woo::ValueError("id1 and id2 arguments must have same length.");
	if(cgff.size()+cpff.size()>0 && cgff.size()*cpff.size()==0) woo::ValueError("Either both CGeomFunctors and CPhysFunctors must be specified, or neither of them (now: "+lexical_cast<string>(cgff.size())+", "+lexical_cast<string>(cpff.size())+")");
	Scene* scene=Master::instance().getScene().get(); shared_ptr<DemField> dem=DemFuncs::getDemField(scene);
	shared_ptr<CGeomDispatcher> gDisp; shared_ptr<CPhysDispatcher> pDisp;
	if(cgff.empty()){ // find dispatchers in current engines
		for(const shared_ptr<Engine>& e: scene->engines){
			ContactLoop* cl(dynamic_cast<ContactLoop*>(e.get()));
			if(!cl) continue;
			gDisp=cl->geoDisp; pDisp=cl->phyDisp;
			break;
		}
		if(!gDisp) woo::RuntimeError("No ContactLoop in Scene.engines; supply CGeomFunctors and CPhysFunctors explicitly.");
	} else { // create dispatcher objects from supplied functors
		gDisp=shared_ptr<CGeomDispatcher>(new CGeomDispatcher);
		pDisp=shared_ptr<CPhysDispatcher>(new CPhysDispatcher);
		for(const shared_ptr<CGeomFunctor>& cgf: cgff) gDisp->add(cgf);
		for(const shared_ptr<CPhysFunctor>& cpf: cpff) pDisp->add(cpf);
	}
	assert(gDisp && pDisp);
	gDisp->scene=pDisp->scene=scene; gDisp->field=pDisp->field=dem; gDisp->updateScenePtr(); pDisp->updateScenePtr();
	vector<shared_ptr<Contact> > ret;
	for(int k=0; k<(int)ids1.size(); k++){
		Particle::id_t id1=ids1[k], id2=ids2[k];
		const shared_ptr<Particle>& b1=dem->particles->safeGet(id1); const shared_ptr<Particle>& b2=dem->particles->safeGet(id2);
		if(b1->contacts.find(id2)!=b1->contacts.end()) woo::ValueError("Contact ##"+lexical_cast<string>(id1)+"+"+lexical_cast<string>(id2)+" already exists.");
		shared_ptr<Contact> C=gDisp->explicitAction(scene,b1,b2,/*force*/force);
		if(force && !C) throw std::logic_error("CGeomFunctor did not create contact, although force==true");
		if(!C) continue;
		pDisp->explicitAction(scene,b1->material,b2->material,C);
		C->stepCreated=scene->step;
		dem->contacts->add(C);
		ret.push_back(C);
	}
	return ret;
}


Real unbalancedForce(const shared_ptr<Scene>& _scene, bool useMaxForce=false){
	Scene* scene=(_scene?_scene:Master::instance().getScene()).get(); DemField* field=DemFuncs::getDemField(scene).get();
	return DemFuncs::unbalancedForce(scene,field,useMaxForce);
}




py::tuple stressStiffnessWork(Real volume=0, bool skipMultinodal=false, const Vector6r& prevStress=(Vector6r()<<NaN,NaN,NaN,NaN,NaN,NaN).finished()){
	Matrix6r K(Matrix6r::Zero());
	Matrix3r stress(Matrix3r::Zero());
	Scene* scene=Master::instance().getScene().get(); DemField* dem=DemFuncs::getDemField(scene).get();
	
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

Real muStiffnessScaling(Real piHat=M_PI, bool skipFloaters=false, Real V=-1){
	Scene* scene=Master::instance().getScene().get(); const auto& dem=DemFuncs::getDemField(scene);
	if(V<=0){
		if(scene->isPeriodic) V=scene->cell->getVolume();
		else woo::RuntimeError("Positive value of volume (V) must be given for aperiodic simulations.");
	}
	int N=0;
	Real Rr2=0; // r'*r^2
	FOREACH(const shared_ptr<Contact>& c, *dem->contacts){
		const Particle *pA=c->leakPA(), *pB=c->leakPB();
		if(!(dynamic_pointer_cast<Sphere>(pA->shape) && dynamic_pointer_cast<Sphere>(pB->shape))) continue;
		if(skipFloaters && (
			count_if(pA->contacts.begin(),pA->contacts.end(),[](const Particle::MapParticleContact::value_type& v){return v.second->isReal();})<=1 || /* smaller than one would be a grave bug*/
			count_if(pB->contacts.begin(),pB->contacts.end(),[](const Particle::MapParticleContact::value_type& v){return v.second->isReal();})<=1
		)) continue;
		Real rr=.5*c->dPos(scene).norm(); // handles PBC
		Real r=min(pA->shape->cast<Sphere>().radius,pB->shape->cast<Sphere>().radius);
		Rr2+=rr*r*r;
		N++;
	}
	Rr2/=N; // averages
	return (1/3.)*(N*piHat/V)*Rr2;
}

/* Liao1997: Stress-strain relationship for granular materials based on the hypothesis of the best fit */
Matrix6r bestFitCompliance(){
	Scene* scene=Master::instance().getScene().get(); const auto& dem=DemFuncs::getDemField(scene);
	if(!scene->isPeriodic) woo::RuntimeError("Only implemented fro periodic simulations.");
	Real V=scene->cell->getVolume();
	// stuff data in here first
	struct ContData{ Real hn,hs; Vector3r l,n,s,t; };
	vector<ContData> CC;
	FOREACH(const shared_ptr<Contact>& c, *dem->contacts){
		const Particle *pA=c->leakPA(), *pB=c->leakPB();
		if(!(dynamic_pointer_cast<Sphere>(pA->shape) && dynamic_pointer_cast<Sphere>(pB->shape))) continue;
		const Matrix3r& trsf=c->geom->cast<L6Geom>().trsf;
		ContData cd={1/c->phys->cast<FrictPhys>().kn,1/c->phys->cast<FrictPhys>().kt,c->dPos(scene),trsf.col(0),trsf.col(1),trsf.col(2)};
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

vector<Vector3r> facetsPlaneIntersectionSegments(vector<shared_ptr<Particle>> facets, Vector3r pt, Vector3r normal){
	vector<Vector3r> ret;
	for(const auto& p: facets){
		if(!p->shape || p->shape->nodes.size()!=3) woo::TypeError("Particle.shape must have 3 nodes (Facet), #"+to_string(p->id));
		assert(p->shape->nodes.size()==3);
		for(int i:{0,1,2}){
			const Vector3r& A(p->shape->nodes[i]->pos), B(p->shape->nodes[(i+1)%3]->pos);
			Real u=CompUtils::segmentPlaneIntersection(A,B,pt,normal);
			if(u>0 && u<1) ret.push_back(A+u*(B-A));
		}
	}
	return ret;
};

Real outerTri2Dist(const Vector2r& pt, const Vector2r& A, const Vector2r& B, const Vector2r& C){
	Vector3r abc[]={Vector3r(A[0],A[1],0),Vector3r(B[0],B[1],0),Vector3r(C[0],C[1],0)};
	Vector3r P=Vector3r(pt[0],pt[1],0);
	Real minDist=Inf;
	bool inside=true;
	for(int i:{0,1,2}){
		Vector3r& M(abc[i]), N(abc[(i+1)%3]);
		Vector3r Q=CompUtils::closestSegmentPt(P,M,N);
		Real dist=(P-Q).norm();
		inside=inside && ((N-M).cross(P-M))[2]>0;
		if(minDist>dist) minDist=dist;
	};
	return minDist*(inside?-1:1);
}

py::tuple reactionInPoint(const shared_ptr<DemField>& dem, int mask, const Vector3r& pt, bool multinodal){
	Vector3r F,T;
	size_t n=DemFuncs::reactionInPoint(dem->scene,dem.get(),mask,pt,multinodal,F,T);
	return py::make_tuple(F,T,n);
}

py::tuple particleStress(const shared_ptr<Particle>& p){
	Vector3r sigN, sigT;
	bool ok=DemFuncs::particleStress(p,sigN,sigT);
	if(!ok) throw std::runtime_error("Failed to compute stress (is this particle a sphere?).");
	return py::make_tuple(sigN,sigT);
}

#if 0
py::tuple radialAxialForce(const shared_ptr<DemField>& dem, int mask, const Vector3r& axis, bool shear){
	Vector3r F,T;
	size_t n=DemFuncs::radialAxialForce(dem->scene,dem.get(),mask,axis,shear);
	return py::make_tuple(F,T,n);
}
#endif

WOO_PYTHON_MODULE(_utils2);
BOOST_PYTHON_MODULE(_utils2){
	// http://numpy.scipy.org/numpydoc/numpy-13.html mentions this must be done in module init, otherwise we will crash
	//import_array();

	WOO_SET_DOCSTRING_OPTS;
	py::def("pWaveDt",pWaveDt,(py::arg("scene")=py::object(),py::arg("noClumps")=false),"Get timestep accoring to the velocity of P-Wave propagation; computed from sphere radii, rigidities and masses.");
	py::def("reactionInPoint",reactionInPoint,(py::arg("dem"),py::arg("mask"),py::arg("pt"),py::arg("multinodal")=true),"Compute force and torque in point *pt* generated by forces on particles matching *mask* in :obj:`DemField` *dem*. Multinodal particles (such as :obj:`facets <Facet>`) usually don't nodal forces evaluated; their contacts are traversed and contact reaction computed directly if *multinodal* is ``True`` (in that case, multinodal-multinodal contacts should not be present at all). Return tuple ``(force,torque,N)``, where ``N`` is the number of particles matched by *mask*.");
	py::def("particleStress",particleStress,"Return tuple of normal and shear stresses acting on given particle; raise exception for non-spherical particles");
	py::def("boxPsd",boxPsd,(py::arg("box")=AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)),py::arg("mass")=true,py::arg("num")=20,py::arg("mask")=0,py::arg("dRange")=Vector2r(0.,0.),py::arg("zip")=true),"Compute Particle size distribution in given box (min,max) or in the whole simulation, if box is not specified; list of couples (diameter,passing) is returned; with *unzip*, tuple of two sequences, diameters and passing values, are returned.");
	py::def("psd",psd,(py::arg("ddmm"),py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("num")=80),"Return PSD of given particles (given as array of Vector2, where each Vector2 specifies diameter and mass) as arrays of diameters and passing; the semantics is the same as for :obj:`woo.dem.BoxDeleter.psd` and :obj:`woo.dem.ParticleGenerator.psd`.");
	py::def("createContacts",createContacts,(py::arg("ids1"),py::arg("id2s"),py::arg("geomFunctors")=vector<shared_ptr<CGeomFunctor> >(),py::arg("physFunctors")=vector<shared_ptr<CPhysFunctor> >(),py::arg("force")=true),"Create contacts between given DEM particles.\n\nCurrent engines are searched for :obj:`woo.dem.ContactLoop`, unless *geomFunctors* and *physFunctors* are given. *force* will make :obj:`woo.dem.CGeomFunctor` acknowledge the contact even if particles don't touch geometrically.\n\n.. warning::\n\tThis function will very likely behave incorrectly for periodic simulations (though it could be extended it to handle it farily easily).");
	py::def("stressStiffnessWork",stressStiffnessWork,(py::arg("volume")=0,py::arg("skipMultinodal")=true,py::arg("prevStress")=(Vector6r()<<NaN,NaN,NaN,NaN,NaN,NaN).finished()),"Compute stress and stiffness tensors, and work increment of current velocity gradient (*nan* for aperiodic simulations); returns tuple (stress, stiffness, work), where stress and stiffness are in Voigt notation. *skipMultinodal* skips all contacts involving particles with multiple nodes, where stress & stiffness values can be determined only by In2 functors.");
	py::def("muStiffnessScaling",muStiffnessScaling,(py::arg("piHat")=M_PI/2,py::arg("skipFloaters")=false,py::arg("V")=-1),"Compute stiffness scaling parameter relating continuum-like stiffness with packing stiffness; see 'Particle assembly with cross-anisotropic stiffness tensor' for details. With *skipFloaters*, ignore contacts where any of the two contacting particlds has only one *real* contact (thus not contributing to the assembly stability).");
	py::def("bestFitCompliance",bestFitCompliance,"Compute compliance based on best-fit hypothesis, using the paper [Liao1997], equations (30) and (28,31).");
	py::def("mapColor",CompUtils::scalarOnColorScale,(py::arg("x"),py::arg("min")=0,py::arg("max")=1,py::arg("cmap")=-1,py::arg("reversed")=false),"Map scalar to color (as 3-tuple). See :obj:`woo.core.Master.cmap`, :obj:`woo.core.Master.cmaps` to set colormap globally.");
	py::def("unbalancedForce",unbalancedForce,(py::arg("scene")=shared_ptr<Scene>(),py::arg("useMaxForce")=false),"Compute the ratio of mean (or maximum, if *useMaxForce*) summary force on bodies and mean force magnitude on interactions. It is an adimensional measure of staticity, which approaches zero for quasi-static states.");
	py::def("facetsPlaneIntersectionSegments",facetsPlaneIntersectionSegments,(py::arg("facets"),py::arg("pt"),py::arg("normal")),"Return list of points, where consecutive pairs are segment where *facets* were intersecting plane given by *pt* and *normal*.");
	py::def("outerTri2Dist",outerTri2Dist,(py::arg("pt"),py::arg("A"),py::arg("B"),py::arg("C")),"Return signed distance of point *pt* in triangle A,B,C. The result is distance of point *pt* to the closest point on triangle A,B,C. The distance is negative is *pt* is inside, 0 if exactly on the triangle and positive outside. Signedness supposes that A,B,C are given anti-clockwise; otherwise, the sign will be reversed");
	py::def("importSTL",DemFuncs::importSTL,(py::arg("filename"),py::arg("mat"),py::arg("mask")=(int)DemField::defaultBoundaryMask,py::arg("color")=-.999,py::arg("scale")=1.,py::arg("shift")=Vector3r::Zero().eval(),py::arg("ori")=Quaternionr::Identity(),py::arg("threshold")=-1e-6,py::arg("maxBox")=NaN,py::arg("readColors")=true,py::arg("flex")=false),"Return list of :obj:`particles <woo.dem.Particle>` (with the :obj:`Facet <woo.dem.Facet>` :obj:`shape <woo.dem.Particle.shape>`, or :obj:`~woo.fem.Membrane` if *flex* is ``True``) imported from given STL file.\nBoth ASCII and binary formats are supported; ``COLOR`` and ``MATERIAL`` values in the binary format, if given and with *readColors*, are read but currently ignored (they should translate to the :obj:`woo.dem.Shape.color` scalar − that color difference would be preserved, but not the color as such), and a warning is issued.\n*scale*, *shift* and *ori* are applied in this order before :obj:`nodal <woo.core.Node>` coordinates are computed. The *threshold* value serves for merging incident vertices: if positive, it is distance in the STL space (before scaling); if negative, it is relative to the max bounding box size of the entire mesh. *maxBox*, if positive, will cause any faces to be tesselated until the smallest dimension of its bbox (in simulation space) is smaller than *maxBox*; this is to avoid many spurious (potential) contacts with large obliquely-oriented faces.");
	py::def("contactCoordQuantiles",DemFuncs::contactCoordQuantiles,(py::arg("dem"),py::arg("quantiles"),py::arg("node"),py::arg("box")),"Return list of local contact z-coordinates for given quantile values; *box* is specified in node-local coordinates (but may be infinite to include all contacts).");
	py::def("coordNumber",DemFuncs::coordNumber,(py::arg("dem"),py::arg("node")=shared_ptr<Node>(),py::arg("box")=AlignedBox3r(Vector3r(-Inf,-Inf,-Inf),Vector3r(Inf,Inf,Inf)),py::arg("mask")=0,py::arg("skipFree")=true),"Return coordination number of the sampled area given by optional local coordinates (*node*) and *box* (which is in local coordinates if *node* is given and global if not); if *mask* is non-zero, particles without matching :obj:`mask <woo.dem.Particle.mask` are ignored. See `yade.utils.avgNumInterations <https://yade-dem.org/doc/yade.utils.html#yade.utils.avgNumInteractions>`__ for temporary doc.\n.. warn:: Clumps are not handled properly by this function and an exception will be raised if they are encountered.");
	py::def("porosity",DemFuncs::porosity,(py::arg("dem"),py::arg("node")=shared_ptr<Node>(),py::arg("box")),"Return `porosity <http://en.wikipedia.org/wiki/Porosity>`__ for given box (in local coordinates, if *node* is not ``None``), considering (only) spherical particles of which centroid is in the box.");
}


