#ifdef YADE_VTK
#include<yade/pkg/sparc/SparcField.hpp>

#include<boost/preprocessor.hpp>
#include<boost/filesystem/convenience.hpp>

namespace bfs=boost::filesystem;


using boost::format;

YADE_PLUGIN(sparc,(SparcField)(SparcData)(ExplicitNodeIntegrator)(StaticEquilibriumSolver));

void SparcField::constructLocator(){
	locator=vtkPointLocator::New(); points=vtkPoints::New(); grid=vtkUnstructuredGrid::New(); grid->SetPoints(points); locator->SetDataSet(grid);
}

template<bool useNext>
void SparcField::updateLocator(){
	// FIXME: just for debugging, leaks memory by discarding old objects
	// however, it fixes errors when called with point positions being only updated
	constructLocator();
	assert(locator && points && grid);
	/* adjust points size */
	if(points->GetNumberOfPoints()!=(int)nodes.size()) points->SetNumberOfPoints(nodes.size());
	//FOREACH(const shared_ptr<Node>& n, nodes){
	size_t sz=nodes.size();
	for(size_t i=0; i<sz; i++){
		const shared_ptr<Node>& n(nodes[i]);
		assert(n->hasData<SparcData>());
		if(!useNext) points->SetPoint(i,n->pos[0],n->pos[1],n->pos[2]);
		else{ Vector3r pos=n->pos+n->getData<SparcData>().v*scene->dt; points->SetPoint(i,pos[0],pos[1],pos[2]); }
	}
	// points->ComputeBounds(); points->GetBounds(bb)
	locator->BuildLocator();
	locDirty=false;
};

template<typename M> bool eig_isnan(const M& m){for(int j=0; j<m.cols(); j++)for(int i=0;i<m.rows(); i++) if(isnan(m(i,j))) return true; return false; }
template<typename M> bool eig_all_isnan(const M&m){for(int j=0; j<m.cols(); j++)for(int i=0;i<m.rows();i++) if(!isnan(m(i,j))) return false; return true; }
template<> bool eig_isnan<Vector3r>(const Vector3r& v){for(int i=0; i<v.size(); i++) if(isnan(v[i])) return true; return false; }
template<> bool eig_isnan(const Real& r){ return isnan(r); }

vector<shared_ptr<Node> > SparcField::nodesAround(const Vector3r& pt, int count, Real radius, const shared_ptr<Node>& self){
	if(!locator) throw runtime_error("SparcField::locator not initialized!");	
	if((radius<=0 && count<=0) || (radius>0 && count>0)) throw std::invalid_argument("SparcField.nodesAround: exactly one of radius or count must be positive!");
	vtkIdList* _ids=vtkIdList::New(); vtkIdList& ids(*_ids);;
	if(count>0 && self) count+=1; // find self in the middle
	if(count<=0){ locator->FindPointsWithinRadius(radius,(const double*)(&pt),&ids); }
	else{ locator->FindClosestNPoints(count,(const double*)(&pt),&ids); }
	int numIds=ids.GetNumberOfIds();
	vector<shared_ptr<Node > > ret; ret.resize(numIds);
	int j=0;
	for(int i=0; i<numIds; i++){
		if(self && self==nodes[ids.GetId(i)]){ ret.resize(numIds-1); continue; }
		ret[j++]=nodes[ids.GetId(i)];
	};
	if(self && j>numIds-1){
		cerr<<"Neighbors found: "<<endl; for(int i=0; i<numIds; i++) cerr<<"\t"<<ids.GetId(i)<<": "<<nodes[ids.GetId(i)]->pos.transpose()<<" (@ "<<nodes[i].get()<<")"<<endl;
		throw std::runtime_error(("SparcField::nodesAround asked to remove (central?) point form the result, but it was not there (me @"+lexical_cast<string>(self.get())+" pos="+lexical_cast<string>(self->pos.transpose())+", searching around "+lexical_cast<string>(pt.transpose())+")").c_str()); }
	_ids->Delete();
	return ret;
};

template<bool useNext>
void ExplicitNodeIntegrator::findNeighbors(const shared_ptr<Node>&n) const {
	SparcData& dta(n->getData<SparcData>());
	size_t prevN=dta.neighbors.size();
	Vector3r pos(!useNext?n->pos:n->pos+dta.v*scene->dt);
	dta.neighbors=mff->nodesAround(pos,/*count*/-1,rSearch,/*self=*/n);

	// say if number of neighbors changes between steps
	if(!useNext && prevN>0 && dta.neighbors.size()!=prevN){
		cerr<<"Nid "<<dta.nid<<" changed number of neighbors: "<<prevN<<" → "<<dta.neighbors.size()<<endl;
	}
};

Real ExplicitNodeIntegrator::pointWeight(Real distSq) const {
	if(weightFunc==WEIGHT_DIST){
		Real w=(rPow%2==0?pow(distSq,rPow/2):pow(sqrt(distSq),rPow));
		assert(!(rPow==0 && w!=1.)); // rPow==0 → weight==1.
		return w;
	}
	else if(weightFunc==WEIGHT_GAUSS){
		Real w=pow(sqrt(distSq),-1); // make all points the same weight first
		Real hSq=pow(rSearch,2);
		assert(distSq<=hSq); // taken care of by neighbor search algorithm already
		w*=exp(-gaussAlpha*(distSq/hSq));
		return w;
	}
	throw std::logic_error("ExplicitNodeIntegrator.weightFunc has inadmissible value; this should have been trapped in postLoad already!");
}

