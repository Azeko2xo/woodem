// Copyright (C) 2004 by Olivier Galizzi, Janek Kozicki                  *
// © 2008 Václav Šmilauer
#pragma once

#include<yade/core/Omega.hpp>
#include<yade/pkg/gl/Renderer.hpp>

#include<QGLViewer/qglviewer.h>
#include<QGLViewer/constraint.h>

#include<QMouseEvent>

#include<boost/date_time/posix_time/posix_time.hpp>
#include<set>


#if 1
#include<yade/core/Engine.hpp>

/*****************************************************************************
*********************************** SnapshotEngine ***************************
*****************************************************************************/
/* NOTE: this engine bypasses usual registration of plugins;
	pyRegisterClass must be called in the module import stanza in gui/qt4/_GLViewer.cpp
*/
struct SnapshotEngine: public PeriodicEngine{
	virtual void run();
	virtual bool needsField(){ return false; }
	virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d);
	YADE_CLASS_BASE_DOC_ATTRS(SnapshotEngine,PeriodicEngine,"Periodically save snapshots of GLView(s) as .png files. Files are named *fileBase*+*counter*+'.png' (counter is left-padded by 0s, i.e. snap00004.png).",
		((string,format,"PNG",,"Format of snapshots (one of JPEG, PNG, EPS, PS, PPM, BMP) `QGLViewer documentation <http://www.libqglviewer.com/refManual/classQGLViewer.html#abbb1add55632dced395e2f1b78ef491c>`_. File extension will be lowercased *format*. Validity of format is not checked."))
		((string,fileBase,"",,"Basename for snapshots"))
		((int,counter,0,Attr::readonly,"Number that will be appended to fileBase when the next snapshot is saved (incremented at every save)."))
		((bool,ignoreErrors,true,,"Only report errors instead of throwing exceptions, in case of timeouts."))
		((vector<string>,snapshots,,,"Files that have been created so far"))
		((int,msecSleep,0,,"number of msec to sleep after snapshot (to prevent 3d hw problems) [ms]"))
		((Real,deadTimeout,3,,"Timeout for 3d operations (opening new view, saving snapshot); after timing out, throw exception (or only report error if *ignoreErrors*) and make myself :yref:`dead<Engine.dead>`. [s]"))
		((string,plot,,,"Name of field in :yref:`yade.plot.imgData` to which taken snapshots will be appended automatically."))
	);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(SnapshotEngine);

#endif

// for movable colorscales
// mostly copied from
// http://www.libqglviewer.com/refManual/classqglviewer_1_1MouseGrabber.html#_details
struct QglMovableObject: public qglviewer::MouseGrabber{
	QglMovableObject(int x0, int y0): qglviewer::MouseGrabber(), pos(x0,y0), moved(false){}
	void checkIfGrabsMouse(int x, int y, const qglviewer::Camera* const){
		QPoint relPos(QPoint(x,y)-pos);
		bool isInside=(relPos.x()>=0 && relPos.y()>=0 && relPos.x()<=dim.x() && relPos.y()<=dim.y());
		// cerr<<"relPos="<<relPos.x()<<","<<relPos.y()<<", dim="<<dim.x()<<","<<dim.y()<<", pos="<<pos.x()<<","<<pos.y()<<", mouse="<<x<<","<<y<<", moved="<<moved<<", test="<<isInside<<endl;
		setGrabsMouse(moved||isInside);
	}
	void mousePressEvent(QMouseEvent* const e, qglviewer::Camera* const){  prevPos=e->pos(); moved=true; }
   void mouseMoveEvent(QMouseEvent* const e, const qglviewer::Camera* const){
		if(!moved) return;
      pos+=e->pos()-prevPos; prevPos=e->pos();
	}
	void mouseReleaseEvent(QMouseEvent* const e, qglviewer::Camera* const c) { mouseMoveEvent(e,c); moved=false; }
	// drawing itself done in GLViewer::postDraw
	#if 0
	   void draw() {
	      // The object is drawn centered on its pos, with different possible aspects:
	      if (grabsMouse()){
	        if (moved)
	          // Object being moved, maybe a transparent display
	        else
	          // Object ready to be moved, maybe a highlighted visual feedback
	      else
	        // Normal display
	    }
	#endif
	 QPoint pos, prevPos;
	 QPoint dim;
	 bool moved;
};


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
		shared_ptr<Renderer> renderer;

	private:

		Vector2i prevSize; // used to move scales accordingly

		bool			isMoving;
		float			cut_plane;
		int			cut_plane_delta;
		bool			gridSubdivide;
		int manipulatedClipPlane;
		set<int> boundClipPlanes;
		shared_ptr<qglviewer::LocalConstraint> xyPlaneConstraint;
		string strBoundGroup(){string ret;FOREACH(int i, boundClipPlanes) ret+=" "+lexical_cast<string>(i+1);return ret;}
		boost::posix_time::ptime last_user_event;

     public:
		//virtual void updateGL(void);

		const int viewId;

		void centerMedianQuartile();
		int 	drawGrid;
		bool 	drawScale;
		int timeDispMask;
		enum{TIME_REAL=1,TIME_VIRT=2,TIME_ITER=4};

		GLViewer(int viewId, const shared_ptr<Renderer>& renderer, QGLWidget* shareWidget=0);
		virtual ~GLViewer();
		#if 0
			virtual void paintGL();
		#endif
		virtual void draw(){ draw(/*withNames*/false); }
		virtual void drawWithNames(){ draw(/*withNames*/true); }
		virtual void draw(bool withNames);
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
		//! Load display parameters (QGLViewer and Renderer) from Scene::dispParams[n] and use them
		void useDisplayParameters(size_t n, bool fromHandler=false);
		//! Save display parameters (QGOViewer and Renderer) to Scene::dispParams[n]
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
	protected:
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



