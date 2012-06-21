/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#ifndef YADE_OPENGL
#error "This build doesn't support openGL. Therefore, this header must not be used."
#endif

#include<woo/lib/base/Math.hpp>


// disable temporarily
//#include<boost/static_assert.hpp>
//#define STATIC_ASSERT(arg) 


#include<GL/gl.h>
#include<GL/glut.h>

struct OpenGLWrapper {}; // for ctags

///	Primary Templates

#define _VOID_IMPL { Type::undefined_template; }
template< typename Type > inline void glScale		( Type ,Type , Type  ) _VOID_IMPL;
template< typename Type > inline void glScalev		( const Type  ) _VOID_IMPL;
template< typename Type > inline void glTranslate	( Type ,Type , Type  ) _VOID_IMPL;
template< typename Type > inline void glTranslatev	( const Type ) _VOID_IMPL;
template< typename Type > inline void glVertex2		( Type ,Type  ) _VOID_IMPL;
template< typename Type > inline void glVertex3		( Type ,Type , Type  ) _VOID_IMPL;
template< typename Type > inline void glVertex2v	( const Type ) _VOID_IMPL;
template< typename Type > inline void glVertex3v	( const Type ) _VOID_IMPL;
template< typename Type > inline void glNormal3		( Type ,Type ,Type  ) _VOID_IMPL;
template< typename Type > inline void glNormal3v	( const Type ) _VOID_IMPL;
template< typename Type > inline void glIndex		( Type  ) _VOID_IMPL;
template< typename Type > inline void glIndexv		( Type  ) _VOID_IMPL;
template< typename Type > inline void glColor3		( Type ,Type ,Type  ) _VOID_IMPL;
template< typename Type > inline void glColor4		( Type ,Type ,Type , Type  ) _VOID_IMPL;
template< typename Type > inline void glColor3v		( const Type ) _VOID_IMPL;
template< typename Type > inline void glColor4v		( const Type ) _VOID_IMPL;

template< typename Type > inline void glRect		( Type ,Type ,Type , Type  )	_VOID_IMPL;
template< typename Type > inline void glMaterial	( GLenum face, GLenum pname, Type param ) _VOID_IMPL;
template< typename Type > inline void glMaterialv	( GLenum face, GLenum pname, Type param ) _VOID_IMPL;
template< typename Type > inline void glMultMatrix	(const Type*) _VOID_IMPL;
#undef _VOID_IMPL


///	Template Specializations
template< > inline void glMultMatrix<double>(const double* m){glMultMatrixd(m);	};
template< > inline void glMultMatrix<long double>(const long double* m){double mm[16]; for(int i=0;i<16;i++)mm[i]=(double)m[i]; glMultMatrixd(mm);};

template< > inline void glScale< double >			( double x,double y, double z )		{	glScaled(x,y,z);	};
template< > inline void glScale< long double >			( long double x,long double y, long double z )		{	glScaled(x,y,z);	};
template< > inline void glScale< float >			( float x,float y,float z )		{	glScalef(x,y,z);	};
template< > inline void glScalev< Vector3r >		( const Vector3r v )		{	glScaled(v[0],v[1],v[2]);};

template< > inline void glTranslate< double >			( double x,double y, double z )		{	glTranslated(x,y,z);	};
template< > inline void glTranslate< long double >			( long double x, long double y, long double z )		{	glTranslated(x,y,z);	};
template< > inline void glTranslate< float >			( float x,float y,float z )		{	glTranslatef(x,y,z);	};
template< > inline void glTranslatev< Vector3r >		( const Vector3r v )		{	glTranslated(v[0],v[1],v[2]);};

template< > inline void glVertex2< double >			( double x,double y )			{	glVertex2d(x,y);	};
template< > inline void glVertex2< float >			( float x,float y )			{	glVertex2f(x,y);	};
template< > inline void glVertex2< int >			( int x,int y )				{	glVertex2i(x,y);	};

template< > inline void glVertex3< double >			( double x,double y, double z )		{	glVertex3d(x,y,z);	};
template< > inline void glVertex3< float >			( float x,float y,float z )		{	glVertex3f(x,y,z);	};
template< > inline void glVertex3< int >			( int x, int y, int z )			{	glVertex3i(x,y,z);	};

