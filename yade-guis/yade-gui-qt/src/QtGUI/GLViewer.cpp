/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2005 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "GLViewer.hpp"
#include <GL/glut.h>
#include <yade/yade-lib-opengl/FpsTracker.hpp>
#include <yade/yade-core/Body.hpp>
#include <yade/yade-core/Interaction.hpp>
#include <yade/yade-core/Omega.hpp>


GLViewer::GLViewer(int id, shared_ptr<RenderingEngine> rendererInit, const QGLFormat& format, QWidget * parent, QGLWidget * shareWidget) : QGLViewer(format,parent,"glview",shareWidget)//, qglThread(this,rendererInit)
{
	isMoving=false;
	renderer=rendererInit;
	drawGrid = false;
	viewId = id;
	resize(320, 240);

	if (id==0)
		setCaption("Primary View (not closable)");
	else
		setCaption("Secondary View number "+lexical_cast<string>(id));
	show();
	
	notMoving();

	if(manipulatedFrame() == 0 )
		setManipulatedFrame(new qglviewer::ManipulatedFrame());
}

void GLViewer::notMoving()
{
	camera()->frame()->setWheelSensitivity(-1.0f);
	setMouseBinding(Qt::LeftButton + Qt::RightButton, CAMERA, ZOOM);
	setMouseBinding(Qt::LeftButton, CAMERA, ROTATE);
	setMouseBinding(Qt::MidButton, CAMERA, TRANSLATE);
	setMouseBinding(Qt::RightButton, CAMERA, TRANSLATE);
	setWheelBinding(Qt::NoButton, CAMERA, ZOOM);
	setMouseBinding(Qt::SHIFT + Qt::LeftButton, SELECT);
	//setMouseBinding(Qt::RightButton, NO_CLICK_ACTION);

	setMouseBinding(Qt::SHIFT + Qt::LeftButton + Qt::RightButton, FRAME, ZOOM);
	setMouseBinding(Qt::SHIFT + Qt::MidButton, FRAME, TRANSLATE);
	setMouseBinding(Qt::SHIFT + Qt::RightButton, FRAME, ROTATE);
	setWheelBinding(Qt::ShiftButton , FRAME, ZOOM);
};

GLViewer::~GLViewer()
{
//	std::cerr << "GLViewer dtor:" << viewId << "\n";
}


void GLViewer::keyPressEvent(QKeyEvent *e)
{
	if ( e->key()==Qt::Key_M )
		if( !(isMoving = !isMoving ) )
		{
			displayMessage("moving finished");
			notMoving();
		}
		else
		{
			displayMessage("moving selected object");
			setMouseBinding(Qt::LeftButton + Qt::RightButton, FRAME, ZOOM);
			setMouseBinding(Qt::LeftButton, FRAME, TRANSLATE);
			setMouseBinding(Qt::MidButton, FRAME, TRANSLATE);
			setMouseBinding(Qt::RightButton, FRAME, ROTATE);
			setWheelBinding(Qt::NoButton , FRAME, ZOOM);
		}
	else if( e->key()==Qt::Key_G )
		drawGrid = !drawGrid;
	else if( e->key()!=Qt::Key_Escape && e->key()!=Qt::Key_Space )
		QGLViewer::keyPressEvent(e);
}

void GLViewer::centerScene()
{
	if (!Omega::instance().getRootBody())
		return;

	if(Omega::instance().getRootBody()->bodies->size() < 500)
		displayMessage("Less than 500 bodies, moving possible. Select with shift, press 'm' to move.", 6000);
	else
		displayMessage("More than 500 bodies. Moving not possible", 6000);

	Vector3r min = Omega::instance().getRootBody()->boundingVolume->min;
	Vector3r max = Omega::instance().getRootBody()->boundingVolume->max;
	Vector3r center = (max+min)*0.5;
	Vector3r halfSize = (max-min)*0.5;
	float radius = std::max(halfSize[0] , std::max(halfSize[1] , halfSize[2]) );
	setSceneCenter(qglviewer::Vec(center[0],center[1],center[2]));
	setSceneRadius(radius*1.5);
	showEntireScene();
}

