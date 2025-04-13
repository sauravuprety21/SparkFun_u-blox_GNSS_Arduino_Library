/*
  Configuring the GNSS to automatically send TIM TP reports over UART and display the data using a callback
  By: Saurav Uprety
  SparkFun Electronics
  Date: December 30th, 2024
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.

  This example shows how to configure the u-blox GNSS to send TIM TP reports automatically
  and display the data via a callback. No more polling!

  The TIM TP message outputs the timing information for the next PPS (Pulse Per Second) edge. You can
  acheive this timing information with nanosecond resolution, and upto nanoseconds accuracy. 

  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/15136
  NEO-M8P RTK: https://www.sparkfun.com/products/15005
  NEO-M9N: https://www.sparkfun.com/products/17285

  Hardware Connections:
  Plug a Qwiic cable into the GPS and a BlackBoard
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/
#include <Wire.h> //Needed for I2C to GPS

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

#define PPS_PIN     (PC4)

using flag = volatile bool ;

using float32_t = float;
using float64_t = double;

typedef struct{
    int64_t  time;   /* time in whole second */
    float64_t sec;  /* fractional part */
}gtime_t;

const static float64_t gpst0[]={1980,1, 6,0,0,0}; /* gps time reference */

/* add time --------------------------------------------------------------------
* add time to gtime_t struct
* args   : gtime_t t        I   gtime_t struct
*          float64_t sec    I   time to add (s)
* return : gtime_t struct (t+sec)
*-----------------------------------------------------------------------------*/
gtime_t gtime_timeadd(gtime_t t,float64_t sec)
{
    float64_t tt;

    t.sec+=sec; tt=floor(t.sec); t.time+=(int32_t)tt; t.sec-=tt;
    return t;
}
/* time difference -------------------------------------------------------------
* difference between gtime_t structs
* args   : gtime_t t1,t2    I   gtime_t structs
* return : time difference (t1-t2) (s)
*-----------------------------------------------------------------------------*/
float64_t gtime_timediff(gtime_t t1,gtime_t t2)
{
    return (t1.time-t2.time)+t1.sec-t2.sec;
}

/* convert calendar day/time to time -------------------------------------------
* convert calendar day/time to gtime_t struct
* args   : float64_t *ep       I   day/time {year,month,day,hour,min,sec}
* return : gtime_t struct
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
gtime_t gtime_epoch2time(const float64_t *ep)
{
    const int32_t doy[]={1,32,60,91,121,152,182,213,244,274,305,335};
    gtime_t time={0};
    int32_t days,sec,year=(int32_t)ep[0],mon=(int32_t)ep[1],day=(int32_t)ep[2];

    if(year<1970||2099<year||mon<1||12<mon) return time;

    /* leap year if year%4==0 in 1901-2099 */
    days=(year-1970)*365+(year-1969)/4+doy[mon-1]+day-2+(year%4==0&&mon>=3?1:0);
    sec=(int32_t)floor(ep[5]);
    time.time=(int64_t)days*86400+(int64_t)ep[3]*3600+(int64_t)ep[4]*60+sec;
    time.sec=ep[5]-sec;
    return time;
}

/* get time struct with gps time ----------------------------------------------
* convert week and tow in gps time to gtime_t struct
* args   : int32_t    week  I   week number in gps time
*          float64_t  sec   I   time of week in gps time (s)
* return : gtime_t struct
*-----------------------------------------------------------------------------*/
gtime_t gtime_gpst2time(int32_t week,float64_t sec){
    gtime_t t=gtime_epoch2time(gpst0);

    if(sec<-1E9||1E9<sec) sec=0.0;
    t.time+=(int64_t)86400*7*week+(int32_t)sec;
    t.sec=sec-(int32_t)sec;
    return t;
}

SFE_UBLOX_GNSS myGNSS;

gtime_t lastPps_gtime{0,0}, lastTp_gtime{0,0};
volatile float32_t ppsDiff_sec;
volatile uint32_t lastPps_us, ppsDiff_us;
volatile uint32_t lastTp_us;
flag ppsInit, newTp;

gtime_t now_gtime{0,0};


void onTimTP_Callback(UBX_TIM_TP_data_t *ubxDataStruct)
{
  lastTp_us = micros();
  float64_t tpTow;

  tpTow = (float64_t)ubxDataStruct->towMS;
  tpTow += (((float64_t)ubxDataStruct->towSubMS)*2E-32);
  tpTow /= 1000.0;

  lastTp_gtime = gtime_gpst2time(ubxDataStruct->week, tpTow);
  newTp = true;
  return;
}

void onPps_Callback(void){
  
  uint32_t temp_ = micros();

  if(!newTp) {return;}
  newTp = false;

  if(ppsInit) {
    ppsDiff_us = temp_ - lastPps_us;
    ppsDiff_sec = gtime_timediff(lastTp_gtime, lastPps_gtime);
  } else{
    ppsInit = true;
  }
  lastPps_us = temp_;
  lastPps_gtime.time = lastTp_gtime.time;
  lastPps_gtime.sec = lastTp_gtime.sec;
  return;
}

gtime_t systime_get(void){
  uint32_t now_us = micros();
  float64_t timeAdd_sec = ((float32_t)(now_us - lastPps_us)) / ((float32_t)(ppsDiff_us)) * ppsDiff_sec;
  
  return gtime_timeadd(lastPps_gtime, timeAdd_sec);
}


void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for user to open terminal
  Serial.println("SparkFun u-blox Example");

  Wire.begin();

  if (myGNSS.begin() == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.setNavigationFrequency(1); //Produce one solution per second
  myGNSS.saveConfiguration(); //Save the current settings to flash and BBR
  myGNSS.setAutoTIMTPcallbackPtr(&onTimTP_Callback); // Enable automatic TIM TP messages with callback to printTIMTPdata

  ppsInit = false; newTp = false;
  // Init to these values, incase no GPS signals
  lastPps_us=0; ppsDiff_us=1; ppsDiff_sec=1e-6;
  lastTp_us=0;

  pinMode(PPS_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PPS_PIN), onPps_Callback, RISING);

}
void loop()
{
  if(!newTp){
    myGNSS.checkUblox(); // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
  }

  now_gtime = systime_get();

  Serial.print("\n");
  Serial.print((uint32_t)now_gtime.time);
  Serial.print("\t");
  Serial.print((float32_t)now_gtime.sec,12);
  Serial.print("\t");
  Serial.print((uint8_t)(lastTp_us < lastPps_us));
  Serial.print("\t");
  Serial.print((uint8_t)(newTp));
  Serial.print("\n");

  delay(200);
}
