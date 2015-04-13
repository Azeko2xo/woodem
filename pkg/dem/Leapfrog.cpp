#include<woo/pkg/dem/Leapfrog.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<iomanip>

WOO_PLUGIN(dem,(Leapfrog)(ForceResetter));
WOO_IMPL_LOGGER(Leapfrog);

WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Leapfrog__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_ForceResetter__CLASS_BASE_DOC);


void ForceResetter::run(){
	const auto& dem=field->cast<DemField>();
	bool hasGravity(dem.gravity!=Vector3r::Zero());
	FOREACH(const shared_ptr<Node>& n, field->nodes){
		DemData& dyn=n->getData<DemData>();
		dyn.force=(hasGravity && !dyn.isGravitySkip())?(dem.gravity*dyn.mass).eval():Vector3r::Zero();
		dyn.torque=Vector3r::Zero();
		if(dyn.impose && (dyn.impose->what & Impose::FORCE)) dyn.impose->force(scene,n);
		// zero gravity on clump members, already applied to the clump itself
		if(dyn.isClump()) ClumpData::resetForceTorque(n);
	}
}

// 1st order numerical damping
void Leapfrog::nonviscDamp1st(Vector3r& force, const Vector3r& vel){
	for(int i=0; i<3; i++) force[i]*=1-damping*Mathr::Sign(force[i]*vel[i]);
}
// 2nd order numerical damping
void Leapfrog::nonviscDamp2nd(const Real& dt, const Vector3r& force, const Vector3r& vel, Vector3r& accel){
	for(int i=0; i<3; i++) accel[i]*=1-damping*Mathr::Sign(force[i]*(vel[i]+0.5*dt*accel[i]));
}

Vector3r Leapfrog::computeAccel(const Vector3r& force, const Real& mass, const DemData& dyn){
	if(likely(dyn.isBlockedNone())) return force/mass;
	Vector3r ret(Vector3r::Zero());
	for(int i=0; i<3; i++) if(!(dyn.isBlockedAxisDOF(i,false))) ret[i]+=force[i]/mass;
	return ret;
}
Vector3r Leapfrog::computeAngAccel(const Vector3r& torque, const Vector3r& inertia, const DemData& dyn){
	if(likely(dyn.isBlockedNone())) return (torque.array()/inertia.array()).matrix();
	Vector3r ret(Vector3r::Zero());
	for(int i=0; i<3; i++) if(!(dyn.isBlockedAxisDOF(i,true))) ret[i]+=torque[i]/inertia[i];
	return ret;
}




void Leapfrog::doDampingDissipation(const shared_ptr<Node>& node){
	const DemData& dyn(node->getData<DemData>());
	if(dyn.isEnergySkip()) return;
	/* damping is evaluated incrementally, therefore computed with mid-step values */
	// always positive dissipation, by-component: |F_i|*|v_i|*damping*dt (|T_i|*|ω_i|*damping*dt for rotations)
	scene->energy->add(
		dyn.vel.array().abs().matrix().dot(dyn.force.array().abs().matrix())*damping*scene->dt
		// with aspherical integrator, torque is damped instead of ang acceleration; this is only approximate
		+ dyn.angVel.array().abs().matrix().dot(dyn.torque.array().abs().matrix())*damping*scene->dt
		,"nonviscDamp",nonviscDampIx,EnergyTracker::IsIncrement | EnergyTracker::ZeroDontCreate
	);
}


void Leapfrog::doGravityWork(const DemData& dyn, const DemField& dem){
	/* evaluated using mid-step values */
	Real gr=0;
	if(dyn.isGravitySkip()) return;
	if(dyn.isBlockedNone()) gr=-dem.gravity.dot(dyn.vel)*dyn.mass*dt;
	else { for(int ax:{0,1,2}){ if(!(dyn.isBlockedAxisDOF(ax,false))) gr-=dem.gravity[ax]*dyn.vel[ax]*dyn.mass*dt; } }
	scene->energy->add(gr,"grav",gravWorkIx,EnergyTracker::IsIncrement);
}

