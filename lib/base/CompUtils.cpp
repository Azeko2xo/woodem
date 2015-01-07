#include<woo/lib/base/CompUtils.hpp>
#include<stdexcept>
#include<string>
#include<boost/lexical_cast.hpp>

using std::min; using std::max;

int CompUtils::defaultCmap=19; // corresponds to 'jet' in Colormap-data.ipp

const vector<CompUtils::Colormap> CompUtils::colormaps={
	#include"Colormap-data.matplotlib-part.ipp"
	,
	#include"Colormap-data.ncView.ipp"
	// #include"Colormap-data.matplotlib.ipp"
};

Vector3r CompUtils::mapColor(Real normalizedColor, int cmap, bool reversed){
	if(cmap<0 || cmap>(int)colormaps.size()) cmap=defaultCmap;
	assert(cmap>=0 && cmap<(int)colormaps.size());
	normalizedColor=max(0.,min(normalizedColor,1.));
	if(reversed) normalizedColor=1-normalizedColor;
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

Vector3r CompUtils::scalarOnColorScale(Real x, Real xmin, Real xmax, int cmap, bool reversed){
	Real xnorm=min((Real)1.,max((x-xmin)/(xmax-xmin),(Real)0.));
	return mapColor(xnorm,cmap,reversed);
}

Vector2r CompUtils::closestParams_LineLine(const Vector3r& P, const Vector3r& u, const Vector3r& Q, const Vector3r& v, bool& parallel){
	// http://geomalgorithms.com/a07-_distance.html
	Vector3r w0(P-Q);
	Real a=u.squaredNorm(), b=u.dot(v), c=v.squaredNorm(), d=u.dot(w0), e=v.dot(w0);
	Real denom=a*c-b*b;
	if(unlikely(denom==0.)){
		parallel=true;
		return Vector2r(0,b!=0.?(d/b):(e/c));
	} else {
		parallel=false;
		return Vector2r((b*e-c*d)/denom,(a*e-b*d)/denom);
	}
}

Real CompUtils::distSq_LineLine(const Vector3r& P, const Vector3r& u, const Vector3r& Q, const Vector3r& v, bool& parallel, Vector2r& st){
	st=closestParams_LineLine(P,u,Q,v,parallel);
	return ((P+u*st[0])-(Q+v*st[1])).squaredNorm();
}
Vector3r CompUtils::closestSegmentPt(const Vector3r& P, const Vector3r& A, const Vector3r& B, Real* normPos){
	Vector3r BA=B-A;
	if(unlikely(BA.squaredNorm()==0.)){ if(normPos) *normPos=0.; return A; }
	Real u=(P.dot(BA)-A.dot(BA))/(BA.squaredNorm());
	if(normPos) *normPos=u;
	return A+min(1.,max(0.,u))*BA;
}

Vector3r CompUtils::inscribedCircleCenter(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2){
	return v0+((v2-v0)*(v1-v0).norm()+(v1-v0)*(v2-v0).norm())/((v1-v0).norm()+(v2-v1).norm()+(v0-v2).norm());
}

Vector3r CompUtils::circumscribedCircleCenter(const Vector3r& A, const Vector3r& B, const Vector3r& C){
	// http://en.wikipedia.org/wiki/Circumscribed_circle#Circumcircle_equations
	Vector3r a(A-C), b(B-C);
	return C+((a.squaredNorm()*b-b.squaredNorm()*a).cross(a.cross(b)))/(2*(a.cross(b)).squaredNorm());
}

Real CompUtils::segmentPlaneIntersection(const Vector3r& A, const Vector3r& B, const Vector3r& pt, const Vector3r& normal){
	// http://paulbourke.net/geometry/planeline/
	return normal.dot(pt-A)/normal.dot(pt-B);
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

Vector3r CompUtils::triangleBarycentrics(const Vector3r& x, const Vector3r& A, const Vector3r& B, const Vector3r& C){
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

// convert cartesian coordinates to cylindrical
Vector3r CompUtils::cart2cyl(const Vector3r& cart){
	return Vector3r(cart.head<2>().norm(),atan2(cart[1],cart[0]),cart[2]);
}

// convert cylindrical coordinates to cartesian
Vector3r CompUtils::cyl2cart(const Vector3r& cyl){
	return Vector3r(cyl[0]*cos(cyl[1]),cyl[0]*sin(cyl[1]),cyl[2]);
}

bool CompUtils::angleInside(const Real& phi, Real a, const Real& b){
	if(std::abs(a-b)>=2*M_PI) return true; // interval covers everything
	if(a>b) a-=2*M_PI;
	if(a==b) return (fmod(a,2*M_PI)==fmod(phi,2*M_PI)); // corner case
	assert(b-a>0 && b-a<2*M_PI); // unless I overlooked something?
	// wrap phi so that a+pphi is in a..a+2*M_PI, i.e. pphi in 0..2*M_PI
	Real pphi=wrapNum(phi-a,2*M_PI);
	return pphi<(b-a);
}


// return sample for cylindrical coordinate "box" uniformly-distributed in cartesian space; returned point is in cylindrical coordinates
Vector3r CompUtils::cylCoordBox_sample_cylindrical(const AlignedBox3r& box, const Vector3r& unitRand){
	// three random numbers; can be user-provided (for biased sampling), otherwise computed
	Vector3r rand=isnan(unitRand.maxCoeff())?Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom()):unitRand;
	// explanation at http://www.anderswallin.net/2009/05/uniform-random-points-in-a-circle-using-polar-coordinates/
	const Real& r0(box.min()[0]); const Real& r1(box.max()[0]);
	Real r=sqrt(pow(r0,2)+rand.x()*(pow(r1,2)-pow(r0,2)));
	Real theta=box.min().y()+rand.y()*box.sizes().y();
	Real z=box.min().z()+rand.z()*box.sizes().z();
	return Vector3r(r,theta,z);
}
// test whether the point (given in cylindrical coordinates) is contained in box
bool CompUtils::cylCoordBox_contains_cylindrical(const AlignedBox3r& box, const Vector3r& pt){
	return (pt[0]>=box.min()[0] && pt[0]<=box.max()[0]) && angleInside(pt[1],box.min()[1],box.max()[1]) && (pt[2]>=box.min()[2] && pt[2]<=box.max()[2]);
}

Vector3r CompUtils::cylCoordBox_sample_cartesian(const AlignedBox3r& box, const Vector3r& unitRand){
	return cyl2cart(cylCoordBox_sample_cylindrical(box,unitRand));
}
bool CompUtils::cylCoordBox_contains_cartesian(const AlignedBox3r& box, const Vector3r& pt){
	return cylCoordBox_contains_cylindrical(box,cart2cyl(pt));
}

// initial size is 10, grows if necessary
// doubt that anyone ever uses more than 5 anyway
vector<vector<Vector3r>> unitSphereTri20_vertices(10);
vector<vector<Vector3i>> unitSphereTri20_faces(10);
void unitSphereTri20_compute(int); // defined below

std::tuple<const vector<Vector3r>&,const vector<Vector3i>&> CompUtils::unitSphereTri20(int level){
	if(level<0) throw std::invalid_argument("CompUtils::unitSphereTri20: level must non-negative (not "+to_string(level)+").");
	if(unitSphereTri20_vertices.size()<(size_t)level+1){
		unitSphereTri20_vertices.resize(level+1);	
		unitSphereTri20_faces.resize(level+1);
	}
	for(int l=0; l<=level; l++){
		if(!unitSphereTri20_vertices[level].empty()) continue;
		unitSphereTri20_compute(l);
	}
	return std::tie(unitSphereTri20_vertices[level],unitSphereTri20_faces[level]);
}

void unitSphereTri20_compute(int l){
	assert(l>=0);
	if(l==0){
		// icosahedron, inspired by http://www.neubert.net/Htmapp/SPHEmesh.htm
		Real t=(1+sqrt(5))/2;
		Real tau=t/sqrt(1+t*t);
		Real one=1/(sqrt(1+t*t));
		unitSphereTri20_vertices[0]={
			Vector3r(tau,one,0),Vector3r(-tau,one,0),Vector3r(-tau,-one,0),Vector3r(tau,-one,0),
			Vector3r(one,0,tau),Vector3r(one,0,-tau),Vector3r(-one,0,-tau),Vector3r(-one,0,tau),
			Vector3r(0,tau,one),Vector3r(0,-tau,one),Vector3r(0,-tau,-one),Vector3r(0,tau,-one)
		};
		unitSphereTri20_faces[0]={Vector3i(4,8,7),Vector3i(4,7,9),Vector3i(5,6,11),Vector3i(5,10,6),
			Vector3i(0,4,3),Vector3i(0,3,5),Vector3i(2,7,1),Vector3i(2,1,6),
			Vector3i(8,0,11),Vector3i(8,11,1),Vector3i(9,10,3),Vector3i(9,2,10),
			Vector3i(8,4,0),Vector3i(11,0,5),Vector3i(4,9,3),Vector3i(5,3,10),
			Vector3i(7,8,1),Vector3i(6,1,11),Vector3i(7,2,9),Vector3i(6,10,2)
		};
		return;
	}
	// tesselate the lower level
	const auto& v0(unitSphereTri20_vertices[l-1]);
	const auto& f0(unitSphereTri20_faces[l-1]);
	auto& v1(unitSphereTri20_vertices[l]);
	auto& f1(unitSphereTri20_faces[l]);
	assert(!v0.empty() && !f0.empty());
	assert(v1.empty() && f1.empty());
	//v1.reserve(v0.size()+3*f0.size());
	// copy existing vertices
	v1=v0;
	// map for looking up if the midpoint exists in v1 already, with custom comparator
	// edges to be stored with the first index always lower
	auto vec2i_comp=[](const Vector2i& a, const Vector2i& b){ return a[0]<b[0] || (a[0]==b[0] && a[1]<b[1]); };
	std::map<Vector2i,int,decltype(vec2i_comp)> midPtMap(vec2i_comp);
	f1.reserve(4*f0.size());
	// std::cerr<<"Tesselation level "<<l<<std::endl;
	for(size_t i=0; i<f0.size(); i++){
		/*
		For each face, add 3 vertices (normalized midpoints) and have 4 faces (the original one is repaced by 4 new)

		      A
		      +
		     / \
		  c +---+ b
		   / \ / \
		B +---+---+ C
		      a

		*/
		const Vector3i& ABC(f0[i]); // indices of original points
		Vector3i abc; // indices of midpoints -- set in the following loop
		for(int j:{0,1,2}){
			Vector2i edge(ABC[j],ABC[(j+1)%3]); // indices of edge points
			if(edge[0]>edge[1]) std::swap(edge[0],edge[1]); // same order so that they work as keys properly
			short j2=(j+2)%3; // index of the opposing point in abc
			auto I=midPtMap.find(edge);
			if(I==midPtMap.end()){
				midPtMap[edge]=abc[j2]=v1.size();
				v1.push_back((.5*(v0[edge[0]]+v0[edge[1]])).normalized());
			} else {
				abc[j2]=I->second;
			}
		}
		f1.push_back(Vector3i(ABC[0],abc[2],abc[1]));
		f1.push_back(Vector3i(ABC[1],abc[0],abc[2]));
		f1.push_back(Vector3i(ABC[2],abc[1],abc[0]));
		f1.push_back(Vector3i(abc[0],abc[1],abc[2]));
		// std::cerr<<"   Face #"<<i<<": "<<ABC.transpose()<<" → "<<f1[f1.size()-4].transpose()<<", "<<f1[f1.size()-3].transpose()<<", "<<f1[f1.size()-2].transpose()<<", "<<f1[f1.size()-1].transpose()<<std::endl;
	}
	// std::cerr<<"DONE: "<<v1.size()<<" vertices, "<<f1.size()<<" faces"<<std::endl;
}


/*
copied and adapted from http://www.geometrictools.com/LibMathematics/Distance/Wm5DistSegment3Segment3.cpp
(licensed under the Boost license) and adapted. Much appreciated!

The arguments provided are such that there is minimum compution in the routine itself; direction* arguments must be unit vectors.

The *st* vector returns parameters for the closest points; both s and t run from -HALF_LENGTH to +HALF_LENGTH.
*/
Real CompUtils::distSq_SegmentSegment(const Vector3r& center0, const Vector3r& direction0, const Real& halfLength0, const Vector3r& center1, const Vector3r& direction1, const Real& halfLength1, Vector2r& st, bool& parallel){
	Vector3r diff=center0-center1;
	const Real& extent0(halfLength0);
	const Real& extent1(halfLength1);
	Real a01=-direction0.dot(direction1);
	Real b0=diff.dot(direction0);
	Real b1=-diff.dot(direction1);
	Real c=diff.squaredNorm();
	Real det=abs(1.-a01*a01);
	Real& s0(st[0]);
	Real& s1(st[1]);
	Real sqrDist, extDet0, extDet1, tmpS0, tmpS1;
	#if 0
	    Real a01 = -direction0.Dot(direction1);
	    Real b0 = diff.Dot(direction0);
	    Real b1 = -diff.Dot(direction1);
	    Real c = diff.SquaredLength();
	    Real det = Math<Real>::FAbs((Real)1 - a01*a01);
	    Real s0, s1, sqrDist, extDet0, extDet1, tmpS0, tmpS1;
    if (det >= Math<Real>::ZERO_TOLERANCE)
	#endif
	if(det>=1e-8){
		parallel=false;
		// Segments are not parallel.
		s0 = a01*b1 - b0;
		s1 = a01*b0 - b1;
		extDet0 = extent0*det;
		extDet1 = extent1*det;

		if (s0 >= -extDet0)
		{
			if (s0 <= extDet0)
			{
				if (s1 >= -extDet1)
				{
					if (s1 <= extDet1)  // region 0 (interior)
					{
						// Minimum at interior points of segments.
						Real invDet = ((Real)1)/det;
						s0 *= invDet;
						s1 *= invDet;
						sqrDist = s0*(s0 + a01*s1 + ((Real)2)*b0) +
							s1*(a01*s0 + s1 + ((Real)2)*b1) + c;
					}
					else  // region 3 (side)
					{
						s1 = extent1;
						tmpS0 = -(a01*s1 + b0);
						if (tmpS0 < -extent0)
						{
							s0 = -extent0;
							sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
								s1*(s1 + ((Real)2)*b1) + c;
						}
						else if (tmpS0 <= extent0)
						{
							s0 = tmpS0;
							sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
						}
						else
						{
							s0 = extent0;
							sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
								s1*(s1 + ((Real)2)*b1) + c;
						}
					}
				}
				else  // region 7 (side)
				{
					s1 = -extent1;
					tmpS0 = -(a01*s1 + b0);
					if (tmpS0 < -extent0)
					{
						s0 = -extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
					else if (tmpS0 <= extent0)
					{
						s0 = tmpS0;
						sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
					}
					else
					{
						s0 = extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
				}
			}
			else
			{
				if (s1 >= -extDet1)
				{
					if (s1 <= extDet1)  // region 1 (side)
					{
						s0 = extent0;
						tmpS1 = -(a01*s0 + b1);
						if (tmpS1 < -extent1)
						{
							s1 = -extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
						else if (tmpS1 <= extent1)
						{
							s1 = tmpS1;
							sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
						}
						else
						{
							s1 = extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
					}
					else  // region 2 (corner)
					{
						s1 = extent1;
						tmpS0 = -(a01*s1 + b0);
						if (tmpS0 < -extent0)
						{
							s0 = -extent0;
							sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
								s1*(s1 + ((Real)2)*b1) + c;
						}
						else if (tmpS0 <= extent0)
						{
							s0 = tmpS0;
							sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
						}
						else
						{
							s0 = extent0;
							tmpS1 = -(a01*s0 + b1);
							if (tmpS1 < -extent1)
							{
								s1 = -extent1;
								sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
									s0*(s0 + ((Real)2)*b0) + c;
							}
							else if (tmpS1 <= extent1)
							{
								s1 = tmpS1;
								sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
							}
							else
							{
								s1 = extent1;
								sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
									s0*(s0 + ((Real)2)*b0) + c;
							}
						}
					}
				}
				else  // region 8 (corner)
				{
					s1 = -extent1;
					tmpS0 = -(a01*s1 + b0);
					if (tmpS0 < -extent0)
					{
						s0 = -extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
					else if (tmpS0 <= extent0)
					{
						s0 = tmpS0;
						sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
					}
					else
					{
						s0 = extent0;
						tmpS1 = -(a01*s0 + b1);
						if (tmpS1 > extent1)
						{
							s1 = extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
						else if (tmpS1 >= -extent1)
						{
							s1 = tmpS1;
							sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
						}
						else
						{
							s1 = -extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
					}
				}
			}
		}
		else 
		{
			if (s1 >= -extDet1)
			{
				if (s1 <= extDet1)  // region 5 (side)
				{
					s0 = -extent0;
					tmpS1 = -(a01*s0 + b1);
					if (tmpS1 < -extent1)
					{
						s1 = -extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
					else if (tmpS1 <= extent1)
					{
						s1 = tmpS1;
						sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
					}
					else
					{
						s1 = extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
				}
				else  // region 4 (corner)
				{
					s1 = extent1;
					tmpS0 = -(a01*s1 + b0);
					if (tmpS0 > extent0)
					{
						s0 = extent0;
						sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
							s1*(s1 + ((Real)2)*b1) + c;
					}
					else if (tmpS0 >= -extent0)
					{
						s0 = tmpS0;
						sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
					}
					else
					{
						s0 = -extent0;
						tmpS1 = -(a01*s0 + b1);
						if (tmpS1 < -extent1)
						{
							s1 = -extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
						else if (tmpS1 <= extent1)
						{
							s1 = tmpS1;
							sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
						}
						else
						{
							s1 = extent1;
							sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
								s0*(s0 + ((Real)2)*b0) + c;
						}
					}
				}
			}
			else   // region 6 (corner)
			{
				s1 = -extent1;
				tmpS0 = -(a01*s1 + b0);
				if (tmpS0 > extent0)
				{
					s0 = extent0;
					sqrDist = s0*(s0 - ((Real)2)*tmpS0) +
						s1*(s1 + ((Real)2)*b1) + c;
				}
				else if (tmpS0 >= -extent0)
				{
					s0 = tmpS0;
					sqrDist = -s0*s0 + s1*(s1 + ((Real)2)*b1) + c;
				}
				else
				{
					s0 = -extent0;
					tmpS1 = -(a01*s0 + b1);
					if (tmpS1 < -extent1)
					{
						s1 = -extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
					else if (tmpS1 <= extent1)
					{
						s1 = tmpS1;
						sqrDist = -s1*s1 + s0*(s0 + ((Real)2)*b0) + c;
					}
					else
					{
						s1 = extent1;
						sqrDist = s1*(s1 - ((Real)2)*tmpS1) +
							s0*(s0 + ((Real)2)*b0) + c;
					}
				}
			}
		}
	}
	else
	{
		parallel=true;
		// The segments are parallel.  The average b0 term is designed to
		// ensure symmetry of the function.  That is, dist(seg0,seg1) and
		// dist(seg1,seg0) should produce the same number.
		Real e0pe1 = extent0 + extent1;
		Real sign = (a01 > (Real)0 ? (Real)-1 : (Real)1);
		Real b0Avr = ((Real)0.5)*(b0 - sign*b1);
		Real lambda = -b0Avr;
		if (lambda < -e0pe1)
		{
			lambda = -e0pe1;
		}
		else if (lambda > e0pe1)
		{
			lambda = e0pe1;
		}

		s1 = -sign*lambda*extent1/e0pe1;
		s0 = lambda + sign*s1;
		sqrDist = lambda*(lambda + ((Real)2)*b0Avr) + c;
	}
	
	#if 0
		mClosestPoint0 = center0 + s0*direction0;
		mClosestPoint1 = center1 + s1*direction1;
		mSegment0Parameter = s0;
		mSegment1Parameter = s1;
	#endif

	// Account for numerical round-off errors.
	if (sqrDist < (Real)0)
	{
		sqrDist = (Real)0;
	}
	return sqrDist;
}



