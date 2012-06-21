#include<woo/pkg/dem/Clump.hpp>

WOO_PLUGIN(dem,(ClumpData));
CREATE_LOGGER(ClumpData);

shared_ptr<Node> ClumpData::makeClump(const vector<shared_ptr<Node>>& nn, shared_ptr<Node> centralNode, bool intersecting){
	/* TODO? check that nodes are unique */
	auto clump=make_shared<ClumpData>();
	clump->setClump();
	auto cNode=(centralNode?centralNode:make_shared<Node>());
	cNode->setData<DemData>(clump);

	size_t N=nn.size();
	if(N==1){
		LOG_DEBUG("Treating 1-clump specially.");
		cNode->pos=nn[0]->pos;
		cNode->ori=nn[0]->ori;
		clump->nodes.push_back(nn[0]);
		auto& d0=nn[0]->getData<DemData>();
		d0.setClumped();
		clump->relPos.push_back(Vector3r::Zero());
		clump->relOri.push_back(Quaternionr::Identity());
		clump->mass=d0.mass;
		clump->inertia=d0.inertia;
		return cNode;
	}

	/* clumps with more than one particle */

	if(intersecting) yade::NotImplementedError("Self-intersecting clumps not yet implemented.");
	double M=0; // mass
	Vector3r Sg=Vector3r::Zero(); // static moment, for getting clump's centroid
	Matrix3r Ig=Matrix3r::Zero(); // tensors of inertia; is upper triangular
	// first loop: compute clump's centroid and principal orientation
	for(const auto& n: nn){
		const auto& dem=n->getData<DemData>();
		if(dem.isClumped()) yade::RuntimeError("Node "+lexical_cast<string>(n)+": already clumped.");
		if(!dem.parCount>0) yade::RuntimeError("Node "+lexical_cast<string>(n)+": particle count for clumped particles must be > 0, not "+to_string(dem.parCount));
		M+=dem.mass;
		Sg+=dem.mass*n->pos;
		Ig+=inertiaTensorTranslate(inertiaTensorRotate(dem.inertia.asDiagonal(),n->ori.conjugate()),dem.mass,-1.*n->pos);
	}
	if(M>0){
		assert(M>0); LOG_TRACE("M=\n"<<M<<"\nIg=\n"<<Ig<<"\nSg=\n"<<Sg);
		// clump's centroid
		cNode->pos=Sg/M;
		// this will calculate translation only, since rotation is zero
		Matrix3r Ic_orientG=inertiaTensorTranslate(Ig, -M /* negative mass means towards centroid */, cNode->pos); // inertia at clump's centroid but with world orientation
		LOG_TRACE("Ic_orientG=\n"<<Ic_orientG);

		Ic_orientG(1,0)=Ic_orientG(0,1); Ic_orientG(2,0)=Ic_orientG(0,2); Ic_orientG(2,1)=Ic_orientG(1,2); // symmetrize
		//TRWM3MAT(Ic_orientG);
		Eigen::SelfAdjointEigenSolver<Matrix3r> decomposed(Ic_orientG);
		const Matrix3r& R_g2c(decomposed.eigenvectors());
		// has NaNs for identity matrix??
		LOG_TRACE("R_g2c=\n"<<R_g2c);
		// set quaternion from rotation matrix
		cNode->ori=Quaternionr(R_g2c); cNode->ori.normalize();
		clump->inertia=decomposed.eigenvalues();
		clump->mass=M;
		// this block will be removed once EigenDecomposition works for diagonal matrices
		#if 1
			if(isnan(R_g2c(0,0))||isnan(R_g2c(0,1))||isnan(R_g2c(0,2))||isnan(R_g2c(1,0))||isnan(R_g2c(1,1))||isnan(R_g2c(1,2))||isnan(R_g2c(2,0))||isnan(R_g2c(2,1))||isnan(R_g2c(2,2))){
				throw std::logic_error("Clump::updateProperties: NaNs in eigen-decomposition of inertia matrix?!");
			}
		#endif
		LOG_TRACE("clump->inertia="<<clump->inertia.transpose());
		// TODO: these might be calculated from members... but complicated... - someone needs that?!
		clump->vel=clump->angVel=Vector3r::Zero();
		#ifdef WOO_DEBUG
			AngleAxisr aa(cNode->ori);
		#endif
		LOG_TRACE("pos="<<cNode->pos.transpose()<<", ori="<<aa.axis()<<":"<<aa.angle());
	} else {
		// nodes have no mass; in that case, centralNode is used
		if(!centralNode) throw std::runtime_error("Clump::updateProperties: no nodes with mass, therefore centralNode must be given, of which position will be used instead");
		throw std::runtime_error("Clump::updateProperties: massless clumps not yet implemented.");
	}

	clump->nodes.reserve(N); clump->relPos.reserve(N); clump->relOri.reserve(N);
	for(size_t i=0; i<N; i++){
		const auto& n=nn[i];
		auto& dem=n->getData<DemData>();
		clump->nodes. push_back(n);
		clump->relPos.push_back(cNode->ori.conjugate()*(n->pos-cNode->pos));
		clump->relOri.push_back(cNode->ori.conjugate()*n->ori);
		#ifdef WOO_DEBUG
			AngleAxisr aa(*(clump->relOri.rbegin()));
		#endif
		LOG_TRACE("relPos="<<clump->relPos.rbegin()->transpose()<<", relOri="<<aa.axis()<<":"<<aa.angle());
		dem.setClumped();
	}
	return cNode;
}

