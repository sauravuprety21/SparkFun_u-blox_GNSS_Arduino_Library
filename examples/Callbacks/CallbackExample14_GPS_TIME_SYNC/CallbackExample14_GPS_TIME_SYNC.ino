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

#define mySerial Serial1
#define PPS_PIN     (12u)
#define BTN_PIN     (11u)
typedef volatile bool        flag;

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

#include "gtime.h"

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

  //Assume that the U-Blox GNSS is running at 9600 baud (the default) or at 38400 baud.
  //Loop until we're in sync and then ensure it's at 38400 baud.
  do {
    Serial.println("GNSS: trying 38400 baud");
    mySerial.begin(38400);
    if (myGNSS.begin(mySerial) == true) break;

    delay(100);
    Serial.println("GNSS: trying 9600 baud");
    mySerial.begin(9600);
    if (myGNSS.begin(mySerial) == true) {
        Serial.println("GNSS: connected at 9600 baud, switching to 38400");
        myGNSS.setSerialRate(38400);
        delay(100);
    } else {
        //myGNSS.factoryReset();
        delay(2000); //Wait a bit before trying again to limit the Serial output
    }
  } while(1);
  Serial.println("GNSS serial connected");;

  myGNSS.setUART1Output(COM_TYPE_UBX); //Set the UART port to output UBX only
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
