#ifdef YADE_VTK
#include<yade/pkg/sparc/SparcField.hpp>

#include<boost/preprocessor.hpp>

using boost::format;

YADE_PLUGIN(sparc,(SparcField)(SparcData)(ExplicitNodeIntegrator)(StaticEquilibriumSolver));

#if 0
// http://listengine.tuxfamily.org/lists.tuxfamily.org/eigen/2010/01/msg00187.html
template<typename MatrixT>
static bool Matrix_pseudoInverse(const MatrixT &a, MatrixT &result, double epsilon=std::numeric_limits<typename MatrixT::Scalar>::epsilon()) {
	if(a.rows()<a.cols()) return false;
	Eigen::SVD<MatrixT>svd (a.svd());
	typename MatrixT::Scalar tolerance = epsilon * std::max(a.cols(), a.rows()) * svd.singularValues().cwise().abs().maxCoeff();
	result = svd.matrixV() * (svd.singularValues().cwise().abs() > tolerance).select(svd.singularValues().cwise().inverse(), 0).asDiagonal() * svd.matrixU().adjoint();
	return true;
}
#endif

void SparcField::updateLocator(){
	assert(locator && points && grid);
	/* adjust points size */
	if(points->GetNumberOfPoints()!=(int)nodes.size()) points->SetNumberOfPoints(nodes.size());
	//FOREACH(const shared_ptr<Node>& n, nodes){
	size_t sz=nodes.size();
	for(size_t i=0; i<sz; i++){
		const shared_ptr<Node>& n(nodes[i]);
		assert(n->hasData<SparcData>());
		//n->getData<SparcData>().locatorId=
		points->SetPoint(i,n->pos[0],n->pos[1],n->pos[2]);
	}
	// points->ComputeBounds(); points->GetBounds(bb)
	locator->BuildLocator();
	locDirty=false;
};

// template<> std::string boost::lexical_cast<Vector3r,std::string>(const Vector3r& v){ return "("+lexical_cast<string>(v[0])+","+lexical_cast<string>(v[1])+","+lexical_cast<string>(v[2])+")"; }

// using std::isnan;

template<typename M>
bool eig_isnan(const M& m){for(int j=0; j<m.cols(); j++)for(int i=0;i<m.rows(); i++) if(isnan(m(i,j))) return true; return false; }

template<typename M>
bool eig_all_isnan(const M&m){for(int j=0; j<m.cols(); j++)for(int i=0;i<m.rows();i++) if(!isnan(m(i,j))) return false; return true; }

template<>
bool eig_isnan<Vector3r>(const Vector3r& v){for(int i=0; i<v.size(); i++) if(isnan(v[i])) return true; return false; }

template<>
bool eig_isnan(const Real& r){ return isnan(r); }

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
	if(self && j>numIds-1){ throw std::runtime_error(("SparcField::nodesAround asked to remove (central?) point form the result, but it was not there (node pos="+lexical_cast<string>(self->pos.transpose())+", search point="+lexical_cast<string>(pt.transpose())+")").c_str()); }
	_ids->Delete();
	return ret;
};

void ExplicitNodeIntegrator::findNeighbors(const shared_ptr<Node>&n) const {
	SparcData& dta(n->getData<SparcData>());
	dta.neighbors=mff->nodesAround(n->pos,/*count*/-1,rSearch,/*self=*/n);
};

