#pragma once
#ifdef WOO_SPARC

#ifdef WOO_VTK

// moved to features
// #define WOO_SPARC

#include<woo/core/Field.hpp>
#include<woo/core/Scene.hpp>
#include<woo/core/Field-templates.hpp>

#include<vtkPointLocator.h>
#include<vtkIdList.h>
#include<vtkUnstructuredGrid.h>
#include<vtkPoints.h>

#ifdef WOO_DEBUG
	// uncomment to trace most calculations in files, slows down!
	//#define DOGLEG_DEBUG
#endif

#include<unsupported/Eigen/NonLinearOptimization>
#include<unsupported/Eigen/MatrixFunctions>

// trace many intermediate numbers in a file given by StaticEquilibriumSolver::dbgOut
#ifdef WOO_DEBUG
	#define SPARC_TRACE
#endif

#ifdef SPARC_TRACE
	#define SPARC_TRACE_OUT(a) out<<a
	#define SPARC_TRACE_SES_OUT(a) ses->out<<a
#else
	#define SPARC_TRACE_OUT(a)
	#define SPARC_TRACE_SES_OUT(a)
#endif

// enable Levenberg-Marquardt solver; not useful until functor can evaluate the jacobian
#define SPARC_LM


class vtkPointLocator;
class vtkPoints;
class vtkUnstructuredGrid;

struct SparcField: public Field{
	vtkPointLocator* locator;
	vtkPoints* points;
	vtkUnstructuredGrid* grid;
	// return nodes around x not further than radius
	// if count is given, only count closest nodes are returned; radius is the initial radius, which will be however expanded, if insufficient number of points is found.
	// count does not include self in this case; not finding self in the result throws an exception
	std::vector<shared_ptr<Node> > nodesAround(const Vector3r& x, int count=-1, Real radius=-1, const shared_ptr<Node>& self=shared_ptr<Node>(), Real* relLocPtDensity=NULL, const Vector2i& mnMxPts=Vector2i::Zero());
	void constructLocator();

	template<bool useNext=false>
	void updateLocator();

	DECLARE_LOGGER;

	~SparcField(){ locator->Delete(); points->Delete(); grid->Delete(); }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(SparcField,Field,"Field for SPARC meshfree method",
		// ((Real,maxRadius,-1,,"Maximum radius for neighbour search (required for periodic simulations)"))
		((bool,locDirty,true,AttrTrait<Attr::readonly>(),"Flag whether the locator is updated."))
		((Vector2r,neighAdjFact,Vector2r(.5,2.),,"Factors for adjusting neighbor search range."))
		((int,neighAdjSteps,10,,"Maximum range for adjusting neighbor search range"))
		((int,neighRelMax,3,,"Try to keep number of neighbors below *neighRelMax* × basis dimension (which is hard minimum of neighbors)."))
		,/*ctor*/  createIndex(); constructLocator(); 
		,/*py*/
			.def("nodesAround",&SparcField::nodesAround,(py::arg("pt"),py::arg("radius")=-1,py::arg("count")=-1,py::arg("ptNode")=shared_ptr<Node>()),"Return array of nodes close to given point *pt*")
			.def("updateLocator",&SparcField::updateLocator</*useNext*/false>,"Update the locator, should be done manually before the first step perhaps.")
			.def("sceneHasField",&Field_sceneHasField<SparcField>).staticmethod("sceneHasField")
			.def("sceneGetField",&Field_sceneGetField<SparcField>).staticmethod("sceneGetField")

	);
};
REGISTER_SERIALIZABLE(SparcField);

#define SPARC_INSPECT


