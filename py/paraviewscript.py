'''
Convenience module for setting up visualization pipelines for Paraview through python scripts.
'''
import woo
import woo.dem


def findPV():
	'Find Paraview executable in the system. Under Windows (64bit only), this entails searching the registry (see `this post <http://www.paraview.org/pipermail/paraview/2010-August/018645.html>`__); under Linux, look in ``$PATH44 for executable called ``paraview``. Returns ``None`` is Paraview is not found.'
	import sys,operator
	if sys.platform=='win32':
		# oh gee...
		# http://www.paraview.org/pipermail/paraview/2010-August/018645.html
		# - HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Kitware Inc.\ParaView 3.8.X (64bit machines single user install)
		# - HKEY_CURRENT_USER\SOFTWARE\Kitware Inc.\ParaView 3.8.X (32bit machines single user install)
		# - HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Kitware Inc.\ParaView 3.8.X (64bit machines all users)
		# - HKEY_LOCAL_MACHINE\SOFTWARE\Kitware Inc.\ParaView 3.8.X (32bit machines all users)
		#
		# we don't care about 32 bit, do just 64bit
		import _winreg as winreg
		path='SOFTWARE\Wow6432Node\Kitware Inc.'
		paraviews=[]
		for reg in (winreg.HKEY_CURRENT_USER,winreg.HKEY_LOCAL_MACHINE):
			aReg=winreg.ConnectRegistry(None,reg)
			try:
				aKey=winreg.OpenKey(aReg,r'SOFTWARE\Wow6432Node\Kitware, Inc.')
			except WindowsError: continue
			for i in range(0,winreg.QueryInfoKey(aKey)[0]):
				keyname=winreg.EnumKey(aKey,i)
				if not keyname.startswith('ParaView '):
					# print 'no match:',keyname
					continue
				subKey=winreg.OpenKey(aKey,keyname)
				paraviews.append((keyname,subKey))
		# sort lexically using key name and use the last one (highest version)
		pv=sorted(paraviews,key=operator.itemgetter(1))
		if not pv: return None # raise RuntimeError('ParaView installation not found in registry.')
		pvName,pvKey=pv[-1]
		pvExec=''
		for i in range(0,winreg.QueryInfoKey(pvKey)[1]):
			key,val,T=winreg.EnumValue(pvKey,i)
			if key=='':
				pvExec=val+'/bin/paraview'
				break
		if not pvExec: return None # raise RuntimeError('ParaView installation: found in registry, but default key is missing...?')
		return pvExec
	else:
		# on Linux, find it in $PATH
		import distutils.spawn
		return distutils.spawn.find_executable('paraview')

def launchPV(script):
	'Launch paraview as background process, passing --script=*script* as the only argument. If Paraview executable is not found via :obj:`findPV`, only a warning is printed and Paraview is not launched.'
	import subprocess
	# will launch it in the background
	pv=findPV()
	if not pv:
		print 'Paraview not executed since the executable was not found.'
		return
	cmd=[pv,'--script='+script]
	print 'Running ',' '.join(cmd)
	subprocess.Popen(cmd)

def fromEngines(S,out=None,launch=False,noDataOk=False):
	'''Write paraview script showing data from the current VTK-related engines:
	
	* :obj:`woo.dem.VtkExport` is queried for files already written, and those are returned;
	* :obj:`woo.dem.FlowAnalysis` is made to export its internal data at the moment of calling this function;
	* :obj:`woo.dem.Tracer` indicates there are particles traces, which are exported by calling :obj:`woo.utils.vtkExportTraces`.

	'''
	sphereFiles,meshFiles,conFiles=[],[],[]
	if not out: out=woo.master.tmpFilename()+'.py'
	if not out.endswith('.py'): out=out+'.py'
	outPrefix=out[:-3] # without extension
	kw={}
	for e in S.engines:
		if isinstance(e,woo.dem.VtkExport) and e.nDone>0: kw.update(kwFromVtkExport(e))
		elif isinstance(e,woo.dem.FlowAnalysis) and e.nDone>0: kw.update(kwFromFlowAnalysis(e,outPrefix=outPrefix)) # without the .py
		elif isinstance(e,woo.dem.Tracer): kw.update(kwFromVtkExportTraces(S,e.field,outPrefix=outPrefix))
	if 'meshFiles' in kw: kw['flowMeshFile']=kw['meshFiles'][-1]
	# no data found from engines
	if not kw:
		if noDataOk: return None
		raise RuntimeError("No Paraview data found in engines.")
	write(out,**kw)
	if launch: launchPV(out)
	return out

