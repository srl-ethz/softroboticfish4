#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Top Block
# Generated: Fri Jul 10 03:11:37 2015
##################################################

from gnuradio import analog
from gnuradio import audio
from gnuradio import gr
from gnuradio.eng_option import eng_option
from optparse import OptionParser

class top_block(gr.top_block):

    def __init__(self, txstr, carrier=10000, samp_rate = 80000, bw=1):
        gr.top_block.__init__(self, "Top Block")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate 
        self.carrier = carrier 

        ##################################################
        # Blocks
        ##################################################
        self.audio_sink_0 = audio.sink(samp_rate)
        # XXX Hack: Should be bw, but it errors out on that
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_COS_WAVE, carrier, 0.07, 0)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_sig_source_x_0, 0), (self.audio_sink_0, 0))


# QT sink close method reimplementation

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)

    def get_carrier(self):
        return self.carrier

    def set_carrier(self, carrier):
        self.carrier = carrier
        self.analog_sig_source_x_0.set_frequency(self.carrier)

def send(txstr, carrier, samp_rate, bw):
    tb = top_block(txstr, carrier, samp_rate, bw)
    tb.start()
    tb.wait()

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    parser.add_option("-m", "--message", dest="txstr", default="Hello World!",
                      help="set tx string MSG", metavar="MSG")
    parser.add_option("-c", "--carrier", dest="carrier", default=10000, type="int",
                      help="set carrier to FREQ (Hz)", metavar="FREQ")
    parser.add_option("-s", "--samprate", dest="samp_rate", default=80000, type="int",
                      help="set sample rate to RATE (Hz)", metavar="RATE")
    parser.add_option("-b", "--bandwidth", dest="bw", default=1, type="float",
                      help="set bandwidth to BW (Hz)", metavar="BW")

    (options, args) = parser.parse_args()

    send(options.txstr, options.carrier, options.samp_rate, options.bw)

