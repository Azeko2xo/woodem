#include<woo/lib/base/CompUtils.hpp>
#include<stdexcept>
#include<string>
#include<boost/lexical_cast.hpp>

using std::min; using std::max;

int CompUtils::defaultCmap=18; // corresponds to 'jet' in Colormap-data.ipp

const vector<CompUtils::Colormap> CompUtils::colormaps={
	#include"Colormap-data.ncView.ipp"
	// ,
	// #include"Colormap-data.matplotlib.ipp"
};

Vector3r CompUtils::mapColor(Real normalizedColor, int cmap){
	if(cmap<0 || cmap>(int)colormaps.size()) cmap=defaultCmap;
	assert(cmap>=0 && cmap<(int)colormaps.size());
	normalizedColor=max(0.,min(normalizedColor,1.));
	// throw std::invalid_argument(("Colormap number "+boost::lexical_cast<std::string>(cmap)+" is not defined.").c_str());
	Vector3r* v=(Vector3r*)(&colormaps[cmap].rgb[3*((int)(normalizedColor*255))]);
	return Vector3r(*v);
};

Vector3r CompUtils::mapColor_map0(Real xnorm){
	if(xnorm<.25) return Vector3r(0,4.*xnorm,1);
	if(xnorm<.5)  return Vector3r(0,1,1.-4.*(xnorm-.25));
	if(xnorm<.75) return Vector3r(4.*(xnorm-.5),1.,0);
	return Vector3r(1,1-4.*(xnorm-.75),0);
}

Vector3r CompUtils::scalarOnColorScale(Real x, Real xmin, Real xmax, int cmap){
	Real xnorm=min((Real)1.,max((x-xmin)/(xmax-xmin),(Real)0.));
	return mapColor(xnorm,cmap);
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

int CompUtils::lineSphereIntersection(const Vector3r& A, const Vector3r& u, const Vector3r& C, Real r, Real& t0, Real& t1, Real relTol){
	// http://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
	// ray goes through origin, therefore we move sphere relative to A
	Vector3r Cr=C-A;
	Real disc=(pow(u.dot(Cr),2)-Cr.dot(Cr)+pow(r,2));
	if(disc<0) return 0; // no intersection
	if(4*disc<pow(relTol*r,2)){ t0=u.dot(Cr); return 1; }
	disc=sqrt(disc);
	t0=u.dot(Cr)-disc; t1=u.dot(Cr)+disc;
	return 2;
}

Vector3r CompUtils::facetBarycentrics(const Vector3r& x, const Vector3r& A, const Vector3r& B, const Vector3r& C){
	/* http://www.blackpawn.com/texts/pointinpoly/default.html, but we used different notation
		vert=[A,B,C]
		reference point (origin) is A
		u,v, are with respect to B-A, C-A; the third coord is computed
		x-A=u*(B-A)+v*(C-A), i.e. vX=u*vB+v*vC;
		dot-multiply by vB and vC to get 2 equations for u,v

		return (1-u-v,u,v)
	*/
		 
	typedef Eigen::Matrix<Real,2,2> Matrix2r;
	Vector3r vB=B-A, vC=C-A, vX=x-A;
	Matrix2r M;
	M<<vB.dot(vB),vC.dot(vB),
	   vB.dot(vC),vC.dot(vC);
	Vector2r uv=M.inverse()*Vector2r(vX.dot(vB),vX.dot(vC));
	return Vector3r(1-uv[0]-uv[1],uv[0],uv[1]);
}