template<bool useNext>
void ExplicitNodeIntegrator::updateNeighborsRelPos(const shared_ptr<Node>& n) const {
	SparcData& dta(n->getData<SparcData>());
	// current or next inputs
	const vector<shared_ptr<Node>>& neighbors(useNext?dta.nextNeighbors:dta.neighbors);
	VectorXr& weights(useNext?dta.nextWeights:dta.weights);
	MatrixXr& relPosInv(useNext?dta.nextRelPosInv:dta.relPosInv);

	int sz(neighbors.size());
	if(sz<3) throw std::runtime_error((format("Node #%d at %s has only %d neighbors (3 is minimum)")%dta.nid%n->pos.transpose()%sz).str());
	MatrixXr relPos(sz,3);
	weights=VectorXr(sz);
	Vector3r midPos(n->pos+(useNext?Vector3r(scene->dt*dta.v):Vector3r::Zero()));
	for(int i=0; i<sz; i++){
		Vector3r dX=VectorXr(neighbors[i]->pos+(useNext?Vector3r(scene->dt*neighbors[i]->getData<SparcData>().v):Vector3r::Zero())-midPos);
		weights[i]=pointWeight(dX.squaredNorm());
		relPos.row(i)=weights[i]*dX;
	}
	#ifdef SPARC_INSPECT
		if(useNext) dta.relPos=relPos;
		else dta.nextRelPos=relPos;
	#endif
	if(eig_isnan(relPos)){
		cerr<<"=== Relative positions for nid "<<dta.nid<<":\n"<<relPos<<endl;
		throw std::runtime_error("NaN's in relative positions in O.sparc.nodes["+lexical_cast<string>(dta.nid)+"].sparc."+string(useNext?"nextRelPos":"relPos")+") .");
	}
	bool succ=MatrixXr_pseudoInverse(relPos,/*result*/relPosInv);
	if(!succ) throw std::runtime_error("ExplicitNodeIntegrator::updateNeigbours: pseudoInverse failed.");
}

template<bool useNext>
Vector3r ExplicitNodeIntegrator::computeDivT(const shared_ptr<Node>& n) const {
	int vIx[][2]={{0,0},{1,1},{2,2},{1,2},{2,0},{0,1}}; // from voigt index to matrix indices
	const SparcData& midDta=n->getData<SparcData>();
	const vector<shared_ptr<Node> >& NN(useNext?midDta.nextNeighbors:midDta.neighbors);
	const VectorXr& weights(useNext?midDta.nextWeights:midDta.weights);
	const MatrixXr& relPosInv(useNext?midDta.nextRelPosInv:midDta.relPosInv);
	size_t sz=NN.size();
	Vector3r A[6];
	for(int l:{0,1,2,3,4,5}){
		int i=vIx[l][0], j=vIx[l][1]; // matrix indices from voigt indices
		VectorXr rhs(sz);
		for(size_t r=0; r<sz; r++){
			const SparcData& nDta=NN[r]->getData<SparcData>();
			if(useNext) rhs[r]=nDta.nextT(i,j)-midDta.nextT(i,j);
			else rhs[r]=nDta.T(i,j)-midDta.T(i,j);
			rhs[r]*=weights[r];
		}
		A[l]=relPosInv*rhs;
		if(eig_isnan(A[l])) yade::ValueError(format("Node %d at %s has NaNs in A[%i]=%s")% midDta.nid % n->pos.transpose() % l % A[l].transpose());
	};
	#ifdef SPARC_INSPECT
		/* non-const ref*/ SparcData& midDta2=n->getData<SparcData>();
		midDta2.gradT=MatrixXr(6,3);
		for(int l:{0,1,2,3,4,5}) midDta2.gradT.row(l)=A[l];
	#endif
	int vIx33[3][3]={{0,5,4},{5,1,3},{4,3,2}}; // from matrix indices to voigt
	return Vector3r(
		A[vIx33[0][0]][0]+A[vIx33[0][1]][1]+A[vIx33[0][2]][2],
		A[vIx33[1][0]][0]+A[vIx33[1][1]][1]+A[vIx33[1][2]][2],
		A[vIx33[2][0]][0]+A[vIx33[2][1]][1]+A[vIx33[2][2]][2]
	);
}

Matrix3r ExplicitNodeIntegrator::computeGradV(const shared_ptr<Node>& n) const {
	const SparcData& midDta=n->getData<SparcData>();
	const vector<shared_ptr<Node> >& NN(midDta.neighbors); size_t sz=NN.size();
	MatrixXr relVels(sz,3);
	for(size_t i=0; i<sz; i++){
		relVels.row(i)=VectorXr(NN[i]->getData<SparcData>().v-midDta.v);
		relVels.row(i)*=midDta.weights[i];
	}
	#ifdef SPARC_INSPECT
		n->getData<SparcData>().relVels=relVels;
	#endif
	return midDta.relPosInv*relVels;
}

