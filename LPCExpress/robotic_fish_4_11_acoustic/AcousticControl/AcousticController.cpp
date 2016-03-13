/*
 * Author: Joseph DelPreto
 */

#include "AcousticController.h"

// The static instance
AcousticController acousticController;

// Map received state to fish values
const float pitchLookupAcoustic[] = {0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8}; // [0.2 - 0.8]
const float yawLookupAcoustic[] = {-1, -0.7, -0.5, 0, 0.5, 0.7, 1}; // [-1, 1]
const float thrustLookupAcoustic[] = {0, 0.25, 0.50, 0.75};
const float frequencyLookupAcoustic[] = {0.0000009, 0.0000012, 0.0000014, 0.0000016}; // cycles/us // NOTE also update periodHalfLookup if you update these values
const float periodHalfLookupAcoustic[] = {555555, 416666, 357142, 312500}; // 1/(2*frequencyLookup) -> us

// AGC definition
const uint8_t agcGains[] = {1, 1, 2, 5, 10, 20, 50, 100};  // the possible gains of the AGC
const uint8_t agcGainsLength = sizeof(agcGains)/sizeof(agcGains[0]);
const uint16_t signalLevelBufferLength = (agcUpdateWindow)/(sampleWindow*sampleInterval);         // first constant is window length for updating agc in microseconds
const int32_t signalLevelSumDesired = ((uint32_t)500)*((uint32_t)signalLevelBufferLength); // desire about 2 VPP, so signal minus offset should have 1V amplitude, so say mean a bit smaller, about 0.4V ~= 500

// Fish timeout
const uint16_t fishTimeoutAcousticBuffers = fishTimeoutAcousticWindow/(sampleWindow*sampleInterval); // first constant is timeout in microseconds

// Apparently it's hard to get a proper function pointer to a member function
// so this dummy method will be set as the toneDetector callback function
void toneDetectorCallback(int32_t* newTonePowers, uint32_t signalLevel)
{
	acousticController.processTonePowers(newTonePowers, signalLevel);
}

// Constructor
AcousticController::AcousticController(Serial* usbSerialObject /* = NULL */) :
    // Initialize variables
    bufferCount(0),
	lastDataWord(0),
	timeSinceGoodWord(0),
	streamCurFishStateAcoustic(0),
	streamCurFishStateEventAcoustic(0)
{
	// AGC Pins
	gain0 = new DigitalOut(agcPin0);
	gain1 = new DigitalOut(agcPin1);
	gain2 = new DigitalOut(agcPin2);
	agc[0] = gain0;
	agc[1] = gain1;
	agc[2] = gain2;
	#ifdef AGCLeds
	agcLED0 = new DigitalOut(LED1);
	agcLED1 = new DigitalOut(LED2);
	agcLED2 = new DigitalOut(LED3);
	agcLEDs[0] = agcLED0;
	agcLEDs[1] = agcLED1;
	agcLEDs[2] = agcLED2;
	#endif

	// Low battery
	lowBatteryVoltageInput = new DigitalIn(lowBatteryVoltagePin);

	// Misc
	#ifdef artificialPowers
	FILE* finPowers; = fopen("/local/powers.wp", "r");
	#endif

	// Complete initialization
	init(usbSerialObject);
}

// Initialization
void AcousticController::init(Serial* usbSerialObject /* = NULL */)
{
	// Create usb serial object or use provided one
	if(usbSerialObject == NULL)
	{
		usbSerialObject = new Serial(USBTX, USBRX);
		usbSerialObject->baud(serialDefaultBaudUSB);
	}
	usbSerial = usbSerialObject;
	// Miscellaneous
	bufferCount = 0;
	lastDataWord = 0;
	timeSinceGoodWord = 0;

	// Will check if battery is low in every buffer callback
	lowBatteryVoltageInput->mode(PullUp);
	//lowBatteryInterrupt.fall(&lowBatteryCallback);

	// TODO remove this?
	wait(1.5);

	// Configure the tone detector
	toneDetector.setCallback(&toneDetectorCallback);
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
		agc[i]->write(currentGainIndex & (1 << i));
		#ifdef AGCLeds
		agcLEDs[i].write(currentGainIndex & (1 << i));
		#endif
	}
}


