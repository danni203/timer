/*

 ProcessTime -  wrapper for set RTC and Sync messages.

 License:

 Danyi wang
 29/01/2019
 */

 #ifndef __ProcessTime_H__
#define __ProcessTime_H__

#include "Arduino.h"
#include <TimeLib.h>
#include <Time.h>

class ProcessTime
{
  public:
  //ProcessTime();
  void WritebatterybackedTZoffset(long offs);
  long ReadbatterybackedTZoffset(void);
  void processSyncMessage(long offs, time_t cur_t);
  void printDateTime(time_t utc, time_t local);
  };
#endif
