isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


LIBS += -lSDECLinkGeometry \
        -lElasticContactLaw \
        -lElasticCohesiveLaw \
        -lMacroMicroElasticRelationships \
        -lElasticCriterionTimeStepper \
        -lPhysicalActionVectorVector \
        -lInteractionVecSet \
        -lBodyRedirectionVector \
        -lInteractingSphere \
        -lInteractingBox \
        -lCundallNonViscousMomentumDamping \
        -lCundallNonViscousForceDamping \
        -lMetaInteractingGeometry \
        -lGravityEngine \
        -lyade-lib-serialization \
        -lyade-lib-base -lWm3Foundation \
        -lPhysicalActionContainerInitializer \
        -lPhysicalActionContainerReseter \
        -lInteractionGeometryMetaEngine \
        -lInteractionPhysicsMetaEngine \
        -lPhysicalActionApplier \
        -lPhysicalParametersMetaEngine \
        -lBoundingVolumeMetaEngine \
        -lBox \
        -lSphere \
        -lAABB \
        -lPersistentSAPCollider \
        -lParticleParameters \
        -lSAPCollider \
        -lMetaInteractingGeometry2AABB \
        -lyade-lib-multimethods \
	-lboost_filesystem \
	-lTranslationEngine \
        -rdynamic 
INCLUDEPATH += $${YADE_QMAKE_PATH}/include/ \
               ../../Engine/StandAloneEngine/ElasticCriterionTimeStepper \
               ../../Engine/StandAloneEngine/ElasticContactLaw \
               ../../Engine/StandAloneEngine/ElasticCohesiveLaw \
               ../../Engine/EngineUnit/MacroMicroElasticRelationships \
               ../../DataClass/InteractionPhysics/SDECLinkPhysics \
               ../../DataClass/InteractionGeometry/SDECLinkGeometry \
               ../../DataClass/PhysicalParameters/BodyMacroParameters 
QMAKE_LIBDIR = ../../../bin \
               $${YADE_QMAKE_PATH}/lib/yade/yade-package-common/ \
               $${YADE_QMAKE_PATH}/lib/yade/yade-libs/ 
QMAKE_CXXFLAGS_RELEASE += -lpthread \
                          -pthread 
QMAKE_CXXFLAGS_DEBUG += -lpthread \
                        -pthread 
DESTDIR = ../../../bin 
CONFIG += debug \
          thread \
          warn_on \
          dll 
TEMPLATE = lib 
HEADERS += ThreePointBending.hpp 
SOURCES += ThreePointBending.cpp 
QMAKE_RUN_CXX_IMP = $(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $(shell pwd)/$<
