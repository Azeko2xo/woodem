isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


SUBDIRS += BoundingVolume \
           InteractingGeometry \
           GeometricalModel \
           State \
           PhysicalParameters \
           InteractionGeometry \
           InteractionPhysics \
           PhysicalAction 
CONFIG += debug \
          thread \
          warn_on 
TEMPLATE = subdirs 
INCLUDEPATH += $${YADE_QMAKE_PATH}/include

