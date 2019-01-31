#include "Arduino.h"
#include <TimeLib.h>
#include <Time.h>
#include <DS3232RTC.h>
#include <ProcessTime.h>

void ProcessTime:: WritebatterybackedTZoffset(long offs) {
  long ltz_mins = offs / 60;
  int tz_mins = ltz_mins;
  RTC.writeRTC(0x09, highByte(tz_mins));
  RTC.writeRTC(0x0A, lowByte(tz_mins));
}

long ProcessTime::ReadbatterybackedTZoffset(void) {
  int tz_mins = (RTC.readRTC(0x09) << 8) | (RTC.readRTC(0x0A));
  return (60 * (long)tz_mins);
}

void ProcessTime:: processSyncMessage(long tz_offs, time_t cur_t) {

  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
  char inbyte = Serial.read();
  if (inbyte == 'T') {
    unsigned long stime = (unsigned long)Serial.parseInt();
    if ( stime >= DEFAULT_TIME) // check the integer is a valid time (greater than Jan 1 2013)
    {
      setTime(stime); // Sync Arduino clock to the time received on the serial port
      Serial.println("Posix Time Change");
    }
    else Serial.println("Posix Invalid Time");
  }
  else if (inbyte == 'Z') { //(Serial.find('Z')) {
    //   char inbyte = Serial.read();
    long zone = Serial.parseInt();
    if (zone < 60 * 12 && zone > -60 * 12) {
      tz_offs =  (zone * 60);
      Serial.print("Time Zone Change "); Serial.print(tz_offs); Serial.print("  "); Serial.println(zone);
    }
    else Serial.println("Invalid Time Zone");
  }
  else if (inbyte == 'S') {
    // char inbyte = Serial.read();
    int command = Serial.parseInt();
    if (command == 0) {
      Serial.println("Nonvolatile Save Settings");
      RTC.set(cur_t);
      WritebatterybackedTZoffset(tz_offs);
    }
    else Serial.println("Invalid Save Settings");
  }
}

void ProcessTime::printDateTime(time_t utc, time_t local)
{
  char buf[30];
  char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)

  time_t t = local;
  strcpy(m, monthShortStr(month(t)));
  sprintf(buf, "L = %.2d:%.2d:%.2d %s %.2d %s %d %ld min | ",
          hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), (local - utc) / 60);
  Serial.print(buf);

  t = utc;
  strcpy(m, monthShortStr(month(t)));
  sprintf(buf, "Z = %.2d:%.2d:%.2d %s %.2d %s %d",
          hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t));
  Serial.println(buf);
  //  Serial.print(windows.array[0][0]); Serial.print( ); Serial.println((long)utc);
}
