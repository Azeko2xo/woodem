#pragma once
#include<woo/core/Engine.hpp>
#include<woo/pkg/dem/Particle.hpp>

struct AxialGravity: public Engine {
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	virtual void run();
	WOO_CLASS_BASE_DOC_ATTRS(AxialGravity,Engine,"Apply acceleration (independent of distance) directed towards an axis.",
		((Vector3r,axisPt,Vector3r::Zero(),,"Point through which the axis is passing."))
		((Vector3r,axisDir,Vector3r::UnitX(),,"direction of the gravity axis (will be normalized automatically)"))
		((Real,accel,0,,"Acceleration magnitude [kgms⁻²]"))
		// ((int,mask,0,,"If mask defined, only bodies with corresponding groupMask will be affected by this engine. If 0, all bodies will be affected."))
	);
};
WOO_REGISTER_OBJECT(AxialGravity);

#if 0

/*! Engine attracting all bodies towards a central body (doesn't depend on distance);
 *
 * @todo This code has not been yet tested at all.
 */
class CentralGravityEngine: public FieldApplier {
	public:
		virtual void run();
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(CentralGravityEngine,FieldApplier,"Engine applying acceleration to all bodies, towards a central body.",
		((Body::id_t,centralBody,Body::ID_NONE,,"The :obj:`body<Body>` towards which all other bodies are attracted."))
		((Real,accel,0,,"Acceleration magnitude [kgms⁻²]"))
		((bool,reciprocal,false,,"If true, acceleration will be applied on the central body as well."))
		((int,mask,0,,"If mask defined, only bodies with corresponding groupMask will be affected by this engine. If 0, all bodies will be affected."))
		,,
	);
};
WOO_REGISTER_OBJECT(CentralGravityEngine);

/*! Apply acceleration (independent of distance) directed towards an axis.
 *
 */

class HdapsGravityEngine: public GravityEngine{
	public:
	Vector2i readSysfsFile(const std::string& name);
	virtual void run();
	WOO_CLASS_BASE_DOC_ATTRS(HdapsGravityEngine,GravityEngine,"Read accelerometer in Thinkpad laptops (`HDAPS <http://en.wikipedia.org/wiki/Active_hard_drive_protection>`__ and accordingly set gravity within the simulation. This code draws from `hdaps-gl <https://sourceforge.net/project/showfiles.php?group_id=138242>`__ . See :woosrc:`scripts/test/hdaps.py` for an example.",
		((string,hdapsDir,"/sys/devices/platform/hdaps",,"Hdaps directory; contains ``position`` (with accelerometer readings) and ``calibration`` (zero acceleration)."))
		((Real,msecUpdate,50,,"How often to update the reading."))
		((int,updateThreshold,4,,"Minimum difference of reading from the file before updating gravity, to avoid jitter."))
		((Real,lastReading,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Time of the last reading."))
		((Vector2i,accel,Vector2i::Zero(),AttrTrait<Attr::noSave|Attr::readonly>(),"reading from the sysfs file"))
		((Vector2i,calibrate,Vector2i::Zero(),,"Zero position; if NaN, will be read from the *hdapsDir* / calibrate."))
		((bool,calibrated,false,,"Whether *calibrate* was already updated. Do not set to ``True`` by hand unless you also give a meaningful value for *calibrate*."))
		((Vector3r,zeroGravity,Vector3r(0,0,-1),,"Gravity if the accelerometer is in flat (zero) position."))
	);
};
WOO_REGISTER_OBJECT(HdapsGravityEngine);
#endif
