#include<woo/pkg/dem/Ellipsoid.hpp>
#include<woo/pkg/dem/Sphere.hpp>

#include <boost/math/tools/minima.hpp>
#include <boost/math/tools/tuple.hpp>

WOO_PLUGIN(dem,(Ellipsoid)(Bo1_Ellipsoid_Aabb)(Cg2_Wall_Ellipsoid_L6Geom)(Cg2_Ellipsoid_Ellipsoid_L6Geom));
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Ellipsoid));
#endif

void woo::Ellipsoid::selfTest(const shared_ptr<Particle>& p){
	if(!(semiAxes.minCoeff()>0)) throw std::runtime_error("Ellipsoid #"+to_string(p->id)+": all semi-princial semiAxes must be positive (current minimum is "+to_string(semiAxes.minCoeff())+")");
	if(!numNodesOk()) throw std::runtime_error("Ellipsoid #"+to_string(p->id)+": numNodesOk() failed: must be 1, not "+to_string(nodes.size())+".");
}

void woo::Ellipsoid::updateDyn(const Real& density) const {
	assert(numNodesOk());
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

bool Cg2_Wall_Ellipsoid_L6Geom::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const Wall& wall(s1->cast<Wall>());
	const Ellipsoid& ell(s2->cast<Ellipsoid>());
	const auto& ax(wall.axis); const auto& sense(wall.sense);
	const Vector3r ellPos(ell.nodes[0]->pos+shift2); const Vector3r& wallPos(wall.nodes[0]->pos);
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
	const Vector3r& ra(s1->nodes[0]->pos); const Vector3r& rb(s2->nodes[0]->pos);
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



