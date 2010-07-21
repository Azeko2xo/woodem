#pragma once
#include<yade/pkg-common/PeriodicEngines.hpp>


class VTKRecorder: public PeriodicEngine {
	public:
		enum {REC_SPHERES=0,REC_FACETS,REC_COLORS,REC_CPM,REC_INTR,REC_VELOCITY,REC_ID,REC_CLUMPID,REC_SENTINEL,REC_MATERIALID,REC_STRESS,REC_MASK,REC_RPM};
		virtual void action();
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(VTKRecorder,PeriodicEngine,"Engine recording snapshots of simulation into series of *.vtu files, readable by VTK-based postprocessing programs such as Paraview. Both bodies (spheres and facets) and interactions can be recorded, with various vector/scalar quantities that are defined on them.\n\n:yref:`PeriodicEngine.initRun` is initialized to ``True`` automatically.",
		((bool,compress,false,"Compress output XML files [experimental]."))
		((bool,skipFacetIntr,true,"Skip interactions with facets, when saving interactions"))
		((bool,skipNondynamic,false,"Skip non-dynamic spheres (but not facets)."))
		((bool,multiblock,false,"Use multi-block (``.vtm``) files to store data, rather than separate ``.vtu`` files."))
		((string,fileName,"","Base file name; it will be appended with {spheres,intrs,facets}-243100.vtu (unless *multiblock* is ``True``) depending on active recorders and step number (243100 in this case). It can contain slashes, but the directory must exist already."))
		((vector<string>,recorders,,"List of active recorders (as strings). Accepted recorders are:\n\n``all``\n\tSaves all possible parameters, except of specific. Default value.\n``spheres``\n\tSaves positions and radii (``radii``) of :yref:`spherical<Sphere>` particles.\n``id``\n\tSaves id's (field ``id``) of spheres; active only if ``spheres`` is active.\n``clumpId``\n\tSaves id's of clumps to which each sphere belongs (field ``clumpId``); active only if ``spheres`` is active.\n``colors``\n\tSaves colors of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>` (field ``color``); only active if ``spheres`` or ``facets`` are activated.\n``mask``\n\tSaves groupMasks of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>` (field ``mask``); only active if ``spheres`` or ``facets`` are activated.\n``materialId``\n\tSaves materialID of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>`; only active if ``spheres`` or ``facets`` are activated.\n``velocity``\n\tSaves linear and angular velocities of spherical particles as Vector3 and length(fields ``linVelVec``, ``linVelLen`` and ``angVelVec``, ``angVelLen`` respectively``); only effective with ``spheres``.\n``facets``\n\tSave :yref:`facets<Facet>` positions (vertices).\n``stress``\n\tSaves stresses of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>`  as Vector3 and length; only active if ``spheres`` or ``facets`` are activated.\n``cpm``\n\tSaves data pertaining to the :yref:`concrete model<Law2_Dem3DofGeom_CpmPhys_Cpm>`: ``cpmDamage`` (normalized residual strength averaged on particle), ``cpmSigma`` (stress on particle, normal components), ``cpmTau`` (shear components of stress on particle), ``cpmSigmaM`` (mean stress around particle); ``intr`` is activated automatically by ``cpm``.\n``intr``\n\tWhen ``cpm`` is used, it saves magnitude of normal (``forceN``) and shear (``absForceT``) forces.\n\n\tWithout ``cpm``, saves [TODO]\n``rpm``\n\tSaves data pertaining to the :yref:`rock particle model<Law2_Dem3DofGeom_RockPMPhys_Rpm>`: ``rpmSpecNum`` shows different pieces of separated stones, only ids.\n ``rpmSpecMass`` shows masses of separated stones.\n\n"))
		((int,mask,0,"If mask defined, only bodies with corresponding groupMask will be exported. If 0 - all bodies will be exported.")),
		/*ctor*/
		initRun=true;
		if (recorders.empty()) {recorders.push_back("all");}	//Default value
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(VTKRecorder);
