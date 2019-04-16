/**
 * DavidHeafHivesMonitor v3
 * Read/record temperatures of 6 sensors
 * DS18B20 using their addresses
 * 
 * try to refactor DavidHeafHivesMonitor V2
 * using tricks from The Cave Pearl Project:
 * https://thecavepearlproject.org
 * 
 * Arduino Pro mini 3.3V@8MHz in deep sleep, wake up by RTC
 * every minutes to read sensors and write to sdCard
 * 
 * cf: https://github.com/EKMallon/ProMiniDatalogger-BasicStarterSketch
 */
/*
  Estimations batteries AA in deep sleep
  1000mAh / 0.4mA  = 2500h
  => 104 days
  1000mAh / 0.31 = 3225h
  => 134 days
  
*/

#define DEBUG   1 // set to 1 to trace activity via serial console
const char DELIMITER = ','; // value delimiter in file

#include <HX711_ADC.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"

//HX711 constructor (dout pin, sck pin):
HX711_ADC LoadCell(5, 4);
long t;// keep last loop millis()

float calValue; // calibration value
long tareOffset;

volatile int updateStatus;

// structure of the saved datas to keep in EEPROM
struct config_t {
  float calValue;  // calibration
  long  tareOffset;// offset
  byte  Saved;     // set to 111 once first saved
} configuration;

void EEPROMLoad() {
  EEPROM_readAnything(0, configuration);
  if (configuration.Saved != 111) {
    #if DEBUG
      Serial.println("no saved values in EEPROM yet");
    #endif
    return;// we have no values yet skipping
  }
  calValue   = configuration.calValue;
  tareOffset = configuration.tareOffset;
  #if DEBUG
    Serial.print("calValue: ");Serial.println(calValue);
    Serial.print("tareOffset eeprom: ");Serial.println(tareOffset);
  #endif
}

void EEPROMSave() {
  configuration.calValue   = calValue;
  configuration.tareOffset = tareOffset;
  configuration.Saved      = 111;
  EEPROM_writeAnything(0, configuration);
}
/*******/


#include "DHMonitor.h"
#include <LowPower.h>   // https://github.com/rocketscream/Low-Power //for low power sleeping between readings

#define RTC_power_pin 9
DHMonitor monitor;

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

DeviceAddress temperature1 {0x28, 0xFF, 0xE7, 0x30, 0xB3, 0x16, 0x04, 0xF4};
//DeviceAddress temperature2 {0x28, 0x20, 0x7B, 0x08, 0x00, 0x00, 0x80, 0x25};
//DeviceAddress temperature3 {0x28, 0xDE, 0x61, 0x08, 0x00, 0x00, 0x80, 0x85};
//DeviceAddress temperature4 {0x28, 0xBE, 0xCB, 0x27, 0x00, 0x00, 0x80, 0x22};
//DeviceAddress temperature5 {0x28, 0x59, 0x69, 0x08, 0x00, 0x00, 0x80, 0xD4};
//DeviceAddress temperature6 {0x28, 0x59, 0x75, 0x08, 0x00, 0x00, 0x80, 0x89};
//DeviceAddress temperature7 {0x28, 0x25, 0x75, 0x08, 0x00, 0x00, 0x80, 0x6C};
//DeviceAddress temperature8 {0x28, 0x83, 0x7C, 0x08, 0x00, 0x00, 0x80, 0x71};
//DeviceAddress temperature9 {0x28, 0xFF, 0x23, 0x6B, 0x87, 0x16, 0x05, 0x9B};



/**
 * Writing on SD card
 *
 * SD card uses SPI bus:
 * MOSI       - pin 11
 * MISO       - pin 12
 * CLK or SCK - pin 13
 * CS         - pin 10
 *
 * SPI for Serial Peripheral Interface
 *
 * created  24 Nov 2010
 * modified 9 Apr 2012
 * by Tom Igoe
 * cf: https://www.arduino.cc/en/Tutorial/Datalogger
 */
#include <SPI.h>


