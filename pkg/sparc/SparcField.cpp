#ifdef YADE_VTK
#include<yade/pkg/sparc/SparcField.hpp>

#include<boost/preprocessor.hpp>

#include<unsupported/Eigen/NonLinearOptimization>


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
	#if 0
		// update nids
		dta.neighborNids.resize(dta.neighbors);
		int i=0;
		FOREACH(const shared_ptr<Node>& nn, dta.neighbors) n->neighborNids[i++]=nn->getData<SparcData>().nid;
	#endif
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
		// cerr<<(NN[i]->getData<SparcData>().v-midDta.v).transpose()<<" || "<<relVels.row(i)<<" $$ "<<NN[i]->getData<SparcData>().v.transpose()<<" ## "<<midDta.v.transpose()<<endl;
	}
	#ifdef SPARC_INSPECT
		n->getData<SparcData>().relVels=relVels;
	#endif
	return midDta.relPosInv*relVels;
}

Matrix3r ExplicitNodeIntegrator::computeStressRate(const Matrix3r& T, const Matrix3r& D) const {
	// linear elastic, doesn't depend on current T
	return voigt_toSymmTensor((C*tensor_toVoigt(D,/*strain*/true)).eval());
};

void ExplicitNodeIntegrator::applyKinematicConstraints(const shared_ptr<Node>& n) const {
	// apply kinematic constraints (prescribed velocities)
	SparcData& dta(n->getData<SparcData>());
	size_t i=-1;
	FOREACH(const Vector3r& dir, dta.dirs){
		i++;
		#if 1
			Real val=dta.getDirVel(i);
			if(dir==Vector3r::UnitX()){ dta.v[0]=val; continue; }
			else if(dir==Vector3r::UnitY()){ dta.v[1]=val; continue; }
			else if(dir==Vector3r::UnitZ()){ dta.v[2]=val; continue; }
			else 
		#endif
		{ // numerically perhaps unstable?
			throw std::runtime_error("Only axis-aligned prescribed velocities are reliably supported now.");
			dta.v-=dir*dir.dot(dta.v); // subtract component along that vector
			dta.v+=dir*dta.getDirVel(i); // set velocity in that sense
		}
	}
};


