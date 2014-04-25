#include<woo/pkg/dem/Ellipsoid.hpp>
#include<woo/pkg/dem/Sphere.hpp>

#include <boost/math/tools/minima.hpp>
#include <boost/math/tools/tuple.hpp>

WOO_PLUGIN(dem,(Ellipsoid)(Bo1_Ellipsoid_Aabb)(Cg2_Wall_Ellipsoid_L6Geom)(Cg2_Facet_Ellipsoid_L6Geom)(Cg2_Ellipsoid_Ellipsoid_L6Geom));

WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Ellipsoid_Ellipsoid_L6Geom__CLASS_BASE_DOC_ATTRS);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Wall_Ellipsoid_L6Geom__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_Facet_Ellipsoid_L6Geom__CLASS_BASE_DOC);

#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Ellipsoid));
#endif

void woo::Ellipsoid::selfTest(const shared_ptr<Particle>& p){
	if(!(semiAxes.minCoeff()>0)) throw std::runtime_error("Ellipsoid #"+to_string(p->id)+": all semi-princial semiAxes must be positive (current minimum is "+to_string(semiAxes.minCoeff())+")");
	if(!numNodesOk()) throw std::runtime_error("Ellipsoid #"+to_string(p->id)+": numNodesOk() failed: must be 1, not "+to_string(nodes.size())+".");
}

Real woo::Ellipsoid::equivRadius() const {
	// volume-based equivalent radius
	return cbrt((4/3.)*M_PI*semiAxes.prod());
};

Real woo::Ellipsoid::volume() const { return (4/3.)*M_PI*semiAxes.prod(); }
void woo::Ellipsoid::applyScale(Real scale) { semiAxes*=scale; }


bool woo::Ellipsoid::isInside(const Vector3r& pt) const {
	Vector3r l=nodes[0]->glob2loc(pt);
	return pow(l[0]/semiAxes[0],2)+pow(l[1]/semiAxes[1],2)+pow(l[2]/semiAxes[2],2)<=1;
}


void woo::Ellipsoid::updateMassInertia(const Real& density) const {
	checkNodesHaveDemData();
	auto& dyn=nodes[0]->getData<DemData>();
	dyn.mass=(4/3.)*M_PI*semiAxes.prod()*density;
	dyn.inertia=(1/5.)*dyn.mass*Vector3r(pow(semiAxes[1],2)+pow(semiAxes[2],2),pow(semiAxes[2],2)+pow(semiAxes[0],2),pow(semiAxes[0],2)+pow(semiAxes[1],2));
};

/* return matrix transforming unit sphere to this ellipsoid */
Matrix3r woo::Ellipsoid::trsfFromUnitSphere() const{
	Matrix3r M;
	for(int i:{0,1,2}) M.col(i)=nodes[0]->ori*(semiAxes[i]*Vector3r::Unit(i));
	return M;
}

/* return extents (half-size) spanned in the direction of *axis*; this is the same algo
	as when computing Aabb, but evaluated for one axis only.
 */
Real woo::Ellipsoid::axisExtent(short axis) const {
	Matrix3r M=trsfFromUnitSphere();
	return M.row(axis).norm();
}

AlignedBox3r Ellipsoid::alignedBox() const {
	Matrix3r M=trsfFromUnitSphere();
	// http://www.loria.fr/~shornus/ellipsoid-bbox.html
	const Vector3r& pos(nodes[0]->pos);
	Vector3r delta(M.row(0).norm(),M.row(1).norm(),M.row(2).norm());
	return AlignedBox3r(pos-delta,pos+delta);
};


void Bo1_Ellipsoid_Aabb::go(const shared_ptr<Shape>& sh){
	//goGeneric(sh,sh->cast<Ellipsoid>().semiAxes.maxCoeff()*Vector3r::Ones());
	if(!sh->bound){ sh->bound=make_shared<Aabb>(); }
	Aabb& aabb=sh->bound->cast<Aabb>();
	Matrix3r M=sh->cast<Ellipsoid>().trsfFromUnitSphere();
	// http://www.loria.fr/~shornus/ellipsoid-bbox.html
	const Vector3r& pos(sh->nodes[0]->pos);
	Vector3r delta(M.row(0).norm(),M.row(1).norm(),M.row(2).norm());
	aabb.min=pos-delta;
	aabb.max=pos+delta;
}