struct SparcData: public NodeData{
	Matrix3r getD() const{ return .5*(gradV+gradV.transpose()); }
	Matrix3r getW() const{ return .5*(gradV-gradV.transpose()); }
	Quaternionr getRotQ(const Real& dt) const;
	Vector3r getRotVec(){ Quaternionr q(getRotQ(Master::instance().getScene()->dt)); AngleAxisr aa(q); return aa.axis()*aa.angle(); }
	py::list getGFixedV(const Quaternionr& ori){ return getGFixedAny(fixedV,ori); }
	py::list getGFixedT(const Quaternionr& ori){ return getGFixedAny(fixedT,ori); }
	py::list getGFixedAny(const Vector3r& any, const Quaternionr& ori);
	void catchCrap1(int nid, const shared_ptr<Node>&);
	void catchCrap2(int nid, const shared_ptr<Node>&);
	// Real getDirVel(size_t i) const { return i<dirVels.size()?dirVels[i]:0.; }
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(SparcData,NodeData,"Nodal data needed for SPARC; everything is in global coordinates, except for constraints (fixedV, fixedT)",
		// informational
		((Real,color,Mathr::UnitRandom(),AttrTrait<>().noGui(),"Set node color, so that rendering is more readable"))
		((int,nid,-1,,"Node id (to locate coordinates in solution matrix)"))
		((Real,relLocPtDensity,1.,,"Local density of points, by which :obj:`ExplicitNodeIntegrator.rSearch` is multiplied; automatically adjusted so that enough neighbor points are found, and that they are not too many."))

		// state variables
		((Matrix3r,T,Matrix3r::Zero(),,"Stress"))
		((Real,rho,0,,"Density"))
		((Real,e,0,,"Porosity"))

		// prescribed values
		((Vector3r,fixedV,Vector3r(NaN,NaN,NaN),,"Prescribed velocity, in node-local (!!) coordinates. NaN prescribes noting along respective axis."))
		((Vector3r,fixedT,Vector3r(NaN,NaN,NaN),,"Prescribed stress divergence, in node-local (!!) coordinates. NaN prescribes nothing along respective axis."))

		// computed in prologue (once per timestep)
		((Vector3i,dofs,Vector3i(-1,-1,-1),AttrTrait<Attr::readonly>(),"Degrees of freedom in the solution system corresponding to 3 locV components (negative for prescribed velocity, not touched by the solver)"))
		((vector<shared_ptr<Node> >,neighbors,,AttrTrait<>().noGui(),"List of neighbours, updated internally"))
		//((MatrixXr,relPosInv,,,"Relative positions' pseudo-inverse, with INTERP_KOLY."))
		#ifdef SPARC_INSPECT
			((MatrixXr,stencil,,,"Stencil matrix, with INTERP_WLS."))
			((MatrixXr,bVec,,,"Matrix with base functions derivatives evaluated at stencil points (4 columns: 0th, x, y, z derivative)"))
			((MatrixXr,weightSq,,,"Vector with squared weights of neighbor points"))
		#endif
		((MatrixXr,dxDyDz,,,"Operator matrix built from bVec^T*stencil"))

		// recomputed in each solver iteration
		((Vector3r,locV,Vector3r::Zero(),,"Velocity in local coordinates"))
		((Vector3r,v,Vector3r::Zero(),,"Velocity"))
		((Matrix3r,gradV,Matrix3r::Zero(),,"gradient of velocity (only used as intermediate storage)"))
		((Vector3r,Lout,Vector3r::Zero(),,"diagonal components of gradV for degenerate space problems. Lout[0] is only used with 0d basis, Lout[1] with 0d and 1d basis, Lout[2] with 0d, 1d and 2d basis."))
		// ((Matrix3r,Tdot,Matrix3r::Zero(),,"Jaumann Stress rate")) 
		((Matrix3r,nextT,Matrix3r::Zero(),,"Stress in the next step")) 
		((Quaternionr,nextOri,,,"Orientation in the next step (FIXME: not yet updated)"))
	#if 0
		((vector<shared_ptr<Node> >,nextNeighbors,,AttrTrait<>().noGui(),"Neighbors in the next step"))
		((VectorXr,nextWeights,,,"Weights in t+dt/2, with positions updated as per v"))
		((MatrixXr,nextRelPosInv,,,"Relative position's pseudoinverse in the next step"))
	#endif

		// explicit solver
		((Vector3r,accel,Vector3r::Zero(),,"Acceleration"))
	#ifdef SPARC_INSPECT
		// debugging only
		((MatrixXr,relPos,,AttrTrait<Attr::noSave>().noGui(),"Debug storage for relative positions"))
		((MatrixXr,nextRelPos,,AttrTrait<Attr::noSave>().noGui(),"Relative positions in next step"))
		((MatrixXr,gradT,,AttrTrait<Attr::noSave>().noGui(),"Debugging only -- derivatives of stress components (6 lines, one for each voigt-component of stress tensor); evaluated as by-product in computeDivT"))

		((Matrix3r,Tcirc,Matrix3r::Zero(),AttrTrait<Attr::noSave>(),"Debugging only -- stress rate"))
		((Vector3r,divT,Vector3r::Zero(),AttrTrait<Attr::noSave>(),"Debugging only -- stress divergence"))
		((Vector3r,resid,Vector3r::Zero(),AttrTrait<Attr::noSave>().noGui(),"Debugging only -- implicit solver residuals for global DoFs"))
		((MatrixXr,relVels,,AttrTrait<Attr::noSave>().noGui(),"Debugging only -- relative neighbor velocities"))
	#endif
		// ((int,locatorId,-1,AttrTrait<Attr::hidden>(),"Position in the point locator array"))
		, /* ctor */
		, /*py*/
		.def("_getDataOnNode",&Node::pyGetData<SparcData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<SparcData>).staticmethod("_setDataOnNode")
		.add_property("D",&SparcData::getD).add_property("W",&SparcData::getW).add_property("rot",&SparcData::getRotVec)
		.def("gFixedV",&SparcData::getGFixedV).def("gFixedT",&SparcData::getGFixedT)
		;
	);
};
REGISTER_SERIALIZABLE(SparcData);