// :%s/\(void \)\(gl[A-Z,a-z,0-9]\+\)\(f\)( GLfloat \([a-z]\), GLfloat \([a-z]\), GLfloat \([a-z]\) );/template< > inline \1\2< float >			( float \4,float \5,float \6 )	{	\2\3(\4,\5,\6);	};/
template< > inline void glVertex2v< Vector3r >		( const Vector3r v )		{	glVertex2dv((double*)&v);		};
template< > inline void glVertex2v< Vector3i >		( const Vector3i v )		{	glVertex2iv((int*)&v);		};

template< > inline void glVertex3v< Vector3r >		( const Vector3r v )		{	glVertex3dv((double*)&v);		};
template< > inline void glVertex3v< Vector3i >		( const Vector3i v )		{	glVertex3iv((int*)&v);		};

template< > inline void glNormal3< double >			(double nx,double ny,double nz )	{	glNormal3d(nx,ny,nz);	};
template< > inline void glNormal3< int >			(int nx,int ny,int nz )			{	glNormal3i(nx,ny,nz);	};

template< > inline void glNormal3v< Vector3r >		( const Vector3r v )		{	glNormal3dv((double*)&v);		};
template< > inline void glNormal3v< Vector3i >		( const Vector3i v )		{	glNormal3iv((int*)&v);		};

template< > inline void glIndex< double >			( double c )				{	glIndexd(c);	};
template< > inline void glIndex< float >			( float c )				{	glIndexf(c);	};
template< > inline void glIndex< int >				( int c )				{	glIndexi(c);	};
template< > inline void glIndex< unsigned char >		( unsigned char c )			{	glIndexub(c);	};

template< > inline void glIndexv<const Vector3r >	( const Vector3r c)		{	glIndexdv((double*)&c);	}
template< > inline void glIndexv<const Vector3i >		( const Vector3i c)			{	glIndexiv((int*)&c);	}

template< > inline void glColor3< double >			(double red,double green,double blue )							{	glColor3d(red,green,blue);	};
template< > inline void glColor3< float >			(float red,float green,float blue )							{	glColor3f(red,green,blue);	};
template< > inline void glColor3< int >			(int red,int green,int blue )								{	glColor3i(red,green,blue);	};

template< > inline void glColor4< double >			(double red,double green,double blue, double alpha )					{	glColor4d(red,green,blue,alpha);	};
template< > inline void glColor4< float >			(float red,float green,float blue, float alpha )					{	glColor4f(red,green,blue,alpha);	};
template< > inline void glColor4< int >			(int red,int green,int blue, int alpha )						{	glColor4i(red,green,blue,alpha);	};


template< > inline void glColor3v< Vector3r >		( const Vector3r v )		{	glColor3dv((double*)&v);		};
template< > inline void glColor3v< Vector3i >		( const Vector3i v )		{	glColor3iv((int*)&v);		};

template< > inline void glColor4v< Vector3r >		( const Vector3r v )		{	glColor4dv((double*)&v);		};
template< > inline void glColor4v< Vector3i >		( const Vector3i v )		{	glColor4iv((int*)&v);		};



template< > inline void glRect< double >			(double x1,double y1,double x2, double y2 )	{	glRectd(x1,y1,x2,y2);	};
template< > inline void glRect< float >			(float	x1,float  y1,float x2,float y2 )	{	glRectf(x1,y1,x2,y2);	};
template< > inline void glRect< int >				(int	x1,int	  y1,int x2, int y2 )		{	glRecti(x1,y1,x2,y2);	};

template< > inline void glMaterial< float >			( GLenum face, GLenum pname, float param )			{	glMaterialf(face,pname,param);		};
template< > inline void glMaterial< double >			( GLenum face, GLenum pname, double param )			{	glMaterialf(face,pname,param);		};
template< > inline void glMaterial< int >			( GLenum face, GLenum pname, int param )			{	glMateriali(face,pname,param);		};
template< > inline void glMaterialv< Vector3r >	( GLenum face, GLenum pname, const Vector3r params )	{	const GLfloat _p[3]={(GLfloat)params[0],(GLfloat)params[1],(GLfloat)params[2]}; glMaterialfv(face,pname,_p);	};
template< > inline void glMaterialv< Vector3i >		( GLenum face, GLenum pname, const Vector3i params )	{	glMaterialiv(face,pname,(int*)&params);	};


