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

#ifndef __GLVIEWER_H__
#define __GLVIEWER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <QGLViewer/qglviewer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "FpsTracker.hpp"
#include "NonConnexBody.hpp"
#include "QGLThread.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class GLViewer : public QGLViewer
{	
	// construction
	public : GLViewer (QWidget * parent=0, QGLWidget * shareWidget=0);
	public : ~GLViewer ();

	private : QGLThread qglThread;
	friend class QGLThread;
	
	protected : void resizeEvent(QResizeEvent *evt);
	protected : void paintEvent(QPaintEvent *);
	protected : void closeEvent(QCloseEvent *evt);

	public : void paintGL()
	{
		//ThreadSafe::cerr("painGL");
	}
	
	public slots: void updateGL()
	{
		//ThreadSafe::cerr("updateGL");
	}
	
// 	private : shared_ptr<FpsTracker> fpsTracker;

// 	protected : void mouseMoveEvent(QMouseEvent * e);
// 	protected : void mousePressEvent(QMouseEvent *e);
// 	protected : void mouseReleaseEvent(QMouseEvent *e);
// 	protected : void keyPressEvent(QKeyEvent *e);
// 	protected : void mouseDoubleClickEvent(QMouseEvent *e);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __GLVIEWER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
