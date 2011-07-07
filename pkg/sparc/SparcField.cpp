#ifdef YADE_VTK
#include<yade/pkg/sparc/SparcField.hpp>

#include<boost/preprocessor.hpp>

#include<unsupported/Eigen/NonLinearOptimization>
#include<unsupported/Eigen/MatrixFunctions>


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


template<typename M>
bool eig_isnan(const M& m){for(int i=0; i<m.cols(); i++)for(int j=0;j<m.rows(); j++) if(isnan(m(i,j))) return true; return false; }

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
	Vector3r currPos(n->pos+(useNext?Vector3r(scene->dt*dta.v):Vector3r::Zero()));
	for(int i=0; i<sz; i++) relPos.row(i)=VectorXr(dta.neighbors[i]->pos+(useNext?Vector3r(scene->dt*dta.neighbors[i]->getData<SparcData>().v):Vector3r::Zero())-currPos);
	bool succ=MatrixXr_pseudoInverse(relPos,/*result*/dta.relPosInv);
	if(!succ) throw std::runtime_error("ExplicitNodeIntegrator::updateNeigbours: pseudoInverse failed.");
	#ifdef SPARC_INSPECT
		dta.relPos=relPos;
	#endif
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
	dta.v=n->ori.conjugate()*dta.v;
	if(!permitFixedDivT && dta.fixedDivT!=Vector3r(NaN,NaN,NaN)){
		throw std::invalid_argument((boost::format("Nid %d: fixedDivT not allowed (is %s")%dta.fixedDivT.transpose()).str());
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


struct StressDivergenceFunctor{
	typedef Real Scalar;
	enum { InputsAtCompileTime=Eigen::Dynamic, ValuesAtCompileTime=Eigen::Dynamic};
	typedef VectorXr InputType;
	typedef VectorXr ValueType;
	typedef MatrixXr JacobianType;
	const int m_inputs, m_values;
	int inputs() const { return m_inputs; }
	int values() const { return m_values; }
	StaticEquilibriumSolver* ses;
	StressDivergenceFunctor(int inputs, int values, StaticEquilibriumSolver* _ses): m_inputs(inputs), m_values(values), ses(_ses){}
	int operator()(const VectorXr &v, VectorXr& divT) const {
		// copy velocity to inside node data (could be eliminated later to be more efficient)
		ses->copyVelocityToNodes(v);
		FOREACH(const shared_ptr<Node>& n, ses->field->nodes){
			SparcData& dta(n->getData<SparcData>());
			ses->updateNeighborsRelPos(n,/*useNext*/true);
			dta.gradV=ses->computeGradV(n);
			Matrix3r D=dta.getD(), W=dta.getW(); // symm/antisymm decomposition of dta.gradV
			Matrix3r Tcirc=ses->computeStressRate(dta.T,D,/*porosity*/ses->nextPorosity(dta.e,D));
			dta.Tdot=Tcirc+dta.T*W-W*dta.T;
			#ifdef SPARC_INSPECT
				dta.Tcirc=Tcirc;
			#endif
		}
		// divT needs Tdot of other points, in separate loop
		FOREACH(const shared_ptr<Node>& n, ses->field->nodes){
			SparcData& dta(n->getData<SparcData>());
			Vector3r nodeDivT=ses->computeDivT(n,/*useNext*/true);
			ses->applyConstraintsAsDivT(n,nodeDivT,/*useNextT*/true);
			divT.segment<3>(dta.nid*3)=nodeDivT;
			#ifdef SPARC_INSPECT
				dta.divT=nodeDivT;
			#endif
		}
		return 0;
	};
};

void StaticEquilibriumSolver::copyVelocityToNodes(const VectorXr &v) const{
	size_t sz=field->nodes.size();
	for(size_t nid=0; nid<sz; nid++){
		field->nodes[nid]->getData<SparcData>().v=v.segment<3>(nid*3);
	}
};

void StaticEquilibriumSolver::renumberNodes() const{
	int i=0; FOREACH(const shared_ptr<Node>& n, field->nodes) n->getData<SparcData>().nid=i++;
}

// set divT in the sense of kinematic constraints, so that they are fulfilled by the static solution
void StaticEquilibriumSolver::applyConstraintsAsDivT(const shared_ptr<Node>& n, Vector3r& divT, bool useNextT) const {
	SparcData& dta(n->getData<SparcData>());
	#ifdef YADE_DEBUG
		for(int i=0;i<3;i++) if(!isnan(dta.fixedV[i])&&!isnan(dta.fixedDivT[i])) throw std::invalid_argument((boost::format("Nid %d: both velocity and stress prescribed along local axis %d (%s in global space)") %dta.nid %i %(n->ori.conjugate()*(i==0?Vector3r::UnitX():(i==1?Vector3r::UnitY():Vector3r::UnitZ()))).transpose()).str());
	#endif
	if(dta.fixedV!=Vector3r(NaN,NaN,NaN)){
		Vector3r locV=n->ori*dta.v;
		Vector3r locDivT=n->ori*divT;
		for(int i=0;i<3;i++){
			if(isnan(dta.fixedV[i])) continue; // no velocity prescribed, leave locDivT at current value
			locDivT[i]=(locV[i]-dta.fixedV[i])*scene->dt*supportStiffness;
			//cerr<<"Nid "<<dta.nid<<", v["<<i<<"] "<<locV[i]<<"(should be "<<dta.fixedV[i]<<") adds locDivT "<<locDivT[i]<<endl;
		}
		divT=n->ori.conjugate()*locDivT;
	}
	if(dta.fixedDivT!=Vector3r(NaN,NaN,NaN)){
		Vector3r locDivT=n->ori*divT;
		// considered stress value (current or next one?)
		Matrix3r T=dta.T+(useNextT?Matrix3r(dta.Tdot*scene->dt):Matrix3r::Zero());
		Matrix3r trsf(n->ori);
		Matrix3r locT=trsf*T*trsf.transpose();
		for(int i=0;i<3;i++){
			if(isnan(dta.fixedDivT[i])) continue;
			// sum stress components in the direction of the prescribed value (local axis i)
			// mechanical meaning: internal stress in that direction
			Real locAxT=locT.row(i).sum(); // locT(0,i)+locT(1,i)+locT(2,i);
			// locDivT is actually stress residual
			// we make it correspond to difference between current and prescribed (internal) stress
			// the term is only added, since there might be divergence from other sources (??)
			locDivT[i]=locAxT-dta.fixedDivT[i];
			//cerr<<"Nid "<<dta.nid<<", divT["<<i<<"] "<<locAxT<<"(should be "<<dta.fixedDivT[i]<<") adds locDivT "<<locDivT[i]<<endl;
		}
		//cerr<<"@@@ Nid "<<dta.nid<<", divT "<<divT.transpose()<<" -> "<<n->ori.conjugate()*locDivT<<endl; // <<", fixedDivT="<<dta.fixedDivT.transpose()<<", T=\n"<<T<<"\nlocT=\n"<<locT<<endl;
		divT=n->ori.conjugate()*locDivT;
	}
}

VectorXr StaticEquilibriumSolver::computeInitialVelocities() const {
	VectorXr ret; ret.setZero(field->nodes.size()*3);
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		const SparcData& dta(n->getData<SparcData>());
		// fixedV and zeroed are in local coords
		Vector3r zeroed; for(int i=0;i<3;i++) zeroed[i]=isnan(dta.fixedV[i])?0:dta.fixedV[i];
		ret.segment<3>(3*dta.nid)=n->ori.conjugate()*zeroed; 
	}
	return ret;
};

