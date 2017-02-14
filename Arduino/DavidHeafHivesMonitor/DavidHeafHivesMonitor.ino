/**
 * Read/record temperatures of 6 sensors
 * DS18B20 using their addresses
 */
#define DEBUG   0 // set to 1 to trace activity via serial console
const char DELIMITER = ','; // value delimiter in file

#include <JeeLib.h>
#include <avr/sleep.h>

volatile bool adcDone;
ISR(WDT_vect) { Sleepy::watchdogEvent(); }
ISR(ADC_vect) { adcDone = true; }

#define SLEEP_DURATION 1000 * 30 // 1000ms * 30

/**
 *  We need OneWire library to access DS18B20 sensors
 *  the library must be placed in libraries directory
 *  in the sketchs directory of Arduino IDE (settings menu)
 *
 *  cf: https://github.com/PaulStoffregen/OneWire/archive/master.zip
 */
#include <OneWire.h>

/**
 * We also need DallasTemperature library
 * cf: https://github.com/milesburton/Arduino-Temperature-Control-Library
 */
#include <DallasTemperature.h>

/*
 *  The data wire of DS18B20 (often yellow?) is wired to pin 8 (D8) of Arduino
 *  a 4,7kOhm resistor is wired between Vcc and D8
 */
#define ONE_WIRE_BUS 8

/**
 * Cretae a oneWire instance to be able to
 * communicate with OneWire devices
 * Nb: not only Maxim/Dallas sensors
 */
OneWire oneWire(ONE_WIRE_BUS);

/**
 * Passing Onewire instance to our new
 * sensors object DallasTemperature
 */
DallasTemperature sensors(&oneWire);

/**
 * Each sensors has its own adresses on bus,
 * cf: https://git.io/vwM2x
 * to scan the addresses
 ex:
 adresse :	0x28, 0x42, 0x6B, 0x1C, 0x07, 0x00, 0x00, 0x34
 adresse :	0x28, 0xED, 0x0B, 0x2A, 0x07, 0x00, 0x00, 0x1C
 adresse :	0x28, 0xF7, 0x9D, 0x29, 0x07, 0x00, 0x00, 0x2B
 */

DeviceAddress temperature1 = { 0x28, 0xFF, 0xD2, 0x23, 0x36, 0x16, 0x03, 0xC6 };
DeviceAddress temperature2 = { 0x28, 0xFF, 0xFA, 0xA2, 0x36, 0x16, 0x03, 0x56 };
DeviceAddress temperature3 = { 0x28, 0xFF, 0x7E, 0x23, 0x36, 0x16, 0x03, 0xD4 };
DeviceAddress temperature4 = { 0x28, 0xFF, 0xB9, 0x43, 0x36, 0x16, 0x04, 0xB4 };
DeviceAddress temperature5 = { 0x28, 0xFF, 0xC5, 0x2C, 0x36, 0x16, 0x04, 0xF2 };
DeviceAddress temperature6 = { 0x28, 0xFF, 0xF5, 0x04, 0x36, 0x16, 0x03, 0x99 };

/**
 * Writing on SD card
 *
 * SD card uses SPI bus:
 * MOSI       - pin 11
 * MISO       - pin 12
 * CLK or SCK - pin 13
 * CS         - pin 4
 *
 * SPI for Serial Peripheral Interface
 *
 * created  24 Nov 2010
 * modified 9 Apr 2012
 * by Tom Igoe
 * cf: https://www.arduino.cc/en/Tutorial/Datalogger
 */
#include <SPI.h>
//#include <SD.h>
#include "SdFat.h"
SdFat SD;

// Arduino Uno pin 4
// cf: https://www.arduino.cc/en/Reference/SPI
const int chipSelect = 4;

bool sDisReady = false; // only try to write if sd is ready (detected)

/**
 * Tiny RTC module (clock)
 *
 * DS3231 on I2C bus
 * with lithium battery CR1225
 *
 * Arduino I2C port is on
 * pin A4 and A5
 *
 * Analog pin A5 <-> SCL (blue wire)
 * Analog pin A4 <-> SDA (green wire)
 */

#include <Wire.h>
/**
 * We are using the library RTClib
 * cf: https://github.com/adafruit/RTClib/archive/master.zip
 */
#include "RTClib.h"
RTC_DS3231 RTC;


void setup(void)
{
  // Serial port initialisation (to communicate with computer)
  #if DEBUG
    Serial.begin(9600);
    Serial.println("start");
  #endif


  if(SD.begin(chipSelect))
  {
    sDisReady = true;
  }
  // sensors initialisation
  sensors.begin();
}


void loop(void)
{

    String tolog = builString();
  #if DEBUG
    Serial.println(tolog);   // send "file line" to computer serial
    Serial.flush();delay(5); // needed to flush serial when woke up
  #endif

    if (sDisReady) {
      /**
       * Opening the file
       * Nb: only one file can be opened
       * the file name is : journal.csv
       */
      File dataFile = SD.open("journal.csv", FILE_WRITE);
      // we can write if the file can be open :
      if (dataFile) {
        dataFile.println(tolog);
        dataFile.close();
        //Sleepy::loseSomeTime(8);
      }
    }
    Sleepy::loseSomeTime(SLEEP_DURATION); // time to sleep
}

/**
* we build the string that we will write on sd Card
* the output will be a file line
* time,t1,t2,t3,t4,t5,t6,
*/
String builString()
{
  String dataString = buildTime();

  dataString += DELIMITER;

  sensors.requestTemperatures(); // asking temperatures
   /**
   * temperature1 is an array of first sensor adresse elements
   *
   * cf: https://git.io/vwM2x to scan addresses
   */

  dataString += String(sensors.getTempC(temperature1));
  dataString += DELIMITER;

  dataString += String(sensors.getTempC(temperature2));
  dataString += DELIMITER;

  dataString += String(sensors.getTempC(temperature3));
  dataString += DELIMITER;

  dataString += String(sensors.getTempC(temperature4));
  dataString += DELIMITER;

  dataString += String(sensors.getTempC(temperature5));
  dataString += DELIMITER;

  dataString += String(sensors.getTempC(temperature6));
  dataString += DELIMITER;

  return dataString;
}

/**
 * build a string with time
 */
String buildTime()
{
  String dateString = "";

    DateTime now = RTC.now();
    dateString += String(now.year());

    dateString += "-";
    int month = now.month();
    if(month<10)
    {
      dateString += "0";
    }
    dateString += month;

    dateString += "-";
    int day = now.day();
    if(day<10)
    {
      dateString += "0";
    }
    dateString += day;

    dateString += "T"; // need for spreadsheet time format
    int hour = now.hour();
    if(hour<10)
    {
      dateString += "0";
    }
    dateString += hour;

    dateString += ":";
    int minute = now.minute();
    if(minute<10)
    {
      dateString += "0";
    }
    dateString += minute;

    dateString += ":";
    int second = now.second();
    if(second<10)
    {
      dateString += "0";
    }
    dateString += second;

  return dateString;
}
