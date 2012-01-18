#include<yade/pkg/dem/Particle.hpp>

struct ClumpData: public DemData{
	static shared_ptr<Node> makeClump(const vector<shared_ptr<Node>>&, bool intersecting=false);
	static void collectFromMembers(const shared_ptr<Node>&);
	static void applyToMembers(const shared_ptr<Node>&);

	//! Recalculates inertia tensor of a body after translation away from (default) or towards its centroid.
	static Matrix3r inertiaTensorTranslate(const Matrix3r& I,const Real m, const Vector3r& off);
	//! Recalculate body's inertia tensor in rotated coordinates.
	static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Matrix3r& T);
	//! Recalculate body's inertia tensor in rotated coordinates.
	static Matrix3r inertiaTensorRotate(const Matrix3r& I, const Quaternionr& rot);

	DECLARE_LOGGER;
	YADE_CLASS_BASE_DOC_ATTRS(ClumpData,DemData,"Data of a DEM particle which binds multiple particles together.",
		((vector<shared_ptr<Node>>,nodes,,Attr::readonly,"Member nodes"))
		((vector<Vector3r>,relPos,,Attr::readonly,"Relative member's positions"))
		((vector<Quaternionr>,relOri,,Attr::readonly,"Relative member's orientations"))
	);
};
REGISTER_SERIALIZABLE(ClumpData);
