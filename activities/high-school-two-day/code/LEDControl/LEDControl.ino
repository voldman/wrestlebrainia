// Simple sketch to control the brightness of an LED
// Joel Voldman, MIT
// Written July 2014

//Define some constants
const int ledPin = 6;        // select the pin for the LED

void setup() {
  // declare the ledPin as an OUTPUT:  
  pinMode(ledPin, OUTPUT);  

}

void loop() {
  // set the brightness of the LED:
  analogWrite(ledPin, 255);    

  delay(50);      // Wait 50 ms
}

