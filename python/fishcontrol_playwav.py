
import fileLock
import time
import os

interWordDelay = 200

fishFileStatePending = 'fishStatePending.txt'
fishFileStateCurrent = 'fishStateCurrent.txt'

curState = 0

try:
  # set highest priority
  os.nice(-20)
except OSError:
  # not running as root
  pass

try:
  while(True):
    # Read pending state
    fileLock.acquireLock(fishFileStatePending, waitTime=0.05)
    fin = open(fishFileStatePending, 'r')
    try:
      stateNum = int(fin.read())
    except:
      stateNum = None
    fin.close()
    fileLock.releaseLock(fishFileStatePending)
    
    # Play the wav file to send the new state
    if stateNum is not None:
      wavFile = '/home/pi/fish/python/wav_files_20bps/' + '{0:04d}'.format(stateNum) + '.wav'
      if stateNum != curState:
        print 'playing new state:', stateNum
        curState = stateNum
      if os.system("aplay -q " + wavFile) != 0:
        print '*** aplay command terminated'
        raise KeyboardInterrupt
    else:
      print '*** No pending state! ***'

    # Write the current state
    fileLock.acquireLock(fishFileStateCurrent)
    fout = open(fishFileStateCurrent, 'w')
    if stateNum is not None:
      fout.write(str(stateNum))
    else:
      fout.write(str(-1))
    fout.close()
    fileLock.releaseLock(fishFileStateCurrent)

    time.sleep(interWordDelay/1000)
        
except KeyboardInterrupt:
  pass




