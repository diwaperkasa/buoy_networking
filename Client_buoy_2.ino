#include "Manager_data.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "Sleep_n0m1.h"

Manager_data manager;
Sleep sleep;
unsigned long sleepTime = 0; //how long you want the arduino to sleep
int minuteSleepTime = 15;

void setup() {
  delay(2000);
  // put your setup code here, to run once:
  Serial.begin(9600);
  manager.setup();
  //sleepTime = min2sec(minuteSleepTime);
  sleepTime = 3000;
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long now = millis();
  manager.loop();
  unsigned long loop_time = millis() - now;
  Serial.print(F("Loop time "));
  Serial.print(loop_time);
  Serial.println(F(" ms"));
  delay(100); //delay to allow serial to fully print before sleep
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(sleepTime); //sleep for: sleepTime
}

unsigned long min2sec(int min)
{
  return min * 60000;
}
