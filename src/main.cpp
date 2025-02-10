#include <Arduino.h>
#include <DFRobot_MAX31855.h>
#include <TimeLib.h>

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

  Serial1.begin(9600);
  pinMode(13, OUTPUT);
  tempsense.begin();

}


void loop() {
  while(runFlag == true){
    int incomingByte;
    digitalWrite(13, 1);

    // Read Temperature
    data1.temp1 = tempsense.readCelsius();
    data1.temp2 = tempsense.readCelsius();
    data1.temp3 = tempsense.readCelsius();

    // Format data packet
    Serial1.print(hour());
    Serial1.print(":");
    Serial1.print(minute());
    Serial1.print(":");
    Serial1.print(second());
    Serial1.print(" - ");
    Serial1.print("T1 = ");
    Serial1.print(data1.temp1);
    Serial1.print(" | ");
    Serial1.print("T2 = ");
    Serial1.print(data1.temp2);
    Serial1.print(" | ");
    Serial1.print("T3 = ");
    Serial1.print(data1.temp3);
    Serial1.print(" | ");
    Serial1.print("FORCE = ");
    Serial1.print(data1.force);
    Serial1.print(" | ");
    Serial1.print("P1 = ");
    Serial1.print(data1.pressure);  
    Serial1.println("");  


    // Pause program if char 'c' is received
    if (Serial1.available() > 0) {
      incomingByte = Serial1.read();
      Serial1.print("UART received: ");
      Serial1.println(incomingByte, 16);
      if (incomingByte == 0x63) runFlag = false;
    }

    delay(500);
    digitalWrite(13,0);
  }

  // Resume program if char 'c' is received
  if (Serial1.available() > 0) {
    int incomingByte = Serial1.read();
    Serial1.print("UART received: ");
    Serial1.println(incomingByte, 16);
    if (incomingByte == 0x63) runFlag = true;
  }
  delay(500);

}