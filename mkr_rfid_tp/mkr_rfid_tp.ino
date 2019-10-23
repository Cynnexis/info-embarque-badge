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
#define MAX_NB_BLACKLISTED_IDS  10
#define MAX_NB_CHARS_ID 12

#include <SPI.h>
#include <MFRC522.h>
#include <LoRa.h>
#include <MKRWAN.h>
#include <ctype.h>
#include <string.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

const long freq = 868E6;
const int led = 6;
LoRaModem modem;

unsigned long enableRFIDAfter = 0;

// Array containing all the blacklisted ids
char blacklist[MAX_NB_BLACKLISTED_IDS][MAX_NB_CHARS_ID] = {
  "22 d6 ba 1e"
};

void initRFID() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void launchLoRa() {
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

String printHexUID(byte* uid){
  String content= "";
  for (byte i = 0; i < 4; i++) 
  {
    content.concat(String(uid[i], HEX));
    if(i != 3 ) content.concat(" ");
  }
  Serial.println("New card presented.");
  Serial.print(" UFID : ");
  Serial.println(content);
}

/**
 * Change the given string into a lower case string
 */
void toLowerCase(char* str, int size) {
  for (int i = 0; i < size; i++) {
    str[i] = tolower(str[i]);
  }
}

/**
 * Check if the given ID is valid (a.k.a. not in the blacklist)
 * Note that the given ID will be converted into a lowercase char array.
 */
bool isValidID(char* id) {
  for (int i = 0; i < MAX_NB_BLACKLISTED_IDS; i++) {
    toLowerCase(id, MAX_NB_CHARS_ID);
    if (strcmp(id, blacklist[i]) == 0)
      return false;
  }
  
  return true;
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
  printHexUID(mfrc522.uid.uidByte);
  
  digitalWrite(led, HIGH);
  Serial.println("Sending packet <card number> to server.");
  
  //LoRa.send packet
  LoRa.beginPacket();
  int i = 0;
  
  LoRa.write(01); //src
  LoRa.write(43); //dest
  LoRa.write(01); //code fonction
  //LoRa.write(mfrc522.uid.uidByte); //UID
  while (mfrc522.uid.uidByte[i] != 0) LoRa.write((uint8_t)mfrc522.uid.uidByte[i++]);
  
  LoRa.endPacket();
  delay(40);
  digitalWrite(led, LOW); 

}
