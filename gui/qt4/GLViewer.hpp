// Copyright (C) 2004 by Olivier Galizzi, Janek Kozicki                  *
// © 2008 Václav Šmilauer
#pragma once

#ifdef WOO_OPENGL

#include<woo/pkg/gl/Renderer.hpp>

#include<QGLViewer/qglviewer.h>
#include<QGLViewer/constraint.h>
#include<QGLViewer/manipulatedFrame.h>
#include<QGLViewer/manipulatedCameraFrame.h>
#include<QGLViewer/mouseGrabber.h>

#include<QMouseEvent>

#include<boost/date_time/posix_time/posix_time.hpp>
#include<set>


#include<woo/core/Engine.hpp>



/*****************************************************************************
*********************************** SnapshotEngine ***************************
*****************************************************************************/
struct SnapshotEngine: public PeriodicEngine{
	virtual void run() WOO_CXX11_OVERRIDE;
	virtual bool needsField() WOO_CXX11_OVERRIDE { return false; }
	virtual void pyHandleCustomCtorArgs(py::tuple& t, py::dict& d) WOO_CXX11_OVERRIDE;
	WOO_CLASS_BASE_DOC_ATTRS(SnapshotEngine,PeriodicEngine,"Periodically save snapshots of GLView(s) as .png files. Files are named :obj:`fileBase` + :obj:`counter` + ``.png`` (counter is left-padded by 0s, i.e. snap00004.png).",
		((string,fileBase,"",,"Basename for snapshots"))
		((string,format,"PNG",AttrTrait<>().choice({"JPEG","PNG","EPS","PS","PPM","BMP"}),"Format of snapshots (one of JPEG, PNG, EPS, PS, PPM, BMP) `QGLViewer documentation <http://www.libqglviewer.com/refManual/classQGLViewer.html#abbb1add55632dced395e2f1b78ef491c>`_. File extension will be lowercased :obj:`format`. Validity of format is not checked."))
		((int,counter,0,AttrTrait<Attr::readonly>(),"Number that will be appended to fileBase when the next snapshot is saved (incremented at every save)."))
		((string,plot,,,"Name of field in :obj:`woo.core.Plot.imgData` to which taken snapshots will be appended automatically."))
		((vector<string>,snapshots,,AttrTrait<>().startGroup("Saved files").readonly().noGui(),"Files that have been created so far"))
		((bool,ignoreErrors,true,AttrTrait<>().startGroup("Error handling"),"Only report errors instead of throwing exceptions, in case of timeouts."))
		((int,msecSleep,0,,"number of msec to sleep after snapshot (to prevent 3d hw problems) [ms]"))
		((Real,deadTimeout,3,,"Timeout for 3d operations (opening new view, saving snapshot); after timing out, throw exception (or only report error if :obj:`ignoreErrors`) and make myself :obj:`dead <Engine.dead>`. [s]"))
		((bool,tryOpenView,false,,"Attempt to open new view if there is none; this is very unreliable (off by default)."))
	);
	WOO_DECL_LOGGER;
};
WOO_REGISTER_OBJECT(SnapshotEngine);

// for movable colorscales
// mostly copied from
// http://www.libqglviewer.com/refManual/classqglviewer_1_1MouseGrabber.html#_details
struct QglMovableObject: public qglviewer::MouseGrabber{
	QglMovableObject(int x0, int y0): qglviewer::MouseGrabber(), pos(x0,y0), moved(false), reset(false), dL(0){}
	void checkIfGrabsMouse(int x, int y, const qglviewer::Camera* const){
		QPoint relPos(QPoint(x,y)-pos);
		bool isInside=(relPos.x()>=0 && relPos.y()>=0 && relPos.x()<=dim.x() && relPos.y()<=dim.y());
		// cerr<<"relPos="<<relPos.x()<<","<<relPos.y()<<", dim="<<dim.x()<<","<<dim.y()<<", pos="<<pos.x()<<","<<pos.y()<<", mouse="<<x<<","<<y<<", moved="<<moved<<", test="<<isInside<<endl;
		setGrabsMouse(moved||isInside);
	}
	void mousePressEvent(QMouseEvent* const e, qglviewer::Camera* const){
		if(e->button()==Qt::RightButton){
			reset=true;
		} else {
			prevPos=e->pos(); moved=true;
		}
	}
	void wheelEvent(QWheelEvent* const e, qglviewer::Camera* const camera){
		//QPoint numDeg=e->angleDelta()/8.; // qt5
		int dist=e->delta();
		dL=dist;
		e->accept();
	}
   void mouseMoveEvent(QMouseEvent* const e, qglviewer::Camera* const camera){
		if(!moved) return;
      pos+=e->pos()-prevPos; prevPos=e->pos();
	}
	//void mouseDoubleClickEvent(QMouseEvent* const e, qglviewer::Camera* const){ reset=true; cerr<<"@@"; }
	void mouseReleaseEvent(QMouseEvent* const e, qglviewer::Camera* const c) { mouseMoveEvent(e,c); moved=false; }
	// drawing itself done in GLViewer::postDraw

