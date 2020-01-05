#include <SoftwareSerial.h>

SoftwareSerial serial2(2, 3);
byte* inBuffer = new byte[1000];

int led_duration = 0;

void setup()
{
  Serial.begin(9600);
  serial2.begin(9600);
  serial2.listen();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop()
{
  int count;

  // LED control
  if (led_duration > 0)
  {
    led_duration--;
    digitalWrite(13, HIGH);
  }
  else
    digitalWrite(13, LOW);
  
  // forwarding (2,3) to (0,1)
  count = serial2.available();
  if (count > 0)
  {
    count = serial2.readBytes(inBuffer, count);
    Serial.write(inBuffer, count);
    led_duration = 30;
  }
  delay(1);
  
  // forwarding (0,1) to (2,3)
  count = Serial.available();
  if (count > 0)
  {
    count = Serial.readBytes(inBuffer, count);
    serial2.write(inBuffer, count);
    led_duration = 30;
  }
  delay(1);
}
