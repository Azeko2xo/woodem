#pragma once
#include<yade/pkg/dem/Particle.hpp>

struct ClumpData: public DemData{
	static shared_ptr<Node> makeClump(const vector<shared_ptr<Node>>&, bool intersecting=false);
	static void collectFromMembers(const shared_ptr<Node>&);
	static void applyToMembers(const shared_ptr<Node>&, bool resetForceTorque=false);
	static void resetForceTorque(const shared_ptr<Node>&);

	//! Recalculates inertia tensor of a body after translation away from (default) or towards its centroid.
	static Matrix3r inertiaTensorTranslate(const Matrix3r& I,const Real m, const Vector3r& off);
	//! Recalculate body's inertia tensor in rotated coordinates.
	static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Matrix3r& T);
	//! Recalculate body's inertia tensor in rotated coordinates.
	static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Quaternionr& rot);

	DECLARE_LOGGER;
	YADE_CLASS_BASE_DOC_ATTRS(ClumpData,DemData,"Data of a DEM particle which binds multiple particles together.",
		((vector<shared_ptr<Node>>,nodes,,AttrTrait<Attr::readonly>(),"Member nodes"))
		((vector<Particle::id_t>,memberIds,,AttrTrait<Attr::readonly>(),"Ids of member particles (used only for deletion)"))
		((vector<Vector3r>,relPos,,AttrTrait<Attr::readonly>(),"Relative member's positions"))
		((vector<Quaternionr>,relOri,,AttrTrait<Attr::readonly>(),"Relative member's orientations"))
		((long,clumpLinIx,-1,AttrTrait<Attr::hidden>(),"Index in the O.dem.clumps array, for efficient deletion."))
	);
};
REGISTER_SERIALIZABLE(ClumpData);