template<> struct NodeData::Index<SparcData>{enum{value=Node::ST_SPARC};};

struct ExplicitNodeIntegrator: public GlobalEngine {
	bool acceptsField(Field* f){ return dynamic_cast<SparcField*>(f); }

	enum{ MAT_ELASTIC=0, MAT_BARODESY, MAT_VISCOUS, MAT_SENTINEL /* to check max value */ };
	SparcField* mff; // lazy to type
	template<bool useNext>
	void findNeighbors(const shared_ptr<Node>& n) const;
	template<bool useNext>
	void updateLocalInterp(const shared_ptr<Node>& n) const;
	template<bool useNext>
	Vector3r computeDivT(const shared_ptr<Node>& n) const;

	Matrix3r computeGradV(const shared_ptr<Node>& n) const;
	Matrix3r computeStressRate(const Matrix3r& T, const Matrix3r& D, Real e=-1) const;
	// porosity updated using current deformation rate
	Real nextPorosity(Real e, const Matrix3r& D) const { return e+scene->dt*(1+e)*D.trace(); }
	void applyKinematicConstraints(const shared_ptr<Node>& n, bool permitFixedDivT) const;
	Matrix6r C; // updated at every step
	void postLoad(ExplicitNodeIntegrator&,void*);
	virtual void run();
	Real pointWeight(Real distSq, Real relLocPtDensity=1.) const;

	void setWlsBasisFuncs();
	typedef vector<std::function<Real(const Vector3r&)>> vecReal3dFunc;
	vecReal3dFunc wlsPhi;
	vecReal3dFunc wlsPhiDx;
	vecReal3dFunc wlsPhiDy;
	vecReal3dFunc wlsPhiDz;

	DECLARE_LOGGER;

