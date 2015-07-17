#!/bin/python

import numpy as np
from scipy.signal import decimate
from rtlsdr import RtlSdr, limit_calls
from codes import manchester, codes154, codes2corr
import matplotlib.pyplot as plt

import sys, select

def enum(*sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    return type('Enum', (), enums)

Mods = enum("MAGNITUDE", "PHASE", "DPHASE")

class Demod:
  SAMP_RATE = 256000.
  SAMP_WINDOW = 1024*40

  def __init__(self, carrier=32000, bw=1000, sps=8, codes=manchester, mod=Mods.MAGNITUDE):
    self.mod = mod

    ts = np.arange(Demod.SAMP_WINDOW*2)/Demod.SAMP_RATE
    self.mixer = np.exp(-2*np.pi*carrier*ts*1j)

    decim = Demod.SAMP_RATE/bw/sps
    assert decim == int(decim)
    self.decim = int(decim)
    assert Demod.SAMP_WINDOW % self.decim == 0

    self.sampchips = Demod.SAMP_WINDOW / self.decim 
    self.corr = codes2corr(codes, sps)
    self.codelen = len(self.corr[0])

    self.sdr = RtlSdr()
    # Sampling rate
    self.sdr.rs = Demod.SAMP_RATE
    # Pins 4 and 5
    self.sdr.set_direct_sampling(2)
    # Center frequency
    self.sdr.fc = 0
    # I don't think this is used?
    self.sdr.gain = 1

  def bb2c(self, baseband):
    mag = np.abs(baseband)
    phase = np.angle(baseband)
    dp = np.mod(np.ediff1d(phase)+np.pi, 2*np.pi)-np.pi
    return mag[1:], phase[1:], dp

  def decode(self, chips):
    corrs = []
    for c in self.corr:
      corrs.append(np.correlate(chips, c))
    maxes = np.max(np.array(corrs), 0)
    codes = np.argmax(np.array(corrs), 0)
    return maxes, codes

  def ddc(self, samp, sdr):
    extsamp = np.concatenate((self.last, samp))
    self.last = samp
    #baseband = decimate(extsamp * self.mixer, self.decim, ftype='fir')
    baseband = np.sum(np.reshape(extsamp * self.mixer, (-1,self.decim)), 1)
    mag, phase, dp  = self.bb2c(baseband)
    if self.mod == Mods.PHASE:
      sig = phase
    elif self.mod == Mods.DPHASE:
      sig = dp
    else:
      sig = mag
    corrs, codes = self.decode(sig)

    self.chips[self.index*self.sampchips:(self.index+1)*self.sampchips] = sig[self.codelen:self.codelen+self.sampchips]
    self.corrs[self.index*self.sampchips:(self.index+1)*self.sampchips] = corrs[self.codelen:self.codelen+self.sampchips]
    self.demod[self.index*self.sampchips:(self.index+1)*self.sampchips] = codes[self.codelen:self.codelen+self.sampchips]
    self.index += 1

  def end(self):
    self.sdr.close()

  def run(self, limit):
    self.last = np.zeros(Demod.SAMP_WINDOW)
    self.chips = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)
    self.corrs = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)
    self.demod = np.zeros(limit * Demod.SAMP_WINDOW / self.decim)
    self.index = 0

    @limit_calls(limit)
    def callback(samp, sdr):
      if select.select([sys.stdin], [], [], 0)[0]:
        sdr.cancel_read_async()
      self.ddc(samp, sdr)

    self.sdr.read_samples_async(callback, Demod.SAMP_WINDOW)
    print self.index, "samples read"
    sys.stdout.flush()

  def plot(self):
    plt.ion()
    fig = plt.figure()
    ax1 = fig.add_subplot(311)
    ax1.plot(self.chips)
    ax2 = fig.add_subplot(312, sharex=ax1)
    ax2.plot(self.corrs)
    ax3 = fig.add_subplot(313, sharex=ax1)
    ax3.plot(self.demod)
    plt.show()

  def checkdemod(self, index, demod=None, packetlen=5):
    if demod is None:
      demod = self.demod
    l = packetlen*8+6+7
    b = self.demod[index:index+l*self.codelen:self.codelen]
    return chipsToString(np.concatenate(([1,0], b, [0])))

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
    char += (1 << (i%8)) if b else 0
    if i % 8 == 7:
      rxstr += chr(char)
      char = 0
  return rxstr, char
  
if __name__ == "__main__":
  d = Demod(carrier=32000, bw=1000, sps=4, codes=manchester)
  d.run(50)
  d.findstring()
