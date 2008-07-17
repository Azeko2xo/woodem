#pragma once
 
/* Class for storing set of display parameters.
 *
 * The interface sort of emulates map string->string (which is not handled by yade-serialization).
 *
 * The "keys" (called displayTypes) are intended to be "OpenGLRenderingEngine" or "GLViewer" (and perhaps other).
 * The "values" are intended to be XML representation of display parameters, obtained either by yade-serialization
 * with OpenGLRenderingEngine and saveStateToStream with QGLViewer (and GLViewer).
 *
 */

class DisplayParameters: public Serializable{
	private:
		vector<string> values;
		vector<string> displayTypes;
	public:
		//! Get value of given display type and put it in string& value and return true; if there is no such display type, return false.
		bool getValue(string displayType, string& value){assert(values.size()==displayTypes.size()); vector<string>::iterator I=std::find(displayTypes.begin(),displayTypes.end(),displayType); if(I==displayTypes.end()) return false; value=values[std::distance(displayTypes.begin(),I)]; return true;}
		//! Set value of given display type; if such display type exists, it is overwritten, otherwise a new one is created.
		void setValue(string displayType, string value){assert(values.size()==displayTypes.size()); vector<string>::iterator I=std::find(displayTypes.begin(),displayTypes.end(),displayType); if(I==displayTypes.end()){displayTypes.push_back(displayType); values.push_back(value);} else {values[std::distance(displayTypes.begin(),I)]=value;};}
	DisplayParameters(){}
	virtual ~DisplayParameters(){}
	virtual void registerAttributes(){ REGISTER_ATTRIBUTE(displayTypes); REGISTER_ATTRIBUTE(values); }
	REGISTER_CLASS_NAME(DisplayParameters);
	REGISTER_BASE_CLASS_NAME(Serializable);
};
REGISTER_SERIALIZABLE(DisplayParameters,false);
