#ifdef WOO_OPENGL
#include<woo/lib/opengl/GLUtils.hpp>
#include<GL/glu.h>
#include<GL/freeglut_ext.h>
#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string/classification.hpp>

void GLUtils::Grid(const Vector3r& pos, const Vector3r& unitX, const Vector3r& unitY, const Vector2i& size, int edgeMask){
	//glPushAttrib(GL_ALL_ATTRIB_BITS);
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
			for(int x=(edgeMask&1?0:1); x<=size[0]-(edgeMask&2?0:1); x++){ glVertex3v(Vector3r(pos+x*unitX)); glVertex3v(Vector3r(pos+x*unitX+size[1]*unitY)); }
			for(int y=(edgeMask&4?0:1); y<=size[1]-(edgeMask&8?0:1); y++){ glVertex3v(Vector3r(pos+y*unitY)); glVertex3v(Vector3r(pos+y*unitY+size[0]*unitX)); }
		glEnd();
		glEnable(GL_LIGHTING);
	//glPopAttrib();
}

void GLUtils::Parallelepiped(const Vector3r& a, const Vector3r& b, const Vector3r& c){
   glBegin(GL_LINE_STRIP);
	 	glVertex3v(b); glVertex3v(Vector3r(Vector3r::Zero())); glVertex3v(a); glVertex3v(Vector3r(a+b)); glVertex3v(Vector3r(a+b+c)); glVertex3v(Vector3r(b+c)); glVertex3v(b); glVertex3v(Vector3r(a+b));
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex3v(Vector3r(b+c)); glVertex3v(c); glVertex3v(Vector3r(a+c)); glVertex3v(a);
	glEnd();
	glBegin(GL_LINES);
		glVertex3v(Vector3r(Vector3r::Zero())); glVertex3v(c);
	glEnd();
	glBegin(GL_LINES);
		glVertex3v(Vector3r(a+c)); glVertex3v(Vector3r(a+b+c));
	glEnd();
}

void GLUtils::AlignedBox(const AlignedBox3r& box, const Vector3r& color){
	glPushMatrix();
		if(!isnan(color[0])) glColor3v(color);
		glTranslatev(box.center().eval());
		glScalev(Vector3r(box.max()-box.min()));
		glDisable(GL_LINE_SMOOTH);
		glutWireCube(1);
		glEnable(GL_LINE_SMOOTH);
	glPopMatrix();
}

void GLUtils::AlignedBoxWithTicks(const AlignedBox3r& box, const Vector3r& stepLen, const Vector3r& tickLen, const Vector3r& color){
	AlignedBox(box,color);
	glColor3v(color);
	glBegin(GL_LINES);
		// for each axis (along which the sides go), there are found corners
		for(int ax:{0,1,2}){
			int ax1=(ax+1)%3, ax2=(ax+2)%3;
			// coordinates of the corner
			Real c1[]={box.min()[ax1],box.max()[ax1],box.min()[ax1],box.max()[ax1]};
			Real c2[]={box.min()[ax2],box.min()[ax2],box.max()[ax2],box.max()[ax2]};
			// tics direction (endpoint offsets from the point at the edge)
			Real dc1[]={tickLen[ax1],-tickLen[ax1],tickLen[ax1],-tickLen[ax1]};
			Real dc2[]={tickLen[ax2],tickLen[ax2],-tickLen[ax2],-tickLen[ax2]};
			for(int corner:{0,1,2,3}){
				Vector3r pt; pt[ax1]=c1[corner]; pt[ax2]=c2[corner];
				Vector3r dx1; dx1[ax]=0; dx1[ax1]=dc1[corner]; dx1[ax2]=0;
				Vector3r dx2; dx2[ax]=0; dx2[ax1]=0; dx2[ax2]=dc2[corner];
				for(pt[ax]=box.min()[ax]+stepLen[ax]; pt[ax]<box.max()[ax]; pt[ax]+=stepLen[ax]){
					glVertex3v(pt); glVertex3v((pt+dx1).eval());
					glVertex3v(pt); glVertex3v((pt+dx2).eval());
				}
			}
		}
	glEnd();
}


void GLUtils::RevolvedRectangle(const AlignedBox3r& box, const Vector3r& color, int div){
	Real p0(box.min()[1]); Real p1(box.max()[1]);
	const Real& r0(box.min()[0]); const Real& r1(box.max()[0]);
	const Real& z0(box.min()[2]); const Real& z1(box.max()[2]);
	bool boundary;
	if(abs(p0-p1)>=2*M_PI){ boundary=false; p0=0; p1=2*M_PI; }
	else{
		boundary=true;
		if(p0>p1) p0-=2*M_PI;
	}
	int nn=div*(p1-p0)/(2*M_PI);
	Real step=(p1-p0)/nn;
	glColor3v(color);
	for(const auto& rz:{Vector2r(r0,z0),Vector2r(r0,z1),Vector2r(r1,z0),Vector2r(r1,z1)}){
		glBegin(GL_LINE_STRIP);
			for(int n=0; n<=nn; n++){
				glVertex3v(CompUtils::cyl2cart(Vector3r(rz[0],p0+n*step,rz[1])));
			}
		glEnd();
	}
	if(boundary){
		for(const Real& p:{p0,p1}){
			glBegin(GL_LINE_LOOP);
				glVertex3v(CompUtils::cyl2cart(Vector3r(r0,p,z0)));
				glVertex3v(CompUtils::cyl2cart(Vector3r(r1,p,z0)));
				glVertex3v(CompUtils::cyl2cart(Vector3r(r1,p,z1)));
				glVertex3v(CompUtils::cyl2cart(Vector3r(r0,p,z1)));
			glEnd();
		}
	}
};


