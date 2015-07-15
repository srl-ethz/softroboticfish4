#!/bin/python

import numpy as np
from scipy.signal import decimate
from rtlsdr import RtlSdr, limit_calls

class Demod:
  SAMP_RATE = 1024e3
  SAMP_WINDOW = 1024*40

  def __init__(self, carrier=32000, bw=1000, sps=16, limit=100):
    self.sdr = RtlSdr()
    # Sampling rate
    self.sdr.rs = Demod.SAMP_RATE
    # Pins 4 and 5
    self.sdr.set_direct_sampling(2)
    # Center frequency
    self.sdr.fc = 0
    # I don't think this is used?
    self.sdr.gain = 1

    self.bw = bw
    self.sps = sps
    self.limit = limit

    ts = np.arange(Demod.SAMP_WINDOW)/Demod.SAMP_RATE
    self.mixer = np.exp(-2*np.pi*carrier*ts*1j)

    decim = Demod.SAMP_RATE/self.bw/self.sps
    assert decim == int(decim)
    self.decim = int(decim)
    assert Demod.SAMP_WINDOW % self.decim == 0

    self.last = 0
    self.chips = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)
    self.index = 0

  def ddc(self, samp, sdr):
    assert len(samp) == len(self.mixer)
    baseband = samp * self.mixer
    baseband = decimate(baseband, self.decim)
    baseband = np.angle(baseband)
    chips = np.unwrap(np.ediff1d(baseband, to_begin=baseband[0]-self.last))
    chips[-1] = -7
    self.last = baseband[-1]

    self.chips[self.index*len(chips):(self.index+1)*len(chips)] = chips
    self.index += 1

  def run(self):
    @limit_calls(self.limit)
    def callback(samp, sdr):
      self.ddc(samp, sdr)

    self.sdr.read_samples_async(callback, Demod.SAMP_WINDOW)

d = Demod()
d.run()

import matplotlib.pyplot as plt
p = plt.plot(d.chips)
plt.show()

