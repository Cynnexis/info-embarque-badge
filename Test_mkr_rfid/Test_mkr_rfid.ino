/*
 * 
 * 
 */


/* Adaptation pour MKr 1300 2019-04-01 Joseph Ciccarello */
#define RST_PIN         6 //9          // Configurable, see typical pin layout above
#define SS_PIN          7 //10         // Configurable, see typical pin layout above


#include <SPI.h>

//This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
#include <MFRC522.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

/*
 * 
 */
void setup() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}
