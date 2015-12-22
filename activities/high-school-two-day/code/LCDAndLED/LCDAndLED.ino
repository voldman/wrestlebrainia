/*
  Simple sketch to control the brightness of an LED using a potentiometer
  Joel Voldman, MIT
  Written July 2014, updated June 2015 for Olimex boards and serial LCD

   
  The LCD circuit:
 * LCD RX pin 1 to digital pin 8
 * LCD GND pin 2 to gnd
 * LCD VDD pin 3 to +5V 
 */


// include the library code to run the LCD display:
#include <serLCD.h>
#include <SoftwareSerial.h>

// initialize the library with the number of the interface pin
serLCD lcd(8);

//Define some constants
const int ledPin = 6;         // select the pin for the LED
const int potPin = A0;        // select the input pin for the potentiometer
const float filterFactor = 0.2; // holds the amount of smoothing (0..1). 1 is no smoothing.

//Define some variables
int potValue = 0;        // variable to store the value coming from the pot
int lastValue = 0;       // variable to store the last value from the pot

void setup() {
  // declare the ledPin as an OUTPUT:  
  pinMode(ledPin, OUTPUT);  

}


void loop() {
  // read the value from the potentiometer (range of 0..1023)
  potValue = analogRead(potPin);   

// When reading from the EMG, we need to rectify and smooth the signal.  
// Comment the preceding line 39 and uncomment the following line to do so:
//  potValue = readRectifyAndEnvelope(potPin, potValue);     
   
  // use that value to set the brightness of the LED
  analogWrite(ledPin, potValue / 4);    

  lcd.clear();
  // Print a message to the LCD.
  lcd.print("Value: ");
  // set the cursor to row 2, column 1
  lcd.setCursor(2, 1);
  // print the number of seconds since reset:
  lcd.print(potValue);
  delay(50);
}


// Rectify and scale signal.  Takes any values <512 and flips them so they are >512, so that values range from 512..1023.
// Then it shifts signals so they go from 0..511, and then scales by 2x so they span the full scale of 0..1023.
int rectify(int in) {
   if (in < 512){
     in = 1023 - in;
   }
   in = (in - 512 ) << 1;
   return in;
}   

int readRectifyAndEnvelope(int in, int old) {
  int r = analogRead(in);   // read

  if (r < 512){     //rectify
     r = 1023 - r;
   }
   r = (r - 512 ) << 1; //scale 2x
   int n = filterFactor * r + (1-filterFactor) * old; //envelope
   return n;
}  


