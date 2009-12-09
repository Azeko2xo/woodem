// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include<yade/core/StandAloneEngine.hpp>
#include<yade/pkg-common/InteractionGeometryDispatcher.hpp>
#include<yade/pkg-common/InteractionPhysicsDispatcher.hpp>
#include<yade/pkg-common/LawDispatcher.hpp>

class InteractionDispatchers: public StandAloneEngine {
	bool alreadyWarnedNoCollider;
	public:
		InteractionDispatchers();
		virtual void action(Scene*);
		shared_ptr<InteractionGeometryDispatcher> geomDispatcher;
		shared_ptr<InteractionPhysicsDispatcher> physDispatcher;
		shared_ptr<LawDispatcher> lawDispatcher;
		REGISTER_CLASS_AND_BASE(InteractionDispatchers,StandAloneEngine);
		REGISTER_ATTRIBUTES(StandAloneEngine,
			(geomDispatcher)
			(physDispatcher)
			(lawDispatcher)
		);
		DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(InteractionDispatchers);