void ExplicitNodeIntegrator::postLoad(ExplicitNodeIntegrator&){
	// update stiffness matrix
	C<<(Matrix3r()<<1-nu,nu,nu, nu,1-nu,nu, nu,nu,1-nu).finished(),Matrix3r::Zero(),Matrix3r::Zero(),Matrix3r(((1-2*nu)/2.)*Matrix3r::Identity());
	C*=E/((1+nu)*(1-2*nu));
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
		2. from old T, compute divT, acceleration
	loop 2: (updates state variables, and does not
		1. update x (from prev x, prev v)
		2. update T (from prev T, prev Tdot)
		3. update v (from prev v, prev accel, constraints)
		4. update rho (from prev rho and prev L)
	*/

	if(mff->locDirty || (neighborUpdate%scene->step)==0) mff->updateLocator(); // if(mff->locDirty) throw std::runtime_error("SparcField locator was not updated to new positions.");

	FOREACH(const shared_ptr<Node>& n, field->nodes){
		nid++; SparcData& dta(n->getData<SparcData>());
		findNeighbors(n);
		updateNeighborsRelPos(n);
		dta.gradV=computeGradV(n);
		Matrix3r D(dta.getD()), W(dta.getW()); // symm/antisymm decomposition
		Matrix3r Tcirc=computeStressRate(dta.T,D);
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
		applyKinematicConstraints(n);
		dta.rho+=dt*(-dta.rho*dta.gradV.trace());
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
		// copy velocity to inside node data (could be eliminated later to be more effective)
		ses->copyVelocityToNodes(v);
		FOREACH(const shared_ptr<Node>& n, ses->field->nodes){
			SparcData& dta(n->getData<SparcData>());
			ses->updateNeighborsRelPos(n,/*useNext*/true);
			dta.gradV=ses->computeGradV(n);
			Matrix3r D(dta.getD()), W(dta.getW()); // symm/antisymm decomposition
			Matrix3r Tcirc=ses->computeStressRate(dta.T,D);
			dta.Tdot=Tcirc+dta.T*W-W*dta.T;
			Vector3r nodeDivT=ses->computeDivT(n,/*useNext*/true);
			ses->applyConstraintReaction(n,nodeDivT);
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
void StaticEquilibriumSolver::applyConstraintReaction(const shared_ptr<Node>& n, Vector3r& divT) const {
	SparcData& dta(n->getData<SparcData>());
	if(dta.dirs.empty()) return;
	// in the sense of prescribed velocity, set divT to be proportional to the difference between current and prescribed velocity
	//cerr<<"Nid "<<dta.nid<<", solver v = "<<dta.v<<endl;
	int i=-1; FOREACH(const Vector3r& dir, dta.dirs){
		i++;
		Real dv=dta.v.dot(dir)-dta.getDirVel(i);
		divT=divT/*set to zero in that sense first */ -dir*divT.dot(dir) /*assign value*/ +dir*dv*scene->dt*supportStiffness;
		//cerr<<"Nid "<<dta.nid<<" prescribed v="<<dta.getDirVel(i)<<" along "<<dir<<"; solver v="<<dta.v.dot(dir)<<" divT -> "<<divT<<" ("<<divT.dot(dir)<<endl;
		// if(dta.nid==26) cerr<<"Nid "<<dta.nid<<": v="<<dta.v<<", prescribed "<<dta.getDirVel(i)<<" along "<<dir<<", dv="<<dv<<endl;
	}
};

VectorXr StaticEquilibriumSolver::computeInitialVelocities() const {
	VectorXr ret; ret.setZero(field->nodes.size()*3);
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		const SparcData& dta(n->getData<SparcData>());
		if(dta.dirs.empty()) continue;
		size_t sz=dta.dirs.size();	for(size_t i=0; i<sz; i++) ret.segment<3>(3*dta.nid)+=dta.dirs[i]*dta.getDirVel(i);
	}
	return ret;
};

void StaticEquilibriumSolver::integrateSolution() const {
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		applyKinematicConstraints(n); // be more precise than the solver itself
		dta.T+=scene->dt*dta.Tdot;
		n->pos+=scene->dt*dta.v;
	}
};


void StaticEquilibriumSolver::dumpUnbalanced(){
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		cerr<<(boost::format("%d: vel=%s, Tdot=%g %g %g, %g %g %g, divT=%s\n")%dta.nid%dta.v.transpose()%dta.Tdot(0,0)%dta.Tdot(1,1)%dta.Tdot(2,2)%dta.Tdot(1,2)%dta.Tdot(2,0)%dta.Tdot(0,1)%dta.divT.transpose()).str();
	}
}

VectorXr StaticEquilibriumSolver::trySolution(const VectorXr& vv){
	mff=static_cast<SparcField*>(field.get());
	mff->updateLocator();
	renumberNodes();
	FOREACH(const shared_ptr<Node>& n, field->nodes) findNeighbors(n); 

	if(vv.size()!=field->nodes.size()*3) yade::ValueError("len(vv) must be 3*number of nodes");
	StressDivergenceFunctor functor(vv.size(),vv.size(),this);
	VectorXr divT(vv.size());
	int status=functor(vv,divT);
	return divT;
};

void StaticEquilibriumSolver::run(){
	mff=static_cast<SparcField*>(field.get());
	if(mff->locDirty || (neighborUpdate%scene->step)==0) mff->updateLocator();

	renumberNodes();  // not necessary at every step...
	// assume neighbors don't change during one step
	FOREACH(const shared_ptr<Node>& n, field->nodes) findNeighbors(n); 
	// intial solution is zero velocities, except where they are prescribed
	VectorXr vv=computeInitialVelocities();
	// functor used by solver, and solver itself
	StressDivergenceFunctor functor(vv.size(),vv.size(),this);
	Eigen::HybridNonLinearSolver<StressDivergenceFunctor> solver(functor);

	// solution loop
	int status;
	solver.parameters.maxfev=100000;
	solver.parameters.factor=.1;
	status=solver.solveNumericalDiffInit(vv);
	if(status==Eigen::HybridNonLinearSolverSpace::ImproperInputParameters) yade::RuntimeError("StaticEquilibriumSolver:: improper input parameters for the solver.");
	dumpUnbalanced();
	int step=0;
	while(status==Eigen::HybridNonLinearSolverSpace::Running || step<100){
		status=solver.solveNumericalDiffOneStep(vv);
		dumpUnbalanced();
	}
	// cerr<<"StaticEquilibriumSolver: status "<<status<<endl;
	#ifdef SPARC_INSPECT
		solverDivT=solver.fvec;
	#endif
	integrateSolution();
};


#endif
