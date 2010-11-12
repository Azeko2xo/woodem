// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#include<yade/pkg/common/Dispatching.hpp>
/* Experimental constitutive law using the LawDispatcher.
 * Has only purely elastic normal and shear components. */
class ef2_Dem3Dof_NormalShear_ElasticFrictionalLaw: public LawFunctor {
	virtual void go(shared_ptr<IGeom>&, shared_ptr<IPhys>&, Interaction*);
	FUNCTOR2D(Dem3DofGeom,NormShearPhys);
	REGISTER_CLASS_AND_BASE(ef2_Dem3Dof_NormalShear_ElasticFrictionalLaw,LawFunctor);
};
REGISTER_SERIALIZABLE(ef2_Dem3Dof_NormalShear_ElasticFrictionalLaw);
