print "Loading modules..."

from evjs import Joystick
from rpitx import gettx, oqpsktx, ooktx2
from leds import LEDs
import time
import os

try:
  # set highest priority
  os.nice(-20)
except OSError:
  # not running as root
  pass 

print "Initializing hardware..."

j = Joystick()
tx = gettx(carrier=32000, bw=1000, samp_rate=192000, block=oqpsktx)
leds = LEDs(j)

print "Fish control started."

delay = 0.2
try:
  while True:
    j.scan()
    #print j
    leds.go()
    tx.send('a_hi' +  j.toString() + 'x')
    #time.sleep(delay)
except KeyboardInterrupt:
  print "Fish control ended."
  leds.end()
  
