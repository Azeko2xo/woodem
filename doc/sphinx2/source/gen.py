import woo.document
# import all modules here
from woo import utils,log,timing,pack,document,manpage,plot,post2d,runtime,ymport,WeightedAverage2d
import minieigen
cxxRst,pyRst=woo.document.packageClasses('.')

cxxMods='cxxMods.rst'
with open(cxxMods,'w') as f:
	f.write('Woo modules\n######################\n\n')
	f.write('.. toctree::\n\n')
	for o in cxxRst: f.write('    %s\n'%o)
	for o in pyRst: f.write('    %s\n'%o)
import sys
import sphinx
sphinx.main(['','-P','-b','html','-d','../build/doctrees','../source','../build/html'])

