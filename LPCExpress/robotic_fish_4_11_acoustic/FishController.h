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
// Value ranges
#define fishMinPitch     ((float)(0.2))
#define fishMaxPitch     ((float)(0.8))
#define fishMinYaw       ((float)(-1.0))
#define fishMaxYaw       ((float)(1.0))
#define fishMinThrust    ((float)(0.0))
#define fishMaxThrust    ((float)(0.75))
#define fishMinFrequency ((float)(0.0000009))
#define fishMaxFrequency ((float)(0.0000016))
// Preset states for auto mode definition
// Each one is pitch, yaw, thrust, frequency
#define FISH_STRAIGHT 	{resetPitchValue, resetYawValue	, (fishMaxThrust + fishMinThrust)/2.0	, (fishMaxFrequency + fishMinFrequency)/2.0}
#define FISH_UP 		{fishMaxPitch	, resetYawValue	, (fishMaxThrust + fishMinThrust)/2.0	, (fishMaxFrequency + fishMinFrequency)/2.0}
#define FISH_DOWN 		{fishMinPitch	, resetYawValue	, (fishMaxThrust + fishMinThrust)/2.0	, (fishMaxFrequency + fishMinFrequency)/2.0}
#define FISH_LEFT 		{resetPitchValue, fishMaxYaw	, (fishMaxThrust + fishMinThrust)/2.0	, (fishMaxFrequency + fishMinFrequency)/2.0}
#define FISH_RIGHT 		{resetPitchValue, fishMinYaw	, (fishMaxThrust + fishMinThrust)/2.0	, (fishMaxFrequency + fishMinFrequency)/2.0}
#define FISH_STOP 		{resetPitchValue, resetYawValue	, resetThrustValue						, resetFrequencyValue}

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

/* Button board commands
  Commented indexes go from top left (0) to bottom right (5) as follows:
                 /========================|
  	  	  	   /   ____________________   |
  	  	  	 /	  |	 (0:4) (1:2) (2:1) |  |
  fish nose |     |(3:32) (4:16) (5:8) |  |  fish tail
  	  	  	 \     --------------------|  |
  	  	  	   \                          |
  	  	  	     \========================|
                 /=========================|
  	  	  	   /   ______________________  |
  	  	  	 /	  |	 (0:8) (1:16) (2:32) | |
  fish nose |     |(3:1) (4:2) (5:4)     | |  fish tail
  	  	  	 \     ----------------------| |
  	  	  	   \                           |
  	  	  	     \=========================|
  The numbers after the colons are the values to use for that button
*/
#define BTTN_FASTER      1  // 3
#define BTTN_SLOWER      2  // 4
#define BTTN_YAW_LEFT    4  // 5
#define BTTN_YAW_RIGHT   8  // 0
#define BTTN_PITCH_UP    16 // 1 // swims down
#define BTTN_PITCH_DOWN  32 // 2 // swims up
#define BTTN_RESET_MBED  36 // 2 and 5
#define BTTN_SHUTDOWN_PI 9  // 0 and 3
#define BTTN_AUTO_MODE   33 // 2 and 3
#define BTTN_BTTN_MODE   12 // 0 and 5


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
        // Set New State (which will take affect at next appropriate point in control cycle)
		void setSelectButton(bool newSelectButtonValue, bool master = false);
		void setPitch(float newPitchValue, bool master = false);
		void setYaw(float newYawValue, bool master = false);
		void setThrust(float newThrustValue, bool master = false);
		void setFrequency(float newFrequencyValue, float newPeriodHalfValue = -1, bool master = false);
		// Get (possible pending) State
		bool getSelectButton();
		float getPitch();
		float getYaw();
		float getThrust();
		float getFrequency();
		float getPeriodHalf();
		// Auto mode
		volatile bool autoMode;
		void startAutoMode();
		void stopAutoMode();
		void autoModeCallback();

		void setIgnoreExternalCommands(bool ignore);
		bool getIgnoreExternalCommands();
    private:
		// Misc State
		volatile bool ignoreExternalCommands;
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

        // Auto mode
        Ticker autoModeTicker;
        uint32_t autoModeCount;
        uint16_t autoModeIndex;
        bool ignoreExternalCommandsPreAutoMode;
};

// Create a static instance of FishController to be used by anyone doing detection
extern FishController fishController;
extern volatile uint8_t streamFishStateEvent;
extern volatile uint16_t streamCurFishState;

#endif // ifndef FISH_CONTROLLER_H
