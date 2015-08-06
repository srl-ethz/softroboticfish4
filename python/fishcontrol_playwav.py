
import fileLock
import time
import os
from time import strftime
from datetime import datetime

#delay (msec)  between playbacks 
interWordDelay = 200
#number of seconds between which we create a new logfile
logfileInterval = 120
fishFileStatePending = 'fishStatePending.txt'
fishFileStateCurrent = 'fishStateCurrent.txt'

curState = 0

# create the log directory
Txlogdir = '/home/pi/fish/python/TxLog'
if not os.path.exists(Txlogdir):
  print 'creating /home/pi/fish/python/TxLog dir'
  os.makedirs(Txlogdir)

#get initial name of logfile
logfilename = 'TXLog' + strftime('%Y-%m-%d_%H_%M_%S')

#init the file start time
filestrttm = 0


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
    
    # Write the log file
    curtime=time.time()
    # create a new log file every N seconds
    if filestrttm < (curtime - logfileInterval):
      filestrttm=curtime
      logfilename = 'TXLog' + strftime('%Y-%m-%d_%H_%M_%S')
    #if the file exists, open for append, else open new file
    if os.path.isfile(os.path.join(Txlogdir,logfilename)):
      logfile = open(os.path.join(Txlogdir,logfilename), 'a')
    else:
      logfile = open(os.path.join(Txlogdir,logfilename), 'w')
    #get timestamp for entry
    t=datetime.now()
    millisec=t.microsecond/1000
    logfile.write(t.strftime('%Y-%m-%d_%H_%M_%S')+'_'+str(millisec)+','+str(stateNum)+'\n')
    logfile.close()

    time.sleep(interWordDelay/1000)
        
except KeyboardInterrupt:
  pass




