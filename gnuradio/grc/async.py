#!/bin/python

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from rtlsdr import RtlSdr, limit_calls
from multiprocessing import Process


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
ax = fig.add_subplot(211, autoscale_on=False, xlim=(-1, 129), ylim=(-1.1,1.1))
ax.grid()
rline, = ax.plot([], [], 'r-', lw=2)
iline, = ax.plot([], [], 'g-', lw=2)
ax = fig.add_subplot(212, autoscale_on=False, xlim=(-1, 129), ylim=(-1.3,1.3))
ax.grid()
mline, = ax.plot([], [], 'b-', lw=2)

global pdata
pdata = [0] * 128

@limit_calls(1000)
def callback(samples, sdr):
    global pdata
    pdata = list(pdata[-64:]) + list(samples[0:64])
    print "in callback", len(samples), samples[0], ":", pdata[0]

def animate(i):
    global pdata
    samples = pdata
    print "in animate", samples[0], ":", pdata[0]

    rline.set_data(range(len(samples)), np.real(samples))
    iline.set_data(range(len(samples)), np.imag(samples))
    mline.set_data(range(len(samples)), np.abs(samples) * np.sign(np.angle(samples)))

t = 100
ani = animation.FuncAnimation(fig, animate, t,
    interval=50)

p = Process(target=sdr.read_samples_async, args=(callback, 10240))
p.start()

plt.show()
p.join()
