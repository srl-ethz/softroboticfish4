
//#define threshold1
#define threshold2

#include "mbed.h"
#include "ToneDetector.h"
#include "FishController.h"

#define infiniteLoop

//#define artificialPowers // will use tone powers saved in a file rather than actually sampling (expects two tab-separated columns of powers in /local/powers.wp)
//#define singleDataStream // just read a single continous stream of bits (as opposed to segmenting into words/decoding)

//#define controlFish   // whether to start fishController to control servos/motor
#define useAGC        // if undefined, will only set agc once at startup 
//#define AGCLeds     // if defined, will light the LEDs to indicate gain index (whether or not useAGC is on)

//#define saveData  // whether to save data to an array (and print it at the end)
//#define streamData // whether to stream data to screen as it's received
//#define streamSignalLevel // stream signal level to screen

Serial pc(USBTX, USBRX); 
#define loopCount 20000       // number of buffers to process before terminating the program
volatile uint32_t count = 0;                // number of buffers sampled so far
Timer timer;                       // see how long the program runs

// Sampling / Thresholding configuration
// 20 bps (5/45 ms)
#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
#define period 80          // the number of buffers from a rising edge to when we start looking for the next rising edge
#define bitPeriod 100      // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
#define interWordWait 300  // how many buffers of silence to expect between words
// 50 bps (5/15 ms)
//#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
//#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
//#define period 20          // the number of buffers from a rising edge to when we start looking for the next rising edge
//#define bitPeriod 40       // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
//#define interWordWait 300  // how many buffers of silence to expect between words
// 77 bps (5/8 ms)
//#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
//#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
//#define period 10          // the number of buffers from a rising edge to when we start looking for the next rising edge
//#define bitPeriod 26       // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
//#define interWordWait 300  // how many buffers of silence to expect between words
// 100 bps (5/5 ms)
//#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
//#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
//#define period 6           // the number of buffers from a rising edge to when we start looking for the next rising edge
//#define bitPeriod 20       // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
//#define interWordWait 300  // how many buffers of silence to expect between words

// Detecting data // TODO check what actually needs to be volatile
volatile bool waitingForEnd;     // got data, now waiting until next bit transmission is expected (waiting until "period" buffers have elapsed since edge)
volatile uint16_t periodIndex;   // how many buffers it's been since the last clock edge
volatile uint8_t fskIndex;       // which set of 0/1 frequencies are next expected

#if defined(threshold2)
#define powerHistoryLength 50   // number of results to consider when deciding if a tone is present.  should be half of a bit-width
#define powerHistoryDetectWindow 20 // portion of powerHistory to consider as the detection zone (where a peak is expected)
volatile int32_t powerHistory[powerHistoryLength][numTones];  // History of Goertzel results for the last powerHistoryLength buffers
volatile int16_t powerHistoryIndex;                   // Index into powerHistory (circular buffer)
volatile int16_t powerHistoryDetectIndex;             // Marks where in powerHistory to start window for detection; will always be detectWindow elements behind powerHistoryIndex in the circular buffer
volatile int32_t powerHistorySumDetect[numTones];     // Sum of powers stored in powerHistory (stand-in for mean)
volatile int32_t powerHistorySumNoDetect[numTones];     // Sum of powers stored in powerHistory (stand-in for mean)
volatile int32_t powerHistoryMaxDetect[numTones];     // Max of powers stored in powerHistory
volatile int32_t powerHistoryMaxNoDetect[numTones];     // Max of powers stored in powerHistory
volatile bool readyToThreshold;                       // Whether powerHistory has been filled at least once
volatile bool tonesPresent[detectWindow][numTones];  // Circular buffer of whether tones were detected in last detectWindow powers
volatile uint8_t detectSums[numTones];              // Rolling sum over last detectWindow tonesPresent results (when > detectWindow/2, declares data bit)
volatile uint8_t detectWindowIndex;
#elif defined(threshold1)
#define powerHistoryLength 50                    // number of results to consider when deciding if a tone is present.  should be half of a bit-width
volatile uint8_t detectSums[numTones];              // Rolling sum over last detectWindow buffer results (when > detectWindow/2, declares clock/data)
int32_t powerHistory[powerHistoryLength][numTones];  // History of Goertzel results for the last powerHistoryLength buffers
volatile int16_t powerHistoryIndex;                   // Index into powerHistory (circular buffer)
volatile int16_t powerHistoryDetectIndex;             // Marks where in powerHistory to start window for detection; will always be detectWindow elements behind powerHistoryIndex in the circular buffer
int32_t powerHistorySum[numTones];     // Sum of powers stored in powerHistory (stand-in for mean)
int32_t powerHistoryMax[numTones];     // Max of powers stored in powerHistory
volatile bool readyToThreshold;                       // Whether powerHistory has been filled at least once
#endif

