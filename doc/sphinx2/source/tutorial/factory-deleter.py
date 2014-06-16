# use the setup from the other file
execfile('factory-shooter.py')
# replace the generator
S.lab.feed.generator=PsdCapsuleGenerator(psdPts=[(.03,0),(.04,.8),(.06,1.)],shaftRadiusRatio=(.4,3.))
# increase its mass rate
S.lab.feed.massRate=10
# necessary for capsules... ?
S.dtSafety=0.4 
S.engines=S.engines+[
   BoxDeleter(
      box=((0,0,0),(1,1.1,1)),
      stepPeriod=100,
      save=True,      # keep track of diameters/masses of deleted particles, for PSD
      inside=False,   # delete particles outside the box, not inside
      label='out'
   )
]
S.saveTmp()

if 1:
   S.run(30000)
   S.wait()

   # plots PSDs
   import pylab
   pylab.plot(*S.lab.feed.generator.psd(),label='feed',lw=2)
   pylab.plot(*S.lab.out.psd(),label='out',lw=2)
   pylab.legend(loc='best')
   # pylab.show()
   pylab.savefig('factory-deleter_psds.svg')
   pylab.savefig('factory-deleter_psds.pdf')
