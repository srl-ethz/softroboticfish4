import rx, codes
d = rx.Demod(carrier=32000, bw=1000, sps=1, codes=codes.mycode, mod=rx.Mods.DPHASE)
d.run()
