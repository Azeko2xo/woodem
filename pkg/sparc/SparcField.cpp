#ifdef WOO_SPARC

#ifdef WOO_VTK
#include<woo/pkg/sparc/SparcField.hpp>

#include<boost/preprocessor.hpp>
#include<boost/filesystem/convenience.hpp>

namespace bfs=boost::filesystem;


using boost::format;

WOO_PLUGIN(sparc,(SparcField)(SparcData)(ExplicitNodeIntegrator)(StaticEquilibriumSolver));

WOO_IMPL_LOGGER(SparcField);
WOO_IMPL_LOGGER(ExplicitNodeIntegrator);

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

vector<shared_ptr<Node>> SparcField::nodesAround(const Vector3r& pt, int count, Real radius, const shared_ptr<Node>& self, Real* relLocPtDens, const Vector2i& mnMxPts){
	if(!locator) throw runtime_error("SparcField::locator not initialized!");	
	if((radius<=0 && count<=0) || (radius>0 && count>0)) throw std::invalid_argument("SparcField.nodesAround: exactly one of radius or count must be positive!");
	vtkIdList* _ids=vtkIdList::New(); vtkIdList& ids(*_ids);;
	// get given number of closest points
	if(count>0){ locator->FindClosestNPoints(count,(const double*)(&pt),&ids); }
	// search within given radius
	else {
		if(!relLocPtDens) locator->FindPointsWithinRadius(radius,(const double*)(&pt),&ids);
		else{
			// 10 attempts to adjust relLocPtDens, if given
			Real fallback=-1; bool forceNextDone=false;
			for(int i=0; i<neighAdjSteps; i++){
				// last step?
				if(i==(neighAdjSteps-1)){
					forceNextDone=true;
					// don't experiment in the last step and use whatever worked already, if available
					if(fallback>0) (*relLocPtDens)=fallback;
				}
				locator->FindPointsWithinRadius(radius*(*relLocPtDens),(const double*)(&pt),&ids);
				int n=ids.GetNumberOfIds();
				LOG_DEBUG("Found "<<n<<" neighbors with relLocPtDens="<<(*relLocPtDens));
				// we don't want to look any furher: either return what is OK or throw an exception
				if(forceNextDone){
					if(n<mnMxPts[0]) throw std::runtime_error("Unable to find suitable number of neighbors (at least "+to_string(mnMxPts[0])+", less than "+to_string(mnMxPts[1])+" if possible) within "+to_string(neighAdjSteps)+". Check neighAdjSteps and neighAdjFact values.");
					break;
				}
				// store last number in case we got enough points (perhaps too many, but good as fallback)
				if(n>mnMxPts[0]) fallback=(*relLocPtDens); 
				if(n<mnMxPts[0]){ // too little points
					if(fallback>0){
						// go with what was previous, and use that no matter what
						(*relLocPtDens)=fallback; forceNextDone=true;
						LOG_TRACE("   "<<n<<" points not enough ("<<mnMxPts[0]<<" required), forcing use of previous relLocPtDens="<<fallback);
					} 
					else{
						(*relLocPtDens)*=neighAdjFact[1]; // increase distance
						LOG_TRACE("   "<<n<<" points not enough ("<<mnMxPts[0]<<" required), next try with relLocPtDens="<<*relLocPtDens);
					}
				}
				if(n>mnMxPts[1]){
					*relLocPtDens*=neighAdjFact[0];
					LOG_TRACE("   "<<n<<" points is too much (max "<<mnMxPts[1]<<" advised), next try with relLocPtDense="<<*relLocPtDens);
				}
			}
		}
	}
	int numIds=ids.GetNumberOfIds();
	vector<shared_ptr<Node>> ret; ret.reserve(numIds);
	bool selfSeen=false;
	LOG_DEBUG(numIds<<" nodes around "<<pt.transpose()<<" (radius="<<radius<<", count="<<count<<")");
	if(self) LOG_DEBUG("   self is nid "<<self->getData<SparcData>().nid<<" at "<<self->pos.transpose());
	for(int i=0; i<numIds; i++){
		if(nodes[ids.GetId(i)]){ LOG_TRACE("Nid "<<ids.GetId(i)<<", at "<<nodes[ids.GetId(i)]->pos.transpose());}
		else{ LOG_TRACE("Nid "<<ids.GetId(i)<<" is None?!");}
		ret.push_back(nodes[ids.GetId(i)]);
		if(self && nodes[ids.GetId(i)].get()==self.get()) selfSeen=true;
	};
	if(self && !selfSeen) throw std::runtime_error("SparcFields::nodesAround: central node at "+lexical_cast<string>(self->pos.transpose())+" given but not found within neighbors around "+lexical_cast<string>(pt.transpose())+".");
	_ids->Delete();
	return ret;
};

template<bool useNext>
void ExplicitNodeIntegrator::findNeighbors(const shared_ptr<Node>&n) const {
	SparcData& dta(n->getData<SparcData>());
	// don't search for neighbors in 0d (useless)
	if(dim==0){ dta.neighbors.clear(); return; }
	size_t prevN=dta.neighbors.size();
	Vector3r pos(!useNext?n->pos:n->pos+dta.v*scene->dt);
	// maximum number of tries to get satisfactory number of points
	dta.neighbors=mff->nodesAround(pos,/*count*/-1,rSearch,/*self=*/n,/*relLocPtDensity*/&dta.relLocPtDensity,/*mnMxPt*/Vector2i(wlsPhi.size(),mff->neighRelMax*wlsPhi.size()));
	LOG_DEBUG("Nid "<<dta.nid<<" has "<<dta.neighbors.size()<<" neighbors");
	// say if number of neighbors changes between steps
	if(!useNext && prevN>0 && dta.neighbors.size()!=prevN){
		LOG_INFO("Nid "<<dta.nid<<" changed number of neighbors: "<<prevN<<" → "<<dta.neighbors.size());
	}
};

Real ExplicitNodeIntegrator::pointWeight(Real distSq, Real relLocPtDensity) const {
	assert(relLocPtDensity>0);
	switch(weightFunc){
		case WEIGHT_DIST: {
			Real w=(rPow%2==0?pow(distSq,rPow/2):pow(sqrt(distSq),rPow));
			assert(!(rPow==0 && w!=1.)); // rPow==0 → weight==1.
			return w;
		}
		case WEIGHT_GAUSS: {
			//Real w=pow(sqrt(distSq),-1); // make all points the same weight first
			Real hSq=pow(relLocPtDensity*rSearch,2);
			//assert(distSq<=hSq); // taken care of by neighbor search algorithm already
			if(distSq>hSq) LOG_WARN("distSq="<<distSq<<">hSq="<<hSq<<" ?: returning 0");
			return exp(-gaussAlpha*(distSq/hSq));
		}
		default:
			throw std::logic_error("ExplicitNodeIntegrator.weightFunc has inadmissible value; this should have been trapped in postLoad already!");
	};
}