#if defined(singleDataStream) && defined(saveData)
    volatile bool data[loopCount/(bitPeriod) * (numTones-1) + 50]; // Received data (each clock pulse is associated with numTones-1 bits) // TODO update for FSK, but this is fine for one FSK group
    volatile uint16_t dataIndex;
#else
    #define dataWordLength 16 // bits per word
    volatile uint8_t dataBitIndex;     // current bit of that word
    volatile bool interWord;           // whether we are currently in between words (waiting out the long inter-word pause)
    void decodeDataWord(volatile bool* data); 
    #ifdef saveData
    volatile bool data[loopCount/(bitPeriod*dataWordLength)][dataWordLength]; // Each element is a word as an array of bits
    volatile uint16_t dataWordIndex;   // current data word
    #else
    volatile bool dataWord[dataWordLength]; // an array of bits
    #endif
#endif

// Adjustable gain
volatile uint8_t currentGainIndex;
const uint8_t agcGains[] = {1, 1, 2, 5, 10, 20, 50, 100};
const uint8_t agcGainsLength = sizeof(agcGains)/sizeof(agcGains[0]);
const uint16_t signalLevelBufferLength = (10000000)/(sampleWindow*sampleInterval);  // first constant is us window length for updating agc
const int32_t signalLevelSumDesired = ((uint32_t)500)*((uint32_t)signalLevelBufferLength); // desire about 2 VPP, so signal minus offset should have 1V amplitude, so say mean a bit smaller, about 0.4V ~= 500
volatile uint16_t signalLevelBufferIndex;
volatile int32_t signalLevelSum;

DigitalOut gain0(p16);
DigitalOut gain1(p17);
DigitalOut gain2(p18);
DigitalOut agc[3] = {gain0, gain1, gain2};
#ifdef AGCLeds
    #undef debugLEDs
    DigitalOut agcLED0(LED1);
    DigitalOut agcLED1(LED2);
    DigitalOut agcLED2(LED3);
    DigitalOut agcLEDs[3] = {agcLED0, agcLED1, agcLED2};
#endif

// Fish reset timeout
const uint16_t fishResetTimeout = 5000000/(sampleWindow*sampleInterval); // first const arg is timeout in us
volatile uint16_t lastDataWord = 0;
volatile uint16_t timeSinceGoodWord = 0;

// Low battery monitor
DigitalIn lowBatteryVoltage(p26);
void lowBatteryCallback();

// Autonomous mode
uint16_t autoModeCommands[] = {0x05B6 /* forward */, 0x05BC /*down*/, 0x05B6 /* forward */, 0x0586 /* left */,
								0x05B6 /* forward */, 0x05E6 /* right */,  0x05B6 /* forward */,  0x05B0 /* up */,
								0x05B6 /* forward */, 0x0236 /* stopped */};
uint32_t autoModeDurations[] = {30000, 60000, 30000, 30000, 30000, 30000, 30000, 30000, 30000, 10000};
//uint32_t autoModeDurations[] = {6000, 12000, 6000, 6000, 6000, 6000, 6000, 6000, 6000, 10000};
const uint8_t autoModeLength = 10;
volatile uint32_t autoModeCount;
volatile uint32_t autoModeIndex;

// Testing/debugging
#ifdef artificialPowers
    #ifndef recordStreaming
    LocalFileSystem local("local");
    #endif
    FILE* finPowers = fopen("/local/powers.wp", "r");
    int32_t nextPowers[numTones];
#endif

// Log data commands

