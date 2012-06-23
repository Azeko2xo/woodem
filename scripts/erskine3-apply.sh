#!/bin/sh
#set -x


ERSKINE=./erskine3.py
YADE=../
# erskine3 accepts scons variables smuggled as @VARIABLE and resulting in $VARIABLE in generated scripts; do that for $PREFIX (ac scons var)
export WOO_QMAKE_PATH=@PREFIX YADECOMPILATIONPATH=$YADE
ENGINE=--scons; SCRIPT=SConscript
#ENGINE=--waf; SCRIPT=wscript_build

# erskine2.py syntax:
### project-file [--waf|--scons] buildscript-dir > buildscript
# projects are traversed recursively and corresponding waf wscript_build files are sent to the standard output

$ERSKINE $ENGINE $YADE/woo-libs/*/src/*.pro $YADE/woo-libs > $YADE/woo-libs/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-core/src/woo.pro $YADE/woo-core > $YADE/woo-core/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-guis/woo-gui-qt/src/QtGUI.pro $YADE/woo-guis > $YADE/woo-guis/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-extra/woo-spherical-dem-simulator/src/woo-spherical-dem-simulator.pro $YADE/woo-extra > $YADE/woo-extra/$SCRIPT

# packages are have separated buildscripts each
$ERSKINE $ENGINE $YADE/woo-packages/woo-package-common/src/woo-package-common.pro $YADE/woo-packages/woo-package-common > $YADE/woo-packages/woo-package-common/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-packages/woo-package-dem/src/woo-package-dem.pro $YADE/woo-packages/woo-package-dem > $YADE/woo-packages/woo-package-dem/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-packages/woo-package-fem/src/woo-package-fem.pro $YADE/woo-packages/woo-package-fem > $YADE/woo-packages/woo-package-fem/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-packages/woo-package-lattice/src/woo-package-lattice.pro $YADE/woo-packages/woo-package-lattice > $YADE/woo-packages/woo-package-lattice/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-packages/woo-package-mass-spring/src/woo-package-mass-spring.pro $YADE/woo-packages/woo-package-mass-spring > $YADE/woo-packages/woo-package-mass-spring/$SCRIPT
$ERSKINE $ENGINE $YADE/woo-packages/woo-package-realtime-rigidbody/src/woo-package-realtime-rigidbody.pro $YADE/woo-packages/woo-package-realtime-rigidbody > $YADE/woo-packages/woo-package-realtime-rigidbody/$SCRIPT

echo Creating entry for woo-extra/Clump in $YADE/woo-extra/$SCRIPT
echo "env.SConscript(dirs=['clump'])" >> $YADE/woo-extra/$SCRIPT
