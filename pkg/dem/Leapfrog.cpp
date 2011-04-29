#include<yade/pkg/dem/Leapfrog.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/dem/Particle.hpp>

YADE_PLUGIN(dem,(Leapfrog)(ForceResetter));
CREATE_LOGGER(Leapfrog);

void ForceResetter::action(){
	FOREACH(const shared_ptr<Node>& n, scene->field->nodes){
		if(n->dyn) n->dyn->force=n->dyn->torque=Vector3r::Zero();
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

Vector3r Leapfrog::computeAccel(const Vector3r& force, const Real& mass, int blockedDOFs){
	if(likely(blockedDOFs==0)) return force/mass;
	Vector3r ret(Vector3r::Zero());
	for(int i=0; i<3; i++) if(!(blockedDOFs & DynState::axisDOF(i,false))) ret[i]+=force[i]/mass;
	return ret;
}
Vector3r Leapfrog::computeAngAccel(const Vector3r& torque, const Vector3r& inertia, int blockedDOFs){
	if(likely(blockedDOFs==0)) return torque.cwise()/inertia;
	Vector3r ret(Vector3r::Zero());
	for(int i=0; i<3; i++) if(!(blockedDOFs & DynState::axisDOF(i,true))) ret[i]+=torque[i]/inertia[i];
	return ret;
}

void Leapfrog::updateEnergy(const shared_ptr<Node>& node, const Vector3r& fluctVel, const Vector3r& f, const Vector3r& m){
	const DynState& dyn(*node->dyn);
	// always positive dissipation, by-component: |F_i|*|v_i|*damping*dt (|T_i|*|ω_i|*damping*dt for rotations)
	if(damping!=0.){
		scene->energy->add(
			fluctVel.cwise().abs().dot(f.cwise().abs())*damping*scene->dt
			// with aspherical integrator, torque is damped instead of ang acceleration; this is only approximate
			+ dyn.angVel.cwise().abs().dot(m.cwise().abs())*damping*scene->dt
			,"nonviscDamp",nonviscDampIx,/*non-incremental*/false
		);
	}
	// kinetic energy
	Real Etrans=.5*dyn.mass*fluctVel.squaredNorm();
	Real Erot;
	// rotational terms
	if(dyn.isAspherical()){
		Matrix3r mI(dyn.inertia.asDiagonal());
		Matrix3r T(node->ori);
		Erot=.5*dyn.angVel.transpose().dot((T.transpose()*mI*T)*dyn.angVel);
	} else { Erot=0.5*dyn.angVel.dot(dyn.inertia.cwise()*dyn.angVel); }
	if(!kinSplit) scene->energy->add(Etrans+Erot,"kinetic",kinEnergyIx,/*non-incremental*/true);
	else{
		scene->energy->add(Etrans,"kinTrans",kinEnergyTransIx,true);
		scene->energy->add(Erot,"kinRot",kinEnergyRotIx,true);
	}
}

void Leapfrog::action(){
	const Real& dt=scene->dt;
	homoDeform=(scene->isPeriodic ? scene->cell->homoDeform : -1); // -1 for aperiodic simulations
	dVelGrad=scene->cell->velGrad-prevVelGrad;

	const bool trackEnergy(scene->trackEnergy);
	const bool isPeriodic(scene->isPeriodic);

	DemField* field=dynamic_cast<DemField*>(scene->field.get());
	assert(field);

	/* temporary hack */
	if(field->nodes.empty()){
		LOG_WARN("No nodes found, collecting nodes from all particles; shared nodes will be erroneously added multiple times");
		int i=0;
		FOREACH(const shared_ptr<Particle>& p, field->particles){
			if(!p->shape) continue;
			FOREACH(const shared_ptr<Node>& n, p->shape->nodes){
				field->nodes.push_back(n);
				i++;
			}
		}
		LOG_WARN("Collected "<<i<<" nodes.");
	}
	
	FOREACH(const shared_ptr<Node>& node, field->nodes){
		if(!node->dyn) continue;
		DynState& dyn(*node->dyn);
		Vector3r& f=dyn.force;	Vector3r& t=dyn.torque;
		// fluctuation velocity does not contain meanfield velocity in periodic boundaries
		// in aperiodic boundaries, it is equal to absolute velocity
		Vector3r fluctVel=isPeriodic?scene->cell->bodyFluctuationVel(node->pos,dyn.vel):dyn.vel;
		// numerical damping & kinetic energy
		if(unlikely(trackEnergy)) updateEnergy(node,fluctVel,f,t);
		// whether to use aspherical rotation integration for this body; for no accelerations, spherical integrator is "exact" (and faster)
		bool useAspherical=(dyn.isAspherical() && dyn.blockedDOFs!=DynState::DOF_ALL);

		// for particles not totally blocked, compute accelerations; otherwise, the computations would be useless
		if (dyn.blockedDOFs!=DynState::DOF_ALL) {
			Vector3r linAccel=computeAccel(f,dyn.mass,dyn.blockedDOFs);
			nonviscDamp2nd(dt,f,fluctVel,linAccel);
			dyn.vel+=dt*linAccel;
			// angular acceleration
			if(!useAspherical){ // uses angular velocity
				Vector3r angAccel=computeAngAccel(t,dyn.inertia,dyn.blockedDOFs);
				nonviscDamp2nd(dt,t,dyn.angVel,angAccel);
				dyn.angVel+=dt*angAccel;
			} else { // uses torque
				for(int i=0; i<3; i++) if(dyn.blockedDOFs & DynState::axisDOF(i,true)) t[i]=0; // block DOFs here
				nonviscDamp1st(t,dyn.angVel);
			}
		}

		// update positions from velocities (or torque, for the aspherical integrator)
		leapfrogTranslate(node,dt);
		if(!useAspherical) leapfrogSphericalRotate(node,dt);
		else leapfrogAsphericalRotate(node,dt,t);

		if(reset){ dyn.force=dyn.torque=Vector3r::Zero(); }
	}
	if(scene->isPeriodic) { prevCellSize=scene->cell->getSize(); prevVelGrad=scene->cell->velGrad; }
}

void Leapfrog::leapfrogTranslate(const shared_ptr<Node>& node, const Real& dt){
	if (homoDeform==Cell::HOMO_VEL || homoDeform==Cell::HOMO_VEL_2ND) {
		// update velocity reflecting changes in the macroscopic velocity field, making the problem homothetic.
		//NOTE : if the velocity is updated before moving the body, it means the current velGrad (i.e. before integration in cell->integrateAndUpdate) will be effective for the current time-step. Is it correct? If not, this velocity update can be moved just after "node->pos += node->dyn->vel*dt", meaning the current velocity impulse will be applied at next iteration, after the contact law. (All this assuming the ordering is resetForces->integrateAndUpdate->contactLaw->PeriCompressor->NewtonsLaw. Any other might fool us.)
		//NOTE : dVel defined without wraping the coordinates means bodies out of the (0,0,0) period can move realy fast. It has to be compensated properly in the definition of relative velocities (see Ig2 functors and contact laws).
		//This is the convective term, appearing in the time derivation of Cundall/Thornton expression (dx/dt=velGrad*pos -> d²x/dt²=dvelGrad/dt+velGrad*vel), negligible in many cases but not for high speed large deformations (gaz or turbulent flow).
		if (homoDeform==Cell::HOMO_VEL_2ND) node->dyn->vel+=scene->cell->prevVelGrad*node->dyn->vel*dt;

		//In all cases, reflect macroscopic (periodic cell) acceleration in the velocity. This is the dominant term in the update in most cases
		Vector3r dVel=dVelGrad*node->pos;
		node->dyn->vel+=dVel;
	} else if (homoDeform==Cell::HOMO_POS){
		node->pos+=scene->cell->velGrad*node->pos*dt;
	}
	node->pos+=node->dyn->vel*dt;
}

void Leapfrog::leapfrogSphericalRotate(const shared_ptr<Node>& node, const Real& dt){
	Vector3r axis=node->dyn->angVel;
	if (axis!=Vector3r::Zero()) {//If we have an angular velocity, we make a rotation
		Real angle=axis.norm(); axis/=angle;
		Quaternionr q(AngleAxisr(angle*dt,axis));
		node->ori=q*node->ori;
	}
	node->ori.normalize();
}

void Leapfrog::leapfrogAsphericalRotate(const shared_ptr<Node>& node, const Real& dt, const Vector3r& M){
	Quaternionr& ori(node->ori);
	Vector3r& angMom(node->dyn->angMom);
	Vector3r& angVel(node->dyn->angVel);
	const Vector3r& inertia(node->dyn->inertia);

	Matrix3r A=ori.conjugate().toRotationMatrix(); // rotation matrix from global to local r.f.
	const Vector3r l_n = angMom + dt/2 * M; // global angular momentum at time n
	const Vector3r l_b_n = A*l_n; // local angular momentum at time n
	const Vector3r angVel_b_n = l_b_n.cwise()/inertia; // local angular velocity at time n
	const Quaternionr dotQ_n=DotQ(angVel_b_n,ori); // dQ/dt at time n
	const Quaternionr Q_half = ori + dt/2 * dotQ_n; // Q at time n+1/2
	angMom+=dt*M; // global angular momentum at time n+1/2
	const Vector3r l_b_half = A*angMom; // local angular momentum at time n+1/2
	Vector3r angVel_b_half = l_b_half.cwise()/inertia; // local angular velocity at time n+1/2
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

