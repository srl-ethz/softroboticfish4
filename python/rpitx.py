#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Top Block
# Generated: Fri Jul 10 03:11:37 2015
##################################################

from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from optparse import OptionParser
from txblocks import ooktx, oqpsktx, ooktx2

class RPITX(gr.top_block):
    def __init__(self, carrier, samp_rate, bw, amp, block):
        block(self, carrier, samp_rate, bw, amp)

    def send(self, txstr):
        bytes = tuple(bytearray(txstr))
        print bytes 
        self.source.set_data(tuple(bytearray(txstr)), [])
        self.start()
        self.wait()

MAX_RETRIES = 100
def gettx(carrier=32000, samp_rate=192000, bw=1000, amp=1, block=ooktx):
    for i in range(MAX_RETRIES):
        try:
            tx = None
            tx = RPITX(carrier, samp_rate, bw, amp, block)
            tx.send("Hi")
        except:
            print "try %d failed" % i
            del(tx)
        else:
            return tx

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    parser.add_option("-e", "--modulation", dest="modstr", default="ooktx",
                      help="use MOD modulation", metavar="MOD")
    parser.add_option("-m", "--message", dest="txstr", default="Hello World!",
                      help="set tx string MSG", metavar="MSG")
    parser.add_option("-c", "--carrier", dest="carrier", default=10000, type="int",
                      help="set carrier to FREQ (Hz)", metavar="FREQ")
    parser.add_option("-s", "--samprate", dest="samp_rate", default=80000, type="int",
                      help="set sample rate to RATE (Hz)", metavar="RATE")
    parser.add_option("-b", "--bandwidth", dest="bw", default=4000, type="int",
                      help="set bandwidth to BW (Hz)", metavar="BW")
    parser.add_option("-a", "--amplitude", dest="amp", default=1, type="float",
                      help="set amplitude to AMP", metavar="AMP")

    (options, args) = parser.parse_args()
    if options.modstr.lower() == "ooktx":
        block = ooktx
    elif options.modstr.lower() == "oqpsktx":
        block = oqpsktx
    else:
        raise ValueError("Unknown modulation scheme")

    tx = gettx(options.carrier, options.samp_rate, options.bw, options.amp, block)
    tx.send(options.txstr)

