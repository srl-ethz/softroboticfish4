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

    def __init__(self, carrier=10000, samp_rate = 80000, amp=1):
        gr.top_block.__init__(self, "Top Block")

        ##################################################
        # Blocks
        ##################################################
        self.audio_sink_0 = audio.sink(samp_rate)
        # XXX Hack: Should be amp instead of hardcoded 0.07, but it errors out on that
        self.analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_COS_WAVE, carrier, 0.07, 0)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_sig_source_x_0, 0), (self.audio_sink_0, 0))

def send(carrier, samp_rate, amp):
    tb = top_block(carrier, samp_rate, amp)
    tb.start()
    tb.wait()

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    parser.add_option("-c", "--carrier", dest="carrier", default=10000, type="int",
                      help="set carrier to FREQ (Hz)", metavar="FREQ")
    parser.add_option("-s", "--samprate", dest="samp_rate", default=80000, type="int",
                      help="set sample rate to RATE (Hz)", metavar="RATE")
    parser.add_option("-a", "--amplitude", dest="amp", default=1, type="float",
                      help="set amplitude to AMP", metavar="AMP")

    (options, args) = parser.parse_args()

    send(options.carrier, options.samp_rate, options.amp)