	 QPoint pos, prevPos;
	 QPoint dim;
	 bool moved, reset;
	 int dL;
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
 * Clip plane number is 3; change WOO_RENDERER_NUM_CLIP_PLANE, complete switches "|| ..." in keyPressEvent
 * and recompile to have more.
 */
class GLViewer : public QGLViewer
{	
	WOO_NOWARN_OVERRIDE_PUSH
		Q_OBJECT 
	WOO_NOWARN_OVERRIDE_POP
	
	friend class QGLThread;
	private:
		Vector2i prevSize; // used to move scales accordingly
		QPoint pressPos; // remember where the last button was pressed, to freeze the cursor

		bool			isMoving;
		float			cut_plane;
		int			cut_plane_delta;
		bool			gridSubdivide;
		int manipulatedClipPlane;
		set<int> boundClipPlanes;
		shared_ptr<qglviewer::LocalConstraint> xyPlaneConstraint;
		string strBoundGroup(){string ret; for(int i: boundClipPlanes) ret+=" "+lexical_cast<string>(i+1);return ret;}
		// set initial view as specified by Renderer::iniViewDir and friends
		void setInitialView();
		// boost::posix_time::ptime last_user_event;

     public:
		//virtual void updateGL(void);

		const int viewId;
		// public because of _GLViewer which exposes this to python
		// shared by all instances
		static bool rotCursorFreeze;
		static bool paraviewLike3d;
		long framesDone;

		void centerMedianQuartile();
		GLViewer(int viewId, QGLWidget* shareWidget=0);
		virtual ~GLViewer();
		#if 0
			virtual void paintGL();
		#endif
		// do fastDraw when regular (non-fast) rendering is slower than max FPS
		// if rendering is faster, draw normally, sicne manipulation will not be slown down
		void fastDraw() WOO_CXX11_OVERRIDE { draw(/*withNames*/false,/*fast*/!(Renderer::renderTime<(1./Renderer::maxFps))
		);}
		void draw() WOO_CXX11_OVERRIDE { draw(/*withNames*/false,
			// /*fast*/(!hasFocus() && ((currentFPS()<.9*Renderer::maxFps && framesDone>100))));
			/*fast*/(!hasFocus() && Renderer::renderTime>.9*(1./Renderer::maxFps)) && framesDone>100);
		}
		void drawWithNames() WOO_CXX11_OVERRIDE { draw(/*withNames*/true); }
		// this one is not virtual
		void draw(bool withNames, bool fast=false);
		void displayMessage(const std::string& s, int delay=2000){ QGLViewer::displayMessage(QString(s.c_str()),delay);}
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
		QDomElement domElement(const QString& name, QDomDocument& document) const  WOO_CXX11_OVERRIDE;
		//! Adds our attributes to the QGLViewer state that can be restored
		void initFromDOMElement(const QDomElement& element) WOO_CXX11_OVERRIDE;

		// if defined, snapshot will be saved to this file right after being drawn and the string will be reset.
		// this way the caller will be notified of the frame being saved successfully.
		string nextSnapFile;
		bool nextSnapMsg; // whether there will be message informing where the snapshot was saved; disabled from SnapshotEngine, and recovered after every snapshot

		// boost::posix_time::ptime getLastUserEvent();

		// called from the init routine
		// http://stackoverflow.com/a/20425778/761090
		GLuint logoTextureId;
		QImage loadTexture(const char *filename, GLuint &textureID);


		WOO_DECL_LOGGER;
	protected:
		virtual void keyPressEvent(QKeyEvent *e) WOO_CXX11_OVERRIDE;
		virtual void postDraw() WOO_CXX11_OVERRIDE;
		// overridden in the player that doesn't get time from system clock but from the db
		virtual string getRealTimeString();
		virtual void closeEvent(QCloseEvent *e) WOO_CXX11_OVERRIDE;
		virtual void postSelection(const QPoint& point) WOO_CXX11_OVERRIDE;
		virtual void endSelection(const QPoint &point) WOO_CXX11_OVERRIDE;
		virtual void mouseDoubleClickEvent(QMouseEvent *e) WOO_CXX11_OVERRIDE;
		virtual void wheelEvent(QWheelEvent* e) WOO_CXX11_OVERRIDE;
		// hijacked to optionally freeze cursor when rotating
		virtual void mouseMoveEvent(QMouseEvent *e) WOO_CXX11_OVERRIDE;
		virtual void mousePressEvent(QMouseEvent *e) WOO_CXX11_OVERRIDE;
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


#endif /* WOO_OPENGL */
