#include <Arduino.h>
#include <DFRobot_MAX31855.h>
#include <TimeLib.h>
#include <Wire.h>
#include <HX711.h>
#include <Adafruit_INA219.h>
#include "SD.h"
#include "NativeEthernet.h"
#include "Servo.h"

byte mac[] = {0x04, 0xe9, 0xe5, 0x14, 0x8f, 0x36};
IPAddress ip(192, 168, 1, 177);

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
char ReplyBuffer[68];

uint16_t port = 2222;

EthernetServer server(port);
EthernetClient client;
EthernetUDP Udp;

const int mchipSelect = BUILTIN_SDCARD;

const uint8_t TEMP_1_PIN_1 = 41;
const uint8_t TEMP_1_PIN_2 = 12;
const uint8_t TEMP_2_PIN_1 = 15;
const uint8_t TEMP_2_PIN_2 = 9;

const int8_t TEMP1_OFFSET = 0;
const int8_t TEMP2_OFFSET = 0;

uint8_t currentTempSensor = 1;

DFRobot_MAX31855 tempsense;
boolean runFlag = true;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;
float loadCell = 0;
// Initialising Load Cell
HX711 scale;

// Initialising Pressure sensors
Adafruit_INA219 ina219A;
Adafruit_INA219 ina219B;
Adafruit_INA219 ina219C;
Adafruit_INA219 ina219D;

typedef struct{
  float temp1 = 0;
  float temp2 = 0;
  float temp3 = 0;
  float force = 0;
  uint16_t pressure = 0;
}data;

data data1;

// function to get mac address of teensy
void teensyMAC(uint8_t *mac) {
  for(uint8_t by=0; by<2; by++) mac[by]=(HW_OCOTP_MAC1 >> ((1-by)*8)) & 0xFF;
  for(uint8_t by=0; by<4; by++) mac[by+2]=(HW_OCOTP_MAC0 >> ((3-by)*8)) & 0xFF;
  Serial.printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void setup() {

  Serial.begin(9600);
  while (!Serial);
  Serial.println("A");
  // get teensy mac address
  teensyMAC(mac);
  // Starting Ethernet
  Ethernet.begin(mac, ip);

  // check for Ethernet Hardware
  Serial.print("Ethernet Hardware Status: ");Serial.println(Ethernet.hardwareStatus());
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Ethernet hardware not found.  Sorry, can't run without hardware. :(");
    //break do nothing
    while (true);
  }
  Serial.print("Ethernet Link Status: ");Serial.println(Ethernet.linkStatus());
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  Udp.begin(port);

  pinMode(41, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(9, OUTPUT);
  Serial.println("A");
  tempsense.begin();
  Serial.println("A");
  // Starting Load Cell 
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Starting ina
  if (! ina219A.begin()) {
    Serial.println("Failed to find INA219A chip");
    while (1) { delay(10); }
  }
  ina219A.setCalibration_32V_1A();
  if (! ina219B.begin()) {
    Serial.println("Failed to find INA219B chip");
    while (1) { delay(10); }
  }
  ina219B.setCalibration_32V_1A();
  if (! ina219C.begin()) {
    Serial.println("Failed to find INA219C chip");
    while (1) { delay(10); }
  }
  ina219C.setCalibration_32V_1A();
  if (! ina219D.begin()) {
    Serial.println("Failed to find INA219D chip");
    while (1) { delay(10); }
  }
  ina219D.setCalibration_32V_1A();

  // Starting SD chip
  if (!SD.begin(mchipSelect))
  {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
  }
  // Opening File and initialising it
  uint8_t fileNum = 0;
  String fileName = "datalog" + String(fileNum) + ".csv";
  char charFileName[fileName.length()+1];
  fileName.toCharArray(charFileName, fileName.length()+1);

  while (SD.exists(charFileName))
  {
    fileNum += 1;
    fileName = "datalog" + String(fileNum) + ".csv";
    fileName.toCharArray(charFileName, fileName.length()+1);
  }
  Serial.println(fileName);
  File dataFile = SD.open(charFileName, FILE_WRITE);
  dataFile.println("ThermocoupleA,ThermocoupleB,PressureA,PressureB,LoadCell");
  dataFile.close();
  Serial.println("Ended Setup");
}
void LogDataToFile(float TA, float TB, float PA, float PB, float PC, float PD, float L)
{
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile)
  {
    dataFile.print(String(TA));
    dataFile.print(",");
    dataFile.print(String(TB));
    dataFile.print(",");
    dataFile.print(String(PA));
    dataFile.print(",");
    dataFile.print(String(PB));
    dataFile.print(",");
    dataFile.print(String(PC));
    dataFile.print(",");
    dataFile.print(String(PD));
    dataFile.print(",");
    dataFile.print(String(L));
    dataFile.println(",");
  }
  else{
    Serial.println("Error opening file");
  }
  dataFile.close();
}

