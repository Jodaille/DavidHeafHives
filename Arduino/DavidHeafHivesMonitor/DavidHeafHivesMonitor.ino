/**
 * Lecture de température avec trois
 * capteurs DS18B20 par leur adresse
 */
#define DEBUG   0 // set to 1 to trace activity via serial console
const char DELIMITER = ',';

#include <JeeLib.h>
#include <avr/sleep.h>

volatile bool adcDone;
ISR(WDT_vect) { Sleepy::watchdogEvent(); }
ISR(ADC_vect) { adcDone = true; }

#define SLEEP_DURATION 1000 * 30 // 1000ms * 30

/**
 *  Nous utilisons la librairie OneWire
 *  Elle doit être présente dans le répertoire libraries
 *  situé dans le répertoire des croquis/sketchs
 *  voir dans le menu Préférences
 *  cf: https://github.com/PaulStoffregen/OneWire/archive/master.zip
 */
#include <OneWire.h>

/**
 * Nous utilisons aussi la librairie DallasTemperature
 * cf: https://github.com/milesburton/Arduino-Temperature-Control-Library
 */
#include <DallasTemperature.h>

/*
 *  Le fil des données (jaune?) est relié au pin 8 (D8) du Arduino
 *  une resistance de 4,7kOhm est placée en Vcc et D2
 */

#define ONE_WIRE_BUS 8

/**
 * Création d'une instance oneWire afin de pouvoir
 * communiquer avec des périphériques OneWire
 * Nb: pas seulement des capteurs Maxim/Dallas
 */
OneWire oneWire(ONE_WIRE_BUS);

/**
 * Nous passons notre instance oneWire à notre
 * objet sensors utilisant DallasTemperature
 */
DallasTemperature sensors(&oneWire);

/**
 * Adresses du capteur, cf: https://git.io/vwM2x
 * pour scanner l'adresse de votre capteur
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
#include <SD.h>

// Arduino Uno pin 4
// cf: https://www.arduino.cc/en/Reference/SPI
const int chipSelect = 4;

bool sDisReady = false;

/**
 * Tiny RTC module
 *
 * DS1307 pour bus I2C
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
  // Initialisation du port série
  #if DEBUG
    Serial.begin(9600);
    Serial.println("start");
  #endif


  if(SD.begin(chipSelect))
  {
    sDisReady = true;
  }
  // Initialisation de notre librairie
  sensors.begin();
}


void loop(void)
{

    String tolog = builString();
  #if DEBUG
    Serial.println(tolog);
    Serial.flush();delay(5);
  #endif

    if (sDisReady) {
      /**
       * nous ouvrons le fichier
       * Nb: un seul fichier peut être ouvert à la fois
       * le fichier se nomme : journal.csv
       */
      File dataFile = SD.open("journal.csv", FILE_WRITE);
      // si le fichier est disponible, nous écrivons dedans :
      if (dataFile) {
        dataFile.println(tolog);
        dataFile.close();
        //Sleepy::loseSomeTime(8);
      }
    }
    Sleepy::loseSomeTime(SLEEP_DURATION);
}

String builString()
{
  String dataString = buildTime();

  dataString += DELIMITER;

  sensors.requestTemperatures(); // envoi de la demande
   /**
   * temperature1 est un tableau contenant l'adresse
   * de notre capteur.
   * cf: https://git.io/vwM2x pour scanner les adresses
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

    dateString += "T";
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
