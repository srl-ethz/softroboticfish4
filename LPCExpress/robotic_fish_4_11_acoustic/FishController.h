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

// Control
#define fishControllerTickerInterval 1000 // how often to call the control ticker, in microseconds

// Constants
#define PI2 6.2831853  // PI is not included with math.h for some reason
// Values to use for resetting the fish to neutral
#define resetSelectButtonValue 0
#define resetPitchValue        0.5
#define resetYawValue          0
#define resetThrustValue       0
#define resetFrequencyValue    0.0000012 // cycles/us
#define resetPeriodHalfValue   416666    // 1/(2*frequency) -> us
// Pins
#define lowBatteryVoltagePin p26

#define motorPWMPin   p23
#define motorOutAPin  p11
#define motorOutBPin  p12
#define servoLeftPin  p21
#define servoRightPin p24

#define buttonBoardSDAPin  p9
#define buttonBoardSCLPin  p10
#define buttonBoardInt1Pin p29
#define buttonBoardInt2Pin p30

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
        // Debug / Logging
        volatile uint8_t streamFishStateEventController; // will indicate the last button board event - up to the caller to reset it if desired
        #ifdef debugFishState
        void printDebugState();
        #endif
        // LEDs
        void setLEDs(char mask, bool turnOn);
        volatile bool autoMode;
        // Set New State (which will take affect at next appropriate point in control cycle)
		void setSelectButton(bool newSelectButtonValue);
		void setPitch(float newPitchValue);
		void setYaw(float newYawValue);
		void setThrust(float newThrustValue);
		void setFrequency(float newFrequencyValue, float newPeriodHalfValue);
		// Get (possible pending) State
		bool getSelectButton();
		float getPitch();
		float getYaw();
		float getThrust();
		float getFrequency();
		float getPeriodHalf();
    private:
		// State which will be applied at the next appropriate time in the control cycle
		volatile bool newSelectButton;
		volatile float newPitch;
		volatile float newYaw;
		volatile float newThrust;
		volatile float newFrequency;
		volatile float newPeriodHalf;
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
