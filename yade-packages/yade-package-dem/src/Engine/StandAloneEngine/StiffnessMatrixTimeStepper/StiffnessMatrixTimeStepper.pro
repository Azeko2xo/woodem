isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


LIBS += -lElasticContactParameters \
        -lSpheresContactGeometry \
        -lMacroMicroElasticRelationships \
        -lSphere \
	-lStiffnessMatrix \
        -rdynamic 
INCLUDEPATH += $${YADE_QMAKE_PATH}/include/ \
               ../../../Engine/EngineUnit/MacroMicroElasticRelationships \
               ../../../DataClass/InteractionPhysics/ElasticContactParameters \
               ../../../DataClass/InteractionGeometry/SpheresContactGeometry \
               ../../../DataClass/PhysicalParameters/BodyMacroParameters \
	       ../../../DataClass/PhysicalAction/StiffnessMatrix
QMAKE_LIBDIR = ../../../../bin \
               ../../../../bin \
               $${YADE_QMAKE_PATH}/lib/yade/yade-package-common/ \
               $${YADE_QMAKE_PATH}/lib/yade/yade-libs/ 
QMAKE_CXXFLAGS_RELEASE += -lpthread \
                          -pthread 
QMAKE_CXXFLAGS_DEBUG += -lpthread \
                        -pthread 
DESTDIR = ../../../../bin 
CONFIG += debug \
          thread \
          warn_on \
          dll 
TEMPLATE = lib 
HEADERS += StiffnessMatrixTimeStepper.hpp 
SOURCES += StiffnessMatrixTimeStepper.cpp 
QMAKE_RUN_CXX_IMP = $(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $(shell pwd)/$<
