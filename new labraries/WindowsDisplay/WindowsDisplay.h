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

class WindowsDisplay
{
  private:
   int ROW_DIM;
   int COL_DIM;
   
  public:


  WindowsDisplay(int row, int col); // constructor, initialize the pin and display type
  
  void LoadWindows(time_t cur_t, int NROWS,long array[40][2]); // Initialize the SD and windows
  
  void windowpanel(SoftDMD dmd, time_t cur_t, int offs, int count,time_t w_open,time_t w_close);

  bool check_load_window(time_t cur_t,int NROWS, int current_window, long array[40][2],time_t w_open, time_t w_close);

  };
#endif