/**
 * DavidHeafHivesMonitor v2
 * Read/record temperatures of 6 sensors
 * DS18B20 using their addresses
 */
#define DEBUG   1 // set to 1 to trace activity via serial console
const char DELIMITER = ','; // value delimiter in file

#include <JeeLib.h>
#include <avr/sleep.h>

volatile bool adcDone;
ISR(WDT_vect) { Sleepy::watchdogEvent(); }
ISR(ADC_vect) { adcDone = true; }

#define SLEEP_DURATION 1000 * 1 // 1000ms * 30

/**
* Ceech board part
*/
int current = A6;
int cell = A2;
int lipo = A0;
int CHRG = A7;

float vout = 0.0;
float vin = 0.0;
float R1 = 47000.0; // resistance of R1
float R2 = 10000.0; // resistance of R2
int value = 0;



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
 * cf: https://github.com/Jodaille/DavidHeafHives/blob/master/Arduino/ScanDS18B20Addresses/ScanDS18B20Addresses.ino
 * to scan the addresses
 ex:
 adresse :	0x28, 0x42, 0x6B, 0x1C, 0x07, 0x00, 0x00, 0x34
 adresse :	0x28, 0xED, 0x0B, 0x2A, 0x07, 0x00, 0x00, 0x1C
 adresse :	0x28, 0xF7, 0x9D, 0x29, 0x07, 0x00, 0x00, 0x2B
 */

DeviceAddress temperature1 {0x28, 0xFF, 0xC8, 0x86, 0x90, 0x15, 0x01, 0xE1}; 
DeviceAddress temperature2 {0x28, 0xFF, 0xA8, 0x67, 0x90, 0x15, 0x01, 0x36}; 
DeviceAddress temperature3 {0x28, 0xFF, 0x76, 0x18, 0x90, 0x15, 0x04, 0xC7}; 
DeviceAddress temperature4 {0x28, 0xFF, 0x5D, 0x88, 0x90, 0x15, 0x01, 0x91}; 
DeviceAddress temperature5 {0x28, 0xFF, 0x4B, 0x1A, 0x90, 0x15, 0x04, 0xA8}; 
DeviceAddress temperature6 {0x28, 0xFF, 0x07, 0x19, 0x90, 0x15, 0x04, 0xE8}; 


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
#include <SD.h>

// Arduino Uno pin 4
// cf: https://www.arduino.cc/en/Reference/SPI
const int chipSelect = 4;

volatile bool sDisReady = false; // only try to write if sd is ready (detected)

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
      }
      else
      {
        sDisReady = false;
        #if DEBUG
          Serial.println("could not write file");   // send "file line" to computer serial
          Serial.flush();delay(5); // needed to flush serial when woke up
        #endif
      }
      
    }
    else
    {
      #if DEBUG
        Serial.println("Sd not ready");   // send "file line" to computer serial
        Serial.flush();delay(5); // needed to flush serial when woke up
      #endif

        if(SD.begin(chipSelect))
        {
          File dataFile = SD.open("journal.csv", FILE_WRITE);
          if (dataFile) {
            sDisReady = true;
          }
        }
        else
        {
          sDisReady = false;
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
  //dataString = ""; // clear the string to remove time for serial tracer

  sensors.requestTemperatures(); // asking temperatures
   /**
   * temperature1 is an array of first sensor adresse elements
   *
   * cf: https://git.io/vwM2x to scan addresses
   */
  float t1 = sensors.getTempC(temperature1);
  float t2 = sensors.getTempC(temperature2);// - 0.98;
  float t3 = sensors.getTempC(temperature3);// + 0.33;
  float t4 = sensors.getTempC(temperature4);
  float t5 = sensors.getTempC(temperature5);// +0.16;
  float t6 = sensors.getTempC(temperature6);// -1.25;
  
  float average = (t1 + t2 + t3 + t4 + t5 + t6) / 6;

  dataString += String(t1);
  dataString += DELIMITER;

  dataString += String(t2);
  dataString += DELIMITER;

  dataString += String(t3);
  dataString += DELIMITER;

  dataString += String(t4);
  dataString += DELIMITER;

  dataString += String(t5);
  dataString += DELIMITER;

  dataString += String(t6);
  dataString += DELIMITER;

  /*dataString += String(average);
  dataString += DELIMITER;*/

  // battery voltage logging
  float vcc = readVcc();
  dataString += String(getBatteryVoltage(vcc));
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

float getBatteryVoltage(float vcc)
{
  float battery = ( analogRead(lipo) * vcc / 1024 ) * 2; // measuring battery voltage
  return battery;
}

float getChargeCurrent(float vcc)
{
   // convert the ADC value to miliamps

  float chargeCurrent = ((analogRead(current) * vcc / 1024 ) *250) / 3.3;
  return chargeCurrent;
}

float readVcc() 
{
  signed long resultVcc;
  float resultVccFloat;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10);                           // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                 // Convert
  while (bit_is_set(ADCSRA,ADSC));
  resultVcc = ADCL;
  resultVcc |= ADCH<<8;
  resultVcc = 1126400L / resultVcc;    // Back-calculate AVcc in mV
  resultVccFloat = (float) resultVcc / 1000.0; // Convert to Float

  return resultVccFloat;
}
