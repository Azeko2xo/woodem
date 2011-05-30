#include<yade/lib/base/CompUtils.hpp>
#include<stdexcept>
#include<string>
#include<boost/lexical_cast.hpp>

using std::min; using std::max;

int CompUtils::defaultCmap=0; 

Vector3r CompUtils::mapColor(Real normalizedColor, int cmap){
	if(cmap<0) cmap=defaultCmap;
	switch(cmap){
		case 0:
			return mapColor_map0(normalizedColor);
		// case 1:
	};
	throw std::invalid_argument(("Colormap number "+boost::lexical_cast<std::string>(cmap)+" is not defined.").c_str());
};

Vector3r CompUtils::mapColor_map0(Real xnorm){
	if(xnorm<.25) return Vector3r(0,4.*xnorm,1);
	if(xnorm<.5)  return Vector3r(0,1,1.-4.*(xnorm-.25));
	if(xnorm<.75) return Vector3r(4.*(xnorm-.5),1.,0);
	return Vector3r(1,1-4.*(xnorm-.75),0);
}

Vector3r CompUtils::scalarOnColorScale(Real x, Real xmin, Real xmax){
	Real xnorm=min((Real)1.,max((x-xmin)/(xmax-xmin),(Real)0.));
	return mapColor(xnorm);
}

Vector3r CompUtils::closestSegmentPt(const Vector3r& P, const Vector3r& A, const Vector3r& B, Real* normPos){
	Vector3r BA=B-A;
	Real u=(P.dot(BA)-A.dot(BA))/(BA.squaredNorm());
	if(normPos) *normPos=u;
	return A+min(1.,max(0.,u))*BA;
}

Vector3r CompUtils::inscribedCircleCenter(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2){
	return v0+((v2-v0)*(v1-v0).norm()+(v1-v0)*(v2-v0).norm())/((v1-v0).norm()+(v2-v1).norm()+(v0-v2).norm());
}


