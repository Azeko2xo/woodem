#include<yade/core/Field.hpp>

std::string DynState::blockedDOFs_vec_get() const {
	std::string ret;
	#define _SET_DOF(DOF_ANY,ch) if((blockedDOFs & DynState::DOF_ANY)!=0) ret.push_back(ch);
	_SET_DOF(DOF_X,'x'); _SET_DOF(DOF_Y,'y'); _SET_DOF(DOF_Z,'z'); _SET_DOF(DOF_RX,'X'); _SET_DOF(DOF_RY,'Y'); _SET_DOF(DOF_RZ,'Z');
	#undef _SET_DOF
	return ret;
}

void DynState::blockedDOFs_vec_set(const std::string& dofs){
	blockedDOFs=0;
	FOREACH(char c, dofs){
		#define _GET_DOF(DOF_ANY,ch) if(c==ch) { blockedDOFs|=DynState::DOF_ANY; continue; }
		_GET_DOF(DOF_X,'x'); _GET_DOF(DOF_Y,'y'); _GET_DOF(DOF_Z,'z'); _GET_DOF(DOF_RX,'X'); _GET_DOF(DOF_RY,'Y'); _GET_DOF(DOF_RZ,'Z');
		#undef _GET_DOF
		throw std::invalid_argument("Invalid  DOF specification `"+lexical_cast<string>(c)+"' in '"+dofs+"', characters must be ∈{x,y,z,X,Y,Z}.");
	}
}

