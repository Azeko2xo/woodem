import yade.config
if 'sparc' not in yade.config.features: raise ImportError("Compiled without the 'sparc' feature.")
