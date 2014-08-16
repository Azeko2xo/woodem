#include<woo/pkg/dem/Potential.hpp>

#include<unsupported/Eigen/NonLinearOptimization>

WOO_PLUGIN(dem,(PotentialFunctor)(PotentialDispatcher)(Pot1_Sphere)(Pot1_Wall)(Cg2_Shape_Shape_L6Geom__Potential));

WOO_IMPL__CLASS_BASE_DOC_PY(woo_dem_PotentialFunctor__CLASS_BASE_DOC_PY);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Pot1_Sphere__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Pot1_Wall__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cg2_Shape_Shape_L6Geom__Potential__CLASS_BASE_DOC_ATTRS);


std::function<Real(const Vector3r&)> PotentialFunctor::funcPotentialValue(const shared_ptr<Shape>& s){
	throw std::runtime_error(pyStr()+".funcPotentialValue: not overridden (called with shape "+s->pyStr()+").");
}
std::function<Real(const Vector3r&)> PotentialFunctor::funcPotentialGradient(const shared_ptr<Shape>&){
	// return empty function by default
	return std::function<Real(const Vector3r&)>();
}
Real PotentialFunctor::go(const shared_ptr<Shape>& s, const Vector3r& pt){
	return funcPotentialValue(s)(pt);
}


Real Pot1_Sphere::boundingSphereRadius(const shared_ptr<Shape>& s){ return s->cast<Sphere>().radius; }

std::function<Real(const Vector3r&)> Pot1_Sphere::funcPotentialValue(const shared_ptr<Shape>& s){
	return std::function<Real(const Vector3r&)>([&](const Vector3r& pt)->Real{ return (pt-s->nodes[0]->pos).norm()-s->cast<Sphere>().radius; });
}

