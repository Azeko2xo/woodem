#include<woo/pkg/dem/Potential.hpp>
#include<woo/lib/pcl/bfgs.h>
#include<unsupported/Eigen/NumericalDiff>

WOO_PLUGIN(dem,(PotentialFunctor)(PotentialDispatcher)(Pot1_Sphere)(Pot1_Wall)(Cg2_Shape_Shape_L6Geom__Potential));

WOO_IMPL__CLASS_BASE_DOC_PY(woo_dem_PotentialFunctor__CLASS_BASE_DOC_PY);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Pot1_Sphere__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Pot1_Wall__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Shape_Shape_L6Geom__Potential__CLASS_BASE_DOC_ATTRS);


PotentialFunctor::PotFuncType PotentialFunctor::funcPotentialValue(const shared_ptr<Shape>& s){
	throw std::runtime_error(pyStr()+".funcPotentialValue: not overridden (called with shape "+s->pyStr()+").");
}
PotentialFunctor::GradFuncType PotentialFunctor::funcPotentialGradient(const shared_ptr<Shape>&){
	// return empty function by default
	return GradFuncType();
}
Real PotentialFunctor::go(const shared_ptr<Shape>& s, const Vector3r& pt){
	return funcPotentialValue(s)(pt);
}


Real Pot1_Sphere::boundingSphereRadius(const shared_ptr<Shape>& s){ return s->cast<Sphere>().radius; }

Real Pot1_Sphere::pot(const shared_ptr<Shape>& s, const Vector3r& pt){
	Real ret=(pt-s->nodes[0]->pos).norm()-s->cast<Sphere>().radius;
	cerr<<"Pot1_Sphere::pot("<<pt.transpose()<<")="<<ret<<endl;
	return ret;
}
PotentialFunctor::PotFuncType Pot1_Sphere::funcPotentialValue(const shared_ptr<Shape>& s){
	// return PotFuncType([&](const Vector3r& pt)->Real{ return (pt-s->nodes[0]->pos).norm()-s->cast<Sphere>().radius; });
	return PotFuncType([&](const Vector3r& pt)->Real{ return this->pot(s,pt); });
}

Real Pot1_Wall::pot(const shared_ptr<Shape>& s, const Vector3r& pt){
	const auto& w(s->cast<Wall>());
	Real ret=w.sense*(pt[w.axis]-w.nodes[0]->pos[w.axis]);
	cerr<<"Pot1_Wall::pot  ("<<pt.transpose()<<")="<<ret<<endl;
	return ret;
}
	// return PotFuncType([&](const Vector3r& pt)->Real{ return w.sense*(pt[w.axis]-w.nodes[0]->pos[w.axis]); });
PotentialFunctor::PotFuncType Pot1_Wall::funcPotentialValue(const shared_ptr<Shape>& s){
	const auto& w(s->cast<Wall>());
	if(w.sense!=-1 && w.sense!=1) throw std::runtime_error("Pot1_Wall: Wall.sense must be +1 or -1, not "+to_string(w.sense)+".");
	// return PotFuncType([&](const Vector3r& pt)->Real{ return w.sense*(pt[w.axis]-w.nodes[0]->pos[w.axis]); });
	return PotFuncType([&](const Vector3r& pt)->Real{ return this->pot(s,pt); });
}

void Cg2_Shape_Shape_L6Geom__Potential::setMinDist00Sq(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const shared_ptr<Contact>& C) {
	Real sum=0;
	for(const auto& s: {s1,s2}){
		const auto& f=potentialDispatcher->getFunctor1D(s);
		if(!f){ C->minDist00Sq=-1; return; }
		Real r=f->boundingSphereRadius(s);
		if(!(r>0)){ C->minDist00Sq=-1; return; }
		sum+=r;
	}
	C->minDist00Sq=pow(sum,2);
}

void Cg2_Shape_Shape_L6Geom__Potential::pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){
	if(py::len(t)==0) return; // nothing to do
	if(py::len(t)!=1) throw invalid_argument(("Collider optionally takes exactly one list of BoundFunctor's as non-keyword argument for constructor ("+lexical_cast<string>(py::len(t))+" non-keyword ards given instead)").c_str());
	if(!potentialDispatcher) potentialDispatcher=make_shared<PotentialDispatcher>();
	vector<shared_ptr<PotentialFunctor>> vf=py::extract<vector<shared_ptr<PotentialFunctor>>>((t[0]))();
	for(const auto& f: vf) potentialDispatcher->add(f);
	t=py::tuple(); // empty the args
}


template<typename _Scalar, int NX>
struct ParticleBFGSFunctor
{
	typedef _Scalar Scalar;
	typedef Eigen::Matrix<Scalar,1,1> Matrix11;
	enum { InputsAtCompileTime = NX, ValuesAtCompileTime=1 };
	typedef Eigen::Matrix<Scalar,InputsAtCompileTime,1> VectorType;
	typedef PotentialFunctor::PotFuncType PotFuncType;
	typedef PotentialFunctor::GradFuncType GradFuncType;
	// for NumericalDiff
	typedef VectorType InputType; 
	typedef Eigen::Matrix<Scalar,1,InputsAtCompileTime> JacobianType;
	typedef Matrix11 ValueType;


	std::function<Real(const Vector3r&)> potFunc;
	std::function<Vector3r(const Vector3r&)> gradFunc; // unset by default
	const int m_inputs;

	ParticleBFGSFunctor(PotFuncType _potFunc, GradFuncType _gradFunc=GradFuncType(), int _inputs=0): potFunc(_potFunc), gradFunc(_gradFunc), m_inputs(_inputs!=0?_inputs:InputsAtCompileTime) { }

