import yade.document
# import all modules here
from yade import utils,log,timing,pack,document,manpage,plot,post2d,runtime,ymport,WeightedAverage2d,wrapper
cxxRst,pyRst=yade.document.packageClasses('.')

cxxMods='cxxMods.rst'
with open(cxxMods,'w') as f:
	f.write('Yade modules\n######################\n\n')
	f.write('.. toctree::\n\n')
	for o in cxxRst: f.write('    %s\n'%o)
	for o in pyRst: f.write('    %s\n'%o)
import sys
import sphinx
sphinx.main(['','-b','html','-d','../build/doctrees','../source','../build/html'])