void StaticEquilibriumSolver::integrateSolution() const {
	const Real& dt=scene->dt;
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		// this applies kinematic constraints directly (the solver might have some error in satisfying them)
		applyKinematicConstraints(n,/*allow prescribed divT*/true); 
		dta.e+=dt*(1+dta.e)*dta.gradV.trace(); // gradV.trace()==D.trace()
		dta.rho+=dt*(-dta.rho*dta.gradV.trace()); // rho is not really used for the implicit solution, but can be useful
		dta.T+=dt*dta.Tdot;
		n->pos+=dt*dta.v;
	}
};


void StaticEquilibriumSolver::dumpUnbalanced(){
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		Matrix3r Tnew=dta.T+scene->dt*dta.Tdot;
		cerr<<(boost::format("%d: vel=%s, Tnew=%s, divT=%s\n")%dta.nid%dta.v.transpose()%tensor_toVoigt(Tnew).transpose()%dta.divT.transpose()).str();
	}
}

VectorXr StaticEquilibriumSolver::trySolution(const VectorXr& vv){
	mff=static_cast<SparcField*>(field.get());
	mff->updateLocator();
	renumberNodes();
	FOREACH(const shared_ptr<Node>& n, field->nodes) findNeighbors(n); 

	if(vv.size()!=(int)field->nodes.size()*3) yade::ValueError("len(vv) must be 3*number of nodes");
	StressDivergenceFunctor functor(vv.size(),vv.size(),this);
	VectorXr divT(vv.size());
	functor(vv,divT);
	return divT;
};

