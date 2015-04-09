import apa102
import time

'''
LEDs:
  status
  status
  T up
  F up
  T down
  F down
  S
  Y up 
  P down
  Y down
  P up
'''

class LEDs:
  def __init__(self, j=None):
    self.j = j
    self.count = 0
    self.strip = apa102.APA102(11)
    self.strip.clearStrip()

  def setvalue(self, pixel, value):
    if value == 3:
      self.strip.setPixelRGB(pixel, 0xff0000)
    elif value == 2:
      self.strip.setPixelRGB(pixel, 0xff00)
    elif value == 1:
      self.strip.setPixelRGB(pixel, 0xff)
    else:
      self.strip.setPixelRGB(pixel, 0)

  def go(self):
    self.count += 1

    if self.count % 4 < 2:
      self.strip.setPixelRGB(0, 0xffffff)
    else:
      self.strip.setPixelRGB(0, 0)

    if (self.count-1) % 4 < 2:
      self.strip.setPixelRGB(1, 0xffffff)
    else:
      self.strip.setPixelRGB(1, 0)

    self.setvalue(2, self.j.T.value)
    self.setvalue(3, self.j.F.value)
    self.setvalue(4, self.j.T.value)
    self.setvalue(5, self.j.F.value)
    self.setvalue(6, self.j.S.value)
    self.setvalue(7, self.j.Y.value)
    self.setvalue(8, -self.j.P.value)
    self.setvalue(9, -self.j.Y.value)
    self.setvalue(10, self.j.P.value)
    self.strip.show()

  def end(self):
    self.strip.clearStrip()
    time.sleep(0.1)
    self.strip.clearStrip()

