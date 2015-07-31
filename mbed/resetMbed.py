import RPi.GPIO as GPIO
import time

resetPin = 3 # BCM pin 3, actually pin 5

def reset():
    # set up the pin as output
    # will generate a warning about pull-up resistor but that's ok
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(resetPin, GPIO.OUT) # ignore the warning

    # reset the mbed
    GPIO.output(resetPin, GPIO.LOW)
    time.sleep(0.5)
    GPIO.output(resetPin, GPIO.HIGH)

    # clean up
    GPIO.cleanup()

if __name__ == '__main__':
    reset()
