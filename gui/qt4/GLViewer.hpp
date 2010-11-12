// Copyright (C) 2004 by Olivier Galizzi, Janek Kozicki                  *
// © 2008 Václav Šmilauer
#pragma once

#include<yade/core/Omega.hpp>
#include<yade/pkg/common/OpenGLRenderer.hpp>

#include<QGLViewer/qglviewer.h>
#include<QGLViewer/constraint.h>

#include<boost/date_time/posix_time/posix_time.hpp>
#include<set>

#include<yade/pkg/common/PeriodicEngines.hpp>

/*****************************************************************************
*********************************** SnapshotEngine ***************************
*****************************************************************************/
/* NOTE: this engine bypasses usual registration of plugins;
	pyRegisterClass must be called in the module import stanza in gui/qt4/_GLViewer.cpp
*/
class SnapshotEngine: public PeriodicEngine{
	public:
	virtual void action();
	YADE_CLASS_BASE_DOC_ATTRS(SnapshotEngine,PeriodicEngine,"Periodically save snapshots of GLView(s) as .png files. Files are named *fileBase*+*counter*+'.png' (counter is left-padded by 0s, i.e. snap00004.png).",
		((string,format,"PNG",,"Format of snapshots (one of JPEG, PNG, EPS, PS, PPM, BMP) `QGLViewer documentation <http://www.libqglviewer.com/refManual/classQGLViewer.html#abbb1add55632dced395e2f1b78ef491c>`_. File extension will be lowercased *format*. Validity of format is not checked."))
		((string,fileBase,"",,"Basename for snapshots"))
		((int,counter,0,,"Number that will be appended to fileBase when the next snapshot is saved (incremented at every save). |yupdate|"))
		((bool,ignoreErrors,true,,"Only report errors instead of throwing exceptions, in case of timeouts."))
		((vector<string>,snapshots,,,"Files that have been created so far"))
		((int,msecSleep,0,,"number of msec to sleep after snapshot (to prevent 3d hw problems) [ms]"))
		((Real,deadTimeout,3,,"Timeout for 3d operations (opening new view, saving snapshot); after timing out, throw exception (or only report error if *ignoreErrors*) and make myself :yref:`dead<Engine.dead>`. [s]"))
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(SnapshotEngine);

/*! Class handling user interaction with the openGL rendering of simulation.
 *
 * Clipping planes:
 * ================
 *
 * Clipping plane is manipulated after hitting F1, F2, .... To end the manipulation, press Escape.
 *
 * Keystrokes during clipping plane manipulation:
 * * space activates/deactives the clipping plane
 * * x,y,z aligns the plane with yz, xz, xy planes
 * * left-double-click aligns the plane with world coordinates system (canonical planes + 45˚ interpositions)
 * * 1,2,... will align the current plane with #1, #2, ... (same orientation)
 * * r reverses the plane (normal*=-1)a
 *
 * Keystrokes that work regardless of whether a clipping plane is being manipulated:
 * * Alt-1,Alt-2,... adds/removes the respective plane to bound group:
 * 	mutual positions+orientations of planes in the group are maintained when one of those planes is manipulated
 *
 * Clip plane number is 3; change YADE_RENDERER_NUM_CLIP_PLANE, complete switches "|| ..." in keyPressEvent
 * and recompile to have more.
 */
class GLViewer : public QGLViewer
{	
	Q_OBJECT 
	
	friend class QGLThread;
	protected:
		shared_ptr<OpenGLRenderer> renderer;

	private :

		bool			isMoving;
		bool			wasDynamic;
		float			cut_plane;
		int			cut_plane_delta;
		bool			grid_subdivision;
		int manipulatedClipPlane;
		set<int> boundClipPlanes;
		shared_ptr<qglviewer::LocalConstraint> xyPlaneConstraint;
		string strBoundGroup(){string ret;FOREACH(int i, boundClipPlanes) ret+=" "+lexical_cast<string>(i+1);return ret;}
		boost::posix_time::ptime last_user_event;

     public:
		//virtual void updateGL(void);

		const int viewId;

		void centerMedianQuartile();
		bool 			drawGridXYZ[3];
		bool 			drawScale;
		int timeDispMask;
		enum{TIME_REAL=1,TIME_VIRT=2,TIME_ITER=4};

		GLViewer(int viewId, const shared_ptr<OpenGLRenderer>& renderer, QGLWidget* shareWidget=0);
		virtual ~GLViewer();
		#if 0
			virtual void paintGL();
		#endif
		virtual void draw();
		virtual void drawWithNames();
		void displayMessage(const std::string& s){ QGLViewer::displayMessage(QString(s.c_str()));}
		void centerScene();
		void centerPeriodic();
		void mouseMovesCamera();
		void mouseMovesManipulatedFrame(qglviewer::Constraint* c=NULL);
		void resetManipulation();
		bool isManipulating();
		void startClipPlaneManipulation(int planeNo);
		//! get QGLViewer state as string (XML); QGLViewer normally only supports saving state to file.
		string getState();
		//! set QGLViewer state from string (XML); QGLVIewer normally only supports loading state from file.
		void setState(string);
		//! Load display parameters (QGLViewer and OpenGLRenderer) from Scene::dispParams[n] and use them
		void useDisplayParameters(size_t n);
		//! Save display parameters (QGOViewer and OpenGLRenderer) to Scene::dispParams[n]
		void saveDisplayParameters(size_t n);
		//! Get radius of the part of scene that fits the current view
		float displayedSceneRadius();
		//! Get center of the part of scene that fits the current view
		qglviewer::Vec displayedSceneCenter();

		//! Adds our attributes to the QGLViewer state that can be saved
		QDomElement domElement(const QString& name, QDomDocument& document) const;
		//! Adds our attributes to the QGLViewer state that can be restored
		void initFromDOMElement(const QDomElement& element);

		// if defined, snapshot will be saved to this file right after being drawn and the string will be reset.
		// this way the caller will be notified of the frame being saved successfully.
		string nextFrameSnapshotFilename;
		#ifdef YADE_GL2PS
			// output stream for gl2ps; initialized as needed
			FILE* gl2psStream;
		#endif

		boost::posix_time::ptime getLastUserEvent();


		DECLARE_LOGGER;
	protected :
		virtual void keyPressEvent(QKeyEvent *e);
		virtual void postDraw();
		// overridden in the player that doesn't get time from system clock but from the db
		virtual string getRealTimeString();
		virtual void closeEvent(QCloseEvent *e);
		virtual void postSelection(const QPoint& point);
		virtual void endSelection(const QPoint &point);
		virtual void mouseDoubleClickEvent(QMouseEvent *e);
		virtual void wheelEvent(QWheelEvent* e);
		virtual void mouseMoveEvent(QMouseEvent *e);
		virtual void mousePressEvent(QMouseEvent *e);
};

/*! Get unconditional lock on a GL view.

Use if you need to manipulate GL context in some way.
The ctor doesn't return until the lock has been acquired
and the lock is released when the GLLock object is desctructed;
*/
class GLLock: public boost::try_mutex::scoped_lock{
	GLViewer* glv;
	public:
		GLLock(GLViewer* _glv);
		~GLLock(); 
};


class YadeCamera : public qglviewer::Camera
{	
	Q_OBJECT 
	private:
		float cuttingDistance;
        public :
		YadeCamera():cuttingDistance(0){};
		virtual float zNear() const;
		virtual void setCuttingDistance(float s){cuttingDistance=s;};
};



