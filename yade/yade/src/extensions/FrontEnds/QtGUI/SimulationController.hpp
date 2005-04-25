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

#ifndef __SIMULATIONCONTROLLER_H__
#define __SIMULATIONCONTROLLER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "GLViewer.hpp"
#include "QtGeneratedSimulationController.h"
#include "QtGUIGenerator.hpp"
#include "RenderingEngine.hpp"
#include "SimulationControllerUpdater.hpp"

#include <qlayout.h>
#include <qframe.h>
#include <qscrollview.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class SimulationController : public QtGeneratedSimulationController
{
	private : boost::mutex mutex;
	private : QtGUIGenerator guiGen;	
	private : /*static*/ QWidget * parentWorkspace;	
	private : shared_ptr<RenderingEngine> renderer;
	//public : static void load();
	private : map<int,GLViewer* > glViews;
	private : int maxNbViews;
		
	private : shared_ptr<SimulationControllerUpdater> updater;
	
	private : QScrollView * scrollView;
	private : QFrame * scrollViewFrame;

	
	// construction
	public : SimulationController (QWidget * parent=0);

	public : ~SimulationController (); 
	
	public slots : virtual void pbApplyClicked();
	public slots : virtual void pbLoadClicked();
	public slots : virtual void pbNewViewClicked();
	public slots : virtual void pbStopClicked();
	public slots : virtual void pbStartClicked();
	public slots : virtual void pbResetClicked();
	public slots : virtual void pbCenterSceneClicked();
	public slots : virtual void pbOneSimulationStepClicked();
	
	public slots : void closeGLViewEvent(int id);

	private : void addNewView();
	
	protected : void closeEvent(QCloseEvent *evt);

};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __SIMULATIONCONTROLLER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
