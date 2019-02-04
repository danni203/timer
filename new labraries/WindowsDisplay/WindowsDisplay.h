/*

 WindowsDisplay -  wrapper for LED windows display.

 License:

 Danyi wang
 21/01/2019
 */

#ifndef __WindowsDisplay_H__
#define __windowsDisplay_H__

#include "Arduino.h"
#include <DMD2.h>
#include <TimeLib.h>
#include <Time.h>

#define ROW_DIM 40
#define COL_DIM 2
class WindowsDisplay
{
  private:
   //int ROW_DIM ;
  // int COL_DIM ;
 
  public:
  
  int NROWS = 0;
  long array[ROW_DIM][COL_DIM];
  time_t w_open = 0;   // fudge factor to allow for compile time (seconds, YMMV)
  time_t w_close = 0;   // fudge factor to allow for compile time (seconds, YMMV)
  int current_window = 0;

  //WindowsDisplay(); // constructor, empty
  
  void LoadWindows(time_t cur_t); // Initialize the SD and windows
  
  void windowpanel(SoftDMD dmd, time_t cur_t, int offs, int count);

  bool check_load_window(time_t cur_t);

 
  };
#endif