template<bool useNext>
void ExplicitNodeIntegrator::updateLocalInterp(const shared_ptr<Node>& n) const {
	SparcData& dta(n->getData<SparcData>());
	if(dim==0){ // no interpolation at all, handle specially
		#ifdef SPARC_INSPECT
			dta.relPos.resize(0,0); dta.nextRelPos.resize(0,0);
			dta.stencil.resize(0,0); dta.bVec.resize(0,0); dta.weightSq.resize(0,0);
		#endif
		dta.dxDyDz.resize(0,0);
		return;
	}
	// current or next inputs
	int sz(dta.neighbors.size());
	MatrixXr relPos(sz,3);
	VectorXr weightSq(sz);
	Vector3r midPos(n->pos+(useNext?Vector3r(scene->dt*dta.v):Vector3r::Zero()));
	for(int i=0; i<sz; i++){
		if(!dta.neighbors[i]) throw std::runtime_error("Nid "+to_string(dta.nid)+", neighbor #"+to_string(i)+" is None?!");
		Vector3r dX=VectorXr(dta.neighbors[i]->pos+(useNext?Vector3r(scene->dt*dta.neighbors[i]->getData<SparcData>().v):Vector3r::Zero())-midPos);
		weightSq[i]=pow(pointWeight(dX.squaredNorm(),dta.relLocPtDensity),2);
		relPos.row(i)=dX;
		for(int d=dim; d<3; d++){
			if(dX[d]!=0) throw std::runtime_error(to_string(d)+"-component of internodal dist ("+to_string(dta.nid)+"-"+to_string(dta.neighbors[i]->getData<SparcData>().nid)+") is nonzero ("+to_string(dX[d])+", but the basis does not span this component.");
		}
	}
	#ifdef SPARC_INSPECT
		if(useNext) dta.relPos=relPos;
		else dta.nextRelPos=relPos;
	#endif
	if(eig_isnan(relPos)){
		cerr<<"=== Relative positions for nid "<<dta.nid<<":\n"<<relPos<<endl;
		throw std::runtime_error("NaN's in relative positions in O.sparc.nodes["+lexical_cast<string>(dta.nid)+"].sparc."+string(useNext?"nextRelPos":"relPos")+") .");
	}
	if(relPos.rows()<(int)wlsPhi.size()) throw std::runtime_error((format("Node #%d at %s has only %d neighbors (%d is minimum, including itself)")%dta.nid%n->pos.transpose()%sz%(wlsPhi.size()-1)).str());
	// evaluate basis functions (in wlsPhi) at all points
	MatrixXr Phi(sz,wlsPhi.size());
	// evaluate all basis funcs in all points
	for(int pt=0; pt<sz; pt++) for(size_t i=0; i<wlsPhi.size(); i++) Phi(pt,i)=wlsPhi[i](relPos.row(pt));
	// compute the stencil matrix
	auto W2=weightSq.asDiagonal();
	MatrixXr stencil=(Phi.transpose()*W2*Phi).inverse()*Phi.transpose()*W2;
	// basis derivatives matrix, in (0,0,0) as relative position to the central point
	MatrixXr bVec;
	// no d/dz in 2d
	switch(dim){
		case 3:{
			bVec=MatrixXr(wlsPhi.size(),4);
			for(size_t i=0; i<wlsPhi.size(); i++) bVec.row(i)<<wlsPhi[i](Vector3r::Zero()),wlsPhiDx[i](Vector3r::Zero()),wlsPhiDy[i](Vector3r::Zero()),wlsPhiDz[i](Vector3r::Zero());
			dta.dxDyDz=bVec.rightCols<3>().transpose()*stencil;
			break;
		}
		case 2:{
			bVec=MatrixXr(wlsPhi.size(),3);
			for(size_t i=0; i<wlsPhi.size(); i++) bVec.row(i)<<wlsPhi[i](Vector3r::Zero()),wlsPhiDx[i](Vector3r::Zero()),wlsPhiDy[i](Vector3r::Zero());
			dta.dxDyDz=bVec.rightCols<2>().transpose()*stencil;
			break;
		}
		case 1:{
			bVec=MatrixXr(wlsPhi.size(),2);
			for(size_t i=0; i<wlsPhi.size(); i++) bVec.row(i)<<wlsPhi[i](Vector3r::Zero()),wlsPhiDx[i](Vector3r::Zero());
			dta.dxDyDz=bVec.rightCols<1>().transpose()*stencil;
			break;
		}
	}
	LOG_DEBUG("stencil is "<<stencil.rows()<<"×"<<stencil.cols()<<", bVec is "<<bVec.rows()<<"×"<<bVec.cols()<<", dxDyDz is "<<dta.dxDyDz.rows()<<"×"<<dta.dxDyDz.cols());
	#ifdef SPARC_INSPECT
		dta.weightSq=weightSq; dta.stencil=stencil; dta.bVec=bVec;
	#endif
}