	enum {WEIGHT_DIST=0,WEIGHT_GAUSS,WEIGHT_SENTINEL};
	enum {WLS_EMPTY=0,WLS_QUAD_X,WLS_CUBIC_X,WLS_LIN_XY,WLS_QUAD_XY,WLS_LIN_XYZ,WLS_QUAD_XYZ};
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ExplicitNodeIntegrator,GlobalEngine,"Monolithic engine for explicit integration of motion of nodes in SparcField.",
		((Real,E,1e6,AttrTrait<Attr::triggerPostLoad>(),"Young's modulus, for the linear elastic constitutive law"))
		((Real,nu,0,AttrTrait<Attr::triggerPostLoad>(),"Poisson's ratio for the linear elastic constitutive law"))
		((vector<Real>,barodesyC,vector<Real>({-1.7637,-1.0249,-0.5517,-1174.,-4175.,2218}),AttrTrait<Attr::triggerPostLoad>(),"Material constants for barodesy"))
		((Real,ec0,.8703,,"Initial void ratio"))
		((Real,rSearch,-1,,"Radius for neighbor-search"))
		((int,weightFunc,WEIGHT_GAUSS,,"Weighting function to be used (WEIGHT_DIST,WEIGHT_GAUSS)"))
		((int,wlsBasis,WLS_QUAD_XYZ,,"Basis used for WLS interpolation."))
		((int,dim,2,AttrTrait<Attr::readonly>(),"Dimension of the basis (set automatically)"))
		((int,rPow,0,,"Exponent for distance weighting ∈{0,-1,-2,…}"))
		((Real,gaussAlpha,.6,,"Decay coefficient used with Gauss weight function."))
		((bool,spinRot,false,,"Rotate particles according to spin in their location; Dofs which prescribe velocity will never be rotated (i.e. only rotation parallel with them will be allowed)."))
		((int,neighborUpdate,1,,"Number of steps to periodically update neighbour information"))
		((int,matModel,0,AttrTrait<Attr::triggerPostLoad>(),"Material model to be used (0=linear elasticity, 1=barodesy (Jesse)"))
		((int,watch,-1,,"Nid to be watched (debugging)."))
		((Real,damping,0,,"Numerical damping, applied by-component on acceleration"))
		((Real,c,0,,"Viscous damping coefficient."))
		((bool,barodesyConvertPaToKPa,true,,"Assume stresses are given in Pa, while parameters are calibrated with kPa. This divides input stress by 1000 (at the beginning of the constitutive law routine), then computes stress rate, which is multiplied by 1000."))
		((Vector3r,Tout,Vector3r(NaN,NaN,NaN),,"Out-of-space stresses, for reduced spaces: Tout[0] is only used with 0d basis, Tout[1] with 0d and 1d bases, Tout[2] with 0d, 1d, 2d bases."))
		((bool,symm01d,true,,"With 0d or 1d basis, suppose Lxx==Lyy==Lzz (shared dofs) if respective out-of-space stresses are prescribed and identical."))
		,/*ctor*/
		,/*py*/
		.def("stressRate",&ExplicitNodeIntegrator::computeStressRate,(py::arg("T"),py::arg("D"),py::arg("e")=-1)) // for debugging
		.def_readonly("C",&ExplicitNodeIntegrator::C).def("pointWeight",&ExplicitNodeIntegrator::pointWeight);


		_classObj.attr("wGauss")=(int)WEIGHT_GAUSS;
		_classObj.attr("wDist")=(int)WEIGHT_DIST;

		_classObj.attr("matElastic")=(int)MAT_ELASTIC;
		_classObj.attr("matViscous")=(int)MAT_VISCOUS;
		_classObj.attr("matBarodesy")=(int)MAT_BARODESY;

		_classObj.attr("wlsEmpty")=(int)WLS_EMPTY;;
		_classObj.attr("wlsQuadX")=(int)WLS_QUAD_X;
		_classObj.attr("wlsCubicX")=(int)WLS_CUBIC_X;
		_classObj.attr("wlsLinXy")=(int)WLS_LIN_XY;
		_classObj.attr("wlsQuadXy")=(int)WLS_QUAD_XY;
		_classObj.attr("wlsLinXyz")=(int)WLS_LIN_XYZ;
		_classObj.attr("wlsQuadXyz")=(int)WLS_QUAD_XYZ;
		
