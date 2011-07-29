#pragma once

#ifdef YADE_VTK

#include<yade/core/Field.hpp>
#include<yade/core/Scene.hpp>

#include<vtkPointLocator.h>
#include<vtkIdList.h>
#include<vtkUnstructuredGrid.h>
#include<vtkPoints.h>

#include<unsupported/Eigen/NonLinearOptimization>
#include<unsupported/Eigen/MatrixFunctions>




class vtkPointLocator;
class vtkPoints;
class vtkUnstructuredGrid;

struct SparcField: public Field{
	struct Engine: public Field::Engine{
		virtual bool acceptsField(Field* f){ return dynamic_cast<SparcField*>(f); }
	};
	vtkPointLocator* locator;
	vtkPoints* points;
	vtkUnstructuredGrid* grid;
	// return nodes around x not further than radius
	// if count is given, only count closest nodes are returned; radius is the initial radius, which will be however expanded, if insufficient number of points is found.
	// count does not include self in this case; not finding self in the result throws an exception
	std::vector<shared_ptr<Node> > nodesAround(const Vector3r& x, int count=-1, Real radius=-1, const shared_ptr<Node>& self=shared_ptr<Node>());
	void updateLocator();
	~SparcField(){ locator->Delete(); points->Delete(); grid->Delete(); }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(SparcField,Field,"Field for SPARC meshfree method",
		// ((Real,maxRadius,-1,,"Maximum radius for neighbour search (required for periodic simulations)"))
		((bool,locDirty,true,Attr::readonly,"Flag whether the locator is updated."))
		((bool,showNeighbors,false,,"Whether to show neighbors in the 3d view (FIXME: should go to Gl1_SparcField, once it is created). When a node is selected, neighbors are shown nevertheless."))
		,/*ctor*/ locator=vtkPointLocator::New(); points=vtkPoints::New(); grid=vtkUnstructuredGrid::New(); grid->SetPoints(points); locator->SetDataSet(grid);
		,/*py*/
			.def("nodesAround",&SparcField::nodesAround,(py::arg("pt"),py::arg("radius")=-1,py::arg("count")=-1,py::arg("ptNode")=shared_ptr<Node>()),"Return array of nodes close to given point *pt*")
			.def("updateLocator",&SparcField::updateLocator,"Update the locator, should be done manually before the first step perhaps.")
	);
};
REGISTER_SERIALIZABLE(SparcField);

#define SPARC_INSPECT


struct SparcData: public NodeData{
	Matrix3r getD() const{ return .5*(gradV+gradV.transpose()); }
	Matrix3r getW() const{ return .5*(gradV-gradV.transpose()); }
	py::list getGFixedV(const Quaternionr& ori){ return getGFixedAny(fixedV,ori); }
	py::list getGFixedT(const Quaternionr& ori){ return getGFixedAny(fixedT,ori); }
	py::list getGFixedAny(const Vector3r& any, const Quaternionr& ori);
	void catchCrap1(int nid, const shared_ptr<Node>&);
	void catchCrap2(int nid, const shared_ptr<Node>&);
	// Real getDirVel(size_t i) const { return i<dirVels.size()?dirVels[i]:0.; }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(SparcData,NodeData,"Nodal data needed for SPARC; everything is in global coordinates, except for constraints (fixedV, fixedT)",

