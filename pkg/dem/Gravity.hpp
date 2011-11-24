#pragma once
#include<yade/core/Engine.hpp>
#include<yade/pkg/dem/Particle.hpp>

struct Gravity: public GlobalEngine, private DemField::Engine{
	virtual void run();
	void pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw);
	YADE_CLASS_BASE_DOC_ATTRS(Gravity,GlobalEngine,"Engine applying constant acceleration to all bodies.",
		((Vector3r,gravity,Vector3r::Zero(),,"Acceleration [kgms⁻²]"))
		((int,gravWorkIx,-1,(Attr::noSave|Attr::hidden),"Index for work of gravity"))
		// ((int,mask,0,,"If mask defined, only bodies with corresponding groupMask will be affected by this engine. If 0, all bodies will be affected."))
	);
};
REGISTER_SERIALIZABLE(Gravity);

struct AxialGravity: public GlobalEngine, private DemField::Engine {
	virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS(AxialGravity,GlobalEngine,"Apply acceleration (independent of distance) directed towards an axis.",
		((Vector3r,axisPt,Vector3r::Zero(),,"Point through which the axis is passing."))
		((Vector3r,axisDir,Vector3r::UnitX(),,"direction of the gravity axis (will be normalized automatically)"))
		((Real,accel,0,,"Acceleration magnitude [kgms⁻²]"))
		// ((int,mask,0,,"If mask defined, only bodies with corresponding groupMask will be affected by this engine. If 0, all bodies will be affected."))
	);
};
REGISTER_SERIALIZABLE(AxialGravity);

#if 0

/*! Engine attracting all bodies towards a central body (doesn't depend on distance);
 *
 * @todo This code has not been yet tested at all.
 */
class CentralGravityEngine: public FieldApplier {
	public:
		virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(CentralGravityEngine,FieldApplier,"Engine applying acceleration to all bodies, towards a central body.",
		((Body::id_t,centralBody,Body::ID_NONE,,"The :yref:`body<Body>` towards which all other bodies are attracted."))
		((Real,accel,0,,"Acceleration magnitude [kgms⁻²]"))
		((bool,reciprocal,false,,"If true, acceleration will be applied on the central body as well."))
		((int,mask,0,,"If mask defined, only bodies with corresponding groupMask will be affected by this engine. If 0, all bodies will be affected."))
		,,
	);
};
REGISTER_SERIALIZABLE(CentralGravityEngine);

/*! Apply acceleration (independent of distance) directed towards an axis.
 *
 */

class HdapsGravityEngine: public GravityEngine{
	public:
	Vector2i readSysfsFile(const std::string& name);
	virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS(HdapsGravityEngine,GravityEngine,"Read accelerometer in Thinkpad laptops (`HDAPS <http://en.wikipedia.org/wiki/Active_hard_drive_protection>`__ and accordingly set gravity within the simulation. This code draws from `hdaps-gl <https://sourceforge.net/project/showfiles.php?group_id=138242>`__ . See :ysrc:`scripts/test/hdaps.py` for an example.",
		((string,hdapsDir,"/sys/devices/platform/hdaps",,"Hdaps directory; contains ``position`` (with accelerometer readings) and ``calibration`` (zero acceleration)."))
		((Real,msecUpdate,50,,"How often to update the reading."))
		((int,updateThreshold,4,,"Minimum difference of reading from the file before updating gravity, to avoid jitter."))
		((Real,lastReading,-1,(Attr::hidden|Attr::noSave),"Time of the last reading."))
		((Vector2i,accel,Vector2i::Zero(),(Attr::noSave|Attr::readonly),"reading from the sysfs file"))
		((Vector2i,calibrate,Vector2i::Zero(),,"Zero position; if NaN, will be read from the *hdapsDir* / calibrate."))
		((bool,calibrated,false,,"Whether *calibrate* was already updated. Do not set to ``True`` by hand unless you also give a meaningful value for *calibrate*."))
		((Vector3r,zeroGravity,Vector3r(0,0,-1),,"Gravity if the accelerometer is in flat (zero) position."))
	);
};
REGISTER_SERIALIZABLE(HdapsGravityEngine);
#endif
