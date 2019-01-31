#include <ProcessTime.h>

#include <WindowsDisplay.h>

#include "Arduino.h"
#include <SPI.h>
#include <DMD2.h>
#include <stdio.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial14.h>
#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <SdFat.h>

#if ENABLE_SOFTWARE_SPI_CLASS  // Must be set in SdFat/SdFatConfig.h

// #define DEBUG
// #define CS_PIN SS                                                            // tim


// Javaan, these are the external push buttons
const uint8_t EXT_BTN_1 = 38;
const uint8_t EXT_BTN_2 = 40;



#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define WINDOW_HEADER  "W"   // Header tag for serial time sync message
#define TIME_TXT_HEADER "S" // Header tag for a local time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message

// On Linux, you can use "date +T%s\n > /dev/ttyACM0" (UTC time zone)


enum window_state {WVALID, WINVALID, WALARM, WINSIDE, WCLOSEDALARM, WENDINGALARM, WBEGINALARM};
window_state win_state;

static int count = 0;

time_t cur_t = 0;
long tz_offs = 0; /*time zone offset in seconds*/
int dst = 0;

SoftDMD dmd(2, 1, 9, 6, 7, 8, 13, 11); // DMD controls the entire display
//SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> SD;

//File file;

#define ROW_DIM 40
#define COL_DIM 2
WindowsDisplay  WindowsDisplay(ROW_DIM, COL_DIM);
ProcessTime ProcessTime;
struct {
  int NROWS = 0;
  long array[ROW_DIM][COL_DIM];
  time_t w_open = 0;   // fudge factor to allow for compile time (seconds, YMMV)
  time_t w_close = 0;   // fudge factor to allow for compile time (seconds, YMMV)
  int current_window = 0;

} windows;


time_t utctolocal(time_t utc)
{
  return (utc + tz_offs + (dst == 0 ? 0 : 3600));
}

void setup()  {
  char buf[40];
  Serial.begin(9600);    // this is USB from PC
  Serial1.begin(9600);   // this is RxTx to AUDIO arduino

  // while (!Serial) ; // Needed for Leonardo only
  pinMode(13, OUTPUT);
  pinMode(EXT_BTN_1, INPUT_PULLUP);   // these are the external buttons
  pinMode(EXT_BTN_2, INPUT_PULLUP);   // these are the external buttons

  setSyncProvider(RTC.get);

  cur_t = now();

#ifdef DEBUG
  windows.array[0][0] = cur_t + 22; windows.array[0][1] = windows.array[0][0] + 20;
  windows.array[1][0] =  windows.array[0][1] + 18; windows.array[1][1] = windows.array[1][0] + 25;
  windows.array[2][0] = windows.array[0][2] + 14; windows.array[2][1] = windows.array[2][0] + 28;
  windows.NROWS = 3;
#else
  WindowsDisplay.LoadWindows(cur_t, windows.NROWS, windows.array);
#endif

  Serial1.print('A');

  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");

  Serial.println("Waiting for sync message: T<posix seconds UTC> Z<time zone rel UTC min> S0 save");
  Serial.println("On posix, you can use \"date +T\%s\\n > /dev/ttyACM0\" ");
  tz_offs = ProcessTime.ReadbatterybackedTZoffset(); /*it is stored in minutes, convert to seconds*/
  sprintf(buf, "Time Zone = %d min", tz_offs / 60);
  Serial.println(buf);

  dmd.selectFont(System5x7);
  dmd.setBrightness(20);
  dmd.begin();
  /*
    windows[0] = cur_t + 15; windows[1] = windows[0] + 15;
    windows[2] = windows[1] + 20; windows[3] = windows[2] + 35;
    windows[4] = windows[3] + 90; windows[5] = windows[4] + 90;
    windows[6] = windows[5] + 900; windows[7] = windows[6] + 900;
  */
  windows.w_open = windows.w_close = cur_t;
}

