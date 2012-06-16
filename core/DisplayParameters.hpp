#pragma once
#include<yade/lib/object/Object.hpp>
#include<string>
#include<vector>
 
/* Class for storing set of display parameters.
 *
 * The interface sort of emulates map string->string (which is not handled by yade-serialization).
 *
 * The "keys" (called displayTypes) are intended to be "OpenGLRenderer" or "GLViewer" (and perhaps other).
 * The "values" are intended to be XML representation of display parameters, obtained either by yade-serialization
 * with OpenGLRenderer and saveStateToStream with QGLViewer (and GLViewer).
 *
 */

class DisplayParameters: public Object{
	public:
		//! Get value of given display type and put it in string& value and return true; if there is no such display type, return false.
		bool getValue(std::string displayType, std::string& value){assert(values.size()==displayTypes.size()); vector<string>::iterator I=std::find(displayTypes.begin(),displayTypes.end(),displayType); if(I==displayTypes.end()) return false; value=values[std::distance(displayTypes.begin(),I)]; return true;}
		//! Set value of given display type; if such display type exists, it is overwritten, otherwise a new one is created.
		void setValue(std::string displayType, std::string value){assert(values.size()==displayTypes.size()); vector<string>::iterator I=std::find(displayTypes.begin(),displayTypes.end(),displayType); if(I==displayTypes.end()){displayTypes.push_back(displayType); values.push_back(value);} else {values[std::distance(displayTypes.begin(),I)]=value;};}
	YADE_CLASS_BASE_DOC_ATTRS(DisplayParameters,Object,"Store display parameters, not useful for user.",
		((std::vector<std::string>,values,,AttrTrait<Attr::hidden>(),""))
		((std::vector<std::string>,displayTypes,,AttrTrait<Attr::hidden>(),""))
	);
};
REGISTER_SERIALIZABLE(DisplayParameters);