// Called by toneDetector when new tone powers are computed (once per buffer)
void AcousticController::processTonePowers(int32_t* newTonePowers, uint32_t signalLevel)
{
	#ifdef streamAcousticControlLog
	acousticControlLogToStream[4] = -10;
	#endif
    // Check for low battery and if so terminate tone detector
    if(lowBatteryVoltageInput == 0)
    {
        lowBatteryCallback();
        return;
    }
    // See if we're done and if so terminate the tone detector
    if(bufferCount == loopCount)
    {
        #ifndef infiniteLoopAcoustic
        toneDetector.stop();
        return;
        #else
        //bufferCount = 0;
        #endif
    }
    bufferCount++;
    // See if we're in autonomous mode and if so just let it be
    if(fishController.autoMode)
    	return;

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
    // If not waiting out silence until next pulse is expected, see if a tone is present
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
            #ifdef singleDataStream // if we just want a stream of bits instead of segmenting into words
                #ifdef saveData
                data[dataIndex++] = tonePresent%2;
                #endif
                #ifdef streamData
                usbSerial->printf("%ld\t%d\n", bufferCount, (bool)(tonePresent%2));
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
                	timeSinceGoodWord = 0;
                	streamCurFishStateEventAcoustic = 5;
                    // Decode last word and then send it out
                    #ifdef saveData
                	decodeAcousticWord(data[dataWordIndex]);
                    dataWordIndex++;
                    #else
                    decodeAcousticWord(dataWord);
                    #endif
                    dataBitIndex = 0;
                    interWord = false;
                }
                else if(!interWord) // missed a bit
                {
                    #ifdef streamData
                	usbSerial->printf("%ld\t0\t-1\n", bufferCount);
                    #endif
					#ifdef saveData
                    for(int dbi = 0; dbi < dataWordLength; dbi++)
                    	data[dataWordIndex][dbi] = 0;
                    data[dataWordIndex][dataWordLength-1] = 1;
                    dataWordIndex++;
					#endif
					#ifdef streamAcousticControlLog
                    acousticControlLogToStream[4] = -1;
                    streamCurFishStateEventAcoustic = 1;
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
                usbSerial->printf("%ld\t0\t-2\n", bufferCount);
                #endif
				#ifdef saveData
				for(int dbi = 0; dbi < dataWordLength; dbi++)
					data[dataWordIndex][dbi] = 0;
				data[dataWordIndex][dataWordLength-2] = 1;
				dataWordIndex++;
				#endif
				#ifdef streamAcousticControlLog
				acousticControlLogToStream[4] = -2;
				streamCurFishStateEventAcoustic = 2;
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
                // Rotate through which FSK frequency we expect for the next pulse
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
    else if(periodIndex > period) // done waiting for next bit (waiting out reflections)
    {
        waitingForEnd = false;
        // Reset the sums and indicators
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
                usbSerial->printf("%d", (bool)(tonePresent%2));
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
                	decodeAcousticWord(data[dataWordIndex]);
                    dataWordIndex++;
                    #else
                    decodeAcousticWord(dataWord);
                    #endif
                    dataBitIndex = 0;
                    interWord = false;

                    timeSinceGoodWord = 0;
                }
                else if(interWord)
                {
                    #ifdef streamData
                	usbSerial->printf("0\t-1\n");
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
                usbSerial->printf("0\t-2\n");
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
    #ifdef streamSignalLevel
    //usbSerial->printf("%ld\t%d\t%d\n", signalLevel, currentGainIndex, lastDataWord);
    usbSerial->printf("%ld\t%d\n", signalLevel, currentGainIndex);
    #endif
	#ifdef streamAcousticControlLog
    acousticControlLogToStream[3] = currentGainIndex;
	#endif
    // If have seen enough readings, choose a new gain
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
            agc[i]->write(currentGainIndex & (1 << i));
            #endif
            #ifdef AGCLeds
            agcLEDs[i].write(currentGainIndex & (1 << i));
            #endif
        }
        // Reset sum
        signalLevelSum = 0;
    }

    #ifdef artificialPowers
    usbSerial->printf("%ld \t%ld \t%ld \t%d \t%ld \t%ld \t%ld \t%ld \t%d \t%d \t%d \n", newTonePowers[0], newTonePowers[1], detectSums[0], detectSums[1], powerHistorySum[0], powerHistorySum[1], powerHistoryMax[0], powerHistoryMax[1], fskIndex, waitingForEnd, periodIndex);
    #endif

    // Check for timeout
	#ifdef fishTimeoutAcoustic
    if(timeSinceGoodWord >= fishTimeoutAcousticBuffers)
    {
        #ifndef singleDataStream
        if(timeSinceGoodWord == fishTimeoutAcousticBuffers)
        {
            bool neutralWord[] = {0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}; // Decodes to state 54, which is neutral state
            decodeAcousticWord(neutralWord);
            streamCurFishStateEventAcoustic = 4;
        }
        #endif
        timeSinceGoodWord = fishTimeoutAcousticBuffers+1; // make sure we won't constantly send a neutral command (only timeout once)
    }
	#endif

    // Stream data
	#ifdef streamAcousticControlLog
    //usbSerial->printf("%ld %ld %ld %ld %ld %ld\n", bufferCount-1, acousticControlLogToStream[0], acousticControlLogToStream[1], acousticControlLogToStream[2], acousticControlLogToStream[3], acousticControlLogToStream[4]);
    //usbSerial->printf("%ld %ld %ld\n", acousticControlLogToStream[0], acousticControlLogToStream[1], acousticControlLogToStream[2]);
    //usbSerial->printf("%ld %ld %ld %ld %ld\n", acousticControlLogToStream[0], acousticControlLogToStream[1], acousticControlLogToStream[2], acousticControlLogToStream[3], acousticControlLogToStream[4]);
    //FunctionPointer fp = &tempMethod;
    //acousticControlLogToStream[0] = 6100;
    //acousticControlLogToStream[1] = 62;
    //acousticControlLogToStream[2] = 63;
    //acousticControlLogToStream[3] = 64;
    //acousticControlLogToStream[4] = -10;
    uint32_t streamGoertzel1 = acousticControlLogToStream[0];// >> 8; // second Fiji day included >> 8 to reduce bits
    uint32_t streamGoertzel2 = acousticControlLogToStream[1];// >> 8; // second Fiji day included >> 8 to reduce bits
    uint16_t streamSignalLevel = ((acousticControlLogToStream[2] >> 2) & ((uint16_t)2047)) + (((uint16_t)21) << 11); // first Fiji day was just acousticControlLogToStream[2], second day included code word
    uint8_t streamGain = acousticControlLogToStream[3] + 168; // only used in first day
    //int16_t streamWord = acousticControlLogToStream[4];
    // Decide the event to transmit, giving priority to acoustic events over button board events
    uint16_t streamFishStateEvent = fishController.streamFishStateEventController;
    if(streamCurFishStateEventAcoustic > 0)
    	streamFishStateEvent = streamCurFishStateEventAcoustic;
    uint16_t streamWord = (uint16_t)streamCurFishStateAcoustic | ((uint16_t)streamFishStateEvent << 11); // same for both Fiji days

    uint8_t* bufferOut = (uint8_t*) (&streamGoertzel1);
    usbSerial->putc(bufferOut[0]);
    usbSerial->putc(bufferOut[1]);
    usbSerial->putc(bufferOut[2]);
    usbSerial->putc(bufferOut[3]); // commented out for second day

    bufferOut = (uint8_t*) (&streamGoertzel2);
    usbSerial->putc(bufferOut[0]);
    usbSerial->putc(bufferOut[1]);
    usbSerial->putc(bufferOut[2]);
    usbSerial->putc(bufferOut[3]); // commented out for second day

	bufferOut = (uint8_t*) (&streamSignalLevel);
	usbSerial->putc(bufferOut[0]);
	usbSerial->putc(bufferOut[1]);

	bufferOut = (uint8_t*) (&streamGain);
	usbSerial->putc(bufferOut[0]); // commented out for second day

    bufferOut = (uint8_t*) (&streamWord);
    usbSerial->putc(bufferOut[0]);
    usbSerial->putc(bufferOut[1]);
    //usbSerial->printf("%d %d\n", streamCurFishState, streamWord);

	/*uint8_t* bufferOut = (uint8_t*) acousticControlLogToStream;
    for(uint8_t i = 0; i < 20; i++)
    	usbSerial->putc(bufferOut[i]);*/

    // Reset the events so we don't constantly print them
    // TODO remove this so we do constantly print them, and only look for changes in the column in case some are dropped?
    streamCurFishStateEventAcoustic = 0;
    fishController.streamFishStateEventController = 0;
	#endif
}


