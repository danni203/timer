#include "Arduino.h"
//#include <fonts/SystemFont5x7.h>
//#include <fonts/Arial14.h>
#include <DS3232RTC.h>
#include <DMD2.h>
#include <stdio.h>
#include <SPI.h>
#include <WindowsDisplay.h>
#include <SdFat.h>
#include <TimeLib.h>
#include <Time.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial14.h>




const uint8_t SOFT_MISO_PIN = 29;
const uint8_t SOFT_MOSI_PIN = 23;
const uint8_t SOFT_SCK_PIN  = 27;
const uint8_t CS_PIN = 25; 
SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> SD;
File file;

/*
WindowsDisplay :: WindowsDisplay()
{
  
 
}
*/
size_t readField(File* file, char* str, size_t size, const char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
      break;
    }
  }
  str[n] = '\0';
  return n;
}
void WindowsDisplay:: LoadWindows(time_t cur_t)
{
  // Initialize the SD.
  if (SD.begin(CS_PIN)) {
    file = SD.open("windows.csv", FILE_READ);
    int i = 0;     // First array index.
    int j = 0;     // Second array index
    size_t n;      // Length of returned field with delimiter.
    char str[20];  // Must hold longest field with delimiter and zero byte.
    char *ptr;     // Test for valid field.

    int fail = 0;
    for (i = 0; i < ROW_DIM; ) {
      for (j = 0; j < COL_DIM; j++) {
        n = readField(&file, str, sizeof(str), ",\n");
        if (n == 0) {
          fail = 1; //read fail
          break;
        }
        else {
          array[i][j] = strtol(str, &ptr, 10);
          if (ptr == str) {
            fail = 2; //not a number
            break;
          }
          while (*ptr == ' ') {
            ptr++;
          }
          if (*ptr != ',' && *ptr != '\n' && *ptr != '\0') {
            fail = 3; //not valid character
            break;
          }
          if (j < (COL_DIM - 1) && str[n - 1] != ',') {
            fail = 4; //wrong format
            break;
          }
        }
      }
      if (fail != 0) break;
      //   i++;
      if (array[i][1] > cur_t) i++; /*window is not past*/
      else Serial.print("skip past window "); Serial.println(i);
    }

    /*how many valid rows?*/
    if (fail != 0) {
      if (i > 0 && j == 0) NROWS = i;
      else if (i > 0 && j == 1) NROWS = i - 1;
      else if (i > 0) NROWS = i;
      else NROWS = 0;
      Serial.print("Window load error: "); Serial.println(fail);
      //    errorHalt("Error fail");
    }
    else NROWS = i;
    Serial.print(i); Serial.print(' '); Serial.print(j); Serial.print(' ' ); Serial.println(NROWS);

    // Print the array.
    for (i = 0; i < NROWS; i++) {
      Serial.print(array[i][0]); Serial.print(' '); Serial.print(array[i][1]);
      if (array[i][1] < cur_t) Serial.println('P');
      else Serial.println(' ');

    }
    Serial.println("Done");
    file.close();
  }

  else {
    Serial.println("No SD Card");
  }
}


