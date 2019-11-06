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

byte STATION_ID = 1;
const long freq = 868E6;
const int led = 6;
const int beep = 6;
LoRaModem modem;


unsigned long enableRFIDAfter = 0;
byte* currentUID = NULL;

// Array containing all the whitelisted ids
byte whitelist[MAX_NB_WHITELISTED_IDS][BYTE_ID_SIZE];

// Stop value for whitelist
byte STOP_VALUE[BYTE_ID_SIZE] = {-1, -1, -1, -1};

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
     case 0:
     Serial.println("Couldn't save uid in database.");
     break;
     case 1:
     Serial.println("Success while saving uid in server base");
     break;
     default:
     Serial.println("Error: Unrecognized server code encountered.");
     break;
  }
}

boolean checkWhiteListCode(byte byteErr){
  switch (byteErr)
  {
    case 0:
      //TODO remove from whitelist
      digitalWrite(beep, HIGH);
      delay(1000);
      digitalWrite(beep, LOW);
      Serial.println("> UID Unauthorized by server.");
      return false;
      break;
    case 1:
      //TODO add to white list
      digitalWrite(beep, HIGH);
      delay(1000);
      digitalWrite(beep, LOW);
      Serial.println("> UID Authorized.");
      return true;
      break;
    default:
      Serial.println("> Error: Unrecognized server code encountered.");
      return false;
      break;
  }
}

/**
 * Check if the two arrays are equal.
 */
boolean areArraysEqual(void* array1, size_t length1, void* array2, size_t length2) {
  return length1 == length2 && !memcmp(array1, array2, length1);
}

/**
 * Return the length of the given array, by searching for the stop value. If no stop value detected, return `MAX_NB_WHITELISTED_IDS`.
 */
int len(byte array1[BYTE_ID_SIZE]) {
  for (int i = 0; i < MAX_NB_WHITELISTED_IDS; i++) {
    if (areArraysEqual((void*) array1[i], BYTE_ID_SIZE, (void*) STOP_VALUE, BYTE_ID_SIZE))
      return i;
  }
  return MAX_NB_WHITELISTED_IDS;
}

/**
 * Setup the whitelist by putting STOP_VALUE everywhere
 */
void setupWhitelist() {
  for (int i = 0; i < MAX_NB_WHITELISTED_IDS; i++) {
    memcpy(whitelist[i], STOP_VALUE, BYTE_ID_SIZE);
  }
}

/**
 * Add an ID to the whitelist.
 */
boolean addToWhitelist(byte uid[BYTE_ID_SIZE]) {
  // Search for an empty slot
  int i = 0;
  while (i < MAX_NB_WHITELISTED_IDS && whitelist[i] != NULL) {
    i++;
  }
  
  // Now, either whitelist[i] == NULL (empty slot), or the whole array has been parsed. If so, return false
  if (i == MAX_NB_WHITELISTED_IDS)
    return false;
  
  // Put the uid at the empty slot
  memcpy(whitelist[i], uid, BYTE_ID_SIZE);
  return true;
}

/**
 * Remove an ID from the whitelist.
 */
boolean removeFromWhitelist(byte uid[BYTE_ID_SIZE]) {
  // Search for the uid in the array
  int i;
  boolean found = false;
  for (i = 0; i < MAX_NB_WHITELISTED_IDS; i++) {
    // Are the two uid equal?
    found = areArraysEqual(whitelist[i], BYTE_ID_SIZE, uid, BYTE_ID_SIZE);
    
    // If found, exit the loop
    if (found)
      break;
  }
    
  // If not found, return false
  if (!found)
    return false;
  
  // The index `i` is where uid is in the array. Remove it from the array by shift the other elements
  for (; i < MAX_NB_WHITELISTED_IDS-1; i++)
    memcpy(whitelist[i], whitelist[i + 1], BYTE_ID_SIZE);
  
  memcpy(whitelist[MAX_NB_WHITELISTED_IDS-1], STOP_VALUE, BYTE_ID_SIZE);
  return true;
}

void printHexUID(byte* uid){
  String content= "";
  for (byte i = 0; i < 4; i++) 
  {
    content.concat(String(uid[i], HEX));
    if(i != 3 ) content.concat(" ");
  }
  Serial.print("> Body (hex) : ");
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
  
  Serial.print("> Body (byte) : ");
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

void loraSendRequest(byte* uid, byte src, byte dest, byte codeF) {
  Serial.println("> Sending packet to server.");
  digitalWrite(led, HIGH);
  LoRa.beginPacket();
  int j = 0;
  LoRa.write(src); //src
  LoRa.write(dest); //dest
  LoRa.write(codeF); //code fonction test validité
  while (j < 4) LoRa.write((uint8_t)uid[j++]);
  LoRa.endPacket();
  delay(40);
  digitalWrite(led, LOW); 
  Serial.println("> Waiting for server response");
}

/*
  SETUP
*/

void setup() {
  setupWhitelist();
  byte badge[] = {70, 235, 167, 172};
  addToWhitelist(badge);
  initRFID(); 
  launchLoRa();
  pinMode(beep, OUTPUT);
}

/*
  Main Loop
*/

void loop() {
  
  //-------------------------------
  //PASSIVE LISTEN FOR MESSAGES
  //-------------------------------

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // read packet
    byte received[RECEIVED_BYTE_SIZE];
    int dataIterator = 0;
    while (LoRa.available()) {
      byte dataByte = LoRa.read();
      received[dataIterator++] = dataByte;
    }
    
    //process packet
    if(received[1] == STATION_ID){ //if message is for us
      Serial.println();
      Serial.println("> Received packet");
      printUID(received);
      Serial.println();
      
      if(received[0] != 43){
        Serial.println("> This is not a message from server.");
      }else if(received[2] == 0){
        Serial.println("> Checking door opening.");
        checkResultCode(received[3]);
      }else if(received[2] == 1){
        Serial.println("> Checking auth response.");
        boolean whitelisted = checkWhiteListCode(received[3]);
        if(whitelisted && currentUID != NULL){
          Serial.println("> Asking to open the door.");
          loraSendRequest(currentUID, STATION_ID , 43, 00);
        }
      }
    }
  }
  
  //-------------------------------
  //PASSIVE LISTEN FOR NEW CARDS
  //-------------------------------
  
  // Reset the loop if no new card present on the sensor/reader.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  //Reset the loop if a card has already been read
  if (currentTime < enableRFIDAfter)
    return;
  
  // Select one of the cards. Reset the loop if no cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  //Update delay
  currentTime = millis();
  enableRFIDAfter = currentTime + MIN_TIMEOUT_MS;
  
  //Print new card UID
  Serial.println();
  Serial.println("> New card presented.");
  printHexUID(mfrc522.uid.uidByte);
  printUID(mfrc522.uid.uidByte);
  
  if (!isValidID(mfrc522.uid.uidByte)) {
    Serial.println("> Blacklisted ID locally !\n\t Unauthorized access!");
    Serial.println("\t> Asking auth. to server.");
    loraSendRequest(mfrc522.uid.uidByte, STATION_ID, 43, 01);
  }else{
    Serial.println("> WhiteListed ID locally ! Authorized access");
    Serial.println("> Asking to open the door.");
    loraSendRequest(mfrc522.uid.uidByte, STATION_ID, 43, 00);
  }
  
  currentUID = mfrc522.uid.uidByte;

}
