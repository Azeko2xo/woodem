isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


SUBDIRS += Container \
           DataClass \
           Engine \
           RenderingEngine \
           PreProcessor \
           PostProcessor
TEMPLATE=subdirs
CONFIG	+= debug \
          thread \
warn_on
LIBS += -lyade-lib-base -lWm3Foundation

INCLUDEPATH += $${YADE_QMAKE_PATH}/include

QMAKE_RUN_CXX_IMP = $(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $(shell pwd)/$<
