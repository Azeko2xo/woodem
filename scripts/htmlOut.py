from woo.core import *

import codecs
out=codecs.open('/tmp/out.html','w','utf-8')
out.write('<head><meta http-equiv="content-type" content="text/html;charset=UTF-8" /></head><body>')
out.write('<h1>Preprocessor</h1>')
import woo.pre
rr=yade.pre.Roro()
O.scene=rr()
while O.dem.con.countReal()<1: O.run(100,True)

for obj,section in ((rr,'Preprocessor'),(O.dem.par[10],'Particle'),([c for c in O.dem.con if c.real][0],'Contact')):
	out.write('<h1>%s</h1>'%section)
	for fmt in ('html','expr','pickle','xml'):
		out.write('<h2>%s</h2>'%fmt)
		if fmt!='html': out.write('<pre>')
		# those can only dump to files			
		if fmt in ('xml',):
			outFile=O.tmpFilename()+'.'+fmt
			obj.dump(outFile)
			out.write(open(outFile).read().replace('<','&lt;').replace('>','&gt;'))
		else:
			obj.dump(out,format=fmt)
		if fmt!='html': out.write('</pre>')

out.write('</body>')
out.close()