	int inputs() const { return m_inputs; }
	int values() const { return 1; }

	int operator() (const InputType& x, ValueType &v) const { v(0,0)=potFunc(x); return 1; }
	int df(const VectorType &x, JacobianType &df) const { if(!gradFunc) throw std::logic_error("ParticleBFGSFunctor.df: gradFunc is undefined and numerical differentiation is not enabled."); df=gradFunc(x).transpose(); return 0; }
	// int fdf(const VectorType &x, ValueType &_f, JacobianType &_df) const { int ret=1; (*this)(x,_f); ret+=df(x,_df); return ret; }
};

typedef ParticleBFGSFunctor<Real,3> ParticleBFGSFunctor_R3;
typedef Eigen::NumericalDiff<ParticleBFGSFunctor_R3,/*mode=*/ /* Eigen::Central */ Eigen::Forward> ParticleBFGSFunctor_R3_numDiff;

string BFGS_status2str(int status){
	#define CASE_STATUS(st) case BFGSSpace::st: return string(#st);
	switch(status){
		CASE_STATUS(NegativeGradientEpsilon);
		CASE_STATUS(NotStarted);
		CASE_STATUS(Running);
		CASE_STATUS(Success);
		CASE_STATUS(NoProgress);
	}
	#undef CASE_STATUS
	throw std::logic_error(("BFGS_status2str: unknown status number "+to_string(status)));
}



CREATE_LOGGER(Cg2_Shape_Shape_L6Geom__Potential);
bool Cg2_Shape_Shape_L6Geom__Potential::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const auto& f1=potentialDispatcher->getFunctor1D(s1);
	const auto& f2=potentialDispatcher->getFunctor1D(s2);
	if(!f1 || !f2) return false; // unable to handle this contact
	auto l1=f1->funcPotentialValue(s1); auto l2=f2->funcPotentialValue(s2);
	Real refLen=(s1->nodes[0]->pos-s2->nodes[0]->pos).norm();
	//auto functor=Eigen::NumericalDiff<ParticleBFGSFunctor_R3>(PotentialFunctor::PotFuncType([&](const Vector3r& pt)->Real{ return l1(pt)+l2(pt); }),refLen*1e-6);
	auto functor=ParticleBFGSFunctor_R3_numDiff(ParticleBFGSFunctor_R3(PotentialFunctor::PotFuncType([&](const Vector3r& pt)->Real{ return l1(pt)+l2(pt); })),refLen*1e-6);
	// scaling factor for numerics
	BFGS<ParticleBFGSFunctor_R3_numDiff> solver(functor);
	// initial solution?
	Vector3r solution;
	if(!C->isReal()){
		solution=.5*(s1->nodes[0]->pos+s2->nodes[0]->pos);
	} else {
		solution=C->geom->node->pos;
	}

	LOG_TRACE("BFGS: initial "<<solution.transpose());
	ParticleBFGSFunctor_R3::JacobianType jac; functor.df(solution,jac);
	LOG_TRACE("BFGS: gradient "<<jac);
	#if 1
		int status=solver.minimizeInit(solution);
		LOG_TRACE("BFGS: initialized, status="<<status<<" "<<BFGS_status2str(status));
		do{
			LOG_TRACE("BFGS: gradient="<<solver.gradient.transpose()<<", pnorm="<<solver.pnorm<<", g0norm="<<solver.g0norm<<", fp0="<<solver.fp0);
			status=solver.minimizeOneStep(solution);
			LOG_TRACE("BFGS: solution step, solution="<<solution.transpose()<<", status="<<status<<" "<<BFGS_status2str(status));
		} while(status==BFGSSpace::Running);
	#else
		int status=solver.minimize(solution);
	#endif
	LOG_TRACE("BFGS: final solution "<<solution.transpose()<<", status="<<status<<" "<<BFGS_status2str(status));
	// solution found
	if(status==BFGSSpace::Success){
		Real p1=l1(solution), p2=l2(solution);
		cerr<<"Solution "<<solution.transpose()<<" found after "<<solver.iter<<" iterations; potentials are "<<p1<<" "<<p2<<endl;
		if(p1*p2<0) throw std::runtime_error("Potential have differing signs at the solution point.");
		// compute the gradient
		ParticleBFGSFunctor_R3_numDiff f2(PotentialFunctor::PotFuncType([&](const Vector3r& pt)->Real{ return l1(pt)-l2(pt); }));
		ParticleBFGSFunctor_R3::JacobianType jac;
		f2.df(solution,jac);
		Vector3r grad(jac.transpose());
		cerr<<"Gradient (contact normal) at the solution point: "<<grad.transpose()<<endl;
		Real r1=s1->equivRadius(), r2=s2->equivRadius();
		if(!(r1>0) && !(r2>0)) throw std::runtime_error("Shape.equivRadius: at least one should be positive ("+to_string(r1)+" for "+s1->pyStr()+", "+to_string(r2)+" for "+s2->pyStr()+")");
		if(!(r1>0)) r1=-r2;
		if(!(r2>0)) r2=-r1;
		Real uN=-p1-p2;
		const auto& dyn1=s1->nodes[0]->getData<DemData>();
		const auto& dyn2=s2->nodes[0]->getData<DemData>();
		handleSpheresLikeContact(C,s1->nodes[0]->pos,dyn1.vel,dyn1.angVel,s2->nodes[0]->pos,dyn2.vel,dyn2.angVel,grad,solution,uN,r1,r2);
	} else {
		// no solution found?
		throw std::runtime_error("BFGS: no solution, status "+BFGS_status2str(status));
	}
	return true;
}

