/**
 * Tiny RTC module
 * module horloge
 *
 * DS1307 pour bus I2C
 * avec batterie au lithium CR1225
 *
 * Le port I2C de l'Arduino est situé
 * sur les pin A4 et A5
 *
 * Analog pin A5 <-> SCL
 * Analog pin A4 <-> SDA
 */

#include <Wire.h>
/**
 * Nous utilisons la librairie RTClib
 * cf: https://github.com/adafruit/RTClib/archive/master.zip
 * à installer dans le répertoire libraries
 * du répertoire de sketchs
 */
#include "RTClib.h"
RTC_DS1307 RTC;

// pour ajuster l'heure changer en : true
bool definirHeure = true;

void setup () {
    Serial.begin(9600);
    Serial.println("start");
    Wire.begin();
    Serial.println("after wire");
    RTC.begin();
    Serial.println("after rtc");
    
  if (! RTC.isrunning()) {
    Serial.println("RTC ne fonctionne pas!");
  }
Serial.println(" rtc running");
  if(definirHeure)
  {
      // la ligne suivante permet d'ajuster la date & l'heure
      // à la date de compilation du sketc
      DateTime current = DateTime(__DATE__, __TIME__);
      long unix = current.unixtime();
      long UTC = unix - 60*60;
      Serial.print("unix: ");Serial.println(unix);
      Serial.print("utc:  ");Serial.println(UTC);
      //RTC.adjust(DateTime(__DATE__, __TIME__));
      RTC.adjust(DateTime(UTC));

  }

}
void loop () {

    String temps = buildTime();
    Serial.println(temps);
    delay(1000);
}

String buildTime()
{
  DateTime now = RTC.now();
  String dateString = String(now.year());

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

  dateString += " ";
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