// Called by toneDetector when new tone powers are computed
void processTonePowers(int32_t* newTonePowers, uint32_t signalLevel)
{
	#ifdef newStream
	toStream[4] = -10;
	#endif
    // Check for low battery and if so terminate tone detector
    if(lowBatteryVoltage == 0)
    {
        lowBatteryCallback();
        return;
    }
    // See if we're done and if so terminate the tone detector
    if(count == loopCount)
    {
        #ifndef infiniteLoop
        toneDetector.stop();
        return;
        #else
        //count = 0;
        #endif        
    }
    count++;
    // See if we're in autonomous mode and if so update the command
    if(fishController.autoMode)
    {
    	fishController.processDataWord(autoModeCommands[autoModeIndex]);
    	autoModeCount++;
    	if(autoModeCount > autoModeDurations[autoModeIndex])
    	{
    		autoModeCount = 0;
    		autoModeIndex++;
    	}
    	if(autoModeIndex >= autoModeLength)
    	{
    		fishController.autoMode = false;
    		fishController.setLEDs(255, false);
    	}
    	return;
    }
    else
    {
		autoModeCount = 0;
		autoModeIndex = 0;
    }

    periodIndex++;
    timeSinceGoodWord++;
    
    #if defined(threshold2)
    // Update threshold window state for each tone
    for(uint8_t t = 0; t < numTones; t++)
    {
        // Update our history of powers for this tone
        powerHistory[powerHistoryIndex][t] = (newTonePowers[t] >> 5);
        // Compute max/sum of the current window (only for frequencies we currently anticipate)
        if((t == fskIndex*2 || t == fskIndex*2+1) && !waitingForEnd)
        {
            powerHistoryMaxDetect[t] = 0;
            powerHistoryMaxNoDetect[t] = 0;
            powerHistorySumDetect[t] = 0;
            powerHistorySumNoDetect[t] = 0;
            // Look at non-detection zone
            for(uint16_t p = ((powerHistoryIndex+1)%powerHistoryLength); p != powerHistoryDetectIndex; p=(p+1)%powerHistoryLength)
            {
                powerHistorySumNoDetect[t] += powerHistory[p][t];
                if(powerHistory[p][t] > powerHistoryMaxNoDetect[t])
                    powerHistoryMaxNoDetect[t] = powerHistory[p][t];
            }
            // Look at detection zone
            for(uint16_t p = powerHistoryDetectIndex; p != ((powerHistoryIndex+1)%powerHistoryLength); p=((p+1)%powerHistoryLength))
            {
                powerHistorySumDetect[t] += powerHistory[p][t];
                if(powerHistory[p][t] > powerHistoryMaxDetect[t])
                    powerHistoryMaxDetect[t] = powerHistory[p][t];
            }
        }
    }
    // Advance our power history index (circular buffer)
    powerHistoryIndex = (powerHistoryIndex+1) % powerHistoryLength;
    powerHistoryDetectIndex = (powerHistoryDetectIndex+1) % powerHistoryLength;
    readyToThreshold = readyToThreshold || (powerHistoryIndex == 0);
    // If not waiting until next pulse is expected, see if a tone is present
    if(!waitingForEnd && readyToThreshold)
    {
        // Based on new max/mean, see how many powers indicate a tone present in the last detectWindow readings
        for(uint8_t t = fskIndex*2; t <= fskIndex*2+1; t++)
        {
            detectSums[t] -= tonesPresent[detectWindowIndex][t];
            if(
                   ((powerHistorySumDetect[t] << 2) > powerHistorySumNoDetect[t])
                && (powerHistoryMaxDetect[t] > powerHistoryMaxNoDetect[t])
                && ((powerHistorySumDetect[t] - (powerHistorySumDetect[t] >> 3)) > powerHistorySumDetect[fskIndex*2+((t+1)%2)])
                && ((powerHistorySumDetect[t] << 2) > powerHistorySumNoDetect[fskIndex*2+((t+1)%2)])
                && ((powerHistoryMaxDetect[t] << 4) > powerHistorySumNoDetect[t])
                && (powerHistoryMaxDetect[t] > 100000)
                )
                tonesPresent[detectWindowIndex][t] = 1;
            else
                tonesPresent[detectWindowIndex][t] = 0;
            detectSums[t] += tonesPresent[detectWindowIndex][t];
        }
        detectWindowIndex = (detectWindowIndex+1) % detectWindow;
        // If both are considered present (should be very rare?), choose the one with a higher mean
        if(detectSums[fskIndex*2] > detectThresh && detectSums[fskIndex*2+1] > detectThresh)
        {
            if(powerHistorySumDetect[fskIndex*2] > powerHistorySumDetect[fskIndex*2+1])
                detectSums[fskIndex*2+1] = 0;
            else
                detectSums[fskIndex*2] = 0;
        }
        // See if a tone is present
        int tonePresent = -1;
        if(detectSums[fskIndex*2] > detectThresh)
            tonePresent = fskIndex*2;
        else if(detectSums[fskIndex*2+1] > detectThresh)
            tonePresent = fskIndex*2+1;
        // Record data and update state
        if(tonePresent > -1)
        {
            #ifdef singleDataStream
                #ifdef saveData
                data[dataIndex++] = tonePresent%2;
                #endif
                #ifdef streamData
                printf("%ld\t%d\n", count, (bool)(tonePresent%2));
                #endif
                periodIndex = detectSums[tonePresent];
                fskIndex = (fskIndex+1) % numFSKGroups;
            #else
            // See if it has been a long time since last pulse
            if(periodIndex >= interWordWait)
            {
                // If we currently think that we're between words, then that's good so process the previous data word
                // If we don't think we're between words, then we missed a bit so discard whatever we've collected in the word so far
                // Either way, this is the start of a new word so reset dataBitIndex
                if(interWord && dataBitIndex == dataWordLength)
                {
                    // Decode last word and then send it out
                    #ifdef saveData
                    decodeDataWord(data[dataWordIndex]);
                    dataWordIndex++;
                    #else
                    decodeDataWord(dataWord);
                    streamFishStateEvent = 5;
                    #endif
                    dataBitIndex = 0;
                    interWord = false;
                    
                    timeSinceGoodWord = 0;
                }
                else if(!interWord)
                {
                    #ifdef streamData
                    printf("%ld\t0\t-1\n", count);
                    #endif
					#ifdef saveData
                    for(int dbi = 0; dbi < dataWordLength; dbi++)
                    	data[dataWordIndex][dbi] = 0;
                    data[dataWordIndex][dataWordLength-1] = 1;
                    dataWordIndex++;
					#endif
					#ifdef newStream
                    toStream[4] = -1;
                    streamFishStateEvent = 1;
					#endif
                    lastDataWord = 0;
                }
                // TODO this is a debug check - if transmitter not putting space between words, set interWordWait to 1
                if(interWordWait > 1)
                {
                    dataBitIndex = 0;
                    interWord = false;
                }
            }
            else if(interWord)
            {
                // It has not been a long time since the last pulse, yet we thought it should be
                // Seems like we erroneously detected a bit that shouldn't have existed
                // Discard current word as garbage and start again
                dataBitIndex = 0;
                #ifdef streamData
                printf("%ld\t0\t-2\n", count);
                #endif
				#ifdef saveData
				for(int dbi = 0; dbi < dataWordLength; dbi++)
					data[dataWordIndex][dbi] = 0;
				data[dataWordIndex][dataWordLength-2] = 1;
				dataWordIndex++;
				#endif
				#ifdef newStream
				toStream[4] = -2;
				streamFishStateEvent = 2;
				#endif
                lastDataWord = 0;
            }
            // If we're not between words (either normally or because we were just reset above), store the new bit
            if(!interWord)
            {
                #ifdef saveData
                data[dataWordIndex][dataBitIndex++] = tonePresent%2;
                #else
                dataWord[dataBitIndex++] = tonePresent%2;
                #endif
                fskIndex = (fskIndex+1) % numFSKGroups;
                // If we've finished a word, say we're waiting between words
                // Word won't be processed until next word begins though in case we accidentally detected a nonexistent bit (see logic above)
                if(dataBitIndex == dataWordLength)
                {
                    interWord = true;
                    fskIndex = 0;
                }
            }
            periodIndex = detectSums[tonePresent];  // Use number of detected points rather than 0 to get it closer to actual rising edge
            #endif
            // Wait out reflections until next pulse
            waitingForEnd = true;
        }
    }
    else if(periodIndex > period) // done waiting for next bit (waiting out reflections)?
    {
        waitingForEnd = false;
        for(uint8_t t = 0; t < numTones; t++)
        {
            detectSums[t] = 0;
            for(uint8_t p = 0; p < detectWindow; p++)
                tonesPresent[p][t] = 0;
        }
    }
    #elif defined(threshold1)
    // Update threshold window state for each tone
    for(uint8_t t = 0; t < numTones; t++)
    {
        // Update our history of powers for this tone
        powerHistorySum[t] -= powerHistory[powerHistoryIndex][t];
        powerHistory[powerHistoryIndex][t] = newTonePowers[t];
        powerHistorySum[t] += newTonePowers[t];
        // Compute max of the current window (only for frequencies we currently anticipate)
        if((t == fskIndex*2 || t == fskIndex*2+1) && !waitingForEnd)
        {
            powerHistoryMax[t] = 0;
            for(uint16_t p = 0; p < powerHistoryLength; p++)
            {
                if(powerHistory[p][t] > powerHistoryMax[t])
                    powerHistoryMax[t] = powerHistory[p][t];
            }
        }
    }
    // Advance our power history index (circular buffer)
    powerHistoryIndex = (powerHistoryIndex+1) % powerHistoryLength;
    readyToThreshold = readyToThreshold || (powerHistoryIndex == 0);
    // If not waiting until next pulse is expected, see if a tone is present
    if(!waitingForEnd && readyToThreshold)
    {
        // Based on new max/mean, see how many powers indicate a tone present in the last detectWindow readings
        for(uint8_t t = fskIndex*2; t <= fskIndex*2+1; t++)
        {
            detectSums[t] = 0;
            for(uint16_t p = powerHistoryDetectIndex; p != powerHistoryIndex; p = (p+1) % powerHistoryLength)
            {
                if((powerHistory[p][t] > (powerHistorySum[fskIndex*2+((t+1)%2)] >> 2)) // power greater than 12.5 times mean of other channel
                    && (powerHistory[p][t] > (powerHistoryMax[t] >> 2))                // power greater than 1/4 of the max on this channel
                    && (powerHistory[p][t] > 1000)                                     // power greater than a fixed threshold
                    && powerHistoryMax[t] > (powerHistorySum[t] >> 3))                 // max on this channel is 6.25 times greater than the mean on this channel (in case this channel noise is just must higher than other channel)
                    detectSums[t]++;
            }
        }
        // If both are considered present (should be very rare?), choose the one with a higher mean
        if(detectSums[fskIndex*2] > detectThresh && detectSums[fskIndex*2+1] > detectThresh)
        {
            if(powerHistorySum[fskIndex*2] > powerHistorySum[fskIndex*2+1])
                detectSums[fskIndex*2+1] = 0;
            else
                detectSums[fskIndex*2] = 0;
        }
        // See if a tone is present
        int tonePresent = -1;
        if(detectSums[fskIndex*2] > detectThresh)
            tonePresent = fskIndex*2;
        else if(detectSums[fskIndex*2+1] > detectThresh)
            tonePresent = fskIndex*2+1;
        // Record data and update state
        if(tonePresent > -1)
        {
            #ifdef singleDataStream
                #ifdef saveData
                data[dataIndex++] = tonePresent%2;
                #endif
                #ifdef streamData
                printf("%d", (bool)(tonePresent%2));
                #endif
                periodIndex = detectSums[tonePresent];
                fskIndex = (fskIndex+1) % numFSKGroups;
            #else
            // See if it has been a long time since last pulse
            if(periodIndex >= interWordWait)
            {
                // If we currently think that we're between words, then that's good so process the previous data word
                // If we don't think we're between words, then we missed a bit so discard whatever we've collected in the word so far
                // Either way, this is the start of a new word so reset dataBitIndex
                if(interWord && dataBitIndex == dataWordLength)
                {
                    // Decode last word and then send it out
                    #ifdef saveData
                    decodeDataWord(data[dataWordIndex]);
                    dataWordIndex++;
                    #else
                    decodeDataWord(dataWord);
                    #endif
                    dataBitIndex = 0;
                    interWord = false;
                    
                    timeSinceGoodWord = 0;
                }
                else if(interWord)
                {
                    #ifdef streamData
                    printf("0\t-1\n");
                    #endif
                    lastDataWord = 0;
                }
                // TODO this is a debug check - if transmitter not putting space between words, set interWordWait to 1
                if(interWordWait > 1)
                {
                    dataBitIndex = 0;
                    interWord = false;
                }
            }
            else if(interWord)
            {
                // It has not been a long time since the last pulse, yet we thought it should be
                // Seems like we erroneously detected a bit that shouldn't have existed
                // Discard current word as garbage and start again
                dataBitIndex = 0;
                #ifdef streamData
                printf("0\t-2\n");
                #endif
                lastDataWord = 0;
            }
            // If we're not between words (either normally or because we were just reset above), store the new bit
            if(!interWord)
            {
                #ifdef saveData
                data[dataWordIndex][dataBitIndex++] = tonePresent%2;
                #else
                dataWord[dataBitIndex++] = tonePresent%2;
                #endif
                fskIndex = (fskIndex+1) % numFSKGroups;
                periodIndex = detectSums[tonePresent];  // Use number of detected points rather than 0 to get it closer to actual rising edge
                // If we've finished a word, say we're waiting between words
                // Word won't be processed until next word begins though in case we accidentally detected a nonexistent bit (see logic above)
                if(dataBitIndex == dataWordLength)
                {
                    interWord = true;
                    fskIndex = 0;
                }
            }
            #endif
            // Wait out reflections until next pulse
            waitingForEnd = true;
        }
    }
    else if(periodIndex > period) // done waiting for next bit (waiting out reflections)?
    {
        waitingForEnd = false;
    }
    // Advance our power history start detect index (circular buffer) to stay detectWindow elements behind main index
    powerHistoryDetectIndex = (powerHistoryDetectIndex+1) % powerHistoryLength;
    #endif // end switch between threshold1 or threshold2
    
    // Update signal level history
    signalLevelSum += signalLevel;
    signalLevelBufferIndex = (signalLevelBufferIndex+1) % signalLevelBufferLength;
    // If have seen enough readings, choose a new gain
    #ifdef streamSignalLevel
    //printf("%ld\t%d\t%d\n", signalLevel, currentGainIndex, lastDataWord);
    printf("%ld\t%d\n", signalLevel, currentGainIndex);
    #endif
	#ifdef newStream
    toStream[3] = currentGainIndex;
	#endif
    if(signalLevelBufferIndex == 0)
    {
        // Calculate ideal gain for signalLevelSum to become signalLevelSumDesired
        int32_t newGain = ((int64_t)signalLevelSumDesired * (int64_t)agcGains[currentGainIndex]) / (int64_t)signalLevelSum;
        // See which available gain is closest
        uint8_t bestIndex = currentGainIndex;
        int32_t minDiff = 10000;
        for(uint8_t g = 0; g < agcGainsLength; g++)
        {
            int32_t diff = (int32_t)agcGains[g] - newGain;
            if(diff > 0 && diff < minDiff)
            {
                minDiff = diff;
                bestIndex = g;
            }
            else if(diff < 0 && -1*diff < minDiff)
            {
                minDiff = -1*diff;
                bestIndex = g;
            }
        }
        // Set the gain
        currentGainIndex = bestIndex;
        for(uint8_t i = 0; i < 3; i++)
        {
            #ifdef useAGC
            agc[i].write(currentGainIndex & (1 << i));
            #endif
            #ifdef AGCLeds
            agcLEDs[i].write(currentGainIndex & (1 << i));
            #endif
        }
        // Reset sum
        signalLevelSum = 0;
    }
    
    #ifdef artificialPowers
    printf("%ld \t%ld \t%ld \t%d \t%ld \t%ld \t%ld \t%ld \t%d \t%d \t%d \n", newTonePowers[0], newTonePowers[1], detectSums[0], detectSums[1], powerHistorySum[0], powerHistorySum[1], powerHistoryMax[0], powerHistoryMax[1], fskIndex, waitingForEnd, periodIndex);
    #endif
    
    // Check for timeout
    if(timeSinceGoodWord >= fishResetTimeout)
    {
        #ifndef singleDataStream
        if(timeSinceGoodWord == fishResetTimeout)
        {
            bool neutralWord[] = {0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}; // Decodes to state 54, which is neutral state
            decodeDataWord(neutralWord);
            streamFishStateEvent = 4;
        }
        #endif
        timeSinceGoodWord = fishResetTimeout+1;
    }

    // Stream data
	#ifdef newStream
    //printf("%ld %ld %ld %ld %ld %ld\n", count-1, toStream[0], toStream[1], toStream[2], toStream[3], toStream[4]);
    //printf("%ld %ld %ld\n", toStream[0], toStream[1], toStream[2]);
    //printf("%ld %ld %ld %ld %ld\n", toStream[0], toStream[1], toStream[2], toStream[3], toStream[4]);
    //FunctionPointer fp = &tempMethod;
    //toStream[0] = 6100;
    //toStream[1] = 62;
    //toStream[2] = 63;
    //toStream[3] = 64;
    //toStream[4] = -10;
    uint32_t streamGoertzel1 = toStream[0];// >> 8; // second day included >> 8
    uint32_t streamGoertzel2 = toStream[1];// >> 8; // second day included >> 8
    uint16_t streamSignalLevel = ((toStream[2] >> 2) & ((uint16_t)2047)) + (((uint16_t)21) << 11); // first day was just toStream[2], second day included code word
    uint8_t streamGain = toStream[3] + 168; // only used in first day
    //int16_t streamWord = toStream[4];
    uint16_t streamWord = (uint16_t)streamCurFishState | ((uint16_t)streamFishStateEvent << 11); // same for both days

    uint8_t* bufferOut = (uint8_t*) (&streamGoertzel1);
    pc.putc(bufferOut[0]);
    pc.putc(bufferOut[1]);
    pc.putc(bufferOut[2]);
    pc.putc(bufferOut[3]); // commented out for second day

    bufferOut = (uint8_t*) (&streamGoertzel2);
    pc.putc(bufferOut[0]);
	pc.putc(bufferOut[1]);
	pc.putc(bufferOut[2]);
	pc.putc(bufferOut[3]); // commented out for second day

	bufferOut = (uint8_t*) (&streamSignalLevel);
	pc.putc(bufferOut[0]);
    pc.putc(bufferOut[1]);

	bufferOut = (uint8_t*) (&streamGain);
	pc.putc(bufferOut[0]); // commented out for second day

    bufferOut = (uint8_t*) (&streamWord);
	pc.putc(bufferOut[0]);
	pc.putc(bufferOut[1]);
    //printf("%d %d\n", streamCurFishState, streamWord);

	/*uint8_t* bufferOut = (uint8_t*) toStream;
    for(uint8_t i = 0; i < 20; i++)
    	pc.putc(bufferOut[i]);*/
	streamFishStateEvent = 0;
	#endif
}

