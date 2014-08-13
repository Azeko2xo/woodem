#ifdef WOO_OPENGL

#include<woo/core/DisplayParameters.hpp>
#include<woo/core/Master.hpp>

WOO_PLUGIN(gl,(DisplayParameters));
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_gl_DisplayParameters__CLASS_BASE_DOC_ATTRS);

bool DisplayParameters::getValue(std::string displayType, std::string& value){
	assert(values.size()==displayTypes.size());
	vector<string>::iterator I=std::find(displayTypes.begin(),displayTypes.end(),displayType);
	if(I==displayTypes.end()) return false;
	value=values[std::distance(displayTypes.begin(),I)]; return true;
}

void DisplayParameters::setValue(std::string displayType, std::string value){
	assert(values.size()==displayTypes.size());
	vector<string>::iterator I=std::find(displayTypes.begin(),displayTypes.end(),displayType);
	if(I==displayTypes.end()){displayTypes.push_back(displayType); values.push_back(value);}
	else {values[std::distance(displayTypes.begin(),I)]=value;}
}

#endif /* WOO_OPENGL */