#ifndef singleDataStream
// Use Hamming code to decode the data word and check for an error
// TODO make constants/defines for magic numbers below that indicate length of hamming code
void AcousticController::decodeAcousticWord(volatile bool* data)
{
	// Note that the below code is written such that "5" (number of parity bits) can be a variable / #define, but it's just not yet
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
        processAcousticWord(word);
        lastDataWord = word;
        #ifdef streamData
        if(timeSinceGoodWord >= fishTimeoutAcousticBuffers)
        	usbSerial->printf("%ld\t%d\t-4\n", bufferCount, (int)word);
        else
        	usbSerial->printf("%ld\t%d\t1\n", bufferCount, (int)word);
        #endif
		#ifdef streamAcousticControlLog
        if(timeSinceGoodWord >= fishTimeoutAcousticBuffers) // check if we're timed out, since we may have reached this method when the timeout checker called us to reset the fish
        	acousticControlLogToStream[4] = -4;
        else
        	acousticControlLogToStream[4] = word;
		#endif
    }
    else // there was an uncorrectable error
    {
        #ifdef streamData
    	uint16_t word = 0;
    	for(int i = 0; i < 16; i++)
    		word += data[i] << i;
    	usbSerial->printf("%ld\t%d\t-3\n", bufferCount, word);
        #endif
		#ifdef streamAcousticControlLog
        acousticControlLogToStream[4] = -3;
        streamCurFishStateEventAcoustic = 3;
		#endif
        lastDataWord = 0;
    }
}

