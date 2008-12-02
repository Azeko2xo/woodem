/*************************************************************************
*  Copyright (C) 2008 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include<yade/pkg-dem/CohesiveFrictionalContactLaw.hpp>
#include<yade/pkg-dem/CohesiveFrictionalRelationships.hpp>
#include<yade/pkg-dem/CohesiveFrictionalBodyParameters.hpp>
#include<yade/pkg-dem/SDECLinkPhysics.hpp>
#include<yade/pkg-dem/GlobalStiffnessCounter.hpp>
#include<yade/pkg-dem/GlobalStiffnessTimeStepper.hpp>
#include<yade/pkg-dem/PositionOrientationRecorder.hpp>

#include<yade/pkg-dem/AveragePositionRecorder.hpp>
#include<yade/pkg-dem/ForceRecorder.hpp>
#include<yade/pkg-dem/VelocityRecorder.hpp>
#include<yade/pkg-dem/TriaxialStressController.hpp>
#include<yade/pkg-dem/TriaxialCompressionEngine.hpp>
#include<yade/pkg-dem/TriaxialStateRecorder.hpp>

#include<yade/pkg-common/Box.hpp>
#include<yade/pkg-common/AABB.hpp>
#include<yade/pkg-common/Sphere.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/pkg-common/SAPCollider.hpp>
#include<yade/pkg-common/DistantPersistentSAPCollider.hpp>
#include<yade/lib-serialization/IOFormatManager.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/pkg-common/BoundingVolumeMetaEngine.hpp>
#include<yade/pkg-common/MetaInteractingGeometry2AABB.hpp>
#include<yade/pkg-common/MetaInteractingGeometry.hpp>

#include<yade/pkg-common/GravityEngines.hpp>
#include<yade/pkg-dem/HydraulicForceEngine.hpp>
#include<yade/pkg-dem/InteractingSphere2InteractingSphere4SpheresContactGeometry.hpp>
#include<yade/pkg-dem/InteractingBox2InteractingSphere4SpheresContactGeometry.hpp>
#include<yade/pkg-common/PhysicalActionApplier.hpp>
#include<yade/pkg-common/PhysicalActionDamper.hpp>
#include<yade/pkg-common/CundallNonViscousDamping.hpp>
#include<yade/pkg-common/CundallNonViscousDamping.hpp>

#include<yade/pkg-common/InteractionGeometryMetaEngine.hpp>
#include<yade/pkg-common/InteractionPhysicsMetaEngine.hpp>
#include<yade/core/Body.hpp>
#include<yade/pkg-common/InteractingBox.hpp>
#include<yade/pkg-common/InteractingSphere.hpp>

#include<yade/pkg-common/PhysicalActionContainerReseter.hpp>
#include<yade/pkg-common/PhysicalActionContainerInitializer.hpp>

#include<yade/pkg-common/PhysicalParametersMetaEngine.hpp>

#include<yade/pkg-common/BodyRedirectionVector.hpp>
#include<yade/pkg-common/InteractionVecSet.hpp>
#include<yade/pkg-common/InteractionHashMap.hpp>
#include<yade/pkg-common/PhysicalActionVectorVector.hpp>

#include<yade/extra/Shop.hpp>

#include<boost/filesystem/convenience.hpp>
#include<boost/lexical_cast.hpp>
#include<boost/numeric/conversion/bounds.hpp>
#include<boost/limits.hpp>


#include"SnowVoxelsLoader.hpp"
#include<set>
#include<yade/pkg-snow/Ef2_BssSnowGrain_BssSnowGrain_makeSpheresContactGeometry.hpp>
#include<yade/pkg-snow/Ef2_InteractingBox_BssSnowGrain_makeSpheresContactGeometry.hpp>

SnowVoxelsLoader::SnowVoxelsLoader() : FileGenerator()
{
	voxel_binary_data_file = "/home/janek/32-Snow-white/20-Programy/31-SNOW-read-data/RESULT.bz2";
//	voxel_txt_dir = "";
//	voxel_caxis_file = "";
//	voxel_colors_file = "";
	grain_binary_data_file = "grain_binary.bz2";
	
	sphereYoungModulus	= 15000000.0;
	spherePoissonRatio	= 0.5;
	sphereFrictionDeg	= 18.0;
	boxYoungModulus		= 15000000.0;
	boxPoissonRatio		= 0.2;
	boxFrictionDeg		= 0.f;
	density			= 1100;
	
	dampingForce		= 0.2;
	dampingMomentum		= 0.2;
	defaultDt		= 0.00001;
	timeStepUpdateInterval	= 50;
	wallStiffnessUpdateInterval = 10;
	radiusControlInterval = 10;

	spheresColor		= Vector3r(0.8,0.3,0.3);

// a pixel is 20.4 microns (2.04 × 10-5 meters)
// the sample was 10.4mm high
	one_voxel_in_meters_is	= 2.04e-5;

	WallStressRecordFile	= "./SnowWallStresses";

	normalCohesion		= 50000000;
	shearCohesion		= 50000000;
	setCohesionOnNewContacts= false;
	
	sigma_iso = 50000;
	creep_viscosity = 4000000;
	
	thickness 		= 0.0003;
	strainRate = 10;
	StabilityCriterion = 0.01;
	autoCompressionActivation = false;
	internalCompaction	=false;
	maxMultiplier = 1.01;
	
	gravity			= Vector3r(0,0,-9.81);
	
	recordIntervalIter	= 20;

	m_grains.clear();
}


SnowVoxelsLoader::~SnowVoxelsLoader ()
{
}

void SnowVoxelsLoader::registerAttributes()
{
	FileGenerator::registerAttributes();
	REGISTER_ATTRIBUTE(voxel_binary_data_file);
	REGISTER_ATTRIBUTE(voxel_txt_dir);
	REGISTER_ATTRIBUTE(voxel_caxis_file);
	REGISTER_ATTRIBUTE(voxel_colors_file);
	REGISTER_ATTRIBUTE(grain_binary_data_file);
	REGISTER_ATTRIBUTE(one_voxel_in_meters_is);

	REGISTER_ATTRIBUTE(shearCohesion);
	REGISTER_ATTRIBUTE(normalCohesion);
	REGISTER_ATTRIBUTE(setCohesionOnNewContacts);

	REGISTER_ATTRIBUTE(sphereYoungModulus);
	REGISTER_ATTRIBUTE(spherePoissonRatio);
	REGISTER_ATTRIBUTE(sphereFrictionDeg);
	REGISTER_ATTRIBUTE(boxYoungModulus);
	REGISTER_ATTRIBUTE(boxPoissonRatio);
	REGISTER_ATTRIBUTE(boxFrictionDeg);
	REGISTER_ATTRIBUTE(density);
	REGISTER_ATTRIBUTE(gravity);
	REGISTER_ATTRIBUTE(dampingForce);
	REGISTER_ATTRIBUTE(dampingMomentum);

	REGISTER_ATTRIBUTE(WallStressRecordFile);
	
	REGISTER_ATTRIBUTE(sigma_iso);
        REGISTER_ATTRIBUTE(creep_viscosity)
}

bool SnowVoxelsLoader::load_voxels()
{
	if(grain_binary_data_file !="" && boost::filesystem::exists(grain_binary_data_file))
	{
		std::cerr << "no need to load voxels - grain binary file exists\n";
		std::cerr << "loading " << grain_binary_data_file << " ...";
			boost::iostreams::filtering_istream ifs;
			ifs.push(boost::iostreams::bzip2_decompressor());
			ifs.push(boost::iostreams::file_source(grain_binary_data_file));
		//std::ifstream ifs(m_config.load_file());
		boost::archive::binary_iarchive ia(ifs);
		ia >> m_grains;
		return true;
	}

	if(voxel_binary_data_file != "" && boost::filesystem::exists(voxel_binary_data_file))
	{
		std::cerr << "loading " << voxel_binary_data_file << " ...";
			boost::iostreams::filtering_istream ifs;
			ifs.push(boost::iostreams::bzip2_decompressor());
			ifs.push(boost::iostreams::file_source(voxel_binary_data_file));
		//std::ifstream ifs(m_config.load_file());
		boost::archive::binary_iarchive ia(ifs);
		ia >> m_voxel;
		std::cerr << " finished\n";
		return true;
	}
	else
	{
		message="cannot load txt yet, or nothing to load";
		return false;
	}
	return false;
}

bool SnowVoxelsLoader::generate()
{
	if(!load_voxels())
		return false;

	rootBody = shared_ptr<MetaBody>(new MetaBody);
	createActors(rootBody);
	positionRootBody(rootBody);

	rootBody->physicalActions		= shared_ptr<PhysicalActionContainer>(new PhysicalActionVectorVector);
	rootBody->bodies 			= shared_ptr<BodyContainer>(new BodyRedirectionVector);
	
	if(m_grains.size() == 0)
	{
		const T_DATA& dat(m_voxel.m_data.get_data_voxel_const_ref().get_a_voxel_const_ref());
		std::set<unsigned char> done;done.clear();done.insert(0);
		BOOST_FOREACH(const std::vector<std::vector<unsigned char> >& a,dat)
			BOOST_FOREACH(const std::vector<unsigned char>& b,a)
				BOOST_FOREACH(unsigned char c,b)
				{
					if(done.find(c)==done.end())
					{
						done.insert(c);
						boost::shared_ptr<BshSnowGrain> grain(new BshSnowGrain(dat,m_voxel.m_data.get_axes_const_ref()[c],c,m_voxel.m_data.get_colors_const_ref()[c],one_voxel_in_meters_is));
						m_grains.push_back(grain);
					}
				};

		std::cerr << "saving "<< grain_binary_data_file << " ...";
		boost::iostreams::filtering_ostream ofs;
		ofs.push(boost::iostreams::bzip2_compressor());
		ofs.push(boost::iostreams::file_sink(grain_binary_data_file));
		boost::archive::binary_oarchive oa(ofs);
#if BOOST_VERSION >= 103500
		oa << m_grains;
#else
		const std::vector<boost::shared_ptr<BshSnowGrain> > tmp(m_grains);
		oa << tmp;
#endif
		std::cerr << " finished\n";
	}
	
	Vector3r upperCorner(-Mathr::MAX_REAL,-Mathr::MAX_REAL,-Mathr::MAX_REAL);
	Vector3r lowerCorner( Mathr::MAX_REAL, Mathr::MAX_REAL, Mathr::MAX_REAL);
	
	shared_ptr<Body> body;
	BOOST_FOREACH(boost::shared_ptr<BshSnowGrain> grain,m_grains)
	{
		//lowerCorner, upperCorner
		BOOST_FOREACH(std::vector<Vector3r>& g,grain->slices)
			BOOST_FOREACH(Vector3r& v,g)
			{
				upperCorner = componentMaxVector(upperCorner,v+grain->center);
				lowerCorner = componentMinVector(lowerCorner,v+grain->center);
			}
	}
	Real sx = upperCorner[0] - lowerCorner[0];
	Real sy = upperCorner[1] - lowerCorner[1];
	upperCorner[0]+=sx*0.1;
	lowerCorner[0]-=sx*0.1;
	upperCorner[1]+=sy*0.1;
	lowerCorner[1]-=sy*0.1;
//	thickness = sx*0.05;
	// Z touches top and bottom, exactly.
	// but X,Y give some free space
	{ // Now make 6 walls for the TriaxialCompressionEngine
	// bottom box
	 	Vector3r center		= Vector3r(
	 						(lowerCorner[0]+upperCorner[0])/2,
	 						lowerCorner[1]-thickness/2.0,
	 						(lowerCorner[2]+upperCorner[2])/2);
	 	Vector3r halfSize	= Vector3r(
	 						1.5*fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
							thickness/2.0,
	 						1.5*fabs(lowerCorner[2]-upperCorner[2])/2+thickness);
	
		create_box(body,center,halfSize,true);
			rootBody->bodies->insert(body);
			triaxialcompressionEngine->wall_bottom_id = body->getId();
	
	// top box
	 	center			= Vector3r(
	 						(lowerCorner[0]+upperCorner[0])/2,
	 						upperCorner[1]+thickness/2.0,
	 						(lowerCorner[2]+upperCorner[2])/2);
	 	halfSize		= Vector3r(
	 						1.5*fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
	 						thickness/2.0,
	 						1.5*fabs(lowerCorner[2]-upperCorner[2])/2+thickness);
	
		create_box(body,center,halfSize,true);
			rootBody->bodies->insert(body);
			triaxialcompressionEngine->wall_top_id = body->getId();
	// box 1
	
	 	center			= Vector3r(
	 						lowerCorner[0]-thickness/2.0,
	 						(lowerCorner[1]+upperCorner[1])/2,
	 						(lowerCorner[2]+upperCorner[2])/2);
		halfSize		= Vector3r(
							thickness/2.0,
	 						1.5*fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
	 						1.5*fabs(lowerCorner[2]-upperCorner[2])/2+thickness);
		create_box(body,center,halfSize,true);
			rootBody->bodies->insert(body);
			triaxialcompressionEngine->wall_left_id = body->getId();
	// box 2
	 	center			= Vector3r(
	 						upperCorner[0]+thickness/2.0,
	 						(lowerCorner[1]+upperCorner[1])/2,
							(lowerCorner[2]+upperCorner[2])/2);
	 	halfSize		= Vector3r(
	 						thickness/2.0,
	 						1.5*fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
	 						1.5*fabs(lowerCorner[2]-upperCorner[2])/2+thickness);
	 	
		create_box(body,center,halfSize,true);
			rootBody->bodies->insert(body);
			triaxialcompressionEngine->wall_right_id = body->getId();
	// box 3
	 	center			= Vector3r(
	 						(lowerCorner[0]+upperCorner[0])/2,
	 						(lowerCorner[1]+upperCorner[1])/2,
	 						lowerCorner[2]-thickness/2.0);
	 	halfSize		= Vector3r(
	 						1.5*fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
	 						1.5*fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
	 						thickness/2.0);
		create_box(body,center,halfSize,true);
			rootBody->bodies->insert(body);
			triaxialcompressionEngine->wall_back_id = body->getId();
	
	// box 4
	 	center			= Vector3r(
	 						(lowerCorner[0]+upperCorner[0])/2,
	 						(lowerCorner[1]+upperCorner[1])/2,
	 						upperCorner[2]+thickness/2.0);
	 	halfSize		= Vector3r(
	 						1.5*fabs(lowerCorner[0]-upperCorner[0])/2+thickness,
	 						1.5*fabs(lowerCorner[1]-upperCorner[1])/2+thickness,
	 						thickness/2.0);
		create_box(body,center,halfSize,true);
			rootBody->bodies->insert(body);
			triaxialcompressionEngine->wall_front_id = body->getId();
			 
	}
	BOOST_FOREACH(boost::shared_ptr<BshSnowGrain> grain,m_grains)
	{
		create_grain(body,grain->center,true,grain);
		rootBody->bodies->insert(body);
	}
	
	return true;
}

void SnowVoxelsLoader::createActors(shared_ptr<MetaBody>& rootBody)
{
	shared_ptr<PhysicalActionContainerInitializer> physicalActionInitializer(new PhysicalActionContainerInitializer);
	physicalActionInitializer->physicalActionNames.push_back("Force");
	physicalActionInitializer->physicalActionNames.push_back("Momentum");
	physicalActionInitializer->physicalActionNames.push_back("GlobalStiffness");
	
	/* GRR
	shared_ptr<InteractionGeometryMetaEngine> interactionGeometryDispatcher(new InteractionGeometryMetaEngine);
	shared_ptr<InteractionGeometryEngineUnit> s1(new Ef2_BssSnowGrain_BssSnowGrain_makeSpheresContactGeometry);
	interactionGeometryDispatcher->add(s1);
	shared_ptr<InteractionGeometryEngineUnit> s2(new Ef2_InteractingBox_BssSnowGrain_makeSpheresContactGeometry);
	interactionGeometryDispatcher->add(s2);
	*/

	shared_ptr<InteractionGeometryMetaEngine> interactionGeometryDispatcher(new InteractionGeometryMetaEngine);
	shared_ptr<InteractionGeometryEngineUnit> s1(new InteractingSphere2InteractingSphere4SpheresContactGeometry);
	interactionGeometryDispatcher->add(s1);
	shared_ptr<InteractionGeometryEngineUnit> s2(new InteractingBox2InteractingSphere4SpheresContactGeometry);
	interactionGeometryDispatcher->add(s2);

	shared_ptr<CohesiveFrictionalRelationships> cohesiveFrictionalRelationships = shared_ptr<CohesiveFrictionalRelationships> (new CohesiveFrictionalRelationships);
	cohesiveFrictionalRelationships->shearCohesion = shearCohesion;
	cohesiveFrictionalRelationships->normalCohesion = normalCohesion;
	cohesiveFrictionalRelationships->setCohesionOnNewContacts = setCohesionOnNewContacts;
	cohesiveFrictionalRelationships->setCohesionNow = true;
	shared_ptr<InteractionPhysicsMetaEngine> interactionPhysicsDispatcher(new InteractionPhysicsMetaEngine);
	interactionPhysicsDispatcher->add(cohesiveFrictionalRelationships);
		
	shared_ptr<BoundingVolumeMetaEngine> boundingVolumeDispatcher	= shared_ptr<BoundingVolumeMetaEngine>(new BoundingVolumeMetaEngine);
	boundingVolumeDispatcher->add("InteractingSphere2AABB");
	boundingVolumeDispatcher->add("InteractingBox2AABB");
	boundingVolumeDispatcher->add("MetaInteractingGeometry2AABB");

	

		
	shared_ptr<GravityEngine> gravityCondition(new GravityEngine);
	gravityCondition->gravity = gravity;
	
	shared_ptr<CundallNonViscousForceDamping> actionForceDamping(new CundallNonViscousForceDamping);
	actionForceDamping->damping = dampingForce;
	shared_ptr<CundallNonViscousMomentumDamping> actionMomentumDamping(new CundallNonViscousMomentumDamping);
	actionMomentumDamping->damping = dampingMomentum;
	shared_ptr<PhysicalActionDamper> actionDampingDispatcher(new PhysicalActionDamper);
	actionDampingDispatcher->add(actionForceDamping);
	actionDampingDispatcher->add(actionMomentumDamping);
	
	shared_ptr<PhysicalActionApplier> applyActionDispatcher(new PhysicalActionApplier);
	applyActionDispatcher->add("NewtonsForceLaw");
	applyActionDispatcher->add("NewtonsMomentumLaw");
		
	shared_ptr<PhysicalParametersMetaEngine> positionIntegrator(new PhysicalParametersMetaEngine);
	positionIntegrator->add("LeapFrogPositionIntegrator");
	shared_ptr<PhysicalParametersMetaEngine> orientationIntegrator(new PhysicalParametersMetaEngine);
	orientationIntegrator->add("LeapFrogOrientationIntegrator");

	shared_ptr<GlobalStiffnessTimeStepper> globalStiffnessTimeStepper(new GlobalStiffnessTimeStepper);
	globalStiffnessTimeStepper->sdecGroupMask = 2;
	globalStiffnessTimeStepper->timeStepUpdateInterval = timeStepUpdateInterval;
	globalStiffnessTimeStepper->defaultDt = defaultDt;
	globalStiffnessTimeStepper->timestepSafetyCoefficient = 0.2;
	
	shared_ptr<CohesiveFrictionalContactLaw> cohesiveFrictionalContactLaw(new CohesiveFrictionalContactLaw);
	cohesiveFrictionalContactLaw->sdecGroupMask = 2;
	cohesiveFrictionalContactLaw->shear_creep = true;
	cohesiveFrictionalContactLaw->twist_creep = true;
	cohesiveFrictionalContactLaw->creep_viscosity = creep_viscosity;
		
	shared_ptr<GlobalStiffnessCounter> globalStiffnessCounter(new GlobalStiffnessCounter);
	// globalStiffnessCounter->sdecGroupMask = 2;
	globalStiffnessCounter->interval = timeStepUpdateInterval;
	
	// moving walls to regulate the stress applied + compress when the packing is dense an stable
	//cerr << "triaxialcompressionEngine = shared_ptr<TriaxialCompressionEngine> (new TriaxialCompressionEngine);" << std::endl;
	triaxialcompressionEngine = shared_ptr<TriaxialCompressionEngine> (new TriaxialCompressionEngine);
	triaxialcompressionEngine-> stiffnessUpdateInterval = wallStiffnessUpdateInterval;// = stiffness update interval
	triaxialcompressionEngine-> radiusControlInterval = radiusControlInterval;// = stiffness update interval
	triaxialcompressionEngine-> sigma_iso = sigma_iso;
	triaxialcompressionEngine-> sigmaLateralConfinement = sigma_iso;
	triaxialcompressionEngine-> sigmaIsoCompaction = sigma_iso;
	triaxialcompressionEngine-> max_vel = 1;
	triaxialcompressionEngine-> thickness = thickness;
	triaxialcompressionEngine->strainRate = strainRate;
	triaxialcompressionEngine->StabilityCriterion = StabilityCriterion;
	triaxialcompressionEngine->autoCompressionActivation = autoCompressionActivation;
	triaxialcompressionEngine->internalCompaction = internalCompaction;
	triaxialcompressionEngine->maxMultiplier = maxMultiplier;
	
	shared_ptr<HydraulicForceEngine> hydraulicForceEngine = shared_ptr<HydraulicForceEngine> (new HydraulicForceEngine);
	hydraulicForceEngine->dummyParameter = true;
		
	//cerr << "fin de section triaxialcompressionEngine = shared_ptr<TriaxialCompressionEngine> (new TriaxialCompressionEngine);" << std::endl;
	
