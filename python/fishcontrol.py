print "Loading modules (1)..."

from evjs import Joystick
from leds import LEDs

j = Joystick()
leds = LEDs(j)

leds.go(1, color=0xffaa00)
print "Loading modules (2)..."

from rpitx import gettx, oqpsktx, ooktx2
import time
import os

try:
  # set highest priority
  os.nice(-20)
except OSError:
  # not running as root
  pass 

leds.go(1, color=0xffaa)
print "Initializing hardware..."

tx = gettx(carrier=32000, bw=1000, samp_rate=192000, block=oqpsktx)

leds.go(1, color=0xff00)
print "Fish control started."

delay = 0.2
count = 1
try:
  while True:
    j.scan()
    #print j
    leds.go(count)
    tx.send('a_hi' +  j.toString() + 'x')
    count += 1
    #time.sleep(delay)
except KeyboardInterrupt:
  print "Fish control ended."
  leds.end()
  