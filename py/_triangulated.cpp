#include<woo/core/Master.hpp>

#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/Funcs.hpp>
#include<woo/lib/base/CompUtils.hpp>

#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/dem/Ellipsoid.hpp>
#include<woo/pkg/dem/VtkExport.hpp>

#ifdef WOO_LOG4CXX
	static log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("woo.triangulated");
#endif

int spheroidsToSTL(const string& out, const shared_ptr<DemField>& dem, Real tol, int mask, bool clipCell){
	if(tol==0 || isnan(tol)) throw std::runtime_error("tol must be non-zero.");
	// first traversal to find reference radius
	auto particleOk=[&](const shared_ptr<Particle>&p){ return (mask==0 || (p->mask & mask)) && (p->shape->isA<Sphere>() || p->shape->isA<Ellipsoid>() || p->shape->isA<Capsule>()); };
	int numTri=0;

	if(tol<0){
		LOG_DEBUG("tolerance is negative, taken as relative to minimum radius.");
		Real minRad=Inf;
		for(const auto& p: *dem->particles){
			if(particleOk(p)) minRad=min(minRad,p->shape->equivRadius());
		}
		if(isinf(minRad) || isnan(minRad)) throw std::runtime_error("Minimum radius not found (relative tolerance specified); no matching particles?");
		tol=-minRad*tol;
		LOG_DEBUG("Minimum radius "<<minRad<<".");
	}
	LOG_DEBUG("Triangulation tolerance is "<<tol);
	
	std::ofstream stl(out,std::ofstream::binary); // binary better, anyway
	if(!stl.good()) throw std::runtime_error("Failed to open output file "+out+" for writing.");
	stl<<"solid woo_export\n";

	Scene* scene=dem->scene;
	if(!scene) throw std::logic_error("DEM field has not associated scene?");

	// periodicity, cache that for later use
	AlignedBox3r cell;

	for(const auto& p: *dem->particles){
		if(!particleOk(p)) continue;
		const auto sphere=dynamic_cast<Sphere*>(p->shape.get());
		const auto ellipsoid=dynamic_cast<Ellipsoid*>(p->shape.get());
		const auto capsule=dynamic_cast<Capsule*>(p->shape.get());
		vector<Vector3r> pts;
		vector<Vector3i> tri;
		if(sphere || ellipsoid){
			Real r=sphere?sphere->radius:ellipsoid->semiAxes.minCoeff();
			// 1 is for icosahedron
			int tess=ceil(M_PI/(5*acos(1-tol/r)));
			LOG_DEBUG("Tesselation level for #"<<p->id<<": "<<tess);
			tess=max(tess,0);
			auto uSphTri(CompUtils::unitSphereTri20(/*0 for icosahedron*/max(tess-1,0)));
			const auto& uPts=std::get<0>(uSphTri); // unit sphere point coords
			pts.resize(uPts.size());
			const auto& node=(p->shape->nodes[0]);
			Vector3r scale=(sphere?sphere->radius*Vector3r::Ones():ellipsoid->semiAxes);
			for(size_t i=0; i<uPts.size(); i++){
				pts[i]=node->loc2glob(uPts[i].cwiseProduct(scale));
			}
			tri=std::get<1>(uSphTri); // this makes a copy, but we need out own for capsules
		}
		if(capsule){
			#ifdef WOO_VTK
				int subdiv=min(4.,ceil(M_PI/(acos(1-tol/capsule->radius))));
				std::tie(pts,tri)=VtkExport::triangulateCapsule(static_pointer_cast<Capsule>(p->shape),subdiv);
			#else
				throw std::runtime_error("Triangulation of capsules is (for internal and entirely fixable reasons) only available when compiled with the 'vtk' features.");
			#endif
		}
		auto writeFacets=[&](const Vector3r& off, std::function<bool(const Vector3r&)> clip=std::function<bool(const Vector3r&)>([](const Vector3r&){ return false; })) {
			for(const Vector3i& t: tri){
				Vector3r pp[]={pts[t[0]]+off,pts[t[1]]+off,pts[t[2]]+off};
				// skip triangles which are entirely out of the clipping box (e.g. periodic cell)
				if(clip(pp[0]) && clip(pp[1]) && clip(pp[2])) continue;
				numTri++;
				Vector3r n=(pp[1]-pp[0]).cross(pp[2]-pp[1]).normalized();
				stl<<"  facet normal "<<n.x()<<" "<<n.y()<<" "<<n.z()<<"\n";
				stl<<"    outer loop\n";
				for(auto p: {pp[0],pp[1],pp[2]}){
					stl<<"      vertex "<<p[0]<<" "<<p[1]<<" "<<p[2]<<"\n";
				}
				stl<<"    endloop\n";
				stl<<"  endfacet\n";
			};
		};
		if(!scene->isPeriodic) writeFacets(/*off*/Vector3r::Zero());
		else{
			// make sure we have aabb, in skewed coords and such
			if(!p->shape->bound){
				// this is a bit ugly, but should do the trick; otherwise we would recompute all that ourselves here
				if(sphere) Bo1_Sphere_Aabb().go(p->shape);
				else if(ellipsoid) Bo1_Ellipsoid_Aabb().go(p->shape);
				else if(capsule) Bo1_Capsule_Aabb().go(p->shape);
			}
			assert(p->shape->bound);
			const AlignedBox3r& box(p->shape->bound->box);
			AlignedBox3r cell(Vector3r::Zero(),scene->cell->getSize()); // possibly in skewed coords
			// central offset
			Vector3i off0;
			scene->cell->canonicalizePt(p->shape->nodes[0]->pos,off0); // computes off0
			Vector3i off; // offset from the original cell
			//cerr<<"#"<<p->id<<" at "<<p->shape->nodes[0]->pos.transpose()<<", off0="<<off0<<endl;
			for(off[0]=off0[0]-1; off[0]<=off0[0]+1; off[0]++) for(off[1]=off0[1]-1; off[1]<=off0[1]+1; off[1]++) for(off[2]=off0[2]-1; off[2]<=off0[2]+1; off[2]++){
				Vector3r dx=scene->cell->intrShiftPos(off);
				//cerr<<"  off="<<off.transpose()<<", dx="<<dx.transpose()<<endl;
				AlignedBox3r boxOff(box); boxOff.translate(dx);
				//cerr<<"  boxOff="<<boxOff.min()<<";"<<boxOff.max()<<" | cell="<<cell.min()<<";"<<cell.max()<<endl;
				if(boxOff.intersection(cell).isEmpty()) continue;
				//cerr<<"  WRITE"<<endl;
				if(!clipCell) writeFacets(dx);
				else writeFacets(dx,[&](const Vector3r& p){ return !scene->cell->isCanonical(p); });
			}
		}
	}
	stl<<"endsolid woo_export\n";
	stl.close();
	return numTri;
}

