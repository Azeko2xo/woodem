isEmpty ( YADE_QMAKE_PATH ) {
error( "YADE_QMAKE_PATH internal qmake variable is not set, you should run for example qmake YADE_QMAKE_PATH=/usr/local, this will not work from inside kdevelop (when they will fix it?)" )
}


INCLUDEPATH += $${YADE_QMAKE_PATH}/include 
win32 {
TARGET = ../../../bin/BINFormatManager 
CONFIG += console
}
!win32 {
TARGET = ../../bin/BINFormatManager 
}

CONFIG += debug \
          thread \
          warn_on \
          dll 
TEMPLATE = lib 
HEADERS += BINFormatManager.hpp
SOURCES += BINFormatManager.cpp
