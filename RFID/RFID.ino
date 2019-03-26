#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Define configurable pins
#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);   //Create instance
LiquidCrystal_I2C lcd(0x27, 16, 2); //Define Display

//Variables
MFRC522::MIFARE_Key key;
int currentUser;
byte userList[1][16] = {
  {"This is a test"}
};
unsigned long currentMillis;
unsigned long oldMillis;
unsigned long usageTime = 0;
unsigned long sessionTime = 0;
const unsigned long interval = 1000;
int buzzer = 2;
int GSM1 = 6;
int in1 = 7;
int in2 = 8;
int GSM2 = 5;
int in3 = 3;
int in4 = 4;

//Init
void setup(){
  Serial.begin(9600); //Initialize serial communication
  SPI.begin();        //Init SPI
  lcd.init();         //Init LCD
  lcd.backlight();    //Turn on Backlight
  UpdateLCD(0, 0, "Sperre aktiv");
  mfrc522.PCD_Init(); //Init MFRC522
  pinMode(buzzer, OUTPUT);
  pinMode(GSM1, OUTPUT);
  pinMode(GSM2, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  currentUser = -1;

  //Create Key A and B (assuming default keys which are usually "0xFF 0xFF 0xFF 0xFF 0xFF 0xFF"
  for(byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.print("Access Keys (A and B): ");
  DumpByteArrayAsHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();
}

void loop() {
  if(currentUser == 0 || currentUser == 1){
    //Serial.print(currentUser);
    currentMillis = millis();
    if(currentMillis - oldMillis >= interval){
      //Serial.println("DEBUG!!!!");
      CountTime();
      UpdateLCD(0, 0, TimeToString(sessionTime, 0));
      UpdateLCD(0, 1, TimeToString(usageTime, 1));
    }
    currentUser = RFIDCheck(4, currentUser);
    if(Serial.available()){
      //Serial.println((char) GetBlueToothInput());
      switch((char) GetBlueToothInput()){
        case 'F': Forward();
          break;
        case 'B': Backward();
          break;
        case 'L': Left();
          break;
        case 'R': Right();
          break;
        case 'S': Stop();
          break;
        case 'V': Horn();
          break;
        case 'v': Horn();
          break;
        case '1': Speed(1);
          break;
        case '2': Speed(2);
          break;
        case '3': Speed(3);
          break;
        case '4': Speed(4);
          break;
        case '5': Speed(5);
          break;
        case '6': Speed(6);
          break;
        case '7': Speed(7);
          break;
        case '8': Speed(8);
          break;
        case '9': Speed(9);
          break;
        case '0': Speed(10);
          break;        
      }
      /*if((char) GetBlueToothInput() == 'V'){
        Horn();
      }*/
    }
  }
  else {
    currentUser = RFIDCheck(1, currentUser);
    if(currentUser >= 0 && currentUser <= 1){
      Serial.println(usageTime);
      currentMillis = millis();
      oldMillis = currentMillis;
      UpdateLCD(0, 0, TimeToString(sessionTime, 0));
      //usageTime = 86395;
      UpdateLCD(0, 1, TimeToString(usageTime, 1));
    }
  }
}

long RFIDCheck(byte block, long userID){
  long UID = userID;
  if(!mfrc522.PICC_IsNewCardPresent()){
    return userID;
  }

  //Try to get the uid of the PICC
  if(!mfrc522.PICC_ReadCardSerial()){
    return userID;
  }

  // Show some details of the PICC
  Serial.print("Card UID: ");
  DumpByteArrayAsHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print("PICC type: ");
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  //Check for compatibility
  if(piccType != MFRC522::PICC_TYPE_MIFARE_MINI
                &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
                &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K){
    Serial.println("This script only works with MIFARE Classic.");
    return UID;
  }

  //Set sector, block, trailerblock and value to write
  byte blockAddr = 1;
  byte sector = block/4;
  //byte dataBlock[16] = "This is a test";
  byte trailerBlock = (block/4)*4+3;

  //Declare status and buffer, declare and initialize size variables
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  //Authenticate with A key
  Serial.println("Authenticating using key A...");
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if(status != MFRC522::STATUS_OK){
    Serial.print("PCD_Authenticate() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return UID;
  }

  //Print whole Sector
  Serial.println("Current data in sector: ");
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  // Read data from the block to double ckeck success of writing
  Serial.print("Reading data from block ");
  Serial.println(block);
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(block, buffer, &size);
  if(status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_Read() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print("Data in block ");
  Serial.println(block);
  DumpByteArrayAsChar(buffer, 16);
  Serial.println();
  Serial.println();

  if(UID == -1){
  //Check that data was written correctly
    byte count = 0;
    for(byte user = 0; user < 2; user++){
      Serial.print("Checking against user ");
      Serial.println(user);
      DumpByteArrayAsChar(userList[user], 16);
      Serial.println();
      DumpByteArrayAsHex(userList[user], 16);
      Serial.println();
      Serial.println("buffer:");
      DumpByteArrayAsChar(buffer, 16);
      Serial.println();
      DumpByteArrayAsHex(buffer, 16);
      Serial.println();
      Serial.println();
      for (byte i = 0; i < 16; i++) {
        //Compare buffer (=what we've read) with dataBlock (=what we've written)
        if (buffer[i] == userList[user][i]){
          count++;
        }
      }
  
      Serial.print("Number of bytes that match = ");
      Serial.print(count);
      if (count == 16) {
        Serial.println(" Success");
        UID = user;
        
        //Authenticate with A key
        Serial.println("Authenticating using key A...");
        status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 7, &key, &(mfrc522.uid));
        if(status != MFRC522::STATUS_OK){
          Serial.print("PCD_Authenticate() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return -1;
        }

        //Print whole Sector
        Serial.println("Current data in sector: ");
        mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, 1);
        Serial.println();

        // Read data from the block to get saved usageTime
        Serial.println("Reading data from block 4");
        status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(4, buffer, &size);
        if(status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_Read() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
        }
        Serial.println("Data in block 4");
        DumpByteArrayAsHex(buffer, 16);
        Serial.println();
        usageTime = (unsigned long) *buffer;
        Serial.print("usageTime: ");
        Serial.println(usageTime);
        Serial.println();
        for(int i = 0; i < 100; i++){
          digitalWrite(buzzer,HIGH);
          delay(2);
          digitalWrite(buzzer,LOW);
          delay(2);
        }
        break;
      }
      else {
        Serial.println(" No match");
      }
      Serial.println();
      count = 0;
    }
  }
  else{
    //Write usageTime to PICC
    byte dataBlock[16] = {usageTime};
    Serial.print("Writing data to block ");
    Serial.println(block);
    DumpByteArrayAsHex(dataBlock, 16);
    Serial.println();
    DumpByteArrayAsChar(dataBlock, 16);
    Serial.println();
    //Check if write was was succesfull
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(block,dataBlock, 16);
    if(status != MFRC522::STATUS_OK) {
      Serial.print("MIFARE_Write() failed: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
    UID = -1;
    StopAll();
    UpdateLCD(0,1,"");
    UpdateLCD(0,0, "Sperre aktiv");
    sessionTime = 0;
    for(int i = 0; i < 80; i++){
      digitalWrite(buzzer,HIGH);
      delay(1);
      digitalWrite(buzzer,LOW);
      delay(1);
    }
  }
  //Stop PICC authentication
  mfrc522.PICC_HaltA();
  //Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  return UID;
}

void CountTime(){
  sessionTime++;
  usageTime++;
  oldMillis = currentMillis;
  /*Serial.print("sessionTime: ");
  Serial.println(sessionTime);
  Serial.print("usageTime: ");
  Serial.println(usageTime);*/
}

//Helper routine to dump a byte array as hex values to Serial.
void DumpByteArrayAsChar(byte *buffer, byte bufferSize) {
  for(byte i = 0; i < bufferSize; i++){
    Serial.print((char) buffer[i]);
  }
}

void DumpByteArrayAsHex(byte *buffer, byte bufferSize) {
  for(byte i = 0; i < bufferSize; i++){
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void UpdateLCD(int pos, int line, char * text){
  lcd.setCursor(pos, line);
  lcd.print(text);
  /*Serial.print("Text: ");
  Serial.println(text);
  Serial.print("Sizeof(text): ");
  Serial.println(strlen(text));*/
  for(int i = strlen(text); i < 16; i++){
    lcd.write(' ');
  }
}

char * TimeToString(unsigned long t, int type){
  /*Serial.print("Type: ");
  Serial.print(type);
  Serial.print(" - Time: ");
  Serial.println(t);*/
  static char str[16];
  if(type == 0){
    int h = t / 3600;
    t = t % 3600;
    int m = t/60;
    int s = t % 60;
    sprintf(str, "%04d:%02d:%02d", h, m, s);
  }
  else if(type == 1){
    int d = t / 86400;
    t = t % 86400;
    int h = t / 3600;
    t = t % 3600;
    int m = t/60;
    int s = t % 60;
    sprintf(str, "%02d Tage %02d:%02d:%02d", d, h, m, s);
  }
  //sprintf(str, "%02d:%02d:%02d:%02d", d, h, m, s);
  //sprintf(str, "%04d:%02d:%02d", h, m, s);
  delay(20);
  return str; 
}

byte GetBlueToothInput(){
  byte blueToothValue;
  blueToothValue = Serial.read();
  return blueToothValue;
}

void Horn(){
  for(int i = 0; i < 150; i++){
      digitalWrite(buzzer,HIGH);
      delay(1);
      digitalWrite(buzzer,LOW);
      delay(1);
    }
}
void Forward(){
  //h-bridge forward
  Serial.println("FORWARD!!");
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void Backward(){
  //h-bridge backwards
  Serial.println("BACKWARD!!");
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void Left(){
  //h-bridge left
  Serial.println("LEFT!!");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void Right(){
  //h-bridge right
  Serial.println("RIGHT!!");
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void ForwardLeft(){
  //h-bridge forward-left
}

void ForwardRight(){
  //h-bridge forward-right
}

void BackwardLeft(){
  //h-bridge backward-left
}

void BackwardRight(){
  //h-bridge backward-right
}

void Stop(){
  //h-bridge stop
  Serial.println("STOP!!");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void Speed(double level){
  //h-bridge speed
  Serial.println("Speeeeeeeeeeed!!!");
  level = 255 *(level / 10);
  Serial.println(level);
  analogWrite(GSM1, (int) level);
  analogWrite(GSM2, (int) level);
}

void StopAll(){
  //Stop All
  Serial.println("STOP ALL!!");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}
