#ifdef YADE_VTK
#include<yade/pkg/sparc/SparcField.hpp>

YADE_PLUGIN(sparc,(SparcField)(SparcData)(ExplicitNodeIntegrator));

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

// http://listengine.tuxfamily.org/lists.tuxfamily.org/eigen/2010/01/msg00173.html
//template<typename Scalar>
bool MatrixXr_pseudoInverse(const MatrixXr &a, MatrixXr &a_pinv, double epsilon=std::numeric_limits<MatrixXr::Scalar>::epsilon()){
	// see : http://en.wikipedia.org/wiki/Moore-Penrose_pseudoinverse#The_general_case_and_the_SVD_method
	if ( a.rows()<a.cols() ) return false;

	// SVD
	Eigen::SVD<MatrixXr> svdA(a); MatrixXr vSingular = svdA.singularValues();

	// Build a diagonal matrix with the Inverted Singular values
	// The pseudo inverted singular matrix is easy to compute :
	// is formed by replacing every nonzero entry by its reciprocal (inversing).
	VectorXr vPseudoInvertedSingular(svdA.matrixV().cols(),1);
		
	for (int iRow =0; iRow<vSingular.rows(); iRow++){
		if(fabs(vSingular(iRow))<=epsilon) vPseudoInvertedSingular(iRow,0)=0.;
	   else vPseudoInvertedSingular(iRow,0)=1./vSingular(iRow);
	}

	// A little optimization here 
	MatrixXr mAdjointU = svdA.matrixU().adjoint().block(0,0,vSingular.rows(),svdA.matrixU().adjoint().cols());

	// Pseudo-Inversion : V * S * U'
	a_pinv = (svdA.matrixV() *  vPseudoInvertedSingular.asDiagonal()) * mAdjointU;

	return true;
}



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
	if(self && j>numIds-1){ throw std::runtime_error(("SparcField::nodesAround asked to remove (central?) point form the result, but it was not there (node pos="+lexical_cast<string>(self->pos)+", search point="+lexical_cast<string>(pt)+")").c_str()); }
	_ids->Delete();
	return ret;
};

void ExplicitNodeIntegrator::updateNeighbours(const shared_ptr<Node>& n){
	SparcData& dta(n->getData<SparcData>());
	dta.neighbors=mff->nodesAround(n->pos,/*count*/-1,rSearch,/*self=*/n);
	int sz(dta.neighbors.size());
	if(sz<3) throw std::runtime_error("Node at "+lexical_cast<string>(n->pos.transpose())+" has less than 3 neighbours.");
	MatrixXr relPos(sz,3);
	for(int i=0; i<sz; i++) relPos.row(i)=VectorXr(dta.neighbors[i]->pos-n->pos);
	bool succ=MatrixXr_pseudoInverse(relPos,/*result*/dta.relPosInv);
	if(!succ){
		cerr<<"====================================\nRelative positions matrix to be pseudo-inverted:\n"<<relPos<<"\n===================================\n";
		throw std::runtime_error("ExplicitNodeIntegrator::updateNeigbours: pseudoInverse failed.");
	}
}

Vector3r ExplicitNodeIntegrator::computeDivT(const shared_ptr<Node>& n){
	int vIx[][2]={{0,0},{1,1},{2,2},{1,2},{2,0},{0,1}}; // from voigt index to matrix indices
	const SparcData& midDta=n->getData<SparcData>();
	const vector<shared_ptr<Node> >& NN(midDta.neighbors);  size_t sz=NN.size();
	Vector3r A[6];
	for(int l=0; l<6; l++){
		int i=vIx[l][0], j=vIx[l][1];
		VectorXr rhs(sz); for(size_t i=0; i<sz; i++) rhs[i]=NN[i]->getData<SparcData>().T(i,j)-midDta.T(i,j);
		A[i]=midDta.relPosInv*rhs;
	};
	int vIx33[3][3]={{0,5,4},{5,1,3},{4,3,2}}; // from matrix indices to voigt
	return Vector3r(
		A[vIx33[0][0]][0]+A[vIx33[0][1]][1]+A[vIx33[0][2]][2],
		A[vIx33[1][0]][0]+A[vIx33[1][1]][1]+A[vIx33[1][2]][2],
		A[vIx33[2][0]][0]+A[vIx33[2][1]][1]+A[vIx33[2][2]][2]
	);
}

