/* Created: Diwa Perkasa 2016, Bogor, Jawa Barat
   var "payload" digunakan untuk pengiriman data ke XBee dalam mode API
   pengiriman dilakukan per-byte sehingga harus dilakukan pencacahan
   index payload: 0:ID Buoy
   String(VBat, 2).c_str()
*/
#include <math.h>    // (no semicolon)

//SET ID BUOY, EACH BUOY HAS UNIQUE ID
#define header "@dp"
char buoyid[] = "001";
//__________________________________________________________________________

long pressure = 0; float temp_press = 0;
float temp_DHT = 0; float hum_DHT = 0;
float temp_DS = 0;
char Date[15], Time[15], filename[15];
float VBat = 0;
//--------------------------------------------------------


#include "Manager_data.h"
#include "Adafruit_BMP085.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "DHT.h"
#include <Wire.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
#include "Xbee_command.h"
#include "Manager_data.h"
#include "Timer.h"
#include "Sleep_n0m1.h"

Timer xbee_timeout;
Xbee_cmd cmd;
//-----------------------------Battery  & Accecories------------------------------------

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

float Vbat(char pin) {
  int sensorValue = analogRead(pin);
  float Vcc = readVcc() / 1000.0;
  float Vin = (sensorValue * Vcc);
  Vin = (Vin / 1023.0) * 3.0;
  return Vin;
}

void Acc_setup() {

}

void Acc_loop() {
  VBat = Vbat(A0);
}
//----------------------------------------BMP085---------------------------------------
Adafruit_BMP085 bmp;
boolean bmp_loop_setup = false;
boolean bmp_exist = false;

void BMP_setup() {
  if (!bmp.begin()) {
    cmd.send("log>Pressure failed");
    bmp_exist = false;
    //Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  } else {
    bmp_exist = true;
  }
}

void BMP_loop() {
  if (bmp_loop_setup) {
    BMP_setup();
  }

  if (bmp_exist) {
    pressure = bmp.readPressure(); // in Pascal
    temp_press = bmp.readTemperature();
  }

  bmp_loop_setup = true;
}
//---------------------------------------MicroSD---------------------------------------
File myFile;
#define CSPin 53
boolean SD_loop_setup = false;
boolean SD_exist = false;

void SD_setup() {
  if (!SD.begin(CSPin)) {
    cmd.send("log>No SD Card");
    SD_exist = false;
    //Serial.println("initialization failed!");
  } else {
    SD_exist = true;
  }
}

void SD_loop() {
  if (SD_loop_setup) {
    SD_setup();
  }

  if (SD_exist) {
    myFile = SD.open(filename, FILE_WRITE);

    if (SD.exists(filename)) {
      if (myFile) {
        myFile.print("ID");
        myFile.print("\t");
        myFile.print("TimeStamp");
        myFile.print("\t");
        myFile.print("Suhu Air (C)");
        myFile.print("\t");
        myFile.print("Suhu Udara (C)");
        myFile.print("\t");
        myFile.print("Suhu Udara BMP (C)");
        myFile.print("\t");
        myFile.print("Kelembapan (%)");
        myFile.print("\t");
        myFile.print("Tekanan (Pa)");
        myFile.print("\t");
        myFile.println("Voltase (V)");
      }
    }

    if (myFile) {
      myFile.print(buoyid);
      myFile.print("\t");
      myFile.print(Date);
      myFile.print(" ");
      myFile.print(Time);
      myFile.print("\t");
      myFile.print(temp_DS);
      myFile.print("\t");
      myFile.print(temp_DHT);
      myFile.print("\t");
      myFile.print(temp_press);
      myFile.print("\t");
      myFile.print(hum_DHT);
      myFile.print("\t");
      myFile.print(pressure);
      myFile.print("\t");
      myFile.println(VBat);
    }
    myFile.close();
  }
  SD_loop_setup = true;
}

//---------------------------------------DHT22-----------------------------------------
#define DHTPIN A11
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
boolean DHT_loop_setup = false;

void DHT_setup() {
  dht.begin();
}

void DHT_loop() {
  if (DHT_loop_setup) {
    DHT_setup();
  }
  hum_DHT = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp_DHT = dht.readTemperature();
  DHT_loop_setup = true;
}

