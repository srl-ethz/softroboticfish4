import fileLock
import apa102
import os
import time

class Heartbeat:
  
  def __init__(self):
    self.leds = apa102.APA102(11)
    self.leds.clearStrip()
    self.leds.show()
    self.fishFile = 'fishAlive.txt'
    print 'init heartbeat'
    self.oldFishValue = None
    if not os.path.exists(self.fishFile):
      touch = open(self.fishFile, 'w+')
      touch.write('0')
      touch.close()
      os.chmod(self.fishFile, 111)
      self.oldFishValue = 0
      self.newFishValue = 0
    else:
      self.readFishValue()
    print 'init done'
    time.sleep(1)

  def readFishValue(self):
    if not fileLock.acquireLock(self.fishFile, timeout=2):
      return False
    fin = open(self.fishFile, 'r')
    self.newFishValue = int(fin.read())
    fin.close()
    fileLock.releaseLock(self.fishFile)
    return True

  def run(self):
    brightness = 1
    increment = 1
    increasing = True
    count = 0
    controllerRunning = True
    checkInterval = 5000
    delay = 75
    print 'running'
    try:
      while(True):
        # If value has not changed, then fish control is not running
        if count > checkInterval/delay:
          if (not self.readFishValue()) or (self.oldFishValue == self.newFishValue):
            controllerRunning = False
          else:
            controllerRunning = True
            self.oldFishValue = self.newFishValue
          count = 0
        # If not running, show heartbeat
        if not controllerRunning:
          self.leds.setPixel(0, 25, 0, brightness/2)
          self.leds.show()
          brightness += increment * (1 if increasing else -1)
          if brightness > 20:
            increasing = False
          if brightness < (increment+1):
            increasing = True
        time.sleep(float(delay)/1000.0)
        count += 1
    except KeyboardInterrupt:
      fileLock.releaseLock(self.fishFile)

if __name__ == '__main__':
  try:
    # increase priority
    os.nice(-5)
  except OSError:
    # not running as root
    pass

  heartbeat = Heartbeat()
  heartbeat.run()
    