void Leapfrog::doKineticEnergy(const shared_ptr<Node>& node, const Vector3r& pprevFluctVel, const Vector3r& pprevFluctAngVel, const Vector3r& linAccel, const Vector3r& angAccel){
	const DemData& dyn(node->getData<DemData>());
	if(dyn.isEnergySkip()) return;
	/* kinetic and potential energy is non-incremental, therefore is evaluated on-step */
	// note: angAccel is zero for aspherical particles; in that case, mid-step value is used
	Vector3r currFluctVel(pprevFluctVel+.5*dt*linAccel), currFluctAngVel(pprevFluctAngVel+.5*dt*angAccel);
	// kinetic energy
	Real Etrans=.5*dyn.mass*currFluctVel.squaredNorm();
	Real Erot;
	// rotational terms
	if(dyn.isAspherical()){
		Matrix3r mI(dyn.inertia.asDiagonal());
		Matrix3r T(node->ori);
		Erot=.5*currFluctAngVel.transpose().dot((T.transpose()*mI*T)*currFluctAngVel);
	} else { Erot=0.5*currFluctAngVel.dot((dyn.inertia.array()*currFluctAngVel.array()).matrix()); }
	if(isnan(Erot) || isinf(dyn.inertia.maxCoeff())) Erot=0;
	if(!kinSplit) scene->energy->add(Etrans+Erot,"kinetic",kinEnergyIx,EnergyTracker::IsResettable);
	else{
		scene->energy->add(Etrans,"kinTrans",kinEnergyTransIx,EnergyTracker::IsResettable);
		scene->energy->add(Erot,"kinRot",kinEnergyRotIx,EnergyTracker::IsResettable);
		// cerr<<"*E+: "<<Etrans<<" (v="<<currFluctVel<<", a="<<linAccel<<", v="<<pprevFluctVel<<")"<<endl;
	}
}


// debugging output, for comparison with OpenCL
//#define DUMP_INTEGRATOR

