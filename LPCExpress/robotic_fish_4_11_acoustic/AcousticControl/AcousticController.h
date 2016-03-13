/*
 * AcousticController.h
 * Author: Joseph DelPreto
 */

#ifndef ACOUSTICCONTROL_ACOUSTICCONTROLLER_H_
#define ACOUSTICCONTROL_ACOUSTICCONTROLLER_H_

#include "ToneDetector.h"
#include "FishController.h"
#include "mbed.h"

// ===== INPUT/OUTPUT =====
// NOTE: To be silent (to not use usbSerial object at all), undefine printBufferSummary, streamAcousticControlLog, streamData, and streamSignalLevel
//       To stream the log info used in Fiji, just define streamAcousticControlLog - these are not human-readable streams
//       To view the data as it's being received/decoded, define streamData (or streamSignalLevel for signal level) - these are human-readable streams
//       To view a data summary (ex. number of buffers processed, words received, etc.) define printBufferSummary
#define serialDefaultBaudUSB 921600 // IMPORTANT: must be 921600 when using streaAcousticControlLog (to be fast enough to keep up with data stream)
#define printBufferSummary    // print a summary of how many buffers were processed, the last computed tone powers, etc. once the controller is stopped
/* SEE ALSO: streamAcousticControlLog, defined in ToneDetector.h
   If defined, will update the following (see end of processTonePowers for how they're packaged):

 acousticControlLogToStream:
 [0]: goertzel powers f1
 [1]: goerztel powers f2
 [2]: signal level
 [3]: AGC gain
 [4]: fish state/events over Serial
        -1: erroneously missed a bit
		-2: erroneously added a bit
		-3: detected an uncorrectable error in the Hamming decoding
		-4: fish timeout (reset to neutral)
		>0: the data word that was successfully decoded and processed
		-10: no event

UPDATE:

  acousticControlLogToStream[4] has been deprecated and replaced with streamCurFishStateEventAcoustic and streamCurFishStateAcoustic
  streamCurFishStateEventAcoustic can have the following values:
	  1: erroneously missed a bit
	  2: erroneously added a bit
	  3: detected an uncorrectable error in the Hamming decoding
	  4: fish timeout (reset to neutral)
	  5: successfully received a word (note streamCurFishState indicates the current fish state)
	  6: button board yaw left
	  7: button board yaw right
	  8: button board faster
	  9: button board slower
	  10: button board pitch up
	  11: button board pitch down
	  12: button board shutdown pi
	  13: button board reset mbed
	  14: button board auto mode
	  15: button board unknown
*/

//#define artificialPowers // will use tone powers saved in a file rather than actually sampling (expects two tab-separated columns of powers in /local/powers.wp)
//#define singleDataStream // just read a single continuous stream of bits (as opposed to segmenting into words/decoding)

//#define saveData   // save data such as received words to an array, then save it to a file at the end - faster than printing continuously but worry about overflow
//#define streamData // stream data to the screen as it's received (controlled independently of saveData)
//#define streamSignalLevel // stream current signal level to the screen

// ===== EXECUTION =====
//#define infiniteLoopAcoustic  // if undefined, will stop after loopCount buffers are processed
#define loopCount 20000       // number of buffers to process before terminating the program (if infiniteLoopAcoustic is not defined)
#define fishTimeoutAcoustic   // whether to reset the fish to neutral if an acoustic word hasn't been received in a while.  Not used with singleDataStream.
#define fishTimeoutAcousticWindow 5000000 // time between good words that cause a timeout (in microseconds)

#define acousticControllerControlFish // whether to start fishController to control the servos and motor

#define useAGC        // if undefined, will only set agc (adjustable gain control of the acoustic input board) once at startup
//#define AGCLeds     // if defined, will light the mBed LEDs to indicate the chosen agc gain index (whether or not useAGC is on)
#define agcUpdateWindow 10000000 // how often to update the AGC gain (in microseconds)

#define dataRate 20 // data rate in bps.  Can be 20, 50, 77, or 100.  20 was used in Fiji. Above 50 may be less reliable.

//#define threshold1 // older algorithm for processing Goertzel buffers (not sure if it still works)
#define threshold2

// ===== PINS =====
#define agcPin0 p16
#define agcPin1 p17
#define agcPin2 p18
// note lowBatteryVoltagePin is defined in FishController

