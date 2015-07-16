#!/bin/python

import numpy as np
from scipy.signal import decimate
from rtlsdr import RtlSdr, limit_calls
from codes import manchester, codes154, codes2corr

class Demod:
  SAMP_RATE = 1024e3
  SAMP_WINDOW = 1024*40

  def __init__(self, carrier=32000, bw=1000, sps=8, codes=manchester):
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

    ts = np.arange(Demod.SAMP_WINDOW)/Demod.SAMP_RATE
    self.mixer = np.exp(-2*np.pi*carrier*ts*1j)

    decim = Demod.SAMP_RATE/self.bw/self.sps
    assert decim == int(decim)
    self.decim = int(decim)
    assert Demod.SAMP_WINDOW % self.decim == 0

    self.corr = codes2corr(codes, sps)
    self.codelen = len(self.corr[0])

    self.last = np.zeros(Demod.SAMP_WINDOW)
    self.index = 0

  def bb2c(self, baseband):
    mag = np.abs(baseband)
    phase = np.angle(baseband)
    dp = np.ediff1d(np.unwrap(phase))
    return mag[1:], phase[1:], dp

  def decode(self, chips):
    corrs = []
    for c in self.corr:
      corrs.append(np.correlate(chips, c))
    return corrs

  def ddc(self, samp, sdr):
    assert len(samp) == len(self.mixer)
    overlap = self.decim * self.codelen
    print self.codelen
    print overlap
    print len(samp)
    print 1/0
    extsamp = np.concatenate((self.last[-2*overlap:], samp))
    self.last = samp
    baseband = decimate(extsamp * self.mixer, self.decim, ftype='fir')
    st = int(self.codelen/2)
    end = st+len(samp)+self.codelen
    mag, phase, dp  = self.bb2c(baseband[st:end])
    corrs = self.decode(mag)

    self.chips[self.index*len(chips):(self.index+1)*len(chips)] = chips
    self.chips[self.index*len(chips):(self.index+1)*len(chips)] = corrs[0]
    self.index += 1

  def run(self, limit):
    self.chips = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)
    self.demod = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)

    @limit_calls(limit)
    def callback(samp, sdr):
      self.ddc(samp, sdr)

    self.sdr.read_samples_async(callback, Demod.SAMP_WINDOW)

if __name__ == "__main__":
  d = Demod(carrier=10000, bw=100, sps=4, codes = manchester)
  d.run(1)

  import matplotlib.pyplot as plt
  p = plt.plot(d.chips)
  plt.show()

  '''
  dx = len(d.corr[0]*8)
  head = np.zeros(len(d.chips)-dx)
  for i in range(len(head)):
    head[i] = sum(d.chips[i:8*dx:dx])

  p = plt.plot(head)
  plt.show()
  '''
