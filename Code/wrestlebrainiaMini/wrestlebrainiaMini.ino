/*
  Wresltebrainia sketch
  Joel Voldman, MIT
  v3.0
  Written July 2014, updated June 2015 for Olimex boards
  Updated July 2015 for Adaptive thresholding, serial LCD

  Uses parts of Adafruit motor shield example files
  
  The LCD circuit:
 * LCD RX pin 1 to digital pin 8
 * LCD GND pin 2 to gnd
 * LCD VDD pin 3 to +5V 
 */

// include libraries for Adafruit motor shield
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"
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

// Define some constants
const int maxRPM = 25;           // maximum stepper RPM 
const int ledPin =  6;           // the number of the pin connected to the LED 
const int EMGArm1Pin = A0;       // Analog input pin to which one EMG board is connected
const int EMGArm2Pin = A1;       // Analog input pin to which the other EMG board is connected
const float readFilterFactor = 0.5;  // Strength of LP filter (0..1). 1 = no filter, lower gives more smoothing
const float limitsFilterFactor = 0.05;  // Strength of LP filter (0..1). 1 = no filter, lower gives more smoothing
const int button1 = 1;       // the number of the pushbutton pin
const int button2 = 2;       // the number of the pushbutton pin
const int EMGArraySize = 50;   // number of EMG readings to store
const int EMGSliceSize = 10; // number of smallest/largest EMG readings to use in setting min/max
const int minFactor = 2;

//Define some variables
int currentEMGArm1,currentEMGArm2;  // current readings
int EMGArm1[EMGArraySize];          // array holding EMG readings
int EMGArm2[EMGArraySize];
int EMGArm1Sorted[EMGArraySize];    // array holding sorted EMG readings
int EMGArm2Sorted[EMGArraySize];
int lastArm1Max = 0;   // hold old values of maxes and mins
int lastArm2Max = 0;
int lastArm1Min = 0;   
int lastArm2Min = 0;
int arm1MaxMin, arm2MaxMin;         // maximal min values allowed
int midPoint;           // number of steps to where arm is vertical
int P1WinButton;        // buttons that determine each winner
int P2WinButton;
int arrayCounter1, arrayCounter2;   // index of array, used to implement queue
boolean start = 1;                  // determines whether a game is starting

// one-time setup function
void setup() {
  pinMode(ledPin, OUTPUT);        // initialize the LEDpin as an output
  pinMode(button1, INPUT);    // initialize the two pushbutton pins as inputs
  pinMode(button2, INPUT);     

  AFMS.begin();  // motor create with the default frequency 1.6KHz
// Serial.begin(57600); // set up hardware serial for debugging

  goToMiddle();
  calibrate();  //run wrestlebrainia calibration procedure
}


