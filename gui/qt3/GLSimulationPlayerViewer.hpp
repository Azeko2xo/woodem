/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef SIMULATIONPLAYERVIEWER_HPP
#define SIMULATIONPLAYERVIEWER_HPP

#include<yade/core/Omega.hpp>
#include<yade/core/RenderingEngine.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/gui-qt3/GLViewer.hpp>
#include<boost/filesystem/operations.hpp>
#include<boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include<yade/lib-sqlite3x/sqlite3x.hpp>

class QtSimulationPlayer;
class FilterEngine;

class GLSimulationPlayerViewer: public GLViewer {
	private :
		shared_ptr<MetaBody>		 rootBody;
		void tryFillingOutputPattern();	
		bool useSQLite;
		shared_ptr<sqlite3x::sqlite3_connection> con;
		//!  read from db, used in getRealTimeString() called in GLViewer::postDraw
		Real wallClock, realTime;
		virtual string getRealTimeString();
	public:
		list<string> snapshots;
		QtSimulationPlayer* simPlayer;
		boost::posix_time::ptime lastCheckPointTime;
		long lastCheckPointFrame;
		string fileName, inputBaseName, inputBaseDirectory, outputBaseName, outputBaseDirectory, snapshotsBase;
		bool saveSnapShots;
		int frameNumber;
		int stride;
 		bool loadNextRecordedData();
 		//! filenames or table names (if useSQLite)
 		list<string> xyzNames;
 		list<string>::iterator xyzNamesIter;
		vector< shared_ptr< FilterEngine > > filters;
	public :
		GLSimulationPlayerViewer(QWidget* parent);
		virtual ~GLSimulationPlayerViewer(){};
		void setRootBody(shared_ptr<MetaBody> rb) { rootBody = rb;};
		void load(const string& fileName, bool fromFile=true);
		void doOneStep();
		void reset();
		void bodyWire(bool wire);
	protected :
		virtual void animate();
		virtual void initializeGL();
		virtual void closeEvent(QCloseEvent *e);
		virtual void keyPressEvent(QKeyEvent* e);

	DECLARE_LOGGER;
};

#endif // SIMULATIONVIEWER_HPP

