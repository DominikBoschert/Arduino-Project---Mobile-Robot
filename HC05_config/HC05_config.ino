#include <SoftwareSerial.h>

#define D6 6
#define D7 7

#define bt_power 11
#define bt_key_power 12

#define BAUD 9600
#define BTBAUD 38400

SoftwareSerial btSerial(D7,D6);
char myChar;

void setup() {
  // put your setup code here, to run once:
pinMode(bt_power, OUTPUT);
pinMode(bt_key_power, OUTPUT);

digitalWrite(bt_power, LOW);
digitalWrite(bt_key_power, LOW);

delay(100);

digitalWrite(bt_key_power, HIGH);

delay(100);

digitalWrite(bt_power, HIGH);

Serial.begin(9600);

btSerial.begin(38400);

delay(1000);

Serial.println("Type AT commands");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(btSerial.available()){
    myChar = btSerial.read();
    Serial.print(myChar);
  }

  if(Serial.available()){
    myChar = Serial.read();
    Serial.print(myChar);
    btSerial.print(myChar);
  }
}
