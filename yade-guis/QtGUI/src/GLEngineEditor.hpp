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

#ifndef __RELATIONSVIEWER_H__
#define __RELATIONSVIEWER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <QGLViewer/qglviewer.h>

#include <set>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <yade/yade-lib-opengl/GLWindowsManager.hpp>
#include <yade/yade-lib-opengl/GLTextLabel.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

class GLEngineEditor : public QGLViewer
{
	Q_OBJECT


	private :	struct lessThanPair
			{
				bool operator()(const pair<int,int>& p1, const pair<int,int>& p2)
				{
					return (p1.first<p2.first || p1.first==p2.first && p1.second<p2.second);
				}
			};
				
	private : set<pair<int,int>, lessThanPair > loop;

	private : GLWindowsManager wm;
	
	private : bool startRelation;
	private : bool relationSelected;
	private : std::pair<int,int> selectedRelation;
	private : std::pair<int,int> relation;
	private : int startX;
	private : int startY;
	private : int endX;
	private : int endY;

	private : void drawGrid();
	private : void drawArrow(int x1,int y1, int x2, int y2);
	private : void cutSegmentWithRectangles(int& x1,int& y1,int& x2,int& y2, int label1, int label2);
	private : bool selectRelation(int x, int y, float threshold, int &a, int &b);

	// construction
	public : GLEngineEditor(QWidget * parent=0, const char * name=0);
	public : ~GLEngineEditor();
	
	public : void draw();
	
	protected : void mouseMoveEvent(QMouseEvent * e);
	protected : void mousePressEvent(QMouseEvent *e);
	protected : void mouseReleaseEvent(QMouseEvent *e);
	protected : void mouseDoubleClickEvent(QMouseEvent *e);

	protected : void keyPressEvent(QKeyEvent *e);
	protected : void resizeEvent ( QResizeEvent * e);

	public : void newFractureDeleted(int i);

	public : void updateLabel(int i);
	
	public : int getWindowMinX(int i) { return wm.getWindow(i)->getMinX(); };
	public : int getWindowMinY(int i) { return wm.getWindow(i)->getMinY(); };

	public : void addEngine(const string& engineName );
	
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // __RELATIONSVIEWER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
