// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<yade/core/Dispatcher.hpp>
#include<yade/lib-multimethods/DynLibDispatcher.hpp>
#include<yade/core/Interaction.hpp>
#include<yade/pkg-common/ConstitutiveLaw.hpp>

class ConstitutiveLawDispatcher:
	public Dispatcher2D <
		InteractionGeometry, // 1st base classe for dispatch
		InteractionPhysics,  // 2nd base classe for dispatch
		ConstitutiveLaw,     // functor base class
		void,                // return type
		TYPELIST_4(shared_ptr<InteractionGeometry>&, shared_ptr<InteractionPhysics>&, Interaction*, World*),
		false                // autosymmetry
	>{
		public:
		virtual void action(World*);
		REGISTER_CLASS_AND_BASE(ConstitutiveLawDispatcher,Dispatcher2D);
};
REGISTER_SERIALIZABLE(ConstitutiveLawDispatcher);

