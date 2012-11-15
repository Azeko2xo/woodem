import woo, woo.batch, woo.core
S=woo.master.scene
woo.batch.readParamsFromTable(unknownOk=True,
	important=6,
	unimportant='foo',
	this=-1,
	notInTable='notInTable'
)
from woo.params import table
print S.tags['description']
print 'important',table.important
print 'unimportant',table.unimportant
import time
S.engines=[woo.core.PyRunner(1,'import time, sys; time.sleep(.005); sys.stderr.write(".")')]
S.run(1000,True)
print 'finished'
import sys
sys.stdout.flush()
sys.exit(0)