void ExplicitNodeIntegrator::updateNeighborsRelPos(const shared_ptr<Node>& n, bool useNext) const {
	SparcData& dta(n->getData<SparcData>());
	int sz(dta.neighbors.size());
	if(sz<3) yade::RuntimeError(format("Node at %s has lass than 3 neighbors")%n->pos.transpose());
	MatrixXr relPos(sz,3);
	#ifdef SPARC_WEIGHTS
		dta.rWeights=rPow>0?VectorXr(sz):VectorXr();
	#endif
	Vector3r currPos(n->pos+(useNext?Vector3r(scene->dt*dta.v):Vector3r::Zero()));
	for(int i=0; i<sz; i++){
		relPos.row(i)=VectorXr(dta.neighbors[i]->pos+(useNext?Vector3r(scene->dt*dta.neighbors[i]->getData<SparcData>().v):Vector3r::Zero())-currPos);
		#if SPARC_WEIGHTS
			if(rPow>0){
				dta.rWeights[i]=(rPow%2==0?pow(Vector3r(relPos.row(i)).squarednorm(),rPow/2):pow(Vector3r(relPos.row(i)).norm(),rPow));
				relPos.row(i)*=rWeights;
			}
		#endif
	}
	#ifdef SPARC_INSPECT
		dta.relPos=relPos;
	#endif
	if(eig_isnan(relPos)){
		cerr<<"=== Relative positions for nid "<<dta.nid<<":\n"<<relPos<<endl;
		throw std::runtime_error("NaN's in relative positions in O.sparc.nodes["+lexical_cast<string>(dta.nid)+"].sparc.relPos .");
	}
	bool succ=MatrixXr_pseudoInverse(relPos,/*result*/dta.relPosInv);
	if(!succ) throw std::runtime_error("ExplicitNodeIntegrator::updateNeigbours: pseudoInverse failed.");
}

Vector3r ExplicitNodeIntegrator::computeDivT(const shared_ptr<Node>& n, bool useNext) const {
	int vIx[][2]={{0,0},{1,1},{2,2},{1,2},{2,0},{0,1}}; // from voigt index to matrix indices
	const SparcData& midDta=n->getData<SparcData>();
	const vector<shared_ptr<Node> >& NN(midDta.neighbors);  size_t sz=NN.size();
	Vector3r A[6];
	for(int l=0; l<6; l++){
		int i=vIx[l][0], j=vIx[l][1]; // matrix indices from voigt indices
		VectorXr rhs(sz); for(size_t r=0; r<sz; r++){
			const SparcData& nDta=NN[r]->getData<SparcData>();
			rhs[r]=nDta.T(i,j)-midDta.T(i,j)+(useNext?scene->dt*(nDta.Tdot(i,j)-midDta.Tdot(i,j)):0);
		}
		A[l]=midDta.relPosInv*rhs;
		if(eig_isnan(A[l])) yade::ValueError(format("Node at %s has NaNs in A[%i]=%s")%n->pos%l%A[l].transpose());
	};
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
	MatrixXr relVels(sz,3); for(size_t i=0; i<sz; i++){
		relVels.row(i)=VectorXr(NN[i]->getData<SparcData>().v-midDta.v);
	}
	#ifdef SPARC_INSPECT
		n->getData<SparcData>().relVels=relVels;
	#endif
	return midDta.relPosInv*relVels;
}