void WindowsDisplay::windowpanel(SoftDMD dmd,time_t cur_t, int offs, int count) {
  char buf1[12], buf2[12];
  long t = cur_t;
  bool in_window = false;
  bool colon = false;

  if (w_open != 0 && w_close != 0 && w_open < w_close && w_close >= cur_t) //valid window
  {
    if (cur_t <= w_open) /*window not open*/
    {
      dmd.drawFilledBox(17 + offs, 0, 20 + offs, 7, GRAPHICS_OFF) ; //clear the mid character area indicator
      in_window = false;
      t =w_open - cur_t;
      if (t >= 3600) /*more than an hour*/
      {
        sprintf(buf1, "%02d", hour(t));
        sprintf(buf2, "%02d", minute(t));
        colon = false;
      }
      else /*needs a colon*/
      {
        sprintf(buf1, "%02d", minute(t));
        sprintf(buf2, "%02d", second(t));
        colon = true;
      }
    }
    else if (cur_t <= w_close)
    {
      in_window = true;
      t = w_close - t;
      if (t >= 3600)
      {
        sprintf(buf1, "%2.d", hour(t));
        sprintf(buf2, "%02d", minute(t) + (second(t) > 29 ? 1 : 0));
        colon = false;
      }
      else
      {
        sprintf(buf1, "%02d", minute(t));
        sprintf(buf2, "%02d", second(t));
        colon = true;
      }
    }
    if (!in_window)     /*draw time to wait, or time to go*/
    {
      dmd.drawString(6 + offs, 0, String(buf1));
      dmd.drawString(21 + offs, 0, String(buf2));
      /*clear useless areas*/
      dmd.drawFilledBox(offs + 1, 0, 5 + offs, 6, GRAPHICS_OFF);
      dmd.drawFilledBox(offs + 1, 7, offs + 31, 8, GRAPHICS_OFF);
      dmd.drawFilledBox(offs + 1, 0, offs + 4, 4, GRAPHICS_OFF);
      /*draw our t- logo*/
      dmd.drawLine(offs + 2, 0, offs + 2, 3);
      dmd.drawLine(offs + 1, 0, offs + 3, 0);
      dmd.drawLine(offs + 1, 5, offs + 3, 5);

      if (colon) {
        dmd.drawFilledBox(17 + offs, 0, 20 + offs, 6, GRAPHICS_OFF) ; //clear the colon
        dmd.drawFilledBox(18 + offs, 1, 19 + offs, 2, GRAPHICS_ON); // 2, count % 10 < 5 ? GRAPHICS_ON : GRAPHICS_OFF) ; //clear the window indicator
        dmd.drawFilledBox(18 + offs, 4, 19 + offs, 5, GRAPHICS_ON); // 5, count % 10 < 5 ? GRAPHICS_ON : GRAPHICS_OFF) ; //clear the window indicator
      }
      /*draw window open time*/
      t = w_close - w_open;
      if (t >= 360000) /*100 hours*/
      {
        sprintf(buf1, "%2.d", hour(t));
        dmd.drawString(1 + offs, 9, String(buf1));
        sprintf(buf1, "%02d", minute(t) + (second(t) > 29 ? 1 : 0));
        //         dmd.drawString(12+offs, 8, String('h'),GRAPHICS_OR);
        dmd.drawLine(13 + offs, 15, 13 + offs, 11, GRAPHICS_OR); //h
        dmd.drawLine(15 + offs, 15, 15 + offs, 13, GRAPHICS_OR);
        dmd.drawLine(15 + offs, 13, 13 + offs, 13, GRAPHICS_OR);

        dmd.drawString(17 + offs, 9, String(buf1));
        dmd.drawString(28 + offs, 9, String('\''), GRAPHICS_OR);
      }
      else if (t > 300)
      {
        sprintf(buf1, "%2.d", hour(t) * 60 + minute(t) + (second(t) > 29 ? 1 : 0));
        dmd.drawString(1 + offs, 9, String(buf1));
        dmd.drawString(14 + offs, 9, String("m"));
        if (second(t) != 0) {
          sprintf(buf1, "%2.d", second(t));
          dmd.drawString(20 + offs, 9, String(buf1));
        }
        else dmd.drawString(20 + offs, 9, String("  "));

      }
      else {
        sprintf(buf1, "%3.d", t);
        dmd.drawString(1 + offs, 9, String(buf1));
        dmd.drawString(19 + offs, 9, String(" s"));
      }
      /*second indicator*/
      {
        t = w_open - cur_t;
        dmd.drawFilledBox(offs, 0, offs, 15, GRAPHICS_OFF);
        int col = 0;
        dmd.setPixel(offs + col, 15 - second(t) / 4, GRAPHICS_ON); //count % 2 ? GRAPHICS_OFF : GRAPHICS_ON);
      }
    }
    else /*we are in a window*/
    {
      /*------------------------------------------------------*/
      //Need some logic to remove the flicker and the left over rubbish
      /*------------------------------------------------------*/
      dmd.drawFilledBox(offs, 0, 32 + offs, 15, GRAPHICS_OFF) ; //clear the right window

      dmd.selectFont(Arial14);

      /*draw time to wait, or time to go*/
      dmd.drawString(1 + offs, 2, String(buf1));
      dmd.drawString(18 + offs, 2, String(buf2));
      if (colon ) {
        dmd.drawFilledBox(15 + offs, 4, 16 + offs, 5, GRAPHICS_ON ) ; //clear the window indicator
        dmd.drawFilledBox(15 + offs, 10, 16 + offs, 11, GRAPHICS_ON ) ; //clear the window indicator
      }
      dmd.selectFont(System5x7);
    }
  }
}


bool WindowsDisplay::check_load_window(time_t cur_t)
{
  time_t t;
  t = cur_t;
  int x;
  for (x = current_window; x < NROWS; x++) {
    if ( t < array[x][0] || t < array[x][1] ) {
      w_open = array[x][0];
      w_close = array[x][1];
      return (true);
    }
  }
  //  Serial.println("No Valid Windows");
  return (false);
}