// the same funcs but with extra rotation
// (for computing in rotated coordinates)
Matrix3r woo::Ellipsoid::trsfFromUnitSphere(const Quaternionr& ori) const{
	Matrix3r M;
	for(int i:{0,1,2}) M.col(i)=ori*nodes[0]->ori*(semiAxes[i]*Vector3r::Unit(i));
	return M;
}
Real woo::Ellipsoid::rotatedExtent(short axis, const Quaternionr& ori) const {
	Matrix3r M=trsfFromUnitSphere(ori);
	return M.row(axis).norm();
}

bool Cg2_Facet_Ellipsoid_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Facet& facet(s1->cast<Facet>());
	const Ellipsoid& ell(s2->cast<Ellipsoid>()); const Quaternionr& ellOri(ell.nodes[0]->ori);
	const Vector3r& facetPosA(facet.nodes[0]->pos); const Vector3r ellPos(ell.nodes[0]->pos+shift2);
	// compute local orientation where facet normal is the z-axis
	Vector3r facetNormal=facet.getNormal();
	Quaternionr locOri; locOri.setFromTwoVectors(facetNormal,Vector3r::UnitZ()); // use this instead of the static Quaternionr::FromTwoVector which did not exist in earlier Eigen version (!)
	Real locZExtent=ell.rotatedExtent(2,locOri); // always positive
	// put ellipsoid in origin, and facet somewhere, perpendicular to z
	// the z-coordinate should be the same for all facet vertices
	Vector3r facetLocPosA=locOri*(facetPosA-ellPos);

	Real uN=abs(facetLocPosA[2])-facet.halfThick-locZExtent;
	if(uN>0 && !C->isReal() && !force){ return false; }

	#if 1
		static bool notImpl=false; // give the warning only once
		if(notImpl){
			cerr<<"[Cg2_Facet_Ellipsoid_L6Geom: not yet implemented]";
			notImpl=true;
		}
		return false;
	// algo A: transform ellipsoid to unit sphere in origin, compute closest point with thus transformed triangle
	// the contact point is transformed back to global coords
	// if the CP is inside the triangle, the normal is coincident with facet's normal
	// otherwise compute the ellipsoid normal at the contact point (potential steepest descent) and use that
	// Move the point by halfThick away from the facet
	Matrix3r fromUnitSphere=ell.trsfFromUnitSphere();
	Matrix3r toUnitSphere=fromUnitSphere.inverse();
	// all points end with 't', signifying the transformed space
	Vector3r vt[3]={toUnitSphere*(facet.nodes[0]->pos-ellPos),toUnitSphere*(facet.nodes[1]->pos-ellPos),toUnitSphere*(facet.nodes[2]->pos-ellPos)};
	Vector3r nt=((vt[1]-vt[0]).cross(vt[2]-vt[0])).normalized(); // facet normal
	Real distt=vt[0].dot(nt); // distance origin-plane
	Vector3r pot=nt*distt; // origin projected onto the plane
	Vector3r ont[3]={(vt[1]-vt[0]).cross(nt),(vt[2]-vt[1]).cross(nt),(vt[0]-vt[2]).cross(nt)}; // outer normals

	cerr<<"-----------------"<<endl;
	cerr<<"vt: "<<vt[0].transpose()<<" | "<<vt[1].transpose()<<" | "<<vt[2].transpose()<<endl;
	cerr<<"distt: "<<distt<<endl;
	cerr<<"nt: "<<nt.transpose()<<endl;
	cerr<<"pot: "<<pot.transpose()<<endl;
	cerr<<"ont :"<<ont[0].transpose()<<" | "<<ont[1].transpose()<<" | "<<ont[2].transpose()<<endl;
	cerr<<"vg: "<<(fromUnitSphere*vt[0]).transpose()<<" | "<<(fromUnitSphere*vt[1]).transpose()<<" | "<<(fromUnitSphere*vt[2]).transpose()<<endl;
	cerr<<"ng: "<<(fromUnitSphere*nt).transpose()<<endl;
	cerr<<"pog: "<<(fromUnitSphere*pot).transpose()<<endl;

	// this algo is similar to what is in Facet::getNearestPt, but we need to know whether the point is inside or at the edge in addition
	short w=0;
	for(int i:{0,1,2}) w&=(ont[i].dot(pot-vt[i])>0?1:0)<<i;
	Vector3r cpt; // contact point in transformed space
	switch(w){
		case 0: cpt=pot; break; // ---: inside triangle
		case 1: cpt=CompUtils::closestSegmentPt(pot,vt[0],vt[1]); break; // +-- (n1)
		case 2: cpt=CompUtils::closestSegmentPt(pot,vt[1],vt[2]); break; // -+- (n2)
		case 4: cpt=CompUtils::closestSegmentPt(pot,vt[2],vt[0]); break; // --+ (n3)
		case 3: cpt=vt[1]; break; // ++- (v1)
		case 5: cpt=vt[0]; break; // +-+ (v0)
		case 6: cpt=vt[2]; break; // -++ (v2)
		case 7: throw logic_error("Cg2_Facet_Ellipsoid_L6Geom: Impossible sphere-facet intersection (all points are outside the edges). (please report bug)"); // +++ (impossible)
		default: throw logic_error("Cg2_Facet_Ellipsoid_L6Geom: Nonsense intersection value. (please report bug)");
	}
	// transform contact point back (origin in ellipsoid's center, global orientation)
	Vector3r cpe=fromUnitSphere*cpt;
	// cerr<<"{"<<cpt.norm()<<","<<w<<"}"; // should be 1 or less (with no halfThick)
	// contact normal, in global space
	Vector3r normal;
	if(w==0){
		// face contact, easy
		normal=facetNormal;
	} else{
		cerr<<".";
		// ellipsoid gradient is 2(x/a²,y/b²,z/c²) in local coords, use that to compute the normal
		Vector3r cpl=ellOri.conjugate()*cpe; //contact point in ellipsoid-local coords
		Vector3r gradl(cpl[0]/ell.semiAxes[0],cpl[1]/ell.semiAxes[1],cpl[2]/ell.semiAxes[2]);
		normal=ellOri*gradl.normalized(); // transform back to global coords
	}
	Real contR=cpe.norm(); // distance to the contact point
	Real facetContR=(facet.halfThick>0?-contR:facet.halfThick); // take halfThick in account here
	Vector3r contPt=cpe+ellPos; // in global coords
	if(facet.halfThick>0) contPt+=normal*facet.halfThick;
	Vector3r facetLinVel,facetAngVel;
	std::tie(facetLinVel,facetAngVel)=facet.interpolatePtLinAngVel(contPt);

	// FIXME: the uN is totally bogus here, it is only valid when for face contact!!!!

	const DemData& ellDyn(ell.nodes[0]->getData<DemData>());
	// const DemData& facetDyn(facet.nodes[0]->getData<DemData>());
	handleSpheresLikeContact(C,contPt,facetLinVel,facetAngVel,ellPos,ellDyn.vel,ellDyn.angVel,normal,contPt,uN,facetContR,contR);
	return true;
	#endif

}


