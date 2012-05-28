# vim: set filetype=python:
Import('*')

import yadeSCons
yadeSCons.buildPluginLibs(env,env['buildPlugs'])
SConscript(dirs=['core','lib','gui','py','resources'],duplicate=0)

#install preprocessor scripts
env.Install('$LIBDIR/py/yade/pre',
	env.File(env.Glob('pkg/pre/*.py'),'pkg/pre'),
)
