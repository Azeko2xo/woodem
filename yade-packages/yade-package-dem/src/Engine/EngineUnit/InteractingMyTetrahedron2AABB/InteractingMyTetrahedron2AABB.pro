isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


LIBS += -lBoundingVolumeMetaEngine \
        -lInteractingMyTetrahedron \
        -lAABB \
        -lyade-lib-wm3-math \
        -rdynamic 
INCLUDEPATH += $${YADE_QMAKE_PATH}/include/ \
               ../../../DataClass/InteractingGeometry/InteractingMyTetrahedron \
               ../../../DataClass/BoundingVolume/AABB \
               ../../../Engine/MetaEngine/BoundingVolumeMetaEngine 
QMAKE_LIBDIR = ../../../../bin \
               $${YADE_QMAKE_PATH}/lib/yade/yade-libs/ \
	       $${YADE_QMAKE_PATH}/lib/yade/yade-package-common 
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
HEADERS += InteractingMyTetrahedron2AABB.hpp 
SOURCES += InteractingMyTetrahedron2AABB.cpp 

QMAKE_RUN_CXX_IMP = $(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $(shell pwd)/$<