Matrix3r ExplicitNodeIntegrator::computeStressRate(const Matrix3r& inT, const Matrix3r& D, const Real e /* =-1 */) const {
	#define CC(i) barodesyC[i-1]
	switch(matModel){
		case MAT_HOOKE:
			// isotropic linear elastic
			return voigt_toSymmTensor((C*tensor_toVoigt(D,/*strain*/true)).eval());
		case MAT_BARODESY_JESSE:{
			Matrix3r T(barodesyConvertPaToKPa?(inT/1e3).eval():inT);
			// barodesy from the Theoretical Soil Mechanics course
			// if(e<0 || e>1) throw std::invalid_argument((boost::format("Porosity %g out of 0..1 range (D=%s)\n")%e%D).str());
			Real Tnorm=T.norm(), Dnorm=D.norm();
			if(Dnorm==0 || Tnorm==0) return Matrix3r::Zero();
			Matrix3r D0=D/Dnorm, T0=T/Tnorm;
			Matrix3r R=D0.trace()*Matrix3r::Identity()+CC(1)*(CC(2)*D0).exp(); Matrix3r R0=R/R.norm();
			Real ec=(1+ec0)*exp(pow(Tnorm,1-CC(3))/(CC(4)*(1-CC(3))))-1;
			Real f=CC(4)*D0.trace()+CC(5)*(e-ec)+CC(6);
			Real g=-CC(6);
			Real h=pow(Tnorm,CC(3));
			// cerr<<"CC(2)*D0=\n"<<(CC(2)*D0)<<"\n(CC(2)*D0).exp()=\n"<<(CC(2)*D0).exp()<<endl;
			Matrix3r Tcirc=h*(f*R0+g*T0)*Dnorm;
			return (barodesyConvertPaToKPa?(1e3*Tcirc).eval():Tcirc);
		}
	}
	throw std::logic_error((boost::format("Unknown material model number %d (this should have been caught much earlier!)")%matModel).str());
};

void ExplicitNodeIntegrator::applyKinematicConstraints(const shared_ptr<Node>& n, bool permitFixedDivT) const {
	// apply kinematic constraints (prescribed velocities)
	SparcData& dta(n->getData<SparcData>());
	Vector3r locVel=n->ori*dta.v;
	for(int i=0;i<3;i++){
		if(!isnan(dta.fixedV[i])) locVel[i]=dta.fixedV[i];
	}
	dta.v=n->ori.conjugate()*locVel;
	if(!permitFixedDivT && !eig_all_isnan(dta.fixedT)){
		throw std::invalid_argument((boost::format("Nid %d: fixedT not allowed (is %s)")%dta.nid%dta.fixedT.transpose()).str());
	}
};


void ExplicitNodeIntegrator::postLoad(ExplicitNodeIntegrator&){
	// update stiffness matrix
	C<<(Matrix3r()<<1-nu,nu,nu, nu,1-nu,nu, nu,nu,1-nu).finished(),Matrix3r::Zero(),Matrix3r::Zero(),Matrix3r(((1-2*nu)/2.)*Matrix3r::Identity());
	C*=E/((1+nu)*(1-2*nu));
	if(matModel<0 || matModel>MAT_SENTINEL) yade::ValueError(boost::format("matModel must be in range 0..%d")%(MAT_SENTINEL-1));
	if(weightFunc<0 || weightFunc>=WEIGHT_SENTINEL) yade::ValueError(boost::format("weightFunc must be in range 0..%d")%(WEIGHT_SENTINEL-1));
	if(barodesyC.size()!=6) yade::ValueError(boost::format("barodesyC must have exactly 6 values (%d given)")%barodesyC.size());
	if(rPow>0) LOG_WARN("Positive value of ExplicitNodeIntegrator.rPow: makes weight increasing with distance, ARE YOU NUTS?!");
}

#define _CATCH_CRAP(x,y,z) if(eig_isnan(z)){ throw std::runtime_error((boost::format("NaN in O.sparc.nodes[%d], attribute %s: %s\n")%nid%BOOST_PP_STRINGIZE(z)%z).str().c_str()); }

void SparcData::catchCrap1(int nid, const shared_ptr<Node>& node){
	BOOST_PP_SEQ_FOR_EACH(_CATCH_CRAP,~,(gradV)(nextT)(accel)
		#ifdef SPARC_INSPECT
			(Tcirc)(divT)
		#endif
	);
};

void SparcData::catchCrap2(int nid, const shared_ptr<Node>& node){
	BOOST_PP_SEQ_FOR_EACH(_CATCH_CRAP,~,(node->pos)(T)(v)(rho));
};

py::list SparcData::getGFixedAny(const Vector3r& any, const Quaternionr& ori){
	py::list ret;
	for(int i=0;i<3;i++){
		if(isnan(any[i])) continue;
		Vector3r a=Vector3r::Zero(); a[i]=1;
		ret.append(py::make_tuple(any[i],ori*a));
	}
	return ret;
};

Quaternionr SparcData::getRotQ(const Real& dt) const {
	// reduce rotation by projections on axes with prescribed velocity; this axis will not rotate
	// this should automatically zero rotation vector if 2 or 3 axes are fixed
	Matrix3r W=getW();
	Vector3r rot(W(1,2),W(0,2),W(0,1));
	for(int ax:{0,1,2}) if(!isnan(fixedV[ax])) rot=Vector3r::Unit(ax)*rot.dot(Vector3r::Unit(ax));
	Real angle=rot.norm();
	if(angle>0) return Quaternionr(AngleAxisr(angle*dt,rot/angle));
	return Quaternionr::Identity();
};


