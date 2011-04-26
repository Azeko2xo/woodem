#include<yade/pkg/dem/GenericSpheresContact.hpp>
YADE_PLUGIN(dem,(GenericSpheresContact));

GenericSpheresContact::~GenericSpheresContact(){}
Real GenericSpheresContact::maxRefR(){ if(refR1<=0&&refR2<=0) throw runtime_error("GenericSpheresContact::{refR1,refR2} are both non-positive!"); return (refR1>0&&refR2>0)?max(refR1,refR2):(refR1>0?refR1:refR2); }
Real GenericSpheresContact::minRefR(){ if(refR1<=0&&refR2<=0) throw runtime_error("GenericSpheresContact::{refR1,refR2} are both non-positive!"); return (refR1>0&&refR2>0)?min(refR1,refR2):(refR1>0?refR1:refR2); }


