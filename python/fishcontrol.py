from evjs import Joystick
#from rpitx import gettx, oqpsktx, ooktx2
from leds import LEDs
import time

delay = 0.2

j = Joystick()
#tx = gettx(carrier=32000, bw=1000, samp_rate=192000)
leds = LEDs(j)

try:
  while True:
    j.scan()
    print j
    #tx.send('a_hi' +  j.toString() + 'x')
    leds.go()
    time.sleep(delay)
except KeyboardInterrupt:
  leds.end()
  