void ExplicitNodeIntegrator::run(){
	mff=static_cast<SparcField*>(field.get());
	int nid=-1;
	const Real& dt(scene->dt);
	/*
	update locator
	loop 1: (does not update state vars x,T,v,rho; only stores values needed for update in the next loop)
		0. find neighbors (+compute relPos inverse)
		1. from old positions, compute L(=gradV), D, W, Tcirc, Tdot
		2. from old T, compute Tdot, divT, acceleration
	loop 2: (updates state variables, and does not
		1. update x (from prev x, prev v)
		2. update T (from prev T, prev Tdot)
		3. update v (from prev v, prev accel, constraints)
		4. update rho (from prev rho and prev L)
		5. update e (from prev e and prev gradV)
	*/

	if(mff->locDirty || neighborUpdate<2 || (scene->step%neighborUpdate)==0) mff->updateLocator</*useNext*/false>(); // if(mff->locDirty) throw std::runtime_error("SparcField locator was not updated to new positions.");

	FOREACH(const shared_ptr<Node>& n, field->nodes){
		nid++; SparcData& dta(n->getData<SparcData>());
		findNeighbors</*useNext*/false>(n);
		updateNeighborsRelPos</*useNext*/false>(n);
		dta.gradV=computeGradV(n);
		Matrix3r D=dta.getD(), W=dta.getW(); // symm/antisymm decomposition of dta.gradV
		Matrix3r Tcirc=computeStressRate(dta.T,D,dta.e);
		dta.nextT=dta.T+dt*(Tcirc+dta.T*W-W*dta.T);
		Vector3r divT=computeDivT</*useNext*/false>(n);
		dta.accel=(1/dta.rho)*(divT-c*dta.v);
		if(damping!=0) for(int i=0;i<3;i++) dta.accel[i]*=(dta.accel[i]*dta.v[i]>0 ? 1.-damping : 1.+damping);
		#ifdef SPARC_INSPECT
			dta.Tcirc=Tcirc;
			dta.divT=divT;
		#endif
		#ifdef YADE_DEBUG
			dta.catchCrap1(nid,n);
		#endif
	};

	nid=-1;
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		nid++; SparcData& dta(n->getData<SparcData>());
		n->pos+=dt*dta.v;
		if(spinRot) n->pos=dta.getRotQ(scene->dt)*n->pos;
		dta.T=dta.nextT; // dt*dta.Tdot;
		dta.v+=dt*dta.accel;
		applyKinematicConstraints(n,/*disallow prescribed stress*/false);
		dta.rho+=dt*(-dta.rho*dta.gradV.trace());
		dta.e+=dt*(1+dta.e)*dta.gradV.trace(); // gradV.trace()==D.trace()
		#ifdef YADE_DEBUG
			dta.catchCrap2(nid,n);
		#endif
	};
};

int StaticEquilibriumSolver::ResidualsFunctorBase::operator()(const VectorXr &v, VectorXr& resid) const {
	#ifdef SPARC_TRACE
		#define _INFO(s) "--- current "<<s->fjac.cols()<<"x"<<s->fjac.rows()<<" jacobian is "<<endl<<s->fjac<<endl<<"--- current diagonal is "<<s->diag.transpose()<<endl
		if(ses->dbgFlags&DBG_JAC){
			if(ses->solverPowell){ SPARC_TRACE_SES_OUT("### functor called"<<endl<<_INFO(ses->solverPowell));}
			else{ SPARC_TRACE_SES_OUT("### functor called"<<endl<<_INFO(ses->solverLM)); }
		}
		#undef _INFO
	#endif
	if(eig_isnan(v)){
		LOG_ERROR("Solver proposing solution with NaN's, return [∞,…,∞]"); resid.setConstant(v.size(),std::numeric_limits<Real>::infinity());
		SPARC_TRACE_SES_OUT("--- input NaN'ed v = "<<v.transpose()<<endl<<"--- !! returning "<<resid.transpose()<<endl);
		return 0;
	}
	#ifdef SPARC_TRACE
		SPARC_TRACE_SES_OUT("#-- input velocities = "<<v.transpose()<<endl);
	#endif

	ses->solutionPhase(resid);

	SPARC_TRACE_SES_OUT("### Final residuals ("<<resid.blueNorm()<<") = "<<resid.transpose()<<endl);
	return 0;
};


void StaticEquilibriumSolver::copyLocalVelocityToNodes(const VectorXr &v) const{
	assert(v.size()==nDofs);
	#ifdef YADE_DEBUG
		if(eig_isnan(v)) throw std::runtime_error("Solver proposing nan's in velocities, vv = "+lexical_cast<string>(v.transpose()));
	#endif
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta=n->getData<SparcData>();
		for(int ax:{0,1,2}){
			assert(!(!isnan(dta.fixedV[ax]) && dta.dofs[ax]>=0)); // handled by assignDofs
			assert(isnan(dta.fixedV[ax]) || dta.dofs[ax]<0);
			dta.locV[ax]=dta.dofs[ax]<0?dta.fixedV[ax]:v[dta.dofs[ax]];
		}
		dta.v=n->ori.conjugate()*dta.locV;
	};
};

void StaticEquilibriumSolver::assignDofs() {
	int nid=0, dof=0;
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta=n->getData<SparcData>();
		dta.nid=nid++;
		for(int ax:{0,1,2}){
			if(!isnan(dta.fixedV[ax])) dta.dofs[ax]=-1;
			else dta.dofs[ax]=dof++;
		}
	}
	nDofs=dof;
}

