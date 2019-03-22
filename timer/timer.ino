unsigned long currentMillis;
unsigned long oldMillis;
unsigned long usageTime =0;
const unsigned long interval = 1000;

void setup() {
 Serial.begin(9600);
 oldMillis = millis();
}

void loop() {
  currentMillis = millis();
  if(currentMillis - oldMillis >= interval){
    usageTime++;
    oldMillis = currentMillis;
    Serial.println(usageTime);
  }
  
}