//--------------------------------------DS1802b----------------------------------------
#define DSPIN A10
OneWire  ds(DSPIN);  // on pin 10 (a 4.7K resistor is necessary)
DallasTemperature sensors(&ds);
DeviceAddress insideThermometer;
boolean ds_loop_setup = false;
boolean ds_exist = false;

void DS_setup() {
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0))
  {
    cmd.send("log>Water Temp failed");
    ds_exist = false;
  } else {
    ds_exist = true;
  }
}

void DS_loop() {
  if (ds_loop_setup)
  {
    DS_setup();
  }

  if (ds_exist) {
    sensors.requestTemperatures();
    temp_DS = sensors.getTempCByIndex(0);
  }

  ds_loop_setup = true;
}
//------------------------------Enable PIN Power---------------------------------------
#define enable_pin 12

void enable_power_setup() {
  pinMode(enable_pin, OUTPUT);
  digitalWrite(enable_pin, HIGH);
}

void enable_power_loop(boolean state) {
  if (state) {
    digitalWrite(enable_pin, HIGH);
  } else {
    digitalWrite(enable_pin, LOW);
  }
  delay(1000);
}

//---------------------------------------RTC-------------------------------------------
RTC_DS3231 rtc;

void RTC_setup() {
  if (! rtc.begin()) {
    cmd.send("log>RTC failed");
    //Serial.println("Couldn't find RTC");
  }
}

void getRTC(char *date, char *time) {
  DateTime now = rtc.now();
  sprintf(date, "%i/%i/%i", now.day(), now.month(), now.year());
  sprintf(time, "%i:%i:%i", now.hour(), now.minute(), now.second());
}

void getFilename(char *file) {
  DateTime now = rtc.now();
  sprintf(file, "%i-%i-%i.txt", now.day(), now.month(), now.year());
}

void RTC_loop() {
  getRTC(Date, Time);
  getFilename(filename);
}

void RTC_ajust(char temp) {
  char * parsing;
  byte i = 0;
  int a[8];
  parsing = strtok (temp, ",");
  while (parsing != NULL) {
    if (i == 0) {
      // nothing
    } else {
      //this is a parameter strcpy(raw_args[i], parsing);
      a[i] = atoi(parsing);
    }
    i++;
    parsing = strtok (NULL, ",");
  }
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  rtc.adjust(DateTime(a[3], a[2], a[1], a[4], a[5], a[6]));
}

//---------------------------------XBEE Command--------------------------------------
void remote_command() {
  cmd.get();
  if (cmd.cmp("dp")) {
    if (cmd.args[1] == 100) {
      cmd.send("log>Ayee, ready captain!!");
    }
  }

  if (cmd.cmp("led")) {
    int nilai = cmd.args[1];
  }

  if (cmd.cmp("rtc_ajust")) {
    RTC_ajust(cmd.raw_args[1]);
  }

  if (cmd.cmp("ping")) {
    cmd.send("log>pong");
  }
}

void sendFormatedData()
{
  char message[max_text];
  sprintf(message, "%s>%s,%s %s,%s,%s,%s,%s,%lu,%s", header, buoyid, Date, Time, String(temp_DS, 2).c_str(), \
          String(temp_DHT, 2).c_str(), String(temp_press, 2).c_str(), String(hum_DHT, 2).c_str(), pressure, String(VBat, 2).c_str()); \
  cmd.send(message);
}

//--------------------------------MAIN PROGRAM-----------------------------------------

void Manager_data::setup()
{
  enable_power_setup();
  Serial1.begin(9600);
  cmd.setup(Serial1);
  BMP_setup();
  DHT_setup();
  DS_setup();
  RTC_setup();
  SD_setup();
  Acc_setup();
  cmd.send("log>Buoy start up");
}

void Manager_data::loop()
{
  enable_power_loop(1);
  Serial.println(F("Bouy is going to wake up"));
  RTC_loop();
  BMP_loop();
  DHT_loop();
  DS_loop();
  Acc_loop();
  SD_loop();
  sendFormatedData();
  xbee_timeout.reset();
  while (!xbee_timeout.elapsed(5000))
  {
    remote_command();
  }
  Serial.println(F("Bouy is going to sleep"));
  enable_power_loop(0);
}