VectorXr StaticEquilibriumSolver::computeInitialDofVelocities(bool useZero) const {
	VectorXr ret; ret.setZero(nDofs);
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		const SparcData& dta(n->getData<SparcData>());
		// fixedV and zeroed are in local coords
		for(int ax:{0,1,2}){
			if(dta.dofs[ax]<0) continue;
			if(useZero){
				// TODO: assign random velocities if scene->step==0
				ret[dta.dofs[ax]]=0.;
			}
			else ret[dta.dofs[ax]]=(n->ori*dta.v)[ax]; 
		}
	}
	return ret;
};

void StaticEquilibriumSolver::prologuePhase(){
	mff->updateLocator</*useNext*/false>();
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		findNeighbors</*useNext*/false>(n); 
		if(neighborsOnce) dta.nextNeighbors=dta.neighbors;
		updateNeighborsRelPos</*useNext*/false>(n);
		if(relPosOnce){ dta.nextRelPosInv=dta.relPosInv; dta.nextWeights=dta.weights; }
	}
	assignDofs();  // this is necessary only after number of free DoFs have changed; assign at every step to ensure consistency (perhaps not necessary if this is detected in the future)
	// intial solution is previous (or zero) velocities, except wher they are prescribed
	currV=computeInitialDofVelocities(/*useZero*/initZero);
	#ifdef SPARC_TRACE
		SPARC_TRACE_OUT("Neighbor numbers: "); FOREACH(const shared_ptr<Node>& n, field->nodes) SPARC_TRACE_OUT(n->getData<SparcData>().neighbors.size()<<" "); SPARC_TRACE_OUT("\n");
	#endif
}


void StaticEquilibriumSolver::solutionPhase(VectorXr& errors){
	solutionPhase_computeResponse();
	solutionPhase_computeErrors(errors);
};

void StaticEquilibriumSolver::solutionPhase_computeResponse(){
	copyLocalVelocityToNodes(currV);
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta=n->getData<SparcData>();
		dta.gradV=computeGradV(n);
		Matrix3r D=dta.getD(), W=dta.getW(); // symm/antisymm decomposition of dta.gradV
		Matrix3r Tcirc=computeStressRate(dta.T,D,/*porosity*/dta.e);
		dta.nextT=dta.T+scene->dt*(Tcirc+dta.T*W-W*dta.T);
		#ifdef SPARC_INSPECT
			dta.Tcirc=Tcirc;
		#endif
	}
}

void StaticEquilibriumSolver::solutionPhase_computeErrors(VectorXr& errors){
	// optionally, update locator
	if(!neighborsOnce){
		mff->updateLocator</*useNext*/true>();
	}
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		findNeighbors</*useNext*/true>(n);
		if(!relPosOnce) updateNeighborsRelPos</*useNext*/true>(n);
		// TODO: update the value really, following spin perhaps?
		SparcData& dta=n->getData<SparcData>();
		dta.nextOri=n->ori;  
	}
	// resid needs nextT of other points, hence in separate loop
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		Vector3r divT=computeDivT</*useNext*/true>(n);
		#ifdef SPARC_TRACE
			if(dbgFlags&DBG_DOFERR){
				SPARC_TRACE_OUT("-->    nid "<<dta.nid<<" (dofs "<<dta.dofs.transpose()<<"), divT="<<divT.transpose()<<", v="<<dta.v.transpose());
				if(dbgFlags&DBG_NIDERR) SPARC_TRACE_OUT(endl);
			}
		#endif
		computeConstraintErrors</*useNext*/true>(n,divT,errors);
		#ifdef SPARC_INSPECT
			dta.divT=divT; // pure equilibrium residual, without constraints yet
		#endif
		#ifdef SPARC_TRACE
			if(dbgFlags&DBG_DOFERR){
				if(dbgFlags&DBG_NIDERR) SPARC_TRACE_OUT("<--     nid "<<dta.nid<<" final residuum "<<dta.resid<<endl);
				else SPARC_TRACE_OUT(", err="<<dta.resid<<endl);
			}
		#endif
	}
}

void StaticEquilibriumSolver::epiloguePhase(){
	solutionPhase_computeResponse();
	integrateStateVariables();
}

