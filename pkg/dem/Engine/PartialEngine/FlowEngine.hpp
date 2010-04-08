/*************************************************************************
*  Copyright (C) 2009 by Emanuele Catalano                               *
*  emanuele.catalano@hmg.inpg.fr                                            *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include<yade/core/PartialEngine.hpp>
#include<yade/pkg-dem/TriaxialCompressionEngine.hpp>
#include<yade/lib-triangulation/FlowBoundingSphere.h>

#ifdef FLOW_ENGINE

class FlowEngine : public PartialEngine
{
	private:
		shared_ptr<TriaxialCompressionEngine> triaxialCompressionEngine;
		shared_ptr<CGT::FlowBoundingSphere> flow;
	public :	
		Vector3r gravity;
		int current_state;
		Real wall_thickness;
		bool Update_Triangulation;
		
		void Triangulate ();
		void AddBoundary ();
		void Build_Triangulation (double P_zero );
		void UpdateVolumes ();
		void Initialize_volumes ();
		Real Volume_cell_single_fictious (CGT::Cell_handle cell);
		Real Volume_cell_double_fictious (CGT::Cell_handle cell);
		Real Volume_cell_triple_fictious (CGT::Cell_handle cell);
		Real Volume_cell (CGT::Cell_handle cell);
		void Oedometer_Boundary_Conditions();
		
		virtual ~FlowEngine();
	
		virtual void action();
		
		YADE_CLASS_BASE_DOC_ATTRS_CTOR(FlowEngine,PartialEngine,"An engine to solve flow problem in saturated granular media",
					((bool,isActivated,true,"Activates Flow Engine"))
					((bool,first,true,"Controls the initialization/update phases"))
					((bool,save_vtk,false,"Enable/disable vtk files creation for visualization"))
					((bool,save_mplot,false,"Enable/disable mplot files creation"))
					((bool, slip_boundary, true, "Controls friction condition on lateral walls"))
					((bool,currentTes,false,"Identifies the current triangulation/tesselation of pore space"))
					((double,P_zero,0,"Initial internal pressure for oedometer test"))
					((double,Tolerance,1e-06,"Gauss-Seidel Tolerance"))
					((double,Relax,1.9,"Gauss-Seidel relaxation"))
					((int,PermuteInterval,100000,"Pore space re-triangulation period"))
					((bool,compute_K,true,"Activates permeability measure within a granular sample"))
					((double,permeability_factor,1.0,"a permability multiplicator"))
					((Real,loadFactor,1.1,"Load multiplicator for oedometer test"))
					((double, K, 0, "Permeability of the sample"))
					((double, MaxPressure, 0, "Maximal value of water pressure within the sample"))
					((double, currentStress, 0, "Current value of axial stress"))
					((double, currentStrain, 0, "Current value of axial strain"))
					((bool, Retriangulation, 0, "Enable/Disable sample retriangulation"))
					((int, intervals, 30, "Number of layers for pressure measurements"))
					((bool,tess_based_force,true,"true=force computation based on tessalation, false=force computation based on triangulation")),
					timingDeltas=shared_ptr<TimingDeltas>(new TimingDeltas));
		DECLARE_LOGGER;
};

REGISTER_SERIALIZABLE(FlowEngine);

#endif
