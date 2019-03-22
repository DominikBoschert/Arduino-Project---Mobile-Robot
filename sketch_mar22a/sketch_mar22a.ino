
byte byteArray[16]{
  0x03, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};
unsigned long number = 0;

void setup() {
  Serial.begin(9600);

}

void loop() {
  DumpByteArrayAsHex(byteArray, 16);
  Serial.println();
  number = (unsigned long) *byteArray;
  Serial.println(number);

}

void DumpByteArrayAsHex(byte *buffer, byte bufferSize) {
  for(byte i = 0; i < bufferSize; i++){
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
