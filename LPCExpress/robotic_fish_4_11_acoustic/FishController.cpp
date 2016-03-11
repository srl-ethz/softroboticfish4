
#include "FishController.h"

// The static instance
FishController fishController;

// Function to reset mbed
extern "C" void mbed_reset();

//============================================
// Initialization
//============================================

// Constructor
FishController::FishController() :
    // Initialize variables
	autoMode(false),
    tickerInterval(fishControllerTickerInterval),
    curTime(0),
    fullCycle(true),
    raiser(3.5),
	inTickerCallback(false),
    // Outputs for motor and servos
    motorPWM(motorPWMPin),
    motorOutA(motorOutAPin),
    motorOutB(motorOutBPin),
    servoLeft(servoLeftPin),
    servoRight(servoRightPin),
    //brushlessMotor(p25),
    brushlessOffTime(30000),
    // Button board
    buttonBoard(buttonBoardSDAPin, buttonBoardSCLPin, buttonBoardInt1Pin, buttonBoardInt2Pin) // sda, scl, int1, int2
{
	streamFishStateEventController = 0;

    newSelectButton = resetSelectButtonValue;
    newPitch = resetPitchValue;
    newYaw = resetYawValue;
    newThrust = resetThrustValue;
    newFrequency = resetFrequencyValue;
    newPeriodHalf = resetPeriodHalfValue;
    
    selectButton = newSelectButton;
    pitch = newPitch;
    yaw = newYaw;
    thrust = newThrust;
    thrustCommand = 0;
    frequency = newFrequency;
    periodHalf = newPeriodHalf;
    dutyCycle = 0;
    brushlessOff = false;
    
    buttonBoard.registerCallback(&FishController::buttonCallback);
    buttonBoard.setLEDs(255, false);
}

// Set the desired state
// They will take affect at the next appropriate time in the control cycle
void FishController::setSelectButton(bool newSelectButtonValue) {newSelectButton = newSelectButtonValue;}
void FishController::setPitch(float newPitchValue) {newPitch = newPitchValue;}
void FishController::setYaw(float newYawValue) {newYaw = newYawValue;}
void FishController::setThrust(float newThrustValue) {newThrust = newThrustValue;}
void FishController::setFrequency(float newFrequencyValue, float newPeriodHalfValue) {newFrequency = newFrequencyValue; newPeriodHalf = newPeriodHalfValue;}
// Get the (possible pending) state
bool FishController::getSelectButton() {return newSelectButton;}
float FishController::getPitch() {return newPitch;}
float FishController::getYaw() {return newYaw;}
float FishController::getThrust() {return newThrust;}
float FishController::getFrequency() {return newFrequency;}
float FishController::getPeriodHalf() {return newPeriodHalf;}

void FishController::start()
{
    //printf("Arming brushless motor\n");
    //wait_ms(3000);
    //Timer timemotor;
    //timemotor.start();
    //brushlessMotor.period_ms(20);
    //brushlessMotor.pulsewidth_us(1500); // neutral position
    //brushlessMotor();
    //timemotor.stop();
    //printf("Setting took %d us\n", timemotor.read_us());
    //wait_ms(3000); // to arm brushless motor
    
    // Blink button board LEDs to indicate startup
    for(uint8_t i = 0; i < 3; i++)
    {
        buttonBoard.setLEDs(255, true);
        wait_ms(500);
        buttonBoard.setLEDs(255, false);
        wait_ms(500);
    }
    
    // Start control ticker callback
    ticker.attach_us(&fishController, &FishController::tickerCallback, tickerInterval);
//    #ifdef debugFishState
//    printf("Starting...\n");
//    #endif
}

void FishController::stop()
{
    // Stop updating the fish
    while(inTickerCallback); // wait for commands to settle
    ticker.detach(); // stop updating commands
    wait_ms(5);      // wait a bit to make sure it stops
    
    // Reset fish state to neutral
    newSelectButton = resetSelectButtonValue;
    newPitch = resetPitchValue;
    newYaw = resetYawValue;
    newThrust = resetThrustValue;
    newFrequency = resetFrequencyValue;
    newPeriodHalf = resetPeriodHalfValue;
    // Send commands to fish (multiple times to make sure we get in the right part of the cycle to actually update it)
    for(int i = 0; i < 200; i++)
    {
    	tickerCallback();
    	wait_ms(10);
    }
    // Make sure commands are sent to motors and applied
    wait(1);
    
    // Put dive planes in a weird position to indicate stopped
    servoLeft = 0.3;
    servoRight = 0.3;
    
    // Light the LEDs to indicate termination
    buttonBoard.setLEDs(255, true);
}

