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
int currentSpeed = 1;

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
  Speed();
}

void loop() {
  if(currentUser == 0 || currentUser == 1){
    //Serial.print(currentUser);
    currentMillis = millis();
    if(currentMillis - oldMillis >= interval){
      //Serial.println("DEBUG!!!!");
      CountTime();
      UpdateLCD(0, 0, TimeToString(sessionTime, 0));
     //delay(20);
      UpdateLCD(0, 1, TimeToString(usageTime, 1));
     // delay(20);
    }
    currentUser = RFIDCheck(currentUser);
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
        case 'G': ForwardLeft();
          break;
        case 'I': ForwardRight();
          break;
        case 'H': BackwardLeft();
          break;
        case 'J': BackwardRight();
          break;
        case 'S': Stop();
          break;
        case 'V': Horn();
          break;
        case 'v': Horn();
          break;
        case '1': currentSpeed = 3;
          Speed();
          break;
        case '2': currentSpeed = 3;
          Speed();
          break;
        case '3': currentSpeed = 3;
          Speed();
          break;
        case '4': currentSpeed = 4;
          Speed();
          break;
        case '5': currentSpeed = 5;
          Speed();
          break;
        case '6': currentSpeed = 6;
        Speed();
          break;
        case '7': currentSpeed = 7;
          Speed();
          break;
        case '8': currentSpeed = 8;
          Speed();
          break;
        case '9': currentSpeed = 9;
          Speed();
          break;
        case '0': currentSpeed = 10;
          Speed();
          break;        
      }
    }
  }
  else {
    currentUser = RFIDCheck(currentUser);
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

long RFIDCheck(long userID){
  long UID = userID;
  if(!mfrc522.PICC_IsNewCardPresent()){
    return userID;
  }

  //Try to get the uid of the PICC
  if(!mfrc522.PICC_ReadCardSerial()){
    return userID;
  }
  
  //Check for compatibility
  if(!RFIDCheckPICCType()){
    return userID;
  }

  //Declare status and buffer, declare and initialize size variables
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  //Authenticate with the PICC
  if(!RFIDAuth(3)){
    return userID;
  }

  RFIDDumpSector(0, &buffer);

  // Read data from the block 
  if(!RFIDRead(1,  &buffer, size)){
    return userID;
  }

  
  Serial.print("Data in block 1");
  DumpByteArrayAsChar(buffer, 16);
  Serial.println();
  Serial.println();
  Serial.print("UID: ");
  Serial.println(UID);
  Serial.print("userID: ");
  Serial.println(userID);

  if(UID == -1){
  //Check if read data matches a user
    if(!RFIDCheckUserList(buffer, &UID)){
      return userID;
    }
    
    Serial.print("NEW UID: ");
    Serial.println(UID);
    
    //Authenticate with the PICC
    if(!RFIDAuth(7)){
      return userID;
    }
    
    RFIDDumpSector(1, &buffer);

    // Read data from the block 
    if(!RFIDRead(4, &buffer, size)){
      return userID;
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
    //break;
  }
  else{
    //Authenticate with the PICC
    if(!RFIDAuth(7)){
      return userID;
    }
    
    if(!RFIDWrite(4)){
      return userID;
    }
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
  RFIDStopConnection();
  return UID;
}

boolean RFIDCheckPICCType(){
  // Show some details of the PICC
  Serial.print("Card UID: ");
  DumpByteArrayAsHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print("PICC type: ");
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  if(piccType != MFRC522::PICC_TYPE_MIFARE_MINI
                &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
                &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K){
    Serial.println("This script only works with MIFARE Classic.");
    return false;
  }
  return true;
}

boolean RFIDAuth(byte trailerBlock){
  //Authenticate with A key
  MFRC522::StatusCode status;
  Serial.println("Authenticating using key A...");
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if(status != MFRC522::STATUS_OK){
    Serial.print("PCD_Authenticate() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}

boolean RFIDDumpSector(byte sector, void *bufferAdr){
  //Print whole Sector
  byte buffer[18];
  Serial.println("Current data in sector: ");
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();
  bufferAdr = buffer;
}

boolean RFIDRead(byte block, void *bufferAdr, byte size){
  Serial.print("Reading data from block ");
  Serial.println(block);
  MFRC522::StatusCode status;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(block, bufferAdr, &size);
  if(status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_Read() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }    
  return true;
}

boolean RFIDWrite(byte block){
  //Write usageTime to PICC
  byte dataBlock[16] = {usageTime};
  MFRC522::StatusCode status;
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
    return false;
  }
  Serial.println();
  return true;
}

boolean RFIDCheckUserList(byte bufferCpy[18], long *UIDAdr){
  byte count = 0;
  for(byte user = 0; user < 2; user++){
    Serial.print("Checking against user ");
    Serial.println(user);
    DumpByteArrayAsChar(userList[user], 16);
    Serial.println();
    DumpByteArrayAsHex(userList[user], 16);
    Serial.println();
    Serial.println("buffer:");
    DumpByteArrayAsChar(bufferCpy, 16);
    Serial.println();
    DumpByteArrayAsHex(bufferCpy, 16);
    Serial.println();
    Serial.println();
    for (byte i = 0; i < 16; i++) {
      //Compare buffer (=what we've read) with dataBlock (=what we've written)
      if (bufferCpy[i] == userList[user][i]){
        count++;
      }
    }
    Serial.print("Number of bytes that match = ");
    Serial.print(count);
    if (count == 16) {
      Serial.println(" Success");
      *UIDAdr = user;
      return true;
    }
    Serial.println();
    count = 0;
  }
  Serial.println("No match");
  return false;
}

void RFIDStopConnection(){
  //Stop PICC authentication
  mfrc522.PICC_HaltA();
  //Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
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
  Speed();
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void Backward(){
  //h-bridge backwards
  Serial.println("BACKWARD!!");
  Speed();
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void Left(){
  //h-bridge left
  Serial.println("LEFT!!");
  Speed();
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void Right(){
  //h-bridge right
  Serial.println("RIGHT!!");
  Speed();
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void ForwardLeft(){
  //h-bridge forward-left
  Serial.println("FORWARD-LEFT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM1, (int) level);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void ForwardRight(){
  //h-bridge forward-right
  Serial.println("FORWARD-RIGHT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM2, (int) level);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void BackwardLeft(){
  //h-bridge backward-left
  Serial.println("BACKWARD-LEFT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM1, (int) level);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void BackwardRight(){
  //h-bridge backward-right
  Serial.println("BACKWARD-RIGHT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM2, (int) level);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void Stop(){
  //h-bridge stop
  Serial.println("STOP!!");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void Speed(){
  //h-bridge speed
  Serial.println("Speeeeeeeeeeed!!!");
  Serial.println(currentSpeed);
  double level = 255 *((double) currentSpeed / 10);
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