void timepanel(int offs, int count) {
  time_t  t = cur_t;
  char buffer[10];
  dmd.drawFilledBox(offs + 1, 7, offs + 31, 8, GRAPHICS_OFF); /*there is an unused area between the two rows, clear it*/

  /*Zulu time*/

  sprintf(buffer, "%.2d%", hour(t));
  dmd.drawString(1 + offs, 0, String(buffer));
  sprintf(buffer, "%.2d", minute(t));
  dmd.drawString(14 + offs, 0, String(buffer));

  dmd.drawFilledBox(offs + 25, 0, offs + 31, 6, GRAPHICS_OFF); /*near the symbols*/
  dmd.drawLine(28 + offs, 0, 31 + offs, 0, GRAPHICS_ON); //z
  dmd.drawLine(31 + offs, 0, 28 + offs, 3, GRAPHICS_ON);
  dmd.drawLine(28 + offs, 3, 31 + offs, 3, GRAPHICS_ON);

  /*local time*/
  t = utctolocal(cur_t); //ausCT.toLocal(cur_t, NULL);
  sprintf(buffer, "%.2d%", hour(t));
  dmd.drawString(1 + offs, 9, String(buffer));
  sprintf(buffer, "%.2d", minute(t));
  dmd.drawString(14 + offs, 9, String(buffer));
  /*L for local time*/
  dmd.drawLine(29 + offs, 12, 29 + offs, 15, GRAPHICS_ON); //L
  dmd.drawLine(29 + offs, 15, 31 + offs, 15, GRAPHICS_ON);
  if (dst) dmd.drawLine(31 + offs, 12, 31 + offs, 13, GRAPHICS_ON);//DST indicator
  /*second indicator*/
  {
    t = cur_t;
    dmd.drawFilledBox(offs, 0, offs, 15, GRAPHICS_OFF);
    int col = 0;
    dmd.setPixel(offs + col, 15 - second(t) / 4, GRAPHICS_ON); //count % 2 ? GRAPHICS_OFF : GRAPHICS_ON);
  }

  /*Window indicator*/
  dmd.drawFilledBox(25 + offs, 5, 31 + offs, 10, GRAPHICS_OFF); /*clear the icon*/
  switch (win_state) {
    case WVALID:/*flash large small filled boxes*/
      if (second(cur_t) % 2) dmd.drawFilledBox(26 + offs, 5, 31 + offs, 10, GRAPHICS_ON);
      else dmd.drawFilledBox(offs + 27, 6, offs + 30, 9, GRAPHICS_ON);
      break;
    case WBEGINALARM: /*flash open/closed window box*/
      if (second(cur_t) % 2) dmd.drawFilledBox(26 + offs, 5, 31 + offs, 10, GRAPHICS_ON);
      else dmd.drawBox(26 + offs, 5, 31 + offs, 10, GRAPHICS_ON);
      break;
    case WINSIDE:/*flash large small unfilled boxes*/
      if (second(cur_t) % 2) dmd.drawBox(26 + offs, 5, 31 + offs, 10, GRAPHICS_ON);
      else dmd.drawBox(offs + 27, 6, offs + 30, 9, GRAPHICS_ON);
      break;
    case WENDINGALARM:/*flash open/x icons*/
      if (second(cur_t) % 2) dmd.drawFilledBox(26 + offs, 5, 31 + offs, 10, GRAPHICS_ON);
      dmd.drawLine(26 + offs, 5, 31 + offs, 10, GRAPHICS_ON); //Big X
      dmd.drawLine(31 + offs, 5, 26 + offs, 10, GRAPHICS_ON);
      break;
    case WINVALID:
    case WCLOSEDALARM:/*show an x*/
      dmd.drawLine(26 + offs, 5, 31 + offs, 10, GRAPHICS_ON); //Big X
      dmd.drawLine(31 + offs, 5, 26 + offs, 10, GRAPHICS_ON);
      break;
  }
}

//#define ACCEL

void window_closed_alarm() {
  dmd.drawFilledBox(32, 0, 63, 15, GRAPHICS_ON ) ; /*flash*/
  //tone(2, 3000, 900);
  delay(1000);
  dmd.drawFilledBox(32, 0, 63, 15, GRAPHICS_OFF) ;
  Serial1.print('E');
  Serial.print('E');

}
void window_closing_alarm() {
  Serial1.print('C');
  Serial.print('C');

}

