from gnuradio import analog
from gnuradio import audio
from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from math import sin, pi, log
from optparse import OptionParser

import codes

def topblock(self, carrier=32000, samp_rate = 80000, bw=1000, amp=1):
    gr.top_block.__init__(self, "Top Block")

    ##################################################
    # Variables
    ##################################################
    self.samp_rate = samp_rate 
    self.carrier = carrier 
    self.bw = bw

    self.source = blocks.vector_source_b((0,0), False, 1, [])

    #XXX Hack: 0.07 should actually be parameter amp, but RPI crashes
    self.sink = blocks.multiply_vcc(1)
    analog_sig_source_x_0 = analog.sig_source_c(samp_rate, analog.GR_COS_WAVE, carrier, 0.2, 0)
    blocks_complex_to_real_0 = blocks.complex_to_real(1)
    stereo = blocks.multiply_const_vff((-1, ))
    audio_sink_0 = audio.sink(samp_rate, "")

    ##################################################
    # Connections
    ##################################################
    self.connect((analog_sig_source_x_0, 0), (self.sink, 1))
    self.connect((self.sink, 0), (blocks_complex_to_real_0, 0))
    self.connect((blocks_complex_to_real_0, 0), (audio_sink_0, 0))
    self.connect((blocks_complex_to_real_0, 0), (stereo, 0))
    self.connect((stereo, 0), (audio_sink_0, 1))

def ooktx(self, carrier=32000, samp_rate = 80000, bw=1000, amp=1):
    topblock(self, carrier, samp_rate, bw, amp)

    ##################################################
    # Blocks
    ##################################################
    blocks_packed_to_unpacked_xx_0 = blocks.packed_to_unpacked_bb(1, gr.GR_LSB_FIRST)
    digital_chunks_to_symbols_xx_0 = digital.chunks_to_symbols_bc(([0,1,1,0]), 2)

    blocks_repeat_0 = blocks.repeat(gr.sizeof_gr_complex*1, samp_rate/bw)
    
    ##################################################
    # Connections
    ##################################################
    self.connect((self.source, 0), (blocks_packed_to_unpacked_xx_0, 0))
    self.connect((blocks_packed_to_unpacked_xx_0, 0), (digital_chunks_to_symbols_xx_0, 0))
    self.connect((digital_chunks_to_symbols_xx_0, 0), (blocks_repeat_0, 0))

    self.connect((blocks_repeat_0, 0), (self.sink, 0))

def ooktx2(self, carrier=32000, samp_rate = 80000, bw=1000, amp=1):
    topblock(self, carrier, samp_rate, bw, amp)

    ##################################################
    # Blocks
    ##################################################
    blocks_packed_to_unpacked_xx_0 = blocks.packed_to_unpacked_bb(1, gr.GR_LSB_FIRST)
    digital_chunks_to_symbols_xx_0 = digital.chunks_to_symbols_bc((codes.codes2list(codes.mycode)), 2)

    blocks_repeat_0 = blocks.repeat(gr.sizeof_gr_complex*1, samp_rate/bw)
    
    ##################################################
    # Connections
    ##################################################
    self.connect((self.source, 0), (blocks_packed_to_unpacked_xx_0, 0))
    self.connect((blocks_packed_to_unpacked_xx_0, 0), (digital_chunks_to_symbols_xx_0, 0))
    self.connect((digital_chunks_to_symbols_xx_0, 0), (blocks_repeat_0, 0))

    self.connect((blocks_repeat_0, 0), (self.sink, 0))

CODE_LIST = codes.mycode
CODE_TABLE, CODE_LEN = codes.codes2table(CODE_LIST), len(CODE_LIST)
CHUNK_LEN = int(log(CODE_LEN,2))

def oqpsktx(self, carrier=10000, samp_rate = 80000, bw=4000, amp=1):
    topblock(self, carrier, samp_rate, bw, amp)

    ##################################################
    # Blocks
    ##################################################
    self.blocks_packed_to_unpacked_xx_0 = blocks.packed_to_unpacked_bb(CHUNK_LEN, gr.GR_LSB_FIRST)
    self.digital_chunks_to_symbols_xx_0 = digital.chunks_to_symbols_bc((CODE_TABLE), CODE_LEN)

    self.blocks_repeat_0 = blocks.repeat(gr.sizeof_gr_complex*1, 4)
    self.blocks_vector_source_x_0 = blocks.vector_source_c([0, sin(pi/4), 1, sin(3*pi/4)], True, 1, [])

    self.blocks_multiply_xx_0 = blocks.multiply_vcc(1)
    self.blocks_complex_to_float_0 = blocks.complex_to_float(1)
    self.blocks_delay_0 = blocks.delay(gr.sizeof_float*1, 2)

    self.blocks_float_to_complex_0 = blocks.float_to_complex(1)
    self.rational_resampler_xxx_0 = filter.rational_resampler_ccc(
            interpolation=int(samp_rate/bw),
            decimation=1,
            taps=None,
            fractional_bw=None,
    )

    ##################################################
    # Connections
    ##################################################
    self.connect((self.source, 0), (self.blocks_packed_to_unpacked_xx_0, 0))
    self.connect((self.blocks_packed_to_unpacked_xx_0, 0), (self.digital_chunks_to_symbols_xx_0, 0))
    self.connect((self.digital_chunks_to_symbols_xx_0, 0), (self.blocks_repeat_0, 0))

    self.connect((self.blocks_vector_source_x_0, 0), (self.blocks_multiply_xx_0, 0))
    self.connect((self.blocks_repeat_0, 0), (self.blocks_multiply_xx_0, 1))

    self.connect((self.blocks_multiply_xx_0, 0), (self.blocks_complex_to_float_0, 0))
    self.connect((self.blocks_complex_to_float_0, 0), (self.blocks_float_to_complex_0, 0))
    self.connect((self.blocks_complex_to_float_0, 1), (self.blocks_delay_0, 0))
    self.connect((self.blocks_delay_0, 0), (self.blocks_float_to_complex_0, 1))

    self.connect((self.blocks_float_to_complex_0, 0), (self.rational_resampler_xxx_0, 0))
    self.connect((self.rational_resampler_xxx_0, 0), (self.sink, 0))