//============================================
// Processing
//============================================ 
void FishController::tickerCallback()
{
    inTickerCallback = true; // so we don't asynchronously stop the controller in a bad point of the cycle
    
    // get the current elapsed time since last reset (us)
    curTime += tickerInterval; 

    // see if brushless should be shut down
    brushlessOff = curTime > (periodHalf-brushlessOffTime);
    
    // update every half cycle
    if(curTime > periodHalf) 
    { 
        // read new yaw value every half cycle
        yaw = newYaw; // a value from -1 to 1

        // Read frequency only every full cycle
        if(fullCycle) 
        { 
            // Read other new inputs
            thrust = newThrust; // a value from 0 to 1
            frequency = newFrequency;
            periodHalf = newPeriodHalf;
            // Adjust thrust if needed
            if(yaw < 0.0)
                thrustCommand = (1.0 + 0.75*yaw)*thrust; // 0.7 can be adjusted to a power of 2 if needed
            else
            	thrustCommand = thrust;
            fullCycle = false;
        } 
        else 
        {
            // Reverse for the downward slope
            if(yaw > 0.0)
            	thrustCommand = -(1.0 - 0.75*yaw)*thrust;
            else
            	thrustCommand = -thrust;
            fullCycle = true;
        }

        // Reset time
        curTime = 0;
    }

    // Update the servos
    pitch = newPitch;
    servoLeft = pitch - 0.05; // The 0.03 calibrates the angles of the servo
    servoRight = (1.0 - pitch) < 0.03 ? 0.03 : (1.0 - pitch);

    // Update the duty cycle
    dutyCycle = raiser * sin(PI2 * frequency * curTime); // add factor 4.0 to get a cut off sinus
    if(dutyCycle > 1)
        dutyCycle = 1;
    if(dutyCycle < -1)
        dutyCycle = -1;
    dutyCycle *= thrustCommand;
    if(dutyCycle >= 0 && dutyCycle < 0.01)
        dutyCycle = 0;
    if(dutyCycle < 0 && dutyCycle > -0.01)
        dutyCycle = 0;
    // Update the brushed motor
    if(dutyCycle >= 0) 
    {
        motorOutA.write(0);
        motorOutB.write(1);
        motorPWM.write(dutyCycle);
    }
    else 
    {
        motorOutA.write(1);
        motorOutB.write(0);
        motorPWM.write(-1 * dutyCycle);
    }
    // Update the brushless motor
    //brushlessMotor = dutyCycle * !brushlessOff;
    //brushlessMotor.pulsewidth_us(dutyCycle*500+1500);
    //brushlessMotor();
    
    
//    #ifdef debugFishState
//    //printDebugState();
//    #endif
    //printf("%f\n", dutyCycle);
    inTickerCallback = false;
}

// button will be mask indicating which button triggered this interrupt
// pressed will indicate whether that button was pressed or released
// buttonState will be a mask that indicates which buttons are currently pressed
void FishController::buttonCallback(char button, bool pressed, char state) // static
{
    //printf("button %d\t pressed: %d\t state: %d\n", button, pressed, state);
    //fishController.buttonBoard.setLEDs(button, !fishController.buttonBoard.getLEDs(button));
    // Only act on button presses (not releases)
    if(!pressed)
        return;
        
    DigitalOut* simBatteryLow;
    float newYaw, newThrust, newPitch;
    switch(state)
    {
        case BTTN_YAW_LEFT:
        	newYaw = fishController.newYaw;
        	newYaw -= 0.5;
        	newYaw = newYaw < -1 ? -1 : newYaw;
        	fishController.setYaw(newYaw);
            fishController.streamFishStateEventController = 6;
            break;
        case BTTN_YAW_RIGHT:
        	newYaw = fishController.newYaw;
			newYaw += 0.5;
			newYaw = newYaw > 1 ? 1 : newYaw;
			fishController.setYaw(newYaw);
            fishController.streamFishStateEventController = 7;
            break;
        case BTTN_FASTER:
        	newThrust = fishController.newThrust;
        	newThrust += 0.2;
        	newThrust = newThrust > 0.8 ? 0.8 : newThrust;
        	fishController.setThrust(newThrust);
            fishController.streamFishStateEventController = 8;
            break;
        case BTTN_SLOWER:
        	newThrust = fishController.newThrust;
			newThrust -= 0.2;
			newThrust = newThrust < 0 ? 0 : newThrust;
			fishController.setThrust(newThrust);
            fishController.streamFishStateEventController = 9;
            break;
        case BTTN_PITCH_UP:
        	newPitch = fishController.newPitch;
        	newPitch += 0.2;
        	newPitch = newPitch > 0.8 ? 0.8 : newPitch;
        	fishController.setPitch(newPitch);
            fishController.streamFishStateEventController = 10;
            break;
        case BTTN_PITCH_DOWN:
        	newPitch = fishController.newPitch;
			newPitch -= 0.2;
			newPitch = newPitch < 0 ? 0 : newPitch;
			fishController.setPitch(newPitch);
            fishController.streamFishStateEventController = 11;
            break;
        case BTTN_SHUTDOWN_PI: // signal a low battery signal to trigger the pi to shutdown
        	fishController.streamFishStateEventController = 12;
            simBatteryLow = new DigitalOut(lowBatteryVoltagePin);
            simBatteryLow->write(0);
            break;
        case BTTN_RESET_MBED:
        	fishController.streamFishStateEventController = 13; // ... if you see this, it didn't happen :)
            mbed_reset();
            break;
        case BTTN_AUTO_MODE:
        	fishController.streamFishStateEventController = 14;
        	fishController.autoMode = !fishController.autoMode;
        	if(fishController.autoMode)
        		fishController.setLEDs(21, true);
        	else
        	{
        		fishController.setLEDs(255, false);
        		// Auto mode was terminated - put fish into a neutral position
        		fishController.setSelectButton(resetSelectButtonValue);
        		fishController.setPitch(resetPitchValue);
        		fishController.setYaw(resetYawValue);
        		fishController.setThrust(resetThrustValue);
        		fishController.setFrequency(resetFrequencyValue, resetPeriodHalfValue);
        	}
        	break;
        default:
        	fishController.streamFishStateEventController = 15;
        	break;
    }
}

//#ifdef debugFishState
//void FishController::printDebugState()
//{
//    printf("\npitch: %f\nyaw: %f\nthrust: %f\nfrequency: %f\nservoLeft: %f\nservoRight: %f\ndutyCycle: %f\n",
//            pitch, yaw, thrust, frequency, servoLeft.read(), servoRight.read(), dutyCycle);
//}
//#endif

void FishController::setLEDs(char mask, bool turnOn)
{
    buttonBoard.setLEDs(mask, turnOn);
}
