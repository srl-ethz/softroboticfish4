/**
 * Author: Joseph DelPreto
 * A class for sampling a signal and detecting whether certain frequencies are present
 * Implements the Goertzel algorithm for tone detection
 *   Uses fixed-point arithmetic to a > 10x speedup over a floating-point implementation
 *   Uses Q15 number format
 *
 * Can easily configure the sampling frequency, the tones to detect, and the length of the buffer window
 * Can set a callback function to be called at the end of each buffer processing to see the relative powers of each target tone
 *   Make sure the callback function execution time, plus the time to process a buffer of samples, takes less time than it takes to acquire the buffer!
 *   You can check how long it takes to process a buffer of samples by enabling "timeProcess" below then using initProcessTimer() and getProcessTimes()
 *
 * Debugging options such as LEDs to indicate status are also available
 * Can also choose to time each processing stage to ensure it is not taking longer the time needed to acquire the buffer of samples
 * Can also record samples and write them to a file (max 1000 will be recorded)
 * LED pattern if enabled:
 *   LED1 indicates tone detection in progress
 *   LED2 indicates at least two buffers have been filled
 *   LED3 turns on when processing starts and off when it finishes (so lower duty cycle is better)
 *   LED4 turns on when tone detection is stopped (and then other LEDs turn off)
 * Debug pins will go high/low in same pattern as described above for LEDs, but using pins p5-p8
 *
 * Test mode can be enabled, in which case samples can be artificially generated instead of using the ADC
 *
 * Currently only set up for one instance of ToneDetector to be in operation
 *   Use the object toneDetector, declared in this file, for all of your tone detecting needs!
 *
 */

#ifndef TONE_DETECTOR_H
#define TONE_DETECTOR_H

//#define streamAcousticControlLog // for every buffer, streams goertzel powers f1, goerztel powers f2, signal level, gain, fish state/events over Serial

// Configuration
#define sampleInterval 4  // us between samples
#define sampleWindow 125  // number of samples in a Goertzel buffer
#define numTones 2
#define numFSKGroups 1
static const double targetTones[numTones] = {36000, 30000};  // Tones to detect (in Hz): first tone is a 0 bit second is a 1 bit

// Test / Debugging options
//#define artificialSamplesMode // if this is defined, should define either sampleFilename OR sumSampleFrequencies
//#define sampleFilename "/local/2tone11.txt"
//#define sumSampleFrequencies {10000,30000} // will be used to initialize float[] array.  Test signal will be summation of cosines with these frequencies (in Hz)

#define debugLEDs
#define debugPins
#define recordStreaming // print to COM each sample/output (save everything), as opposed to only write last few hundred to a file (but faster since don't write to file while processing)
						// note that either recordOutput or recordSamples must be undefined to actually record anything
//#define recordOutput  // save tone powers - will either stream to COM or save the last numSavedTonePowers powers to a file (based on recordStreaming)
//#define recordSamples // save samples - will either stream to COM or save the last numSavedSamples samples to a file (based on recordStreaming)


#if defined(recordSamples) && !defined(recordStreaming)
#define numSavedSamples 1000
#endif
#if defined(recordOutput) && !defined(recordStreaming)
#define numSavedTonePowers 250
#endif

// Will use fixed-point arithmetic for speed
//   numbers will be int32_t with 10 bits as fractional portion
//   note: this means the 12 bits from the ADC can be used directly, implicitly scaling them to be floats in range [0,4]
//     (so if you want to change the Q format, make sure to shift the samples accordingly in the processSamples() method)
#define toFixedPoint(floatNum) ((int32_t)((float)(floatNum) * 1024.0f))
#define toFloat(fixedPointNum) ((float)(fixedPointNum)/1024.0f)
#define fixedPointMultiply(a,b) ((int32_t)(((int64_t)a * (int64_t)b) >> 10))

// Constants
#define tonePowersWindow 32 // change to 5 for threshold1

// Include files
#include <stdint.h>   // for types like int32_t
#include <math.h>     // for cos // TODO remove this once samples are real samples
#define PI 3.1415926  // not included with math.h for some reason
#include "mbed.h"     // for the ticker
#ifndef artificialSamplesMode
#include "MODDMA.h"   // DMA for reading the ADC
#endif

