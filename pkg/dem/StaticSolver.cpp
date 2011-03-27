#include<yade/pkg/dem/StaticSolver.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/common/NormShearPhys.hpp>
#include<yade/pkg/dem/L3Geom.hpp>


#include<Eigen/Cholesky>

Vector2r _foobar; // clang+linker?!


YADE_PLUGIN((StaticSolver));
CREATE_LOGGER(StaticSolver);

/*

* Every particles is assigned 6 dofs.
* Unused nodes (empty stiffness matrix lines) are eliminated automatically.

vector<Dof> dofs has 6 entries for each particle; Node::ix is matrix line number (which does not correspond to index in dofs!)

Dof are by default unused; as the global matrix is assembled, they are flagged as used, to avoid their condensation -- Dof::setUsed(true).

*/

/* for each particle, create 6 dofs, either free/fixed; they will be marked as unused for now */
void StaticSolver::setupDofs(){
	dofs.clear();
	dofs.resize(scene->bodies->size()*6);
	FOREACH(const shared_ptr<Body>& b, *scene->bodies){
		if(!b) continue;
		size_t ix0=b->id*6;
		for(int i=0; i<6; i++){
			assert(dofs[ix0+i].flags==0); // no flags set, the dof is unused; it will be set as used if corresponding matrix entries are added in assembly
			dofs[ix0+i].flags=(b->state->blockedDOFs & State::linDOF(i)) ? DOF_FIXED : 0;
		}
	};
	LOG_TRACE(dofs.size()<<" dofs created.");
};

/* return stiffness matrix for given interaction; K is the matrix in global coordinates, dofNums are corresponding number of dof (-1 for synthatic dof, which should be added by the caller, otherwise 6*id+linDof) */
Matrix12r StaticSolver::contactK(const shared_ptr<Interaction>& I){
	/*
	
	12×12 stiffness matrix in global coordinates, for DEM-like contact transmitting normal and shear force at contact point (no torque transmission (yet?))

		kl = local tangent stiffness matrix; diagonal matrix with (kn,ks,ks)
		T  = local orientation matrix
		kg = T*kl*T^T (tangent stiffness matrix in global coords; generally non-diagonal)
		b1 = branch vector between contact point and particle 1 centroid, in global coords
		b2 = branch vector between contact point and particle 2 centroid, in global coords
		I  = identity matrix

	The resulting matrix is symmetric, with the following 3x3 blocks

	[ kg ]   [kg*I*b1] [-kg ]     [-kg*I*b2]
	         [ 0 ]     [-kg*I*b1] [ 0 ]
	                   [ kg ]     [ kg*I*b2]
	   (sym)                      [ 0 ]

	row order are triplets u₁,φ₁,u₂,φ₂; columns F₁,T₁,F₂,T₂.

	Zeros on diagonal are correct, those dofs must be stabilized by other contacts.

	*/
	Vector3r b1,b2;
	Matrix3r kl, kg, T, Ib1, Ib2;
	const shared_ptr<State>& state1(Body::byId(I->getId1(),scene)->state);
	const shared_ptr<State>& state2(Body::byId(I->getId1(),scene)->state);

	// local stiffness matrix
	NormShearPhys* nsp=YADE_CAST<NormShearPhys*>(I->phys.get()); assert(nsp);
	kl<<nsp->kn,0,0, 0,nsp->ks,0, 0,0,nsp->ks;

	// local orientation matrix; only the normal matters, make up the perpendicular axes orientation
	L3Geom* l3g=YADE_CAST<L3Geom*>(I->geom.get()); assert(l3g);
	T=l3g->trsf;

	// global stiffness matrix (transform with T^T, since T itself is from global to local)
	kg=T.transpose()*kl*T; // TODO: check that it transforms correctly

	// branch vectors/matrices
	b1=l3g->contactPoint-state1->pos;
	b2=l3g->contactPoint-state2->pos+(scene->isPeriodic?Vector3r(scene->cell->hSize*I->cellDist.cast<Real>()):Vector3r::Zero());
	Ib1=b1.asDiagonal();
	Ib2=b2.asDiagonal();

	Matrix12r K;
	// TODO: store only the upper-triangular part, since it is symmetric (eigen3 has direct support for that, see marked<UpperTriangular> for instance)
	K.block<3,3>(0,0)=kg; K.block<3,3>(0,3)=kg*Ib1;           K.block<3,3>(0,6)=-kg;     K.block<3,3>(0,9)=-kg*Ib2;
	                      K.block<3,3>(3,3)=Matrix3r::Zero(); K.block<3,3>(3,6)=-kg*Ib1; K.block<3,3>(3,9)=Matrix3r::Zero();
	                                                          K.block<3,3>(6,6)=kg;      K.block<3,3>(6,9)=kg*Ib2;
	                                                                                     K.block<3,3>(9,9)=Matrix3r::Zero();
	/* symmetrize */
	for(int i=0; i<12; i++) for(int j=i; j<12; j++) K(j,i)=K(i,j);
	
	LOG_TRACE("K for ##"<<I->getId1()<<"+"<<I->getId2()<<":\n"<<K);

	return K;
}