// repeat indefinitely
void loop() {
  // For each arm, read the EMG signal and map it to between the min and max signals.  Allow those min and
  // max to drift around slowly to implement adaptive thresholds.  Then scale the signals between min and max,
  // and take the difference between the each arm.  Depending on the sign of the difference, move the 
  // stepper motor in one direction or the other at a speed proportional to the difference.  After the 
  // end of each motion, check to see if a button has been pressed thus indicating a winner.  If so, run 
  // the celebration routine, and then reinitialize.
  
  if (start) {    // startup to tell players when wrestling begins
    lcd.clear();
    lcd.print("Wrestle in..");
    lcd.setCursor(2,1);
    lcd.print("3");
    delay(1000);
    lcd.print("..2");
    delay(1000);
    lcd.print("..1");
    delay(1000);
    lcd.print("..Go!");
    delay(500);
    start = 0;
  }
  
  int currentEMGArm1 = readRectifyAndEnvelope(EMGArm1Pin, currentEMGArm1);  //read each EMG signal
  arrayCounter1 = addToQueue(EMGArm1, currentEMGArm1, EMGArraySize, arrayCounter1); // add to queue
  int currentEMGArm2 = readRectifyAndEnvelope(EMGArm2Pin, currentEMGArm2);  
  arrayCounter2 = addToQueue(EMGArm2, currentEMGArm2, EMGArraySize, arrayCounter2);
  
  for(int i=0;i<EMGArraySize;i++){      // copy to 2nd arrays for sorting
    EMGArm1Sorted[i] = EMGArm1[i];
    EMGArm2Sorted[i] = EMGArm2[i];
  }
  isort(EMGArm1Sorted,EMGArraySize);    // sort arrays
  isort(EMGArm2Sorted,EMGArraySize);

//  Serial.print("Arm1: ");
//  printArray(EMGArm1,EMGArraySize);
//  Serial.print("Arm2: ");
//  printArray(EMGArm2,EMGArraySize);
//  Serial.print("Arm1 sorted: ");
//  printArray(EMGArm1Sorted,EMGArraySize);
//  Serial.print("Arm2 sorted: ");
//  printArray(EMGArm2Sorted,EMGArraySize);
  
  int EMGArm1Min = getMin(EMGArm1Sorted, EMGSliceSize);                 // get local averages 
  int EMGArm1Max = getMax(EMGArm1Sorted, EMGSliceSize, EMGArraySize);    
  int EMGArm2Min = getMin(EMGArm2Sorted, EMGSliceSize);                 
  int EMGArm2Max = getMax(EMGArm2Sorted, EMGSliceSize, EMGArraySize);   
  lastArm1Max = updateMax(currentEMGArm1, EMGArm1Max,lastArm1Max);      // update mins amd maxes
  lastArm2Max = updateMax(currentEMGArm2, EMGArm2Max,lastArm2Max);
  lastArm1Min = updateMin(currentEMGArm1, EMGArm1Min,lastArm1Min,arm1MaxMin);
  lastArm2Min = updateMin(currentEMGArm2, EMGArm2Min,lastArm2Min,arm2MaxMin);  

// Serial.print("ARM1: current: ");
// Serial.print(currentEMGArm1);
// Serial.print("    last Min: ");
// Serial.print(lastArm1Min);
// Serial.print("    Max: ");
// Serial.print(lastArm1Max);
// Serial.println();
// Serial.print("ARM2: current: ");
// Serial.print(currentEMGArm2);
// Serial.print("    last Min: ");
// Serial.print(lastArm2Min);
// Serial.print("    Max: ");
// Serial.print(lastArm2Max);
// Serial.println();  
   
  // Scale values to between 0..99  
  int scaledEMGArm1 = map(currentEMGArm1, lastArm1Min, lastArm1Max, 0, 99);  //scale
  int scaledEMGArm2 = map(currentEMGArm2, lastArm2Min, lastArm2Max, 0, 99); 
  int posscaledEMGArm1 = max(scaledEMGArm1,0);    // bound at zero, don't allow negative power
  int posscaledEMGArm2 = max(scaledEMGArm2,0);
  int raw1 = float(currentEMGArm1)/10.3;
  int min1 = float(lastArm1Min)/10.3;
  int max1 = float(lastArm1Max)/10.3;
  int raw2 = float(currentEMGArm2)/10.3;
  int min2 = float(lastArm2Min)/10.3;
  int max2 = float(lastArm2Max)/10.3;
  
  lcd.clear();
  lcd.print("P1: ");      //Player 1
  lcd.print(raw1);        //Raw signal
  lcd.print("..");
  lcd.print(min1);
  lcd.print("..");
  lcd.print(max1);
  lcd.print("..");
  lcd.print(posscaledEMGArm1);  //Scaled and bounded signal
  lcd.setCursor(2,1);
  lcd.print("P2: ");              //Player 2
  lcd.print(raw2);        
  lcd.print("..");
  lcd.print(min2);
  lcd.print("..");
  lcd.print(max2);
  lcd.print("..");
  lcd.print(posscaledEMGArm2);  //Scaled and bounded signal

// Serial.print("ARM1Scaled: raw: ");
// Serial.print(raw1);
// Serial.print("    min: ");
// Serial.print(min1);
// Serial.print("    max: ");
// Serial.print(max1);
// Serial.print("    scaled: ");
// Serial.print(posscaledEMGArm1);
// Serial.println();
// Serial.print("ARM2Scaled: raw: ");
// Serial.print(raw2);
// Serial.print("    min: ");
// Serial.print(min2);
// Serial.print("    max: ");
// Serial.print(max2);
// Serial.print("    scaled: ");
// Serial.print(posscaledEMGArm2);
// Serial.println();
  
  //compare two EMG signals, then scale to fraction of maxRPM
  int relativeEMG = (posscaledEMGArm1 - posscaledEMGArm2) * maxRPM / 100;    
// Serial.print("Relative: ");
// Serial.print(relativeEMG);
// Serial.println();
  
  if (relativeEMG > 0) {   //if Arm1 is winning, go one way
    myMotor->setSpeed(relativeEMG);
    myMotor->step(1, FORWARD, DOUBLE);
  }
  else if (relativeEMG < 0) { //else if Arm2 is winning, go the other way
    myMotor->setSpeed(abs(relativeEMG));    //relativeEMG is negative, need to make positive
    myMotor->step(1, BACKWARD, DOUBLE);
  }
  else { //if no one is winning
    delay(40);    // just wait a bit so we don't loop too quickly
  }
  
  int result = checkForWinner();    //check to see if a button has been pressed

  if (result != 0) {    //if so, celebrate and then reinitialize
    celebrate(result);
    goToMiddle(); 
    start = 1;          // set start to 1 because we are restarting
  }  


}