// Debugging
#ifdef debugLEDs
static DigitalOut led1(LED1);
static DigitalOut led2(LED2);
static DigitalOut led3(LED3);
static DigitalOut led4(LED4);
#endif
#ifdef debugPins
static DigitalOut debugPin1(p5);
static DigitalOut debugPin2(p6);
static DigitalOut debugPin3(p7);
static DigitalOut debugPin4(p8);
#endif

// Define the ToneDetector!
class ToneDetector
{
    public:
        // Initialization
        ToneDetector();
        void init();
        void setCallback(void (*myFunction)(int32_t* tonePowers, uint32_t signalLevel));
        // Execution Control
        virtual void run();      // main loop, runs forever or until stop() is called
        virtual void stop();      // stop the main loop
        void finish();            // write final output to files and whatnot
        volatile bool terminated; // indicates main loop should stop (may be set by error callback or by stop())
        // Sampling / Processing (these are public to allow DMA callbacks to access them)
        volatile bool fillingBuffer0;    // Whether sampling is currently writing to buffer 0 or buffer 1
        volatile bool transferComplete;  // Signals that samplesProcessing is ready to be processed
        uint32_t sampleBuffer0[sampleWindow];         // Buffer of samples used by one DMA operation
        uint32_t sampleBuffer1[sampleWindow];         // Buffer of samples used by second DMA operation
        volatile uint32_t* samplesWriting;    // Pointer to the buffer to which we should write (alternates between buffer0 and buffer1)
        volatile uint32_t* samplesProcessing; // Pointer to the buffer which we should process (alternates between buffer0 and buffer1)
        int32_t* getTonePowers();        // Get the most recent tone powers
        #ifndef artificialSamplesMode
        MODDMA dma;  // DMA controller
        #endif

    private:
        // Initialization
        bool readyToBegin;              // Whether sample window, sample interval, and tones have all been specified
        #ifndef artificialSamplesMode
        MODDMA_Config *dmaConf;         // Overall DMA configuration
        MODDMA_LLI *lli[2];             // Linked List Items for chaining DMA operations (will have one for each ADC buffer)
        void startDMA();
        void startADC();
        #endif
        // Sampling
        // Goertzel Processing (arrays will have an element per desired tone)
        //   Tone detecting
        int32_t goertzelCoefficients[numTones];  // Pre-computed coefficients based on target tones
        int32_t tonePowersSum[numTones];         // Result of Goertzel algorithm (summed over tonePowersWindow to smooth results)
        int32_t tonePowers[numTones][tonePowersWindow];           // For each tone, store the most recent tonePowersWindow powers (in circular buffers)
        uint8_t tonePowersWindowIndex;  // Index into the circular buffers of tonePowers
        bool readyToThreshold;          // Whether thresholding is ready, or first buffer is still being filled
        void (*callbackFunction)(int32_t* tonePowers, uint32_t signalLevel);  // Called when new powers are computed
        void processSamples();          // Process a buffer of samples to detect tones

        // Testing / Debugging
        #ifdef artificialSamplesMode
        Ticker sampleTicker;            // Triggers a sample to be read from the pre-filled array
        void initTestModeSamples();     // Creates the samples, either from a file or by summing tones
        void tickerCallback();          // Called each time a sample is read
        uint32_t* testSamples;          // Array of pre-stored sample values to use
        uint16_t numTestSamples;        // Length of the testSamples array (need not be same as buffer length)
        volatile uint16_t testSampleIndex;  // Current index into testSamples
        volatile uint16_t sampleIndex;  // Index into current sample buffer
        #endif
        #ifndef recordStreaming
            #ifdef recordSamples
            uint16_t savedSamplesIndex;
            uint32_t savedSamples[numSavedSamples];
            #endif
            #ifdef recordOutput
            uint16_t savedTonePowersIndex;
            int32_t savedTonePowers[numSavedTonePowers][numTones];
            #endif
        #endif
};

// DMA IRQ callback methods
void TC0_callback();    // Callback for when DMA channel 0 has filled a buffer
void ERR0_callback();   // Error on dma channel 0

// Create a static instance of ToneDetector to be used by anyone doing detection
//   This allows the ticker callback to be a member function
extern ToneDetector toneDetector;
#ifdef streamAcousticControlLog
extern int32_t acousticControlLogToStream[5]; // goertzel powers f1, goerztel powers f2, signal level, gain, events (events is deprecated)
#endif

#endif // ifndef TONE_DETECTOR_H




