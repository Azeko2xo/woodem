#!/bin/bash
set -e -x
# install Woo
python setup.py install
# install extras
for EX in wooExtra/*; do
	[ -f $EX/setup.py ] || continue
	pushd $EX
		python setup.py install
	popd
done

python /c/src/pyinstaller-develop/pyinstaller.py -y woo.pyinstaller.spec

pushd nsis
	# -w: woo
	# -e: extras
	# -l: libs
	# -u: upload (Linux-only)
	bash nsis-runall.sh -w -e
popd

exit 0
