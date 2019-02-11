#include <HID.h>

/*
   Typical pin layout used:
   cardUiD: Card UID: 60 25 1B 14
   keyChainUid: Card UID: 3D 27 08 C2
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
//#include <TinyGPS++.h>
//#include "TinyGPS++.h"
//#include "SoftwareSerial.h"
#define SS_PIN 53
#define RST_PIN 5



SoftwareSerial GPSModule(10, 11); // RX, TX ( For GPS )
SoftwareSerial GSMModule(2, 3); // RX, TX ( For GSM )
const int rs = 6, en = 7, d4 = 8, d5 = 9, d6 = 12, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int updates;
int failedUpdates;
int pos;
int stringplace = 0;
//TinyGPSPlus gps;

String timeUp;
String nmea[15];

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
unsigned long previousMillis = 0; //to store long number
const long interval = 10000; //Duration for sending location to server;

void setup()
{
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  lcd.begin(16, 2);
  GPSModule.begin(9600);//This opens up communications to the GPS
  GSMModule.begin(9600);
  mfrc522.PCD_Init();   // Initiate MFRC522
  //  Serial.println("Put your card to the reader...");
  //  Serial.println();
  rfidCode();
  //  gpsCode();
  connectGPRS();

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

// *********************************************************************************** RFID Code *************************************************************************
void rfidCode() { // RFID Module code
  // Look for new cards
  lcd.clear();
  lcd.print("Scan Your ID");
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    //    Serial.print("new card present");
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    //    Serial.print("purana card present");
    return;
  }
  //Show UID on serial monitor
  //  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    //    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  //  Serial.println();
  //  Serial.print("Message : ");
  content.toUpperCase();
  Serial.println(content.substring(1));
  if (content.substring(1) == "60 25 1B 14" || content.substring(1) == "3D 27 08 C2" || content.substring(1) == "3D 8A 43 B2") //change here the UID of the card/cards that you want to give access
  {
    lcd.clear();
    lcd.print("Authorized User");
    delay(500);
    lcd.clear();
    lcd.print("Welcome!");
    delay(500);
    sendRFIDToServer(content.substring(1));
    lcd.clear();
    //    Serial.println("Authorized access");
    //    Serial.println();

  }

  else   {
    //    Serial.println(" Access denied");
    lcd.clear();
    lcd.print("UnAuthorizedUser");
    delay(500);
    lcd.clear();
    //      lcd.print("Welcome!");
    //      delay(1000);
    lcd.clear();
  }
}

//*************************************************************** GPS CODE ****************************************************************

void gpsCode() { // GPS Code
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
    //    Serial.println("Latitude: " + nmea[2]);
    //    Serial.println("Longitude: " + nmea[4]);
    //    Serial.println("Status: " + nmea[1]);
    //    Serial.println("Time in utc: " + nmea[0]);
    //    Serial.println("Date: " + nmea[8]);

    gsmCode(nmea[2], nmea[4], nmea[0], nmea[8]);

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
      lngfirst = nmea[4].substring(1, i - 2);
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

//******************************************************************************* GSM CODE **************************************************************************

void gsmCode(String latitude, String longitude, String UTCTime, String currDate ) {
  //  Serial.println("gsm round");
  //  GSMModule.println("AT+CMGF=1");
  //  delay(1000);
  //  GSMModule.println("AT+CMGS=\"+923362602053\"\r");
  //  delay(1000);
  sendLatLngAndDateTimeToServer(latitude, longitude, UTCTime, currDate);
  //  GSMModule.println("Latitude: " + latitude + "\nLongitude: " + longitude + "\nUTCTime: " + UTCTime + "\ncurrDate: " + currDate);
  //  delay(1000);
  //  GSMModule.println((char)26);
  //  delay(1000);

}

//***************************************************************************** SEND RFID ID TO SERVER *************************************************************

void sendRFIDToServer(String rfid) {
  //  Serial.println("gsm round");
//  GSMModule.println("AT+CMGF=1");
//  delay(1000);
//  GSMModule.println("AT+CMGS=\"+923362602053\"\r");
//  delay(1000);
//  GSMModule.println("user Id: " + id);
//  delay(1000);+ ",  \"lng\" :" + String(0.000)
//  GSMModule.println((char)26);
//  delay(1000);


GSMModule.println("AT+HTTPINIT");
  //  myGsm.println("AT+CIFSR"); //init the HTTP request
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPSSL=1");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPPARA=\"CID\",1");
  // myGsm.println("AT+CIPSTART=\"TCP\",\"122.178.80.228\",\"350\"");
  delay(1000);
  //  printSerialData();
  //  delay(5000);
  GSMModule.println("AT+HTTPPARA=\"URL\",\"warm-thicket-69046.herokuapp.com/attendance\"");
  // myGsm.println("AT+CIPSEND");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  //sendtemp();
  delay(1000);
  //myGsm.println("AT+CIPCLOSE");
  //  printSerialData();
  String reading = "";
  reading = "{\"bus_name\":\"HU-01\",  \"rfid\" :\"" + rfid  + "\" }";
  Serial.println(reading);
  GSMModule.println("AT+HTTPDATA=" + String(reading.length()) + ",100000");
  //myGsm.println("AT+CIPSHUT");
  delay(1000);
  //  printSerialData();


  GSMModule.println(reading);
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPACTION=1");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPREAD");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPTERM");
  delay(1000);
  //  printSerialData();  `
}







void connectGPRS() {

  GSMModule.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  delay(1000);
  //  /printSerialData();

  GSMModule.println("AT+SAPBR=3,1,\"APN\",\"telenorbg \"");//APN
  delay(1000);
  //  pri/ntSerialData();

  GSMModule.println("AT+SAPBR=1,1");
  delay(1000);
  //  printSeri/alData();

  GSMModule.println("AT+SAPBR=2,1");
  delay(1000);
  //  prin/tSerialData();
}




void sendLatLngAndDateTimeToServer(String latitude, String longitude, String timeFromGps, String dateFromGps) {

  //  Serial.println("writing message");
  //  GPSModule.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  //  delay(1000);  // Delay of 1000 milli seconds or 1 second
  //  GPSModule.println("AT+CMGS=\"+923362602053\"\r"); // Replace x with mobile number
  //  delay(1000);
  //  GPSModule.println("HU-02 " + String(latitude) + " " + String(longitude) + " " + String(timeFromGps) + " " + String( dateFromGps) + " "); // The SMS text you want to send
  //  delay(100);
  //  GPSModule.println((char)26);// ASCII code of CTRL+Z
  //  delay(1000);


  GSMModule.println("AT+HTTPINIT");
  //  myGsm.println("AT+CIFSR"); //init the HTTP request
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPSSL=1");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPPARA=\"CID\",1");
  // myGsm.println("AT+CIPSTART=\"TCP\",\"122.178.80.228\",\"350\"");
  delay(1000);
  //  printSerialData();
  //  delay(5000);
  GSMModule.println("AT+HTTPPARA=\"URL\",\"warm-thicket-69046.herokuapp.com/live\"");
  // myGsm.println("AT+CIPSEND");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  //sendtemp();
  delay(1000);
  //myGsm.println("AT+CIPCLOSE");
  //  printSerialData();
  String reading = "";
  int indexLat = String(latitude).indexOf(".");
  int indexLong = String(longitude).indexOf(".");
  if (indexLat == 0 || indexLong == 0) {
    reading = "{\"bus_name\":\"HU-01\",  \"lat\" :" + String(0.0000) + ",  \"lng\" :" + String(0.000) + " }";
  } else {
    reading = "{\"bus_name\":\"HU-01\",  \"lat\" :" + String(latitude) + ",  \"lng\" :" + String(longitude) + " }";
  }
  Serial.println(reading);
  GSMModule.println("AT+HTTPDATA=" + String(reading.length()) + ",100000");
  //myGsm.println("AT+CIPSHUT");
  delay(1000);
  //  printSerialData();


  GSMModule.println(reading);
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPACTION=1");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPREAD");
  delay(1000);
  //  printSerialData();

  GSMModule.println("AT+HTTPTERM");
  delay(1000);
  //  printSerialData();

}
