/*
 * JoystickController.h
 *
 * Author: Joseph DelPreto
 */

#ifndef SERIALCONTROL_SERIALCONTROLLER_H_
#define SERIALCONTROL_SERIALCONTROLLER_H_

#include "FishController.h"
#include "mbed.h"

#define serialDefaultBaudUSB 115200
#define serialDefaultBaud 115200
#define serialDefaultTX p13
#define serialDefaultRX p14
// note lowBatteryVoltagePin is defined in FishController

//#define printStatusSerialController // whether to print what's going on (i.e. when it gets commands, etc.)
#define debugLEDsSerial    // LED1: initialized LED2: running LED3: receiving a character LED4: done (others turn off)
#define runTimeSerial 10000 	   // how long to run for (in milliseconds) if inifiniteLoopSerial is undefined
#define infiniteLoopSerial // if defined, will run forever (or until stop() is called from another thread)

#define serialControllerControlFish // whether to start fishController to control the servos and motor

// Map bytes sent over serial (1-255) to ranges for each fish property
#define serialMinPitch     fishMinPitch
#define serialMaxPitch     fishMaxPitch
#define serialMinYaw       fishMinYaw
#define serialMaxYaw       fishMaxYaw
#define serialMinThrust    fishMinThrust
#define serialMaxThrust    fishMaxThrust
#define serialMinFrequency fishMinFrequency
#define serialMaxFrequency fishMaxFrequency

class SerialController
{
public:
	// Initialization
	SerialController(Serial* serialObject = NULL, Serial* usbSerialObject = NULL); // if objects are null, ones will be created
	void init(Serial* serialObject = NULL, Serial* usbSerialObject = NULL); // if serial objects are null, ones will be created
	// Execution control
	void run();
	void stop();
	void lowBatteryCallback();
private:
	Timer programTimer;
	bool terminated;
	Serial* usbSerial;
	Serial* serial;

	void processSerialWord(uint8_t* word);

	// Low battery monitor
	DigitalIn* lowBatteryVoltageInput;
	//InterruptIn* lowBatteryInterrupt;
	Ticker lowBatteryTicker;
	bool detectedLowBattery;

	// Debug LEDs
	#ifdef debugLEDsSerial
	DigitalOut* serialLEDs[4];
	#endif
};

// Create a static instance of SerialController to be used by anyone doing serial control
extern SerialController serialController;

#endif /* SERIALCONTROL_SERIALCONTROLLER_H_ */

