import numpy, scipy
import matplotlib.pyplot as plt

f2 = scipy.fromfile(open("floatfifo"), dtype=scipy.float32)

'''
syms = [[]]*11
for i in range(11):
  syms[i] = f2[32*i:32*i+32]

sym = map(int, map(numpy.sign, sum(syms)))
print sym

corrs = [  scipy.dot(sym, f2[i:i+32]) for i in range(len(f2)-32) ]

plt.plot(corrs)
plt.show()

'''

sym = [1, 1, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, -1, 1, -1, 1, 1, 1, -1, -1, 1, 1, -1, 1, 1, -1, -1, 1]

from collections import deque
buf = deque(maxlen=32)

for i in range(10000):
  sample = numpy.fromfile(open("floatfifo"), dtype=scipy.float32, count=1)
  buf.appendleft(sample)
  print(list(buf))