void Leapfrog::run(){
	if(!reset && !_forceResetChecked){
		shared_ptr<ForceResetter> resetter;
		for(const auto& e: scene->engines){ resetter=dynamic_pointer_cast<ForceResetter>(e); if(resetter) break; }
		if(!resetter) LOG_WARN("Leapfrog.reset==False and no ForceResetter in Scene.engines! Are you sure this is ok? (proceeding)");
		_forceResetChecked=true;
	}
	#ifdef DUMP_INTEGRATOR
		cerr<<std::setprecision(17);
	#endif
	homoDeform=(scene->isPeriodic ? scene->cell->homoDeform : -1); // -1 for aperiodic simulations
	dGradV=scene->cell->nextGradV-scene->cell->gradV;
	midGradV=.5*(scene->cell->gradV+scene->cell->nextGradV);
	//cerr<<"gradV\n"<<scene->cell->gradV<<"\nnextGradV\n"<<scene->cell->nextGradV<<endl;
	dt=scene->dt;
	if(homoDeform==Cell::HOMO_GRADV2){
		const Matrix3r& pprevL(scene->cell->gradV); const Matrix3r& nnextL(scene->cell->nextGradV);
		ImLL4hInv=(Matrix3r::Identity()-dt*(nnextL+pprevL)/4.).inverse();
		IpLL4h   = Matrix3r::Identity()+dt*(nnextL+pprevL)/4.;
		LmL=(nnextL-pprevL); 
		// https://answers.launchpad.net/yade/+question/247806 : am I really sure about the .5? no, I am not!!
		// difference of "spin" (angVel) vectors, which are duals of spin tensor
		deltaSpinVec=-.5*leviCivita((.5*(pprevL-pprevL.transpose())).eval())+.5*leviCivita((.5*(nnextL-nnextL.transpose())).eval());
	}

	const bool isPeriodic(scene->isPeriodic);
	/* don't evaluate energy in the first step with non-zero velocity gradient, since kinetic energy will be way off
	   (meanfield velocity has not yet been applied) */
	const bool reallyTrackEnergy=(scene->trackEnergy&&(!isPeriodic || scene->step>0 || scene->cell->gradV==Matrix3r::Identity()));

	DemField* dem=dynamic_cast<DemField*>(field.get());
	assert(dem);
	bool hasGravity(dem->gravity!=Vector3r::Zero());

	/* temporary hack */
	if(dem->nodes.empty() && !dontCollect){
		int i=dem->collectNodes();
		LOG_WARN("No DEM nodes were defined, "<<i<<" nodes were collected from particles.");
	}
	

	size_t size=dem->nodes.size();
	const auto& nodes=dem->nodes;
	#ifdef WOO_OPENMP
		#pragma omp parallel for schedule(guided)
	#endif
	for(size_t i=0; i<size; i++){
		const shared_ptr<Node>& node=nodes[i];
		assert(node->hasData<DemData>()); // checked in DemField::selfTest
		DemData& dyn(node->getData<DemData>());
		// handle clumps
		if(dyn.isClumped()) continue; // those particles are integrated via the clump's master node
		bool isClump=dyn.isClump();
		bool damp=(damping!=0. && !dyn.isDampingSkip());
		if(isClump){
			// accumulates to existing values of dyn.force, dy.torque (normally zero)
			ClumpData::forceTorqueFromMembers(node,dyn.force,dyn.torque);
		}
		Vector3r& f=dyn.force;
		Vector3r& t=dyn.torque;

		if(unlikely(reallyTrackEnergy)){
			if(damp) doDampingDissipation(node);
			if(hasGravity) doGravityWork(dyn,*dem);
		}
			

		// fluctuation velocity does not contain meanfield velocity in periodic boundaries
		// in aperiodic boundaries, it is equal to absolute velocity
		// it is only computed later, since acceleration is needed to be known already
		Vector3r pprevFluctVel, pprevFluctAngVel;

		// whether to use aspherical rotation integration for this body; for no accelerations, spherical integrator is "exact" (and faster)
		bool useAspherical=(dyn.isAspherical() && (!dyn.isBlockedAllRot()));

		Vector3r linAccel(Vector3r::Zero()), angAccel(Vector3r::Zero());
		// for particles not totally blocked, compute accelerations; otherwise, the computations would be useless
		if (!dyn.isBlockedAll()) {
			linAccel=computeAccel(f,dyn.mass,dyn);
			// fluctuation velocities
			if(isPeriodic){
				pprevFluctVel=scene->cell->pprevFluctVel(node->pos,dyn.vel,dt);
				pprevFluctAngVel=scene->cell->pprevFluctAngVel(dyn.angVel);
			} else { pprevFluctVel=dyn.vel; pprevFluctAngVel=dyn.angVel; }
			// linear damping
			if(damp) nonviscDamp2nd(dt,f,pprevFluctVel,linAccel);
			// compute v(t+dt/2)
			if(homoDeform==Cell::HOMO_GRADV2) dyn.vel=ImLL4hInv*(LmL*node->pos+IpLL4h*dyn.vel+linAccel*dt);
			else dyn.vel+=dt*linAccel;  // correction for this case is below
			// angular acceleration
			if(dyn.inertia!=Vector3r::Zero()){
				if(!useAspherical){ // spherical integrator, uses angular velocity
					angAccel=computeAngAccel(t,dyn.inertia,dyn);
					if(damp) nonviscDamp2nd(dt,t,pprevFluctAngVel,angAccel);
					dyn.angVel+=dt*angAccel;
					if(homoDeform==Cell::HOMO_GRADV2) dyn.angVel-=deltaSpinVec;
				} else { // uses torque
					for(int i=0; i<3; i++) if(dyn.isBlockedAxisDOF(i,true)) t[i]=0; // block DOFs here
					if(damp) nonviscDamp1st(t,pprevFluctAngVel);
				}
			}
			// in case velocity is imposed, it must be re-applied to cancel effects of forces
			// this is not necessary if all DOFs are blocked, since then velocity is not modified
			if(dyn.impose && (dyn.impose->what & Impose::VELOCITY)) dyn.impose->velocity(scene,node);
		}
		else{
			// fixed particle, with gradV2: velocity correction, without acceleration
			if(homoDeform==Cell::HOMO_GRADV2) dyn.vel=ImLL4hInv*(LmL*node->pos+IpLL4h*dyn.vel);
		}
		/* adapt node velocity/position in (t+dt/2) to space gradV;	must be done before position update, so that particle follows */
		// this is for both fixed and free particles, without gradV2
		if(homoDeform>=0 && homoDeform!=Cell::HOMO_GRADV2) applyPeriodicCorrections(node,linAccel);

		// kinetic energy
		// accelerations are needed, therefore not evaluated earlier;
		if(unlikely(reallyTrackEnergy)) doKineticEnergy(node,pprevFluctVel,pprevFluctAngVel,linAccel,angAccel);


		// update positions from velocities
		leapfrogTranslate(node);
		#ifdef DUMP_INTEGRATOR
			if(!dyn.isBlockedAll()) cerr<<", posNew="<<node->pos<<endl;
		#endif
		// update orientation from angular velocity (or torque, for aspherical integrator)
		if(!useAspherical) leapfrogSphericalRotate(node);
		else {
			if(dyn.inertia==Vector3r::Zero()) throw std::runtime_error("Leapfrog::run: DemField.nodes["+to_string(i)+"].den.inertia==(0,0,0), but the node wants to use aspherical integrator. Aspherical integrator is selected for non-spherical particles which have at least one rotational DOF free.");
			if(!isPeriodic) leapfrogAsphericalRotate(node,t);
			else{
				// FIXME: add fake torque from rotating space or modify angMom or angVel
				leapfrogAsphericalRotate(node,t); //-dyn.inertia.asDiagonal()*node->ori.conjugate()*deltaSpinVec/dt*2);
			}
		}

		if(reset){
			// apply gravity only to the clump itself (not to the nodes later, in CLumpData::applyToMembers)
			dyn.force=(hasGravity && !dyn.isGravitySkip())?(dyn.mass*dem->gravity).eval():Vector3r::Zero();
			dyn.torque=Vector3r::Zero();
			if(dyn.impose && (dyn.impose->what & Impose::FORCE)) dyn.impose->force(scene,node);
		}

		// if something is imposed, apply it here
		if(dyn.impose && (dyn.impose->what & Impose::VELOCITY)) dyn.impose->velocity(scene,node);

		// for clumps, update positions/orientations of members as well
		// (gravity already applied to the clump node itself, pass zero here! */
		if(isClump) ClumpData::applyToMembers(node,/*resetForceTorque*/reset);
	}
	// if(isPeriodic) prevVelGrad=scene->cell->velGrad;
}

