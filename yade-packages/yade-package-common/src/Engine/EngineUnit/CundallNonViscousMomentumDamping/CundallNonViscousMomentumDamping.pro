isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


LIBS += -lPhysicalActionDamper \
        -lMomentum \
        -lRigidBodyParameters \
        -rdynamic 
INCLUDEPATH += $${YADE_QMAKE_PATH}/include/ \
               ../../../DataClass/PhysicalAction/Momentum \
               ../../../DataClass/PhysicalParameters/RigidBodyParameters \
               ../../../DataClass/PhysicalParameters/ParticleParameters \
               ../../../Engine/MetaEngine/PhysicalActionDamper 
QMAKE_LIBDIR = ../../../../bin \
               ../../../../bin \
               $${YADE_QMAKE_PATH}/lib/yade/yade-libs 
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
HEADERS += CundallNonViscousMomentumDamping.hpp 
SOURCES += CundallNonViscousMomentumDamping.cpp 