bool Cg2_Wall_Ellipsoid_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Wall& wall(s1->cast<Wall>());
	const Ellipsoid& ell(s2->cast<Ellipsoid>());
	const auto& ax(wall.axis); const auto& sense(wall.sense);
	const Vector3r& wallPos(wall.nodes[0]->pos); const Vector3r ellPos(ell.nodes[0]->pos+shift2);
	Real extent=ell.axisExtent(ax);
	if(((wallPos[ax]<(ellPos[ax]-extent)) || (wallPos[ax]>(ellPos[ax]+extent))) && !C->isReal() && !force){ return false; }
	// penetration distance and normal can be computed simply, being axis-aligned
	Real dist=ellPos[ax]-wallPos[ax]; // signed distance: positive for ellipsoid above the wall
	assert(sense==-1 || sense==0 || sense==1);
	short normAxSgn; // sign of the normal along wall's axis
	if(wall.sense==0) normAxSgn=dist>0?1:-1; // both-side wall
	else normAxSgn=(sense==1?1:-1);
	Vector3r normal=normAxSgn*Vector3r::Unit(ax);
	Real uN=normAxSgn*dist-extent;
	// for small deformations, we can suppose the contact point is the one touching the bounding box
	// http://www.loria.fr/~shornus/ellipsoid-bbox.html -- using their notation
	Matrix3r M(ell.trsfFromUnitSphere());
	Matrix3r Mprime=M.transpose();
	for(short i:{0,1,2}) Mprime.col(i).normalize();
	// Columns of this product should be deltas from ellipsoid center to the extremal point, projected to the plane.
	// normAxSign is ±1 depending on sense and position, re-use that one here, but opposite:
	// it wall it touched from above, we need the lower point on the ellipsoid and vice versa
	Vector3r contPt=ellPos+(-normAxSgn)*(M*Mprime).col(ax);
	contPt[ax]=wallPos[ax];

	Real contR=(contPt-ellPos).norm();
	const DemData& ellDyn(ell.nodes[0]->getData<DemData>()); const DemData& wallDyn(wall.nodes[0]->getData<DemData>());
	handleSpheresLikeContact(C,wallPos,wallDyn.vel,wallDyn.angVel,ellPos,ellDyn.vel,ellDyn.angVel,normal,contPt,uN,/*negative for wall*/-contR,contR);
	return true;
}



