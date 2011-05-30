#include"GLUtils.hpp"
#include<GL/glu.h>

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

void GLUtils::Cylinder(const Vector3r& a, const Vector3r& b, Real rad1, const Vector3r& color, bool wire, Real rad2 /* if negative, use rad1 */,int slices, int stacks){
	if(rad2<0) rad2=rad1;
	static GLUquadric* gluQuadric;
	if(!gluQuadric) gluQuadric=gluNewQuadric(); assert(gluQuadric);
	Real dist=(b-a).norm();
	glPushMatrix();
		glTranslatev(a);
		Quaternionr q(Quaternionr().setFromTwoVectors(Vector3r(0,0,1),(b-a)/dist /* normalized */));
		// using Transform with OpenGL: http://eigen.tuxfamily.org/dox/TutorialGeometry.html
		#if EIGEN_MAJOR_VERSION<30              //Eigen3 definition, while it is not realized
			glMultMatrixd(Eigen::Affine3d(q).data());
		#else
			glMultMatrixd(Eigen::Affine3d(q).data());
		#endif
		glColor3v(color);
		gluQuadricDrawStyle(gluQuadric,wire?GLU_LINE:GLU_FILL);
		if(stacks<0) stacks=(int)(dist/(rad1*(-stacks/10.))+.5);
		gluCylinder(gluQuadric,rad1,rad2,dist,slices,stacks);
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
void GLUtils::QGLViewer::drawArrow(float length, float radius, int nbSubdivisions)
{
	static GLUquadric* quadric = gluNewQuadric();

	if (radius < 0.0)
		radius = 0.05 * length;

	const float head = 2.5*(radius / length) + 0.1;
	const float coneRadiusCoef = 4.0 - 5.0 * head;

	gluCylinder(quadric, radius, radius, length * (1.0 - head/coneRadiusCoef), nbSubdivisions, 1);
	glTranslatef(0.0, 0.0, length * (1.0 - head));
	gluCylinder(quadric, coneRadiusCoef * radius, 0.0, head * length, nbSubdivisions, 1);
	glTranslatef(0.0, 0.0, -length * (1.0 - head));
}

/*! Draws a 3D arrow between the 3D point \p from and the 3D point \p to, both defined in the
current ModelView coordinates system.

See drawArrow(float length, float radius, int nbSubdivisions) for details. */
void GLUtils::QGLViewer::drawArrow(const Vector3r& from, const Vector3r& to, float radius, int nbSubdivisions)
{
	glPushMatrix();
	glTranslatef(from[0],from[1],from[2]);
	Quaternionr q(Quaternionr().setFromTwoVectors(Vector3r(0,0,1),to-from));
	glMultMatrixd(q.toRotationMatrix().data());
	drawArrow((to-from).norm(), radius, nbSubdivisions);
	glPopMatrix();
}