#ifndef singleDataStream
// Use Hamming code to decode the data word and check for an error
// TODO make constants/defines for magic numbers below that indicate length of hamming code
void decodeDataWord(volatile bool* data)
{
    // Check whether each parity bit is correct
    bool paritySums[5] = {0}; 
    for(uint8_t b = 0; b < dataWordLength-1; b++)
    {
        for(uint8_t p = 0; p < 5; p++)
        {
            if(((b+1) & (1 << p)) > 0 || p == 5-1) // check if bit belongs to parity set (overall parity at end includes everything)
                paritySums[p] ^= data[b];
        }
    }
    paritySums[5-1] ^= data[dataWordLength-1]; // overall parity bit
    // See if we have an error to correct
    uint8_t errorBit = 0;
    uint8_t numParityErrors = 0;
    for(uint8_t p = 0; p < 5-1; p++)
    {
        if(paritySums[p])
        {
            errorBit += (1 << p);
            numParityErrors++;
        }
    }
    // Flip the erroneous bit if applicable
    if(numParityErrors > 1)
        data[errorBit-1] = !data[errorBit-1];
    // If using final parity bit, check that we don't detect an uncorrectable error
    if(!(numParityErrors > 0 && !paritySums[5-1]))
    {
        uint16_t word = 0;
        uint8_t i = 0;
        uint8_t nextParity = 1;
        for(uint8_t b = 0; b < dataWordLength; b++)
        {
            if(b == nextParity-1)
                nextParity <<= 1;
            else
                word |=  data[b] << i++;
        }
        // Update the fish
        fishController.processDataWord(word);
        lastDataWord = word;
        #ifdef streamData
        if(timeSinceGoodWord >= fishResetTimeout)
        	printf("%ld\t%d\t-4\n", count, (int)word);
        else
        	printf("%ld\t%d\t1\n", count, (int)word);
        #endif
		#ifdef newStream
        if(timeSinceGoodWord >= fishResetTimeout)
        	toStream[4] = -4;
        else
        	toStream[4] = word;
		#endif
    }
    else
    {
        #ifdef streamData
    	uint16_t word = 0;
    	for(int i = 0; i < 16; i++)
    		word += data[i] << i;
        printf("%ld\t%d\t-3\n", count, word);
        #endif
		#ifdef newStream
        toStream[4] = -3;
        streamFishStateEvent = 3;
		#endif
        lastDataWord = 0;
    }
}
#endif