void GLUtils::Cylinder(const Vector3r& a, const Vector3r& b, Real rad1, const Vector3r& color, bool wire, bool caps, Real rad2 /* if negative, use rad1 */,int slices, int stacks){
	if(rad2<0) rad2=rad1;
	static GLUquadric* gluQuadric;
	static GLUquadric* gluDiskQuadric;
	if(!gluQuadric) gluQuadric=gluNewQuadric(); assert(gluQuadric);
	if(!gluDiskQuadric) gluDiskQuadric=gluNewQuadric(); assert(gluDiskQuadric);
	Real dist=(b-a).norm();
	glPushMatrix();
		glTranslatev(a);
		Quaternionr q(Quaternionr().setFromTwoVectors(Vector3r(0,0,1),(b-a)/dist /* normalized */));
		// using Transform with OpenGL: http://eigen.tuxfamily.org/dox/TutorialGeometry.html
		glMultMatrixd(Eigen::Affine3d(q).data());
		if(!isnan(color[0]))	glColor3v(color);
		gluQuadricDrawStyle(gluQuadric,wire?GLU_LINE:GLU_FILL);
		if(stacks<0) stacks=max(1,(int)(dist/(rad1*(-stacks/10.))+.5));
		gluCylinder(gluQuadric,rad1,rad2,dist,slices,stacks);
		if(caps){
			gluQuadricDrawStyle(gluDiskQuadric,wire?GLU_LINE:GLU_FILL);
			if(rad1>0) gluDisk(gluDiskQuadric,/*inner*/0,/*outer*/rad1,/*slices*/slices,/*loops*/3);
			if(rad2>0){ /* along local z axis*/ glTranslatev(Vector3r(0,0,(b-a).norm())); gluDisk(gluDiskQuadric,/*inner*/0,/*outer*/rad1,/*slices*/slices,/*loops*/3); }
		}
	glPopMatrix();
}

/****
 code copied over from qglviewer
****/

/*! Draws a 3D arrow along the positive Z axis.

\p length, \p radius and \p nbSubdivisions define its geometry. If \p radius is negative
(default), it is set to 0.05 * \p length.

Use drawArrow(const Vec& from, const Vec& to, float radius, int nbSubdivisions) or change the \c
ModelView matrix to place the arrow in 3D.

Uses current color and does not modify the OpenGL state. */
void GLUtils::QGLViewer::drawArrow(float length, float radius, int nbSubdivisions, bool doubled)
{
	static GLUquadric* quadric = gluNewQuadric();

	if (radius < 0.0)
		radius = 0.05 * length;

	const float head = 2.5*(radius / length) + 0.1;
	const float coneRadiusCoef = 4.0 - 5.0 * head;

	gluCylinder(quadric, radius, radius, length * (1.0 - head/coneRadiusCoef), nbSubdivisions, 1);
	glTranslatef(0.0, 0.0, length * (1.0 - head));
	gluCylinder(quadric, coneRadiusCoef * radius, 0.0, head * length, nbSubdivisions, 1);
	if(!doubled){
		glTranslatef(0.0, 0.0,-length*(1.0-head));
	} else {
		glTranslatef(0.0, 0.0,-length*(.3*head));
		gluCylinder(quadric, coneRadiusCoef * radius, 0.0, head * length, nbSubdivisions, 1);
		glTranslatef(0.0, 0.0,-length*(.7-head));
	}
}

/*! Draws a 3D arrow between the 3D point \p from and the 3D point \p to, both defined in the
current ModelView coordinates system.

See drawArrow(float length, float radius, int nbSubdivisions) for details. */
void GLUtils::QGLViewer::drawArrow(const Vector3r& from, const Vector3r& to, float radius, int nbSubdivisions, bool doubled)
{
	glPushMatrix();
		GLUtils::setLocalCoords(from,Quaternionr().setFromTwoVectors(Vector3r(0,0,1),to-from));
		drawArrow((to-from).norm(), radius, nbSubdivisions, doubled);
	glPopMatrix();
}

void GLUtils::GLDrawText(const std::string& txt, const Vector3r& pos, const Vector3r& color, bool center, void* font, const Vector3r& bgColor, bool shiftIfNeg){
	font=font?font:GLUT_BITMAP_8_BY_13;
	Vector2i xyOff=center?Vector2i(-glutBitmapLength(font,(unsigned char*)txt.c_str())/2,glutBitmapHeight(font)/2):Vector2i::Zero();
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
		// glut will drop any text which starts at negative coordinate; compensate for that here optionally
		if(!shiftIfNeg || ((pos[0]+xyOff[0]>0) && (pos[1]+xyOff[1]>0))) glTranslatev(pos);
		else{ glTranslatev(Vector3r(max(pos[0],-1.*xyOff[0]),max(pos[1],-1.*xyOff[1]),pos[2])); }
	 	// copied from http://libqglviewer.sourcearchive.com/documentation/2.3.1-3/qglviewer_8cpp-source.html
      glDisable(GL_LIGHTING);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(1.0);

		if(bgColor[0]>=0){ // draw multiple copies with background color
			glColor3v(bgColor);
			int pos0[][2]={{-1,-1},{1,1},{1,-1},{-1,1}};
			for(int i=0; i<4; i++){
				glRasterPos2i(pos0[i][0]+xyOff[0],pos0[i][1]+xyOff[1]);
				glutBitmapString(font,(unsigned char*)txt.c_str());
			}
		}
		glColor3v(color);
		glRasterPos2i(xyOff[0],xyOff[1]);
		glutBitmapString(font,(unsigned char*)txt.c_str());
	glPopAttrib();
	glPopMatrix();
}
#endif
