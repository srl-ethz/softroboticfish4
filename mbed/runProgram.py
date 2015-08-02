import sys
import os
import time
import resetMbed
import serialMonitor

def runProgram(argv):
    # If a bin file was given as argument, program it onto the mbed
    for arg in argv:
        if '.bin' in arg:
            print 'Copying bin file...'
            #os.system('sudo rm /media/MBED/*.bin')
            #time.sleep(1)
            #os.system('sudo cp /home/pi/Downloads/*.bin /media/MBED')
            #time.sleep(1)
            os.system('sudo /home/pi/fish/mbed/programMbed.sh ' + arg)
    # Remount mbed
    os.system("sudo /home/pi/fish/mbed/remountMbed.sh")
    # Start mbed program and serial monitor
    print 'Resetting mbed and starting serial monitor'
    print ''
    resetMbed.reset()
    print '============'
    serialMonitor.run(argv)

if __name__ == '__main__':
    runProgram(sys.argv)
    
    
