/*
  Sketch to control motor rotation rate using EMG sensor
  Joel Voldman, MIT
  v3.1
  Written July 2014, updated June 2015 for Olimex boards
  Updated July 2015 for serial LCD

  Uses parts of Adafruit motor shield example files
  
  The LCD circuit:
 * LCD RX pin 1 to digital pin 8
 * LCD GND pin 2 to gnd
 * LCD VDD pin 3 to +5V 
 */

 
// include libraries for Adafruit motor shield
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS)PWMServoDriver.h"
// include the library code to run the LCD display:
#include <serLCD.h>
#include <SoftwareSerial.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #1 (M1 and M2)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1);

// initialize the library with the number of the interface pin
serLCD lcd(8);

//Define some constants
const int maxRPM = 250;    // maximum stepper RPM 
const int ledPin = 6;      // select the pin for the LED
const int EMGPin = A0;     // Analog input pin to which the EMG board is connected
const int calibrationCount = 10;  // Number of data points to measure for calibration
const float filterFactor = 0.2; // holds the amount of smoothing (0..1). 1 is no smoothing.

//Define some variables
int noiseThreshold;   // define noise threshold variable
int currentEMG = 0;   // current reading




// one-time setup function
void setup() {
  // declare the ledPin as an OUTPUT:  
  pinMode(ledPin, OUTPUT);  
analogReference(EXTERNAL);
  AFMS.begin();  // create motor shield with the default frequency 1.6KHz

  //Here we will rotate the motor a bit, then measure the EMG signal, 
  //then repeat (10x total). We will use those values to set our noise threshold
  myMotor->setSpeed(50);  // set motor speed (in RPM)  
  for(int i=0; i < calibrationCount; i++){      //Measure the EMG noise
    myMotor->step(50, FORWARD, INTERLEAVE);  //take 50 steps FORWARD using INTERLEAVE 
    delay(200);
    currentEMG = readRectifyAndEnvelope(EMGPin, currentEMG);      // read
    
    lcd.clear();
    // Print a message to the LCD.
    lcd.print("Calibration");
    lcd.setCursor(2, 1);
    lcd.print("Relax : ");
    lcd.setCursor(2, 10);
    lcd.print(currentEMG);
    noiseThreshold += currentEMG;
  }
  noiseThreshold = noiseThreshold / calibrationCount; //set threshold 
  lcd.clear();
  lcd.print("Calibration val");
  lcd.setCursor(2,1);
  lcd.print(noiseThreshold);
  delay(1000);
}


// repeat indefinitely
void loop() {
  //Read the EMG signal and subtract off noise, then make sure the signal is non-negative
  //Then scale the signal to between 0 and maxRPM, and, if the signal is non-zero, move the 
  //stepper a few steps forward at that power (speed)

  currentEMG = readRectifyAndEnvelope(EMGPin, currentEMG);      // read
  int scaledEMG = map(currentEMG, noiseThreshold, 1023, 0, maxRPM) ;  //scale

  //Update display
  lcd.clear();
  lcd.print("EMG signal: ");
  lcd.setCursor(2,1);
  lcd.print(currentEMG);

  if (scaledEMG > 0)  //If we have a value above threshold, rotate the motor
    {
      myMotor->setSpeed(scaledEMG);
      myMotor->step(6, FORWARD, INTERLEAVE);
    }
  else
    {    
      delay(40);  // Else wait a bit so we don't loop too quickly
    }
}



// Read, rectify, and envelope (smooth) the signal.  
// Combine these operations into one function since they are always used together.
// To rectify, we take any values <512 and flip them so they are >512, so that values 
// range from 512..1023. Then shift signals so they go from 0..511, and then scale by 2x 
// so they span the full scale of 0..1023.
// To envelope, we use first-order recursive digital filter
int readRectifyAndEnvelope(int in, int old) {
  int r = analogRead(in);   // read

  if (r < 512){     //rectify
     r = 1023 - r;
   }
   r = (r - 512 ) << 1; //scale 2x
   int n = filterFactor * r + (1-filterFactor) * old; //envelope
   return n;
}  









