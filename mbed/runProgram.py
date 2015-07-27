import sys
import os
import time
import resetMbed
import serialMonitor

def runProgram(argv):
    copyBin = raw_input('Copy program from downloads folder? (y/n): ').strip().lower() == 'y'
    if copyBin:
        p = os.popen('rm /media/MBED/*.bin', "r")
        p.readline()
        time.sleep(1)
        p = os.popen('cp /home/pi/Downloads/* /media/MBED', "r")
        p.readline()
        time.sleep(4)
        
    resetMbed.reset()
    serialMonitor.run(argv)

if __name__ == '__main__':
    runProgram(sys.argv)
    
    
