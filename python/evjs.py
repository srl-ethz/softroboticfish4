from evdev import InputDevice, categorize, ecodes, list_devices
from hamming import encode

'''
288 A
289 B
290 X
291 Y
292 L
293 R
294 select
295 start

1.1 up
1.2 down
0.1 left
0.2 right
'''

class KeyEvent:
  def __init__(self):
    self.lastts = None
    self.lastval = None
    self.totalts = 0
    self.numpush = 0

  def update(self, event):
    retval = False
    value = event.value
    if event.type == ecodes.EV_ABS:
      value = 0 if abs(event.value - 127)<10 else 1

    if self.lastts and self.lastval == 1 and value == 0:
      self.numpush += 1
      self.totalts += event.timestamp() - self.lastts
      retval = True

    self.lastval = value
    self.lastts = event.timestamp()
    return retval

  def __repr__(self):
    return "%d : %f" % (self.numpush, self.totalts)

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

    self.S = Channel( 0, 1, 1, 295, 294) # select, start
    self.P = Channel(-3, 3, 3, 1.1, 1.2) # up, down
    self.Y = Channel(-3, 3, 3, 0.2, 0.1) # left, right
    self.T = Channel( 0, 3, 2, 290, 289) # X, B
    self.F = Channel( 0, 3, 2, 288, 291) # A, Y
    self.spytf = (self.S, self.P, self.Y, self.T, self.F)

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
    
  def toBits(self):
    bits = []
    for k in self.spytf:
      bits += k.toBits()
    return encode(bits)

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
        key = event.code
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
    except IOError:
      pass

if __name__ == "__main__":
  import time, pprint

  j = Joystick()
  while True:
    j.scan()
    print repr(j)
    print j.toBits()
    print j.toString()
    time.sleep(0.5)