// recording global stress
	triaxialStateRecorder = shared_ptr<TriaxialStateRecorder>(new TriaxialStateRecorder);
	triaxialStateRecorder-> outputFile 	= WallStressRecordFile;
	triaxialStateRecorder-> interval 		= recordIntervalIter;
	//triaxialStateRecorder-> thickness 		= thickness;
	
	
	// moving walls to regulate the stress applied
	//cerr << "triaxialstressController = shared_ptr<TriaxialStressController> (new TriaxialStressController);" << std::endl;
	triaxialstressController = YADE_PTR_CAST<TriaxialStressController>(triaxialcompressionEngine);
	triaxialstressController-> stiffnessUpdateInterval = wallStiffnessUpdateInterval;// = recordIntervalIter
	triaxialstressController-> sigma_iso = sigma_iso;
	triaxialstressController-> max_vel = 1;
	triaxialstressController-> thickness = thickness;
	triaxialstressController->wall_bottom_activated = false;
	triaxialstressController->wall_top_activated = false;
	triaxialstressController->wall_left_activated = false;
	triaxialstressController->wall_right_activated = false;
	triaxialstressController->wall_front_activated = true;
	triaxialstressController->wall_back_activated = true;
		//cerr << "fin de sezction triaxialstressController = shared_ptr<TriaxialStressController> (new TriaxialStressController);" << std::endl;
	
	rootBody->engines.clear();
	rootBody->engines.push_back(shared_ptr<Engine>(new PhysicalActionContainerReseter));
