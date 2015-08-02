
import fileLock
import time

interWordDelay = 2

fishFileStatePending = 'fishStatePending.txt'
fishFileStateCurrent = 'fishStateCurrent.txt'

curState = 0

try:
  while(True):
    # Read pending state
    fileLock.acquireLock(fishFileStatePending)
    fin = open(fishFileStatePending, 'r')
    stateNum = int(fin.read())
    fin.close()
    fileLock.releaseLock(fishFileStatePending)
    
    # Play the wav file to send the new state
    wavFile = 'wav_files_20bps/' + '{0:04d}'.format(stateNum) + '.wav'
    if stateNum != curState:
      print 'playing new state:', stateNum
      curState = stateNum
    #if os.system("aplay -q " + wavFile) != 0:
    #  raise KeyboardInterrupt

    # Write the current state
    fileLock.acquireLock(fishFileStateCurrent)
    fout = open(fishFileStateCurrent, 'w')
    fout.write(str(stateNum))
    fout.close()
    fileLock.releaseLock(fishFileStateCurrent)

    time.sleep(interWordDelay)
        
except KeyboardInterrupt:
  pass