def kwFromVtkExport(vtkExport,out=None,launch=False):
	'Extract keywords suitable for :obj:`write` from a given :obj:`~woo.dem.VtkExport` instance.'
	assert isinstance(vtkExport,woo.dem.VtkExport)
	ff=vtkExport.outFiles
	return dict(sphereFiles=ff['spheres'] if 'spheres' in ff else [],meshFiles=ff['mesh'] if 'mesh' in ff else [],conFiles=ff['con'] if 'con' in ff else [],triFiles=ff['tri'] if 'tri' in ff else [])

def kwFromVtkExportTraces(S,dem,outPrefix=None):
	if not outPrefix: outPrefix=woo.master.tmpFilename()
	out=outPrefix+'_traces.vtp'
	tr=woo.utils.vtkExportTraces(S,dem,out)
	if tr: return {'tracesFile':out}
	else: return {}


def kwFromFlowAnalysis(flowAnalysis,outPrefix=None,fractions=[],fracA=[],fracB=[]):
	'Extract keywords suitable for :obj:`write` from a given :obj:`~woo.dem.VtkFlowAnalysis` instance.'
	assert isinstance(flowAnalysis,woo.dem.FlowAnalysis)
	if not outPrefix: outPrefix=woo.master.tmpFilename()
	ret={}
	fa=flowAnalysis
	expFlow=fa.vtkExportFractions(outPrefix+'.all',fractions=fractions)
	ret['flowFile']=expFlow
	# export split data
	if fa.nFractions>1:
		if fracA and fracB: pass
		else:
			logging.info('Guessing values for fracA and fracB as they were not given')
			fracA=range(0,fa.nFractions/2); fracB=range(fa.nFractions/2,fa.nFractions)
		expSplit=fa.vtkExportVectorOps(outPrefix+'.split',frac=fracA,fracB=fracB)
		ret['splitFile']=expSplit
	return ret


def fromVtkExport(vtkExport,out=None,launch=False):
	'Write script for *vtkExport* (:obj:`~woo.dem.VtkExport`) and optionally *launch* Paraview on it.'
	if not out: out=woo.master.tmpFilename()+'.py'
	write(out,**kwFromVtkExport(vtkExport))
	if launch: launchPV(out)

def fromFlowAnalysis(flowAnalysis,out=None,findMesh=True,launch=False):
	'Have *flowAnalysis* (:obj:`~woo.dem.FlowAnalysis`) export flow and split data, write Paraview script for it and optionally launch Paraview. With *findMesh*, try to find a :obj:`~woo.dem.VtkExport` and use its last exported mesh file as mesh to show with flow data as well.'
	if not out: out=woo.master.tmpFilename()+'.py'
	flowMeshFile=''
	for e in flowAnalysis.scene.engines:
		if isinstance(e,woo.dem.VtkExport) and 'mesh' in e.outFiles:
			flowMeshFile=e.outFiles['mesh'][-1]
	write(out,flowMeshFile=flowMeshFile,**kwFromFlowAnalysis(flowAnalysis))
	if launch: launchPV(out)


