#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Define configurable pins
#define RST_PIN 9
#define SS_PIN 10
#define buzzer 2
#define GSM1 6
#define GSM2 5
#define in1 7
#define in2 8
#define in3 3
#define in4 4

//Variables
MFRC522 mfrc522(SS_PIN, RST_PIN);   //Create instance for RFID
MFRC522::MIFARE_Key key;  //Variable for RFID key
LiquidCrystal_I2C lcd(0x27, 16, 2); //Create instance for Display
const unsigned long interval = 1000;

byte userList[1][16] = {
  {"This is a test"}
};

unsigned long currentMillis;
unsigned long oldMillis;
unsigned long usageTime = 0;
unsigned long sessionTime = 0;

int currentUser = -1; //-1 for no authenticated user - Lock is active
int currentSpeed = 1;

//Init
void setup(){
  Serial.begin(9600); //Initialize serial communication
  SPI.begin();        //Init SPI
  lcd.init();         //Init LCD
  lcd.backlight();    //Turn on Backlight
  mfrc522.PCD_Init(); //Init MFRC522
  UpdateLCD(0, 0, "Sperre aktiv");
  
  pinMode(buzzer, OUTPUT);
  pinMode(GSM1, OUTPUT);
  pinMode(GSM2, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

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
  //if a user is authenticated
  char LCDtext[16];
  if(currentUser == 0 || currentUser == 1){
    //Serial.print(currentUser);
    currentMillis = millis();
    if(currentMillis - oldMillis >= interval){
      CountTime();
      UpdateLCD(0, 0, TimeToString(LCDtext, sessionTime, 0));
      UpdateLCD(0, 1, TimeToString(LCDtext, usageTime, 1));
    }
    currentUser = RFIDCheck(currentUser);
    boolean activeInput = false;
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
        case 'V': BuzzerSignal(150, 1);
          break;
        case 'v': BuzzerSignal(150, 1);
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
  //if there is no authenticated user
  else {
    currentUser = RFIDCheck(currentUser);
    if(currentUser >= 0 && currentUser <= 1){
      Serial.println(usageTime);
      currentMillis = millis();
      oldMillis = currentMillis;
      UpdateLCD(0, 0, TimeToString(LCDtext, sessionTime, 0));
      UpdateLCD(0, 1, TimeToString(LCDtext, usageTime, 1));
    }
  }
}

//Checks if a PICC is in range and tries to ommunicate with it to authenticate a user
int RFIDCheck(int userID){
  //Checks if a new PICC is in range
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

  //Variables
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  //Temporary variable, stores the user ID/index if one is succesfully authenticated, needed because the RFIDCheck
  //can still fail after successful authentication which will then return the origial ID (variable userID)
  int UID = userID;
  //Try to authenticate with the PICC for sector 0
  if(!RFIDAuth(3)){
    return userID;
  }
  //Ty to read data from block 1 (block 1 is set up to contain our user ID) 
  if(!RFIDRead(1,  buffer, size)){
    return userID;
  }
  
  /*Serial.print("Data in block 1");
  DumpByteArrayAsChar(buffer, 16);
  Serial.println();
  Serial.println();
  Serial.print("UID: ");
  Serial.println(UID);
  Serial.print("userID: ");
  Serial.println(userID);*/

  //if there is currently no authenticated user and the lock is active
  if(userID == -1){
  //Check if read data matches a user
    if(!RFIDCheckUserList(buffer, &UID)){
      return userID;
    }
    
    /*Serial.print("NEW UID: ");
    Serial.println(UID);*/
    
    //Try to authenticate with the PICC for sector 1
    if(!RFIDAuth(7)){
      return userID;
    }

    //Try to read data from block 4 (block 4 is set up to contain the saved usage time)
    if(!RFIDRead(4, buffer, size)){
      return userID;
    }

    /*Serial.println("Data in block 4");
    DumpByteArrayAsHex(buffer, 16);
    Serial.println();*/

    usageTime = (unsigned long) *buffer; //convert the read usage time to unsigned long and store it in the global variable
    /*Serial.print("usageTime: ");
    Serial.println(usageTime);
    Serial.println();*/

    BuzzerSignal(100, 2); //Give audible signal that the RFID check is done
  }
  else{
    //Try to authenticate with the PICC for sector 1
    if(!RFIDAuth(7)){
      return userID;
    }

    //Try to write into block 4
    if(!RFIDWrite(4)){
      return userID;
    }
    UID = -1; //User deauthenticated
    Stop(); //stop both motors
    //Update LCD with the text signaling that the lock is active
    UpdateLCD(0,1,"");
    UpdateLCD(0,0, "Sperre aktiv");
    sessionTime = 0; //Reset time of current session
    BuzzerSignal(100, 2); //Give audible signal that the RFID check is done
  }
  RFIDStopConnection();
  return UID;
}

boolean RFIDCheckPICCType(){
  //Show some details of the PICC
  /*Serial.print("Card UID: ");
  DumpByteArrayAsHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print("PICC type: ");*/
  //Get PICC type and check if it's compatible with our PCD
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
  //Try to authenticate with the "A" key
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

boolean RFIDRead(byte block, byte *bufferAdr, byte size){
  //Try to read the data from the block
  /*Serial.print("Reading data from block ");
  Serial.println(block);*/
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
  //Try to write usageTime to PICC
  MFRC522::StatusCode status;
  byte dataBlock[16] = {usageTime};
  /*Serial.print("Writing data to block ");
  Serial.println(block);
  DumpByteArrayAsHex(dataBlock, 16);
  Serial.println();
  DumpByteArrayAsChar(dataBlock, 16);
  Serial.println();*/
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(block,dataBlock, 16);
  if(status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_Write() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  Serial.println();
  return true;
}

boolean RFIDCheckUserList(byte bufferCpy[18], int *UIDAdr){
  //Check the given byte array against the byte arrays in userList until a match is found or there are no new byte arrays left in userList
  byte count = 0;
  for(byte user = 0; user < 2; user++){
    /*Serial.print("Checking against user ");
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
    Serial.println();*/
    for (byte i = 0; i < 16; i++) {
      if (bufferCpy[i] == userList[user][i]){
        count++;
      }
    }/*
    Serial.print("Number of bytes that match = ");
    Serial.print(count);*/
    if (count == 16) {
      Serial.println(" Success");
      *UIDAdr = user;
      return true;
    }
    Serial.println();
    count = 0; //Reset count of matching bytes after each byte array
  }
  Serial.println("No match");
  return false;
}

void RFIDStopConnection(){
  //Instruct PICC to go from State "Active" to state "Halt"
  mfrc522.PICC_HaltA();
  //Leave "authenticated" state on the PCD, if this is not done there cannot be another connection
  mfrc522.PCD_StopCrypto1();
}

void CountTime(){
  //increment the variables for the timers and update currentMillis to start a new time measurement
  sessionTime++;
  usageTime++;
  oldMillis = currentMillis;
  /*Serial.print("sessionTime: ");
  Serial.println(sessionTime);
  Serial.print("usageTime: ");
  Serial.println(usageTime);*/
}

//Helper function to dump a byte array as char values to Serial.
void DumpByteArrayAsChar(byte *buffer, byte bufferSize) {
  for(byte i = 0; i < bufferSize; i++){
    Serial.print((char) buffer[i]);
  }
}


//Helper function to dump a byte array as char values to Serial.
void DumpByteArrayAsHex(byte *buffer, byte bufferSize) {
  for(byte i = 0; i < bufferSize; i++){
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void UpdateLCD(int pos, int line, char * text){
  //Updates the LCD at the given position, line and with the given text out of the char array
  lcd.setCursor(pos, line);
  lcd.print(text);
  /*Serial.print("Text: ");
  Serial.println(text);
  Serial.print("Sizeof(text): ");
  Serial.println(strlen(text));*/
  //If the char array has less than 16 characters, fill the following position with spaces to overwrite leftover characters from the old text
  for(int i = strlen(text); i < 16; i++){
    lcd.write(' ');
  }
}

char * TimeToString(char *textVar, unsigned long t, int type){
  /*Serial.print("Type: ");
  Serial.print(type);
  Serial.print(" - Time: ");
  Serial.println(t);*/
  /*Format the given time into the given type of output 
    type 0 = hhhh:mm:ss
    type 1 = dd Tage hh:mm:ss*/
  if(type == 0){
    int h = t / 3600;
    t = t % 3600;
    int m = t/60;
    int s = t % 60;
    sprintf(textVar, "%04d:%02d:%02d", h, m, s);
  }
  else if(type == 1){
    int d = t / 86400;
    t = t % 86400;
    int h = t / 3600;
    t = t % 3600;
    int m = t/60;
    int s = t % 60;
    sprintf(textVar, "%02d Tage %02d:%02d:%02d", d, h, m, s);
  }
  
  delay(20); //Delay to ensure the LCD has finished writing. LCD output will be bugged if this isn't done. 
  return textVar; 
}

byte GetBlueToothInput(){
  //Read the input given by the BT-module 
  byte blueToothValue;
  blueToothValue = Serial.read();
  return blueToothValue;
}

void BuzzerSignal(int duration, int customDelay){
  //Create Frequency for Buzzer to give an audible signal
  for(int i = 0; i < duration; i++){
    digitalWrite(buzzer,HIGH);
    delay(customDelay);
    digitalWrite(buzzer,LOW);
    delay(customDelay);
  }
}

//These functions control the H-bridge and motors via switching different pins between high and low
void Speed(){
  //Set the speed of the motors via 8 bit number (NOTE: motors seem to be too weak to spin the wheels below 75
  Serial.println("Speeeeeeeeeeed!!!");
  Serial.println(currentSpeed);
  double level = 200 *((double) currentSpeed / 10) + 50;
  Serial.println(level);
  analogWrite(GSM1, (int) level);
  analogWrite(GSM2, (int) level);
}

void Stop(){
  //Stop motors
  Serial.println("STOP!!");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void Forward(){
  Serial.println("FORWARD!!");
  Speed();
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void Backward(){
  Serial.println("BACKWARD!!");
  Speed();
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void Left(){
  Serial.println("LEFT!!");
  Speed();
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void Right(){
  Serial.println("RIGHT!!");
  Speed();
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void ForwardLeft(){
  //Put one motor on 50% of the speed of the other to be able to turn while keeping forward acceleration
  Serial.println("FORWARD-LEFT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM1, (int) level);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void ForwardRight(){
  //Put one motor on 50% of the speed of the other to turn while keeping forward acceleration
  Serial.println("FORWARD-RIGHT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM2, (int) level);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void BackwardLeft(){
  //Put one motor on 50% of the speed of the other to turn while keeping backward acceleration
  Serial.println("BACKWARD-LEFT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM1, (int) level);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void BackwardRight(){
  //Put one motor on 50% of the speed of the other to turn while keeping backward acceleration
  Serial.println("BACKWARD-RIGHT!!");
  double level = 255 *((double) currentSpeed / 10) / 2;
  analogWrite(GSM2, (int) level);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}
