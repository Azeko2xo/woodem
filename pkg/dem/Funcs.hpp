#pragma once 

#include<yade/lib/base/Math.hpp>
#include<yade/core/Scene.hpp>
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/Sphere.hpp>

struct DemFuncs{
	DECLARE_LOGGER;
	static std::tuple</*stress*/Matrix3r,/*stiffness*/Matrix6r> stressStiffness(const Scene* scene, const DemField* dem, bool skipMultinodal, Real volume);
	static Real unbalancedForce(const Scene* scene, const DemField* dem, bool useMaxForce);
	static shared_ptr<Particle> makeSphere(Real radius, const shared_ptr<Material>& m);
	static vector<Vector2r> boxPsd(const Scene* scene, const DemField* dem, const AlignedBox3r& box=AlignedBox3r(Vector3r(NaN,NaN,NaN),Vector3r(NaN,NaN,NaN)), int num=20, int mask=0, Vector2r rRange=Vector2r::Zero());
	
	template<class IteratorRange>
	static vector<Vector2r> psd(const IteratorRange& particleRange, int num, Vector2r rRange){
		if(isnan(rRange[0]) || isnan(rRange[1]) || rRange[0]<0 || rRange[1]<=0 || rRange[0]>=rRange[1]){
			rRange=Vector2r(Inf,-Inf);
			for(const shared_ptr<Particle>& p: particleRange){
				if(!p || !p->shape || !dynamic_pointer_cast<yade::Sphere>(p->shape)) continue;
				Real r=p->shape->cast<Sphere>().radius;
				if(r<rRange[0]) rRange[0]=r;
				if(r>rRange[1]) rRange[1]=r;
			}
			if(isinf(rRange[0])) throw std::runtime_error("DemFuncs::boxPsd: no spherical particles?");
		}
		vector<Vector2r> ret(num,Vector2r::Zero());
		size_t nPar=0;
		for(const shared_ptr<Particle>& p: particleRange){
			if(!p || !p->shape || !dynamic_pointer_cast<yade::Sphere>(p->shape)) continue;
			Real r=p->shape->cast<Sphere>().radius;
			nPar++;
			if(r>rRange[1]) continue;
			int bin=max(0,min(num-1,1+(int)((num-1)*((r-rRange[0])/(rRange[1]-rRange[0])))));
			ret[bin][1]+=1;
		}
		for(int i=0;i<num;i++){
			ret[i][0]=2*(rRange[0]+i*(rRange[1]-rRange[0])/(num-1));
			ret[i][1]=ret[i][1]/nPar+(i>0?ret[i-1][1]:0.);
		}
		// for(int i=0; i<num; i++) ret[i][1]/=ret[num-1][1];
		return ret;
	};

};


