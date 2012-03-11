#pragma once 

#include<yade/lib/base/Math.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/dem/Particle.hpp>

struct DemFuncs{
	static void stressStiffness(/*results*/ Matrix3r& stress, Matrix6r& K, /* inputs*/ const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume);
	static Real unbalancedForce(const Scene* scene, const DemField* dem, bool useMaxForce);
};


