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

    self.sink = blocks.multiply_vcc(1)
    analog_sig_source_x_0 = analog.sig_source_c(samp_rate, analog.GR_COS_WAVE, carrier, amp, 0)
    blocks_complex_to_real_0 = blocks.complex_to_real(1)
    stereo = blocks.multiply_const_vff((-1, ))
    self.out = blocks.wavfile_sink("/tmp/outbits.wav", 2, samp_rate)

    ##################################################
    # Connections
    ##################################################
    self.connect((analog_sig_source_x_0, 0), (self.sink, 1))
    self.connect((self.sink, 0), (blocks_complex_to_real_0, 0))
    self.connect((blocks_complex_to_real_0, 0), (self.out, 0))
    self.connect((blocks_complex_to_real_0, 0), (stereo, 0))
    self.connect((stereo, 0), (self.out, 1))

def asktx(self, carrier=32000, samp_rate = 80000, bw=1000, amp=1, code=codes.mycode, balanced=False, **kwargs):
    code_len = len(code)
    chunk_len = int(log(code_len,2))

    topblock(self, carrier, samp_rate, bw, amp)

    ##################################################
    # Blocks
    ##################################################
    blocks_packed_to_unpacked_xx_0 = blocks.packed_to_unpacked_bb(chunk_len, gr.GR_LSB_FIRST)
    digital_chunks_to_symbols_xx_0 = digital.chunks_to_symbols_bc((codes.codes2list(code, balanced)), len(code[0]))
    blocks_repeat_0 = blocks.repeat(gr.sizeof_gr_complex*1, samp_rate/bw)
    
    ##################################################
    # Connections
    ##################################################
    self.connect((self.source, 0), (blocks_packed_to_unpacked_xx_0, 0))
    self.connect((blocks_packed_to_unpacked_xx_0, 0), (digital_chunks_to_symbols_xx_0, 0))
    self.connect((digital_chunks_to_symbols_xx_0, 0), (blocks_repeat_0, 0))
    self.connect((blocks_repeat_0, 0), (self.sink, 0))

def ooktx(*args, **kwargs):
  kwargs["balanced"] = False
  asktx(*args, **kwargs)

def bpsktx(*args, **kwargs):
  kwargs["balanced"] = True
  asktx(*args, **kwargs)

def oqpsktx(self, carrier=10000, samp_rate = 80000, bw=4000, amp=1, code=codes.mycode, **kwargs):
    code_table, code_len = codes.codes2table(code), len(code)
    chunk_len = int(log(code_len,2))

    topblock(self, carrier, samp_rate, bw, amp)

    ##################################################
    # Blocks
    ##################################################
    self.blocks_packed_to_unpacked_xx_0 = blocks.packed_to_unpacked_bb(chunk_len, gr.GR_LSB_FIRST)
    self.digital_chunks_to_symbols_xx_0 = digital.chunks_to_symbols_bc((code_table), len(code_table)/code_len)

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
