/*************************************************************************
*  Copyright (C) 2008 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/FileGenerator.hpp>
#include<Wm3Vector3.h>
#include<yade/lib-base/yadeWm3.hpp>

#include<set>
#include<string>
#include<boost/foreach.hpp>
#include<boost/archive/binary_oarchive.hpp>
#include<boost/archive/binary_iarchive.hpp>
#include<boost/serialization/vector.hpp>
#include<boost/serialization/set.hpp>
#include<boost/serialization/list.hpp>
#include<boost/serialization/utility.hpp>

#include<boost/iostreams/filtering_stream.hpp>
#include<boost/iostreams/filter/gzip.hpp>
#include<boost/iostreams/filter/bzip2.hpp>
#include<boost/iostreams/device/file.hpp>
#include<boost/shared_ptr.hpp>

#include"Voxel/VoxelEnvelope.hpp"
#include<yade/pkg-snow/BshSnowGrain.hpp>
#include<yade/pkg-snow/BssSnowGrain.hpp>

class VoxelDocument
{
	public:
		VoxelEnvelope	m_data;
	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			//boost::mutex::scoped_lock scoped_lock(m_document_mutex);
			ar & m_data;
		}
};

class SnowVoxelsLoader : public FileGenerator
{
	private:
		Vector3r	 gravity
				,lowerCorner
				,upperCorner;

		Vector3r	 spheresColor;

		Real		 sphereYoungModulus
				,spherePoissonRatio
				,sphereFrictionDeg
				,boxYoungModulus
				,boxPoissonRatio
				,boxFrictionDeg
				,density
				
				,dampingForce
				,dampingMomentum

				,defaultDt

				,normalCohesion
				,shearCohesion
				
				,creep_viscosity
				,sigma_iso
				,thickness
				
				,strainRate
				,StabilityCriterion
				
				,maxMultiplier ///max multiplier of diameters during internal compaction
				;
		bool		 setCohesionOnNewContacts
				,autoCompressionActivation
				,internalCompaction
				;
		Real		 one_voxel_in_meters_is;

		int		 timeStepUpdateInterval
				,radiusControlInterval
				,wallStiffnessUpdateInterval

				,recordIntervalIter
				;
				
		std::string	WallStressRecordFile;

		std::string	voxel_binary_data_file;
		std::string	voxel_txt_dir;
		std::string	voxel_caxis_file;
		std::string	voxel_colors_file;
		std::string	grain_binary_data_file;
		
		shared_ptr<TriaxialCompressionEngine> triaxialcompressionEngine;
		shared_ptr<TriaxialStressController> triaxialstressController;
		shared_ptr<TriaxialStateRecorder> triaxialStateRecorder;

		VoxelDocument	m_voxel;
		std::vector<boost::shared_ptr<BshSnowGrain> > m_grains;

	public : 
		SnowVoxelsLoader();
		~SnowVoxelsLoader();
		virtual bool generate();
		bool load_voxels();
		void createActors(shared_ptr<MetaBody>& rootBody);
		void positionRootBody(shared_ptr<MetaBody>& rootBody);
		void create_grain(shared_ptr<Body>& body, Vector3r position, bool dynamic , boost::shared_ptr<BshSnowGrain> grain);
		void create_box(shared_ptr<Body>& body, Vector3r position, Vector3r extents, bool wire);
	
	protected :
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(SnowVoxelsLoader);
	REGISTER_BASE_CLASS_NAME(FileGenerator);
};

REGISTER_SERIALIZABLE(SnowVoxelsLoader);