		//.add_property("wDist",& [](){ return WEIGHT_DIST; })
		// .enum_<WeightFunc>("weight").value("dist",WEIGHT_DIST).value("gauss",WEIGHT_GAUSS)
	);
};
REGISTER_SERIALIZABLE(ExplicitNodeIntegrator);

namespace NewtonSolverSpace { 
	enum Status {
		Running = -1,
		JacobianNotInvertible = 0,
		TooManyFunctionEvaluation = 1,
		RelativeErrorTooSmall = 2,
		AbsoluteErrorTooSmall = 3,
		UserAsked = 4
	};
};

template<typename FunctorType, typename Scalar=double>
struct NewtonSolver{
	typedef typename FunctorType::JacobianType JacobianType;
	typedef typename FunctorType::InputType FVectorType;

	NewtonSolver(FunctorType &_functor): functor(_functor) {
		// number of function and jacobian evaluations
		nfev=njev=iter=0;
		fnorm=0.;
		xtol=1e-6;
		abstol=1e-6;
		jacEvery=0;
		maxfev=0;
		jacEigen=false;
		epsScale=true;
	};
	int iter, nfev, njev, maxfev;
	int jacEvery;
	bool jacEigen, epsScale;
	JacobianType jac;
	JacobianType jacInv;
	FVectorType fvec; // residuals
	//typedef typename JacobianType::Scalar Scalar;
	typedef typename JacobianType::Index Index;
	Scalar fnorm0, fnorm, xtol, abstol;
	FunctorType functor;

	NewtonSolverSpace::Status recomputeJacobian(typename FunctorType::InputType& x, int retry=0){
		jac=MatrixXr::Zero(x.size(),x.size());
		fvec=VectorXr::Zero(x.size());
		if(jacEigen){
			// use numerical differentiation from eigen
			if(functor.df(x,jac)<0) return NewtonSolverSpace::UserAsked;
		} else {
			// use our own routine (forward num diff)
			if(functor(x,fvec)<0) return NewtonSolverSpace::UserAsked;
			for(Index dof=0; dof<jac.rows(); dof++){
				// distortion
				Scalar dx, sqrtEps=sqrt(Eigen::NumTraits<Scalar>::epsilon());
				if(epsScale) { dx=sqrtEps*std::abs(x[dof]); if(dx==0) dx=sqrtEps; }
				else dx=sqrtEps;
				FVectorType x2=x; x2[dof]+=dx;
				FVectorType fvec2=VectorXr::Zero(jac.rows());
				if(functor(x2,fvec2)<0) return NewtonSolverSpace::UserAsked;
				jac.col(dof)=(fvec2-fvec).transpose()/dx;
			}
		}
		njev++;
		fnorm0=functor(x,fvec);
		jacInv=jac.inverse();
		if(isinf(jacInv.maxCoeff()) || isnan(jacInv.maxCoeff())){
			Index r,c; Scalar j=jacInv.maxCoeff(&r,&c);
			cerr<<"Maximum jacInv coeff at ("<<r<<","<<c<<") is "<<j<<endl;
			if(retry<5) return recomputeJacobian(x,retry+1);
			return NewtonSolverSpace::JacobianNotInvertible;
		}
		return NewtonSolverSpace::Running;
	};
	NewtonSolverSpace::Status solveNumericalDiffInit(typename FunctorType::InputType& x){
		nfev=njev=iter=0;
		return recomputeJacobian(x);
	};
	NewtonSolverSpace::Status solveNumericalDiffOneStep(typename FunctorType::InputType& x){
		if(jacEvery>0 && (iter%jacEvery)==0) recomputeJacobian(x);
		assert(jacInv.rows()==x.size() && jacInv.cols()==x.size());
		fvec.resize(x.size());
		if(functor(x,fvec)<0) return NewtonSolverSpace::UserAsked;
		nfev++;
		if(maxfev>0 && nfev>maxfev) return NewtonSolverSpace::TooManyFunctionEvaluation;
		fnorm=fvec.stableNorm();
		if(fnorm<xtol*fnorm0) return NewtonSolverSpace::RelativeErrorTooSmall;
		if(fnorm<abstol) return NewtonSolverSpace::AbsoluteErrorTooSmall;
		x-=jacInv*fvec; // compute new x
		iter++; // increase iteration count
		return NewtonSolverSpace::Running;
	}
};



