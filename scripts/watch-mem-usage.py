# collect data about max memory usage of processes matching some patterns
import psutil, re, operator, time, sys
sampleTime=.5 # seconds
# matching lines will be taken in account
#cmdPattern='.*cc1plus.*src/([a-zA-Z0-9_-]\.cpp).*'
cmdPattern=r'.*cc1plus.* (\S+/)?([^/ ]+\.cpp).*'

# group in the pattern which identifies the process (e.g. source code file)
cmdKeyGroup=2

maxMem={}

while True:
	try:
		for p in psutil.process_iter():
			m=re.match(cmdPattern,' '.join(p.cmdline))
			if not m: continue
			key=m.group(cmdKeyGroup)
			mem=p.get_memory_info()[1] # tuple of RSS (resident set size) and VMS (virtual memory size)
			if key not in maxMem:
				print 'New process with key',key
				maxMem[key]=mem
			elif maxMem[key]<mem: maxMem[key]=mem
		time.sleep(sampleTime)
	except (KeyboardInterrupt,SystemExit):
		# print summary, exit
		for k,v in sorted(maxMem.iteritems(),key=operator.itemgetter(1)):
			print '{:>10.1f}  {}'.format(1e-6*v,k)
		sys.exit(0)



