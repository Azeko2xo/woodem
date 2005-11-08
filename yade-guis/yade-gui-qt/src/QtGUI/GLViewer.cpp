/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "GLViewer.hpp"
#include <GL/glut.h>
#include <yade/yade-lib-threads/ThreadSynchronizer.hpp>
#include <yade/yade-lib-opengl/FpsTracker.hpp>
#include <yade/yade-core/Body.hpp>
#include <yade/yade-core/Interaction.hpp>
#include <yade/yade-core/Omega.hpp>


GLViewer::GLViewer(int id, shared_ptr<RenderingEngine> renderer, const QGLFormat& format, QWidget * parent, QGLWidget * shareWidget) : QGLViewer(format,parent,"glview",shareWidget), qglThread(this,renderer)
{

	viewId = id;
	setAutoBufferSwap(false);
	resize(320, 240);

	if (id==0)
		setCaption("Primary View (not closable)");
	else
		setCaption("Secondary View number "+lexical_cast<string>(id));
	show();

	wm.addWindow(shared_ptr<GLWindow>(new FpsTracker()),shared_ptr<GLWindowsManager::EventSubscription>(new GLWindowsManager::EventSubscription()));
	
}


GLViewer::~GLViewer()
{
	
}


void GLViewer::initializeGL()
{
	QGLViewer::initializeGL();
	qglThread.initializeGL();
	glClearColor(0,0,0,0);
}


void GLViewer::finishRendering()
{
	qglThread.finish();
}


void GLViewer::joinRendering()
{
	qglThread.join();
}


void GLViewer::stopRendering()
{
	qglThread.stop();
}


void GLViewer::startRendering()
{	
	qglThread.start();
}


void GLViewer::centerScene()
{
	qglThread.centerScene();
}
	

void GLViewer::paintGL()
{
	// Handled by the GLThread.
}


void GLViewer::updateGL()
{
	// Handled by the GLThread.
}


void GLViewer::resizeEvent(QResizeEvent *evt)
{
	qglThread.resize(evt->size().width(),evt->size().height()); 
}


void GLViewer::paintEvent(QPaintEvent *)
{
	// Handled by the GLThread.
}


void GLViewer::closeEvent(QCloseEvent *)
{
	emit closeSignal(viewId);
}


void GLViewer::mouseMoveEvent(QMouseEvent * e)
{
	if (wm.mouseMoveEvent(e->x(),e->y())==-1)
		QGLViewer::mouseMoveEvent(e);
}


void GLViewer::mousePressEvent(QMouseEvent *e)
{
	if (wm.mousePressEvent(e->x(),e->y())==-1)
		QGLViewer::mousePressEvent(e);
}


void GLViewer::mouseReleaseEvent(QMouseEvent *e)
{
	if (wm.mouseReleaseEvent(e->x(),e->y())==-1)
		QGLViewer::mouseReleaseEvent(e);
}


void GLViewer::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (wm.mouseDoubleClickEvent(e->x(),e->y())==-1)
		QGLViewer::mouseDoubleClickEvent(e);
}


void GLViewer::keyPressEvent(QKeyEvent *e)
{
	if (e->key()=='F' || e->key()=='f')
		wm.getWindow(0)->swapDisplayed();
	else
		QGLViewer::keyPressEvent(e);
}

