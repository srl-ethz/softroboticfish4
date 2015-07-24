from evjs import Joystick
from rpitx import gettx, oqpsktx, ooktx2
import time

delay = 0.2

j = Joystick()
tx = gettx(carrier=32000, bw=1000, samp_rate=192000)

while True:
  j.scan()
  print j
  tx.send('a_hi' +  j.toString() + 'x')
  #time.sleep(delay)

