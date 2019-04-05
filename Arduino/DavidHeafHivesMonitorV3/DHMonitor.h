/**
* DHMonitor.h
*/
/**
 * We are using the library RTClib
 * cf: https://github.com/adafruit/RTClib/archive/master.zip
 */
#include "RTClib.h"
#include "SdFat.h"

class DHMonitor
{

    public:
        DHMonitor();
        void setNextWakeUp();
        void disableAlarm();
        bool initSdCard(int chipSelect);
        bool writeLine(String tolog);
        String getTime();
        int readBatteryVoltage();
        float readVcc();

    private:
        bool sDisReady;
        unsigned long interval;
        unsigned long previousMillis;
        int BatteryPin;
        float voltageDividerR1;
        float voltageDividerR2;
        float voltageDividerTuning;
        RTC_DS3231 RTC;
        void setInterval(DateTime now);
        unsigned int SampleIntervalMinutes;
        byte Alarmhour;
        byte Alarmminute;
        byte Alarmday;
        SdFat SD;
};