		((Matrix3r,T,Matrix3r::Zero(),,"Stress"))
		((Vector3r,v,Vector3r::Zero(),,"Velocity"))
		((Real,rho,0,,"Density"))
		((Real,e,0,,"Porosity"))
		((Real,color,Mathr::UnitRandom(),Attr::noGui,"Set node color, so that rendering is more readable"))
		((Vector3r,fixedV,Vector3r(NaN,NaN,NaN),,"Prescribed velocity, in node-local (!!) coordinates. NaN prescribes noting along respective axis."))
		((Vector3r,fixedT,Vector3r(NaN,NaN,NaN),,"Prescribed stress divergence, in node-local (!!) coordinates. NaN prescribes nothing along respective axis."))
		// storage within the step
		((vector<shared_ptr<Node> >,neighbors,,Attr::noGui,"List of neighbours, updated internally"))
		((Vector3r,accel,Vector3r::Zero(),,"Acceleration"))
		((Matrix3r,Tdot,Matrix3r::Zero(),,"Jaumann Stress rate")) 
		((MatrixXr,relPosInv,,,"Relative positions' pseudo-inverse"))
		#ifdef SPARC_WEIGHTS
			((VectorXr,rWeights,,,"Weight when distance weighting is effective"))
		#endif
		((Matrix3r,gradV,Matrix3r::Zero(),,"gradient of velocity (only used as intermediate storage)"))
		// static equilibrium solver data
			((int,nid,-1,,"Node id (to locate coordinates in solution matrix)"))
	#ifdef SPARC_INSPECT
		// debugging only
		((MatrixXr,relPos,,Attr::noSave,"Debug storage for relative positions"))
		((Matrix3r,Tcirc,Matrix3r::Zero(),Attr::noSave,"Debugging only -- stress rate"))
		((Vector3r,divT,Vector3r::Zero(),Attr::noSave,"Debugging only -- stress divergence"))
		((Vector3r,resid,Vector3r::Zero(),Attr::noSave,"Debugging only -- implicit solver residuals for global DoFs"))
		((MatrixXr,relVels,,Attr::noSave,"Debugging only -- relative neighbor velocities"))
	#endif
		// ((int,locatorId,-1,Attr::hidden,"Position in the point locator array"))
		, /* ctor */
		, /*py*/
		.def("_getDataOnNode",&Node::pyGetData<SparcData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<SparcData>).staticmethod("_setDataOnNode")
		.add_property("D",&SparcData::getD).add_property("W",&SparcData::getW)
		.def("gFixedV",&SparcData::getGFixedV).def("gFixedT",&SparcData::getGFixedT)
		;
	);
};
REGISTER_SERIALIZABLE(SparcData);

template<> struct NodeData::Index<SparcData>{enum{value=Node::ST_SPARC};};

struct ExplicitNodeIntegrator: public GlobalEngine, private SparcField::Engine{
	enum{ MAT_HOOKE=0, MAT_BARODESY_JESSE, MAT_SENTINEL /* to check max value */ };
	SparcField* mff; // lazy to type
	void findNeighbors(const shared_ptr<Node>& n) const;
	void updateNeighborsRelPos(const shared_ptr<Node>& n, bool useNext=false) const;
	Vector3r computeDivT(const shared_ptr<Node>& n, bool useNext=false) const;
	Matrix3r computeGradV(const shared_ptr<Node>& n) const;
	Matrix3r computeStressRate(const Matrix3r& T, const Matrix3r& D, Real e=-1) const;
	// porosity updated using current deformation rate
	Real nextPorosity(Real e, const Matrix3r& D) const { return e+scene->dt*(1+e)*D.trace(); }
	void applyKinematicConstraints(const shared_ptr<Node>& n, bool permitFixedDivT) const;
	Matrix6r C; // updated at every step
	void postLoad(ExplicitNodeIntegrator&);
	virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(ExplicitNodeIntegrator,GlobalEngine,"Monolithic engine for explicit integration of motion of nodes in SparcField.",
		((Real,E,1e6,Attr::triggerPostLoad,"Young's modulus, for the linear elastic constitutive law"))
		((Real,nu,0,Attr::triggerPostLoad,"Poisson's ratio for the linear elastic constitutive law"))
		((vector<Real>,barodesyC,vector<Real>({-1.7637,-1.0249,-0.5517,-1174.,-4175.,2218}),Attr::triggerPostLoad,"Material constants for barodesy"))
		((Real,ec0,.8703,,"Initial void ratio"))
		((Real,rSearch,-1,,"Radius for neighbor-search"))
		((int,rPow,0,,"Exponent for distance weighting ∈{0,1,2}"))
		((int,neighborUpdate,1,,"Number of steps to periodically update neighbour information"))
		((int,matModel,0,Attr::triggerPostLoad,"Material model to be used (0=linear elasticity, 1=barodesy (Jesse)"))
		((Real,damping,0,,"Numerical damping, applied by-component on acceleration"))
		((Real,c,0,,"Viscous damping coefficient."))
		,/*ctor*/
		,/*py*/
		.def("stressRate",&ExplicitNodeIntegrator::computeStressRate,(py::arg("T"),py::arg("D"),py::arg("e")=-1)) // for debugging
		.def_readonly("C",&ExplicitNodeIntegrator::C)
	);
};
REGISTER_SERIALIZABLE(ExplicitNodeIntegrator);

