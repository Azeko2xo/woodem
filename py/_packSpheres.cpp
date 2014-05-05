// 2009 © Václav Šmilauer <eudoxos@arcig.cz>

#include<woo/lib/sphere-pack/SpherePack.hpp>
#include<woo/lib/pyutil/doc_opts.hpp>
#include<woo/lib/base/Math.hpp>
#include<woo/core/Master.hpp>
#include<woo/pkg/dem/Funcs.hpp>

vector<Particle::id_t> SpherePack_toSimulation_fast(const shared_ptr<SpherePack>& self, const shared_ptr<Scene>& scene, const shared_ptr<Material>& mat, int mask=0, Real color=NaN){
	DemField* dem=(DemFuncs::getDemField(scene.get()).get());
	return DemFuncs::SpherePack_toSimulation_fast(self,scene.get(),dem,mat,mask,color);
}

WOO_PYTHON_MODULE(_packSpheres);
BOOST_PYTHON_MODULE(_packSpheres){
	py::scope().attr("__doc__")="Creation, manipulation, IO for generic sphere packings.";
	WOO_SET_DOCSTRING_OPTS;
	py::class_<SpherePack,shared_ptr<SpherePack>,boost::noncopyable>("SpherePack","Set of spheres represented as centers and radii. This class is returned by :obj:`woo.pack.randomDensePack`, :obj:`woo.pack.randomPeriPack` and others. The object supports iteration over spheres, as in \n\n\t>>> sp=SpherePack()\n\t>>> for center,radius in sp: print center,radius\n\n\t>>> for sphere in sp: print sphere[0],sphere[1]   ## same, but without unpacking the tuple automatically\n\n\t>>> for i in range(0,len(sp)): print sp[i][0], sp[i][1]   ## same, but accessing spheres by index\n\n\n.. admonition:: Special constructors\n\n\tConstruct from list of ``[(c1,r1),(c2,r2),…]``. To convert two same-length lists of ``centers`` and ``radii``, construct with ``zip(centers,radii)``.\n",py::init<py::optional<py::list> >(py::args("list"),"Empty constructor, optionally taking list [ ((cx,cy,cz),r), … ] for initial data." ))
		.def("toSimulation_fast",SpherePack_toSimulation_fast,(py::arg("scene"),py::arg("mat"),py::arg("mask")=1,py::arg("color")=NaN),"Create spheres from this pack in the simulation; unlike :obj:`SpherePack.toSimulation`, this method is implemented in c++ (hence rather fast) but lacks some flexibility")
		.def("add",&SpherePack::add,(py::arg("center"),py::arg("radius"),py::arg("clumpId")=-1),"Add single sphere to packing, given center as 3-tuple and radius")
		.def("toList",&SpherePack::toList,"Return packing data as [(c,r),(c,r),...].")
		.def("toCcRr",&SpherePack::toCcRr,"Return packing data as ([c,c,...],[r,r,...]). Raises exception if there are clumps.")
		.def("fromList",&SpherePack::fromList,"Make packing from given list, same format as for constructor. Discards current data.")
		.def("fromList",&SpherePack::fromLists,(py::arg("centers"),py::arg("radii")),"Make packing from given list, same format as for constructor. Discards current data.")
		.def("load",&SpherePack::fromFile,(py::arg("fileName")),"Load packing from external text file (current data will be discarded).").def("loadTxt",&SpherePack::fromFile,"Identical to :obj:`load`, with newer name for ShapePack compatibility.")
		.def("reset",&SpherePack::reset,"Re-inialize this object (clear all spheres and reset periodic cell size.")
		.def("save",&SpherePack::toFile,(py::arg("fileName")),"Save packing to external text file (will be overwritten).").def("saveTxt",&SpherePack::toFile,"Identical to :obj:`save:`, with newer name for ShapePack compatibility.")
		.def("makeCloud",&SpherePack::makeCloud,(py::arg("minCorner")=Vector3r(Vector3r::Zero()),py::arg("maxCorner")=Vector3r(Vector3r::Zero()),py::arg("rMean")=-1,py::arg("rRelFuzz")=0,py::arg("num")=-1,py::arg("periodic")=false,py::arg("porosity")=0.5,py::arg("psdSizes")=vector<Real>(),py::arg("psdCumm")=vector<Real>(),py::arg("distributeMass")=false,py::arg("seed")=0,py::arg("hSize")=Matrix3r(Matrix3r::Zero())),"Create random loose packing enclosed in a parallelepiped."
		"\nSphere radius distribution can be specified using one of the following ways:\n\n#. *rMean*, *rRelFuzz* and *num* gives uniform radius distribution in *rMean* (1 ± *rRelFuzz* ). Less than *num* spheres can be generated if it is too high.\n#. *rRelFuzz*, *num* and (optional) *porosity*, which estimates mean radius so that *porosity* is attained at the end.  *rMean* must be less than 0 (default). *porosity* is only an initial guess for the generation algorithm, which will retry with higher porosity until the prescibed *num* is obtained.\n#. *psdSizes* and *psdCumm*, two arrays specifying points of the `particle size distribution <http://en.wikipedia.org/wiki/Particle_size_distribution>`__ function. As many spheres as possible are generated.\n#. *psdSizes*, *psdCumm*, *num*, and (optional) *porosity*, like above but if *num* is not obtained, *psdSizes* will be scaled down uniformly, until *num* is obtained (see :obj:`appliedPsdScaling <woo._packSpheres.SpherePack.appliedPsdScaling>`).\n\nBy default (with ``distributeMass==False``), the distribution is applied to particle radii. The usual sense of \"particle size distribution\" is the distribution of *mass fraction* (rather than particle count); this can be achieved with ``distributeMass=True``."
		"\n\nIf *num* is defined, then sizes generation is deterministic, giving the best fit of target distribution. It enables spheres placement in descending size order, thus giving lower porosity than the random generation."
		"\n\n:param Vector3 minCorner: lower corner of an axis-aligned box\n:param Vector3 maxCorner: upper corner of an axis-aligned box\n:param Matrix3 hSize: base vectors of a generalized box (arbitrary parallelepiped, typically :obj:`woo.core.Cell.hSize`), superseeds minCorner and maxCorner if defined. For periodic boundaries only.\n:param float rMean: mean radius or spheres\n:param float rRelFuzz: dispersion of radius relative to rMean\n:param int num: number of spheres to be generated. If negavite (default), generate as many as possible with stochastic sizes, ending after a fixed number of tries to place the sphere in space, else generate exactly *num* spheres with deterministic size distribution.\n:param bool periodic: whether the packing to be generated should be periodic\n:param float porosity: initial guess for the iterative generation procedure (if *num*>1). The algorithm will be retrying until the number of generated spheres is *num*. The first iteration tries with the provided porosity, but next iterations increase it if necessary (hence an initialy high porosity can speed-up the algorithm). If *psdSizes* is not defined, *rRelFuzz* ($z$) and *num* ($N$) are used so that the porosity given ($\\rho$) is approximately achieved at the end of generation, $r_m=\\sqrt[3]{\\frac{V(1-\\rho)}{\\frac{4}{3}\\pi(1+z^2)N}}$. The default is $\\rho$=0.5. The optimal value depends on *rRelFuzz* or  *psdSizes*.\n:param psdSizes: sieve sizes (particle diameters) when particle size distribution (PSD) is specified\n:param psdCumm: cummulative fractions of particle sizes given by *psdSizes*; must be the same length as *psdSizes* and should be non-decreasing\n:param bool distributeMass: if ``True``, given distribution will be used to distribute sphere's mass rather than radius of them.\n:param seed: number used to initialize the random number generator.\n:returns: number of created spheres, which can be lower than *num* depending on the method used.\n")
		// new psd
		.def("psd",&SpherePack::psd,(py::arg("bins")=50,py::arg("mass")=true),"Return `particle size distribution <http://en.wikipedia.org/wiki/Particle_size_distribution>`__ of the packing.\n:param int bins: number of bins between minimum and maximum diameter\n:param mass: Compute relative mass rather than relative particle count for each bin. Corresponds to :obj:`distributeMass parameter for makeCloud <woo.pack._packSpheres.makeCloud>`.\n:returns: tuple of ``(cumm,edges)``, where ``cumm`` are cummulative fractions for respective diameters  and ``edges`` are those diameter values. Dimension of both arrays is equal to ``bins+1``.")
		//
		.def("aabb",&SpherePack::aabb,"Get axis-aligned bounding box coordinates.")
		.def("dim",&SpherePack::dim,"Return dimensions of the packing in terms of aabb().")
		.def("center",&SpherePack::midPt,"Return coordinates of the bounding box center.")
		.def_readwrite("cellSize",&SpherePack::cellSize,"Size of periodic cell; is Vector3(0,0,0) if not periodic. (Change this property only if you know what you're doing).")
		.def_readwrite("userData",&SpherePack::userData,"Arbitrary string (not cotaining newlines) which will be saved and loaded with this object") 
		.def("cellFill",&SpherePack::cellFill,"Repeat the packing (if periodic) so that the results has dim() >= given size. The packing retains periodicity, but changes cellSize. Raises exception for non-periodic packing.")
		.def("canonicalize",&SpherePack::canonicalize,"Move all sphere's centers inside *cellSize*; only works for periodic packings without clumps.")
		.def("maxRelOverlap",&SpherePack::maxRelOverlap,"Return maximum relative overlap of particles.")
		.def("makeOverlapFree",&SpherePack::makeOverlapFree,"Scale by 1+maxRelOverlap(), without changing radii.")
		.def("cellRepeat",&SpherePack::cellRepeat,"Repeat the packing given number of times in each dimension. Periodicity is retained, cellSize changes. Raises exception for non-periodic packing.")
		.def("sphereVol",&SpherePack::sphereVol,"Summary volume of spheres, disregarding any overlaps (:math:`\\frac{4}{3}\\pi\\sum r_i^3`).")
		.def("solidVolume",&SpherePack::sphereVol,"Alias for :obj:`sphereVol`, for forward compatibility with :obj:`woo.dem.ShapePack`.")
		.def("relDensity",&SpherePack::relDensity,"Relative packing density, measured as sum of spheres' volumes / aabb volume.\n(Sphere overlaps are ignored.)")
		.def("translate",&SpherePack::translate,"Translate all spheres by given vector.")
		.def("rotate",&SpherePack::rotate,(py::arg("axis"),py::arg("angle")),"Rotate all spheres around packing center (in terms of aabb()), given axis and angle of the rotation.")
		.def("scale",&SpherePack::scale,(py::arg("scale"),py::arg("keepRadius")=false),"Scale the packing around its center (in terms of aabb()) by given factor (may be negative). If periodic, scale around origin. If *keepRadius* is True, radii are not scaled, only coordinates.")
		.def("hasClumps",&SpherePack::hasClumps,"Whether this object contains clumps.")
		.def("getClumps",&SpherePack::getClumps,"Return lists of sphere ids sorted by clumps they belong to. The return value is (standalones,[clump1,clump2,…]), where each item is list of id's of spheres.")
		.def("__len__",&SpherePack::len,"Get number of spheres in the packing")
		.def("__getitem__",&SpherePack::getitem,"Get entry at given index, as tuple of center and radius.")
		.def("__iter__",&SpherePack::getIterator,"Return iterator over spheres.")
		.def_readonly("appliedPsdScaling",&SpherePack::appliedPsdScaling,"A factor between 0 and 1, uniformly applied on all sizes of the PSD.")
		;
	py::class_<SpherePack::_iterator>("SpherePackIterator",py::init<SpherePack::_iterator&>())
		.def("__iter__",&SpherePack::_iterator::iter)
		.def("next",&SpherePack::_iterator::next)
	;
}

