import pygame
pygame.init()
pygame.joystick.init()
j = pygame.joystick.Joystick(0)
j.init()

from contextlib import contextmanager
import sys, os
import time

@contextmanager
def suppress_stdout():
    with open(os.devnull, "w") as devnull:
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        sys.stdout = devnull
        sys.stderr = devnull
        try:  
            yield
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr

try:
  while True:
    pygame.event.pump()
    with suppress_stdout():
      k = [0] * j.get_numbuttons()
      for i in range(len(k)):
        k[i] = j.get_button(i)
    print k
    for i in range(j.get_numaxes()):
      with suppress_stdout():
        k = j.get_axis(i)
      print k
    print
    print "----"
    time.sleep(1)
except:
  raise