// Arduino Pro mini pin 10
// cf: https://www.arduino.cc/en/Reference/SPI
const int chipSelect = 10;

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
// variables for reading the RTC time & handling the INT(0) interrupt it generates
#define DS3231_I2C_ADDRESS 0x68
#define DS3231_CONTROL_REG 0x0E
#define RTC_INTERRUPT_PIN 2

volatile boolean clockInterrupt = false;  //this flag is set to true when the RTC interrupt handler is executed
volatile unsigned int countReading = 0;
volatile float weight;
void setup(void)
{
  // Serial port initialisation (to communicate with computer)
  #if DEBUG
    Serial.begin(115200);
    Serial.println("start");
  #endif

  calValue = 20.4; // calibration value
  EEPROMLoad();    // load previous saved values from EEPROM
  LoadCell.begin();
  long stabilisingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilising time
  LoadCell.start(stabilisingtime);
  if(LoadCell.getTareTimeoutFlag()) {
    Serial.println("Tare timeout, check MCU>HX711 wiring and pin designations");
  }
  else {
    LoadCell.setCalFactor(calValue); // set calibration value (float)
    Serial.println("Startup + tare is complete");
  }
  updateStatus = LoadCell.update();
  LoadCell.setTareOffset(tareOffset);

  pinMode(RTC_power_pin, OUTPUT);
  digitalWrite(RTC_power_pin, LOW);

  Serial.print("readBatteryVoltage: ");
  Serial.println(monitor.readBatteryVoltage());

  Serial.print("getTime: ");
  Serial.println(monitor.getTime());Serial.print("END getTime: ");

  pinMode(RTC_INTERRUPT_PIN,INPUT_PULLUP);// RTC alarms low, so need pullup on the D2 line
  clearClockTrigger();
  monitor.disableAlarm();
  enableRTCAlarmsonBackupBattery(); // this is only needed if you cut the VCC pin supply on the DS3231

  sDisReady = monitor.initSdCard(chipSelect);

  delay(5);
  Serial.println("Sensors initialisation");
  // sensors initialisation
  sensors.begin();
  delay(5);
}


void loop(void)
{
    // We just wake up : time to work
    if (clockInterrupt) {
      digitalWrite(RTC_power_pin, HIGH);// RTC seems to work without power ?
      monitor.disableAlarm();
      clockInterrupt = false;                //reset the interrupt flag to false
    }
    int isUpdated = LoadCell.update();
    if (isUpdated) {
      countReading++;
      weight = LoadCell.getData();
      #if DEBUG
        Serial.print("countReading:");Serial.print(countReading);
        Serial.print(" index:");Serial.print(LoadCell.getReadIndex());
        Serial.print(" w:");Serial.println(weight);Serial.flush();
      #endif
    }

    if ( countReading > (SAMPLES -1) ) {
      countReading = 0;

      String tolog = buildString();
      #if DEBUG
        Serial.println(tolog);   // send "file line" to computer serial
        Serial.flush();delay(5); // needed to flush serial when woke up
      #endif

      if (sDisReady) {
        monitor.writeLine(tolog);
      } else {
        #if DEBUG
          Serial.println("could not write log");   // send "file line" to computer serial
          Serial.flush();delay(5); // needed to flush serial when woke up
        #endif
      }
      monitor.setNextWakeUp(); // create alarm that will wake up us
  
      //——– sleep and wait for next RTC alarm ————–
      // Enable interrupt on pin2 & attach it to rtcISR function:
      attachInterrupt(0, rtcISR, LOW);
      digitalWrite(RTC_power_pin, LOW);
      LoadCell.powerDown();
      // Enter power down state with ADC module disabled to save power:
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_ON);
    }/* else {
      #if DEBUG
        Serial.print("loadcell isUpdated:");Serial.println(isUpdated);
        Serial.flush();delay(5); // needed to flush serial when woke up
      #endif
    }*/
    //processor starts HERE AFTER THE RTC ALARM WAKES IT UP
    detachInterrupt(0); // immediately disable the interrupt on waking
    //Interrupt woke processor, now go back to the start of the main loop
    LoadCell.powerUp();
}