void loop() {
  EthernetClient client = server.available();
  Serial.println("Looping");

  while(runFlag == true){
    int incomingByte;
    digitalWrite(13, 1);

    //Serial.println(currentTempSensor);
    // Read Temperature

    switch (currentTempSensor)
    {
    case 1:
      digitalWrite(TEMP_1_PIN_1, HIGH);
      digitalWrite(TEMP_1_PIN_2, HIGH);
      digitalWrite(TEMP_2_PIN_1, LOW);
      digitalWrite(TEMP_2_PIN_2, LOW);
      data1.temp1 = tempsense.readCelsius();
      break;
    case 2:
      digitalWrite(TEMP_2_PIN_1, HIGH);
      digitalWrite(TEMP_2_PIN_2, HIGH);
      digitalWrite(TEMP_1_PIN_1, LOW);
      digitalWrite(TEMP_1_PIN_2, LOW);
      data1.temp2 = tempsense.readCelsius();
      break;
    }

    delay(200);
    currentTempSensor++;

    if (currentTempSensor == 3)
    {
      currentTempSensor = 1;
    }
    String stringReplyBuffer = "TIME:";

    // Format data packet
    stringReplyBuffer += hour();
    stringReplyBuffer += ":";
    stringReplyBuffer += minute();
    stringReplyBuffer += ":";
    stringReplyBuffer += second();
    stringReplyBuffer += " - T1 = ";
    stringReplyBuffer += data1.temp1;
    stringReplyBuffer += " | T2 = ";
    stringReplyBuffer += data1.temp2;
    stringReplyBuffer += " | T3 = ";
    stringReplyBuffer += data1.temp3;
    stringReplyBuffer += " | FORCE = ";
    stringReplyBuffer += data1.force;
    stringReplyBuffer += " | P1 = ";
    stringReplyBuffer += data1.pressure;
    // Pring it all to Serial
    Serial.print(stringReplyBuffer);
    // Serial.print(hour());
    // Serial.print(":");
    // Serial.print(minute());
    // Serial.print(":");
    // Serial.print(second());
    // Serial.print(" - ");
    // Serial.print("T1 = ");
    // Serial.print(data1.temp1);
    // Serial.print(" | ");
    // Serial.print("T2 = ");
    // Serial.print(data1.temp2);
    // Serial.print(" | ");
    // Serial.print("T3 = ");
    // Serial.print(data1.temp3);
    // Serial.print(" | ");
    // Serial.print("FORCE = ");
    // Serial.print(data1.force);
    // Serial.print(" | ");
    // Serial.print("P1 = ");
    // Serial.print(data1.pressure);  
    // Serial.println("");  

    // Sending Ethernet Message
    stringReplyBuffer.toCharArray(ReplyBuffer, stringReplyBuffer.length() + 1);
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
      IPAddress remote = Udp.remoteIP();
      for (int i=0; i < 4; i++) {
        Serial.print(remote[i], DEC);
        if (i < 3) {
          Serial.print(".");
        }
      }
      Serial.print(", port ");
      Serial.println(Udp.remotePort());

      // read the packet into packetBufffer
      Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      Serial.println("Contents:");
      Serial.println(packetBuffer);

      // send a reply to the IP address and port that sent us the packet we received
    }
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Serial.println(ReplyBuffer);
    Udp.endPacket();

    //LogDataToFile(data1.temp1, data1.temp2, data1.)

    // Pause program if char 'c' is received
    if (Serial.available() > 0) {
      incomingByte = Serial.read();
      Serial.print("UART received: ");
      Serial.println(incomingByte, 16);
      if (incomingByte == 0x63) runFlag = false;
    }

    //delay(500);
    digitalWrite(13,0);
  }

  // Resume program if char 'c' is received
  if (Serial.available() > 0) {
    int incomingByte = Serial.read();
    Serial.print("UART received: ");
    Serial.println(incomingByte, 16);
    if (incomingByte == 0x63) runFlag = true;
  }
  delay(500);

}