#ifndef WOO_GRADV2
void Leapfrog::applyPeriodicCorrections(const shared_ptr<Node>& node, const Vector3r& linAccel){
	DemData& dyn(node->getData<DemData>());
	if (homoDeform==Cell::HOMO_VEL || homoDeform==Cell::HOMO_VEL_2ND) {
		// update velocity reflecting changes in the macroscopic velocity field, making the problem homothetic.
		//NOTE : if the velocity is updated before moving the body, it means the current gradV (i.e. before integration in cell->integrateAndUpdate) will be effective for the current time-step. Is it correct? If not, this velocity update can be moved just after "pos += vel*dt", meaning the current velocity impulse will be applied at next iteration, after the contact law. (All this assuming the ordering is resetForces->integrateAndUpdate->contactLaw->PeriCompressor->NewtonsLaw. Any other might fool us.)
		//NOTE : dVel defined without wraping the coordinates means bodies out of the (0,0,0) period can move realy fast. It has to be compensated properly in the definition of relative velocities (see Ig2 functors and contact laws).
		//This is the convective term, appearing in the time derivation of Cundall/Thornton expression (dx/dt=gradV*pos -> d²x/dt²=dGradV/dt+gradV*vel), negligible in many cases but not for high speed large deformations (gaz or turbulent flow).
		if (homoDeform==Cell::HOMO_VEL_2ND) dyn.vel+=midGradV*(dyn.vel-(dt/2.)*linAccel)*dt;
		//In all cases, reflect macroscopic (periodic cell) acceleration in the velocity. This is the dominant term in the update in most cases
		dyn.vel+=dGradV*node->pos;
	} else if (homoDeform==Cell::HOMO_POS){
		node->pos+=scene->cell->nextGradV*node->pos*dt;
	}
	/* should likewise apply angular velocity due to gradV spin, but this has never been done and contact laws don't handle that either */
}
#endif

