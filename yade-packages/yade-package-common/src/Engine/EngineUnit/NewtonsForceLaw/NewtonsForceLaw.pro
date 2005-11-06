isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


LIBS += -lPhysicalActionApplier \
        -lForce \
        -lParticleParameters \
        -lPhysicalActionApplier \
        -rdynamic 
INCLUDEPATH += $${YADE_QMAKE_PATH}/include/ \
               ../../../DataClass/PhysicalAction/Force \
               ../../../DataClass/PhysicalParameters/ParticleParameters \
               ../../../Engine/MetaEngine/PhysicalActionApplier 
QMAKE_LIBDIR = ../../../../bin \
               ../../../../bin \
               ../../../../bin \
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
HEADERS += NewtonsForceLaw.hpp 
SOURCES += NewtonsForceLaw.cpp 