//	rootBody->engines.push_back(sdecTimeStepper);	
	rootBody->engines.push_back(boundingVolumeDispatcher);
	rootBody->engines.push_back(shared_ptr<Engine>(new DistantPersistentSAPCollider));
	rootBody->engines.push_back(interactionGeometryDispatcher);
	rootBody->engines.push_back(interactionPhysicsDispatcher);
	rootBody->engines.push_back(cohesiveFrictionalContactLaw);
	rootBody->engines.push_back(triaxialcompressionEngine);
	//rootBody->engines.push_back(stiffnesscounter);
	//rootBody->engines.push_back(stiffnessMatrixTimeStepper);
	rootBody->engines.push_back(globalStiffnessCounter);
	rootBody->engines.push_back(globalStiffnessTimeStepper);
	rootBody->engines.push_back(triaxialStateRecorder);
/////	rootBody->engines.push_back(gravityCondition);
	rootBody->engines.push_back(actionDampingDispatcher);
	rootBody->engines.push_back(applyActionDispatcher);
	rootBody->engines.push_back(positionIntegrator);
	//if(!rotationBlocked)
		rootBody->engines.push_back(orientationIntegrator);
	//rootBody->engines.push_back(resultantforceEngine);
	//rootBody->engines.push_back(triaxialstressController);
	
		
	//rootBody->engines.push_back(averagePositionRecorder);
	//rootBody->engines.push_back(velocityRecorder);
	//rootBody->engines.push_back(forcerec);
	
	//if (saveAnimationSnapshots) {
	//shared_ptr<PositionOrientationRecorder> positionOrientationRecorder(new PositionOrientationRecorder);
	//positionOrientationRecorder->outputFile = AnimationSnapshotsBaseName;
	//rootBody->engines.push_back(positionOrientationRecorder);}
	
	rootBody->initializers.clear();
	rootBody->initializers.push_back(physicalActionInitializer);
	rootBody->initializers.push_back(boundingVolumeDispatcher);
	
}