std::function<Real(const Vector3r&)> Pot1_Wall::funcPotentialValue(const shared_ptr<Shape>& s){
	const auto& w(s->cast<Wall>());
	if(w.sense!=-1 && w.sense!=1) throw std::runtime_error("Pot1_Wall: Wall.sense must be +1 or -1, not "+to_string(w.sense)+".");
	std::function<Real(const Vector3r&)> f=[&](const Vector3r& pt)->Real{ return w.sense*(pt[w.axis]-w.nodes[0]->pos[w.axis]); };
	return f;
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

#if EIGEN_WORLD_VERSION==3 && EIGEN_MAJOR_VERSION<3
	// copy over from newer Eigen
	template <typename _Scalar, int NX=Eigen::Dynamic, int NY=Eigen::Dynamic>
	struct DenseFunctor
	{
	 typedef _Scalar Scalar;
	 enum {
	 InputsAtCompileTime = NX,
	 ValuesAtCompileTime = NY
	 };
	 typedef Eigen::Matrix<Scalar,InputsAtCompileTime,1> InputType;
	 typedef Eigen::Matrix<Scalar,ValuesAtCompileTime,1> ValueType;
	 typedef Eigen::Matrix<Scalar,ValuesAtCompileTime,InputsAtCompileTime> JacobianType;
	 typedef Eigen::ColPivHouseholderQR<JacobianType> QRSolver;
	 const int m_inputs, m_values;

	 DenseFunctor() : m_inputs(InputsAtCompileTime), m_values(ValuesAtCompileTime) {}
	 DenseFunctor(int inputs, int values) : m_inputs(inputs), m_values(values) {}

	 int inputs() const { return m_inputs; }
	 int values() const { return m_values; }

	 //int operator()(const InputType &x, ValueType& fvec) { }
	 // should be defined in derived classes

	 //int df(const InputType &x, JacobianType& fjac) { }
	 // should be defined in derived classes
	};
#else
	using Eigen::DenseFunctor; // later versions of Eigen support that natively
#endif

struct ParticlePotentialFunctor: public DenseFunctor<Real> {
	std::function<Real(const Vector3r&)> potFunc;
	std::function<Vector3r(const Vector3r&)> gradFunc; // unset by default
	ParticlePotentialFunctor(): DenseFunctor(3,1) {}
	int operator()(const InputType& x, ValueType& p) const {
		cerr<<"["<<x.transpose()<<": "<<potFunc(x)<<"]";
		p(0,0)=potFunc(x);
		return 0;
	}
	int df(const InputType &x, JacobianType& fjac) { fjac=gradFunc(x); return 0; }
};

typedef Eigen::NumericalDiff<ParticlePotentialFunctor,/*mode=*/Eigen::Central/*Eigen::Forward*/> ParticlePotentialFunctor_numDiff;
typedef Eigen::LevenbergMarquardt<ParticlePotentialFunctor_numDiff,Real> SolverLM;

string solverLM_status2str(int status){
	#define CASE_STATUS(st) case Eigen::LevenbergMarquardtSpace::st: return string(#st);
	switch(status){
		CASE_STATUS(NotStarted);
		CASE_STATUS(Running);
		CASE_STATUS(ImproperInputParameters);
		CASE_STATUS(RelativeReductionTooSmall);
		CASE_STATUS(RelativeErrorTooSmall);
		CASE_STATUS(RelativeErrorAndReductionTooSmall);
		CASE_STATUS(CosinusTooSmall);
		CASE_STATUS(TooManyFunctionEvaluation);
		CASE_STATUS(FtolTooSmall);
		CASE_STATUS(XtolTooSmall);
		CASE_STATUS(GtolTooSmall);
		CASE_STATUS(UserAsked);
	}
	#undef CASE_STATUS
	throw std::logic_error(("solverLM_status2str called with unknown status number "+lexical_cast<string>(status)).c_str());
}



CREATE_LOGGER(Cg2_Shape_Shape_L6Geom__Potential);
bool Cg2_Shape_Shape_L6Geom__Potential::go(const shared_ptr<Shape>& s1, const shared_ptr<Shape>& s2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	const auto& f1=potentialDispatcher->getFunctor1D(s1);
	const auto& f2=potentialDispatcher->getFunctor1D(s2);
	if(!f1 || !f2) return false; // unable to handle this contact
	auto l1=f1->funcPotentialValue(s1); auto l2=f2->funcPotentialValue(s2);
	ParticlePotentialFunctor_numDiff functor;
	functor.potFunc=std::function<Real(const Vector3r&)>([&](const Vector3r& pt)->Real{ return l1(pt)+l2(pt); });
	SolverLM solver(functor);
	// initial solution?
	Vector3r solution;
	if(!C->isReal()){
		solution=.5*(s1->nodes[0]->pos+s2->nodes[0]->pos);
	} else {
		solution=C->geom->node->pos;
	}
	VectorXr solution2(solution);
	LOG_TRACE("LM: initial "<<solution2.transpose());
	#if 1
		int status=solver.minimizeInit(solution2);
		LOG_TRACE("LM: initialized, status="<<status<<" "<<solverLM_status2str(status));
		do{
			status=solver.minimizeOneStep(solution2);
			LOG_TRACE("LM: solution step, solution="<<solution2.transpose()<<", status="<<status<<" "<<solverLM_status2str(status));
		} while(status==Eigen::LevenbergMarquardtSpace::Running);
	#else
		int status=solver.minimize(solution2);
	#endif
	LOG_TRACE("LM: final solution "<<solution2.transpose()<<", status="<<status<<" "<<solverLM_status2str(status));
	solution=solution2;
	// solution found
	if(status==Eigen::LevenbergMarquardtSpace::RelativeErrorTooSmall || status==Eigen::LevenbergMarquardtSpace::RelativeErrorAndReductionTooSmall || status==Eigen::LevenbergMarquardtSpace::RelativeReductionTooSmall || status==Eigen::LevenbergMarquardtSpace::CosinusTooSmall){
		Real p1=l1(solution), p2=l2(solution);
		cerr<<"Solution found at"<<solution.transpose()<<" after "<<solver.nfev<<" iterations; potentials are "<<p1<<" "<<p2<<endl;
		if(p1*p2<0) throw std::runtime_error("Potential have differing signs at the solution point.");
		// compute the gradient
		ParticlePotentialFunctor_numDiff f2;
		f2.potFunc=std::function<Real(const Vector3r&)>([&](const Vector3r& pt)->Real{ return l1(pt)-l2(pt); });
		MatrixXr jac(3,3);
		f2.df(solution,jac);
		cerr<<"Jacobian at the solution point: "<<jac<<endl;
		Vector3r normal(jac(0,0),jac(1,0),jac(2,0)); 
		Real r1=s1->equivRadius(), r2=s2->equivRadius();
		if(!(r1>0) && !(r2>0)) throw std::runtime_error("Shape.equivRadius: at least one should be positive ("+to_string(r1)+" for "+s1->pyStr()+", "+to_string(r2)+" for "+s2->pyStr()+")");
		if(!(r1>0)) r1=-r2;
		if(!(r2>0)) r2=-r1;
		Real uN=-p1-p2;
		const auto& dyn1=s1->nodes[0]->getData<DemData>();
		const auto& dyn2=s2->nodes[0]->getData<DemData>();
		handleSpheresLikeContact(C,s1->nodes[0]->pos,dyn1.vel,dyn1.angVel,s2->nodes[0]->pos,dyn2.vel,dyn2.angVel,normal,solution,uN,r1,r2);
	} else {
		// no solution found?
		throw std::runtime_error("LevenbergMarquardt minimizer found no solution?!");
	}
	return true;
}

