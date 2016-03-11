/*
 * AcousticController.h
 * Author: Joseph DelPreto
 */

#ifndef FISH_CONTROLLER_H
#define FISH_CONTROLLER_H

#include "mbed.h"
#include "Servo.h"
#include "ButtonBoard.h"
#include "esc.h" // brushless motor controller

// Constants
#define PI2 6.2831853  // PI is not included with math.h for some reason

// Macros
#define getSelectIndex(d)     ((d & (1 << 0)) >> 0)
#define getPitchIndex(d)      ((d & (7 << 1)) >> 1)
#define getYawIndex(d)        ((d & (7 << 4)) >> 4)
#define getThrustIndex(d)     ((d & (3 << 7)) >> 7)
#define getFrequencyIndex(d)  ((d & (3 << 9)) >> 9)

#define getSelectButton()   (newSelectButtonIndex > 0)
#define getPitch()           pitchLookup[newPitchIndex]
#define getYaw()               yawLookup[newYawIndex]
#define getThrust()         thrustLookup[newThrustIndex]
#define getFrequency()   frequencyLookup[newFrequencyIndex]
#define getPeriodHalf() periodHalfLookup[newFrequencyIndex]

#define getCurStateWord (uint16_t)((uint16_t)newSelectButtonIndex + ((uint16_t)newPitchIndex << 1) + ((uint16_t)newYawIndex << 4) + ((uint16_t)newThrustIndex << 7) + ((uint16_t)newFrequencyIndex << 9));

class FishController
{
    public:
        // Initialization
        FishController();
        void start();
        void stop();
        // Processing
        void processAcousticWord(uint16_t word);
        void tickerCallback();
        // Debug
        #ifdef debugFishState
        void printDebugState();
        #endif
        // LEDs
        void setLEDs(char mask, bool turnOn);
        volatile bool autoMode;
    private:
        // State extracted from data words (index into lookup tables)
        volatile uint8_t newSelectButtonIndex;
        volatile uint8_t newPitchIndex;
        volatile uint8_t newYawIndex;
        volatile uint8_t newThrustIndex;
        volatile uint8_t newFrequencyIndex;
        // State currently executing on fish
        volatile bool selectButton;
        volatile float pitch;
        volatile float yaw;
        volatile float thrust;
        volatile float thrustCommand;
        volatile float frequency;
        volatile float periodHalf;
        volatile float dutyCycle;
        volatile bool brushlessOff;
        // Ticker for controlling tail
        Ticker ticker;
        const uint16_t tickerInterval;
        volatile uint32_t curTime;
        volatile bool fullCycle;
        const float raiser;
        volatile bool inTickerCallback;
        
        // Outputs for motor and servos
        PwmOut motorPWM;
        DigitalOut motorOutA;
        DigitalOut motorOutB;
        Servo servoLeft;
        Servo servoRight;
        //PwmOut brushlessMotor;
        const uint32_t brushlessOffTime;
        
        // Button control
        ButtonBoard buttonBoard;
        static void buttonCallback(char button, bool pressed, char state);
};

// Create a static instance of FishController to be used by anyone doing detection
extern FishController fishController;
extern volatile uint8_t streamFishStateEvent;
extern volatile uint16_t streamCurFishState;

#endif // ifndef FISH_CONTROLLER_H
