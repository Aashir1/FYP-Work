/* 
 * Typical pin layout used:
 * cardUiD: Card UID: 60 25 1B 14
 * keyChainUid: Card UID: 3D 27 08 C2
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
//#include "TinyGPS++.h"
//#include "SoftwareSerial.h"
#define SS_PIN 53
#define RST_PIN 5



SoftwareSerial GPSModule(10, 11); // RX, TX
int updates;
int failedUpdates;
int pos;
int stringplace = 0;
TinyGPSPlus gps;

String timeUp;
String nmea[15];
String labels[12] {"Time: ", "Status: ", "Latitude: ", "Hemisphere: ", "Longitude: ", "Hemisphere: ", "Speed: ", "Track Angle: ", "Date: "};

//SoftwareSerial serial_connection(10, 11); //RX=pin 10, TX=pin 11
//TinyGPSPlus gps;//This is the GPS object that will pretty much do all the grunt work with the NMEA data
 
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
unsigned long previousMillis = 0; //to store long number
const long interval = 10000; //Duration for sending location to server;
 
void setup() 
{
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  GPSModule.begin(9600);//This opens up communications to the GPS
  mfrc522.PCD_Init();   // Initiate MFRC522
  Serial.println("Put your card to the reader...");
  Serial.println();
  rfidCode();
  gpsCode();

}
void loop() 
{
  rfidCode();
  
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    gpsCode();
  }

} 


void rfidCode(){ // RFID Module code
  // Look for new cards
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "60 25 1B 14" ||content.substring(1) == "3D 27 08 C2") //change here the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access");
    Serial.println();
//    delay(3000);
  }
 
 else   {
    Serial.println(" Access denied");
//    delay(3000);
  }
 }
//*************************************************************** GPS CODE ****************************************************************
void gpsCode(){ // GPS Code 
    Serial.flush();
  GPSModule.flush();
  while (GPSModule.available() > 0)
  {
    GPSModule.read();

  }
  if (GPSModule.find("$GPRMC,")) {
    String tempMsg = GPSModule.readStringUntil('\n');
    for (int i = 0; i < tempMsg.length(); i++) {
      if (tempMsg.substring(i, i + 1) == ",") {
        nmea[pos] = tempMsg.substring(stringplace, i);
        stringplace = i + 1;
        pos++;
      }
      if (i == tempMsg.length() - 1) {
        nmea[pos] = tempMsg.substring(stringplace, i);
      }
    }
    updates++;
    nmea[2] = ConvertLat();
    nmea[4] = ConvertLng();
    Serial.println("Latitude: " + nmea[2]);
    Serial.println("Longitude: " + nmea[4]);
    Serial.println("Status: " + nmea[1]);
    Serial.println("Time in utc: " + nmea[0]);
    Serial.println("Date: " + nmea[8]);
//    ********************************** Comment_Code *****************************************
//    for (int i = 0; i < 9; i++) {
//      Serial.print(labels[i]);
//      Serial.print(nmea[i]);
//      Serial.println("");
//    }
//    ********************************** Comment_Code *****************************************

  }
  else {

    failedUpdates++;

  }
  stringplace = 0;
  pos = 0;
//  delay(12000);
}

//*************************************************************** CONVERT LAT ****************************************************************

String ConvertLat() {
  String posneg = "";
  if (nmea[3] == "S") {
    posneg = "-";
  }
  String latfirst;
  float latsecond;
  for (int i = 0; i < nmea[2].length(); i++) {
    if (nmea[2].substring(i, i + 1) == ".") {
      latfirst = nmea[2].substring(0, i - 2);
      latsecond = nmea[2].substring(i - 2).toFloat();
    }
  }
  latsecond = latsecond / 60;
  String CalcLat = "";

  char charVal[9];
  dtostrf(latsecond, 4, 6, charVal);
  for (int i = 0; i < sizeof(charVal); i++)
  {
    CalcLat += charVal[i];
  }
  latfirst += CalcLat.substring(1);
  latfirst = posneg += latfirst;
  return latfirst;
}

//*************************************************************** CONVERT LONGITUDE ****************************************************************

String ConvertLng() {
  String posneg = "";
  if (nmea[5] == "W") {
    posneg = "-";
  }

  String lngfirst;
  float lngsecond;
  for (int i = 0; i < nmea[4].length(); i++) {
    if (nmea[4].substring(i, i + 1) == ".") {
      lngfirst = nmea[4].substring(0, i - 2);
      //Serial.println(lngfirst);
      lngsecond = nmea[4].substring(i - 2).toFloat();
      //Serial.println(lngsecond);

    }
  }
  lngsecond = lngsecond / 60;
  String CalcLng = "";
  char charVal[9];
  dtostrf(lngsecond, 4, 6, charVal);
  for (int i = 0; i < sizeof(charVal); i++)
  {
    CalcLng += charVal[i];
  }
  lngfirst += CalcLng.substring(1);
  lngfirst = posneg += lngfirst;
  return lngfirst;
}