struct StaticEquilibriumSolver: public ExplicitNodeIntegrator{
	struct ResidualsFunctorBase {
		typedef Real Scalar;
		typedef VectorXr::Index Index;
		enum { InputsAtCompileTime=Eigen::Dynamic, ValuesAtCompileTime=Eigen::Dynamic};
		typedef VectorXr InputType;
		typedef VectorXr ValueType;
		typedef MatrixXr JacobianType;
		const Index m_inputs, m_values;
		Index inputs() const { return m_inputs; }
		Index values() const { return m_values; }
		StaticEquilibriumSolver* ses;
		ResidualsFunctorBase(Index inputs, Index values): m_inputs(inputs), m_values(values){}
		enum{ MODE_TRIAL_V_IS_ARG=0, MODE_TRIAL_V_IN_NODES, MODE_CURRENT_STATE };
		int operator()(const VectorXr &v, VectorXr& resid) const;
	};
	// adds df() method to ResidualsFunctorBase; it is used by SolverLM (not by SolverPowell, where we use solverNumericalDiff* functions which evaluate Jacobian internally (and differently?))
	// defines a templated forward ctor (with const refs only :-| )
	typedef Eigen::NumericalDiff<ResidualsFunctorBase,/*mode=*/Eigen::Forward /*Eigen::Central*/> ResidualsFunctor;
	typedef Eigen::HybridNonLinearSolver<ResidualsFunctor,Real> SolverPowell;
	typedef Eigen::LevenbergMarquardt<ResidualsFunctor,Real> SolverLM;
	typedef NewtonSolver<ResidualsFunctor> SolverNewton;

	shared_ptr<ResidualsFunctor> functor;
	shared_ptr<SolverPowell> solverPowell;
	shared_ptr<SolverLM> solverLM;
	shared_ptr<SolverNewton> solverNewton;

	void solverInit(VectorXr& x);
	int solverStep(VectorXr& x);


	int nFactorLowered;
	ofstream out;

	DECLARE_LOGGER;

	virtual void run();
#if 0
	VectorXr compResid(const VectorXr& v);
#endif
	Real gradVError(const shared_ptr<Node>&, int rPow=0);

	void prologuePhase(VectorXr& initVel);
		void assignDofs();
		VectorXr computeInitialDofVelocities(bool useZero=true) const;

	void solutionPhase(const VectorXr& trialVel, VectorXr& errors);
		void solutionPhase_computeResponse(const VectorXr& trialVel);
			void copyLocalVelocityToNodes(const VectorXr& vel) const;
		void solutionPhase_computeErrors(VectorXr& errors);
			template<bool useNext>
			void computeConstraintErrors(const shared_ptr<Node>& n, const Vector3r& divT, VectorXr& errors);

	void epiloguePhase(const VectorXr& vel, VectorXr& errors);
		void integrateStateVariables();

	// make those accessible form python, for debugging
	VectorXr prologuePy(){ VectorXr ret; prologuePhase(ret); return ret; }
	VectorXr solutionPy(const VectorXr& x){ VectorXr ret(x.size()); solutionPhase(x,ret); return ret; }
	//VectorXr epiloguePhase(const VectorXr& x} VectorXr ret(x.size()); epiloguePhase(x,ret); return ret; }

	enum {DBG_JAC=1,DBG_DOFERR=2,DBG_NIDERR=4};
	enum {SOLVER_NONE=0,SOLVER_POWELL,SOLVER_LM,SOLVER_NEWTON};
	enum {PROGRESS_DONE=0,PROGRESS_RUNNING,PROGRESS_ERROR};

