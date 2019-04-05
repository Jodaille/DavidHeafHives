/**
*
*/
#define DEBUG   1
#include "DHMonitor.h"
#include <Arduino.h> // need for pinouts constants at least
#include <string.h>

DHMonitor::DHMonitor() : interval(1000), BatteryPin(A3), voltageDividerR1(1000000.0), voltageDividerR2(1000000.0), voltageDividerTuning(1.01389)
{
  SampleIntervalMinutes = 1;// One minute record
}

bool DHMonitor::initSdCard(int chipSelect)
{
  bool sDisReady;
  // Setting the SPI pins high helps some sd cards go into sleep mode
  // the following pullup resistors only needs to be enabled for the ProMini builds - not the UNO loggers
  pinMode(chipSelect, OUTPUT); digitalWrite(chipSelect, HIGH); //ALWAYS pullup the ChipSelect pin with the SD library
  //and you may need to pullup MOSI/MISO, usually MOSIpin=11, and MISOpin=12 if you do not already have hardware pulls
  pinMode(11, OUTPUT);digitalWrite(11, HIGH); //pullup the MOSI pin on the SD card module
  pinMode(12, INPUT_PULLUP); //pullup the MISO pin on the SD card module
  delay(5);
  if (SD.begin(chipSelect,SPI_FULL_SPEED)) {
    sDisReady = true;
  }
  else {
    Serial.println(F("Card failed, or not present"));
    Serial.flush();
  }
  return sDisReady;
}

bool DHMonitor::writeLine(String tolog)
{
  #if DEBUG
    Serial.println("write file");Serial.flush();delay(5);
  #endif
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

void DHMonitor::setInterval(DateTime now)
{
    //============Get the next alarm time =============
    Alarmhour = now.hour();
    Alarmminute = now.minute() + SampleIntervalMinutes;
    Alarmday = now.day();
    // check for roll-overs
    if (Alarmminute > 59) { //error catching the 60 rollover!
      Alarmminute = 0;
      Alarmhour = Alarmhour + 1;
      if (Alarmhour > 23) {
        Alarmhour = 0;
      }
    }
    Serial.print(F("alarm d:"));Serial.print(Alarmday);Serial.print(F(" h:"));Serial.print(Alarmhour);Serial.print(F(" m:"));Serial.print(Alarmminute);  
    Serial.println();Serial.flush();
}
void DHMonitor::disableAlarm()
{
  if (RTC.checkIfAlarm(1)) {       //Is the RTC alarm still on?    
    RTC.turnOffAlarm(1);              //then turn it off.
  }
}

void DHMonitor::setNextWakeUp()
{
  #if DEBUG
    Serial.println("DHMonitor::setNextWakeUp");
    Serial.print("Alarmhour, Alarmminute: ");Serial.print(Alarmhour);Serial.print(Alarmminute);Serial.println();
    Serial.flush();
  #endif
  // then set the alarm
  RTC.setAlarm1Simple(Alarmhour, Alarmminute);
  RTC.turnOnAlarm(1);
  if (RTC.checkAlarmEnabled(1)) {
    #ifdef ECHO_TO_SERIAL
      Serial.print(F("RTC Alarm Enabled!"));
      Serial.print(F(" Going to sleep for : "));
      Serial.print(SampleIntervalMinutes);
      Serial.println(F(" minute(s)"));
      Serial.println();
      Serial.flush();//adds a carriage return & waits for buffer to empty
    #endif
  }
}

String DHMonitor::getTime()
{
    String dateString = "";
    DateTime now = RTC.now();
    setInterval(now);
    dateString += String(now.year());

    dateString += "-";
    int month = now.month();
    if (month<10) {
      dateString += "0";
    }
    dateString += month;

    dateString += "-";
    int day = now.day();
    if (day<10) {
      dateString += "0";
    }
    dateString += day;

    dateString += "T"; // need for spreadsheet time format
    int hour = now.hour();
    if (hour<10) {
      dateString += "0";
    }
    dateString += hour;

    dateString += ":";
    int minute = now.minute();
    if (minute<10) {
      dateString += "0";
    }
    dateString += minute;

    dateString += ":";
    int second = now.second();
    if (second<10) {
      dateString += "0";
    }
    dateString += second;

  return dateString;
}

int DHMonitor::readBatteryVoltage()
{
    float preSDsaveBatterycheck = 9999;
    float floatbuffer           = 9999.9;
    float dividerRatio          = (voltageDividerR1 + voltageDividerR2) / voltageDividerR2;
    analogReference(DEFAULT);
    analogRead(BatteryPin);delay(5);  //throw away the first reading, high impedance divider!
    floatbuffer = analogRead(BatteryPin);
    //floatbuffer = (floatbuffer+0.5)*(3.3/1024.0)*4.030303; // 4.0303 = (Rhigh+Rlow)/Rlow for 10M/3.3M resistor combination
    floatbuffer = (floatbuffer)*(3.3/1024.0) * dividerRatio * voltageDividerTuning;
    preSDsaveBatterycheck=int(floatbuffer*1000.0);
    return preSDsaveBatterycheck;
}

float DHMonitor::readVcc()
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