void AcousticController::processAcousticWord(uint16_t word)
{
    // Extract state from word
    newSelectButtonIndex = getSelectIndexAcoustic(word);
    newPitchIndex = getPitchIndexAcoustic(word);
    newYawIndex = getYawIndexAcoustic(word);
    newThrustIndex = getThrustIndexAcoustic(word);
    newFrequencyIndex = getFrequencyIndexAcoustic(word);
    // Log it
    streamCurFishStateAcoustic = getCurStateWordAcoustic;
    // Set the states
	#ifdef acousticControllerControlFish
    fishController.setSelectButton(newSelectButtonIndex > 0);
    fishController.setPitch(pitchLookupAcoustic[newPitchIndex]);
    fishController.setYaw(yawLookupAcoustic[newYawIndex]);
    fishController.setThrust(thrustLookupAcoustic[newThrustIndex]);
    fishController.setFrequency(frequencyLookupAcoustic[newFrequencyIndex], periodHalfLookupAcoustic[newFrequencyIndex]);
	#endif
}
#endif

// Stop the AcousticController
// Will stop the tone detector and will stop the fishController
// Note that the acoustic controller's run() method is blocking, so if you
//  want to use this stop method you should launch run() in a separate thread
void AcousticController::stop()
{
	// Stop the tone detector
	// This will end the toneDetector's run() method at the next buffer
	// which will cause our run() method to advance and finish up
	toneDetector.stop();
}

