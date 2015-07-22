from gnuradio import analog
from gnuradio import audio
from gnuradio import gr

class mintx(gr.top_block):
  def __init__(self, carrier=8000, samp_rate=192000, amp=0.1):
    gr.top_block.__init__(self, "Top Block")

    analog_sig_source_x_0 = analog.sig_source_f(samp_rate, analog.GR_COS_WAVE, carrier, amp, 0)
    audio_sink_0 = audio.sink(samp_rate, "")

    self.connect((analog_sig_source_x_0, 0), (audio_sink_0, 0))

tx = mintx(carrier=8000)
tx.start()
tx.wait()
