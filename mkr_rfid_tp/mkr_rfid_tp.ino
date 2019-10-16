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

#define MIN_TIMEOUT_MS   2000


#include <SPI.h>
#include <MFRC522.h>
#include <LoRa.h>
#include <MKRWAN.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

const long freq = 868E6;
const int led = 6;
LoRaModem modem;

unsigned long enableRFIDAfter = 0;

void initRFID(){
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

boolean checkAuth(){
  if (content.substring(1) == "BD 31 15 2B")
  {
    Serial.println("Authorized access");
    Serial.println();
    delay(3000);
  }
 
 else   {
    Serial.println(" Access denied");
    delay(3000);
  }
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
  
    unsigned int readingCard = 0;
    String content= "";
    //Print card UID
    Serial.println("New card presented.");
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
       content.concat(String(mfrc522.uid.uidByte[i], HEX));
       if(i != mfrc522.uid.size -1 ) content.concat(" ");
    }
    Serial.print(" UFID : ");
    Serial.println(content);
    
    digitalWrite(led, HIGH);
    Serial.print("Sending packet <card number> to server: ");
  
    //LoRa.send packet
    LoRa.beginPacket();
    
    LoRa.println("src : 01");
    LoRa.println("dest : 43");
    LoRa.println("obj : UID");
    LoRa.print("msg : ");
    LoRa.println(content);
    
    LoRa.endPacket();
    delay(40);
    digitalWrite(led, LOW);

}