struct StaticEquilibriumSolver: public ExplicitNodeIntegrator{
	struct StressDivergenceFunctor{
		typedef Real Scalar;
		enum { InputsAtCompileTime=Eigen::Dynamic, ValuesAtCompileTime=Eigen::Dynamic};
		typedef VectorXr InputType;
		typedef VectorXr ValueType;
		typedef MatrixXr JacobianType;
		const int m_inputs, m_values;
		int inputs() const { return m_inputs; }
		int values() const { return m_values; }
		StaticEquilibriumSolver* ses;
		StressDivergenceFunctor(int inputs, int values, StaticEquilibriumSolver* _ses): m_inputs(inputs), m_values(values), ses(_ses){}
		int operator()(const VectorXr &v, VectorXr& resid) const;
	};
	typedef Eigen::HybridNonLinearSolver<StressDivergenceFunctor> SolverT;
	shared_ptr<SolverT> solver;
	shared_ptr<StressDivergenceFunctor> functor;
	VectorXr vv;
	int nFactorLowered;

	virtual void run();
	void renumberNodes() const;
	void copyVelocityToNodes(const VectorXr&) const;
	void applyConstraintsAsResiduals(const shared_ptr<Node>& n, Vector3r& divT, bool useNextT) const ;
	VectorXr computeInitialVelocities() const;
	void integrateSolution() const;
	VectorXr trySolution(const VectorXr& vv);
	VectorXr currResid() const;
	void dumpUnbalanced();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(StaticEquilibriumSolver,ExplicitNodeIntegrator,"Find global static equilibrium of a Sparc system.",
		((bool,substep,false,,"Whether the solver tries to find solution within one step, or does just one iteration towards the solution"))
		((int,nIter,0,Attr::readonly,"Whether we are at the start of the solution (0), or further down the way?"))
		((Real,supportStiffness,1e10,,"Stiffness of constrained DoFs"))
		((VectorXr,solution,,Attr::readonly,"Current solution which the solver computes"))
		((VectorXr,residuals,,Attr::readonly,"Residuals corresponding to the current solution"))
		((Real,residuum,NaN,Attr::readonly,"Norm of residuals (fnorm) as reported by the solver."))
		((Real,solverFactor,200,,"Factor for the Dogleg method (automatically lowered in case of convergence troubles"))
		((Real,relMaxfev,10000,,"Maximum number of function evaluation in solver, relative to number of DoFs"))
		((int,watch,-1,,"Nid to be watched (debugging)."))
		#ifdef SPARC_INSPECT
			((VectorXr,solverDivT,,Attr::readonly,"Solution vector as provided by the solver"))
		#endif
		, /* ctor */
		, /* py */ .def("trySolution",&StaticEquilibriumSolver::trySolution)
			.def("currResid",&StaticEquilibriumSolver::currResid)
	);

};
REGISTER_SERIALIZABLE(StaticEquilibriumSolver);

#ifdef YADE_OPENGL
#include<yade/pkg/gl/NodeGlRep.hpp>
struct SparcConstraintGlRep: public NodeGlRep{
	void render(const shared_ptr<Node>&, GLViewInfo*);
	void renderLabeledArrow(const Vector3r& pos, const Vector3r& vec, const Vector3r& color, Real num, bool posIsA, bool doubleHead=false);
	YADE_CLASS_BASE_DOC_ATTRS(SparcConstraintGlRep,NodeGlRep,"Render static and kinematic constraints on Sparc nodes",
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
#endif // YADE_OPENGL


#endif // YADE_VTK
