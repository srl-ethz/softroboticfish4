import time, pprint
from evdev import InputDevice, categorize, ecodes, list_devices

dev = InputDevice(list_devices()[0])

class KeyEvent:
  def __init__(self):
    self.lastts = None
    self.lastval = None
    self.totalts = 0
    self.numpush = 0

  def update(self, event):
    value = event.value
    if event.type == ecodes.EV_ABS:
      value = 0 if abs(event.value - 127)<10 else 1

    if self.lastts and self.lastval == 1 and value == 0:
      self.numpush += 1
      self.totalts += event.timestamp() - self.lastts
    self.lastval = value
    self.lastts = event.timestamp()

  def __repr__(self):
    return "%d : %f" % (self.numpush, self.totalts)

keys = {}

while True:
  try:
    for event in dev.read():
      key = event.code
      if event.type == ecodes.EV_ABS:
        if abs(event.value - 127) < 10:
          try:
            keys[key + 0.1].update(event)
          except KeyError:
            keys[key + 0.1] = KeyEvent()
          try:
            keys[key + 0.2].update(event)
          except KeyError:
            keys[key + 0.2] = KeyEvent()
        key += 0.1 if event.value < 128 else 0.2
      if event.type in [ecodes.EV_ABS, ecodes.EV_KEY]:
        try:
          keys[key].update(event)
        except KeyError:
          keys[key] = KeyEvent()
      pprint.pprint(keys)
  except IOError:
    pass

  time.sleep(0.01)