// set resid in the sense of kinematic constraints, so that they are fulfilled by the static solution
template<bool useNext>
void StaticEquilibriumSolver::computeConstraintErrors(const shared_ptr<Node>& n, const Vector3r& divT, VectorXr& resid){
	SparcData& dta(n->getData<SparcData>());
	#ifdef YADE_DEBUG
		for(int i=0;i<3;i++) if(!isnan(dta.fixedV[i])&&!isnan(dta.fixedT[i])) throw std::invalid_argument((boost::format("Nid %d: both velocity and stress prescribed along local axis %d (axis %s in global space)") %dta.nid %i %(n->ori.conjugate()*(i==0?Vector3r::UnitX():(i==1?Vector3r::UnitY():Vector3r::UnitZ()))).transpose()).str());
	#endif
	#define _WATCH_NID(aaa) SPARC_TRACE_OUT("           "<<aaa<<endl); if(dta.nid==watch) cerr<<aaa<<endl;
	#ifdef SPARC_INSPECT
		dta.resid=Vector3r::Zero(); // in global coords here
	#endif
	// use current or next orientation,stress
	Matrix3r oriTrsf(useNext?dta.nextOri:n->ori);
	const Matrix3r& T(!useNext?dta.T:dta.nextT); 

	for(int ax:{0,1,2}){
		// prescribed velocity, does not come up in the solution
		if(dta.dofs[ax]<0){ continue; }
		// nothing prescribed, divT is the residual
		if(isnan(dta.fixedT[ax])){
			// NB: dta.divT is not the current value, that exists only with SPARC_INSPECT
			Vector3r locDivT=oriTrsf*divT;
			resid[dta.dofs[ax]]=charLen*locDivT[ax];
			#ifdef SPARC_TRACE
				if(dbgFlags&DBG_DOFERR) _WATCH_NID("\tnid "<<dta.nid<<" dof "<<dta.dofs[ax]<<": locDivT["<<ax<<"] "<<locDivT[ax]<<" (should be 0) sets resid["<<dta.dofs[ax]<<"]="<<resid[dta.dofs[ax]]);
			#endif
		}
		// prescribed stress
		else{
			// considered global stress
			// TODO: check that this could be perhaps written as locAxT=(T*n).dot(n) ?
			#if 0
				Vector3r normal=oriTrsf*Vector3r::Unit(ax); // local normal (i.e. axis) in global coords
				Real locAxT=(T*normal).dot(normal);
			#else
				// current local stress
				Matrix3r locT=oriTrsf*T*oriTrsf.transpose();
				Real locAxT=locT(ax,ax);
			#endif
			// TODO end 
			resid[dta.dofs[ax]]=-(dta.fixedT[ax]-locAxT);
			#ifdef SPARC_TRACE
				if(dbgFlags&DBG_DOFERR) _WATCH_NID("\t nid"<<dta.nid<<" dof "<<dta.dofs[ax]<<": ΣT["<<ax<<",i] "<<locAxT<<" (should be "<<dta.fixedT[ax]<<") sets resid["<<dta.dofs[ax]<<"]="<<resid[dta.dofs[ax]]);
			#endif
		}
		#ifdef SPARC_INSPECT
			dta.resid+=oriTrsf.transpose()*Vector3r::Unit(ax)*resid[dta.dofs[ax]];
		#endif
	};
}

void StaticEquilibriumSolver::integrateStateVariables(){
	assert(functor);
	const Real& dt(scene->dt);

	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		// this applies kinematic constraints directly (the solver might have had some error in satisfying them)
		applyKinematicConstraints(n,/*allow prescribed divT*/true); 
		dta.e+=dt*(1+dta.e)*dta.gradV.trace(); // gradV.trace()==D.trace()
		dta.rho+=dt*(-dta.rho*dta.gradV.trace()); // rho is not really used for the implicit solution, but can be useful
		dta.T=dta.nextT;
		n->pos+=dt*dta.v;
		n->ori=dta.nextOri;
	}
};

#if 0
VectorXr StaticEquilibriumSolver::compResid(const VectorXr& vv){
	bool useNext=vv.size()>0;
	if(useNext && (size_t)vv.size()!=nDofs) yade::ValueError("len(vv) must be equal to number of nDofs ("+lexical_cast<string>(nDofs)+"), or empty for only evaluating current residuals.");
	mff=static_cast<SparcField*>(field.get());
	mff->updateLocator();
	assignDofs();
	FOREACH(const shared_ptr<Node>& n, field->nodes) findNeighbors(n); 
	if(vv.size()==0){
		copyLocalVelocityToNodes(computeInitialDofVelocities(/*useCurrV*/true));
	}
	ResidualsFunctor functor(nDofs,nDofs,this);
	VectorXr ret(nDofs);
	functor(vv,ret,useNext ? ResidualsFunctor::MODE_TRIAL_V_IS_ARG : ResidualsFunctor::MODE_CURRENT_STATE);
	return ret;
};
#endif

template<typename Solver>
string solverStatus2str(int status);

template<> string solverStatus2str<StaticEquilibriumSolver::SolverPowell>(int status){
	#define CASE_STATUS(st) case Eigen::HybridNonLinearSolverSpace::st: return string(#st);
	switch(status){
		CASE_STATUS(Running);
		CASE_STATUS(ImproperInputParameters);
		CASE_STATUS(RelativeErrorTooSmall);
		CASE_STATUS(TooManyFunctionEvaluation);
		CASE_STATUS(TolTooSmall);
		CASE_STATUS(NotMakingProgressJacobian);
		CASE_STATUS(NotMakingProgressIterations);
		CASE_STATUS(UserAsked);
	}
	#undef CASE_STATUS
	throw std::logic_error(("solverStatus2str<HybridNonLinearSolver> called with unknown status number "+lexical_cast<string>(status)).c_str());
}

template<> string solverStatus2str<StaticEquilibriumSolver::SolverLM>(int status){
	#define CASE_STATUS(st) case Eigen::LevenbergMarquardtSpace::st: return string(#st);
	switch(status){
		CASE_STATUS(NotStarted);
		CASE_STATUS(Running);
		CASE_STATUS(ImproperInputParameters);
		CASE_STATUS(RelativeReductionTooSmall);
		CASE_STATUS(RelativeErrorTooSmall);
		CASE_STATUS(RelativeErrorAndReductionTooSmall);
		CASE_STATUS(CosinusTooSmall);
		CASE_STATUS(TooManyFunctionEvaluation);
		CASE_STATUS(FtolTooSmall);
		CASE_STATUS(XtolTooSmall);
		CASE_STATUS(GtolTooSmall);
		CASE_STATUS(UserAsked);
	}
	#undef CASE_STATUS
	throw std::logic_error(("solverStatus2str<LevenbergMarquardt> called with unknown status number "+lexical_cast<string>(status)).c_str());
}


