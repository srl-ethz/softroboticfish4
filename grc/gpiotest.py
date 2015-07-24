from rtlsdr import RtlSdr

sdr = RtlSdr()

for i in range(8):
  sdr.set_gpio_output(i)
  sdr.set_gpio_output(i+8)

for a in range(1000):
  for i in range(8):
    print i,
    for j in range(i+1):
      print '.',
      sdr.set_gpio_bit(i, 1)
      sdr.set_gpio_bit(i+8, 1)
      sdr.set_gpio_bit(i, 0)
      sdr.set_gpio_bit(i, 1)
      sdr.set_gpio_bit(i, 0)
      sdr.set_gpio_bit(i+8, 0)
      for k in range(1000):
        pass
    print