// ===== Sampling / Thresholding configuration =====
// 20 bps (5/45 ms tone/silence)
#if dataRate == 20
#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
#define period 80          // the number of buffers from a rising edge to when we start looking for the next rising edge
#define bitPeriod 100      // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
#define interWordWait 300  // how many buffers of silence to expect between words
#elif dataRate == 50
// 50 bps (5/15 ms tone/silence)
#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
#define period 20          // the number of buffers from a rising edge to when we start looking for the next rising edge
#define bitPeriod 40       // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
#define interWordWait 300  // how many buffers of silence to expect between words
#elif dataRate == 77
// 77 bps (5/8 ms tone/silence)
#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
#define period 10          // the number of buffers from a rising edge to when we start looking for the next rising edge
#define bitPeriod 26       // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
#define interWordWait 300  // how many buffers of silence to expect between words
#elif dataRate == 100
// 100 bps (5/5 ms tone/silence)
#define detectWindow 10    // number of buffers to look at when deciding if a tone is present
#define detectThresh 6     // number of 1s that must be present in a detectWindow group of buffer outputs to declare a tone present
#define period 6           // the number of buffers from a rising edge to when we start looking for the next rising edge
#define bitPeriod 20       // only used with saveData: how many buffers it should actually be between rising edges (only used to size the data array)
#define interWordWait 300  // how many buffers of silence to expect between words
#endif

// ===== Macros for extracting state from data words =====
#define getSelectIndexAcoustic(d)     ((d & (1 << 0)) >> 0)
#define getPitchIndexAcoustic(d)      ((d & (7 << 1)) >> 1)
#define getYawIndexAcoustic(d)        ((d & (7 << 4)) >> 4)
#define getThrustIndexAcoustic(d)     ((d & (3 << 7)) >> 7)
#define getFrequencyIndexAcoustic(d)  ((d & (3 << 9)) >> 9)

#define getCurStateWordAcoustic (uint16_t)((uint16_t)newSelectButtonIndex + ((uint16_t)newPitchIndex << 1) + ((uint16_t)newYawIndex << 4) + ((uint16_t)newThrustIndex << 7) + ((uint16_t)newFrequencyIndex << 9));

class AcousticController
{
	public:
		// Initialization
		AcousticController(Serial* usbSerialObject = NULL); // if serial object is null, one will be created
		void init(Serial* usbSerialObject = NULL); // if serial object is null, one will be created
		// Execution control
		void run();
		void stop();
		// Called by toneDetector when new tone powers are computed
		// Only public since we needed a dummy non-member function to call it from the static instance to pass a pointer to tonedetector (see comments on toneDetectorCallback)
		void processTonePowers(int32_t* newTonePowers, uint32_t signalLevel);
	private:
		// Control
		volatile uint32_t bufferCount;
		Timer programTimer;
		Serial* usbSerial;

		#ifndef singleDataStream
		void decodeAcousticWord(volatile bool* data);  // Use Hamming code to decode the data word and check for an error
		void processAcousticWord(uint16_t word);       // Parse the decoded word into the various fish states
		#endif

		// TODO check what actually needs to be volatile

		// Data detection
		volatile bool waitingForEnd;     // got data, now waiting until next bit transmission is expected (waiting until "period" buffers have elapsed since edge)
		volatile uint16_t periodIndex;   // how many buffers it's been since the last clock edge
		volatile uint8_t fskIndex;       // which set of 0/1 frequencies are next expected

