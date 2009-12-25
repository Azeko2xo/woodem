/*************************************************************************
 Copyright (C) 2008 by Bruno Chareyre		                         *
*  bruno.chareyre@hmg.inpg.fr      					 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include"NewtonIntegrator.hpp"
#include<yade/core/Scene.hpp>
#include<yade/pkg-dem/Clump.hpp>
#include<yade/pkg-common/VelocityBins.hpp>
#include<yade/lib-base/yadeWm3Extra.hpp>



YADE_PLUGIN((NewtonIntegrator));
CREATE_LOGGER(NewtonIntegrator);
void NewtonIntegrator::cundallDamp(const Real& dt, const Vector3r& N, const Vector3r& V, Vector3r& A){
	for(int i=0; i<3; i++) A[i]*= 1 - damping*Mathr::Sign ( N[i]*(V[i] + (Real) 0.5 *dt*A[i]) );
}
void NewtonIntegrator::blockTranslateDOFs(unsigned blockedDOFs, Vector3r& v) {
	if(blockedDOFs==State::DOF_NONE) return;
	if(blockedDOFs==State::DOF_ALL) { v=Vector3r::ZERO; return; }
	if((blockedDOFs & State::DOF_X)!=0) v[0]=0;
	if((blockedDOFs & State::DOF_Y)!=0) v[1]=0;
	if((blockedDOFs & State::DOF_Z)!=0) v[2]=0;
}
void NewtonIntegrator::blockRotateDOFs(unsigned blockedDOFs, Vector3r& v) {
	if(blockedDOFs==State::DOF_NONE) return;
	if(blockedDOFs==State::DOF_ALL) { v=Vector3r::ZERO; return; }
	if((blockedDOFs & State::DOF_RX)!=0) v[0]=0;
	if((blockedDOFs & State::DOF_RY)!=0) v[1]=0;
	if((blockedDOFs & State::DOF_RZ)!=0) v[2]=0;
}
void NewtonIntegrator::handleClumpMemberAccel(Scene* scene, const body_id_t& memberId, State* memberState, State* clumpState){
	const Vector3r& f=scene->forces.getForce(memberId);
	Vector3r diffClumpAccel=f/clumpState->mass;
	// damp increment of accels on the clump, using velocities of the clump MEMBER
	cundallDamp(scene->dt,f,memberState->vel,diffClumpAccel);
	clumpState->accel+=diffClumpAccel;
}
void NewtonIntegrator::handleClumpMemberAngAccel(Scene* scene, const body_id_t& memberId, State* memberState, State* clumpState){
	// angular acceleration from: normal torque + torque generated by the force WRT particle centroid on the clump centroid
	const Vector3r& m=scene->forces.getTorque(memberId); const Vector3r& f=scene->forces.getForce(memberId);
	Vector3r diffClumpAngularAccel=diagDiv(m,clumpState->inertia)+diagDiv((memberState->pos-clumpState->pos).Cross(f),clumpState->inertia); 
	// damp increment of accels on the clump, using velocities of the clump MEMBER
	cundallDamp(scene->dt,m,memberState->angVel,diffClumpAngularAccel);
	clumpState->angAccel+=diffClumpAngularAccel;
}
void NewtonIntegrator::handleClumpMemberTorque(Scene* scene, const body_id_t& memberId, State* memberState, State* clumpState, Vector3r& M){
	const Vector3r& m=scene->forces.getTorque(memberId); const Vector3r& f=scene->forces.getForce(memberId);
	M+=(memberState->pos-clumpState->pos).Cross(f)+m;
}
void NewtonIntegrator::saveMaximaVelocity(Scene* scene, const body_id_t& id, State* state){
	if(haveBins) velocityBins->binVelSqUse(id,VelocityBins::getBodyVelSq(state));
	#ifdef YADE_OPENMP
		Real& thrMaxVSq=threadMaxVelocitySq[omp_get_thread_num()]; thrMaxVSq=max(thrMaxVSq,state->vel.SquaredLength());
	#else
		maxVelocitySq=max(maxVelocitySq,state->vel.SquaredLength());
	#endif
}

void NewtonIntegrator::action(Scene*)
{
	// only temporary
#ifndef VELGRAD
	if(homotheticCellResize) throw runtime_error("Homothetic resizing not yet implemented with new, core/Cell.hpp based cell (extension+shear).");
#endif

	scene->forces.sync();
	Real dt=scene->dt;
	// account for motion of the periodic boundary, if we remember its last position
	// its velocity will count as max velocity of bodies
	// otherwise the collider might not run if only the cell were changing without any particle motion
	if(scene->isPeriodic && (prevCellSize!=scene->cell->getSize())){ cellChanged=true; maxVelocitySq=(prevCellSize-scene->cell->getSize()).SquaredLength()/pow(dt,2); }
	else { maxVelocitySq=0; cellChanged=false; }
	haveBins=(bool)velocityBins;
	if(haveBins) velocityBins->binVelSqInitialize(maxVelocitySq);

	#ifdef YADE_OPENMP
		FOREACH(Real& thrMaxVSq, threadMaxVelocitySq) { thrMaxVSq=0; }
		const BodyContainer& bodies=*(scene->bodies.get());
		const long size=(long)bodies.size();
		#pragma omp parallel for schedule(static)
		for(long _id=0; _id<size; _id++){
			const shared_ptr<Body>& b(bodies[_id]);
	#else
		FOREACH(const shared_ptr<Body>& b, *scene->bodies){
	#endif
			if(!b) continue;
			State* state=b->state.get();
			const body_id_t& id=b->getId();
			// clump members are non-dynamic; we only get their velocities here
			if (!b->isDynamic || b->isClumpMember()){
				saveMaximaVelocity(scene,id,state);
				continue;
			}

			if (b->isStandalone()){
				// translate equation
				const Vector3r& f=scene->forces.getForce(id); 
				state->accel=f/state->mass; 
				cundallDamp(dt,f,state->vel,state->accel); 
				leapfrogTranslate(scene,state,id,dt);
				// rotate equation
				if (!exactAsphericalRot){ // exactAsphericalRot disabled
					const Vector3r& m=scene->forces.getTorque(id); 
					state->angAccel=diagDiv(m,state->inertia);
					cundallDamp(dt,m,state->angVel,state->angAccel);
					leapfrogSphericalRotate(scene,state,id,dt);
				} else { // exactAsphericalRot enabled
					const Vector3r& m=scene->forces.getTorque(id); 
					leapfrogAsphericalRotate(scene,state,id,dt,m);
				}
			} else if (b->isClump()){
				// clump mass forces
				const Vector3r& f=scene->forces.getForce(id);
				Vector3r dLinAccel=f/state->mass;
				cundallDamp(dt,f,state->vel,dLinAccel);
				state->accel+=dLinAccel;
				const Vector3r& m=scene->forces.getTorque(id);
				Vector3r M(m);
				// sum force on clump memebrs
				if (exactAsphericalRot){
					FOREACH(Clump::memberMap::value_type mm, static_cast<Clump*>(b.get())->members){
						const body_id_t& memberId=mm.first;
						const shared_ptr<Body>& member=Body::byId(memberId,scene); assert(b->isClumpMember());
						State* memberState=member->state.get();
						handleClumpMemberAccel(scene,memberId,memberState,state);
						handleClumpMemberTorque(scene,memberId,memberState,state,M);
						saveMaximaVelocity(scene,memberId,memberState);
					}
					// motion
					leapfrogTranslate(scene,state,id,dt);
					leapfrogAsphericalRotate(scene,state,id,dt,M);
				} else { // exactAsphericalRot disabled
					Vector3r dAngAccel=diagDiv(M,state->inertia);
					cundallDamp(dt,M,state->angVel,dAngAccel);
					state->angAccel+=dAngAccel;
					FOREACH(Clump::memberMap::value_type mm, static_cast<Clump*>(b.get())->members){
						const body_id_t& memberId=mm.first;
						const shared_ptr<Body>& member=Body::byId(memberId,scene); assert(member->isClumpMember());
						State* memberState=member->state.get();
						handleClumpMemberAccel(scene,memberId,memberState,state);
						handleClumpMemberAngAccel(scene,memberId,memberState,state);
						saveMaximaVelocity(scene,memberId,memberState);
					}
					// motion
					leapfrogTranslate(scene,state,id,dt);
					leapfrogSphericalRotate(scene,state,id,dt);
				}
				static_cast<Clump*>(b.get())->moveMembers();
			}
			saveMaximaVelocity(scene,id,state);
	}
	#ifdef YADE_OPENMP
		FOREACH(const Real& thrMaxVSq, threadMaxVelocitySq) { maxVelocitySq=max(maxVelocitySq,thrMaxVSq); }
	#endif
	if(haveBins) velocityBins->binVelSqFinalize();
	if(scene->isPeriodic) { prevCellSize=scene->cell->getSize(); }
}

inline void NewtonIntegrator::leapfrogTranslate(Scene* scene, State* state, const body_id_t& id, const Real& dt )
{
	// for homothetic resize of the cell (if enabled), compute the position difference of the homothetic transformation
	// the term ξ=(x'-x₀')/(x₁'-x₀') is normalized cell coordinate (' meaning at previous step),
	// which is then used to compute new position x=ξ(x₁-x₀)+x₀. (per component)
	// Then we update either velocity by (x-x')/Δt (homotheticCellResize==1)
	//   or position by (x-x') (homotheticCellResize==2)
	// FIXME: wrap state->pos first, then apply the shift; otherwise result might be garbage
	Vector3r dPos(Vector3r::ZERO); // initialize to avoid warning; find way to avoid it in a better way
#ifndef VELGRAD
	if(cellChanged && homotheticCellResize){ for(int i=0; i<3; i++) dPos[i]=(state->pos[i]/prevCellSize[i])*scene->cell->getSize()[i]-state->pos[i]; }
#else
	if(homotheticCellResize){ dPos = scene->cell->getShearIncrMatrix()*scene->cell->wrapShearedPt(state->pos);
	}
#endif	
	blockTranslateDOFs(state->blockedDOFs, state->accel);
	state->vel+=dt*state->accel;
	state->pos += state->vel*dt + scene->forces.getMove(id);
	//apply cell deformation
	if(homotheticCellResize>=1) state->pos+=dPos;
	//update velocity for usage in rate dependant equations (e.g. viscous law) FIXME : it is not recommended to do that because it impacts the dynamics (this modified velocity will be used as reference in the next time-step)
	if(homotheticCellResize==2) state->vel+=dPos/dt;

}
inline void NewtonIntegrator::leapfrogSphericalRotate(Scene* scene, State* state, const body_id_t& id, const Real& dt )
{
	blockRotateDOFs(state->blockedDOFs, state->angAccel);
	state->angVel+=dt*state->angAccel;
	Vector3r axis = state->angVel; Real angle = axis.Normalize();
	Quaternionr q; q.FromAxisAngle ( axis,angle*dt );
	state->ori = q*state->ori;
	if(scene->forces.getMoveRotUsed() && scene->forces.getRot(id)!=Vector3r::ZERO){ Vector3r r(scene->forces.getRot(id)); Real norm=r.Normalize(); Quaternionr q; q.FromAxisAngle(r,norm); state->ori=q*state->ori; }
	state->ori.Normalize();
}
void NewtonIntegrator::leapfrogAsphericalRotate(Scene* scene, State* state, const body_id_t& id, const Real& dt, const Vector3r& M){
	Matrix3r A; state->ori.Conjugate().ToRotationMatrix(A); // rotation matrix from global to local r.f.
	const Vector3r l_n = state->angMom + dt/2 * M; // global angular momentum at time n
	const Vector3r l_b_n = A*l_n; // local angular momentum at time n
	const Vector3r angVel_b_n = diagDiv(l_b_n,state->inertia); // local angular velocity at time n
	const Quaternionr dotQ_n=DotQ(angVel_b_n,state->ori); // dQ/dt at time n
	const Quaternionr Q_half = state->ori + dt/2 * dotQ_n; // Q at time n+1/2
	state->angMom+=dt*M; // global angular momentum at time n+1/2
	const Vector3r l_b_half = A*state->angMom; // local angular momentum at time n+1/2
	Vector3r angVel_b_half = diagDiv(l_b_half,state->inertia); // local angular velocity at time n+1/2
	blockRotateDOFs( state->blockedDOFs, angVel_b_half );
	const Quaternionr dotQ_half=DotQ(angVel_b_half,Q_half); // dQ/dt at time n+1/2
	state->ori+=dt*dotQ_half; // Q at time n+1
	state->angVel=state->ori.Rotate(angVel_b_half); // global angular velocity at time n+1/2
	if(scene->forces.getMoveRotUsed() && scene->forces.getRot(id)!=Vector3r::ZERO){ Vector3r r(scene->forces.getRot(id)); Real norm=r.Normalize(); Quaternionr q; q.FromAxisAngle(r,norm); state->ori=q*state->ori; }
	state->ori.Normalize(); 
}
	
Quaternionr NewtonIntegrator::DotQ(const Vector3r& angVel, const Quaternionr& Q){
	Quaternionr dotQ(Quaternionr::ZERO);
	dotQ[0] = (-Q[1]*angVel[0]-Q[2]*angVel[1]-Q[3]*angVel[2])/2;
	dotQ[1] = ( Q[0]*angVel[0]-Q[3]*angVel[1]+Q[2]*angVel[2])/2;
	dotQ[2] = ( Q[3]*angVel[0]+Q[0]*angVel[1]-Q[1]*angVel[2])/2;
	dotQ[3] = (-Q[2]*angVel[0]+Q[1]*angVel[1]+Q[0]*angVel[2])/2;
	return dotQ;
}
