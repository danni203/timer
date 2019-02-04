# timer
====<br>

two new labraries for the main code:<br>

>WindowsDisplay<br>

>ProssTime<br>

##`WindowsDisplay`<br>
Build for LED windows display<br>

###`functionlity`<br>
The functions acailable in the library include<br>
void LoadWindows(time_t);  //Initialize the SD and windows<br>
void windowpanel(softDMD , time_t , int , int);  // set LED windows<br>
bool check_Load_windows(time_t); // check windows<br>

##`ProcessTime`<br>
Build for set RTC and sync messages<br>

###`functionlity`<br>
void WritebatterybackedTZoffset(long); //<br>
long ReadbatterybackedTZoffset();<br>
void ProcessSyncMessage(long , time_t);<br>
void PrintDataTime(time_t , time_t);<br>
