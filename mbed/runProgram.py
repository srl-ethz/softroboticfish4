import sys
import os
import time
import resetMbed
import serialMonitor

def runProgram(argv):
    copyBin = raw_input('Copy program from downloads folder? (y/n): ').strip().lower() == 'y'
    if copyBin:
        print 'Copying bin file...'
        #os.system('sudo rm /media/MBED/*.bin')
        #time.sleep(1)
        #os.system('sudo cp /home/pi/Downloads/*.bin /media/MBED')
        #time.sleep(1)
        os.system('sudo /home/pi/fish/mbed/programMbed.sh')
    print 'Remounting mbed...'
    os.system("sudo /home/pi/fish/mbed/remountMbed.sh")
    print 'Resetting mbed and starting serial monitor'
    print ''
    resetMbed.reset()
    print '============'
    serialMonitor.run(argv)

if __name__ == '__main__':
    runProgram(sys.argv)
    
    