void GLViewer::draw() // FIXME maybe rename to RendererFlowControl, or something like that?
{
	if(Omega::instance().getRootBody())
	{
		int selection = selectedName();
		if(selection != -1 && (*(Omega::instance().getRootBody()->bodies)).exists(selection) )
		{
			Quaternionr& q = (*(Omega::instance().getRootBody()->bodies))[selection]->physicalParameters->se3.orientation;
			Vector3r&    v = (*(Omega::instance().getRootBody()->bodies))[selection]->physicalParameters->se3.position;
			float v0,v1,v2;
			manipulatedFrame()->getPosition(v0,v1,v2);
			v[0]=v0;v[1]=v1;v[2]=v2;
			double q0,q1,q2,q3;
			manipulatedFrame()->getOrientation(q0,q1,q2,q3);
			q[0]=q0;q[1]=q1;q[2]=q2;q[3]=q3;
		}
		
	// FIXME - here we want to actually call all responsible GLDraw Actors
		renderer->render(Omega::instance().getRootBody(), selectedName());
	}
}

void GLViewer::drawWithNames() // FIXME maybe rename to RendererFlowControl, or something like that?
{
	if (Omega::instance().getRootBody() && Omega::instance().getRootBody()->bodies->size() < 500 )
	// FIXME - here we want to actually call all responsible GLDraw Actors
		renderer->renderWithNames(Omega::instance().getRootBody());
}

// new object selected.
// set frame coordinates, and isDynamic=false;
void GLViewer::postSelection(const QPoint& point) 
{
	int selection = selectedName();
	if(selection == -1)
	{
		if(isMoving)
		{
			displayMessage("moving finished");
			notMoving();
			isMoving=false;
		}
		return;
	}
	if( (*(Omega::instance().getRootBody()->bodies)).exists(selection) )
	{
		wasDynamic = (*(Omega::instance().getRootBody()->bodies))[selection]->isDynamic;
		(*(Omega::instance().getRootBody()->bodies))[selection]->isDynamic = false;

		Quaternionr& q = (*(Omega::instance().getRootBody()->bodies))[selection]->physicalParameters->se3.orientation;
		Vector3r&    v = (*(Omega::instance().getRootBody()->bodies))[selection]->physicalParameters->se3.position;
		manipulatedFrame()->setPositionAndOrientation(qglviewer::Vec(v[0],v[1],v[2]),qglviewer::Quaternion(q[0],q[1],q[2],q[3]));
	}

}

// maybe new object will be selected.
// if so, then set isDynamic of previous selection, to old value
void GLViewer::endSelection(const QPoint &point)
{
	int old = selectedName();

	QGLViewer::endSelection(point);

	if(old != -1 && old!=selectedName() && (*(Omega::instance().getRootBody()->bodies)).exists(old) )
		(*(Omega::instance().getRootBody()->bodies))[old]->isDynamic = wasDynamic;
}

void GLViewer::postDraw()
{
	if( drawGrid ) // FIXME drawGrid is yet another RendererFlowControl's Actor.
	{
//		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		qglColor(foregroundColor());
		glDisable(GL_LIGHTING);
		glLineWidth(0.1);
		glBegin(GL_LINES);

		float sizef = QGLViewer::camera()->sceneRadius()*3.0f; 
		int size = static_cast<int>(sizef);
		qglviewer::Vec v = QGLViewer::camera()->sceneCenter();
		int x = static_cast<int>(v[0]); int y = static_cast<int>(v[1]);
		float xf = (static_cast<int>(v[0]*100.0))/100.0;
		float yf = (static_cast<int>(v[1]*100.0))/100.0;
//		float nbSubdivisions = size;
//		for (int i=0; i<=nbSubdivisions; ++i)
		for (int i= -size ; i<=size; ++i )
		{
//			const float pos = size*(2.0*i/nbSubdivisions-1.0);
			glVertex2i( i   +x, -size+y);
			glVertex2i( i   +x, +size+y);
			glVertex2i(-size+x, i    +y);
			glVertex2i( size+x, i    +y);
		}
		if(sizef <= 2.0)
		{
			glColor3f(0.9,0.9,0.9);
			for (float i= -(static_cast<int>(sizef*100.0))/100.0 ; i<=sizef; i+=0.01 )
			{
				glVertex2f( i    +xf, -sizef+yf);
				glVertex2f( i    +xf, +sizef+yf);
				glVertex2f(-sizef+xf, i     +yf);
				glVertex2f( sizef+xf, i     +yf);
			}
		}
		
		glEnd();
		glPopAttrib();
		glPopMatrix();
	}
	QGLViewer::postDraw();
}

void GLViewer::closeEvent(QCloseEvent *e)
{
	emit closeSignal(viewId);
}

