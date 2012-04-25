#pragma once 

#include<yade/lib/base/Math.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/dem/Particle.hpp>

struct DemFuncs{
	DECLARE_LOGGER;
	static std::tuple</*stress*/Matrix3r,/*stiffness*/Matrix6r> stressStiffness(const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume);
	static Real unbalancedForce(const Scene* scene, const DemField* dem, bool useMaxForce);
	static shared_ptr<Particle> makeSphere(Real radius, const shared_ptr<Material>& m);
	static vector<Vector2r> boxPsd(const Scene* scene, const DemField* dem, const AlignedBox3r& box=AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)), int num=20, int mask=0, Vector2r rRange=Vector2r::Zero());
	static vector<Vector2r> psd(const vector<shared_ptr<Particle>>& p, int num=20, Vector2r rRange=Vector2r::Zero());
};


