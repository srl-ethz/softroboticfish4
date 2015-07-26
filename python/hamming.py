# file: hamming.py
# author: Bob Muller
# date: December 1, 2008
#
# This file contains code for Hamming Coding of binary
# data streams. The main entry points are:
#
# encode: which accepts a list of bits and returns a
#         list of bits with (odd) Hamming parity bits
#         set;
# decode: which accepts a list of Hamming coded bits
#         with possible errors; it repairs one-bit
#         errors and returns the decoded list of data
#         bits.

import math

# several utility functions.
#
def isPowerOfTwo(n):
    m = math.log(n, 2)
    return m == int(m)

def power2Below(n):
    return 2 ** int(math.log(n,2))

# Is the parity bit in position i, a parity bit for position j?
# NB that >> is the shift-right operator.
#
def covers(i,j):
    return (j >> int(math.log(i,2))) % 2 == 1

# sum all of the bits covered by parity bit i.
#
def sumBits(bits, i, j):
    if j > len(bits):
        return 0
    else:
        restAnswer = sumBits(bits, i, j+1)
        if covers(i,j):
            # NB: j-1 below because lists are 0-based.
            #
            return bits[j-1] + restAnswer
        else:
            return restAnswer

def hasOddParity(bits,i):
    return sumBits(bits,i,i) % 2 == 1

def hasEvenParity(bits,i):
    return not(hasOddParity(bits,i))

# constructs an integer from a list of bits.
#
def bitsToNumber(bits):
    if bits == []:
        return 0
    else:
        n = bits[0] * (2 ** (len(bits) - 1))
        return n + bitsToNumber(bits[1:])

# prepare accepts the list of data bits and an index
# into the list (originally 1) and returns a new list with
# zeros in the power of two positions in the list.
#
def prepare(bits,i):
    if bits == []:
        return []
    else:
        if isPowerOfTwo(i):
            return [0] + prepare(bits, i+1)
        else:
            return [bits[0]] + prepare(bits[1:], i+1)

# The input list has zeros in the parity positions; the
# output has the parity bits properly set.
#
def setParityBits(bits, i):
    if i > len(bits):
        return []
    else:
        restAnswer = setParityBits(bits, i+1)
        if isPowerOfTwo(i):
            if hasOddParity(bits,i):
                return [0] + restAnswer
            else:
                return [1] + restAnswer
        else:
            return [bits[i-1]] + restAnswer

def encode(bits):
    paritysAreZero = prepare(bits, 1)
    s = setParityBits(paritysAreZero, 1)
    if sum(s)%2 == 1:
        return s + [1]
    else:
        return s + [0]

def decode(bits):
    parityResults = checkParity(bits,power2Below(len(bits)))
    n = bitsToNumber(parityResults)

    if n != 0:
        #print "NB: bit ", n, " is bad. Flipping."

        # WARNING!!! Destructive update!
        #
        bits[n - 1] = 1 - bits[n - 1]

    return extractData(bits,1), n

# extractData gathers the bits in non-parity positions.
#
def extractData(bits,i):
    if i > len(bits):
        return []
    else:
        restAnswer = extractData(bits, i+1)
        if isPowerOfTwo(i):
            return restAnswer
        else:
            return [bits[i - 1]] + restAnswer
        
# i is a power of 2. checkParity works from right-to-left to 
# simplify construction of the correct result bit list.
#
def checkParity(bits,i):
    if i == 1:
        return [0] if hasOddParity(bits,i) else [1]
    else:
        bit = 0 if hasOddParity(bits,i) else 1
        return [bit] + checkParity(bits, power2Below(i-1))
