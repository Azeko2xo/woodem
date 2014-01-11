import woo.config
if 'sparc' not in woo.config.features: raise ImportError("Compiled without the 'sparc' feature.")

