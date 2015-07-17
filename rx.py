#!/bin/python

import numpy as np
from scipy.signal import decimate
from rtlsdr import RtlSdr, limit_calls
from codes import manchester, codes154, codes2corr
import matplotlib.pyplot as plt

import sys, select

class Demod:
  SAMP_RATE = 256000.
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

    ts = np.arange(Demod.SAMP_WINDOW*2)/Demod.SAMP_RATE
    self.mixer = np.exp(-2*np.pi*carrier*ts*1j)

    decim = Demod.SAMP_RATE/self.bw/self.sps
    assert decim == int(decim)
    self.decim = int(decim)
    assert Demod.SAMP_WINDOW % self.decim == 0

    self.sampchips = Demod.SAMP_WINDOW / self.decim 
    self.corr = codes2corr(codes, sps)
    self.codelen = len(self.corr[0])

    self.last = np.zeros(Demod.SAMP_WINDOW)

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
    extsamp = np.concatenate((self.last, samp))
    self.last = samp
    #baseband = decimate(extsamp * self.mixer, self.decim, ftype='fir')
    baseband = np.sum(np.reshape(extsamp * self.mixer, (-1,self.decim)), 1)
    mag, phase, dp  = self.bb2c(baseband)
    corrs = self.decode(mag)

    self.chips[self.index*self.sampchips:(self.index+1)*self.sampchips] = mag[self.codelen:self.codelen+self.sampchips]
    self.demod[self.index*self.sampchips:(self.index+1)*self.sampchips] = corrs[0][self.codelen:self.codelen+self.sampchips]
    self.index += 1

  def end(self):
    self.sdr.close()

  def run(self, limit):
    self.chips = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)
    self.demod = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)
    self.index = 0

    @limit_calls(limit)
    def callback(samp, sdr):
      if select.select([sys.stdin], [], [], .000001)[0]:
        sdr.cancel_read_async()
      self.ddc(samp, sdr)

    self.sdr.read_samples_async(callback, Demod.SAMP_WINDOW)
    print self.index, "samples read"
    sys.stdout.flush()

  def plot(self):
    fig = plt.figure()
    ax1 = fig.add_subplot(211)
    ax1.plot(self.chips)
    ax2 = fig.add_subplot(212, sharex=ax1)
    ax2.plot(self.demod)
    plt.show()

  def checkdemod(self, index, demod=None, packetlen=5):
    if demod is None:
      demod = self.demod
    l = packetlen*8+6+7
    b = self.demod[index:index+l*self.codelen:self.codelen]
    return chipsToString(np.concatenate(([-1,1], b, [1])))

  def findstring(self, demod = None, packetlen=5):
    if demod is None:
      demod = self.demod
    find = {}
    for i in range(len(demod)):
      s, c = self.checkdemod(i, demod, packetlen)
      if len(s) and s[0] == 'a' and s[-1] == 'x':
        find[i] = s[1:-1]
    return find

def chipsToString(bits):
  char = 0
  rxstr = ""
  for i, b in enumerate(bits):
    char += 0 if b > 0 else (1 << (i%8))
    if i % 8 == 7:
      rxstr += chr(char)
      char = 0
  return rxstr, char
  
if __name__ == "__main__":
  d = Demod(carrier=32000, bw=1000, sps=4, codes=manchester)
  d.run(50)
  d.findstring()
