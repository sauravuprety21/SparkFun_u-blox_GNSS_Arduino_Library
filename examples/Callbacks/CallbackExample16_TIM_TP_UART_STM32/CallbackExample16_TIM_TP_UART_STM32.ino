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

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

HardwareSerial mySerial(PA10, PA9);
HardwareSerial Serial(PA3, PA2);


SFE_UBLOX_GNSS myGNSS;


int dotsPrinted = 0; // Print dots in rows of 50 while waiting for a TIM TM2 message

// Callback: printTIMTPdata will be called when new TIM TP data arrives
// See u-blox_structs.h for the full definition of UBX_TIM_TP_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoTIMTPcallback
//        /                  _____  This _must_ be UBX_TIM_TP_data_t
//        |                 /                   _____ You can use any name you like for the struct
//        |                 |                  /
//        |                 |                  |
void printTIMTPdata(UBX_TIM_TP_data_t *ubxDataStruct)
{
    Serial.println();

    Serial.print(F(" towMS: ")); // Time pulse time of week according to time base: ms
    Serial.print(ubxDataStruct->towMS);

    Serial.print(F(" towSubMS: ")); // Submillisecond part of towMS:  ns
    Serial.print(ubxDataStruct->towSubMS);

    Serial.print(F(" qErr: "));  // Quantization error of time pulse: ps
    Serial.print(ubxDataStruct->qErr);

    Serial.print(F(" week: ")); // Time pulse week number according to time base: weeks
    Serial.print(ubxDataStruct->week);

    Serial.print(F(" timeBase: ")); // 0=Time base is GNSS time; 1=Time base is UTC
    Serial.print(ubxDataStruct->flags.bits.timeBase);

    Serial.print(F(" utc: ")); // 0=UTC not available; 1=UTC available
    Serial.print(ubxDataStruct->flags.bits.utc);

    Serial.print(F(" raim: ")); // 0=Information not available; 1=Not active; 2=Active
    Serial.print(ubxDataStruct->flags.bits.raim);

    Serial.print(F(" qErrInvalid: ")); // 0=Quantization error valid; 1=Quantization error invalid
    Serial.print(ubxDataStruct->flags.bits.qErrInvalid);

    Serial.print(F(" timeRefGnss: ")); //0=GPS; 1=GLONASS; 2=BeiDou; 3=Galileo; 15=Unknown
    Serial.print(ubxDataStruct->refInfo.bits.timeRefGnss);

    Serial.print(F(" utcStandard: ")); 
    Serial.print(ubxDataStruct->refInfo.bits.utcStandard);

    dotsPrinted = 0; // Reset dotsPrinted
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
  myGNSS.setAutoTIMTPcallbackPtr(&printTIMTPdata); // Enable automatic TIM TP messages with callback to printTIMTPdata
}

void loop()
{
  myGNSS.checkUblox(); // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

  Serial.print(".");
  delay(50);
  if (++dotsPrinted > 50)
  {
    Serial.println();
    dotsPrinted = 0;
  }
}