void Cg2_Ellipsoid_Ellipsoid_L6Geom::setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C){
	C->minDist00Sq=pow(s1->cast<Ellipsoid>().semiAxes.maxCoeff()+s2->cast<Ellipsoid>().semiAxes.maxCoeff(),2);
}

bool Cg2_Ellipsoid_Ellipsoid_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	/* notation taken from Perram, Rasmussen, Præstgaard, Lebowtz: Ellipsoid contact potential */
	const Vector3r& ra(s1->nodes[0]->pos); Vector3r rb(s2->nodes[0]->pos+shift2);
	const Vector3r& a(s1->cast<Ellipsoid>().semiAxes); const Vector3r& b(s2->cast<Ellipsoid>().semiAxes);
	const Quaternionr& oa(s1->nodes[0]->ori); const Quaternionr& ob(s2->nodes[0]->ori);
	Vector3r u[]={oa*Vector3r::UnitX(),oa*Vector3r::UnitY(),oa*Vector3r::UnitZ()};
	Vector3r v[]={ob*Vector3r::UnitX(),ob*Vector3r::UnitY(),ob*Vector3r::UnitZ()};
	const DemData& dyn1(s1->nodes[0]->getData<DemData>());const DemData& dyn2(s2->nodes[0]->getData<DemData>());

	Vector3r R(rb-ra); // (2.1)
	Matrix3r A(Matrix3r::Zero()), B(Matrix3r::Zero());
	for(int k:{0,1,2}){
		A+=u[k]*u[k].transpose()/pow(a[k],2); // (2.2a)
		B+=v[k]*v[k].transpose()/pow(b[k],2); // (2.2b)
	}
	Matrix3r Ainv=A.inverse(), Binv=B.inverse(); // (2.3a), (2.3b)

	// (2.4), for Brent's maximization: return only the function value
	// the result is negated so that we can use boost::math::brent_find_minima which finds the minimum
	auto neg_S_lambda_0=[&](const Real& l) -> Real { return -l*(1-l)*R.transpose()*((1-l)*Ainv+l*Binv).inverse()*R; };

	// cerr<<"S(l):"; for(Real l=0; l<1; l+=.05){ cerr<<" "<<-neg_S_lambda_0(l); }; cerr<<endl;

	// TODO: define funcs returning 1st derivative (zero at the extreme) and 2nd derivative (used by Newton-Raphson)
	#if 0
		// (2.4) + (2.8) for Newton-Raphson, return both value and derivative
		auto S_lambda_01=[&](const Real& l) -> boost::math::tuple {
			Matrix3r G=(1-l)*Ainv+l*Binv; // (2.6)
			Matrix3r GinvR=G.inverse()*R; // (2.9): GinvR≡X (GX=R)
			return boost::math::make_tuple(
				l*(a-l)*R.transpose()*GinvR,
				GinvR.transpose()*(pow(1-l,2)*Ainv-pow(l,2)*Binv)*GinvR
			);
		}
	#endif
	// set once the iteration has result
	Real L,Fab;
	if(brent){
		// boost docs claims that accuracy higher than half bits is ignored, so set just that -- half of 8*sizeof(Real)
		auto lambda_negSmin=boost::math::tools::brent_find_minima(neg_S_lambda_0,0.,1.,brentBits);
		L=boost::math::get<0>(lambda_negSmin);
		Fab=-boost::math::get<1>(lambda_negSmin); // invert the sign
	} else {
		#if 0
			// use Newton-Raphson
			// FIXME: we should use the previous value of lambda as the initial guess!
			Real l0=a.maxCoeff()/(a.maxCoeff()+b/maxCoeff()); // initial guess as for spheres; Donev, pg 773
			// this is what boost docs do in the example on newton_raphson_iterate
			int digits=(2*std::numeric_limits<T>::digits)/3;
			boost::math::newton_raphson_iterate(S_lambda_01,...)
		#else
			throw std::runtime_error("Cg2_Ellipsoid_Ellipsoid_L6Geom::go: Newton-Raphson iteration is not yet implemented; use Brent's method by saying brent=True.");
		#endif
	}
	// cerr<<"l="<<L<<", Fab="<<Fab<<endl;
	// Perram, Rasmussen, pg 6567
	if(Fab>1 && !C->isReal() && !force){ return false;	}

	Matrix3r G=(1-L)*Ainv+L*Binv; // (2.6)
	Vector3r nUnnorm=G.inverse()*R; // Donev, (19)
	Vector3r contPt=ra+(1-L)*Ainv*nUnnorm; // Donev, (19)
	// compute uN: Perram&Wertheim (3.17): Fab=μ², where μ is the scaling factor
	Real mu=sqrt(Fab);
	// μ scales sizes while keeping ellipsoid distance
	// 1/μ therefore scales distance keeping ellipsoid sizes
	// with d (≡|R|) being the current distance, we have
	// uN'=d-d₀=d-(1/μ)*d=d(1-1/μ) 
	// however, the scaling approaches ellipsoids at that rate in the direction of R
	// so if the normal has different orientation, we need to scale that by the cosine,
	// which happens to be the scalar product of normalized rUnit and nUnit (normalized R and nUnnorm)
	// so finally we have
	// uN=uN'*cos(rUnit,nUnit)=|R|(1-1/μ)*(rUnit·nUnit)
	Real Rnorm=R.norm();
	Vector3r nUnit=nUnnorm.normalized(), rUnit=R/Rnorm;
	Real uN=Rnorm*(1-1/mu)*rUnit.dot(nUnit);
	// cerr<<"Fab="<<Fab<<", mu="<<mu<<", cos="<<rUnit.dot(nUnit)<<", l="<<l<<", l0="<<l0<<", |nUnnorm|="<<nUnnorm.norm()<<", uN="<<uN<<endl;

	// cerr<<"lambda: "<<L<<", Fab: "<<Fab<<", n: "<<nUnnorm.transpose()<<", CP: "<<contPt.transpose()<<endl;
	handleSpheresLikeContact(C,ra,dyn1.vel,dyn1.angVel,rb,dyn2.vel,dyn2.angVel,nUnit,contPt,uN,(contPt-ra).norm(),(contPt-rb).norm());

	return true;
}



