'''
This module reimplements the gts.read function in python, since it is stragenly broken under windows, and replaces the original one by monkey-patching.
'''


def py_gts_read(f):
	'''Pure-python implementation of the pygts._gts.read function; it hangs under Windows
	when using woo.gts (not pure pygts), in gts-0.7.6/src/misc.c:173 in next_char, where
	fgetc is waiting for the next character indefinitely (why?); when fgetc was replaced by lower-level read(fileno,buf,1), GTS was complaining about not seeing the first integer
	(number of vertices) at the beginning of the file.
	
	Since there seemed little point digging into GTS any further, this monkey-patch
	replaces that function with a pure-python implementation, which is perhaps not as
	efficient, but works reliably under Windows.
	
	The patch is only applied under Windows, and if 'gts' is in woo.config.features
	(i.e. the module woo.gts exists, and is also exposed as gts).
	'''
	
	import woo.gts
	lineno=1
	surf=woo.gts.Surface()
	#nVert,nEdge,nFace=0,0,0
	try:
		l1=f.readline()[:-1].split()
		nVert,nEdge,nFace=int(l1[0]),int(l1[1]),int(l1[2])
		if l1[3:]!=['GtsSurface','GtsFace','GtsEdge','GtsVertex']: sys.stderr.write('Warning: expected "GtsSurface GtsFace GtsEdge GtsVertex" at the end of line 1; ignoring.\n')
	except: raise RuntimeError('Error reading GTS header (number of vertices, edges, faces), line %d'%lineno)
	vv,ee=[],[]
	for vn in range(0,nVert):
		lineno+=1
		try:
			vv.append(woo.gts.Vertex(*tuple([float(x) for x in f.readline()[:-1].split()])))
		except: raise RuntimeError('Error reading vertex no. %d, line %d'%(vn,lineno))
	assert len(vv)==nVert
	for en in range(0,nEdge):
		lineno+=1
		try:
			n=[int(i) for i in f.readline()[:-1].split()]
			assert len(n)==2
			ee.append(woo.gts.Edge(vv[n[0]-1],vv[n[1]-1]))
		except: raise RuntimeError('Error reading edge no. %d, line %d'%(en,lineno))
	assert len(ee)==nEdge
	for fn in range(0,nFace):
		lineno+=1
		try:
			n=[int(i) for i in f.readline()[:-1].split()]
			assert len(n)==3
		except:
			raise RuntimeError('Error reading face no. %d, line %d'%(fn,lineno))
		surf.add(woo.gts.Face(ee[n[0]-1],ee[n[1]-1],ee[n[2]-1]))
	return surf

import woo.config, sys

if 'gts' in woo.config.features and sys.platform=='win32':
	import woo.gts
	woo.gts.read=py_gts_read
