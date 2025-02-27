#include <Arduino.h>
#include <DFRobot_MAX31855.h>
#include <Wire.h>
#include <HX711.h>
#include <Adafruit_INA219.h>
#include "SD.h"
#include "NativeEthernet.h"
#include "Servo.h"

//SD
const int mchipSelect = BUILTIN_SDCARD;

//Thermocouples
const uint8_t TEMP_1_PIN_1 = 41;
const uint8_t TEMP_1_PIN_2 = 12;
const uint8_t TEMP_2_PIN_1 = 15;
const uint8_t TEMP_2_PIN_2 = 9;

const int8_t TEMP1_OFFSET = 0;
const int8_t TEMP2_OFFSET = 0;

uint8_t currentTempSensor = 1;

DFRobot_MAX31855 tempsense;

boolean runFlag = true;

//Load cell
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;
const double CALIBRATION_WEIGHT = 2.2;//kg
int64_t LoadcellOffset = 11145;
int64_t LoadcellDivider = -8367;
HX711 LoadCell;

//Pressure sensors
int64_t ina219AOffset = 0;
int64_t ina219AScale = 1;
Adafruit_INA219 ina219A;
int64_t ina219BOffset = 0;
int64_t ina219BScale = 1;
Adafruit_INA219 ina219B;
int64_t ina219COffset = 0;
int64_t ina219CScale = 1;
Adafruit_INA219 ina219C;
int64_t ina219DOffset = 0;
int64_t ina219DScale = 1;
Adafruit_INA219 ina219D;

// SD stuff
uint8_t fileNum = 0;
String fileName = "datalog" + String(fileNum) + ".csv"; 

//Generic
uint64_t lastRead = 0;
const uint8_t READ_DELAY = 200;
typedef struct{
  float temp1 = 0;
  float temp2 = 0;
  float force = 0;
  float pressure1 = 0;
  float pressure2 = 0;
  float pressure3 = 0;
  float pressure4 = 0;
}data;

data data1;


void SetupThermocouple()
{
  
  pinMode(41, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(9, OUTPUT);
  
  tempsense.begin();
}

void SetupSD()
{
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
}

void SetupCurrentSensor()
{
  // Starting ina
  if (! ina219A.begin()) {
    Serial.println("Failed to find INA219A chip");
    while (1);
  }
  ina219A.setCalibration_32V_1A();
  // if (! ina219B.begin()) {
  //   Serial.println("Failed to find INA219B chip");
  //   while (1);
  // }
  // ina219B.setCalibration_32V_1A();
  // if (! ina219C.begin()) {
  //   Serial.println("Failed to find INA219C chip");
  //   while (1);
  // }
  // ina219C.setCalibration_32V_1A();
  // if (! ina219D.begin()) {
  //   Serial.println("Failed to find INA219D chip");
  //   while (1);
  // }
  // ina219D.setCalibration_32V_1A();
}

//only run the calibration once then comment out function call
void CalibrateLoadCell()
{
  Serial.println("Calibrating LoadCell...");
  Serial.println("Remove all weight from the loadcell");
  for (uint8_t i = 0; i < 6; i++)
  {
    delay(1000);
    Serial.println(6 - i);
  }
  
  LoadcellOffset = LoadCell.read_average(20);
  Serial.println("Place the calibration weight on the loadcell (>1kg)");
  for (uint8_t i = 0; i < 6; i++)
  {
    delay(1000);
    Serial.println(6 - i);
  }
  LoadcellDivider = (LoadCell.read_average(20)-LoadcellOffset)/CALIBRATION_WEIGHT;

  Serial.print("Loadcell offset = "); Serial.print(LoadcellOffset);
  Serial.print("Loadcell divider = "); Serial.print(LoadcellDivider);
  delay(10000);
  Serial.println("Finished calibrating");
}

void SetupLoadCell()
{
  LoadCell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void setup() {

  Serial.begin(9600);
  while (!Serial);

  Serial.println("Begun Setup...");
  // gets teensy mac address
  // Starting Ethernet
  //SetupEthernet();

  SetupThermocouple();
  SetupLoadCell();
  //CalibrateLoadCell();
  LoadCell.tare();
  SetupCurrentSensor();

  SetupSD();

  

  Serial.println("Ended Setup");
}

void LogDataToFile(float TA, float TB, float PA, float PB, float PC, float PD, float L)
{
  char charFileName[fileName.length()+1];
  fileName.toCharArray(charFileName, fileName.length()+1);
  File dataFile = SD.open(charFileName, FILE_WRITE);
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

String FormatData()
{
  String reply = "TIME:";
  reply += millis();
  reply += ",T1:";
  reply += data1.temp1;
  reply += ",T2:";
  reply += data1.temp2;
  reply += ",P1:";
  reply += data1.pressure1;
  reply += ",P2:";
  reply += data1.pressure2;
  reply += ",P3:";
  reply += data1.pressure3;
  reply += ",P4:";
  reply += data1.pressure4;
  reply += ",L1:";
  reply += data1.force;
  return reply;
}

void ReadThermocouple()
{
  // switch temp sensor, wait to give enough time for each sensor to read properly
  if ((lastRead - millis()) > READ_DELAY)
  {
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
    currentTempSensor++;
    if (currentTempSensor == 3)
    {
      currentTempSensor = 1;
    }
    lastRead = millis();
  }
}

float CurrentToPressure(float current, uint64_t offset, uint64_t scale)
{

  return (current - offset)/scale;
}

void ReadPressureTransducer()
{
  //Serial.println("S");
  float currentA = -1 * ina219A.getCurrent_mA();
  //Serial.println("GotCurrent");
  // float currentB = ina219B.getCurrent_mA();
  // float currentC = ina219C.getCurrent_mA();
  // float currentD = ina219D.getCurrent_mA();

  data1.pressure1 = CurrentToPressure(currentA, ina219AOffset, ina219AScale);
  //Serial.print("A");Serial.println(currentA);
  // data1.pressure2 = CurrentToPressure(currentB, ina219BOffset, ina219BScale);
  // data1.pressure3 = CurrentToPressure(currentC, ina219COffset, ina219CScale);
  // data1.pressure4 = CurrentToPressure(currentD, ina219DOffset, ina219DScale);

}

void ReadLoadCell()
{
  if (LoadCell.is_ready())
  {
    data1.force = (LoadCell.read() - LoadcellOffset)/LoadcellDivider;
  }
}

void loop() {
  delay(500);
  while(runFlag == true){
    int incomingByte;
    //Serial.println("loop");
    digitalWrite(13, 1);

    ReadThermocouple();
    //Serial.println("A");
    ReadPressureTransducer();
    //Serial.println("B");
    ReadLoadCell();
    //Serial.println("C");
    

    String stringReplyBuffer=FormatData();
    Serial.println(stringReplyBuffer);

    // SendEthernet(stringReplyBuffer);

    // String response = RecieveEthernet();
    
    //LogDataToFile(data1.temp1, data1.temp2, data1.)

    //Pause program if char 'c' is received
    if (Serial.available() > 0) {
      incomingByte = Serial.read();
      Serial.print("UART received: ");
      Serial.println(incomingByte, 16);
      if (incomingByte == 0x63) runFlag = false;
    }

    delay(100);
    digitalWrite(13,0);
  }

  // // Resume program if char 'c' is received
   if (Serial.available() > 0) {
     int incomingByte = Serial.read();
     Serial.print("UART received: ");
     Serial.println(incomingByte, 16);
     if (incomingByte == 0x63) runFlag = true;
   }
  delay(100);

}