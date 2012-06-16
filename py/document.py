

def packageClasses(outDir='.'):
	'''Generate documentation of packages in the Restructured Text format. Each package is written to file called *out*/.`woo.[package].rst` and list of files created is returned.'''
	allClasses=yade.core.Serializable._derivedCxxClasses
	# create class tree; top-level nodes are packages
	# each level of child nodes is section in the documentation, as requested by ClassTraits of each class

