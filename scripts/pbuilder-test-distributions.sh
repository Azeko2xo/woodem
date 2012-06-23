#
# grab latest trunk, try building packages for various distributions
# see http://woo.wikia.com/wiki/DebianPackages for configuration of the pbuilder environment
#
set -x
TMP=/tmp/pbuild-woo-$$
mkdir $TMP
cd $TMP
bzr checkout --lightweight lp:woo trunk
cd trunk; scripts/debian-prep jaunty; cd .. # distro not important here
dpkg-source -b -I trunk
DSC=woo-`cat trunk/VERSION`_1.dsc
for DIST in lenny squeeze hardy jaunty karmic; do
	echo ============================================= TESTING $DIST ===================================
	echo ===============================================================================================
	BASETGZ=/var/cache/pbuilder/$DIST.tgz
	sudo pbuilder --build --basetgz $BASETGZ --debbuildopts "-j5 -Zlzma" $DSC  || FAILED="$FAILED $DIST"
done
rm -rf $TMP
if [ '$FAILED' ]; then echo "***************** Build failed for $FAILED"; exit 1; fi