Matrix3r ExplicitNodeIntegrator::computeStressRate(const Matrix3r& T, const Matrix3r& D, const Real e /* =-1 */) const {
	#define CC(i) barodesyC[i-1]
	switch(matModel){
		case MAT_HOOKE:
			// isotropic linear elastic
			return voigt_toSymmTensor((C*tensor_toVoigt(D,/*strain*/true)).eval());
		case MAT_BARODESY_JESSE:{
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
			return h*(f*R0+g*T0)*Dnorm;
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
	if(matModel<0 || matModel>MAT_SENTINEL) yade::ValueError(boost::format("matModel must be in range 0..%d")%MAT_SENTINEL);
	if(barodesyC.size()!=6) yade::ValueError(boost::format("barodesyC must have exactly 6 values (%d given)")%barodesyC.size());
}

#define _CATCH_CRAP(x,y,z) if(eig_isnan(z)){ throw std::runtime_error((boost::format("NaN in O.sparc.nodes[%d], attribute %s: %s\n")%nid%BOOST_PP_STRINGIZE(z)%z).str().c_str()); }

void SparcData::catchCrap1(int nid, const shared_ptr<Node>& node){
	BOOST_PP_SEQ_FOR_EACH(_CATCH_CRAP,~,(gradV)(Tdot)(accel)
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


void ExplicitNodeIntegrator::run(){
	mff=static_cast<SparcField*>(field.get());
	const Real& dt=scene->dt;
	int nid=-1;
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

	if(mff->locDirty || (scene->step%neighborUpdate)==0) mff->updateLocator(); // if(mff->locDirty) throw std::runtime_error("SparcField locator was not updated to new positions.");

	FOREACH(const shared_ptr<Node>& n, field->nodes){
		nid++; SparcData& dta(n->getData<SparcData>());
		findNeighbors(n);
		updateNeighborsRelPos(n);
		dta.gradV=computeGradV(n);
		Matrix3r D=dta.getD(), W=dta.getW(); // symm/antisymm decomposition of dta.gradV
		Matrix3r Tcirc=computeStressRate(dta.T,D,dta.e);
		dta.Tdot=Tcirc+dta.T*W-W*dta.T;
		Vector3r divT=computeDivT(n);
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
		dta.T+=dt*dta.Tdot;
		dta.v+=dt*dta.accel;
		applyKinematicConstraints(n,/*disallow prescribed stress*/false);
		dta.rho+=dt*(-dta.rho*dta.gradV.trace());
		dta.e+=dt*(1+dta.e)*dta.gradV.trace(); // gradV.trace()==D.trace()
		#ifdef YADE_DEBUG
			dta.catchCrap2(nid,n);
		#endif
	};
};

/*
mode determines what input data are used to compute the residual, and at which time point:

1. MODE_CURRENT_STATE
	only update relative positions and compute residuals, in the current moment
2. MODE_TRIAL_V_IN_NODES
	use velocity inside nodes (SparcData.v) to compute relative positions at t+Δt,
	evaluate Tdot etc to compute residuals at a future point
3. MODE_TRIAL_V_IS_ARG
	like 2, but copy velocities passed in the first argument inside nodes first
	(this is what the solver uses)

*/
int StaticEquilibriumSolver::ResidualsFunctor::operator()(const VectorXr &v, VectorXr& resid, int mode) const {
	// static unsigned long N=0; if((N++%1)==0) cerr<<".["<<v.size()<<","<<ses->field->nodes.size()<<"]";
	SPARC_TRACE_SES_OUT(
		"### functor called"<<endl
		<<"--- current "<<ses->solver->fjac.cols()<<"x"<<ses->solver->fjac.rows()<<" jacobian is "<<endl<<ses->solver->fjac<<endl
		<<"--- current diagonal is "<<ses->solver->diag.transpose()<<endl);
	if(eig_isnan(v)){
		LOG_ERROR("Solver proposing solution with NaN's, return [∞,…,∞]"); resid.setConstant(v.size(),std::numeric_limits<Real>::infinity());
		SPARC_TRACE_SES_OUT("--- input NaN'ed v = "<<v.transpose()<<endl<<"--- !! returning "<<resid.transpose()<<endl);
		return 0;
	}
	assert((mode==MODE_TRIAL_V_IS_ARG || v.size()>0) || mode!=MODE_TRIAL_V_IS_ARG);
	// copy velocity to inside node data (could be eliminated later to be more efficient perhaps)
	if(mode==MODE_TRIAL_V_IS_ARG) ses->copyLocalVelocityToNodes(v);
	bool useNext=(mode!=MODE_CURRENT_STATE);
	SPARC_TRACE_SES_OUT("--- input v = "<<v.transpose()<<endl);
	if(!useNext){ // move conditional outside the loop
		FOREACH(const shared_ptr<Node>& n, ses->field->nodes){
			ses->updateNeighborsRelPos(n,/*useNext*/false);
		}
	}
	else {
		FOREACH(const shared_ptr<Node>& n, ses->field->nodes){
			SparcData& dta(n->getData<SparcData>());
			if(!ses->relPosOnce) ses->updateNeighborsRelPos(n,/*useNext*/true);
			dta.gradV=ses->computeGradV(n);
			Matrix3r D=dta.getD(), W=dta.getW(); // symm/antisymm decomposition of dta.gradV
			Matrix3r Tcirc=ses->computeStressRate(dta.T,D,/*porosity*/ses->nextPorosity(dta.e,D));
			dta.Tdot=Tcirc+dta.T*W-W*dta.T;
			#ifdef SPARC_INSPECT
				dta.Tcirc=Tcirc;
			#endif
		}
	}
	// resid needs Tdot of other points, in separate loop
	FOREACH(const shared_ptr<Node>& n, ses->field->nodes){
		SparcData& dta(n->getData<SparcData>());
		Vector3r divT=ses->computeDivT(n,/*useNext*/useNext);
		SPARC_TRACE_SES_OUT("-->    nid "<<dta.nid<<" (dofs "<<dta.dofs.transpose()<<"), divT="<<divT.transpose()<<endl);
		ses->applyConstraintsAsResiduals(n,divT,resid,/*useNextT*/useNext);
		#ifdef SPARC_INSPECT
			dta.divT=divT; // pure equilibrium residual, without constraints yet
			SPARC_TRACE_SES_OUT("<--     nid "<<dta.nid<<" final residuum "<<dta.resid<<endl);
		#endif
	}
	SPARC_TRACE_SES_OUT("@@ Final residuals ("<<resid.blueNorm()<<") = "<<resid.transpose()<<endl);
	SPARC_TRACE_SES_OUT("@@ for input velocities = "<<v.transpose()<<endl);
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

// set resid in the sense of kinematic constraints, so that they are fulfilled by the static solution
void StaticEquilibriumSolver::applyConstraintsAsResiduals(const shared_ptr<Node>& n, const Vector3r& divT, VectorXr& resid, bool useNextT){
	SparcData& dta(n->getData<SparcData>());
	#ifdef YADE_DEBUG
		for(int i=0;i<3;i++) if(!isnan(dta.fixedV[i])&&!isnan(dta.fixedT[i])) throw std::invalid_argument((boost::format("Nid %d: both velocity and stress prescribed along local axis %d (axis %s in global space)") %dta.nid %i %(n->ori.conjugate()*(i==0?Vector3r::UnitX():(i==1?Vector3r::UnitY():Vector3r::UnitZ()))).transpose()).str());
	#endif
	#define _WATCH_NID(aaa) SPARC_TRACE_OUT("           "<<aaa<<endl); if(dta.nid==watch) cerr<<aaa<<endl;
	// _WATCH_NID("\tnid "<<dta.nid<<", divT="<<divT);
	#ifdef SPARC_INSPECT
		dta.resid=Vector3r::Zero(); // in global coords here
	#endif
	for(int ax:{0,1,2}){
		// prescribed velocity, does not come up in the solution
		if(dta.dofs[ax]<0){
			#ifdef SPARC_INSPECT
				// dta.resid[ax]=NaN;
			#endif
			continue; 
		}
		// nothing prescribed, divT is the residual
		if(isnan(dta.fixedT[ax])){
			// NB: dta.divT is not the current value, that exists only with SPARC_INSPECT
			Vector3r locDivT=n->ori*divT;
			resid[dta.dofs[ax]]=charLen*locDivT[ax];
			_WATCH_NID("\t#"<<dta.nid<<" dof "<<dta.dofs[ax]<<": locDivT["<<ax<<"] "<<locDivT[ax]<<" (should be 0) sets resid["<<dta.dofs[ax]<<"]="<<resid[dta.dofs[ax]]);
		}
		// prescribed stress
		else{
			// TODO: check that this could be perhaps written as locAxT=(T*n).dot(n) ?
			// current global stress
			Matrix3r T=dta.T+(useNextT?Matrix3r(dta.Tdot*scene->dt):Matrix3r::Zero()); 
			// current local stress
			Matrix3r trsf(n->ori);
			Matrix3r locT=trsf*T*trsf.transpose();
			Real locAxT=locT.row(ax).sum(); // Real locAxT=locT(i,i);
			// TODO end 
			resid[dta.dofs[ax]]=-(dta.fixedT[ax]-locAxT);
			_WATCH_NID("\t#"<<dta.nid<<" dof "<<dta.dofs[ax]<<": ΣT["<<ax<<",i] "<<locAxT<<" (should be "<<dta.fixedT[ax]<<") sets resid["<<dta.dofs[ax]<<"]="<<resid[dta.dofs[ax]]);
		}
		#ifdef SPARC_INSPECT
			dta.resid+=n->ori.conjugate()*Vector3r::Unit(ax)*resid[dta.dofs[ax]];
		#endif
	};
}

VectorXr StaticEquilibriumSolver::computeInitialDofVelocities() const {
	VectorXr ret; ret.setZero(nDofs);
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		const SparcData& dta(n->getData<SparcData>());
		// fixedV and zeroed are in local coords
		const bool useCurrV=false;
		for(int ax:{0,1,2}){
			if(dta.dofs[ax]<0) continue;
			ret[dta.dofs[ax]]=useCurrV?dta.locV[ax]:0;
		}
	}
	return ret;
};

void StaticEquilibriumSolver::integrateLocalVels(const VectorXr& v, VectorXr& resid){
	const Real& dt=scene->dt;
	assert(functor);
	// recompute node variables from v
	// TODO: to avoid extra loop for integration, it could be moved to inside the functor, and executed conditionally?
	(*functor)(v,resid,/*mode*/ResidualsFunctor::MODE_TRIAL_V_IS_ARG);

	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		// this applies kinematic constraints directly (the solver might have had some error in satisfying them)
		applyKinematicConstraints(n,/*allow prescribed divT*/true); 
		dta.e+=dt*(1+dta.e)*dta.gradV.trace(); // gradV.trace()==D.trace()
		dta.rho+=dt*(-dta.rho*dta.gradV.trace()); // rho is not really used for the implicit solution, but can be useful
		dta.T+=dt*dta.Tdot;
		n->pos+=dt*dta.v;
	}
};


VectorXr StaticEquilibriumSolver::compResid(const VectorXr& vv){
	bool useNext=vv.size()>0;
	if(useNext){
		mff=static_cast<SparcField*>(field.get());
		mff->updateLocator();
		assignDofs();
		if((size_t)vv.size()!=nDofs) yade::ValueError("len(vv) must be equal to number of nDofs ("+lexical_cast<string>(nDofs)+"), or empty for only evaluating current residuals.");
		FOREACH(const shared_ptr<Node>& n, field->nodes) findNeighbors(n); 
	}
	ResidualsFunctor functor(nDofs,nDofs,this);
	VectorXr ret(nDofs);
	functor(vv,ret,useNext ? ResidualsFunctor::MODE_TRIAL_V_IS_ARG : ResidualsFunctor::MODE_CURRENT_STATE);
	return ret;
};

template<typename Solver>
string solverStatus2str(int status);

template<> string solverStatus2str<StaticEquilibriumSolver::SolverT>(int status){
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
	throw std::logic_error(("solverStatus2str called with unknown status number "+lexical_cast<string>(status)).c_str());
}

void StaticEquilibriumSolver::run(){
	#ifdef SPARC_TRACE
		out.open(dbgOut.empty()?"/dev/null":dbgOut.c_str(),ios::out|ios::app);
	#endif
	if(!solver || !substep){ /* start fresh iteration*/ nIter=0; }
	int status;
	using namespace Eigen::HybridNonLinearSolverSpace; // status constants
	SPARC_TRACE_OUT("\n\n ==== Scene step "<<scene->step<<", solution iteration "<<nIter<<endl);
	if(nIter==0 || nIter<0){
		if(nIter<0) LOG_WARN("Previous solver step ended abnormally, restarting solver.");
		mff=static_cast<SparcField*>(field.get());
		/* BEGIN: factor out to a common step prologue func? */
			if(mff->locDirty || (scene->step%neighborUpdate)==0) mff->updateLocator();
			assignDofs();  // this is necessary only after number of free DoFs have changed; assign at every step to ensure consistency (perhaps not necessary if this is detected in the future)
			// assume neighbors don't change during one step
			FOREACH(const shared_ptr<Node>& n, field->nodes) findNeighbors(n); 
			SPARC_TRACE_OUT("Neighbor numbers: "); FOREACH(const shared_ptr<Node>& n, field->nodes) SPARC_TRACE_OUT(n->getData<SparcData>().neighbors.size()<<" "); SPARC_TRACE_OUT("\n");
		/* END */

		// intial solution is previous (or zero) velocities, except where they are prescribed
		vv=computeInitialDofVelocities();
		SPARC_TRACE_OUT("Initial DOF velocities "<<vv.transpose()<<endl);
		// evaluate relpos here rather than from the functor; this needs velocities in nodes already
		if(relPosOnce){
			copyLocalVelocityToNodes(vv);
			FOREACH(const shared_ptr<Node>& n, field->nodes) updateNeighborsRelPos(n,/*useNext*/true);
		}
		// http://stackoverflow.com/questions/6895980/boostmake-sharedt-does-not-compile-shared-ptrtnew-t-does
		// functor=make_shared<ResidualsFunctor>(vv.size(),vv.size(),this); 
		functor=shared_ptr<ResidualsFunctor>(new ResidualsFunctor(nDofs,nDofs,this));
		solver=make_shared<SolverT>(*functor);
		solver->parameters.factor=solverFactor;
		solver->parameters.maxfev=relMaxfev*vv.size(); // this is perhaps bogus, what is exactly maxfev?
		#ifdef SPARC_TRACE
			// avoid uninitialized values giving false positives in text comparisons
			solver->fjac.setZero(nDofs,nDofs); solver->diag.setZero(nDofs);
		#endif
		// solver->parameters.epsfcn=1e-6;
		nFactorLowered=0;
		
		// intial solver setup
		status=solver->solveNumericalDiffInit(vv);
		SPARC_TRACE_OUT("Initial solution norm "<<solver->fnorm<<endl);
		#ifdef SPARC_TRACE
			solver->fjac.setZero();	// SPARC_TRACE_OUT("Zeroed jacobian "<<endl<<solver->fjac<<endl);
		#endif
		if(status==ImproperInputParameters) throw std::runtime_error("StaticEquilibriumSolver:: improper input parameters for the solver.");
		nIter=0;
	}

	// solution loop
	while(true){
		// nIter is negative inside step to detect interruption by an exception in python
		nIter=-abs(nIter);
		nIter--; 

		status=solver->solveNumericalDiffOneStep(vv);
		SPARC_TRACE_OUT(
			"Inner iteration "<<-nIter<<endl
			<<"Solver proposed solution "<<vv.transpose()<<endl
			<<"Residuals vector "<<solver->fvec.transpose()<<endl
			<<"Error norm "<<solver->fnorm<<endl);

		cerr<<"\titer="<<-nIter<<", "<<nDofs<<" dofs, residuum "<<solver->fnorm<<endl;
		#ifdef SPARC_INSPECT
			residuals=solver->fvec; residuum=solver->fnorm;
		#endif
		// good progress
		if(status==Running){ if(substep) goto substepDone; else continue; }
		// solution found
		if(status==RelativeErrorTooSmall) goto solutionFound;
		// bad convergence
		if(status==NotMakingProgressIterations || status==NotMakingProgressJacobian){
			// try decreasing factor
			solver->parameters.factor*=.6;
			if(++nFactorLowered<10){ LOG_WARN("Step "<<scene->step<<": solver did not converge, lowering factor to "<<solver->parameters.factor); if(substep) goto substepDone; else continue; }
			// or give up
			LOG_WARN("Step "<<scene->step<<": solver not converging, but factor already lowered too many times ("<<nFactorLowered<<"). giving up.");
		}
		throw std::runtime_error((boost::format("Solver did not find acceptable solution, returned %s (%d).")%solverStatus2str<SolverT>(status)%status).str());
	}
	substepDone:
		if(substep) copyLocalVelocityToNodes(vv);
		#ifdef SPARC_INSPECT  // copy residuals to nodes for easy inspection
			FOREACH(const shared_ptr<Node>& node, field->nodes){ SparcData& dta(node->getData<SparcData>()); for(int ax:{0,1,2}) dta.resid[ax]=dta.dofs[ax]>0?residuals[dta.dofs[ax]]:NaN; }
		#endif
		nIter=abs(nIter); // make nIter positive, to indicate successful substep
		return;
	solutionFound:
		LOG_WARN("Solution found, error norm "<<solver->fnorm);
		nIter=0; // next step will be from the start again
		// residuum=solver->fnorm;
		/* The last call to functor is not necessarily the one with chosen solution; therefore, velocities must be copied back to nodes again! */
		integrateLocalVels(vv,residuals);
		return;
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
