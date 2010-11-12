// 2010 © Václav Šmilauer <eudoxos@arcig.cz>

#include<yade/pkg/common/MatchMaker.hpp>
#include<boost/foreach.hpp>
#ifndef FOREACH
	#define FOREACH BOOST_FOREACH
#endif

YADE_PLUGIN((MatchMaker));
MatchMaker::~MatchMaker(){}

Real MatchMaker::operator()(int id1, int id2, Real val1, Real val2) const {
	FOREACH(const Vector3r& m, matches){
		if(((int)m[0]==id1 && (int)m[1]==id2) || ((int)m[0]==id2 && (int)m[1]==id1)) return m[2];
	}
	// no match
	if(fbNeedsValues && (isnan(val1) || isnan(val2))) throw std::invalid_argument("MatchMaker: no match for ("+lexical_cast<string>(id1)+","+lexical_cast<string>(id2)+"), and values required for fallback computation '"+fallback+"' not specified.");
	return computeFallback(val1,val2);
}

void MatchMaker::postLoad(MatchMaker&){
	if(fallback=="val")      { fbPtr=&MatchMaker::fbVal; fbNeedsValues=false; }
	else if(fallback=="zero"){ fbPtr=&MatchMaker::fbZero;fbNeedsValues=false; } 
	else if(fallback=="avg") { fbPtr=&MatchMaker::fbAvg; fbNeedsValues=true;  }
	else if(fallback=="min") { fbPtr=&MatchMaker::fbMin; fbNeedsValues=true;  }
	else if(fallback=="max") { fbPtr=&MatchMaker::fbMax; fbNeedsValues=true;  }
	else if(fallback=="harmAvg") { fbPtr=&MatchMaker::fbHarmAvg; fbNeedsValues=true; }
	else throw std::invalid_argument("MatchMaker:: fallback '"+fallback+"' not recognized (possible values: val, avg, min, max, harmAvg).");
}

Real MatchMaker::computeFallback(Real v1, Real v2) const { return (this->*MatchMaker::fbPtr)(v1,v2); }