def write(out,sphereFiles=[],meshFiles=[],conFiles=[],triFiles=[],flowFile='',splitFile='',flowMeshFile='',tracesFile='',flowMeshOpacity=.2,splitStride=2,splitClip=False,flowStride=2):
	'''Write out script suitable for running with Paraview (with the ``--script`` option). The options to this function are:

	:param sphereFiles: files from :obj:`woo.dem.VtkExport.outFiles` (``spheres``);
	:param meshFiles: files from :obj:`woo.dem.VtkExport.outFiles` (``mesh``);
	:param conFiles: files from :obj:`woo.dem.VtkExport.outFiles` (``con``);
	:param triFiles: files from :obj:`woo.dem.VtkExport.outFiles` (``tri``);
	:param flowFile: file written by :obj:`woo.dem.FlowAnalysis.vtkExportFractions` (usually all fractions together);
	:param splitFile: file written by :obj:`woo.dem.FlowAnalysis.vtkExportVectorOps`;
	:param flowMeshFile: mesh file to be used for showing boundaries for split analysis;
	:param tracesFile: file written by :obj:`woo.utils.vtkExportTraces`, for per-particle traces;
	:param flowMeshOpacity: opacity of the mesh for flow analysis;
	:param splitStride: spatial stride for segregation analysis; use 2 and more for very dense data meshes which are difficult to see through;
	:param flowStride: spatial stride for flow analysis.
	'''
	def fixPath(p):
		import os,os.path
		if not p or os.path.isabs(p): return p         # empty or absolute, nothing to do
		if os.path.dirname(out)==os.getcwd(): return p # script in the current directory, so relative is OK
		return os.path.relpath(p,os.path.dirname(out)) # relative to where the script is being written
	subst=dict(
		sphereFiles=[fixPath(f) for f in sphereFiles],
		meshFiles=[fixPath(f) for f in meshFiles],
		conFiles=[fixPath(f) for f in conFiles],
		triFiles=[fixPath(f) for f in triFiles],
		flowFile=fixPath(flowFile),
		splitFile=fixPath(splitFile),
		flowMeshFile=fixPath(flowMeshFile),
		tracesFile=fixPath(tracesFile),
		flowMeshOpacity=flowMeshOpacity,
		splitStride=splitStride,
		splitClip=splitClip,
		flowStride=flowStride,
	)
	f=open(out,'w')
	f.write(_paraviewScriptTemplate.format(**subst))
	f.close()
	

