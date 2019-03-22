/**
   ----------------------------------------------------------------------------
   This script uses a MFRC522 library; see https://github.com/miguelbalboa/rfid
   for further details.

   NOTE: The library file MFRC522.h has a lot of useful info. Please read it.

   Released into the public domain.
   ----------------------------------------------------------------------------
   Author: Boschert Dominik
*/

#include <SPI.h>
#include <MFRC522.h>

//Define configurable pins
#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);   //Create instance

//Declare variables
MFRC522::MIFARE_Key key;

//Init
void setup(){
  Serial.begin(9600); //Initialize serial communication
  SPI.begin();        //Init SPI
  mfrc522.PCD_Init(); //Init MFRC522

  //Create Key A and B (assuming default keys which are usually "0xFF 0xFF 0xFF 0xFF 0xFF 0xFF"
  for(byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("Save user-ID onto a PICC");
  Serial.print("Access Keys (A and B): ");
  DumpByteArrayAsHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();

  Serial.println("BEWARE: Data will be written to the PICC, in block #1");
}

// Main loop.
void loop(){
  //Wait for new cards
  if(!mfrc522.PICC_IsNewCardPresent())
    return;

  //Try to get the uid of the PICC
  if(!mfrc522.PICC_ReadCardSerial())
    return;

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
    return;
  }
  
  //Set sector, block, trailerblock and value to write
  byte sector = 1;
  byte blockAddr = 4;
  byte dataBlock[16] = {
    0x03
  };
  byte trailerBlock = 7;

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
    return;
  }

  //Print whole Sector
  Serial.println("Current data in sector: ");
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  //Authenticate with B key
  Serial.println("Authenticating again using key B...");
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
  if(status != MFRC522::STATUS_OK) {
    Serial.print("PCD_Authenticate() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Write data
  Serial.print("Writing data to block ");
  Serial.println(blockAddr);
  DumpByteArrayAsChar(dataBlock, 16);
  Serial.println();
  //Check if write was was succesfull
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if(status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_Write() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.println();

  // Read data from the block to double ckeck success of writing
  Serial.print("Reading data from block ");
  Serial.println(blockAddr);
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if(status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_Read() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print("Data in block ");
  Serial.println(blockAddr);
  DumpByteArrayAsChar(buffer, 16);
  Serial.println();
  Serial.println();

  //Check that data was written correctly
  Serial.println("Checking result...");
  byte count = 0;
  for (byte i = 0; i < 16; i++) {
  //Compare buffer (=what we've read) with dataBlock (=what we've written)
    if ((char) buffer[i] == dataBlock[i])
      count++;
  }
  Serial.print("Number of bytes that match = ");
  Serial.print(count);
  if (count == 16) {
    Serial.println(" Success");
  }
  else {
    Serial.println(" No match");
    Serial.println("  perhaps the write didn't work properly...");
  }
  Serial.println();

  //Dump the sector data again
  Serial.println("Current data in sector:");
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  //Stop PICC authentication
  mfrc522.PICC_HaltA();
  //Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
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

/*byte ReadInput(){
  byte charCount = 0;
  char endMarker = '\n';
  byte inputArray[16];
  byte input;

  while(Serial.available() > 0 && charCount < 16){
    input = Serial.read();
    if(input != endMarker){
      inputArray[charCount] = input;
      charCount++;
    }
    else{
      if(charCount >= 16){
        Serial.println("Your input was longer than 16 bytes!! Only the first 16 bytes will be used!");
      }
      else{
        inputArray[charCount] = '\0';
      }
      charCount = 0;
    }
  }
  return inputArray;
}*/