template<typename Solver>
string solverStatus2str(int status);

template<> string solverStatus2str<Eigen::HybridNonLinearSolver<StressDivergenceFunctor> >(int status){
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
	mff=static_cast<SparcField*>(field.get());
	if(mff->locDirty || (scene->step%neighborUpdate)==0) mff->updateLocator();

	renumberNodes();  // not necessary at every step...
	// assume neighbors don't change during one step
	FOREACH(const shared_ptr<Node>& n, field->nodes) findNeighbors(n); 
	// intial solution is zero velocities, except where they are prescribed
	VectorXr vv=computeInitialVelocities();
	// functor used by solver, and solver itself
	StressDivergenceFunctor functor(vv.size(),vv.size(),this);
	Eigen::HybridNonLinearSolver<StressDivergenceFunctor> solver(functor);
	solver.parameters.factor=solverFactor;
	solver.parameters.maxfev=maxfev;

	// solution loop
	int status;
	int nFactorLowered=0;
	status=solver.solveNumericalDiffInit(vv);
	if(status==Eigen::HybridNonLinearSolverSpace::ImproperInputParameters) throw std::runtime_error("StaticEquilibriumSolver:: improper input parameters for the solver.");
	while(true){
		status=solver.solveNumericalDiffOneStep(vv);
		if(status==Eigen::HybridNonLinearSolverSpace::Running) continue; // good
		// dumpUnbalanced();
		if(status==Eigen::HybridNonLinearSolverSpace::RelativeErrorTooSmall) break; // done
		if(status==Eigen::HybridNonLinearSolverSpace::NotMakingProgressIterations || status==Eigen::HybridNonLinearSolverSpace::NotMakingProgressJacobian){
			solver.parameters.factor*=.2;
			if(++nFactorLowered<20){ LOG_WARN("Step "<<scene->step<<": solver did not converge, lowering factor to "<<solver.parameters.factor); continue; }
			LOG_WARN("Step "<<scene->step<<": solver not converging, but factor already lowered too many times; giving up.");
		}
		break;
	}
	if(status!=Eigen::HybridNonLinearSolverSpace::RelativeErrorTooSmall){
		throw std::runtime_error((boost::format("Solver did not find acceptable solution, returned %s (%d)")%solverStatus2str<decltype(solver)>(status)%status).str());
	}
	#ifdef SPARC_INSPECT
		solverDivT=solver.fvec;
	#endif
	residuum=solver.fnorm;
	integrateSolution();
};


#endif
