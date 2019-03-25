char blueToothVal;
char lastValue;

void setup(){
  Serial.begin(9600);
  pinMode(6, OUTPUT);
}

void loop(){
  if(Serial.available()){
    blueToothVal = Serial.read();
    Serial.println(blueToothVal);
  }
  if(blueToothVal == '1'){
    digitalWrite(6,HIGH);
    if(lastValue != '1'){
      Serial.println("LED is on");
    }
    lastValue = blueToothVal;
  }
  else if(blueToothVal == '0'){
    digitalWrite(6,LOW);
    if(lastValue != '0'){
      Serial.println("LED is off");
    }
    lastValue = blueToothVal;
  }
}
