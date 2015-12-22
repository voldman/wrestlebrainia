// Simple sketch to control the brightness of an LED using a potentiometer
// Joel Voldman, MIT
// Written July 2014

//Define some constants
const int ledPin = 6;        // select the pin for the LED
const int potPin = A0;       // select the input pin for the potentiometer
const int divisor = 1;       // divider between analog input and pot control

//Define some variables
int potValue=0;      // variable to store the value coming from the pot

void setup() {
  // declare the ledPin as an OUTPUT:  
  pinMode(ledPin, OUTPUT);  

}

void loop() {
  // read the value from the potentiometer (range of 0..1023)
  potValue = analogRead(potPin);    
  
  // use that value to set the brightness of the LED
  analogWrite(ledPin, potValue / divisor);    

  delay(50);      // Wait 50 ms
}
