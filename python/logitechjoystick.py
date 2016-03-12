# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
from evdev import InputDevice, categorize, ecodes, list_devices, KeyEvent
from hamming import encode
import time
import traceback

'''
305 A
306 B
304 X
307 Y
308 LB
310 LT
309 RB
311 RT
312 back
313 start

1.1 up
1.2 down
0.1 left
0.2 right
'''
"""
class KeyEvent:
  def __init__(self):
    self.lastts = None
    self.lastval = None
    self.totalts = 0
    self.numpush = 0
    self.lastPushTime = -1

  def update(self, event):
    retval = False
    value = event.value
    if event.type == ecodes.EV_ABS:
      value = 0 if abs(event.value - 127)<10 else 1

    timeSincePush = time.time()*1000.0-self.lastPushTime # ms since button was pressed
                                                         # will filter out spurious presses
                                                         # (which show as button presses less than 1 ms long)
    if self.lastts and self.lastval > 0 and value == 0 and timeSincePush > 50:
      self.numpush += 1
      self.totalts += event.timestamp() - self.lastts
      retval = True
      #print time.time()*1000.0-self.lastpushtime

    if value == 0:
      self.lastval = 0
    else:
      self.lastval = 1
      self.lastPushTime = time.time()*1000.0
    self.lastts = event.timestamp()
    return retval

  def __repr__(self):
    return "%d : %f" % (self.numpush, self.totalts)
"""
class Channel:
  def __init__(self, minval, maxval, bits, upkey, dnkey):
    self.minval = minval
    self.maxval = maxval
    self.bits = bits
    self.upkey = upkey
    self.dnkey = dnkey

    self.value = 0

  def press(self, key):
    if key == self.upkey:
      self.value = min(self.maxval, self.value + 1)
    elif key == self.dnkey:
      self.value = max(self.minval, self.value - 1)

  def toBits(self):
    s = "{:0%db}" % self.bits
    b = s.format(self.value & ((1 << self.bits) - 1))
    return map(int, b)

class Joystick:
  def __init__(self, device=0, fake=False):
    self.keys = {}
    if fake:
      self.dev = None
    else:
      self.dev = InputDevice(list_devices()[device])

    self.S = Channel( 0, 1, 1, 312, 313) # select, start
    self.P = Channel(-3, 3, 3, 1.1, 1.2) # up, down
    self.Y = Channel(-3, 3, 3, 0.2, 0.1) # left, right
    self.T = Channel( 0, 3, 2, 304, 306) # X, B
    self.F = Channel( 0, 3, 2, 305, 307) # A, Y
    self.spytf = (self.S, self.P, self.Y, self.T, self.F)

    # Create keys that aren't created automatically on startup
    self.keys[312] = KeyEvent()
    self.keys[313] = KeyEvent()
    self.keys[304] = KeyEvent()
    self.keys[306] = KeyEvent()
    self.keys[305] = KeyEvent()
    self.keys[307] = KeyEvent()

  def __repr__(self):
    return "S: %d, P: %d, Y: %d, T: %d, F: %d" % tuple([x.value for x in self.spytf])

  def setBits(self, bits):
    index = 0
    for k in self.spytf:
      val = int("".join(map(str, bits[index:index + k.bits])), 2)
      if val > 3:
        val = val - 8
      k.value = val
      index += k.bits
    
  def toBits(self, encodeData=True):
    bits = []
    for k in self.spytf:
      bits += k.toBits()
    if encodeData:
      return encode(bits)
    else:
      return bits

  def toStateNum(self):
    stateNum = self.S.value
    stateNum += (self.P.value + 3) << 1
    stateNum += (self.Y.value + 3) << 4
    stateNum += (self.T.value) << 7
    stateNum += (self.F.value) << 9

    return stateNum

  def toString(self):
    bits = "".join(map(str, self.toBits()))
    c1 = chr(int(bits[0:8],2))
    c2 = chr(int(bits[8:16],2))
    return c1+c2

  def pressed(self, key):
    for k in self.spytf:
      k.press(key)

  def scan(self):
    try:
        for event in self.dev.read():
            if event.type == ecodes.EV_KEY:
                keyevent = categorize(event)
                print 'KEY: %s (%s)' %  (keyevent.keycode, ('down' if keyevent.keystate == KeyEvent.key_down else 'up'))
            elif event.type == ecodes.EV_ABS:
                print 'ABS: %s (%s)' % (categorize(event).keycode, str(event.value))
    except IOError:
        pass
    return
    try:
      for event in self.dev.read():
        key = event.code
	print 'see key: %s' % str(key)
        if event.type == ecodes.EV_ABS:
          if abs(event.value - 127) < 10:
            try:
              if self.keys[key + 0.1].update(event):
                self.pressed(key + 0.1)
            except KeyError:
              self.keys[key + 0.1] = KeyEvent()
            try:
              if self.keys[key + 0.2].update(event):
                self.pressed(key + 0.2)
            except KeyError:
              self.keys[key + 0.2] = KeyEvent()
          key += 0.1 if event.value < 128 else 0.2
        if event.type in [ecodes.EV_ABS, ecodes.EV_KEY]:
          try:
            if self.keys[key].update(event):
              self.pressed(key)
          except KeyError:
            self.keys[key] = KeyEvent()
            #print traceback.format_exc()
    except IOError:
	print 'no event'
	pass

if __name__ == "__main__":
  import time, pprint

  j = Joystick()
  while True:
    j.scan()
    #print repr(j)
    #print j.toBits()
    #print j.toString()
    time.sleep(0.5)

