
import sys
import os
import time

if len(sys.argv) < 2:
	raise AssertionError("Enter the filename as an argument")

filename = str(sys.argv[1]).strip()
count = 100
delay = 10
for arg in sys.argv:
	if 'count' in arg:
		count = int(arg.split('=')[1].strip())
	if 'delay' in arg:
		delay = int(arg.split('=')[1].strip())

print 'About to play ' + filename + " " + str(count) + " times..."

try:
	for i in range(count):
		print "\tplaying\t" + str(i)
		if os.system("aplay -q " + filename) != 0:
                        raise KeyboardInterrupt
		time.sleep(delay/1000)
except KeyboardInterrupt:
	print '\nDone!'
	
