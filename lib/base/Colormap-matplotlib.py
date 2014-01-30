from pylab import *
maps=[m for m in cm.datad if not m.endswith("_r")]
maps.sort()
out=[]
# colormaps we want; if empty, all will be output
wanted=set(['coolwarm',])
for m in maps:
	if wanted and m not in wanted: continue
	ff=list(get_cmap(m)(arange(256))[:,:3].flatten())
	out.append('\t\tColormap{"%s",{%s}}'%(m,','.join(['%.4g'%f for f in ff])))
print ',\n'.join(out)