void SnowVoxelsLoader::positionRootBody(shared_ptr<MetaBody>& rootBody)
{
	rootBody->isDynamic		= false;

	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);
	shared_ptr<ParticleParameters> physics(new ParticleParameters); // FIXME : fix indexable class PhysicalParameters
	physics->se3			= Se3r(Vector3r(0,0,0),q);
	physics->mass			= 0;
	physics->velocity		= Vector3r::ZERO;
	physics->acceleration		= Vector3r::ZERO;
	
	shared_ptr<MetaInteractingGeometry> set(new MetaInteractingGeometry());
	
	set->diffuseColor		= Vector3r(0,0,1);

	shared_ptr<AABB> aabb(new AABB);
	aabb->diffuseColor		= Vector3r(0,0,1);
	
	rootBody->interactingGeometry	= YADE_PTR_CAST<InteractingGeometry>(set);	
	rootBody->boundingVolume	= YADE_PTR_CAST<BoundingVolume>(aabb);
	rootBody->physicalParameters 	= physics;
	
}

void SnowVoxelsLoader::create_grain(shared_ptr<Body>& body, Vector3r position, bool dynamic , boost::shared_ptr<BshSnowGrain> grain)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),2));
	shared_ptr<CohesiveFrictionalBodyParameters> physics(new CohesiveFrictionalBodyParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<BshSnowGrain> gSnowGrain(grain);
	

	//GRR shared_ptr<BssSnowGrain> iSphere(new BssSnowGrain(gSnowGrain.get(),one_voxel_in_meters_is));
	//GRR Real radius = iSphere->radius;
	shared_ptr<InteractingSphere> iSphere(new InteractingSphere);
	Real radius = (grain->start-grain->end).Length()*0.5;
	
	Quaternionr q;//(Mathr::SymmetricRandom(),Mathr::SymmetricRandom(),Mathr::SymmetricRandom(),Mathr::SymmetricRandom());
	q.FromAxisAngle( Vector3r(0,0,1),0);
	q.Normalize();
	
	body->isDynamic			= dynamic;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= 4.0/3.0*Mathr::PI*radius*radius*radius*density;
	
	physics->inertia		= Vector3r( 	2.0/5.0*physics->mass*radius*radius,
							2.0/5.0*physics->mass*radius*radius,
							2.0/5.0*physics->mass*radius*radius);
	physics->se3			= Se3r(position,q);
	physics->young			= sphereYoungModulus;
	physics->poisson		= spherePoissonRatio;
	physics->frictionAngle		= sphereFrictionDeg * Mathr::PI/180.0;

	aabb->diffuseColor		= Vector3r(0,1,0);


	gSnowGrain->diffuseColor	= grain->color;
	gSnowGrain->wire		= false;
	gSnowGrain->visible		= true;
	gSnowGrain->shadowCaster	= true;
	
	iSphere->radius			= radius; // already calculated //GRR
	iSphere->diffuseColor		= grain->color;

	body->interactingGeometry	= iSphere;
	body->geometricalModel		= gSnowGrain;
	body->boundingVolume		= aabb;
	body->physicalParameters	= physics;
}


