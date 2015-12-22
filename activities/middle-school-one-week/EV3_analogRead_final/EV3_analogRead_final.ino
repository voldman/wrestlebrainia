#include <Wire.h>

#define SLAVE_ADDRESS 0x04
void setup() 
{
    Serial.begin(9600);         // start serial for output
    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
    Serial.println("Ready!");
}
int pin,st,val=0,flag=0,index=0;
char buf[8];
void loop()
{
  if(flag==1)
  {
    flag=0;
    val=analogRead(pin);
  }
}

void receiveData(int byteCount)
{
    while(Wire.available()>0) 
    {
      pin=Wire.read();
      flag=1;
    }
}

// callback for sending data
void sendData()
{
  delay(100);
  val=rectify(analogRead(0));
  Wire.write(val);
  Serial.println(val);
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
