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
		,/*ctor*/ locator=vtkPointLocator::New(); points=vtkPoints::New(); grid=vtkUnstructuredGrid::New(); grid->SetPoints(points); locator->SetDataSet(grid);
		,/*py*/
			.def("nodesAround",&SparcField::nodesAround,(py::arg("pt"),py::arg("radius")=-1,py::arg("count")=-1,py::arg("ptNode")=shared_ptr<Node>()),"Return array of nodes close to given point *pt*")
			.def("updateLocator",&SparcField::updateLocator,"Update the locator, should be done manually before the first step perhaps.")
	);
};
REGISTER_SERIALIZABLE(SparcField);




struct SparcData: public NodeData{
	MatrixXr relPosInv; // intermediate storage
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(SparcData,NodeData,"Nodal data needed for SPARC",
		((Matrix3r,T,Matrix3r::Zero(),,"Stress"))
		((Vector3r,v,Vector3r::Zero(),,"Velocity"))
		((Real,rho,0,,"Density"))
		((Real,e,0,,"Porosity"))
		((Matrix3r,gradV,Matrix3r::Zero(),,"gradient of velocity (only used as intermediate storage)"))
		((vector<Vector3r>,dirs,,,"Directions in which velocity is prescribed."))
		((vector<Real>,dirVels,,,"Velocities along dir"))
		((vector<shared_ptr<Node> >,neighbors,,Attr::noGui,"List of neighbours, updated internally"))
		((Real,color,Mathr::UnitRandom(),Attr::noGui,"Set node color, so that rendering is more readable"))
		// ((int,locatorId,-1,Attr::hidden,"Position in the point locator array"))
		, /* ctor */
		, /*py*/
		.def("_getDataOnNode",&Node::pyGetData<SparcData>).staticmethod("_getDataOnNode").def("_setDataOnNode",&Node::pySetData<SparcData>).staticmethod("_setDataOnNode");
	);
};
REGISTER_SERIALIZABLE(SparcData);

template<> struct NodeData::Index<SparcData>{enum{value=Node::ST_SPARC};};

struct ExplicitNodeIntegrator: public GlobalEngine, private SparcField::Engine{
	SparcField* mff; // lazy to type
	void updateNeighbours(const shared_ptr<Node>& n);
	Vector3r computeDivT(const shared_ptr<Node>& n);
	Matrix3r computeGradV(const shared_ptr<Node>& n);
	Matrix3r stressRate(const Matrix3r& T, const Matrix3r& D);
	Matrix6r C; // update at every step
	virtual void run();
	YADE_CLASS_BASE_DOC_ATTRS(ExplicitNodeIntegrator,GlobalEngine,"Monolithic engine for explicit integration of motion of nodes in SparcField.",
		((Real,E,1e6,,"Young's modulus, for the linear elastic constitutive law"))
		((Real,nu,0,,"Poisson's ratio for the linear elastic constitutive law"))
		((Real,rSearch,-1,,"Radius for neighbor-search"))
	);
};
REGISTER_SERIALIZABLE(ExplicitNodeIntegrator);

#endif
