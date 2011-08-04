#pragma once
#include<yade/core/Engine.hpp>
#include<yade/core/Field.hpp>
#include<yade/pkg/dem/Particle.hpp>
#ifdef YADE_OPENMP
	#include<omp.h>
#endif

class ForceResetter: public GlobalEngine{
	void run();
	YADE_CLASS_BASE_DOC(ForceResetter,GlobalEngine,"Reset forces on nodes in DEM field.");
};
REGISTER_SERIALIZABLE(ForceResetter);

class Leapfrog: public GlobalEngine, private DemField::Engine {
	void nonviscDamp1st(Vector3r& force, const Vector3r& vel);
	void nonviscDamp2nd(const Real& dt, const Vector3r& force, const Vector3r& vel, Vector3r& accel);
	void leapfrogTranslate(const shared_ptr<Node>&, const Real& dt);
	void leapfrogSphericalRotate(const shared_ptr<Node>&, const Real& dt);
	void leapfrogAsphericalRotate(const shared_ptr<Node>&, const Real& dt, const Vector3r& M);
	Quaternionr DotQ(const Vector3r& angVel, const Quaternionr& Q);
	// compute linear and angular acceleration, respecting DemData::blocked
	Vector3r computeAccel(const Vector3r& force, const Real& mass, int blocked);
	Vector3r computeAngAccel(const Vector3r& torque, const Vector3r& inertia, int blocked);
	void updateEnergy(const shared_ptr<Node>&, const Vector3r& fluctVel, const Vector3r& f, const Vector3r& m);
	// whether the cell has changed from the previous step
	bool cellChanged;
	int homoDeform; // updated from scene at every call; -1 for aperiodic simulations, otherwise equal to scene->cell->homoDeform
	Matrix3r dVelGrad; // dtto

	public:
		virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS(Leapfrog,GlobalEngine,"Engine integrating newtonian motion equations, using the leap-frog scheme.",
		((Real,damping,0.2,,"damping coefficient for non-viscous damping"))
		((bool,reset,false,,"Reset forces immediately after applying them."))
		// ((bool,exactAsphericalRot,true,,"Enable more exact body rotation integrator for :yref:`aspherical bodies<Body.aspherical>` *only*, using formulation from [Allen1989]_, pg. 89."))
		((Matrix3r,prevVelGrad,Matrix3r::Zero(),,"Store previous velocity gradient (:yref:`Cell::velGrad`) to track acceleration. |yupdate|"))
		((Vector3r,prevCellSize,Vector3r(NaN,NaN,NaN),Attr::hidden,"cell size from previous step, used to detect change and find max velocity"))
		// energy tracking
		((int,nonviscDampIx,-1,(Attr::hidden|Attr::noSave),"Index of the energy dissipated using the non-viscous damping (:yref:`damping<Leapfrog.damping>`)."))
		((bool,kinSplit,false,,"Whether to separately track translational and rotational kinetic energy."))
		((int,kinEnergyIx,-1,(Attr::hidden|Attr::noSave),"Index for kinetic energy in scene->energies."))
		((int,kinEnergyTransIx,-1,(Attr::hidden|Attr::noSave),"Index for translational kinetic energy in scene->energies."))
		((int,kinEnergyRotIx,-1,(Attr::hidden|Attr::noSave),"Index for rotational kinetic energy in scene->energies."))
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Leapfrog);


