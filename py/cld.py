import woo.config
if 'cldem' not in woo.config.features: raise ImportError("Compiled without the 'cldem' feature")
# intially empty, filled at initialization
