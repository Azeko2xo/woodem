#include "GLViewer.hpp"
#include "Body.hpp"
#include "Interaction.hpp"
#include "GL/glut.h"
#include "Omega.hpp"

GLViewer::GLViewer(QWidget * parent) : QGLViewer(parent) , frame( Omega::instance().getIterReference() )
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

	if (Omega::instance().rootBody) // if the scene is loaded
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
	Omega::instance().rootBody->moveToNextTimeStep();

// 	static bool progress = Omega::instance().getProgress();
// 	static long int max  = Omega::instance().getMaxiter();
// 
// 	if( frame % 100 == 0 )					// checks every 100th iteration
// 	{
// 		if(progress)
// 			cout << "iteration: " << frame << endl;
// 		if( max != 0 )
// 			if( frame > max )
// 			{
// 				cerr << "Calc finished at: " << frame << endl;
// 				exit(0);			// terminate.
// 			}
// 	}

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
