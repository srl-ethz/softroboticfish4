#!/bin/python

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from rtlsdr import RtlSdr

sdr = RtlSdr()
# Sampling rate
sdr.rs = 1024e3
# Pins 4 and 5
sdr.set_direct_sampling(2)

# Center frequency
sdr.fc = 0
# I don't think this is used?
sdr.gain = 1

fig = plt.figure()
ax = fig.add_subplot(211, autoscale_on=False, xlim=(-1, 513), ylim=(-1.1,1.1))
ax.grid()
rline, = ax.plot([], [], 'r-', lw=2)
iline, = ax.plot([], [], 'g-', lw=2)
ax = fig.add_subplot(212, autoscale_on=False, xlim=(-1, 513), ylim=(-1.3,1.3))
ax.grid()
mline, = ax.plot([], [], 'b-', lw=2)

def animate(i):
    samples = sdr.read_samples(1024)
    try:
      zc = np.where(np.diff(np.sign(samples))>0)[0][0]
    except:
      zc = 0

    try:
      samples = samples[zc:zc+512]
    except:
      samples = samples[0:512]

    rline.set_data(range(len(samples)), np.real(samples))
    iline.set_data(range(len(samples)), np.imag(samples))
    mline.set_data(range(len(samples)), np.abs(samples) * np.sign(np.angle(samples)))

t = 100
ani = animation.FuncAnimation(fig, animate, t,
    interval=50)

plt.show()