void Leapfrog::leapfrogTranslate(const shared_ptr<Node>& node){
	DemData& dyn(node->getData<DemData>());
	node->pos+=dyn.vel*dt;
}

void Leapfrog::leapfrogSphericalRotate(const shared_ptr<Node>& node){
	Vector3r axis=node->getData<DemData>().angVel;
	if (axis!=Vector3r::Zero()) {//If we have an angular velocity, we make a rotation
		Real angle=axis.norm(); axis/=angle;
		Quaternionr q(AngleAxisr(angle*dt,axis));
		node->ori=q*node->ori;
	}
	node->ori.normalize();
}

void Leapfrog::leapfrogAsphericalRotate(const shared_ptr<Node>& node, const Vector3r& M){
	Quaternionr& ori(node->ori); DemData& dyn(node->getData<DemData>());
	Vector3r& angMom(dyn.angMom);	Vector3r& angVel(dyn.angVel);	const Vector3r& inertia(dyn.inertia);
	// initialize angular momentum; it is in local coordinates, therefore angVel must be rotated
	if(isnan(angMom.minCoeff())){
		angMom=dyn.inertia.asDiagonal()*node->ori.conjugate()*dyn.angVel;
	}

	Matrix3r A=ori.conjugate().toRotationMatrix(); // rotation matrix from global to local r.f.
	const Vector3r l_n = angMom + dt/2 * M; // global angular momentum at time n
	const Vector3r l_b_n = A*l_n; // local angular momentum at time n
	const Vector3r angVel_b_n = (l_b_n.array()/inertia.array()).matrix(); // local angular velocity at time n
	const Quaternionr dotQ_n=DotQ(angVel_b_n,ori); // dQ/dt at time n
	const Quaternionr Q_half = ori + dt/2 * dotQ_n; // Q at time n+1/2
	angMom+=dt*M; // global angular momentum at time n+1/2
	const Vector3r l_b_half = A*angMom; // local angular momentum at time n+1/2
	Vector3r angVel_b_half = (l_b_half.array()/inertia.array()).matrix(); // local angular velocity at time n+1/2
	const Quaternionr dotQ_half=DotQ(angVel_b_half,Q_half); // dQ/dt at time n+1/2
	ori=ori+dt*dotQ_half; // Q at time n+1
	angVel=ori*angVel_b_half; // global angular velocity at time n+1/2

	ori.normalize();
}

// http://www.euclideanspace.com/physics/kinematics/angularvelocity/QuaternionDifferentiation2.pdf
Quaternionr Leapfrog::DotQ(const Vector3r& angVel, const Quaternionr& Q){
	Quaternionr dotQ;
	dotQ.w() = (-Q.x()*angVel[0]-Q.y()*angVel[1]-Q.z()*angVel[2])/2;
	dotQ.x() = ( Q.w()*angVel[0]-Q.z()*angVel[1]+Q.y()*angVel[2])/2;
	dotQ.y() = ( Q.z()*angVel[0]+Q.w()*angVel[1]-Q.x()*angVel[2])/2;
	dotQ.z() = (-Q.y()*angVel[0]+Q.x()*angVel[1]+Q.w()*angVel[2])/2;
	return dotQ;
}