void ExplicitNodeIntegrator::setWlsBasisFuncs(){
	wlsPhi.clear(); wlsPhiDx.clear(); wlsPhiDy.clear(); wlsPhiDz.clear();
	switch(wlsBasis){
	#define _BASIS_FUNC(r,cont,expr) cont.push_back([](const Vector3r& x)->Real{ return expr; });
		case WLS_EMPTY:{
			LOG_INFO("Setting empty (0d) basis");
			dim=0;
			break;
		}
		case WLS_QUAD_X:{
			LOG_INFO("Setting 2nd-order monomial basis in X");
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhi,(1)(x[0])(x[0]*x[0]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDx,(0)(1)(2*x[0]));
			dim=1;
			assert(wlsPhi.size()==3);
			break;
		}
		case WLS_CUBIC_X:{
			LOG_INFO("Setting 3rd-order monomial basis in X");
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhi,(1)(x[0])(x[0]*x[0])(x[0]*x[0]*x[0]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDx,(0)(1)(2*x[0])(3*x[0]*x[0]));
			dim=1;
			assert(wlsPhi.size()==4);
			break;
		}
		case WLS_LIN_XY:{
			LOG_INFO("Setting 1st-order monomial basis in XY");
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhi,  (1)(x[0])(x[1]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDx,(0)(1)(0));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDy,(0)(0)(1));
			dim=2;
			assert(wlsPhi.size()==3);
			break;
		}
		case WLS_QUAD_XY:{
			LOG_INFO("Setting 2nd-order monomial basis in XY");
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhi,  (1)(x[0])(x[1])(x[0]*x[0])(x[1]*x[1])(x[0]*x[1]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDx,(0)(1)(0)(2*x[0])(0)(x[1]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDy,(0)(0)(1)(0)(2*x[1])(x[0]));
			dim=2;
			assert(wlsPhi.size()==6);
			break;
		}
		case WLS_LIN_XYZ:{
			LOG_INFO("Setting 1st-order monomial basis in XYZ");
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhi,  (1)(x[0])(x[1])(x[2]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDx,(0)(1)(0)(0));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDy,(0)(0)(1)(0));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDz,(0)(0)(0)(1));
			dim=3;
			assert(wlsPhi.size()==4);
			break;
		}
		case WLS_QUAD_XYZ:{
			LOG_INFO("Setting 2nd-order monomial basis in XYZ");
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhi,(1)(x[0])(x[1])(x[2]) (x[0]*x[0])(x[1]*x[1])(x[2]*x[2]) (x[1]*x[2])(x[2]*x[0])(x[0]*x[1]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDx,(0)(1)(0)(0) (2*x[0])(0)(0) (0)(x[2])(x[1]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDy,(0)(0)(1)(0) (0)(2*x[1])(0) (x[2])(0)(x[0]));
			BOOST_PP_SEQ_FOR_EACH(_BASIS_FUNC,wlsPhiDz,(0)(0)(0)(1) (0)(0)(2*x[2]) (x[1])(x[0])(0));
			dim=3;
			assert(wlsPhi.size()==10);
			break;
		}
		#undef _BASIS_FUNC
		default: throw std::runtime_error("ExplicitNodeIntegrator.wlsBasis has an unknown value of "+to_string(wlsBasis));
	};
	assert(wlsPhi.size()==wlsPhiDx.size());
	if(dim>1) assert(wlsPhi.size()==wlsPhiDy.size());
	if(dim>2) assert(wlsPhi.size()==wlsPhiDz.size());
}

template<bool useNext>
Vector3r ExplicitNodeIntegrator::computeDivT(const shared_ptr<Node>& n) const {
	const SparcData& dta=n->getData<SparcData>();
	switch(dim){
		case 3:{
			// matrix with stress values around the point, in Voigt notation as 6 values (for symmetric tensors)
			int vIx[][2]={{0,0},{1,1},{2,2},{1,2},{2,0},{0,1}};
			MatrixXr T(dta.neighbors.size(),6);
			for(size_t i=0; i<dta.neighbors.size(); i++){
				const Matrix3r& nT=(useNext?dta.neighbors[i]->getData<SparcData>().nextT:dta.neighbors[i]->getData<SparcData>().T);
				for(int j=0; j<6; j++) T(i,j)=nT(vIx[j][0],vIx[j][1]);
			}
			auto bxS=dta.dxDyDz.row(0), byS=dta.dxDyDz.row(1), bzS=dta.dxDyDz.row(2);
			return Vector3r(
				bxS.dot(T.col(0))+byS.dot(T.col(5))+bzS.dot(T.col(4)),
				bxS.dot(T.col(5))+byS.dot(T.col(1))+bzS.dot(T.col(3)),
				bxS.dot(T.col(4))+byS.dot(T.col(3))+bzS.dot(T.col(2))
			);
		}
		case 2:{
			MatrixXr T(dta.neighbors.size(),3);
			for(size_t i=0; i<dta.neighbors.size(); i++){
				const Matrix3r& nT=(useNext?dta.neighbors[i]->getData<SparcData>().nextT:dta.neighbors[i]->getData<SparcData>().T);
				T.row(i)=Vector3r(nT(0,0),nT(1,1),nT(0,1)); // T_22 is perhaps non-zero, but its div is 0
			}
			auto bxS=dta.dxDyDz.row(0), byS=dta.dxDyDz.row(1);
			return Vector3r(
				bxS.dot(T.col(0))+byS.dot(T.col(2)),
				bxS.dot(T.col(2))+byS.dot(T.col(1)),
				0 // equilibrium in the z-direction
			);
		}
		case 1:{
			VectorXr T(dta.neighbors.size());
			for(size_t i=0; i<dta.neighbors.size(); i++){
				const Matrix3r& nT=(useNext?dta.neighbors[i]->getData<SparcData>().nextT:dta.neighbors[i]->getData<SparcData>().T);
				T[i]=nT(0,0);
			}
			return Vector3r(dta.dxDyDz.row(0).dot(T),0,0);
		}
		case 0:{
			return Vector3r::Zero();
		}
		default: abort(); // gcc happy
	}
}
	

Matrix3r ExplicitNodeIntegrator::computeGradV(const shared_ptr<Node>& n) const {
	const SparcData& dta=n->getData<SparcData>();
	const vector<shared_ptr<Node> >& NN(dta.neighbors);
	switch(dim){
		case 3:{
			MatrixXr vel(NN.size(),3);
			for(size_t i=0; i<NN.size(); i++){ vel.row(i)=NN[i]->getData<SparcData>().v; }
			return dta.dxDyDz*vel;
		}
		case 2:{
			MatrixXr vel(NN.size(),2);
			for(size_t i=0; i<NN.size(); i++) vel.row(i)=NN[i]->getData<SparcData>().v.head<2>(); // x,y components only
			Matrix3r ret=Matrix3r::Zero();
			ret.topLeftCorner<2,2>()=dta.dxDyDz*vel;
			ret(2,2)=dta.Lout[2]; // out-of-space Lzz
			return ret;
		}
		case 1:{
			VectorXr vel(NN.size());
			for(size_t i=0; i<NN.size(); i++) vel[i]=NN[i]->getData<SparcData>().v[0];
			assert((dta.dxDyDz*vel).rows()==1 && (dta.dxDyDz*vel).cols()==1);
			return Vector3r((dta.dxDyDz*vel)(0,0),dta.Lout[1],dta.Lout[2]).asDiagonal();
		}
		case 0:{
			return dta.Lout.asDiagonal();
		}
		default: abort(); // gcc happy
	};
}