	WOO_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(StaticEquilibriumSolver,ExplicitNodeIntegrator,"Find global static equilibrium of a Sparc system.",
		((int,solver,SOLVER_POWELL,,"Solver type: 2: Newton-Raphson (finds zero), 0: Powell (minimization) 1: Levenberg-Marquardt."))
		((bool,substep,false,,"Whether the solver tries to find solution within one step, or does just one iteration towards the solution"))
		((Real,dt,NaN,AttrTrait<Attr::readonly>(),"Save Scene.dt here, so that it can be se to 0 during substeps. Other engines should test if S.dt!=0 before running."))
		((int,nIter,0,AttrTrait<Attr::readonly>(),"Number of iterations of the solver (in the last step, or in-progress iteration if substepping)"))
		((int,progress,PROGRESS_DONE,AttrTrait<Attr::readonly>(),"Inner status of the solver (done, progress, error)"))
		((VectorXr,currV,,AttrTrait<Attr::readonly>().noGui(),"Current solution which the solver computes"))
		#ifdef SPARC_INSPECT
			((VectorXr,residuals,,AttrTrait<Attr::readonly>().noGui(),"Residuals corresponding to the current solution (copy of error vector inside the solver)"))
			((MatrixXr,jac,,AttrTrait<Attr::readonly>().noGui(),"Jacobian matrix (if provided by the solver)"))
			((MatrixXr,jacInv,,AttrTrait<Attr::readonly>().noGui(),"Inverted Jacobian matrix (if provided by the solver)"))
		#endif
		((Real,residuum,NaN,AttrTrait<Attr::readonly>(),"Norm of residuals (fnorm) as reported by the solver."))
		((Real,solverFactor,200,,"Factor for the Dogleg method (automatically lowered in case of convergence troubles"))
		((Real,solverXtol,-1,,"Relative tolerance of the solver; if negative, default is used."))
		((Real,relMaxfev,10000,,"Maximum number of function evaluation in solver, relative to number of DoFs"))
		((int,jacEvery,10,,"Recompute the Jacobian every *jacEvery* steps, when using the Newton solver"))
		((bool,jacEigen,true,,"Use Eigen::NumericalDiff routines with the Newton solver; if false, use our own routine."))
		((bool,epsScale,true,,"When using our own numerical differentiation, enable/disable scaling of distortion based on the current value."))
		((Real,epsfcn,0.,,"Epsfcn parameter of the solver (0 = use machine precision), pg. 26 of MINPACK manual"))
		((int,nDofs,-1,,"Number of degrees of freedom, set by renumberDoFs"))
		((Real,charLen,1,,"Characteristic length, for making divT/T errors comensurable"))
		((bool,relPosOnce,false,,"Only compute relative positions when initializing solver step, using initial velocities"))
		((bool,neighborsOnce,true,,"Only compute new neighbor set in epilogue, and use it for subsequent trial solutions as well"))
		((bool,initZero,false,,"Use zero as initial solution for DoFs where velocity is not prescribed; otherwise use velocity from previous step."))
		#ifdef SPARC_TRACE
			((string,dbgOut,,,"Output file where to put debug information for detecting non-determinism in the solver"))
			((int,dbgFlags,0,,"Select what things to dump to the output file: 1: Jacobian, 2: dof residua, 4: nid residua"))
		#endif
		, /*init*/
		, /* ctor */
		, /* py */
		#if 0
			.def("compResid",&StaticEquilibriumSolver::compResid,(py::arg("vv")=VectorXr()),"Compute residuals corresponding to either given velocities *vv*, or to the current state (if *vv* is not given or empty)")
		#endif
		.def("gradVError",&StaticEquilibriumSolver::gradVError,(py::arg("node"),py::arg("rPow")=0),"Compute sum of errors from local velocity linearization (i.e. sum of errors between linear velocity field and real neighbor velocities; errors are weighted according to |x-x₀|^rPow.")
		//.def("solverInit",&StaticEquilibriumSolver::solverInitPy),"Initialize the solver with x0 as the initial solution.")
		//.def("solverStep",&StaticEquilibriumSolver::solverStepPt,(py::arg("x")),"Advance the solver by one step, with the solution x as the start")
		.def("prologue",&StaticEquilibriumSolver::prologuePy)
		.def("solution",&StaticEquilibriumSolver::solutionPy)
		// .def("epilogue",&StaticEquilibriumSolver::epiloguePy)
		;

		_classObj.attr("solverNone")=(int)SOLVER_NONE;
		_classObj.attr("solverPowell")=(int)SOLVER_POWELL;
		_classObj.attr("solverLM")=(int)SOLVER_LM;
		_classObj.attr("solverNewton")=(int)SOLVER_NEWTON;
		_classObj.attr("progressDone")=(int)PROGRESS_DONE;
		_classObj.attr("progressRunning")=(int)PROGRESS_RUNNING;
		_classObj.attr("progressError")=(int)PROGRESS_ERROR;
		_classObj.attr("dbgJac")=(int)DBG_JAC;
		_classObj.attr("dbgDofErr")=(int)DBG_DOFERR;
		_classObj.attr("dbgNidErr")=(int)DBG_NIDERR;
	);

};
REGISTER_SERIALIZABLE(StaticEquilibriumSolver);

