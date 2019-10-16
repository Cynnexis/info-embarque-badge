#include <SPI.h>
#include <LoRa.h>
#include <MKRWAN.h>


const long freq = 868E6;
int counter = 1;
const int led = 6;
LoRaModem modem;


void setup()
{
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



void loop()
{
static int count;
count ++;

// Tous les 1000 passages ...
// Todo : Utiliser le temps sytème pour plus de précision
if (count > 1000)
  {
  digitalWrite(led, HIGH);
  Serial.print("Sending packet: ");
  Serial.println(counter);

  //LoRa.send packet
  LoRa.beginPacket();
  LoRa.print("Msg N° : ");
  LoRa.print(counter);
  LoRa.endPacket();
  count =0;
  counter++;
  delay(40);
  digitalWrite(led, LOW);
  }

  
  
 

//Serial.print("Check for packet  ");
int packetSize = LoRa.parsePacket();
  if (packetSize){
  Serial.print("RX packet size =  ");
  Serial.println(packetSize);
  while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
   // print RSSI of packet
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
  LoRa.endPacket();
  }
}