Matrix3r ExplicitNodeIntegrator::computeStressRate(const Matrix3r& inT, const Matrix3r& D, const Real e /* =-1 */) const {
	switch(matModel){
		case MAT_ELASTIC:
			// isotropic linear elastic
			return voigt_toSymmTensor((C*tensor_toVoigt(D,/*strain*/true)).eval());
		case MAT_BARODESY:{
			Matrix3r T(barodesyConvertPaToKPa?(inT/1e3).eval():inT);
			#define CC(i) barodesyC[i-1]
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
			#undef CC
			// cerr<<"CC(2)*D0=\n"<<(CC(2)*D0)<<"\n(CC(2)*D0).exp()=\n"<<(CC(2)*D0).exp()<<endl;
			Matrix3r Tcirc=h*(f*R0+g*T0)*Dnorm;
			return (barodesyConvertPaToKPa?(1e3*Tcirc).eval():Tcirc);
		}
		case MAT_VISCOUS: throw std::runtime_error("Viscous material not yet implemented.");
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


void ExplicitNodeIntegrator::postLoad(ExplicitNodeIntegrator&,void*){
	// update stiffness matrix
	C<<(Matrix3r()<<1-nu,nu,nu, nu,1-nu,nu, nu,nu,1-nu).finished(),Matrix3r::Zero(),Matrix3r::Zero(),Matrix3r(((1-2*nu)/2.)*Matrix3r::Identity());
	C*=E/((1+nu)*(1-2*nu));
	if(matModel<0 || matModel>MAT_SENTINEL) woo::ValueError(boost::format("matModel must be in range 0..%d")%(MAT_SENTINEL-1));
	if(weightFunc<0 || weightFunc>=WEIGHT_SENTINEL) woo::ValueError(boost::format("weightFunc must be in range 0..%d")%(WEIGHT_SENTINEL-1));
	if(barodesyC.size()!=6) woo::ValueError(boost::format("barodesyC must have exactly 6 values (%d given)")%barodesyC.size());
	if(rPow>0) LOG_WARN("Positive value of ExplicitNodeIntegrator.rPow: makes weight increasing with distance, ARE YOU NUTS?!");
	setWlsBasisFuncs();
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

#undef _CATCH_CRAP

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
	if(dim==0) woo::ValueError("0d space impossible with ExplicitNodeIntegrator, use StaticEquilibriumSolver).");
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

	for(const shared_ptr<Node>& n: field->nodes){
		nid++; SparcData& dta(n->getData<SparcData>());
		findNeighbors</*useNext*/false>(n);
		updateLocalInterp</*useNext*/false>(n);
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
		#ifdef WOO_DEBUG
			dta.catchCrap1(nid,n);
		#endif
	};

	nid=-1;
	for(const shared_ptr<Node>& n: field->nodes){
		nid++; SparcData& dta(n->getData<SparcData>());
		n->pos+=dt*dta.v;
		if(spinRot) n->pos=dta.getRotQ(scene->dt)*n->pos;
		dta.T=dta.nextT; // dt*dta.Tdot;
		dta.v+=dt*dta.accel;
		applyKinematicConstraints(n,/*disallow prescribed stress*/false);
		dta.rho+=dt*(-dta.rho*dta.gradV.trace());
		dta.e+=dt*(1+dta.e)*dta.gradV.trace(); // gradV.trace()==D.trace()
		#ifdef WOO_DEBUG
			dta.catchCrap2(nid,n);
		#endif
	};
};

WOO_IMPL_LOGGER(StaticEquilibriumSolver);

int StaticEquilibriumSolver::ResidualsFunctorBase::operator()(const VectorXr &v, VectorXr& resid) const {
	#ifdef SPARC_TRACE
		#define _INFO(s) "{ jacobian "<<s->fjac.cols()<<"x"<<s->fjac.rows()<<endl<<s->fjac<<endl<<"--- current diagonal is "<<s->diag.transpose()<<"}"<<endl
		if(ses->dbgFlags&DBG_JAC){
			switch(ses->solver){
				case SOLVER_POWELL: SPARC_TRACE_SES_OUT("// functor called"<<endl<<_INFO(ses->solverPowell)); break;
				case SOLVER_LM: SPARC_TRACE_SES_OUT("// functor called"<<endl<<_INFO(ses->solverLM)); break;
			}
		}
		#undef _INFO
	#endif
	if(eig_isnan(v)){
		LOG_ERROR("Solver proposing solution with NaN's, return [∞,…,∞]"); resid.setConstant(v.size(),std::numeric_limits<Real>::infinity());
		throw std::runtime_error("Solver proposing solution with NaN's.");
		SPARC_TRACE_SES_OUT("--- input NaN'ed v = "<<v.transpose()<<endl<<"--- !! returning "<<resid.transpose()<<endl);
		return 0;
	}
	#ifdef SPARC_TRACE
		SPARC_TRACE_SES_OUT("#-- input velocities = "<<v.transpose()<<endl);
	#endif

	ses->solutionPhase(v,resid);

	SPARC_TRACE_SES_OUT("### errors |"<<lexical_cast<string>(resid.blueNorm())<<"| = "<<resid.transpose()<<endl);
	return 0;
};


void StaticEquilibriumSolver::copyLocalVelocityToNodes(const VectorXr &v) const{
	assert(v.size()==nDofs);
	#ifdef WOO_DEBUG
		if(eig_isnan(v)) throw std::runtime_error("Solver proposing nan's in velocities, vv = "+lexical_cast<string>(v.transpose()));
	#endif
	for(const shared_ptr<Node>& n: field->nodes){
		SparcData& dta=n->getData<SparcData>();
		for(int ax:{0,1,2}){
			assert(!(!isnan(dta.fixedV[ax]) && dta.dofs[ax]>=0)); // handled by assignDofs
			assert(isnan(dta.fixedV[ax]) || dta.dofs[ax]<0);
			if(ax<dim){
				// in the spanned space
				dta.locV[ax]=dta.dofs[ax]<0?dta.fixedV[ax]:v[dta.dofs[ax]];
			} else {
				// out of the spanned space
				// don't move out of the space (assignDofs check local rotation is not out-of-plane)
				dta.locV[ax]=0; 
				if(dta.dofs[ax]>=0){ // prescribed stress; in that case v[dof] is L(ax,ax)
					assert(!isnan(Tout[ax]));
					dta.Lout[ax]=v[dta.dofs[ax]];
				} else {
					// in out-of-space deformation dta.Lout[ax] is prescribed, just leave that one alone
					assert(!isnan(dta.Lout[ax]));
				}
			}
		}
		dta.v=n->ori.conjugate()*dta.locV;
	};
};

void StaticEquilibriumSolver::assignDofs() {
	int nid=0, dof=0;
	// check that Tout is only prescribed where it makes sense
	for(int ax=0; ax<dim; ax++){
		if(!isnan(Tout[ax])) woo::ValueError("StaticEquilibriumSolver.Tout["+to_string(ax)+" must be NaN, since the space is "+to_string(dim)+"d (only out-of-space components may be assigned)");
	}
	for(const shared_ptr<Node>& n: field->nodes){
		SparcData& dta=n->getData<SparcData>();
		dta.nid=nid++;
		// check that if point is rotated, it does not rotate out of the spanned space
		switch(dim){
			case 2:{
				AngleAxisr aa(n->ori);
				if(aa.angle()>1e-8 && abs(aa.axis().dot(Vector3r::UnitZ()))<(1-1e-8)) woo::ValueError("Node "+to_string(nid)+" is rotated, but not along (0,0,1) [2d problem]!");
				break;
			}
			case 1:{
				if(n->ori!=Quaternionr::Identity()) woo::ValueError("Node "+to_string(nid)+" is rotated, which is not allowed with 1d basis");
			}
		}
		for(int ax:{0,1,2}){
			if(ax<dim){
				if(!isnan(dta.fixedV[ax])) dta.dofs[ax]=-1;
				else dta.dofs[ax]=dof++;
			} else {
				if(!isnan(dta.fixedV[ax])) woo::ValueError("Node "+to_string(nid)+" prescribes fixedV["+to_string(ax)+"]: not allowed in "+to_string(dim)+"d problems (would move out of basis space); use Lout in each point to prescribe diagonal gradV components.");
				if(!isnan(dta.fixedT[ax])) woo::ValueError("Node "+to_string(nid)+" prescribed fixedT["+to_string(ax)+"]: not allowed in "+to_string(dim)+"d problems, use ExplicitNodeIntegrator.Tout (the same for all points)");
				// stress prescribed along this axis, gradV is then unknown in that direction
				if(!isnan(Tout[ax])){
					// recycle some of the previous dofs if Tout was the same (with symm01d only)
					// (== checks also the previous value, as it will be false if any of the operands is NaN)
					if(symm01d && ((ax==1 && Tout[1]==Tout[0]) || (ax==2 && Tout[2]==Tout[0]))){ assert(dta.dofs[0]>=0); dta.dofs[ax]=dta.dofs[0]; }
					else if(symm01d && ax==2 && Tout[2]==Tout[1]) { assert(dta.dofs[1]>=0); dta.dofs[ax]=dta.dofs[1]; }
					else dta.dofs[ax]=dof++; // otherwise allocate a new dof for this unknown
				}
				else dta.dofs[ax]=-1; // strain in this direction given - no unknowns
			}
		}
	}
	nDofs=dof;
}

VectorXr StaticEquilibriumSolver::computeInitialDofVelocities(bool useZero) const {
	VectorXr ret; ret.setZero(nDofs);
	for(const shared_ptr<Node>& n: field->nodes){
		const SparcData& dta(n->getData<SparcData>());
		// fixedV and zeroed are in local coords
		for(int ax:{0,1,2}){
			if(dta.dofs[ax]<0) continue;
			if(ax>=dim){
				// if Tout[ax] are not prescribed, then there should be no unknown here at all
				assert(!isnan(Tout[ax]));
				if(isnan(dta.Lout[ax])) woo::ValueError("Node "+to_string(dta.nid)+": Lout["+to_string(ax)+"] must not be NaN when out-of-space stress Tout["+to_string(ax)+"] is prescribed; the value will be changed automatically).");
				ret[dta.dofs[ax]]=dta.Lout[ax];
			} 
			else if(useZero){ ret[dta.dofs[ax]]=0.; }
			else ret[dta.dofs[ax]]=(n->ori*dta.v)[ax]; 
		}
	}
	return ret;
};

void StaticEquilibriumSolver::prologuePhase(VectorXr& initVel){
	mff->updateLocator</*useNext*/false>();
	for(const shared_ptr<Node>& n: field->nodes){
		findNeighbors</*useNext*/false>(n); 
		updateLocalInterp</*useNext*/false>(n);
	}
	#ifdef SPARC_TRACE
		SPARC_TRACE_OUT("Neighbor numbers: "); for(const shared_ptr<Node>& n: field->nodes) SPARC_TRACE_OUT(n->getData<SparcData>().neighbors.size()<<" "); SPARC_TRACE_OUT("\n");
	#endif
	assignDofs();  // this is necessary only after number of free DoFs have changed; assign at every step to ensure consistency (perhaps not necessary if this is detected in the future)
	// intial solution is previous (or zero) velocities, except wher they are prescribed
	initVel=computeInitialDofVelocities(/*useZero*/initZero);
}


void StaticEquilibriumSolver::solutionPhase(const VectorXr& trialVel, VectorXr& errors){
	solutionPhase_computeResponse(trialVel);
	solutionPhase_computeErrors(errors);
};

void StaticEquilibriumSolver::epiloguePhase(const VectorXr& vel, VectorXr& errors){
	solutionPhase_computeResponse(vel);
	solutionPhase_computeErrors(errors); // this is just to have the error terms consistent with the solution at the end of the step
	integrateStateVariables();
}

void StaticEquilibriumSolver::solutionPhase_computeResponse(const VectorXr& trialVel){
	copyLocalVelocityToNodes(trialVel);
	for(const shared_ptr<Node>& n: field->nodes){
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
	for(const shared_ptr<Node>& n: field->nodes){
		if(!neighborsOnce) findNeighbors</*useNext*/true>(n);
		if(!relPosOnce) updateLocalInterp</*useNext*/true>(n);
		// TODO: update the value really, following spin perhaps?
		SparcData& dta=n->getData<SparcData>();
		dta.nextOri=n->ori;  
	}
	#ifdef SPARC_TRACE
		if(dbgFlags&DBG_DOFERR) SPARC_TRACE_OUT("{"<<endl);
	#endif
	// resid needs nextT of other points, hence in separate loop
	for(const shared_ptr<Node>& n: field->nodes){
		SparcData& dta(n->getData<SparcData>());
		Vector3r divT=computeDivT</*useNext*/true>(n);
		#ifdef SPARC_TRACE
			if(dbgFlags&DBG_DOFERR){
				SPARC_TRACE_OUT("  -->    nid "<<dta.nid<<" (dofs "<<dta.dofs.transpose()<<"), divT="<<divT.transpose()<<", locV="<<dta.locV.transpose()<<", v="<<dta.v.transpose());
				if(dbgFlags&DBG_NIDERR) SPARC_TRACE_OUT(endl);
			}
		#endif
		computeConstraintErrors</*useNext*/true>(n,divT,errors);
		#ifdef SPARC_INSPECT
			dta.divT=divT; // pure equilibrium residual, without constraints yet
		#endif
		#ifdef SPARC_TRACE
			if(dbgFlags&DBG_DOFERR){
				if(dbgFlags&DBG_NIDERR) SPARC_TRACE_OUT("         <--   final residuum "<<dta.resid<<endl);
				else SPARC_TRACE_OUT(", err="<<dta.resid<<endl);
			}
		#endif
	}
	#ifdef SPARC_TRACE
		if(dbgFlags&DBG_DOFERR) SPARC_TRACE_OUT("}"<<endl);
	#endif
}

// set resid in the sense of kinematic constraints, so that they are fulfilled by the static solution
template<bool useNext>
void StaticEquilibriumSolver::computeConstraintErrors(const shared_ptr<Node>& n, const Vector3r& divT, VectorXr& resid){
	SparcData& dta(n->getData<SparcData>());
	#ifdef WOO_DEBUG
		for(int i=0;i<3;i++) if(!isnan(dta.fixedV[i])&&!isnan(dta.fixedT[i])) throw std::invalid_argument((boost::format("Nid %d: both velocity and stress prescribed along local axis %d (axis %s in global space)") %dta.nid %i %(n->ori.conjugate()*(i==0?Vector3r::UnitX():(i==1?Vector3r::UnitY():Vector3r::UnitZ()))).transpose()).str());
	#endif
	#define _WATCH_NID(aaa) SPARC_TRACE_OUT("           "<<aaa<<endl); if(dta.nid==watch) cerr<<aaa<<endl;
	#ifdef SPARC_INSPECT
		dta.resid=Vector3r::Zero(); // in global coords here
	#endif
	if(dta.dofs.maxCoeff()<0) return; // don't bother if there are no dofs → no resid

	// use current or next orientation,stress
	Matrix3r oriTrsf(useNext?dta.nextOri:n->ori);
	const Matrix3r& T(!useNext?dta.T:dta.nextT); 
	Matrix3r locT=oriTrsf*T*oriTrsf.transpose();

	for(int ax:{0,1,2}){
		// velocity is prescribed; if not spanned, strain is prescribed, i.e. Tout[ax] are NaN
		int dof=dta.dofs[ax];
		if(dof<0){
			assert((ax<dim && !isnan(dta.fixedV[ax])) || (ax>=dim && isnan(Tout[ax])));
			continue;
		}
		if(ax>=dim) { // out-of-space dimension
			assert(!isnan(Tout[ax])); // it must not be NaN (i.e. prescribed L??): there would be no dof there
			resid[dof]=-(Tout[ax]-locT(ax,ax)); // difference between out-of-dimension stress and current stress
			#ifdef SPARC_TRACE
				if(dbgFlags&DBG_DOFERR) _WATCH_NID("\tnid "<<dta.nid<<" dof "<<dof<<" (out-of-space): locT["<<ax<<","<<ax<<"]="<<locT(ax,ax)<<" (should be "<<Tout[ax]<<") sets resid["<<dof<<"]="<<resid[dof]);
			#endif
		} else { // regular (boring) dimension
			// nothing prescribed, divT is the residual
			if(isnan(dta.fixedT[ax])){
				// NB: dta.divT is not the current value, that exists only with SPARC_INSPECT
				Vector3r locDivT=oriTrsf*divT;
				resid[dof]=charLen*locDivT[ax];
				#ifdef SPARC_TRACE
					if(dbgFlags&DBG_DOFERR) _WATCH_NID("\tnid "<<dta.nid<<" dof "<<dof<<": locDivT["<<ax<<"] "<<locDivT[ax]<<" (should be 0) sets resid["<<dof<<"]="<<resid[dof]);
				#endif
			}
			// prescribed stress
			else{
				// considered global stress
				resid[dof]=-(dta.fixedT[ax]-locT(ax,ax));
				#ifdef SPARC_TRACE
					if(dbgFlags&DBG_DOFERR) _WATCH_NID("\tnid "<<dta.nid<<" dof "<<dof<<": locT["<<ax<<","<<ax<<"] "<<locT(ax,ax)<<" (should be "<<dta.fixedT[ax]<<") sets resid["<<dof<<"]="<<resid[dof]);
				#endif
			}
		}
		#ifdef SPARC_INSPECT
			dta.resid+=oriTrsf.transpose()*Vector3r::Unit(ax)*resid[dof];
		#endif
	}
}

void StaticEquilibriumSolver::integrateStateVariables(){
	assert(functor);
	const Real& dt(scene->dt);

	for(const shared_ptr<Node>& n: field->nodes){
		SparcData& dta(n->getData<SparcData>());
		// this applies kinematic constraints directly (the solver might have had some error in satisfying them)
		applyKinematicConstraints(n,/*allow prescribed divT*/true); 
		dta.e+=dt*(1+dta.e)*dta.gradV.trace(); // gradV.trace()==D.trace()
		dta.rho+=dt*(-dta.rho*dta.gradV.trace()); // rho is not really used for the implicit solution, but can be useful
		dta.T=dta.nextT;
		n->pos+=dt*dta.v;
		for(int ax=dim; ax<3; ax++){
			if(dta.v[ax]!=0.) throw std::logic_error("Node "+to_string(dta.nid)+" has non-zero "+to_string(ax)+"th component of the velocity in a "+to_string(dim)+"d problem.");
		}
		n->ori=dta.nextOri;
	}
};

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
		// CASE_STATUS(UserAsked);
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

template<> string solverStatus2str<StaticEquilibriumSolver::SolverNewton>(int status){
	#define CASE_STATUS(st) case NewtonSolverSpace::st: return string(#st);
	switch(status){
		CASE_STATUS(Running);
		CASE_STATUS(JacobianNotInvertible);
		CASE_STATUS(TooManyFunctionEvaluation);
		CASE_STATUS(RelativeErrorTooSmall);
		CASE_STATUS(AbsoluteErrorTooSmall);
		CASE_STATUS(UserAsked);
	}
	#undef CASE_STATUS
	throw std::logic_error(("solverStatus2str<NewtonSolver> called with unknown status number "+lexical_cast<string>(status)).c_str());
}


void StaticEquilibriumSolver::solverInit(VectorXr& currV){
	prologuePhase(currV);
	SPARC_TRACE_OUT("Initial DOF velocities "<<currV.transpose()<<endl);
	if(nDofs==0 && solver!=SOLVER_NONE){
		LOG_WARN("Setting StaticEquilibriumSolver.solver=solverNone because of 0 dofs.");
		solver=SOLVER_NONE;
	}

	// http://stackoverflow.com/questions/6895980/boostmake-sharedt-does-not-compile-shared-ptrtnew-t-does
	// functor=make_shared<ResidualsFunctor>(vv.size(),vv.size(),this); 
	functor=shared_ptr<ResidualsFunctor>(new ResidualsFunctor(nDofs,nDofs)); functor->ses=this;
	int status;
	switch(solver){
		case SOLVER_NONE:
			if(nDofs>0) throw std::runtime_error("StaticEquilibriumSolver.solver==solverNone is not admissible with nonzero number of dofs.");
			solverPowell.reset(); solverLM.reset(); solverNewton.reset();
			return;
		case SOLVER_POWELL:
			solverLM.reset(); solverNewton.reset();
			solverPowell=make_shared<SolverPowell>(*functor);
			solverPowell->parameters.factor=solverFactor;
			solverPowell->parameters.epsfcn=epsfcn;
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
			break;
		case SOLVER_LM:
			solverPowell.reset(); solverNewton.reset();
			solverLM=make_shared<SolverLM>(*functor);
			solverLM->parameters.factor=solverFactor;
			solverLM->parameters.maxfev=relMaxfev*currV.size(); // this is perhaps bogus, what is exactly maxfev?
			solverLM->parameters.epsfcn=epsfcn;
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
			break;
		case SOLVER_NEWTON:
			solverLM.reset(); solverPowell.reset();
			solverNewton=make_shared<SolverNewton>(*functor);
			if(solverXtol>0) solverNewton->xtol=solverXtol;
			solverNewton->maxfev=relMaxfev*currV.size();
			solverNewton->jacEvery=jacEvery;
			solverNewton->jacEigen=jacEigen;
			solverNewton->epsScale=epsScale;
			status=solverNewton->solveNumericalDiffInit(currV);
			LOG_TRACE("Newton solver: initial solution has residuum "<<solverNewton->fnorm0<<" (tol. rel "<<solverNewton->xtol<<"×"<<solverNewton->fnorm0<<"="<<solverNewton->fnorm0*solverNewton->xtol<<", abs "<<solverNewton->abstol<<")");
			#ifdef SPARC_INSPECT
				residuals=solverNewton->fvec; residuum=solverNewton->fnorm; jac=solverNewton->jac; jacInv=solverNewton->jacInv;
			#endif
			if(status==NewtonSolverSpace::JacobianNotInvertible) throw std::runtime_error("StaticEquilibriumSolver (Newton): Jacobian matrix not invertible.");
			break;
		default:
			throw std::logic_error("Unknown value of StaticEquilibriumSolver.solver=="+to_string(solver));
	}
	#if defined(SPARC_TRACE) and defined(SPARC_INSPECT)
		if(dbgFlags&DBG_JAC){
			SPARC_TRACE_OUT("{ Initial jacobian is "<<jac.rows()<<"×"<<jac.cols()<<":"<<endl<<jac<<endl<<"}"<<endl);
		}
	#endif
}

int StaticEquilibriumSolver::solverStep(VectorXr& currV){
	int status;
	switch(solver){
		case SOLVER_NONE:{
			assert(currV.size()==0); VectorXr fvec; fvec.resize(0);
			int ret=(*functor)(currV,fvec);
			#ifdef SPARC_INSPECT
				residuals.resize(0,0); residuum=0.; jac.resize(0,0);
			#endif
			return (ret>=0?0:-1);
		}
		case SOLVER_POWELL:
			assert(solverPowell);
			status=solverPowell->solveNumericalDiffOneStep(currV);
			SPARC_TRACE_OUT("Powell inner iteration "<<nIter<<endl<<"Solver proposed solution "<<currV.transpose()<<endl<<"Residuals vector "<<solverPowell->fvec.transpose()<<endl<<"Error norm "<<lexical_cast<string>(solverPowell->fnorm)<<endl);
			LOG_TRACE("Powell inner iteration "<<nIter<<" with residuum "<<solverPowell->fnorm);
			#ifdef SPARC_INSPECT
				residuals=solverPowell->fvec; residuum=solverPowell->fnorm; jac=solverPowell->fjac;
			#endif
			break;
		case SOLVER_LM:
			assert(solverLM);
			status=solverLM->minimizeOneStep(currV);
			SPARC_TRACE_OUT("Levenberg-Marquardt inner iteration "<<nIter<<endl<<"Solver proposed solution "<<currV.transpose()<<endl<<"Residuals vector "<<solverLM->fvec.transpose()<<endl<<"Error norm "<<solverLM->fnorm<<endl);
			LOG_TRACE("Levenberg-Marquardt inner iteration "<<nIter<<" with residuum "<<solverLM->fnorm);
			#ifdef SPARC_INSPECT
				residuals=solverLM->fvec; residuum=solverLM->fnorm; jac=solverLM->fjac;
			#endif
			break;
		case SOLVER_NEWTON:
			assert(solverNewton);
			status=solverNewton->solveNumericalDiffOneStep(currV);
			LOG_TRACE("Newton inner iteration "<<nIter<<", residuum "<<solverNewton->fnorm<<" (functor "<<solverNewton->nfev<<"×, jacobian "<<solverNewton->njev<<"×)");
			#ifdef SPARC_INSPECT
				residuals=solverNewton->fvec; residuum=solverNewton->fnorm; jac=solverNewton->jac; jacInv=solverNewton->jacInv;
			#endif
			break;
		default:
			throw std::logic_error("Unknown value of StaticEquilibriumSolver.solver=="+to_string(solver));
	}
	return status;
}


void StaticEquilibriumSolver::run(){
	#ifdef SPARC_TRACE
		if(!dbgOut.empty() && scene->step==0 && bfs::exists(dbgOut)){
			bfs::remove(bfs::path(dbgOut));
			LOG_WARN("Old StaticEquilibriumSolver.dbgOut "<<dbgOut<<" deleted.");
		}
		if(!out.is_open()) out.open(dbgOut.empty()?"/dev/null":dbgOut.c_str(),ios::out|ios::app);
		if(scene->step==0) out<<"# vim: guifont=Monospace\\ 7:nowrap:syntax=c:foldenable:foldmethod=syntax:foldopen=percent:foldclose=all:\n";
	#endif
	if(!substep || (solver==SOLVER_POWELL && !solverPowell) || (solver==SOLVER_LM && !solverLM) || (solver==SOLVER_NEWTON && !solverNewton)){ /* start anew */ progress=PROGRESS_DONE; }
	int status;
	SPARC_TRACE_OUT("\n\n ==== Scene step "<<scene->step<<", solution iteration "<<((progress==PROGRESS_DONE || progress==PROGRESS_ERROR?0:nIter))<<endl);
	// when dt>0, it means we reset scene->dt to our value; restore it now
	if(dt>0){
		scene->dt=dt;
		dt=NaN;
	}
	if(progress==PROGRESS_DONE || progress==PROGRESS_ERROR){
		if(progress==PROGRESS_ERROR) LOG_WARN("Previous solver step ended abnormally, restarting solver.");
		mff=static_cast<SparcField*>(field.get());
		// intial solver setup
		solverInit(currV);
		nIter=0;
	}

	// solution loop
	while(true){
		nIter++;
		// one solver step
		status=solverStep(currV);
		// good progress
		if(solver==SOLVER_NONE && status==0) goto solutionFound; // solution computed in one step; 
		if(
			(solver==SOLVER_POWELL && status==Eigen::HybridNonLinearSolverSpace::Running)
			|| (solver==SOLVER_LM && status==Eigen::LevenbergMarquardtSpace::Running)
			|| (solver==SOLVER_NEWTON && status==NewtonSolverSpace::Running)){
			if(substep) goto substepDone; else continue;
		}
		// solution found
		if((solver==SOLVER_POWELL && status==Eigen::HybridNonLinearSolverSpace::RelativeErrorTooSmall)
			|| (solver==SOLVER_NEWTON && (status==NewtonSolverSpace::RelativeErrorTooSmall || status==NewtonSolverSpace::AbsoluteErrorTooSmall))
			|| (solver==SOLVER_LM && (status==Eigen::LevenbergMarquardtSpace::RelativeErrorTooSmall || status==Eigen::LevenbergMarquardtSpace::RelativeErrorAndReductionTooSmall || status==Eigen::LevenbergMarquardtSpace::RelativeReductionTooSmall || status==Eigen::LevenbergMarquardtSpace::CosinusTooSmall))){
			goto solutionFound;
		}
		// bad convergence (Powell)
		if(solver==SOLVER_POWELL && (status==Eigen::HybridNonLinearSolverSpace::NotMakingProgressIterations || status==Eigen::HybridNonLinearSolverSpace::NotMakingProgressJacobian)){
			// try decreasing factor
			solverPowell->parameters.factor*=.6;
			if(++nFactorLowered<10){ LOG_WARN("Step "<<scene->step<<": Powell solver did not converge (error="<<solverPowell->fnorm<<"), lowering factor to "<<solverPowell->parameters.factor); if(substep) goto substepDone; else continue; }
			// or give up
			LOG_WARN("Step "<<scene->step<<": Powell solver not converging, but factor already lowered too many times ("<<nFactorLowered<<"). giving up.");
		}
		string msg="[unhandled solver]";
		switch(solver){
			case SOLVER_NONE: msg="SOLVER_NONE: functor returned an error during response computation?!";
			case SOLVER_POWELL: msg=solverStatus2str<SolverPowell>(status); break;
			case SOLVER_LM: msg=solverStatus2str<SolverLM>(status); break;
			case SOLVER_NEWTON: msg=solverStatus2str<SolverNewton>(status); break;
		}
		progress=PROGRESS_ERROR;
		throw std::runtime_error((boost::format("Solver did not find an acceptable solution, returned %s (%d).")%msg%status).str());
	}
	substepDone:
		assert(substep);
		copyLocalVelocityToNodes(currV);
		progress=PROGRESS_RUNNING;
		// reset timestep so that simulation does not advance until we get a solution
		dt=scene->dt;
		scene->dt=0;
		return;
	solutionFound:
		progress=PROGRESS_DONE;
		// LOG_WARN("Solution found, error norm "<<solver->fnorm);
		switch(solver){
			case SOLVER_NONE: nIter=1; break;
			case SOLVER_POWELL: nIter=solverPowell->iter; break; 
			case SOLVER_LM: nIter=solverLM->iter; break;
			case SOLVER_NEWTON: nIter=solverNewton->iter; break;
			default: abort();
		}
		// residuum=solver->fnorm;
		epiloguePhase(currV,residuals);
		LOG_TRACE("Solution found after "<<nIter<<" (inner) iterations.");
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


#ifdef WOO_OPENGL
#include<woo/lib/base/CompUtils.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/pkg/gl/Renderer.hpp>


WOO_PLUGIN(gl,(Gl1_SparcField)(SparcConstraintGlRep));


bool Gl1_SparcField::nodes;
bool Gl1_SparcField::nid;
bool Gl1_SparcField::neighbors;
vector<int> Gl1_SparcField::conn;
vector<Vector3r> Gl1_SparcField::connColors;

void Gl1_SparcField::go(const shared_ptr<Field>& sparcField, GLViewInfo* _viewInfo){
	sparc=static_pointer_cast<SparcField>(sparcField);
	viewInfo=_viewInfo;

	for(const shared_ptr<Node>& n: sparc->nodes){
		Renderer::glScopedName name(n);
		Renderer::setNodeGlData(n); // assures that GlData is defined
		if(nodes) Renderer::renderRawNode(n);
		if(n->rep){ n->rep->render(n,viewInfo); }
		// GLUtils::GLDrawText((boost::format("%d")%n->getData<SparcData>().nid).str(),n->pos,/*color*/Vector3r(1,1,1), /*center*/true,/*font*/NULL);
		int nnid=n->getData<SparcData>().nid;
		const Vector3r& pos=n->pos+n->getData<GlData>().dGlPos;
		if(nid && nnid>=0) GLUtils::GLDrawNum(nnid,pos);
		if(!neighbors && !name.highlighted) continue;
		// show neighbours with lines, with node colors
		Vector3r color=CompUtils::mapColor(n->getData<SparcData>().color);
		for(const shared_ptr<Node>& neighbor: n->getData<SparcData>().neighbors){
			if(!neighbor->hasData<GlData>()) continue; // neighbor might not have GlData yet, will be ok in next frame
			const Vector3r& np=neighbor->pos+neighbor->getData<GlData>().dGlPos;
			GLUtils::GLDrawLine(pos,pos+.5*(np-pos),color,3);
			GLUtils::GLDrawLine(pos+.5*(np-pos),np,color,1);
		}
	}
	if(!conn.empty()){
		bool strip=false;
		int colNum=0;
		for(int nid: conn){
			if(strip){
				// stop current line strip if next point is invalid
				if(nid<0 || nid>=(int)sparc->nodes.size()){ glEnd(); strip=false; continue; }
				// otherwise to nothing
			} else {
				// start new line strip if the id is valid and there is no strip currently
				if(nid>0 && nid<(int)sparc->nodes.size()){
					if(!connColors.empty()) glColor3v(connColors[colNum%connColors.size()]);
					else glColor3v(Vector3r(1,1,1));
					glBegin(GL_LINE_STRIP); strip=true;
				} else continue; // no strip and id invalid: do nothing
			}
			const auto& n=sparc->nodes[nid];
			glVertex3v((n->pos+n->getData<GlData>().dGlPos).eval());
		}
		if(strip) glEnd();
	}
};



void SparcConstraintGlRep::renderLabeledArrow(const Vector3r& pos, const Vector3r& vec, const Vector3r& color, Real num, bool posIsA, bool doubleHead){
	Vector3r A(posIsA?pos:pos-vec), B(posIsA?pos+vec:pos);
	GLUtils::GLDrawArrow(A,B,color);
	if(doubleHead) GLUtils::GLDrawArrow(A,A+.9*(B-A),color);
	if(!isnan(num)) GLUtils::GLDrawNum(num,posIsA?B:A,Vector3r::Ones());
}

void SparcConstraintGlRep::render(const shared_ptr<Node>& node, const GLViewInfo* viewInfo){
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

#endif /*WOO_OPENGL*/


#endif /*WOO_VTK*/
#endif /*WOO_SPARC*/