//Below are subfunctions used in the setup() and loop() blocks

// Find the extents where the buttons are then the motor goes to the middle to start the game. 
void goToMiddle() {
  int steps = 0;
  
  lcd.clear();
  lcd.print("Finding middle");
  lcd.setCursor(2,1);
  myMotor->setSpeed(maxRPM);

  //Find Player 2's win button
  while((digitalRead(button1) == LOW) && (digitalRead(button2) == LOW)){    //step the motor until we press a button
    myMotor->step(1, BACKWARD, DOUBLE);
    delay(30);
  }
  if (digitalRead(button2)==HIGH){
    P1WinButton = 1;
    P2WinButton = 2;
    lcd.print("P1=1");
    delay(1000);
  } else {
    P1WinButton = 2;
    P2WinButton = 1;    
    lcd.print("P1=2");
    delay(1000);
  }
  
  //Find Player 1's win button
  while(digitalRead(P1WinButton) == LOW) {    //step the motor until the arm presses the other button
    myMotor->step(1, FORWARD, DOUBLE);   
    delay(30); 
    steps++;  //count steps
  }

  //go to the midpoint
  midPoint = steps/2;    //this tells us where the midpoint of the arm travel will be
  myMotor->step(midPoint, BACKWARD, DOUBLE);    
}


// Measure for a while to set initial minimal values
void calibrate() {
  // Start relax calibration
  lcd.clear();
  lcd.print("Arm cal...");
  delay(1000);
  lcd.print("relax");

  for(int i=0; i < EMGArraySize; i++){      // Measure the EMG baseline signal
    currentEMGArm1 = readRectifyAndEnvelope(EMGArm1Pin, currentEMGArm1);      // read
    arrayCounter1 = addToQueue(EMGArm1, currentEMGArm1, EMGArraySize, arrayCounter1);
    lcd.setCursor(2,1);
    lcd.print("A1: ");
    lcd.print(currentEMGArm1);

    currentEMGArm2 = readRectifyAndEnvelope(EMGArm2Pin, currentEMGArm2);      // read
    arrayCounter2 = addToQueue(EMGArm2, currentEMGArm2, EMGArraySize, arrayCounter2);
    lcd.print(" A2: ");
    lcd.print(currentEMGArm2);

//    printArray(EMGArm1,EMGArraySize);
//    printArray(EMGArm2,EMGArraySize);
    delay(50);
  }

  for(int i=0;i<EMGArraySize;i++){      // copy to 2nd array for sorting
    EMGArm1Sorted[i] = EMGArm1[i];
    EMGArm2Sorted[i] = EMGArm2[i];
  }
  isort(EMGArm1Sorted,EMGArraySize);    // sort
  isort(EMGArm2Sorted,EMGArraySize);
  lastArm1Min = getMin(EMGArm1Sorted, EMGSliceSize);      // average of n smallest values
  lastArm2Min = getMin(EMGArm2Sorted, EMGSliceSize);                 
  arm1MaxMin = lastArm1Min * minFactor;                   // scale to set maximal min values
  arm2MaxMin = lastArm2Min * minFactor;  
// printArray(EMGArm1Sorted,EMGArraySize);
// printArray(EMGArm2Sorted,EMGArraySize);
// Serial.print("Arm1 Min: ");
// Serial.print(lastArm1Min);
// Serial.println();
// Serial.print("Arm2 Min: ");
// Serial.print(lastArm2Min);
// Serial.println();

} // End calibration



