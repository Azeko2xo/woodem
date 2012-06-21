#pragma once 

#include<woo/lib/base/Math.hpp>
#include<woo/core/Scene.hpp>
#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Sphere.hpp>

struct DemFuncs{
	DECLARE_LOGGER;
	static std::tuple</*stress*/Matrix3r,/*stiffness*/Matrix6r> stressStiffness(const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume);
	static Real unbalancedForce(const Scene* scene, const DemField* dem, bool useMaxForce);
	static shared_ptr<Particle> makeSphere(Real radius, const shared_ptr<Material>& m);
	static vector<Vector2r> boxPsd(const Scene* scene, const DemField* dem, const AlignedBox3r& box=AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)), bool mass=false, int num=20, int mask=0, Vector2r dRange=Vector2r::Zero());
	
	template<class IteratorRange, class DiameterGetter, class WeightGetter> /* iterate over spheres */
	static vector<Vector2r> psd(const IteratorRange& particleRange,
		bool cumulative, bool normalize, int num, Vector2r dRange,
		DiameterGetter diameterGetter,
		WeightGetter weightGetter
	){
		/* determine dRange if not given */
		if(isnan(dRange[0]) || isnan(dRange[1]) || dRange[0]<0 || dRange[1]<=0 || dRange[0]>=dRange[1]){
			dRange=Vector2r(Inf,-Inf);
			for(const auto& p: particleRange){
				Real d=diameterGetter(p);
				if(d<dRange[0]) dRange[0]=d;
				if(d>dRange[1]) dRange[1]=d;
			}
			if(isinf(dRange[0])) throw std::runtime_error("DemFuncs::psd: no spherical particles?");
		}
		/* put particles in bins */
		vector<Vector2r> ret(num,Vector2r::Zero());
		Real weight=0;
		for(const auto& p: particleRange){
			Real d=diameterGetter(p);
			Real w=weightGetter(p);
			// particles beyong upper bound are discarded, though their weight is taken in account
			weight+=w;
			if(d>dRange[1]) continue;
			int bin=max(0,min(num-1,1+(int)((num-1)*((d-dRange[0])/(dRange[1]-dRange[0])))));
			ret[bin][1]+=w;
		}
		// set radii values
		for(int i=0;i<num;i++) ret[i][0]=dRange[0]+i*(dRange[1]-dRange[0])/(num-1);
		// cummulate and normalize
		if(normalize) for(int i=0;i<num;i++) ret[i][1]=ret[i][1]/weight;
		if(cumulative) for(int i=1;i<num;i++) ret[i][1]+=ret[i-1][1];
		return ret;
	};

};


