#pragma once
#include<woo/lib/object/Object.hpp>
#include<string>
#include<vector>
 
/* Class for storing set of display parameters.
 *
 * The interface sort of emulates map string->string (which is not handled by woo-serialization).
 *
 * The "keys" (called displayTypes) are intended to be "OpenGLRenderer" or "GLViewer" (and perhaps other).
 * The "values" are intended to be XML representation of display parameters, obtained either by woo-serialization
 * with OpenGLRenderer and saveStateToStream with QGLViewer (and GLViewer).
 *
 */

class DisplayParameters: public Object{
	public:
		//! Get value of given display type and put it in string& value and return true; if there is no such display type, return false.
		bool getValue(string displayType, std::string& value);
		//! Set value of given display type; if such display type exists, it is overwritten, otherwise a new one is created.
		void setValue(string displayType, std::string value);
	#define woo_gl_DisplayParameters__CLASS_BASE_DOC_ATTRS \
		DisplayParameters,Object,"Store display parameters, not useful for user.", \
		((vector<string>,values,,AttrTrait<Attr::hidden>(),"")) \
		((vector<string>,displayTypes,,AttrTrait<Attr::hidden>(),"")) 
	WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_gl_DisplayParameters__CLASS_BASE_DOC_ATTRS);
};
WOO_REGISTER_OBJECT(DisplayParameters);