void ClumpData::collectFromMembers(const shared_ptr<Node>& node){
	ClumpData& clump=node->getData<DemData>().cast<ClumpData>();
	assert(clump.nodes.size()==clump.relPos.size()); assert(clump.nodes.size()==clump.relOri.size());
	for(size_t i=0; i<clump.nodes.size(); i++){
		const DemData& dyn=clump.nodes[i]->getData<DemData>();
		clump.force+=dyn.force;
		clump.torque+=dyn.torque+(clump.nodes[i]->pos-node->pos).cross(dyn.force);
		// cerr<<"f="<<clump.force.transpose()<<", t="<<clump.torque.transpose()<<endl;
	}
}

void ClumpData::applyToMembers(const shared_ptr<Node>& node, bool reset){
	ClumpData& clump=node->getData<DemData>().cast<ClumpData>();
	const Vector3r& clumpPos(node->pos); const Quaternionr& clumpOri(node->ori);
	assert(clump.nodes.size()==clump.relPos.size()); assert(clump.nodes.size()==clump.relOri.size());
	for(size_t i=0; i<clump.nodes.size(); i++){
		const shared_ptr<Node>& n(clump.nodes[i]);
		DemData& nDyn(n->getData<DemData>());
		n->pos=clumpPos+clumpOri*clump.relPos[i];
		n->ori=clumpOri*clump.relOri[i];
		nDyn.vel=clump.vel+clump.angVel.cross(n->pos-clumpPos);
		nDyn.angVel=clump.angVel;
		if(reset) nDyn.force=nDyn.torque=Vector3r::Zero();
	}
}

void ClumpData::resetForceTorque(const shared_ptr<Node>& node){
	ClumpData& clump=node->getData<DemData>().cast<ClumpData>();
	for(size_t i=0; i<clump.nodes.size(); i++){
		DemData& nDyn(clump.nodes[i]->getData<DemData>());
		nDyn.force=nDyn.torque=Vector3r::Zero();
	}
}

/*! @brief Recalculates inertia tensor of a body after translation away from (default) or towards its centroid.
 *
 * @param I inertia tensor in the original coordinates; it is assumed to be upper-triangular (elements below the diagonal are ignored).
 * @param m mass of the body; if positive, translation is away from the centroid; if negative, towards centroid.
 * @param off offset of the new origin from the original origin
 * @return inertia tensor in the new coordinate system; the matrix is symmetric.
 */
Matrix3r ClumpData::inertiaTensorTranslate(const Matrix3r& I,const Real m, const Vector3r& off){
	// TODO: check the first gives the same result as the second, delete the second subsequently
	#if 0
		// short eigen implementation; check it gives the same result as above
		return I+m*(off.dot(off)*Matrix3r::Identity()-off*off.transpose());
	#else
		Real ooff=off.dot(off);
		Matrix3r I2=I;
		// translation away from centroid
		/* I^c_jk=I'_jk-M*(delta_jk R.R - R_j*R_k) [http://en.wikipedia.org/wiki/Moments_of_inertia#Parallel_axes_theorem] */
		Matrix3r dI; dI<</* dIxx */ ooff-off[0]*off[0], /* dIxy */ -off[0]*off[1], /* dIxz */ -off[0]*off[2],
			/* sym */ 0., /* dIyy */ ooff-off[1]*off[1], /* dIyz */ -off[1]*off[2],
			/* sym */ 0., /* sym */ 0., /* dIzz */ ooff-off[2]*off[2];
		I2+=m*dI;
		I2(1,0)=I2(0,1); I2(2,0)=I2(0,2); I2(2,1)=I2(1,2); // symmetrize
		//TRWM3MAT(I2);
		return I2;
	#endif
}

/*! @brief Recalculate body's inertia tensor in rotated coordinates.
 *
 * @param I inertia tensor in old coordinates
 * @param T rotation matrix from old to new coordinates
 * @return inertia tensor in new coordinates
 */
Matrix3r ClumpData::inertiaTensorRotate(const Matrix3r& I,const Matrix3r& T){
	/* [http://www.kwon3d.com/theory/moi/triten.html] */
	//TRWM3MAT(I); TRWM3MAT(T);
	return T.transpose()*I*T;
}

/*! @brief Recalculate body's inertia tensor in rotated coordinates.
 *
 * @param I inertia tensor in old coordinates
 * @param rot quaternion that describes rotation from old to new coordinates
 * @return inertia tensor in new coordinates
 */
Matrix3r ClumpData::inertiaTensorRotate(const Matrix3r& I, const Quaternionr& rot){
	Matrix3r T=rot.toRotationMatrix();
	return inertiaTensorRotate(I,T);
}