void StaticEquilibriumSolver::run(){
	#ifdef SPARC_TRACE
		if(!dbgOut.empty() && scene->step==0 && bfs::exists(dbgOut)){
			bfs::remove(bfs::path(dbgOut));
			LOG_WARN("Old StaticEquilibriumSolver.dbgOut "<<dbgOut<<" deleted.");
		}
		if(!out.is_open()) out.open(dbgOut.empty()?"/dev/null":dbgOut.c_str(),ios::out|ios::app);
		if(scene->step==0) out<<"# vim: guifont=Monospace\\ 7:nowrap:\n";
	#endif
	if(!substep || (usePowell && !solverPowell) || (!usePowell && !solverLM)){ /* start fresh iteration*/ nIter=0; }
	int status;
	SPARC_TRACE_OUT("\n\n ==== Scene step "<<scene->step<<", solution iteration "<<nIter<<endl);
	if(nIter==0 || nIter<0){
		if(nIter<0) LOG_WARN("Previous solver step ended abnormally, restarting solver.");
		mff=static_cast<SparcField*>(field.get());

		prologuePhase();

		SPARC_TRACE_OUT("Initial DOF velocities "<<currV.transpose()<<endl);
		// http://stackoverflow.com/questions/6895980/boostmake-sharedt-does-not-compile-shared-ptrtnew-t-does
		// functor=make_shared<ResidualsFunctor>(vv.size(),vv.size(),this); 
		functor=shared_ptr<ResidualsFunctor>(new ResidualsFunctor(nDofs,nDofs)); functor->ses=this;
		if(usePowell){
			solverLM.reset();
			solverPowell=make_shared<SolverPowell>(*functor);
			solverPowell->parameters.factor=solverFactor;
			if(solverXtol>0) solverPowell->parameters.xtol=solverXtol;
			solverPowell->parameters.maxfev=relMaxfev*currV.size(); // this is perhaps bogus, what is exactly maxfev?
			#ifdef SPARC_TRACE
				// avoid uninitialized values giving false positives in text comparisons
				solverPowell->fjac.setZero(nDofs,nDofs); solverPowell->diag.setZero(nDofs);
			#endif
			status=solverPowell->solveNumericalDiffInit(currV);
			SPARC_TRACE_OUT("Initial solution norm "<<solverPowell->fnorm<<endl);
			#ifdef SPARC_TRACE
				solverPowell->fjac.setZero();
			#endif
			if(status==Eigen::HybridNonLinearSolverSpace::ImproperInputParameters) throw std::runtime_error("StaticEquilibriumSolver:: improper input parameters for the Powell dogleg solver.");
			// solver->parameters.epsfcn=1e-6;
			nFactorLowered=0;
		} else {
			solverPowell.reset();
			solverLM=make_shared<SolverLM>(*functor);
			solverLM->parameters.factor=solverFactor;
			solverLM->parameters.maxfev=relMaxfev*currV.size(); // this is perhaps bogus, what is exactly maxfev?
			#ifdef SPARC_TRACE
				// avoid uninitialized values giving false positives in text comparisons
				solverLM->fjac.setZero(nDofs,nDofs); solverLM->diag.setZero(nDofs);
			#endif
			status=solverLM->minimizeInit(currV);
			SPARC_TRACE_OUT("Initial solution norm "<<solverLM->fnorm<<endl);
			#ifdef SPARC_TRACE
				solverLM->fjac.setZero();
			#endif
			if(status==Eigen::LevenbergMarquardtSpace::ImproperInputParameters) throw std::runtime_error("StaticEquilibriumSolver:: improper input parameters for the Levenberg-Marquardt solver.");
		}
		// intial solver setup
		nIter=0;
	}

	// solution loop
	while(true){
		// nIter is negative inside step to detect interruption by an exception in python
		nIter=-abs(nIter);
		nIter--; 

		if(solverPowell){
			status=solverPowell->solveNumericalDiffOneStep(currV);
			SPARC_TRACE_OUT("Powell inner iteration "<<-nIter<<endl<<"Solver proposed solution "<<currV.transpose()<<endl<<"Residuals vector "<<solverPowell->fvec.transpose()<<endl<<"Error norm "<<solverPowell->fnorm<<endl);
			#ifdef SPARC_INSPECT
				residuals=solverPowell->fvec; residuum=solverPowell->fnorm;
			#endif
		} else {
			assert(solverLM);
			status=solverLM->minimizeOneStep(currV);
			SPARC_TRACE_OUT("Levenberg-Marquardt inner iteration "<<-nIter<<endl<<"Solver proposed solution "<<currV.transpose()<<endl<<"Residuals vector "<<solverLM->fvec.transpose()<<endl<<"Error norm "<<solverLM->fnorm<<endl);
			#ifdef SPARC_INSPECT
				residuals=solverLM->fvec; residuum=solverLM->fnorm;
			#endif
		}
		// cerr<<"\titer="<<-nIter<<", "<<nDofs<<" dofs, residuum "<<solver->fnorm<<endl;
		// good progress
		if((solverPowell && status==Eigen::HybridNonLinearSolverSpace::Running) || (solverLM && status==Eigen::LevenbergMarquardtSpace::Running)){ if(substep) goto substepDone; else continue; }
		// solution found
		if((solverPowell && status==Eigen::HybridNonLinearSolverSpace::RelativeErrorTooSmall) ||
			(solverLM && (status==Eigen::LevenbergMarquardtSpace::RelativeErrorTooSmall || status==Eigen::LevenbergMarquardtSpace::RelativeErrorAndReductionTooSmall || status==Eigen::LevenbergMarquardtSpace::RelativeReductionTooSmall || status==Eigen::LevenbergMarquardtSpace::CosinusTooSmall)))
			goto solutionFound;
		// bad convergence
		if(solverPowell && (status==Eigen::HybridNonLinearSolverSpace::NotMakingProgressIterations || status==Eigen::HybridNonLinearSolverSpace::NotMakingProgressJacobian)){
			// try decreasing factor
			solverPowell->parameters.factor*=.6;
			if(++nFactorLowered<10){ LOG_WARN("Step "<<scene->step<<": solver did not converge (error="<<solverPowell->fnorm<<"), lowering factor to "<<solverPowell->parameters.factor); if(substep) goto substepDone; else continue; }
			// or give up
			LOG_WARN("Step "<<scene->step<<": solver not converging, but factor already lowered too many times ("<<nFactorLowered<<"). giving up.");
		}
		string msg(solverPowell?solverStatus2str<SolverPowell>(status):solverStatus2str<SolverLM>(status));
		throw std::runtime_error((boost::format("Solver did not find acceptable solution, returned %s (%d).")%msg%status).str());
	}
	substepDone:
		if(substep) copyLocalVelocityToNodes(currV);
		nIter=abs(nIter); // make nIter positive, to indicate successful substep
		return;
	solutionFound:
		// LOG_WARN("Solution found, error norm "<<solver->fnorm);
		if(solverPowell) nIter=solverPowell->iter; // next step will be from the start again, since the number is positive
		else nIter=solverLM->iter;
		// residuum=solver->fnorm;
		//integrateStateVariables();
		epiloguePhase();
		return;
};