void SnowVoxelsLoader::create_box(shared_ptr<Body>& body, Vector3r position, Vector3r extents, bool wire)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),2));
	shared_ptr<CohesiveFrictionalBodyParameters> physics(new CohesiveFrictionalBodyParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Box> gBox(new Box);
	shared_ptr<InteractingBox> iBox(new InteractingBox);
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);

	body->isDynamic			= false;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= 0; 
	//physics->mass			= extents[0]*extents[1]*extents[2]*density*2; 
	physics->inertia		= Vector3r(
							  physics->mass*(extents[1]*extents[1]+extents[2]*extents[2])/3
							, physics->mass*(extents[0]*extents[0]+extents[2]*extents[2])/3
							, physics->mass*(extents[1]*extents[1]+extents[0]*extents[0])/3
						);
//	physics->mass			= 0;
//	physics->inertia		= Vector3r(0,0,0);
	physics->se3			= Se3r(position,q);

	physics->young			= boxYoungModulus;
	physics->poisson		= boxPoissonRatio;
	physics->frictionAngle		= boxFrictionDeg * Mathr::PI/180.0;
	physics->isCohesive		= false;

	aabb->diffuseColor		= Vector3r(1,0,0);

	gBox->extents			= extents;
	gBox->diffuseColor		= Vector3r(0.5,0.5,0.5);
	gBox->wire			= wire;
	gBox->visible			= true;
	gBox->shadowCaster		= false;
	
	iBox->extents			= extents;
	iBox->diffuseColor		= Vector3r(0.5,0.5,0.5);

	body->boundingVolume		= aabb;
	body->interactingGeometry	= iBox;
	body->geometricalModel		= gBox;
	body->physicalParameters	= physics;
}