_paraviewScriptTemplate=r'''
import sys, os.path

# input parameters

# fraction flow analysis
splitFile='{splitFile}'
splitStride={splitStride} # with dense meshes, subsample to use only each nth point
flowMeshFile='{flowMeshFile}'
flowMeshOpacity={flowMeshOpacity}

# global flow analysis
flowFile='{flowFile}'
flowStride={flowStride}

# traces
tracesFile='{tracesFile}'


# particles
sphereFiles={sphereFiles}
meshFiles={meshFiles}
conFiles={conFiles}
triFiles={triFiles}

## we should change directory here, so that datafiles are found
# Paraview defined current_script_path but that only works when run from Paraview
# http://paraview.org/Bug/view.php?id=13287
if 'current_script_path' in dir():
	d=os.path.dirname(current_script_path)
	if d: os.chdir(d)

### create zip archive
if hasattr(sys,'argv') and len(sys.argv)>1:
	import argparse,re
	parser=argparse.ArgumentParser()
	parser.add_argument('--zip',help='Create self-contained zip file from data files.',dest='zip',type=str,default='')
	parser.add_argument('--slice',type=str,default='',dest='slice',help='Slice for packing files, to avoid large volumes of data; the slice must be in python format, i.e. ``start:stop`` or ``start:stop:step``, where any of the fields may be empty. E.g. to choose every second file, say ``--slice=::2``, to get just the last one, say ``--slice=-1:``.')
	opts=parser.parse_args()
	if opts.zip:
		zipName=opts.zip+('.zip' if not opts.zip.endswith('.zip') else '')
		out0=os.path.basename(os.path.splitext(zipName)[0]) # directory inside the archive
		newFiles=dict()
		if opts.slice:
			# from http://stackoverflow.com/a/681949/761090 in comment by pprzemek
			vtkSlice=slice(*map(lambda x: int(x.strip()) if x.strip() else None, opts.slice.split(':')))
		else: vtkSlice=slice(None,None) # all files
		import zipfile
		with zipfile.ZipFile(zipName,'w',allowZip64=True) as ar:
			zippedFiles=set()
			for fff in ('sphereFiles','meshFiles','conFiles','triFiles'):
				ff=eval(fff) # get the sequence
				ff2=[]
				for f in ff[vtkSlice]:
					fn=os.path.basename(f)
					print zipName+': adding',fn
					zippedFiles.add(fn)
					ar.write(f,out0+'/'+fn)
					ff2.append(fn)
				newFiles[fff]=ff2
			for ff in ('splitFile','flowMeshFile','flowFile','tracesFile'):
				f=eval(ff)
				fn=os.path.basename(f)
				newFiles[ff]=fn
				if not fn: continue # empty filename for missing data
				if fn in zippedFiles: continue # skip if already in the archive
				print zipName+': adding',fn
				zippedFiles.add(fn)
				ar.write(f,out0+'/'+fn)
			# make a copy of this script, adjust filenames in there and add it to the archive as well
			ll=[]
			for l in open(sys.argv[0]):
				var=l.split('=',1)[0]
				if var in ('sphereFiles','meshFiles','conFiles','triFiles','splitFile','flowMeshFile','flowFile','tracesFile'):
					if type(newFiles[var])==list: ll.append(var+'='+str(newFiles[var])+'\n')
					else: ll.append(var+"='"+newFiles[var]+"'\n")
				else: ll.append(l)
			ar.writestr(out0+'/'+os.path.basename(sys.argv[0]),''.join(ll))
		print 'File '+os.path.abspath(zipName)+' written.'
		sys.exit(0)

from paraview.simple import *		
		

def readPointCellData(src):
	src.PointArrayStatus=src.GetPointDataInformation().keys()
	src.CellArrayStatus=src.GetCellDataInformation().keys()

if splitFile:
	# input data
	split=XMLImageDataReader(FileName=splitFile)
	RenameSource(splitFile,split)
	readPointCellData(split)
	_bounds=split.GetDataInformation().GetBounds()
	flowBounds=_bounds ## to be used later with in flowMeshFIle
	_ext=split.GetDataInformation().GetExtent()
	_avgcell=(1/3.)*sum([(_bounds[2*i+1]-_bounds[2*i])/(_ext[2*i+1]-_ext[2*1]) for i in (0,1,2)])
	rep=Show()
	rep.Representation='Outline'
	rep.Visibility=0 # don't show input outline
	# stride extraction
	_ex=_last=ExtractSubset(VOI=_ext,SampleRateI=splitStride,SampleRateJ=splitStride,SampleRateK=splitStride)
	_cl=_last=Clip(ClipType='Box')
	_cl.ClipType.Bounds=_bounds
	_cl.InsideOut=1
	rep=Show()
	rep.Representation='Outline'
	rep.Visibility=1
	rep.CubeAxesVisibility=1

	for arr,name,rgb in [('cross','Segregation',(0,0,1)),('diffA','Small fraction',(0,1,0)),('diffB','Big fraction',(1,0,0))]:
		SetActiveSource(_last)
		arrData=split.GetPointDataInformation().GetArray(arr)
		scale=_avgcell/max([max([abs(r) for r in arrData.GetRange(i)]) for i in (0,1,2)])
		scale*=10*splitStride # biggest vector spans about 20 cells (including stride)
		gl=Glyph(GlyphType="Arrow",GlyphTransform="Transform2",Vectors=['POINTS',arr],RandomMode=0,MaskPoints=0,SetScaleFactor=scale)
		RenameSource(name,gl)
		rep=Show()
		rep.ColorArrayName=('POINT_DATA','') # solid color
		rep.DiffuseColor=rgb

if flowFile:
	flow=XMLImageDataReader(FileName=[flowFile])
	RenameSource(flowFile,flow)
	flow.PointArrayStatus=flow.GetPointDataInformation().keys()
	flow.CellArrayStatus=flow.GetCellDataInformation().keys()
	bb=flow.GetDataInformation().GetBounds()
	flowBounds=bb ## to be used later with in flowMeshFIle
	ext=flow.GetDataInformation().GetExtent()
	midPt=(.5*(bb[0]+bb[1]),.5*(bb[2]+bb[3]),.5*(bb[4]+bb[5]))
	sizes=(bb[1]-bb[0],bb[3]-bb[2],bb[5]-bb[4])
	rad=.5*min(sizes)
	rep=Show()
	rep.Representation='Outline'
	rep.Visibility=0

	_ex=_last=ExtractSubset(VOI=ext,SampleRateI=flowStride,SampleRateJ=flowStride,SampleRateK=flowStride)

	tr=StreamTracer(SeedType='Point Source')
	tr.SeedType.Center=midPt
	tr.SeedType.Radius=rad
	tr.SeedType.NumberOfPoints=200
	tr.Vectors=['POINTS','flow (momentum density)']
	tr.MaximumStreamlineLength=max(sizes) # maximum extent
	RenameSource('Global flow',tr)

	rep=Show()
	rep.LineWidth=3.0
	rep.LookupTable=GetLookupTableForArray('flow (momentum density)',3,VectorMode='Magnitude',EnableOpacityMapping=1)
	rep.ColorArrayName=('POINT_DATA','flow (momentum density)')

if flowMeshFile:
	mesh=XMLUnstructuredGridReader(FileName=[flowMeshFile])
	RenameSource(flowMeshFile,mesh)
	mesh.CellArrayStatus=mesh.GetCellDataInformation().keys()
	try:
		_cl=Clip(ClipType='Box')
		_cl.ClipType.Bounds=flowBounds
		_cl.InsideOut=1
		_cl.Scalars = ['CELLS', 'color']
	except NameError: pass # flowBounds not defined, as flow/split were not exported
	rep=Show()
	rep.Representation='Surface'
	rep.Opacity=flowMeshOpacity

if tracesFile:
	traces=XMLPolyDataReader(FileName=[tracesFile])
	RenameSource(tracesFile,traces)
	traces.CellArrayStatus=traces.GetCellDataInformation().keys()
	rep=Show()

if sphereFiles:
	spheres=XMLUnstructuredGridReader(FileName=sphereFiles)
	readPointCellData(spheres)
	RenameSource(sphereFiles[0],spheres)
	# don't show glyphs for more than 5e4 spheres by default to avoid veeery sloooow rendering
	isBig=(spheres.CellData.Proxy.GetDataInformation().GetNumberOfCells()>5e4)
	# setup glyphs, but don't show those by default
	gl=Glyph(GlyphType='Sphere',ScaleMode='scalar',Scalars=['POINTS','radius'],MaskPoints=0,RandomMode=0,SetScaleFactor=1.0)
	gl.GlyphType.Radius=1.0
	rep=Show()
	rep.Visibility=(0 if isBig else 1)
	rep.ColorArrayName=('POINT_DATA','radius')
	rep.LookupTable=GetLookupTableForArray('radius',1)
	# make sphere points invisible
	SetActiveSource(spheres)
	rep=Show()
	rep.Visibility=(1 if isBig else 0)
	rep.ColorArrayName=('POINT_DATA','radius')
	rep.LookupTable=GetLookupTableForArray('radius',1)

if meshFiles:
	mesh=XMLUnstructuredGridReader(FileName=meshFiles)
	readPointCellData(mesh)
	RenameSource(meshFiles[0],mesh)
	rep=Show()
	rep.Opacity=0.2

if triFiles:
	tri=XMLUnstructuredGridReader(FileName=triFiles)
	readPointCellData(tri)
	RenameSource(triFiles[0],tri)
	rep=Show()
	# rep.Opacity=1.0

if conFiles:
	con=XMLPolyDataReader(FileName=conFiles)
	readPointCellData(con)
	RenameSource(conFiles[0],con)
	rep=Show()
	rep.Visibility=0
	# c2p=CellDatatoPointData(PassCellData=1)
	# rep=Show()
	# rep.Visibility=0
	# tub=Tube(Scalars=['POINTS','Fn'] # not yet finished



GetRenderView().CenterAxesVisibility=0
Render()
'''

