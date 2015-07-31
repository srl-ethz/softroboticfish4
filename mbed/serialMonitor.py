"""
Usage:
  run "sudo python serialMonitor print=1 file=out.txt"
  where the print and file arguments are optional
  while running:
    type p and then enter to toggle screen printing (will store in buffer though)
    press enter to print just next line of buffer when not streaming to screen
    type q and then enter to quit (or ctrl-c)
"""

import os
import sys
import serial
import select

def run(argv):
    # Get the COM port of the mbed
    p = os.popen('ls /dev/ttyACM*', "r")
    port = p.readline().strip()

    print 'Connecting to ' + port
    ser = serial.Serial(port, 921600, timeout=1)
    while ser.inWaiting() > 0:
        ser.read()
    print '\tconnected to', ser.name
    print 'reading serial data (ctrl+c or q-enter to end)'

    dataFile = ''
    dataPrint = ''
    printData = True
    fout = None
    # get options from command line
    for arg in argv:
        if 'print' in arg:
            printData = int((arg.split('=')[1]).strip()) == 1
        if 'file' in arg:
            filename = arg.split('=')[1].strip()
            fout = open(filename, 'w+')

    # begin monitoring
    terminate = False
    try:
        while(not terminate):
            nextChar = ser.read()
            dataFile += nextChar
            dataPrint += nextChar
            # if see a newline, print/write data so far
            if(nextChar == '\n'):
                if printData:
                    print dataPrint[0:-1]
                    dataPrint = ''
                if fout is not None:
                    fout.write(dataFile)
                    dataFile = ''
                    pass
            # if they press enter, see what character they entered
            if select.select([sys.stdin,],[],[],0.0)[0]:
                while select.select([sys.stdin,],[],[],0.0)[0]:
                    entered = sys.stdin.read(1).strip()
                    if entered == 'q':
                        terminate = True
                    if entered == 'p':
                        printData = not printData
                    if len(entered) == 0 and not printData:
                        if '\n' in dataPrint:
                            print dataPrint[0:dataPrint.find('\n')]
                            dataPrint = dataPrint[dataPrint.find('\n')+1:]
    except KeyboardInterrupt:
        pass
    finally:
        # print/write remainder
        try:
            if printData:
                print dataPrint
            if fout is not None:
                fout.write(dataFile)
        except:
            pass
        # clean up
        ser.close()
        if fout is not None:
            fout.close()

if __name__ == '__main__':
    run(sys.argv)
