isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


SUBDIRS += PolyhedralSweptSphere2AABB \
           Box2PolyhedralSweptSphere \
           Tetrahedron2PolyhedralSweptSphere \
           MacroMicroElasticRelationships \
           InteractingBox2InteractingSphere4SpheresContactGeometry \
           InteractingSphere2InteractingSphere4SpheresContactGeometry 
CONFIG += debug \
          thread \
warn_on
TEMPLATE = subdirs
INCLUDEPATH += $${YADE_QMAKE_PATH}/include

