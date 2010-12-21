
#pragma once

#include<yade/core/PartialEngine.hpp>
#include<yade/lib/base/Math.hpp>

class KinematicEngine;

struct CombinedKinematicEngine: public PartialEngine{
	virtual void action();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CombinedKinematicEngine,PartialEngine,"Engine for applying combined displacements on pre-defined bodies. Constructed using ``+`` operator on regular :yref:`KinematicEngines<KinematicEngine>`. The ``ids`` operated on are those of the first engine in the combination (assigned automatically).",
		((vector<shared_ptr<KinematicEngine> >,comb,,,"Kinematic engines that will be combined by this one, run in the order given."))
		, /* ctor */
		, /* py */  .def("__add__",&CombinedKinematicEngine::appendOne)
	);
	// exposed as operator + in python
	static const shared_ptr<CombinedKinematicEngine> appendOne(const shared_ptr<CombinedKinematicEngine>& self, const shared_ptr<KinematicEngine>& other){ self->comb.push_back(other); return self; }
	static const shared_ptr<CombinedKinematicEngine> fromTwo(const shared_ptr<KinematicEngine>& first, const shared_ptr<KinematicEngine>& second);
};
REGISTER_SERIALIZABLE(CombinedKinematicEngine);

struct KinematicEngine: public PartialEngine{
	virtual void action();
	virtual void apply(const vector<Body::id_t>& ids){ LOG_ERROR("KinematicEngine::apply called, derived class ("<<getClassName()<<") did not override that method?"); }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(KinematicEngine,PartialEngine,"Abstract engine for applying prescribed displacement.\n\n.. note:: Derived classes should override the ``apply`` with given list of ``ids`` (not ``action`` with :yref:`PartialEngine.ids`), so that they work when combined together; :yref:`velocity<State.vel>` and :yref:`angular velocity<State.angVel>` of all subscribed bodies is reset before the ``apply`` method is called, it should therefore only increment those quantities.",
		/* attrs*/, /* ctor */, /* py */ .def("__add__",&CombinedKinematicEngine::fromTwo) 
	)
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(KinematicEngine);


struct TranslationEngine: public KinematicEngine{
	virtual void apply(const vector<Body::id_t>& ids);
	void postLoad(TranslationEngine&){ translationAxis.normalize(); }
	YADE_CLASS_BASE_DOC_ATTRS(TranslationEngine,KinematicEngine,"This engine is the base class for different engines, which require any kind of motion.",
		((Real,velocity,,,"Velocity [m/s]"))
		((Vector3r,translationAxis,,Attr::triggerPostLoad,"Direction [Vector3]"))
	);
};
REGISTER_SERIALIZABLE(TranslationEngine);

struct HarmonicMotionEngine: public KinematicEngine{
	virtual void apply(const vector<Body::id_t>& ids);
	YADE_CLASS_BASE_DOC_ATTRS(HarmonicMotionEngine,KinematicEngine,"This engine implements the harmonic oscillation of bodies. http://en.wikipedia.org/wiki/Simple_harmonic_motion#Dynamics_of_simple_harmonic_motion",
		((Vector3r,A,Vector3r::Zero(),,"Amplitude [m]"))
		((Vector3r,f,Vector3r::Zero(),,"Frequency [hertz]"))
		((Vector3r,fi,Vector3r(Mathr::PI/2.0, Mathr::PI/2.0, Mathr::PI/2.0),,"Initial phase [radians]. By default, the body oscillates around initial position."))
	);
};
REGISTER_SERIALIZABLE(HarmonicMotionEngine);

struct RotationEngine: public KinematicEngine{
	virtual void apply(const vector<Body::id_t>& ids);
	void postLoad(RotationEngine&){ rotationAxis.normalize(); }
	YADE_CLASS_BASE_DOC_ATTRS(RotationEngine,KinematicEngine,"Engine applying rotation (by setting angular velocity) to subscribed bodies. If :yref:`rotateAroundZero<RotationEngine.rotateAroundZero>` is set, then each body is also displaced around :yref:`zeroPoint<RotationEngine.zeroPoint>`.",
		((Real,angularVelocity,0,,"Angular velocity. [rad/s]"))
		((Vector3r,rotationAxis,Vector3r::UnitX(),Attr::triggerPostLoad,"Axis of rotation (direction); will be normalized automatically."))
		((bool,rotateAroundZero,false,,"If True, bodies will not rotate around their centroids, but rather around ``zeroPoint``."))
		((Vector3r,zeroPoint,Vector3r::Zero(),,"Point around which bodies will rotate if ``rotateAroundZero`` is True"))
	);
};
REGISTER_SERIALIZABLE(RotationEngine);

struct HelixEngine:public KinematicEngine{
	virtual void apply(const vector<Body::id_t>& ids);
	void postLoad(HelixEngine&){ axis.normalize(); }
	YADE_CLASS_BASE_DOC_ATTRS(HelixEngine,KinematicEngine,"Engine applying both rotation and translation, along the same axis, whence the name HelixEngine",
		((Real,angularVelocity,0,,"Angular velocity [rad/s]"))
		((Real,linearVelocity,0,,"Linear velocity [m/s]"))
		((Vector3r,axis,Vector3r::UnitX(),Attr::triggerPostLoad,"Axis of translation and rotation; will be normalized by the engine."))
		((Vector3r,axisPt,Vector3r::Zero(),,"A point on the axis, to position it in space properly."))
		((Real,angleTurned,0,,"How much have we turned so far. |yupdate| [rad]"))
	);
};
REGISTER_SERIALIZABLE(HelixEngine);

struct InterpolatingHelixEngine: public HelixEngine{
	virtual void apply(const vector<Body::id_t>& ids);
	YADE_CLASS_BASE_DOC_ATTRS(InterpolatingHelixEngine,HelixEngine,"Engine applying spiral motion, finding current angular velocity by linearly interpolating in times and velocities and translation by using slope parameter. \n\n The interpolation assumes the margin value before the first time point and last value after the last time point. If wrap is specified, time will wrap around the last times value to the first one (note that no interpolation between last and first values is done).",
		((vector<Real>,times,,,"List of time points at which velocities are given; must be increasing [s]"))
		((vector<Real>,angularVelocities,,,"List of angular velocities; manadatorily of same length as times. [rad/s]"))
		((bool,wrap,false,,"Wrap t if t>times_n, i.e. t_wrapped=t-N*(times_n-times_0)"))
		((Real,slope,0,,"Axial translation per radian turn (can be negative) [m/rad]"))
		((size_t,_pos,0,(Attr::hidden),"holder of interpolation state, should not be touched by user"))
	);
};
REGISTER_SERIALIZABLE(InterpolatingHelixEngine);

struct HarmonicRotationEngine: public RotationEngine{
	virtual void apply(const vector<Body::id_t>& ids);
	YADE_CLASS_BASE_DOC_ATTRS(HarmonicRotationEngine,RotationEngine,"This engine implements the harmonic-rotation oscillation of bodies. http://en.wikipedia.org/wiki/Simple_harmonic_motion#Dynamics_of_simple_harmonic_motion ; please, set dynamic=False for bodies, droven by this engine, otherwise amplitude will be 2x more, than awaited.",
		((Real,A,0,,"Amplitude [rad]"))
		((Real,f,0,,"Frequency [hertz]"))
		((Real,fi,Mathr::PI/2.0,,"Initial phase [radians]. By default, the body oscillates around initial position."))
	);
};
REGISTER_SERIALIZABLE(HarmonicRotationEngine);