void window_opening_alarm() {
  Serial1.print('A');
  Serial.print('A');

  // Serial.print("OPEN "); Serial.println((char)(a+'0'));
}



window_state machine_state(void)
{
  window_state win_state;

  if (windows.w_open != 0 && windows.w_close != 0 && windows.w_open < windows.w_close && windows.w_close >= cur_t) {
    if (cur_t > windows.w_open) win_state = WINSIDE;
    else win_state = WVALID;
  }
  else win_state = WINVALID;

  if (win_state == WINSIDE && (windows.w_close - cur_t) < 10) {
    if (win_state == WINSIDE && windows.w_close == cur_t) win_state = WCLOSEDALARM;
    else win_state = WENDINGALARM;
  }
  else if (win_state == WVALID && (windows.w_open - cur_t) < 10) win_state = WBEGINALARM;
  return (win_state);
}


void loop() {
  int window_closed_flag = 0;
  static bool chk = WindowsDisplay.check_load_window(cur_t,windows.NROWS, windows.current_window, windows.array,windows.w_open, windows.w_close);

  while (Serial.available() > 0) ProcessTime.processSyncMessage(tz_offs, cur_t);



  // // Tim's button test                                                       // tim
  if (!digitalRead(EXT_BTN_1)){
    // want to increment window here
    // need to check for max windows...
    if (windows.current_window < windows.NROWS+1){
      window_opening_alarm();
      ++windows.current_window;
      Serial.print("  window incr. #");
    }
    else{
      window_closing_alarm();
      Serial.print("  MAX windows... #");
    }
    // write a big W + window number to LED panel
    dmd.drawFilledBox(36, 0, 32 + 32, 15, GRAPHICS_OFF) ; //clear the right window
    char buff20[12];
    sprintf(buff20, "W%02d", windows.current_window);
    dmd.drawString(40, 0, String(buff20));
    Serial.println(windows.current_window);
  }
  if (!digitalRead(EXT_BTN_2)){
    // want to decrement window here
    if (windows.current_window > 0){
      window_opening_alarm();
      --windows.current_window;
      Serial.print("  window decr. #");
    }
    else{
      window_closing_alarm();
      Serial.print("  MIN windows... #");
    }
    // write a big W + window number to LED panel
    dmd.drawFilledBox(36, 0, 32 + 32, 15, GRAPHICS_OFF) ; //clear the right window
    char buff20[12];
    sprintf(buff20, "W%02d", windows.current_window);
    dmd.drawString(40, 0, String(buff20));
    Serial.println(windows.current_window);
  }




  if (cur_t % 60 == 0) {
    ProcessTime.printDateTime(cur_t, utctolocal(cur_t));
    Serial.print("window #"); Serial.println(windows.current_window);
  }

  /*hang waiting for second to roll*/
  time_t p_time = cur_t;
  while (p_time == cur_t)
  {
    //    Check_Set_Message();
#ifdef ACCEL
    cur_t += 1;
#else
    delay(10);
    cur_t = now();
#endif
  }
  win_state = machine_state();

  timepanel(0, count);
  // int loop = 0;
  if ( win_state != WINVALID) WindowsDisplay.windowpanel(dmd, cur_t, 32, count,windows.w_open,windows.w_close);
  else timepanel(32, count);

  switch (win_state) {
    case WBEGINALARM: window_opening_alarm();
      break;
    case WENDINGALARM: window_closing_alarm();
      break;
    case WCLOSEDALARM: window_closed_alarm();
      break;
  }

  WindowsDisplay.check_load_window(cur_t,windows.NROWS, windows.current_window, windows.array,windows.w_open, windows.w_close);
  count++;

}

#else  // ENABLE_SOFTWARE_SPI_CLASS
#error ENABLE_SOFTWARE_SPI_CLASS must be set non-zero in SdFat/SdFatConfig.h
#endif  //ENABLE_SOFTWARE_SPI_CLASS
