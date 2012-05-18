
#include<yade/pkg/voro/VoroField.hpp>
#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/Sphere.hpp>
#include<iostream>

YADE_PLUGIN(voro,(VoroField));

CREATE_LOGGER(VoroField);

void VoroField::preSave(VoroField&){
	LOG_WARN("VoroField: serialization not yet implemented, your data will be lost. Call VoroField.updateFromDem(...) to refresh.");
}
void VoroField::postLoad(VoroField&){};

void VoroField::updateFromDem(){
	// get DEM field we take data from
	Scene* scene=Omega::instance().getScene().get();
	DemField* dem=NULL;
	FOREACH(shared_ptr<Field>& f, scene->fields){
		if(dynamic_cast<DemField*>(f.get())){
			if(dem) throw std::runtime_error("Mutliple DEM fields; fix the interface so that it can be passed as argument.");
			else dem=static_cast<DemField*>(f.get());
		}
	}

	if(!scene->isPeriodic) throw std::invalid_argument("VoroField.updateFromDem: simulation must have periodic boundaries");
	// check that the transformation is upper-triangular
	scene->cell->checkTrsfUpperTriangular(); // throw if not upper-triangular
	scene->cell->trsfUpperTriangular=true; // require in the future as well, checked automatically

	// get average radius first, to know how finely to subdivide the cell
	Real rSum=0; int N=0;
	FOREACH(const shared_ptr<Particle>& par, *dem->particles){
		if(!dynamic_cast<Sphere*>(par->shape.get())) continue;
		rSum+=par->shape->cast<Sphere>().radius; N++;
	}
	Real rAvg=rSum/N;

	for(int i=0; i<3; i++) if(subDiv[i]<=0) subDiv[i]=max(2,(int)(.5+scene->cell->getSize()[i]/(relSubcellSize*2*rAvg)));

	if(conp) delete conp;
	if(vnet) delete vnet;
	/* initialize the container now; the geometry is given by base matrix (cols are base vectors)

	   x  xy xz
	   0   y yz
	   0   0  z

	 */
	const Matrix3r& T(scene->cell->trsf);
	conp=new voropp::container_periodic_poly(/*xb*/T(0,0),/*xyb*/T(0,1),/*yb*/T(1,1),/*xzb*/T(0,2),/*yzb*/T(1,2),/*zb*/T(2,2),/*xn*/subDiv[0],/*yn*/subDiv[1],/*zn*/subDiv[2],initMem);

	// add particles
	FOREACH(const shared_ptr<Particle>& par, *dem->particles){
		if(!dynamic_cast<Sphere*>(par->shape.get())) continue;
		const Vector3r& pos(par->shape->nodes[0]->pos);
		conp->put(par->id,pos[0],pos[1],pos[2],par->shape->cast<Sphere>().radius);
	}

	// compute the network
	vnet=new voropp::voronoi_network(*conp);
	bool isRectangular=(T(0,1)==0 && T(0,2)==0 && T(1,0)==0 && T(1,2)==0 && T(2,0)==0 && T(2,1)==0);
	if(isRectangular) conp->compute_network_rectangular(*vnet);
	else conp->compute_network(*vnet);
	cerr<<"Voronoi tesselation done."<<endl;
}

void VoroField::cellsToPov(const std::string& out){

	if(!conp || !vnet) throw std::runtime_error("VoroField: Particles not inserted or tirnagulation not computed yet.");
	Scene* scene=Omega::instance().getScene().get();
	const Vector3r& size=scene->cell->getSize();
	conp->draw_cells_pov(out.c_str(),0,size[0],0,size[1],0,size[2]);
}

void VoroField::particlesToPov(const std::string& out){
	if(!conp || !vnet) throw std::runtime_error("VoroField: Particles not inserted or tirnagulation not computed yet.");
	conp->draw_particles_pov(out.c_str());
}