//Returns the winner of the game.  If Arm1 has won, 
//1 is returned. If Arm2 has won, 2 is returned.  If no one has won the
//game yet, a zero is returned.
int checkForWinner() {
  if (digitalRead(P1WinButton) == HIGH) {  // Check if their win button has been pressed
    lcd.clear();
    lcd.print("Player 1 wins!");
    return 1;
  } else if (digitalRead(P2WinButton) == HIGH) {
    lcd.clear();
    lcd.print("Player 2 wins!");
    return 2;
  }
  return 0;
}  



//Celebration routine when someone wins. The motor first goes to the midpoint, 
// then rotates back and forth a few times while the LED blinks on and off.
void celebrate(int winner) {

  myMotor->setSpeed(maxRPM);
  //Go to the midpoint
  if (winner==2) {    //If player 2 wins, then we need to move away from their button
    myMotor->step(midPoint, FORWARD, DOUBLE);
  }
  else {  //If player 1 wins, we need to move in the other direction
    myMotor->step(midPoint, BACKWARD, DOUBLE);
  }
  
  //Repeat 10x
  for(int i=0; i <10; i++) {
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  myMotor->step(10, BACKWARD, DOUBLE);  //rotate the motor a bit
  delay(200);               // pause
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
  myMotor->step(10, FORWARD, DOUBLE);  //rotate the motor the other way
  delay(200);               // pause
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
   int n = readFilterFactor * r + (1-readFilterFactor) * old; //envelope
   return n;
}   



// Sort array from smallest to largest.  Simple insertion sort, starting with raw code from 
// http://forum.arduino.cc/index.php?topic=49059.0
void isort(int *a, int n) {
 for (int i = 1; i < n; ++i)  {
   int j = a[i];
   int k;
   for (k = i - 1; (k >= 0) && (j < a[k]); k--) {
     a[k + 1] = a[k];
   }
   a[k + 1] = j;
 }
}


// Add last reading to queue array, throw out oldest reading
int addToQueue(int *a, int r, int s, int c) {
  a[c] = r;     // insert current reading r into place in a determined by c
  c++;          // increment c and wrap around if needed
  if (c==s) c=0;
  return c;     // return index c to keep track of where we are
  }



// Return the mean of the s smallest values of sorted array a
int getMin(int a[], int s) {
  int m=0;
  for(int i=0; i<s;i++){
    m += a[i];
  }
  return m / s;
}



// Return the mean of the s largest values of sorted array a of size l
int getMax(int a[], int s, int l) {
  int m=0;
  for(int i=0; i<s;i++){
    m += a[l-1-i];
  }
  return m / s;
}



// Update maximum value.  If our current value c is bigger than the last max value l, then 
// replace l with c.  If not, then we allow the max to drift downward based on the recent 
// average max ra and the time constant determined by filterfactor.
int updateMax(int c, int ra, int l) {
  if (c >= l) { 
    return c;
  } else {
    return limitsFilterFactor * ra + (1-limitsFilterFactor) * l;
  }
}



// Update minimum value.  If our current value c is smaller than the last min value l,
// replace l with c.  If not, then we allow the min to drift upward based on the recent 
// average min ra and the time constant determined by filterfactor. The min cannot go above 
// a value determined by the maxmin i.
int updateMin(int c, int ra, int l, int i) {
  if (c <= l) { 
    return c;
  } else {
    int n = limitsFilterFactor * ra + (1-limitsFilterFactor) * l;
    return min(i,n);
  }
}

// Utility function to print array to serial when debugging.
void printArray(int *a, int n)
{
 for (int i = 0; i < n; i++)
 {
 // Serial.print(a[i], DEC);
 // Serial.print(", ");
 }
// Serial.println();
}




