#include<woo/lib/smoothing/WeightedAverage2d.hpp>
#include<woo/core/Master.hpp>

/* Tell whether point is inside polygon
 *
 * See _utils.cpp: pointInsidePolygon for docs and license.
 */
bool pyGaussAverage::pointInsidePolygon(const Vector2r& pt, const vector<Vector2r>& vertices){
	int i /*current node*/, j/*previous node*/; bool inside=false; int rows=(int)vertices.size();
	const Real& testx=pt[0],testy=pt[1];
	for(i=0,j=rows-1; i<rows; j=i++){
		const Real& vx_i=vertices[i][0], vy_i=vertices[i][1], vx_j=vertices[j][0], vy_j=vertices[j][1];
		if (((vy_i>testy)!=(vy_j>testy)) && (testx < (vx_j-vx_i) * (testy-vy_i) / (vy_j-vy_i) + vx_i) ) inside=!inside;
	}
	return inside;
}

WOO_PYTHON_MODULE(WeightedAverage2d);
BOOST_PYTHON_MODULE(WeightedAverage2d)
{
	py::scope().attr("__doc__")="Smoothing (2d gauss-weighted average) for postprocessing scalars in 2d.";
	py::class_<pyGaussAverage>("GaussAverage",py::init<py::tuple,py::tuple,py::tuple,Real,py::optional<Real> >(py::args("min","max","nCells","stDev","relThreshold"),"Create empty container for data, which can be added using add and later retrieved using avg."))
		.def("add",&pyGaussAverage::addPt)
		.def("avg",&pyGaussAverage::avg)
		.def("avgPerUnitArea",&pyGaussAverage::avgPerUnitArea)
		.def("cellNum",&pyGaussAverage::cellNum)
		.def("cellSum",&pyGaussAverage::cellSum)
		.def("cellAvg",&pyGaussAverage::cellAvg)
		.add_property("stDev",&pyGaussAverage::stDev_get,&pyGaussAverage::stDev_set)
		.add_property("relThreshold",&pyGaussAverage::relThreshold_get,&pyGaussAverage::relThreshold_set)
		.add_property("clips",&pyGaussAverage::clips_get,&pyGaussAverage::clips_set)
		.add_property("data",&pyGaussAverage::data_get)
		.add_property("aabb",&pyGaussAverage::aabb_get)
		.add_property("nCells",&pyGaussAverage::nCells_get)
		.add_property("cellArea",&pyGaussAverage::cellArea)
		.add_property("cellDim",&pyGaussAverage::cellDim)
	;
};