// This is the Interrupt subroutine that only executes when the RTC alarm goes off:
void rtcISR() {
    clockInterrupt = true;
}
//====================================================================================
// Enable Battery-Backed Square-Wave Enable on the RTC module:
/* Bit 6 (Battery-Backed Square-Wave Enable) of DS3231_CONTROL_REG 0x0E, can be set to 1
 * When set to 1, it forces the wake-up alarms to occur when running the RTC from the back up battery alone.
 * [note: This bit is usually disabled (logic 0) when power is FIRST applied]
 */
void enableRTCAlarmsonBackupBattery(){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);// Attention RTC
  Wire.write(DS3231_CONTROL_REG);            // move the memory pointer to CONTROL_REG
  Wire.endTransmission();                    // complete the ‘move memory pointer’ transaction
  Wire.requestFrom(DS3231_I2C_ADDRESS,1);    // request data from register
  byte resisterData = Wire.read();           // byte from registerAddress
  bitSet(resisterData, 6);                   // Change bit 6 to a 1 to enable
  Wire.beginTransmission(DS3231_I2C_ADDRESS);// Attention RTC
  Wire.write(DS3231_CONTROL_REG);            // target the register
  Wire.write(resisterData);                  // put changed byte back into CONTROL_REG
  Wire.endTransmission();
}
void clearClockTrigger()   // from http://forum.arduino.cc/index.php?topic=109062.0
{
  Wire.beginTransmission(0x68);   //Tell devices on the bus we are talking to the DS3231
  Wire.write(0x0F);               //Tell the device which address we want to read or write
  Wire.endTransmission();         //Before you can write to and clear the alarm flag you have to read the flag first!
  Wire.requestFrom(0x68,1);       //Read one byte
  byte bytebuffer1=Wire.read();   //In this example we are not interest in actually using the byte
  Wire.beginTransmission(0x68);   //Tell devices on the bus we are talking to the DS3231
  Wire.write(0x0F);               //Status Register: Bit 3: zero disables 32kHz, Bit 7: zero enables the main oscilator
  Wire.write(0b00000000);         //Bit1: zero clears Alarm 2 Flag (A2F), Bit 0: zero clears Alarm 1 Flag (A1F)
  Wire.endTransmission();
  clockInterrupt=false;           //Finally clear the flag we used to indicate the trigger occurred
}
/**
* we build the string that we will write on sd Card
* the output will be a file line
* time,t1,t2,t3,t4,t5,t6,
*/
String buildString()
{

  String dataString = buildTime();

  dataString += DELIMITER;
  //dataString = ""; // clear the string to remove time for serial tracer
  //LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
  sensors.requestTemperatures(); // asking temperatures
   /**
   * temperature1 is an array of first sensor adresse elements
   *
   * cf: https://git.io/vwM2x to scan addresses
   */
  float t1 = sensors.getTempC(temperature1);
//  float t2 = sensors.getTempC(temperature2);// - 0.98;
//  float t3 = sensors.getTempC(temperature3);// + 0.33;
//  float t4 = sensors.getTempC(temperature4);
//  float t5 = sensors.getTempC(temperature5);// +0.16;
//  float t6 = sensors.getTempC(temperature6);// -1.25;

  float average = t1;//(t1 + t2 + t3 + t4 + t5 + t6) / 6;

  dataString += String(t1);
  dataString += DELIMITER;

//  dataString += String(t2);
//  dataString += DELIMITER;
//
//  dataString += String(t3);
//  dataString += DELIMITER;
//
//  dataString += String(t4);
//  dataString += DELIMITER;
//
//  dataString += String(t5);
//  dataString += DELIMITER;
//
//  dataString += String(t6);
//  dataString += DELIMITER;

  /*dataString += String(average);
  dataString += DELIMITER;*/

  dataString += String(monitor.readBatteryVoltage());
  dataString += DELIMITER;
  // vcc  logging
  //float vcc = monitor.readVcc();
  dataString += String(monitor.readVcc());
  dataString += DELIMITER;

  LoadCell.update();
  float weight = LoadCell.getData();
  dataString += String(weight);
  dataString += DELIMITER; 
  return dataString;
}

/**
 * build a string with time
 */
String buildTime()
{
  String dateString = monitor.getTime();

  return dateString;
}
