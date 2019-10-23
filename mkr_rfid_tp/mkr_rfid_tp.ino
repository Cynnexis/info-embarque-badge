/*
 * Authors:
 * Valentin BERGER
 * Léa CHEMOUL
 * 
 * Hardware required:
 * Arduino MKR WAN 1300
 * PCD (Proximity Coupling Device): NXP MFRC522
 * PICC (Proximity Integrated Circuit Card): A card using the ISO 14443A interface, eg Mifare or NTAG203.
 */

#define RST_PIN         6 //9
#define SS_PIN          7 //10

#define MIN_TIMEOUT_MS   3000
#define MAX_NB_WHITELISTED_IDS  10
#define BYTE_ID_SIZE 4
#define RECEIVED_BYTE_SIZE   50

#include <SPI.h>
#include <MFRC522.h>
#include <LoRa.h>
#include <MKRWAN.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

const long freq = 868E6;
const int led = 6;
LoRaModem modem;

unsigned long enableRFIDAfter = 0;

// Array containing all the whitelisted ids
byte whitelist[MAX_NB_WHITELISTED_IDS][BYTE_ID_SIZE] = {
  {34, 214, 186, 30}
};

void initRFID() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void launchLoRa(){
  Serial.begin(9600);
  while (!Serial);  // On attend que le port série (série sur USBnatif) soit dispo

  modem.dumb();     // On passe le modem en mode transparent
  
  pinMode(led, OUTPUT);
  LoRa.setPins(LORA_IRQ_DUMB, 6, 1); // set CS, reset, IRQ pin
  LoRa.setTxPower(17, PA_OUTPUT_RFO_PIN);
  LoRa.setSPIFrequency(125E3);
  LoRa.setSignalBandwidth(31.25E3);
  LoRa.setSpreadingFactor(9);
  LoRa.setSyncWord(0x34);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(65535);
  LoRa.begin(freq);
  Serial.println("LoRa Sender");

  if (!LoRa.begin(freq))
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.print("LoRa Started, F = ");
  Serial.println(freq);
}

void checkResultCode(byte byteErr){
  switch (byteErr)
  {
     case 1:
     Serial.println("Succesfully saved uid in database");
     /* TODO si pas dans whitelist add dans whitelist */
     break;
     case 2:
     Serial.println("Couldn't save uid in database.");
     /* TODO si dans whitelist remove dans whitelist */
     break;
     default:
     Serial.println("Error: Unrecognized code encountered.");
     break;
  }
}

void printHexUID(byte* uid){
  String content= "";
  for (byte i = 0; i < 4; i++) 
  {
    content.concat(String(uid[i], HEX));
    if(i != 3 ) content.concat(" ");
  }
  Serial.print(" Body (hex) : ");
  Serial.println(content);
}

void printUID(byte* uid){
  String content= "[";
  for (byte i = 0; i < BYTE_ID_SIZE; i++) {
    content.concat(String(uid[i]));
    if (i != (BYTE_ID_SIZE-1))
      content.concat(String(", "));
  }
  
  content.concat(String("]"));
  
  Serial.print(" Body (byte) : ");
  Serial.print(content);
}

/**
 * Change the given string into a lower case string
 */
int byteArrayCmp(byte* b1, byte* b2, int size) {
  int hash = 0;
  for (int i = 0; i < size; i++)
    hash += b1[i] - b2[i];
  return hash;
}

/**
 * Check if the given ID is valid (a.k.a. in the whitelist)
 * Note that the given ID will be converted into a lowercase char array.
 */
bool isValidID(byte* id) {
  for (int i = 0; i < MAX_NB_WHITELISTED_IDS; i++)
    if (byteArrayCmp(id, whitelist[i], BYTE_ID_SIZE) == 0)
      return true;
  
  return false;
}

/*
  SETUP
*/

void setup() {
  initRFID(); 
  launchLoRa();
}

/*
  Main Loop
*/

void loop() {
  
  //PASSIVE LISTEN FOR MESSAGES
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // read packet
    byte received[RECEIVED_BYTE_SIZE];
    int dataIterator = 0;
    boolean isForUs = true;
    while (LoRa.available()) {
      byte dataByte = LoRa.read();
      received[dataIterator++] = dataByte;
    }
    if(received[1] == 1){
      Serial.println("Received packet");
      printUID(received);
      Serial.println();
      checkResultCode(received[3]);
    }
  }
  
  //PASSIVE LISTEN FOR CARDS
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  if (currentTime < enableRFIDAfter)
    return;
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  currentTime = millis();
  enableRFIDAfter = currentTime + MIN_TIMEOUT_MS;
  
  //Print card UID
  Serial.println("New card presented.");
  printHexUID(mfrc522.uid.uidByte);
  
  // Check if ID is in the whitelist
  if (!isValidID(mfrc522.uid.uidByte)) {
    Serial.println("Blacklisted ID!\nUnauthorized access!\nCalling security...\n");
    return;
  }
  
  digitalWrite(led, HIGH);
  Serial.println("Sending packet <card number> to server.");
  
  //LoRa.send packet
  LoRa.beginPacket();
  int i = 0;
  
  LoRa.write(01); //src
  LoRa.write(43); //dest
  LoRa.write(00); //code fonction
  while (i < 4) LoRa.write((uint8_t)mfrc522.uid.uidByte[i++]);
  
  LoRa.endPacket();
  delay(40);
  digitalWrite(led, LOW); 
  
  Serial.println("Waiting for server response");

}
