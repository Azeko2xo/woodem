#pragma once
#include<woo/core/Engine.hpp>
#include<woo/core/Field.hpp>
#include<woo/pkg/dem/Particle.hpp>
#ifdef WOO_OPENMP
	#include<omp.h>
#endif
struct ForceResetter: public GlobalEngine{
	void run();
	WOO_CLASS_BASE_DOC(ForceResetter,GlobalEngine,"Reset forces on nodes in DEM field.");
};
REGISTER_SERIALIZABLE(ForceResetter);

struct Leapfrog: public GlobalEngine {
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }

	void nonviscDamp1st(Vector3r& force, const Vector3r& vel);
	void nonviscDamp2nd(const Real& dt, const Vector3r& force, const Vector3r& vel, Vector3r& accel);
	void applyPeriodicCorrections(const shared_ptr<Node>&, const Vector3r& linAccel);
	void leapfrogTranslate(const shared_ptr<Node>&);
	void leapfrogSphericalRotate(const shared_ptr<Node>&);
	void leapfrogAsphericalRotate(const shared_ptr<Node>&, const Vector3r& M);
	Quaternionr DotQ(const Vector3r& angVel, const Quaternionr& Q);
	// compute linear and angular acceleration, respecting DemData::blocked
	Vector3r computeAccel(const Vector3r& force, const Real& mass, /* for passing blocked DoFs */const DemData& dyn);
	Vector3r computeAngAccel(const Vector3r& torque, const Vector3r& inertia, const DemData& dyn);
	void doKineticEnergy(const shared_ptr<Node>&, const Vector3r& pprevFluctVel, const Vector3r& pprevFluctAngVel, const Vector3r& linAccel, const Vector3r& angAccel);
	void doDampingDissipation(const shared_ptr<Node>&);
	// whether the cell has changed from the previous step
	int homoDeform; // updated from scene at every call; -1 for aperiodic simulations, otherwise equal to scene->cell->homoDeform
	Real dt; // updated from scene at every call
	Matrix3r dGradV, midGradV; // dtto

	// used with homoDeform==Cell::HOMO_GRADV2
	//Matrix3r IpLL4,ImLL4Inv, LmL; // (1-(nextGradV+gradV)/4).inverse()
	//Vector3r deltaSpinVec;


	public:
		virtual void run();
	WOO_CLASS_BASE_DOC_ATTRS_CTOR(Leapfrog,GlobalEngine,"Engine integrating newtonian motion equations, using the leap-frog scheme.",
		// cached values
		((Matrix3r,IpLL4h,,AttrTrait<Attr::readonly|Attr::noSave>(),"I+(nnextL+pprevL)/4"))
		((Matrix3r,ImLL4hInv,,AttrTrait<Attr::readonly|Attr::noSave>(),"(I-(nnextL+pprevL)/4)^-1"))
		((Matrix3r,LmL,,AttrTrait<Attr::readonly|Attr::noSave>(),"(nnextL-pprevL)/dt"))
		((Vector3r,deltaSpinVec,,AttrTrait<Attr::readonly|Attr::noSave>(),""))

		((Real,damping,0.2,,"damping coefficient for non-viscous damping"))
		((bool,reset,false,,"Reset forces immediately after applying them."))
		((bool,_forceResetChecked,false,AttrTrait<>().noGui(),"Whether we already issued a warning for forces being (probably) not reset"))
		// ((Matrix3r,prevVelGrad,Matrix3r::Zero(),AttrTrait<Attr::readonly>(),"Previous value of velocity gradient, to detect its changes"))
		// ((bool,exactAsphericalRot,true,,"Enable more exact body rotation integrator for :ref:`aspherical bodies<Body.aspherical>` *only*, using formulation from [Allen1989]_, pg. 89."))
		// energy tracking
		((int,nonviscDampIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index of the energy dissipated using the non-viscous damping (:ref:`damping<Leapfrog.damping>`)."))
		((bool,kinSplit,false,,"Whether to separately track translational and rotational kinetic energy."))
		((Real,maxVelocitySq,NaN,AttrTrait<Attr::readonly>(),"store square of max. velocity, for informative purposes; computed again at every step."))

		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene->energies."))
		((int,kinEnergyTransIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for translational kinetic energy in scene->energies."))
		((int,kinEnergyRotIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for rotational kinetic energy in scene->energies."))
		// ((Matrix3r,nextGradV,(Matrix3r()<<NaN,NaN,NaN, NaN,NaN,NaN, NaN,NaN,NaN).finished(),,"Use to set new value of Cell.gradV, setting it directly between steps will produce spurious jumps in kinetic energy due to Cell.homoDeform corrections. Once applied, it will be automatically set to an invalid value (which is NaN at [0,0])"))
		,
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Leapfrog);

