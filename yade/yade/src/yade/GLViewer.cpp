#include "QtFrontEnd.hpp"
#include "Body.hpp"
#include "Interaction.hpp"
#include "GL/glut.h"

GLViewer::GLViewer(QWidget * parent) : QGLViewer(parent)
{
	resize(300,300);
	setSceneCenter(0,0,0);
	setSceneRadius(200);
	showEntireScene();
	setAnimationPeriod(0);
	fpsTracker = shared_ptr<FpsTracker>(new FpsTracker(this));
	frame = 0;

}

GLViewer::~GLViewer()
{

}

void GLViewer::draw()
{
        glEnable(GL_NORMALIZE);
        glEnable(GL_CULL_FACE);
	
	if (Omega::instance().rootBody!=0) // if the scene is loaded
		Omega::instance().rootBody->glDraw();

// 	if (frame%50==0)
// 	{		
// 		string name = "/disc/pictures/pic";
// 		setSnapshotFilename(name);
// 		saveSnapshot(true,false);
// 	}
	frame++;
		
	fpsTracker->glDraw();
}

void GLViewer::animate()
{
	Omega::instance().rootBody->moveToNextTimeStep(0.04);
	//cerr << frame << endl;
	fpsTracker->addOneAction();
}

void GLViewer::mouseMoveEvent(QMouseEvent * e)
{
	if (!fpsTracker->mouseMoveEvent(e))
		QGLViewer::mouseMoveEvent(e);
}

void GLViewer::mousePressEvent(QMouseEvent *e)
{
	if (!fpsTracker->mousePressEvent(e))
		QGLViewer::mousePressEvent(e);
}

void GLViewer::mouseReleaseEvent(QMouseEvent *e)
{
	if (!fpsTracker->mouseReleaseEvent(e))	
		QGLViewer::mouseReleaseEvent(e);
}

void GLViewer::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (!fpsTracker->mouseDoubleClickEvent(e))	
		QGLViewer::mouseDoubleClickEvent(e);
}


void GLViewer::keyPressEvent(QKeyEvent *e)
{
	if (e->key()=='f' || e->key()=='F')
		fpsTracker->swapDisplayed();
	else
		QGLViewer::keyPressEvent(e);
}