		#if defined(threshold2)
		#define powerHistoryLength 50       // number of buffer results to consider when deciding if a tone is present.  should be half of a bit-width
		#define powerHistoryDetectWindow 20 // portion of powerHistory to consider as the detection zone (where a peak is expected)
		volatile int32_t powerHistory[powerHistoryLength][numTones];  // History of Goertzel results for the last powerHistoryLength buffers
		volatile int16_t powerHistoryIndex;                   // Index into powerHistory (circular buffer)
		volatile int16_t powerHistoryDetectIndex;             // Marks where in powerHistory to start window for detection; will always be detectWindow elements behind powerHistoryIndex in the circular buffer
		volatile int32_t powerHistorySumDetect[numTones];     // Sum of powers stored in powerHistory in the detect window (stand-in for mean)
		volatile int32_t powerHistorySumNoDetect[numTones];   // Sum of powers stored in powerHistory outside the detect window (stand-in for mean)
		volatile int32_t powerHistoryMaxDetect[numTones];     // Max of powers stored in powerHistory in the detect window
		volatile int32_t powerHistoryMaxNoDetect[numTones];   // Max of powers stored in powerHistory outside the detect window
		volatile bool readyToThreshold;                       // Whether powerHistory has been filled at least once
		volatile bool tonesPresent[detectWindow][numTones];   // Circular buffer of whether tones were detected in last detectWindow powers
		volatile uint8_t detectSums[numTones];                // Rolling sum over last detectWindow tonesPresent results (when > detectWindow/2, declares data bit)
		volatile uint8_t detectWindowIndex;
		#elif defined(threshold1)
		#define powerHistoryLength 50                       // number of results to consider when deciding if a tone is present.  should be half of a bit-width
		volatile uint8_t detectSums[numTones];              // Rolling sum over last detectWindow buffer results (when > detectWindow/2, declares clock/data)
		int32_t powerHistory[powerHistoryLength][numTones]; // History of Goertzel results for the last powerHistoryLength buffers
		volatile int16_t powerHistoryIndex;                 // Index into powerHistory (circular buffer)
		volatile int16_t powerHistoryDetectIndex;           // Marks where in powerHistory to start window for detection; will always be detectWindow elements behind powerHistoryIndex in the circular buffer
		int32_t powerHistorySum[numTones];                  // Sum of powers stored in powerHistory (stand-in for mean)
		int32_t powerHistoryMax[numTones];                  // Max of powers stored in powerHistory
		volatile bool readyToThreshold;                     // Whether powerHistory has been filled at least once
		#endif

		// Segmenting of data into bits/words
		#if defined(singleDataStream) && defined(saveData)
			volatile bool data[loopCount/(bitPeriod) * (numTones-1) + 50]; // Received data (each clock pulse is associated with numTones-1 bits) // TODO update for multi-hop FSK, but this is fine for one FSK group
			volatile uint16_t dataIndex;
		#else
			#define dataWordLength 16          // bits per word.  note that changing this also requires adjusting the decodeAcousticWord algorithm, since it currently assumes Hamming encoding
			volatile uint8_t dataBitIndex;     // current bit of that word
			volatile bool interWord;           // whether we are currently in between words (waiting out the long inter-word pause)
			#ifdef saveData
			volatile bool data[loopCount/(bitPeriod*dataWordLength)][dataWordLength]; // Each element is a word as an array of bits
			volatile uint16_t dataWordIndex;   // current data word
			#else
			volatile bool dataWord[dataWordLength]; // an array of bits making up the current word
			#endif
		#endif
		volatile uint16_t lastDataWord; // not used anymore, but still updated

		// Adjustable gain control (AGC)
		volatile uint8_t currentGainIndex; 		  // the current index into agcGains
		volatile int32_t signalLevelSum; 		  // the running sum of signal level readings
		volatile uint16_t signalLevelBufferIndex; // index into signal level buffer // note we don't have a buffer anymore, but use this to know when to reset the sum

		DigitalOut* gain0;
		DigitalOut* gain1;
		DigitalOut* gain2;
		DigitalOut* agc[3]; // facilitates loops and whatnot
		#ifdef AGCLeds
			#undef debugLEDs
			DigitalOut* agcLED0;
			DigitalOut* agcLED1;
			DigitalOut* agcLED2;
			DigitalOut* agcLEDs[3];
		#endif

		// Fish reset timeout
		volatile uint16_t timeSinceGoodWord; // measured in buffers

		// Low battery monitor
		DigitalIn* lowBatteryVoltageInput;
		void lowBatteryCallback();

		// Testing/debugging
		#ifdef saveData
		LocalFileSystem local("local");
		FILE* foutDataWords;
		#endif
		#ifdef artificialPowers
		    #if (!defined recordStreaming || (!defined recordOutput && !defined recordSamples)) && !defined saveData
		    LocalFileSystem local("local");
		    #endif
		    FILE* finPowers;
		    int32_t nextPowers[numTones];
		#endif
		volatile uint16_t streamCurFishStateAcoustic; // the current state of the fish
		volatile uint16_t streamCurFishStateEventAcoustic; // events such as decoding words, errors, etc.

		// Fish state extracted from acoustic data word (index into lookup tables defined above)
		volatile uint8_t newSelectButtonIndex;
		volatile uint8_t newPitchIndex;
		volatile uint8_t newYawIndex;
		volatile uint8_t newThrustIndex;
		volatile uint8_t newFrequencyIndex;
};


// Create a static instance of AcousticController to be used by anyone doing detection
extern AcousticController acousticController;

#endif