WOO_PYTHON_MODULE(_triangulated);
BOOST_PYTHON_MODULE(_triangulated){
	WOO_SET_DOCSTRING_OPTS;

	py::def("spheroidsToSTL",spheroidsToSTL,(py::arg("stl"),py::arg("dem"),py::arg("tol"),py::arg("mask")=0,py::arg("cellClip")=false),"Export spheroids (:obj:`spheres <woo.dem.Sphere>`, :obj:`capsules <woo.dem.Capsule>`, :obj:`ellipsoids <woo.dem.Ellipsoid>`) to STL file. *tol* is the maximum distance between triangulation and smooth surface; if negative, it is relative to the smallest equivalent radius of particles for export. *mask* (if non-zero) only selects particles with matching :obj:`woo.dem.Particle.mask`. The exported STL ist ASCII.\n\nSpheres and ellipsoids are exported as tesselated icosahedra, with tesselation level determined from *tol*. The maximum error is :math:`e=r\\left(1-\\cos \\frac{2\\pi}{5}\\frac{1}{2}\\frac{1}{n}\\right)` for given tesselation level :math:`n` (1 for icosahedron, each level quadruples the number of triangles), with :math:`r` being the sphere's :obj:`radius <woo.dem.Sphere.radius>` (or ellipsoid's smallest :obj:`semiAxis <woo.dem.Ellipsoid.semiAxes>`); it follows that :math:`n=\\frac{\\pi}{5\\arccos\\left(1-\\frac{e}{r}\\right)}`, where :math:`n` will be rounded up.\n\nCapsules are triangulated in polar coordinates (slices, stacks). The error for regular :math:`n`-gon is :math:`e=r\\left(1-\\cos\\frac{2\\pi}{2n}\\right)` and it follows that :math:`n=\\frac{\\pi}{\\arccos\\left(1-\\frac{e}{r}\\right)}`; the minimum is restricted to be 4, to avoid degenerate shapes.\n\nThe number of facets written to the STL file is returned.\n\nWith periodic boundaries, *clipCell* will cause all triangles entirely outside of the periodic cell to be discarded.");
};