/* compute errors of velocity interpolation WRT neighbors */
Real StaticEquilibriumSolver::gradVError(const shared_ptr<Node>& n, int rPow){
	Real ret=0;
	const SparcData& dta=n->getData<SparcData>();
	for(size_t i=0; i<dta.neighbors.size(); i++){
		const shared_ptr<Node>& nn=dta.neighbors[i]; 
		const SparcData& nd=nn->getData<SparcData>();
		Vector3r relPos=nn->pos-n->pos;
		ret+=pow(relPos.norm(),rPow)*(dta.v+dta.gradV*relPos-nd.v).norm();
	}
	return ret;
};


#ifdef YADE_OPENGL
#include<yade/lib/base/CompUtils.hpp>
#include<yade/lib/opengl/OpenGLWrapper.hpp>
#include<yade/lib/opengl/GLUtils.hpp>
#include<yade/pkg/gl/Renderer.hpp>


YADE_PLUGIN(gl,(SparcConstraintGlRep));

void SparcConstraintGlRep::renderLabeledArrow(const Vector3r& pos, const Vector3r& vec, const Vector3r& color, Real num, bool posIsA, bool doubleHead){
	Vector3r A(posIsA?pos:pos-vec), B(posIsA?pos+vec:pos);
	GLUtils::GLDrawArrow(A,B,color);
	if(doubleHead) GLUtils::GLDrawArrow(A,A+.9*(B-A),color);
	if(!isnan(num)) GLUtils::GLDrawNum(num,posIsA?B:A,Vector3r::Ones());
}

void SparcConstraintGlRep::render(const shared_ptr<Node>& node, GLViewInfo* viewInfo){
	// if(!vRange || !tRange) return; // require ranges
	Real len=(relSz*viewInfo->sceneRadius);
	Vector3r pos=node->pos+(node->hasData<GlData>()?node->getData<GlData>().dGlPos:Vector3r::Zero());
	for(int i:{0,1,2}){
		for(int j:{0,1}){
			bool isV=(j==0);
			Real val=(isV?fixedV[i]:fixedT[i]);
			if(isnan(val)) continue;
			#if 0
				const Vector2r& cMnMx=(isV?vColor:tColor);
				const shared_ptr<ScalarRange>& rg=(isV?vRange:tRange);
				Real n=rg->norm(val); n=cMnMx[0]+n*(cMnMx[1]-cMnMx[0]);
				Vector3r color=CompUtils::mapColor(n,rg->cmap);
			#endif
			Vector3r color=isV?(val!=0?Vector3r(0,1,0):Vector3r(1-.2*i,0+.2*i,0)):Vector3r(.2,.2,1);
			// axis in global coords
			Vector3r ax=Vector3r::Zero(); ax[i]=(val>=0?1:-1)*len; ax=node->ori.conjugate()*ax; 
			if(isV && val==0){ // zero prescribed velocity gets disc displayed
				GLUtils::Cylinder(pos,pos+.05*ax,.35*len,color,/*wire*/false,/*caps*/true,/*rad2*/-1,/*slices*/12);
				continue;
			}
			// arrow is away from the node for tension (!isV&&val>0) [outside], or velocity inside (isV&&val<0)
			bool posIsA=((!isV&&val>0)||(isV&&val<0));
			renderLabeledArrow(pos,ax,color,/*passing NaN suppresses displaying the number*/num?val:NaN,/*posIsA*/posIsA,/*doubleHead*/!isV);
		}
	};
};

#endif /*YADE_OPENGL*/


#endif /*YADE_VTK*/