Matrix3r ExplicitNodeIntegrator::computeGradV(const shared_ptr<Node>& n){
	const SparcData& midDta=n->getData<SparcData>();
	const vector<shared_ptr<Node> >& NN(midDta.neighbors); size_t sz=NN.size();
	MatrixXr relVels(sz,3); for(size_t i=0; i<sz; i++) relVels.row(i)=VectorXr(NN[i]->getData<SparcData>().v-midDta.v);
	Matrix3r ret;
	for(int l=0; l<3; l++) ret.col(l)=midDta.relPosInv*relVels.col(l);
	return ret;
}

Matrix3r ExplicitNodeIntegrator::stressRate(const Matrix3r& T, const Matrix3r& D){
	// linear elastic, doesn't depend on current T
	return voigt_toSymmTensor((C*tensor_toVoigt(D,/*strain*/true)).eval());
};

void ExplicitNodeIntegrator::run(){
	mff=static_cast<SparcField*>(field.get());
	// if(mff->locDirty) throw std::runtime_error("SparcField locator was not updated to new positions.");
	if(mff->locDirty) mff->updateLocator();
	const Real& dt=scene->dt;
	// update stiffness matrix
	C<<(Matrix3r()<<1,-nu,-nu, -nu,1,-nu, -nu,-nu,1).finished(),Matrix3r::Zero(),Matrix3r::Zero(),Vector3r(2*(1+nu),2*(1+nu),2*(1+nu)).asDiagonal();
	C*=E/((1+nu)*(1-2*nu));
	/*	
	loop 1:
		0. update neighbour lists (used for divT and velGrad)
		1. compute divT (needs old positions) 
		2. compute accel, velocity
	loop 2:
		1. compute velGrad (needs old position, new velocity)
	loop 3:
		1. update position
		2. update density
		3. update stress
	*/

	// loop 1
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		updateNeighbours(n);
		Vector3r divT(computeDivT(n));
		Vector3r accel=(1/dta.rho)*divT;
		dta.v+=dt*accel;
	}
	// loop 2
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		n->getData<SparcData>().gradV=computeGradV(n);
	};
	// loop 3
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		SparcData& dta(n->getData<SparcData>());
		size_t i=0; FOREACH(const Vector3r& dir, dta.dirs){
			dta.v-=dir*dir.dot(dta.v); // subtract component along that vector
			dta.v+=dir*(dta.dirVels.size()>i?dta.dirVels[i]:0); // set velocity in that sense
			i++;
		}
		n->pos+=dt*dta.v;
		Matrix3r D=(dta.gradV+dta.gradV.transpose())/2, W=(dta.gradV-dta.gradV.transpose())/2;
		dta.rho+=dt*(-dta.rho*dta.gradV.trace()); // L.trace == div(v)
		dta.T+=dt*(stressRate(dta.T,D)+W*dta.T-dta.T*W);
	}
	mff->locDirty=true;
};

template<> SparcData& Node::getData<SparcData>(){ return getData(Node::ST_SPARC)->cast<SparcData>(); }
template<> void Node::setData<SparcData>(const shared_ptr<SparcData>& d){ return setData(d,Node::ST_SPARC); }
template<> bool Node::hasData<SparcData>(){ return hasData(Node::ST_SPARC); }

shared_ptr<NodeData> SparcData::pyGetOnNode(const shared_ptr<Node>& n){
	if(!n->hasData<SparcData>()) return shared_ptr<SparcData>(); // return None
	return n->getData(Node::ST_SPARC);
}
void SparcData::pySetOnNode(const shared_ptr<Node>& n, const shared_ptr<SparcData>& d){ n->setData<SparcData>(d); }



#endif
