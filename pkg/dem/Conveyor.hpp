#pragma once
#include<woo/pkg/dem/Factory.hpp>
#include<woo/pkg/dem/Clump.hpp>


struct ConveyorFactory: public ParticleFactory{
	DECLARE_LOGGER;
	bool acceptsField(Field* f){ return dynamic_cast<DemField*>(f); }
	void sortPacking();
	void postLoad(ConveyorFactory&,void*);
	void notifyDead();
	void nodeLeavesBarrier(const shared_ptr<Node>& p);
	void setAttachedParticlesColor(const shared_ptr<Node>& n, Real c);
	#ifdef WOO_OPENGL
		void render(const GLViewInfo&){
			if(isnan(glColor)) return;
			std::ostringstream oss; oss.precision(4); oss<<mass;
			if(maxMass>0){ oss<<"/"; oss.precision(4); oss<<maxMass; }
			if(!isnan(currRate)){ oss.precision(3); oss<<"\n("<<currRate<<")"; }
			GLUtils::GLDrawText(oss.str(),node->pos,CompUtils::mapColor(glColor));
		}
	#endif
	void run();
	vector<shared_ptr<Node>> pyBarrier() const { return vector<shared_ptr<Node>>(barrier.begin(),barrier.end()); }
	void pyClear(){ mass=0; num=0; genDiamMass.clear(); }
	bool hasClumps(){ return !clumps.empty(); }
	py::object pyDiamMass() const;
	Real pyMassOfDiam(Real min, Real max) const ;
	py::tuple pyPsd(bool mass, bool cumulative, bool normalize, Vector2r dRange, int num) const;
	WOO_CLASS_BASE_DOC_ATTRS_CTOR_PY(ConveyorFactory,ParticleFactory,"Factory producing infinite band of particles from packing periodic in the x-direction. (Clumps are not supported (yet?), only spheres).",
		((shared_ptr<Material>,material,,,"Material for new particles"))
		((Real,cellLen,,,"Length of the band cell, which is repeated periodically"))
		((vector<Real>,radii,,AttrTrait<Attr::triggerPostLoad>().noGui(),"Radii for the packing"))
		((vector<Vector3r>,centers,,AttrTrait<Attr::triggerPostLoad>().noGui(),"Centers of spheres/clumps in the packing"))
		((vector<shared_ptr<SphereClumpGeom>>,clumps,,AttrTrait<Attr::triggerPostLoad>().noGui(),"Clump geometry, corresponding to each :obj:`radii` and :obj:`centers`."))
		((int,nextIx,-1,AttrTrait<>().readonly(),"Index of last-generated particles in the packing"))
		((Real,lastX,0,AttrTrait<>().readonly(),"X-coordinate of last-generated particles in the packing"))
		((Real,vel,NaN,,"Velocity of the feed"))
		((Real,prescribedRate,NaN,AttrTrait<Attr::triggerPostLoad>(),"Prescribed average mass flow rate; if given, overwrites *vel*"))
		((Real,relLatVel,0.,,"Relative velocity components lateral to :obj:`vel` (local x-axis); both components are assigned with uniform probability from range `(-relLatVel*vel,+relLatVel*vel)`, at the moment the particle leaves the barrier layer."))
		((int,mask,1,,"Mask for new particles"))
		((Real,startLen,NaN,,"Band to be created at the very start; if NaN, only the usual starting amount is created (depending on feed velocity)"))
		((Real,barrierColor,.2,,"Color for barrier particles (NaN for random)"))
		((Real,color,NaN,,"Color for non-barrier particles (NaN for random)"))
		((Real,barrierLayer,-3.,,"Some length of the last part of new particles has all DoFs blocked, so that when new particles are created, there are no sudden contacts in the band; in the next step, DoFs in this layer are unblocked. If *barrierLayer* is negative, it is relative to the maximum radius in the given packing, and is automatically set to the correct value at the first run"))
		((list<shared_ptr<Node>>,barrier,,AttrTrait<>().readonly().noGui(),"Nodes which make up the barrier and will be unblocked once they leave barrierLayer."))
		((shared_ptr<Node>,node,make_shared<Node>(),AttrTrait<>().readonly(),"Position and orientation of the factory; local x-axis is the feed direction."))
		((Real,avgRate,NaN,AttrTrait<>().readonly(),"Average feed rate (computed from :ref:`material` density, packing and  and :ref:`vel`"))
		((int,kinEnergyIx,-1,AttrTrait<Attr::hidden|Attr::noSave>(),"Index for kinetic energy in scene.energy"))

		((vector<Vector2r>,genDiamMass,,AttrTrait<Attr::readonly>().noGui(),"List of generated diameters and masses (for making granulometry)"))
		((bool,save,true,,"Save generated particles so that PSD can be generated afterwards"))
		,/*ctor*/
		,/*py*/
			.def("barrier",&ConveyorFactory::pyBarrier)
			.def("clear",&ConveyorFactory::pyClear)
			.def("diamMass",&ConveyorFactory::pyDiamMass,"Return 2-tuple of same-length list of diameters and masses.")
			.def("massOfDiam",&ConveyorFactory::pyMassOfDiam,(py::arg("min")=0,py::arg("max")=Inf),"Return mass of particles of which diameters are between *min* and *max*.")
			.def("psd",&ConveyorFactory::pyPsd,(py::arg("mass")=true,py::arg("cumulative")=true,py::arg("normalize")=false,py::arg("dRange")=Vector2r(NaN,NaN),py::arg("num")=80),"Return PSD for particles generated.")
	);
};
WOO_REGISTER_OBJECT(ConveyorFactory);

