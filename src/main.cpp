#include <Arduino.h>
#include <DFRobot_MAX31855.h>
#include <TimeLib.h>

const uint8_t TEMP_1_PIN_1 = 41;
const uint8_t TEMP_1_PIN_2 = 12;
const uint8_t TEMP_2_PIN_1 = 15;
const uint8_t TEMP_2_PIN_2 = 9;

const int8_t TEMP1_OFFSET = 0;
const int8_t TEMP2_OFFSET = 0;

uint8_t currentTempSensor = 1;

DFRobot_MAX31855 tempsense;
boolean runFlag = true;


typedef struct{
  float temp1 = 0;
  float temp2 = 0;
  float temp3 = 0;
  float force = 0;
  uint16_t pressure = 0;
}data;

data data1;

void setup() {

  Serial.begin(9600);
  Serial.println("A");
  pinMode(41, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(9, OUTPUT);
  Serial.println("A");
  tempsense.begin();
  Serial.println("A");
}


void loop() {
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

    // Format data packet
    Serial.print(hour());
    Serial.print(":");
    Serial.print(minute());
    Serial.print(":");
    Serial.print(second());
    Serial.print(" - ");
    Serial.print("T1 = ");
    Serial.print(data1.temp1);
    Serial.print(" | ");
    Serial.print("T2 = ");
    Serial.print(data1.temp2);
    Serial.print(" | ");
    Serial.print("T3 = ");
    Serial.print(data1.temp3);
    Serial.print(" | ");
    Serial.print("FORCE = ");
    Serial.print(data1.force);
    Serial.print(" | ");
    Serial.print("P1 = ");
    Serial.print(data1.pressure);  
    Serial.println("");  


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