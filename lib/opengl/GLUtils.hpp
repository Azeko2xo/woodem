// © 2008 Václav Šmilauer <eudoxos@arcig.cz>
//
// header-only utility functions for GL (moved over from extra/Shop.cpp)
//
#pragma once

#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<boost/lexical_cast.hpp>
#include<sstream>
#include<iomanip>
#include<string>

struct GLUtils{
	// code copied from qglviewer
	struct QGLViewer{
		static void drawArrow(float length=1.0f, float radius=-1.0f, int nbSubdivisions=12);
		static void drawArrow(const Vector3r& from, const Vector3r& to, float radius=-1.0f, int nbSubdivisions=12);
	};
	// render wire of parallelepiped with sides given by vectors a,b,c; zero corner is at origin
	static void Parallelepiped(const Vector3r& a, const Vector3r& b, const Vector3r& c);
	static void AlignedBox(const AlignedBox3r& box, const Vector3r& color=Vector3r(1,1,1));
	// render cylinder, wire or solid
	// if rad2<0, rad1 is used;
	// if stacks<0, then it is approximate stack length (axial subdivision) relative to rad1, multiplied by 10 (i.e. -5 -> stacks approximately .5*rad1)
	static void Cylinder(const Vector3r& a, const Vector3r& b, Real rad1, const Vector3r& color, bool wire=false, bool caps=false, Real rad2=-1 /* if negative, use rad1 */, int slices=6, int stacks=-10);

	// draw parallelepipedic grid with lines starting at pos
	static void Grid(const Vector3r& pos, const Vector3r& unitX, const Vector3r& unitY, const Vector2i& size, int edgeMask=15);

	static void GLDrawArrow(const Vector3r& from, const Vector3r& to, const Vector3r& color=Vector3r(1,1,1)){
		glEnable(GL_LIGHTING); glColor3v(color); QGLViewer::drawArrow(from,to);	
	}
	static void GLDrawLine(const Vector3r& from, const Vector3r& to, const Vector3r& color=Vector3r(1,1,1), int width=-1){
		glEnable(GL_LIGHTING); glColor3v(color);
		if(width>0) glLineWidth(width);
		glBegin(GL_LINES); glVertex3v(from); glVertex3v(to); glEnd();
	}

	static void GLDrawNum(const Real& n, const Vector3r& pos, const Vector3r& color=Vector3r(1,1,1), unsigned precision=3){
		std::ostringstream oss; oss.precision(precision); oss<< /* "w="<< */ n;
		GLUtils::GLDrawText(oss.str(),pos,color);
	}

	static void GLDrawInt(long i, const Vector3r& pos, const Vector3r& color=Vector3r(1,1,1)){
		GLUtils::GLDrawText(boost::lexical_cast<std::string>(i),pos,color);
	}

	static void GLDrawText(const std::string& txt, const Vector3r& pos, const Vector3r& color=Vector3r(1,1,1), bool center=false, void* font=NULL, const Vector3r& bgColor=Vector3r(-1,-1,-1), bool shiftIfNeg=false);

	static void setLocalCoords(const Vector3r& pos, const Quaternionr& ori){
		AngleAxisr aa(ori);
		glTranslatef(pos[0],pos[1],pos[2]);
		glRotatef(aa.angle()*Mathr::RAD_TO_DEG,aa.axis()[0],aa.axis()[1],aa.axis()[2]);
	};
};
