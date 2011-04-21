#pragma once
#include<string>
#include<sstream>
#include<vector>

#include<yade/lib/factory/ClassFactory.hpp>

#define REGISTER_CLASS_AND_BASE(cn,bcn) REGISTER_CLASS_NAME(cn); REGISTER_BASE_CLASS_NAME(bcn);

#define REGISTER_CLASS_NAME(cn) public: virtual std::string getClassName() const { return #cn; };

#define REGISTER_BASE_CLASS_NAME(bcn) \
	public:virtual std::vector<std::string> getBaseClassNames() const { \
		std::string token; std::vector<std::string> tokens; std::istringstream iss(std::string(#bcn));\
		while(!iss.eof()){iss>>token; tokens.push_back(token);} \
		return tokens; \
	}

class Factorable{
	public:
		virtual ~Factorable(){}
		virtual std::vector<std::string> getBaseClassNames() const { return std::vector<std::string>(); }
		std::string getBaseClassName(unsigned int i=0) const { std::vector<std::string> bases(getBaseClassNames()); return (i>=bases.size()?std::string(""):bases[i]); } 
		int getBaseClassNumber(){ return getBaseClassNames().size(); }

		//virtual std::string getBaseClassName(unsigned int i=0) const { return "";}	// FIXME[1]
		//virtual int getBaseClassNumber() { return 0;}				// FIXME[1]
	REGISTER_CLASS_NAME(Factorable);
};


