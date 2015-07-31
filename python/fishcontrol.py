print "Loading modules (1)..."

from evjs import Joystick
from leds import LEDs

j = Joystick()
leds = LEDs(j)

leds.go(1, color=0xffaa00)
print "Loading modules (2)..."

from rpitx import gettx, oqpsktx, ooktx2, ooktx
import time
import os, sys, select

try:
  # set highest priority
  os.nice(-20)
except OSError:
  # not running as root
  pass 

leds.go(1, color=0xffaa)
print "Initializing hardware..."

tx = gettx(carrier=32000, bw=1000, samp_rate=192000)
#tx = gettx(carrier=32000, bw=1000, samp_rate=192000, block=ooktx2)
#tx = gettx(carrier=32000, bw=1000, samp_rate=192000, block=oqpsktx)

leds.go(1, color=0xff00)
print "Fish control started."

delay = 0.2
count = 1
loop = True

try:
  while loop:
    j.scan()
    #print j
    stateNum = j.toStateNum()
    wavFile = 'wavFiles/' + '{0:04d}'.format(stateNum) + '.wav'
        
    leds.go(count)
    #tx.send('a_h' + chr(count & 0xff) + j.toString() + 'x')
    count += 1
    if select.select([sys.stdin], [], [], 0)[0]:
      loop = False
    time.sleep(delay)
except KeyboardInterrupt:
  pass

print "Fish control ended."
leds.end()



    












  
