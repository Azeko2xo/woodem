#pragma once

#ifdef YADE_VTK

#include<yade/core/Field.hpp>
#include<yade/core/Scene.hpp>

#include<vtkPointLocator.h>
#include<vtkIdList.h>
#include<vtkUnstructuredGrid.h>
#include<vtkPoints.h>


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
		((bool,showNeighbors,true,,"Whether to show neighbors in the 3d view (FIXME: should go to Gl1_SparcField, once it is created"))
		,/*ctor*/ locator=vtkPointLocator::New(); points=vtkPoints::New(); grid=vtkUnstructuredGrid::New(); grid->SetPoints(points); locator->SetDataSet(grid);
		,/*py*/
			.def("nodesAround",&SparcField::nodesAround,(py::arg("pt"),py::arg("radius")=-1,py::arg("count")=-1,py::arg("ptNode")=shared_ptr<Node>()),"Return array of nodes close to given point *pt*")
			.def("updateLocator",&SparcField::updateLocator,"Update the locator, should be done manually before the first step perhaps.")
	);
};
REGISTER_SERIALIZABLE(SparcField);

#define SPARC_INSPECT
#define SPARC_STATIC


struct SparcData: public NodeData{
	Matrix3r getD() const{ return .5*(gradV+gradV.transpose()); }
	Matrix3r getW() const{ return .5*(gradV-gradV.transpose()); }
	void catchCrap1(int nid, const shared_ptr<Node>&);
	void catchCrap2(int nid, const shared_ptr<Node>&);
	Real getDirVel(size_t i) const { return i<dirVels.size()?dirVels[i]:0.; }
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(SparcData,NodeData,"Nodal data needed for SPARC",

		((Matrix3r,T,Matrix3r::Zero(),,"Stress"))
		((Vector3r,v,Vector3r::Zero(),,"Velocity"))
		((Real,rho,0,,"Density"))
		((Real,e,0,,"Porosity"))
		((Real,color,Mathr::UnitRandom(),Attr::noGui,"Set node color, so that rendering is more readable"))
		// dirVels can be shorter, in which case zeros are assumed
		((vector<Vector3r>,dirs,,,"Directions in which velocity is prescribed."))
		((vector<Real>,dirVels,,,"Velocities along dir"))
		// storage within the step
		((vector<shared_ptr<Node> >,neighbors,,Attr::noGui,"List of neighbours, updated internally"))
		((Vector3r,accel,Vector3r::Zero(),,"Acceleration"))
		((Matrix3r,Tdot,Matrix3r::Zero(),,"Jaumann Stress rate")) 
		((MatrixXr,relPosInv,,,"Relative positions' pseudo-inverse"))
		((Matrix3r,gradV,Matrix3r::Zero(),,"gradient of velocity (only used as intermediate storage)"))
		// static equilibrium solver data
		#ifdef SPARC_STATIC
			((int,nid,-1,,"Node id (to located coordinates in solution matrix)"))
			((vector<int>,neighborsNids,,Attr::noGui,"List of neighbour nids, updated internally in findNeighbors")) 
		#endif
	#ifdef SPARC_INSPECT
		// debugging only
		((MatrixXr,relPos,,Attr::noSave,"Debug storage for relative positions"))
		((Matrix3r,Tcirc,Matrix3r::Zero(),Attr::noSave,"Debugging only -- stress rate"))
		((Vector3r,divT,Vector3r::Zero(),Attr::noSave,"Debugging only -- stress divergence"))
		((MatrixXr,relVels,,Attr::noSave,"Debugging only -- relative neighbor velocities"))
	#endif
		// ((int,locatorId,-1,Attr::hidden,"Position in the point locator array"))
		, /* ctor */
		, /*py*/
		.def("_getDataOnNode",&Node::pyGetData<SparcData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<SparcData>).staticmethod("_setDataOnNode")
		.add_property("D",&SparcData::getD).add_property("W",&SparcData::getW)
		;
	);
};
REGISTER_SERIALIZABLE(SparcData);

template<> struct NodeData::Index<SparcData>{enum{value=Node::ST_SPARC};};

struct ExplicitNodeIntegrator: public GlobalEngine, private SparcField::Engine{
	SparcField* mff; // lazy to type
	void findNeighbors(const shared_ptr<Node>& n) const;
	void updateNeighborsRelPos(const shared_ptr<Node>& n, bool useNext=false) const;
	Vector3r computeDivT(const shared_ptr<Node>& n, bool useNext=false) const;
	Matrix3r computeGradV(const shared_ptr<Node>& n) const;
	Matrix3r computeStressRate(const Matrix3r& T, const Matrix3r& D) const;
	void applyKinematicConstraints(const shared_ptr<Node>& n) const;
	Matrix6r C; // update at every step
	void postLoad(ExplicitNodeIntegrator&);
	virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(ExplicitNodeIntegrator,GlobalEngine,"Monolithic engine for explicit integration of motion of nodes in SparcField.",
		((Real,E,1e6,Attr::triggerPostLoad,"Young's modulus, for the linear elastic constitutive law"))
		((Real,nu,0,Attr::triggerPostLoad,"Poisson's ratio for the linear elastic constitutive law"))
		((Real,rSearch,-1,,"Radius for neighbor-search"))
		((int,neighborUpdate,1,,"Number of steps to periodically update neighbour information"))
		((Real,damping,0,,"Numerical damping, applied by-component on acceleration"))
		((Real,c,0,,"Viscous damping coefficient."))
		,/*ctor*/
		,/*py*/
		.def("stressRate",&ExplicitNodeIntegrator::computeStressRate) // for debugging
		.def_readonly("C",&ExplicitNodeIntegrator::C)
	);
};
REGISTER_SERIALIZABLE(ExplicitNodeIntegrator);

struct StaticEquilibriumSolver: public ExplicitNodeIntegrator{
	virtual void run();
	void renumberNodes() const;
	void copyVelocityToNodes(const VectorXr&) const;
	void applyConstraintReaction(const shared_ptr<Node>& n, Vector3r& divT) const ;
	VectorXr computeInitialVelocities() const;
	void integrateSolution() const;
	VectorXr trySolution(const VectorXr& vv);
	void dumpUnbalanced();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(StaticEquilibriumSolver,ExplicitNodeIntegrator,"Find global static equilibrium of a Sparc system.",
		((Real,supportStiffness,1e10,,"Stiffness of constrained DoFs"))
		#ifdef SPARC_INSPECT
			((VectorXr,solverDivT,,Attr::readonly,"Solution vector as provided by the solver"))
		#endif
		, /* ctor */
		, /* py */ .def("trySolution",&StaticEquilibriumSolver::trySolution)
	);

};
REGISTER_SERIALIZABLE(StaticEquilibriumSolver);

#endif
