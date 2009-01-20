// © 2008 Václav Šmilauer <eudoxos@arcig.cz>
//
// header-only utility functions for GL (moved over from extra/Shop.cpp)
//
#pragma once

#include<yade/lib-opengl/OpenGLWrapper.hpp>
#include<yade/lib-QGLViewer/qglviewer.h>
#include<sstream>
#include<iomanip>
#include<string>

struct GLUtils{
	static void GLDrawArrow(const Vector3r& from, const Vector3r& to, const Vector3r& color=Vector3r(1,1,1)){
		glEnable(GL_LIGHTING); glColor3v(color); qglviewer::Vec a(from[0],from[1],from[2]),b(to[0],to[1],to[2]); QGLViewer::drawArrow(a,b);	
	}
	static void GLDrawLine(const Vector3r& from, const Vector3r& to, const Vector3r& color=Vector3r(1,1,1)){
		glEnable(GL_LIGHTING); glColor3v(color);
		glBegin(GL_LINES); glVertex3v(from); glVertex3v(to); glEnd();
	}

	static void GLDrawNum(const Real& n, const Vector3r& pos, const Vector3r& color=Vector3r(1,1,1), unsigned precision=3){
		std::ostringstream oss; oss<<std::setprecision(precision)<< /* "w="<< */ (double)n;
		GLUtils::GLDrawText(oss.str(),pos,color);
	}

	static void GLDrawText(const std::string& txt, const Vector3r& pos, const Vector3r& color=Vector3r(1,1,1)){
		glPushMatrix();
		glTranslatev(pos);
		glColor3(color[0],color[1],color[2]);
		glRasterPos2i(0,0);
		for(unsigned int i=0;i<txt.length();i++)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, txt[i]);
		glPopMatrix();
	}
};