// Main loop
int main() 
{
    lowBatteryVoltage.mode(PullUp);
    //lowBatteryInterrupt.fall(&lowBatteryCallback);
    
    wait(0.5);
    pc.baud(921600);

    //printf("\n===== Hello 16 =====\n");
    //printf("(ToneDetector test)\n");
    //printf("\n");
    wait(1);
    
    // Create file for logging data words
	#ifdef saveData
    LocalFileSystem local("local");
    FILE* foutDataWords;
    int fileNum = -1;
    char filename[25];
    foutDataWords = NULL;
    do
    {
    	fileNum++;
    	fclose(foutDataWords);
    	sprintf(filename, "/local/%d.txt", fileNum);
    	foutDataWords = fopen(filename, "r");
    	printf("%d\n", fileNum);
    } while(foutDataWords != NULL);
    foutDataWords = fopen(filename, "w");
    /*Timer t1;
    t1.start();
    uint32_t test[5] = {123000, 122000, 121000, 1, 119};
    for(uint32_t i = 0; i < 1000; i++)
    	printf("%ld %ld %ld %d %d\n", test[0], test[1], test[2], test[3], test[4]);
    	//fwrite(test, 4, 3, foutDataWords);
    t1.stop();
    fclose(foutDataWords);
    printf("time:%d\n", t1.read_us());
    while(true);*/
	#endif

    // Configure the tone detector
    toneDetector.setCallback(&processTonePowers);
    toneDetector.init();
    
    // Clear detection arrays
    #if defined(threshold2)
    for(int t = 0; t < numTones; t++)
    {
        detectSums[t] = 0;
        for(uint8_t p = 0; p < detectWindow; p++)
            tonesPresent[p][t] = 0;
    }
    detectWindowIndex = 0;
    readyToThreshold = false;
    powerHistoryIndex = 0;
    powerHistoryDetectIndex = (powerHistoryLength - 1) - (powerHistoryDetectWindow-1) + 1; // assumes powerHistoryLength >= detectWindow > 1
    for(uint8_t t = 0; t < numTones; t++)
    {
        powerHistorySumDetect[t] = 0;
        powerHistorySumNoDetect[t] = 0;
        powerHistoryMaxDetect[t] = 0;
        powerHistoryMaxNoDetect[t] = 0;
        for(uint16_t p = 0; p < powerHistoryLength; p++)
        {
            powerHistory[p][t] = 0;
        }
    }
    #elif defined(threshold1)
    for(int t = 0; t < numTones; t++)
    {
        detectSums[t] = 0;
    }
    powerHistoryIndex = 0;
    powerHistoryDetectIndex = (powerHistoryLength - 1) - (detectWindow-1) + 1; // assumes powerHistoryLength >= detectWindow > 1
    for(uint8_t t = 0; t < numTones; t++)
    {
        powerHistorySum[t] = 0;
        powerHistoryMax[t] = 0;
        for(uint16_t p = 0; p < powerHistoryLength; p++)
        {
            powerHistory[p][t] = 0;
        }
    }
    readyToThreshold = false;
    #endif
    #ifdef singleDataStream
        #ifdef saveData
        dataIndex = 0;
        #endif
    #else
        #ifdef saveData
        dataWordIndex = 0;
        #endif
        dataBitIndex = 0;
        interWord = false;
    #endif
    waitingForEnd = false;
    periodIndex = 0;
    fskIndex = 0;

    // Initialize adjustable gain control
    signalLevelBufferIndex = 0;
    signalLevelSum = 0;
    currentGainIndex = 4;
    for(uint8_t i = 0; i < sizeof(agc)/sizeof(agc[0]); i++)
    {
        agc[i].write(currentGainIndex & (1 << i));
        #ifdef AGCLeds
        agcLEDs[i].write(currentGainIndex & (1 << i));
        #endif
    }

    #ifdef controlFish
    // Start the fish controller
    fishController.start();
    #endif
    
    #ifndef artificialPowers
    // Start listening for tones
    timer.start();
    toneDetector._run();
    timer.stop();
    toneDetector.finish(); // we won't include the time this takes to write files in the elapsed time
    #else
    // Read powers from file
    printf("newPower[0] \tnewPower[1] \tdetectSums[0] \tdetectSums[1] \tsum[0] \tsum[1] \tmax[0] \tmax[1] \tfskIndex \twaiting \tperiodIndex \n");
    int32_t maxSignalValTemp = 0;
    int res = fscanf(finPowers, "%ld\t%ld\n", &nextPowers[0], &nextPowers[1]);
    while(res > 0)
    {
        processTonePowers(nextPowers, maxSignalValTemp);
        res = fscanf(finPowers, "%ld\t%ld\n", &nextPowers[0], &nextPowers[1]);
    }
    fclose(finPowers);
    #endif
    
    #ifdef controlFish
    // Stop the fish controller
    fishController.stop();
    #endif
    
    // Print results
    int elapsed = timer.read_us();
    printf("\n");
    printf("Buffers processed: %ld\n", count);
    printf("Elapsed time  : %d us\n", elapsed);
    printf("Per-sample time : %f us\n", (double)elapsed/(double)count/(double)sampleWindow);
    printf("  Sample frequency: %f kHz\n", (double)count*(double)sampleWindow/(double)elapsed*1000.0);
    printf("Per-buffer time : %f us\n", (double)elapsed/(double)count);
    printf("  Buffer-processing frequency: %f kHz\n", (double)count/(double)elapsed*1000.0);
    
    printf("\nComputed powers from last buffer: \n");
    int32_t* lastTonePowers = toneDetector.getTonePowers();
    for(int i = 0; i < numTones; i++)
        printf("  Tone %d: %f Hz -> %f\n", i, targetTones[i], toFloat(lastTonePowers[i]));
        
    #if defined(singleDataStream) && defined(saveData)
    printf("\nData received (%d bits):\n", dataIndex);
    fprintf(foutDataWords, "\nData received (%d bits):\n", dataIndex);
    long errors = 0;
    for(int d = 5; d < dataIndex; d++)
    {
        printf("%d", data[d]);
        fprintf(foutDataWords, "%d", data[d]);
        if(d > 0 && data[d] == data[d-1])
            errors++;
    }
    printf("\n");
    printf("errors: %ld\n", errors);
    fprintf(foutDataWords, "\n");
	fprintf(foutDataWords, "errors: %ld\n", errors);
	fclose(foutDataWords);
    #ifdef debugLEDs
    if(errors > 0)
        led1 = 1;
    #endif
    #elif defined(saveData)
    printf("\nData received (%d words):\n", dataWordIndex);
    fprintf(foutDataWords, "\nData received (%d words):\n", dataWordIndex);
    long errors = 0;
    long badWords = 0;
    for(int w = 0; w < dataWordIndex; w++)
    {
        errors = 0;
        printf("  ");
        fprintf(foutDataWords, "  ");
        for(int b = 0; b < dataWordLength; b++)
        {
            printf("%d", data[w][b]);
            fprintf(foutDataWords, "%d", data[w][b]);
            if(b > 0 && data[w][b-1] == data[w][b])
                errors++;
        }
        if(errors > 0)
        {
            printf(" X");
            fprintf(foutDataWords, " X");
            badWords++;
        }
        printf("\n");
        fprintf(foutDataWords, "\n");
    }
    printf("\nbad words: %d\n", badWords);
    fprintf(foutDataWords, "\nbad words: %d\n", badWords);
    fclose(foutDataWords);
    #ifdef debugLEDs
    if(badWords > 0)
        led1 = 1;
    #endif
    #endif
    wait(1);
    printf("\nDone!");
    wait(3);
    printf("\n");
}

void lowBatteryCallback()
{
    // Stop the tone detector
    // This will end the main call to start, causing main to terminate
    // Main will then also stop the fish controller
    toneDetector.stop();
    printf("Low battery!\n");
    // Also force the pin low to signal the Pi
    // (should have already been done, but just in case)
    DigitalOut simBatteryLow(p26);
    simBatteryLow = 0;
}