#ifdef WOO_OPENGL
#include<woo/pkg/gl/NodeGlRep.hpp>
#include<woo/pkg/gl/Functors.hpp>


struct Gl1_SparcField: public GlFieldFunctor{
	virtual void go(const shared_ptr<Field>&, GLViewInfo*);
	Renderer* rrr; // will be removed later, once the parameters are local
	GLViewInfo* viewInfo;
	shared_ptr<SparcField> sparc; // used by do* methods
	RENDERS(SparcField);
	WOO_CLASS_BASE_DOC_STATICATTRS(Gl1_SparcField,GlFieldFunctor,"Render Sparc field.",
		((bool,nodes,true,,"Show local node coordinate systems"))
		((bool,nid,false,,"Show node ids for Sparc models"))
		((bool,neighbors,false,,"Whether to show neighbors in the 3d view (FIXME: should go to Gl1_SparcField, once it is created). When a node is selected, neighbors are shown nevertheless."))
		((vector<int>,conn,,,"Sequence of node IDs which will be connected; every invalid id (such as -1) interrupts the line."))
		((vector<Vector3r>,connColors,{Vector3r(0,1,0)},,"Colors for connecting lines (successive segments advance, colors are cycled through)"))
		/* attrs */
	);
};
REGISTER_SERIALIZABLE(Gl1_SparcField);


struct SparcConstraintGlRep: public NodeGlRep{
	void render(const shared_ptr<Node>&, const GLViewInfo*);
	void renderLabeledArrow(const Vector3r& pos, const Vector3r& vec, const Vector3r& color, Real num, bool posIsA, bool doubleHead=false);
	WOO_CLASS_BASE_DOC_ATTRS(SparcConstraintGlRep,NodeGlRep,"Render static and kinematic constraints on Sparc nodes",
		((Vector3r,fixedV,Vector3r(NaN,NaN,NaN),,"Prescribed velocity value in local coords (nan if not prescribed)"))
		((Vector3r,fixedT,Vector3r(NaN,NaN,NaN),,"Prescribed traction value in local coords (nan if not prescribed)"))
		((Vector2r,vColor,Vector2r(0,.3),,"Color for rendering kinematic constraint."))
		((Vector2r,tColor,Vector2r(.7,1),,"Color for rendering static constraint."))
		((shared_ptr<ScalarRange>,vRange,,,"Range for velocity components"))
		((shared_ptr<ScalarRange>,tRange,,,"Range for stress components"))
		((Real,relSz,.1,,"Relative size of constrain arrows"))
		((bool,num,true,,"Show numbers "))
	);
};
REGISTER_SERIALIZABLE(SparcConstraintGlRep);
#endif // WOO_OPENGL


#endif // WOO_VTK

#endif // WOO_SPARC