SparseXr StaticSolver::assembleK(){
	SparseXr KK0(dofs.size(),dofs.size());
	{
		Eigen::RandomSetter<SparseXr> K(KK0);
		Eigen::Matrix<int,12,1> ixMap;
		FOREACH(const shared_ptr<Interaction>& I, *scene->interactions){
			if(!I->isReal()) continue;
			LOG_DEBUG("Contact ##"<<I->getId1()<<"+"<<I->getId2());
			Matrix12r Kc=contactK(I);
			for(int i=0; i<12; i++) ixMap[i]=(i<6?I->getId1():I->getId2())*6+(i%6);
			LOG_TRACE("Local→global dof map: "<<ixMap.transpose());
			// go through the matrix now; rather inefficient; only use the upper-triangular part here
			for(int i=0; i<12; i++){
				for(int j=i; j<12; j++){
					if(Kc(i,j)==0) continue;
					int ii=ixMap[i], jj=ixMap[j]; // indices in the global matrix
					K(ii,jj)=K(jj,ii)=Kc(i,j);
					// fixed dofs are "unused" in the sense of not appearing in the sense of being condensed away
					// not very clear here!
					if((!(dofs[ii].flags & DOF_FIXED)) && !dofs[ii].isUsed()) dofs[ii].setUsed(true); // mark dof as used (perhaps the compiler would be smart enough to eliminate the conditional internally?)
				}
				LOG_TRACE(i<<": dof "<<ixMap[i]<<"("<<ixMap[i]/6<<","<<ixMap[i]%6<<"): "<<(dofs[ixMap[i]].isUsed()?"used":"unused"));
			}
		}
	}
	return KK0;
};

SparseXr StaticSolver::condenseK(SparseXr& KK0){
	assert(KK0.cols()==dofs.size()); 
	assert(KK0.cols()==KK0.rows());
	// count how many used dofs we have
	int used=0;
	// note: isUsed is _false_ for dofs that either are not connected, or are fixed
	FOREACH(Dof& dof, dofs) if(dof.isUsed()){ dof.ix=used++; }
	int nDofs=KK0.cols();
	SparseXr KK(used,used);
	LOG_DEBUG("K will be condensed (size "<<KK0.rows()<<" → "<<KK.rows()<<")");
	LOG_TRACE(KK0);
	{
		Eigen::RandomSetter<SparseXr> K(KK);
		Eigen::RandomSetter<SparseXr> KK00(KK0); // we need read-only access here; likely inefficient
		for(int i=0; i<nDofs; i++){
			if(!dofs[i].isUsed()){
				LOG_TRACE("#"<<i/6<<"/"<<i%6<<" condensed away.");
				continue;
			}
			for(int j=i; j<nDofs; j++){
				if(!dofs[j].isUsed()) continue;
				int ii=dofs[i].ix, jj=dofs[j].ix;
				K(ii,jj)+=KK00(i,j);
				K(jj,ii)+=KK00(i,j);
			}
		}
	}
	LOG_TRACE("K after condensation:\n"<<KK);
	return KK;
}

VectorXr StaticSolver::condensedRhs(int n){
	// prescribe reversed forces on RHS, so that current ones are cancelled, giving equilibrium
	VectorXr ret=VectorXr::Zero(n);
	scene->forces.sync();
	int nDofs=dofs.size();
	for(int i=0; i<nDofs; i++){
		const Dof& dof=dofs[i];
		if(dof.ix<0) continue;
		Body::id_t id=i/6; int linDof=i%6;
		Real val=(linDof<3?(scene->forces.getForce(id)[linDof]):(scene->forces.getTorque(id)[linDof-3]));
		ret[dof.ix]=val;
		LOG_TRACE("RHS line "<<dof.ix<<": #"<<id<<"/"<<linDof<<"="<<val);
	}
	LOG_DEBUG("RHS is "<<ret.transpose());
	return ret;
}

VectorXr StaticSolver::solveU(const SparseXr& K, const VectorXr& f){
	assert(K.rows()==K.cols()); assert(K.cols()==f.rows());
	Eigen::SparseLLT<SparseXr,Eigen::Cholmod> llt(K);
	VectorXr ret=f;
	LOG_TRACE("Will solve now");
	llt.solveInPlace(ret);
	LOG_TRACE("Solved.");
	return ret;
}

void StaticSolver::applyU(const VectorXr& U, Real alpha){
	int nDofs=dofs.size();
	for(int i=0; i<nDofs; i++){
		const Dof& dof=dofs[i];
		if(!dof.isUsed()) continue;
		Body::id_t id=i/6; int linDof=i%6;
		Real u=alpha*U(dof.ix);
		const shared_ptr<Body>& b=Body::byId(id,scene);
		LOG_DEBUG("Displace #"<<id<<"/"<<linDof<<" by "<<u);
		if(linDof<3){
			b->state->pos[linDof]+=u;
		} else {
			Vector3r rot=Vector3r::Zero(); rot[linDof-3]=u; // rotation vector of the displacement
			AngleAxisr aa(b->state->ori);
			Vector3r phi2=aa.angle()*aa.axis()+rot; Real phi2Norm=phi2.norm(); // new orientation, as rotation vector
			b->state->ori=Quaternionr(AngleAxisr(phi2Norm,phi2/phi2Norm));
		}
	}
};

void StaticSolver::action(){
	// create the dofs array
	setupDofs();
	// get stiffness matrix for each contact, get also its dof indices (create synthetic nodes for facets/walls as necessary, if they are not fixed)
	// add contact matrices to the global matrix (respect already here DOF_SLAVE and DOF_FORCE?)
	SparseXr KK0=assembleK();
	// [[ eliminate rows/cols for unused dofs ]]
	SparseXr KK=condenseK(KK0);
	// assemble RHS, dummy version for now
	VectorXr RHS=condensedRhs(KK.rows());
	// invert the matrix, get displacements
	VectorXr U=solveU(KK,RHS);
	// apply displacements multiplied perhaps by α=1,
	applyU(U,/*α*/1.);
	// check that no new contacts were created, running Ig2 functors on everything (also new collision detection before!), or perhaps find straight from the Ig2 functor what would be the maximum displacement that would not create any overlap yet
	// with new contacts apply displacement only with α←α/2 (or α/10 etc), until there are no new contacts
}