// Main loop
// Is blocking, so won't return until loopCount is reached or another thread calls stop()
void AcousticController::run()
{
	// Create file for logging data words
	#ifdef saveData
	int fileNum = -1;
	char filename[25];
	foutDataWords = NULL;
	do
	{
		fileNum++;
		fclose(foutDataWords);
		sprintf(filename, "/local/%d.txt", fileNum);
		foutDataWords = fopen(filename, "r");
		usbSerial->printf("%d\n", fileNum);
	} while(foutDataWords != NULL);
	foutDataWords = fopen(filename, "w");
	#endif

    #ifdef acousticControllerControlFish
    // Start the fish controller
    fishController.start();
    #endif

    #ifndef artificialPowers
    // Start listening for tones
    programTimer.start();
    toneDetector.run();
    programTimer.stop();
    toneDetector.finish(); // we won't include the time this takes to write files in the elapsed time
    #else
    // Read powers from file
    usbSerial->printf("newPower[0] \tnewPower[1] \tdetectSums[0] \tdetectSums[1] \tsum[0] \tsum[1] \tmax[0] \tmax[1] \tfskIndex \twaiting \tperiodIndex \n");
    int32_t maxSignalValTemp = 0;
    int res = fscanf(finPowers, "%ld\t%ld\n", &nextPowers[0], &nextPowers[1]);
    while(res > 0)
    {
        processTonePowers(nextPowers, maxSignalValTemp);
        res = fscanf(finPowers, "%ld\t%ld\n", &nextPowers[0], &nextPowers[1]);
    }
    fclose(finPowers);
    #endif

    #ifdef acousticControllerControlFish
    // Stop the fish controller
    fishController.stop();
    // If battery died, wait a bit for pi to clean up and shutdown and whatnot
    if(lowBatteryVoltageInput == 0)
    {
		wait(90); // Give the Pi time to shutdown
		fishController.setLEDs(255, false);
    }
    #endif

    // Print results
	#ifdef printBufferSummary
    int elapsed = programTimer.read_us();
    usbSerial->printf("\n");
    usbSerial->printf("Buffers processed: %ld\n", bufferCount);
    usbSerial->printf("Elapsed time  : %d us\n", elapsed);
    usbSerial->printf("Per-sample time : %f us\n", (double)elapsed/(double)bufferCount/(double)sampleWindow);
    usbSerial->printf("  Sample frequency: %f kHz\n", (double)bufferCount*(double)sampleWindow/(double)elapsed*1000.0);
    usbSerial->printf("Per-buffer time : %f us\n", (double)elapsed/(double)bufferCount);
    usbSerial->printf("  Buffer-processing frequency: %f kHz\n", (double)bufferCount/(double)elapsed*1000.0);

    usbSerial->printf("\nComputed powers from last buffer: \n");
    int32_t* lastTonePowers = toneDetector.getTonePowers();
    for(int i = 0; i < numTones; i++)
    	usbSerial->printf("  Tone %d: %f Hz -> %f\n", i, targetTones[i], toFloat(lastTonePowers[i]));
	#endif
    #if defined(singleDataStream) && defined(saveData)
    usbSerial->printf("\nData received (%d bits):\n", dataIndex);
    fprintf(foutDataWords, "\nData received (%d bits):\n", dataIndex);
    long errors = 0;
    for(int d = 5; d < dataIndex; d++)
    {
    	usbSerial->printf("%d", data[d]);
        fprintf(foutDataWords, "%d", data[d]);
        if(d > 0 && data[d] == data[d-1])
            errors++;
    }
    usbSerial->printf("\n");
    usbSerial->printf("errors: %ld\n", errors);
    fprintf(foutDataWords, "\n");
	fprintf(foutDataWords, "errors: %ld\n", errors);
	fclose(foutDataWords);
    #ifdef debugLEDs
    if(errors > 0)
        led1 = 1;
    #endif
    #elif defined(saveData)
    usbSerial->printf("\nData received (%d words):\n", dataWordIndex);
    fprintf(foutDataWords, "\nData received (%d words):\n", dataWordIndex);
    long errors = 0;
    long badWords = 0;
    for(int w = 0; w < dataWordIndex; w++)
    {
        errors = 0;
        usbSerial->printf("  ");
        fprintf(foutDataWords, "  ");
        for(int b = 0; b < dataWordLength; b++)
        {
        	usbSerial->printf("%d", data[w][b]);
            fprintf(foutDataWords, "%d", data[w][b]);
            if(b > 0 && data[w][b-1] == data[w][b])
                errors++;
        }
        if(errors > 0)
        {
        	usbSerial->printf(" X");
            fprintf(foutDataWords, " X");
            badWords++;
        }
        usbSerial->printf("\n");
        fprintf(foutDataWords, "\n");
    }
    usbSerial->printf("\nbad words: %d\n", badWords);
    fprintf(foutDataWords, "\nbad words: %d\n", badWords);
    fclose(foutDataWords);
    #ifdef debugLEDs
    if(badWords > 0)
        led1 = 1;
    #endif
    #endif

    // TODO remove these waits?
    wait(1);
	#ifdef printBufferSummary
    usbSerial->printf("\nAcousticController Done!");
	#endif
    wait(3);
	#ifdef printBufferSummary
    usbSerial->printf("\n");
	#endif
}


void AcousticController::lowBatteryCallback()
{
    // Stop the tone detector
    // This will end the main call to start, causing main to terminate
    // Main will also stop the fish controller once this method ends
    toneDetector.stop();
    // Also force the pin low to signal the Pi
    // (should have already been done, but just in case)
    // TODO check that this really forces it low after this method ends and the pin object may be deleted
    DigitalOut simBatteryLow(lowBatteryVoltagePin);
    simBatteryLow = 0;
}
