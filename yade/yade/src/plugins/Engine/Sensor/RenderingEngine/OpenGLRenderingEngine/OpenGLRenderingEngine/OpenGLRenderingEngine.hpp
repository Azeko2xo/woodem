/***************************************************************************
 *   Copyright (C) 2004 by Olivier Galizzi                                 *
 *   olivier.galizzi@imag.fr                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __OPRNGLRENDERINGENGINE_H__
#define __OPRNGLRENDERINGENGINE_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/RenderingEngine.hpp>
#include <yade-lib-multimethods/DynLibDispatcher.hpp>

#include "GLDrawBoundingVolumeFunctor.hpp"
#include "GLDrawInteractionGeometryFunctor.hpp"
#include "GLDrawGeometricalModelFunctor.hpp"
#include "GLDrawShadowVolumeFunctor.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class OpenGLRenderingEngine : public RenderingEngine
{	
	public : Vector3r lightPos;
	public : bool drawBoundingVolume;
	public : bool drawInteractionGeometry;
	public : bool drawGeometricalModel;
	public : bool castShadow;
	public : bool drawShadowVolumes;
	public : bool useFastShadowVolume;
	public : bool drawWireFrame;
	public : bool drawInside;
	
	private : DynLibDispatcher< BoundingVolume    , GLDrawBoundingVolumeFunctor, void , TYPELIST_1(const shared_ptr<BoundingVolume>&) > boundingVolumeDispatcher;
	private : DynLibDispatcher< InteractingGeometry , GLDrawInteractionGeometryFunctor, void , TYPELIST_2(const shared_ptr<InteractingGeometry>&, const shared_ptr<PhysicalParameters>&) >interactionGeometryDispatcher;
	private : DynLibDispatcher< GeometricalModel  , GLDrawGeometricalModelFunctor, void , TYPELIST_3(const shared_ptr<GeometricalModel>&, const shared_ptr<PhysicalParameters>&,bool) > geometricalModelDispatcher;
	private : DynLibDispatcher< GeometricalModel  , GLDrawShadowVolumeFunctor, void , TYPELIST_3(const shared_ptr<GeometricalModel>&, const shared_ptr<PhysicalParameters>&, const Vector3r& ) > shadowVolumeDispatcher;

	private : vector<vector<string> >  boundingVolumeFunctorNames;
	private : vector<vector<string> >  interactionGeometryFunctorNames;
	private : vector<vector<string> >  geometricalModelFunctorNames;
	private : vector<vector<string> >  shadowVolumeFunctorNames;
	public  : void addBoundingVolumeFunctor(const string& str1,const string& str2);
	public  : void addInteractionGeometryFunctor(const string& str1,const string& str2);
	public  : void addGeometricalModelFunctor(const string& str1,const string& str2);
	public  : void addShadowVolumeFunctor(const string& str1,const string& str2);
	
	private : bool needInit;

			
	public : OpenGLRenderingEngine();
	public : virtual ~OpenGLRenderingEngine();
	
	public : void init();
	public : void render(const shared_ptr<MetaBody>& body);
	
	private : void renderGeometricalModel(const shared_ptr<MetaBody>& rootBody);
	private : void renderBoundingVolume(const shared_ptr<MetaBody>& rootBody);
	private : void renderInteractionGeometry(const shared_ptr<MetaBody>& rootBody);
	private : void renderShadowVolumes(const shared_ptr<MetaBody>& rootBody,Vector3r lightPos);
	private : void renderSceneUsingShadowVolumes(const shared_ptr<MetaBody>& rootBody,Vector3r lightPos);
	private : void renderSceneUsingFastShadowVolumes(const shared_ptr<MetaBody>& rootBody,Vector3r lightPos);
	
	REGISTER_CLASS_NAME(OpenGLRenderingEngine);
	public : void postProcessAttributes(bool deserializing);
	
	public : void registerAttributes();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_SERIALIZABLE(OpenGLRenderingEngine,false);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __OPRNGLRENDERINGENGINE_H__
