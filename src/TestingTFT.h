//
// File			Master_Weather_Station.h
// Header
//
// Details		<#details#>
//
// Project		 Master_Weather_Station
// Developed with [embedXcode](http://embedXcode.weebly.com)
//
// Author		freddie
// 				Kingston Co.
//
// Date			2/9/19 3:33 PM
// Version		<#version#>
//
// Copyright	© freddie, 2019
// Licence    <#license#>
//
// See			ReadMe.txt for references

#include "Arduino.h"

#ifndef Testing_TFT_h
    #define Testing_TFT_h

/*******************************************************************************************************************
 * - Only modify this file to include:
 * - Include files
 * - Function definitions (prototypes)
 * - Constants
 * - External variable definitions, in the appropriate section
 *******************************************************************************************************************/

/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                     Include application, user and local libraries
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/// These includes are from the Arduino program that lives in /Application/Arduino

#include <SPI.h>        // For all SPI transactions on TFT, SD Card, and Touch Screen.
#include <Time.h>       // This is from the Teensy Libraries for time keeping on the Teensy.
#include <Wire.h>       // This is the One Wire or TWI library.
#include <Stream.h>     // Needed for serial streaming stuff.
#include <Streaming.h>  // Needed for special serial streaming stuff.
#include <math.h>

/*******************************************************************************************************************
* This definition determines whether the Teensy is using the attached GPS Controller Chip to get time, date, and
* GPS Coordinates.
* Comment this line out if not using the External GPS chip connected on a Serial Bus.
*******************************************************************************************************************/
#define USINGGPS 1      // This flag determines whether the Teensy is using the GPS controller.

#if defined(USINGGPS)
    // Set this to the GPS hardware serial port you wish to use
    #define GPShwSERIAL 1 // 1 = Serial1, 2 = Serial2, 3 = Serial3, 4 = Serial4 ....

    uint8_t const GPSSerialPort = GPShwSERIAL; // default settings

    uint32_t const BaudDefault = 9600; // default settings

    #define SERIAL1_RX_BUFFER_SIZE = 128;  // Need larger than the stock 64 bytes for incoming buffer for GPS Packet (100 bytes).
#endif

/*******************************************************************************************************************
* This definition is for the TFTM07 FT5206 Capacitive Touch Screen Controller.
* This type of touch screen controller is only presently installed on the 7" Screen ER-TFTM070.
* Comment this line out if not using the Capacitive Touch Screen Controller on the 7" TFT.
*******************************************************************************************************************/
//#define USE_FT5206_TOUCH

#define MAXTOUCHLIMIT     1  // 1...5


/*******************************************************************************************************************
* This definition is for the TFTM08 build-in 4-wire Resistive Touch Screen Controller.
* This type of touch screen controller is only presently installed on the 8" & 9" Screen ER-TFTM080-2 & ER-TFTM090-2.
* Comment this line out if not using the 4-wire REsistive Touch Screen Controller.
*******************************************************************************************************************/
#define USE_RA8875_TOUCH  // Resistive touch screen

/*******************************************************************************************************************
* This definition is for the TFTM08 build-in 4-wire Resistive Touch Screen Controller.
* This type of touch screen controller is only presently installed on the 8" & 9" Screen ER-TFTM080-2 & ER-TFTM090-2.
* This is the maximum speed my Master Weather Station can go because of cabling issues.
*******************************************************************************************************************/
//const static uint32_t MAXSPISPEED	= 12000000UL;  //don't go higher than 22000000!;


/// These are includes from outside the project ~/Documents/Arduino/libraries

//#include "FastCRC.h"            // This library calculates various CRC's using Teensy's builtin math functions.

/// These are include from inside the project
#include <DS3231RTC.h>      // This is for the DS3231 time keeping IC, it also has an EEPROM onboard.
#include <EEPROMex.h>       // This library is to expand on EEPROM reading and writing capabilities.
#include "Version.h"        // This has the most recent build version
#include "Eds_BME680.h"     // Include the BME680 Sensor library.
#include <mcp9808.h>        // Include the MCP9808 Precision Temperature Sensor.
#include <Timezone.h>       // This library is for making Time Zone adjustments on UTC Time.
#include <AT24CX.h>         // This library is for using the AT24C32 EEProm on the RTC board.
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
#include <Ra8876_t3.h>
#include <font_Arial.h>
#include <glcdfont.c>
#include <memcpy.h>
#include "TMP117.h"         // Include the TMP117 Temp sensor for calibrating the other temp sensors.
#include "Watchdog_t4.h"    // Include the Teensy 4.x Watch dog timer Library.

//#include "TeensyTimerTool.h"
//using namespace TeensyTimerTool;

#include "FS.h"
#include <LittleFS.h>
#include "SD.h"
#include <MTP_Teensy.h>



/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                     End of include libraries.
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             Typdefs
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

//typedef unsigned char mybyte;
//mybyte getChecksum(char* str);


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             Enumerations
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

enum dotColorSelection
{
    DOT_RED     = 0,
    DOT_YELLOW  = 1,
    DOT_GREEN   = 2,
    DOT_NONE    = 3
};

enum gpsCompletionCodes
{
    NO_FIX                  = 0,
    DEAD_RECKONING_ONLY     = 1,
    TWO_D_FIX               = 2,
    THREE_D_FIX             = 3,
    GNSS_DR_COMBINED        = 4,
    TIME_FIX_ONLY           = 5

};


// This enum is for the move Data File From Src To Dest Subroutine.
enum diskCopyTypeCodes
    {
        BAD_CODE                                                = 0,
        DEST_LITTLEFS_INIT_FILE_SYS_INIT_FILE_DIR_YES           = 1,
        DEST_LITTLEFS_INIT_FILE_SYS_INIT_FILE_DIR_NO            = 2,
        DEST_LITTLEFS_INIT_FILE_SYS_NOINIT_FILE_DIR_YES         = 3,
        DEST_LITTLEFS_INIT_FILE_SYS_NOINIT_FILE_DIR_NO          = 4,
        DEST_LITTLEFS_NOINIT_FILE_SYS_INIT_FILE_DIR_YES         = 5,
        DEST_LITTLEFS_NOINIT_FILE_SYS_INIT_FILE_DIR_NO          = 6,
        DEST_LITTLEFS_NOINIT_FILE_SYS_NOINIT_FILE_DIR_YES       = 7,
        DEST_LITTLEFS_NOINIT_FILE_SYS_NOINIT_FILE_DIR_NO        = 8,
        DEST_SDCARD_INIT_FILE_SYS_INIT_FILE_DIR_YES             = 9,
        DEST_SDCARD_INIT_FILE_SYS_INIT_FILE_DIR_NO              = 10,
        DEST_SDCARD_INIT_FILE_SYS_NOINIT_FILE_DIR_YES           = 11,
        DEST_SDCARD_INIT_FILE_SYS_NOINIT_FILE_DIR_NO            = 12,
        DEST_SDCARD_NOINIT_FILE_SYS_INIT_FILE_DIR_YES            = 13,
        DEST_SDCARD_NOINIT_FILE_SYS_INIT_FILE_DIR_NO            = 14,
        DEST_SDCARD_NOINIT_FILE_SYS_NOINIT_FILE_DIR_YES         = 15,
        DEST_SDCARD_NOINIT_FILE_SYS_NOINIT_FILE_DIR_NO          = 16
    };


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             Structs
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/


/*******************************************************************************************************************
 * This struct is for the System Sensor function.
 *******************************************************************************************************************/
struct SensorState
{ //xx bytes
    
    float BME680TemperatureC;           //32 bits (4 bytes).
    float BME680TemperatureF;           //32 bits (4 bytes).
    float BME680Baro_Pressure;          //32 bits (4 bytes).
    float BME680Relative_Humidity;      //32 bits (4 bytes).
    float BME680gasSensor;              // This sensor measures particulate matter in the air.
    float BME680altitude;               // Altitude is calculated from the Baro_Pressure.
    bool BME680DataValid;               // Valid Data Flag for BME680.
    float mcp9808TempC;
    float mcp9808TempF;
    uint16_t mcp9808ConfigReg;
    uint8_t mcp9808Resolution;
    uint16_t mcp9808ManufID;
    uint8_t mcp9808DeviceID;
    uint8_t mcp9808Revision;
    float mcp9808TempOffset;
    float mcp9808TupperC;
    float mcp9808TupperF;
    uint8_t mcp9808TUpperStatus;
    float mcp9808TlowerC;
    float mcp9808TlowerF;
    uint8_t mcp9808TlowerStatus;
    float mcp9808TCriticalC;
    float mcp9808TCriticalF;
    uint8_t mcp9808TCriticalStatus;
    bool mcp9808DataValid;               // Valid Data Flag for MCP9808.
    double tmp117TempC;
    double tmp117TempF;
    TMP117_ALERT tmp117AlertType;
    double tmp117OffsetTemp;
    uint16_t tmp117DeviceID;
    uint16_t tmp117DeviceRev;
    bool tmp117DataValid;               // Valid Data Flag for TMP117.
}   SensorStruct;


/*******************************************************************************************************************
 *                           This struct is for ubx GPS data.
 * This struct is 56 bytes total.
 *******************************************************************************************************************/
struct gpsGNSSStruct
{
    uint32_t iTOW;              // GPS time of week of the navigation epoch: ms
    uint16_t year;              // Year (UTC)
    uint8_t month;              // Month, range 1..12 (UTC)
    uint8_t day;                // Day of month, range 1..31 (UTC)
    uint8_t hour;               // Hour of day, range 0..23 (UTC)
    uint8_t min;                // Minute of hour, range 0..59 (UTC)
    uint8_t sec;                // Seconds of minute, range 0..60 (UTC)

    uint8_t validTime;          // 1 = valid UTC time of day
    uint8_t validDate;          // 1 = valid UTC Date

    uint8_t fullyResolved;      // 1 = UTC time of day has been fully resolved (no seconds uncertainty)..

    uint32_t tAcc;              // Time accuracy estimate (UTC): ns
    int32_t nano;               // Fraction of second, range -1e9 .. 1e9 (UTC): ns
 
    uint8_t fixType;            // GNSSfix Type:
                                // 0: no fix
                                // 1: dead reckoning only
                                // 2: 2D-fix
                                // 3: 3D-fix
                                // 4: GNSS + dead reckoning combined
                                // 5: time only fix

    uint8_t numSV;              // Number of satellites used in Nav Solution
    float lon;                  // Longitude: adjusted by dividing by 10 Million.
    float lat;                  // Latitude: adjusted by dividing by 10 Million.
    float hMSL;                 // Height above mean sea level: adjusted by Multipling by 0.00328084.
    float gSpeed;               // Ground Speed (2-D): adjusted to m/sec by multipling by 0.001.
    int32_t headMot;            // Heading of motion (2-D): deg * 1e-5

    int32_t headVeh;            // Heading of vehicle (2-D): deg * 1e-5
    int16_t magDec;             // Magnetic declination: deg * 1e-2
    bool valid = false;         // Data has just been refreshed and is valid.
}   gpsUBXDataStruct;

/*******************************************************************************************************************
*                           This struct is for the TFT touchscreen points touched.
* This struct is 4 bytes total.
*******************************************************************************************************************/
// Touch screen struct.
struct touchedPoints
{
    int16_t xPosition;
    int16_t yPosition;
};

/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             Constants
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

///             Debug constants.

#define DEBUG       true             // Turn on/off wait for Serial USB monitor, and verify setup completes.
//#define DEBUG_PRINT 0               // Set SD card format Debug option to 1 to see more info.
//#define DEBUG1    true              // Turn on/off main functions debug messages.
//#define DEBUG2    true              // Turn on/off Wind Speed interrupt debugging.
//#define DEBUG3    true             // Turn on/off Wind Direction interrupt messages.
//#define DEBUG4    true               // Keyboard input debugging.
//#define DEBUG5    true               // Set keyboard time debugging.
//#define DEBUG6    true               // Show Free RAM Memory.
//#define DEBUG7    true               // Set Run 1 Frequency debugging.
//#define DEBUG8    true               // Set Run Multi Frequencies debugging.
//#define DEBUG9    true              // Set DS3231 Chip debugging.
//#define DEBUG10   true            // Set GPS Chip debugging.
//#define DEBUG11   true            // Set TFT debugging.
//#define DEBUG12   true              // Set Touch Controller debugging.
//#define DEBUG13   true              // Set GPS serial buffer data dump to screen.
//#define DEBUG14   true              // Set debug data printout for atmosphereic pressure debugging.
//#define DEBUG15   true            // Set debug data printout for atmosphereic pressure debugging.
//#define DEBUG16   true            // Set debug data printout for Rainfall sensor debugging.
//#define DEBUG17   true           // Set the debug for UartEvent debugging, transfering data struct from one Teensy to another.
//#define DEBUG18   true           // Set the debug flag for Rain Gauge functions.
//#define DEBUG19   true                // Set the debug flag for the RA8875 Touch Screen Preferences section.
//#define DEBUG20   true                // Set the debug flag for UartEvent ISR's.
//#define DEBUG21   true                // Set the debug flag for UartEvent Send Data To Slave from the Master over the Uart.
//#define DEBUG22   true                // Set the debug flag for UartEvent recieve data from Master from the Uart.
//#define DEBUG23   true                // Set debug for crcCheck from the FastCRC libarary.
//#define DEBUG24   true                // Set debug for crcCreate from the FastCRC libarary.
//#define DEBUG25   true                // Set Main Loop Master/Slave Messages sending and recieving debugging.
//#define DEBUG26   true                // Set Main Loop Master/Slave Messages sending and recieving, Print the incoming buffer debugging.
//#define DEBUG28   true                // Set Debugging for GPS updateTimeFromGPS Function.
//#define DEBUG30   true                // Set Debugging for Set Time and Time Zone Subs.
//#define DEBUG31   true                // Set Search for One Wire Bus DS18B20 Temp Sensor(s) true.
//#define DEBUG32   true                // Set Change the ID Numbers of the One Wire Bus Temp Sensor(s) True.
//#define DEBUG33   true                // Set Temp Sensors on the One Wire Bus Debugging True.
//#define DEBUG34   true                // Set Debugging for Temperature Calibration Offset.
//#define DEBUG35   true                // Set Debugging for the wait For Another Touch Function.
//#define DEBUG36   true                // Set Debugging for the 12 button Numeric Keypad Function.
//#define DEBUG37   true                // Set Debugging tft Print Keypad AnswerFunction.
//#define DEBUG38   true                // Set Debugging TFT get Number From Numeric Keypad Function.
//#define DEBUG39   true                // Set Debugging set Master Yearly Rain Offset Function.
//#define DEBUG40   true                // Set Debugging Reading Startup Values from EEPROM.
//#define DEBUG41   true                // Set Debugging GPS Startup functions in setup routine.
//#define DEBUG42   true                // Set Debugging for the BME280 Sensor Raad Function.
//#define DEBUG43   true                // Set Debugging for the Process Data From Slave Function.
//#define DEBUG44   true                // Set Debugging for the Set Day of the Week Function.
//#define DEBUG45   true                // Set Debugging for the Rain Preferences Function.
//#define DEBUG46   true                // Set Debugging for the WatchDog Setup function.
//#define DEBUG47     true              // Set Debugging for the  GPS status at the end of Loop
//#define DEBUG48     true              // Show the GPS receive buffer Data Contents and return.
//#define DEBUG49     true              // Show the GPS restart info.
//#define DEBUG50     true              // Disply the actual Data coming from the GPS in the restart Subroutine.
//#define DEBUG51     true              // Disply the size of the tcr abbrev in the drawPrintTime/// Subroutine.
//#define DEBUG52     true              // Disply the MCP9808 debug info.
//#define DEBUG53     true              // Display the TMP117 debug info.
//#define DEBUG54     true              // Display the TMP117 debug info.

//#define DEBUG_COPY_TO_DIFF_SYS_SUB      // Display the Move data file to another file system info for debugging.
//#define DEBUG_MEDIA_TRANSFER_SUB        // Display the media Transfer sub detail info for debugging.

#define ONE_SECOND_TICKS                                // Display 1 second tic marks (.) on monitor output for debugging.
#define SDCARD_PRINT_DIRECTORY                          // This define if true will print the current root directory on the SD Card.
//#define FORMAT_SD_CARD_BEFORE_USING                     // Format the SD Card before using it, this is mostly used for testing purposes.
//#define CREATE_SDCARD_FILE_IN_SUBDIRECTORY              // This define if true creates a File in the subdirectory on the SD card.
#define DEBUG_SD_CARD_STARTUP_CODE                      // Display the detail workings for debugging the SD Card Startup code.

#define DEBUG_STOPLOGGINGDATATO_DISK_SUB                // Display the stop logging data to disk debugging info on the Serial monitor.
//#define DEBUG_LOGGINGDATA_TO_RAMDISK_SUB                // Display the media Transfer sub detail info for debugging.
#define DEBUG_COPY_RAMDISK_TO_SDCARD_SUB                // Display the detail working for debug of the copy ramdisk to sdcard subroutine.
#define DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB  // Display the detail workings for debug of the create New Data File And Start Logging subrountine.
#define DEBUG_STARTUP_CODE                              // Display the detail workings for debugging the startup Code.
#define DEBUG_STARTUP_CRASHDATA_CODE                    // Display the Crash Report right after startup.
#define DEBUG_STARTUP_LITTLEFS_CODE                     // Display the detail workings for debugging the startup LittleFS Code.
//#define DEBUG_LOOP_TEN_SECOND_COUNT                     // Display the detail workings for degubbing the Loop Ten Second Count code.
#define DEBUG_LOOP_2_MINUTE_COUNT                       // Display the detail workings for debugging the Loop two minute Count code.
#define DEBUG_LOOP_MIDNIGHT_TIME                        // Display the detail workings for debugging the Loop Midnight hour code.
//#define DEBUG_LOOP_TFTSTATUS_LINE_INFO                  // Display the detail working for debugging the Loop TFT status line outputs.
//#define DEBUG_LOOP_TEMPSENSORS_CALIBRATION              // Display the Temp Sensors Calibration data debug info in the Main Loop.
//#define DEBUG_COMPARETMP117_WITHOTHERS                  // Display the Compare TMP117 temp sensor output with the other temp sensors.
//#define DEBUG_CALIBRATE_BME680_SENSOR                   // Display the Calibrate BME680 using the TMP117 as reference debug info.
//#define DEBUG_CALIBRATE_MCP9808_SENSOR                   // Display the Calibrate MCP9808 using the TMP117 as reference debug info.

//#define DEBUG_DAYOFMONTH_FOR_TESTING                    // Change the naming of the RamDisk file day of the month for testing purposes.
//#define DEBUG_GPS_LOOP_MESSAGES                         // Display the details for debugging GPS info in the Main Loop Code.
#define DEBUG_PRINT_DIRECTORY_SUB                       // Display the detail workings for debugging the Print Directory code.


/*******************************************************************************************************************
* These definitions are for the RA8875 TFT display.
*******************************************************************************************************************/

/*******************************************************************************************************************
* These variables are for the horizontal pixel size (Xsize), Vertical pixel size (Ysize), LineNumber (0-271), (0-479)
* or (0-1023), and current position of the cursor (Xposition) (0-799) or (0-479).
* The RA8875_FONTSCALE varible is for the multiplier of the standard (8 x 16) font size provided with the TFT lib.
* A multiplier of 0 means the font size will be 8 x 16, a multiplier of 1 means the font size will be 16 x 32 ...
*******************************************************************************************************************/

/// Uncomment one of these display definition:
//#define TFTM043
//#define TFTM07
//#define TFTM08
//#define TFTM09
#define TFTM101

#if defined(TFTM043)
    #define X_CONST 480  // These setting are for the 4.2" display (ER-TFTM043-3) 480 x 272 pixels.
    #define Y_CONST 272  // These setting are for the 4.2" display (ER-TFTM043-3) 480 x 272 pixels.
    #define RA8875_FONTSCALE  1  // Font size is 1 or 16 wide x 32 high pixels.

    #if (RA8875_FONTSCALE == 0)
        #define Xsize 8
        #define Ysize 16

    #elif (RA8875_FONTSCALE == 1)
        #define Xsize 16
        #define Ysize 32

    #elif (RA8875_FONTSCALE == 2)
        #define Xsize 32
        #define Ysize 64

    #elif (RA8875_FONTSCALE == 3)
        #define Xsize 64
        #define Ysize 128
    #endif  // #if RA8875_FONTSCALE == 0

    #define YnumberOfRows (Y_CONST / Ysize)
    #define XnumberOfColumns (X_CONST / Xsize)
#endif

/*******************************************************************************************************************
* These definitions are for the TFTM07, TFTM08, or TFTM09 in (X = 800, Y = 480) mode which has 50 Columns,
* and 15 rows of text when the display is in the Landscape mode.
*******************************************************************************************************************/

#ifdef TFTM07
    #define X_CONST 800  // These setting are for the 7" display (ER-TFTM07-5) 800 x 480 pixels.
    #define Y_CONST 480  // These setting are for the 7" display (ER-TFTM07-5) 800 x 480 pixels.

    #define RA8875_FONTSCALE 1  // Font size is 1 or 16 wide x 32 high pixels.

    #if (RA8875_FONTSCALE == 0)
        #define Xsize 8
        #define Ysize 16

    #elif (RA8875_FONTSCALE == 1)
        #define Xsize 16
        #define Ysize 32

    #elif (RA8875_FONTSCALE == 2)
        #define Xsize 32
        #define Ysize 64

    #elif (RA8875_FONTSCALE == 3)
        #define Xsize 64
        #define Ysize 128
    #endif  // #if RA8875_FONTSCALE == 0

    #define YnumberOfRows (Y_CONST / Ysize)
    #define XnumberOfColumns (X_CONST / Xsize)


/*******************************************************************************************************************
 * These definitions are for 50 Rows of text when the display is in the Landscape mode. (X = 800, Y = 480).
 * Font size is 1 or 16 wide x 32 high pixels.
 *******************************************************************************************************************/
#define XLEFT    0  // Left side of screen.

#define XCOL1 0
#define XCOL2  Xsize
#define XCOL3  (2 * Xsize)
#define XCOL4  (3 * Xsize)
#define XCOL5  (4 * Xsize)
#define XCOL6  (5 * Xsize)
#define XCOL7  (6 * Xsize)
#define XCOL8  (7 * Xsize)
#define XCOL9  (8 * Xsize)
#define XCOL10  (9 * Xsize)
#define XCOL11  (10 * Xsize)
#define XCOL12  (11 * Xsize)
#define XCOL13  (12 * Xsize)
#define XCOL14  (13 * Xsize)
#define XCOL15  (14 * Xsize)
#define XCOL16  (15 * Xsize)
#define XCOL17  (16 * Xsize)
#define XCOL18  (17 * Xsize)
#define XCOL19  (18 * Xsize)
#define XCOL20  (19 * Xsize)
#define XCOL21  (20 * Xsize)
#define XCOL22  (21 * Xsize)
#define XCOL23  (22 * Xsize)
#define XCOL24  (23 * Xsize)

#define XMIDDLE 399  // Middle of the screen horizonal.

#define XCOL25  (24 * Xsize)
#define XCOL26  (25 * Xsize)
#define XCOL27  (26 * Xsize)
#define XCOL28  (27 * Xsize)
#define XCOL29  (28 * Xsize)
#define XCOL30  (29 * Xsize)
#define XCOL31  (30 * Xsize)
#define XCOL32  (31 * Xsize)
#define XCOL33  (32 * Xsize)
#define XCOL34  (33 * Xsize)
#define XCOL35  (34 * Xsize)
#define XCOL36  (35 * Xsize)
#define XCOL37  (36 * Xsize)
#define XCOL38  (37 * Xsize)
#define XCOL39  (38 * Xsize)
#define XCOL40  (39 * Xsize)
#define XCOL41  (40 * Xsize)
#define XCOL42  (41 * Xsize)
#define XCOL43  (42 * Xsize)
#define XCOL44  (43 * Xsize)
#define XCOL45  (44 * Xsize)
#define XCOL46  (45 * Xsize)
#define XCOL47  (46 * Xsize)
#define XCOL48  (47 * Xsize)
#define XCOL49  (48 * Xsize)
#define XCOL50  (49 * Xsize)

#define XRIGHT 799  // Extreme right of screen.

/*******************************************************************************************************************
 * These definitions are for 15 Rows of text when the display is in the Landscape mode. (X = 800, Y = 480).
 * Font size is 1 or 16 wide x 32 high pixels.
 *******************************************************************************************************************/
#define YTOP 0  // Top of screen.

#define YLINE1 0
#define YLINE2  Ysize
#define YLINE3  (2 * Ysize)
#define YLINE4  (3 * Ysize)
#define YLINE5  (4 * Ysize)
#define YLINE6  (5 * Ysize)
#define YLINE7  (6 * Ysize)

#define YMIDDLE 239  // Middle of the screen vertically
#define YLINE8  (7 * Ysize)
#define YLINE9  (8 * Ysize)
#define YLINE10  (9 * Ysize)
#define YLINE11  (10 * Ysize)
#define YLINE12  (11 * Ysize)
#define YLINE13  (12 * Ysize)
#define YLINE14  (13 * Ysize)
#define YLINE15  (14 * Ysize)

#define YBOTTOM 479  // Bottom row of screen.

#endif


#if defined(TFTM08)
    #define X_CONST 800  // These setting are for the 7" display (ER-TFTM08-5) 800 x 480 pixels.
    #define Y_CONST 480  // These setting are for the 7" display (ER-TFTM08-5) 800 x 480 pixels.
    #define RA8875_FONTSCALE    1  // Font size is 1 or 16 wide x 32 high pixels.

    #if (RA8875_FONTSCALE == 0)
        #define Xsize 8
        #define Ysize 16

    #elif (RA8875_FONTSCALE == 1)
        #define Xsize 16
        #define Ysize 32

    #elif (RA8875_FONTSCALE == 2)
        #define Xsize 32
        #define Ysize 64

    #elif (RA8875_FONTSCALE == 3)
        #define Xsize 64
        #define Ysize 128
    #endif  // #if RA8875_FONTSCALE == 0

    #define YnumberOfRows (Y_CONST / Ysize)
    #define XnumberOfColumns (X_CONST / Xsize)


/*******************************************************************************************************************
 * These definitions are for 50 Rows of text when the display is in the Landscape mode. (X = 800, Y = 480).
 * Font size is 1 or 16 wide x 32 high pixels.
 *******************************************************************************************************************/
#define XLEFT    0  // Left side of screen.

#define XCOL1 0
#define XCOL2  Xsize
#define XCOL3  (2 * Xsize)
#define XCOL4  (3 * Xsize)
#define XCOL5  (4 * Xsize)
#define XCOL6  (5 * Xsize)
#define XCOL7  (6 * Xsize)
#define XCOL8  (7 * Xsize)
#define XCOL9  (8 * Xsize)
#define XCOL10  (9 * Xsize)
#define XCOL11  (10 * Xsize)
#define XCOL12  (11 * Xsize)
#define XCOL13  (12 * Xsize)
#define XCOL14  (13 * Xsize)
#define XCOL15  (14 * Xsize)
#define XCOL16  (15 * Xsize)
#define XCOL17  (16 * Xsize)
#define XCOL18  (17 * Xsize)
#define XCOL19  (18 * Xsize)
#define XCOL20  (19 * Xsize)
#define XCOL21  (20 * Xsize)
#define XCOL22  (21 * Xsize)
#define XCOL23  (22 * Xsize)
#define XCOL24  (23 * Xsize)

#define XMIDDLE 399  // Middle of the screen horizonal.

#define XCOL25  (24 * Xsize)
#define XCOL26  (25 * Xsize)
#define XCOL27  (26 * Xsize)
#define XCOL28  (27 * Xsize)
#define XCOL29  (28 * Xsize)
#define XCOL30  (29 * Xsize)
#define XCOL31  (30 * Xsize)
#define XCOL32  (31 * Xsize)
#define XCOL33  (32 * Xsize)
#define XCOL34  (33 * Xsize)
#define XCOL35  (34 * Xsize)
#define XCOL36  (35 * Xsize)
#define XCOL37  (36 * Xsize)
#define XCOL38  (37 * Xsize)
#define XCOL39  (38 * Xsize)
#define XCOL40  (39 * Xsize)
#define XCOL41  (40 * Xsize)
#define XCOL42  (41 * Xsize)
#define XCOL43  (42 * Xsize)
#define XCOL44  (43 * Xsize)
#define XCOL45  (44 * Xsize)
#define XCOL46  (45 * Xsize)
#define XCOL47  (46 * Xsize)
#define XCOL48  (47 * Xsize)
#define XCOL49  (48 * Xsize)
#define XCOL50  (49 * Xsize)

#define XRIGHT 799  // Extreme right of screen.

/*******************************************************************************************************************
 * These definitions are for 15 Rows of text when the display is in the Landscape mode. (X = 800, Y = 480).
 * Font size is 1 or 16 wide x 32 high pixels.
 *******************************************************************************************************************/
#define YTOP 0  // Top of screen.

#define YLINE1 0
#define YLINE2  Ysize
#define YLINE3  (2 * Ysize)
#define YLINE4  (3 * Ysize)
#define YLINE5  (4 * Ysize)
#define YLINE6  (5 * Ysize)
#define YLINE7  (6 * Ysize)

#define YMIDDLE 239  // Middle of the screen vertically
#define YLINE8  (7 * Ysize)
#define YLINE9  (8 * Ysize)
#define YLINE10  (9 * Ysize)
#define YLINE11  (10 * Ysize)
#define YLINE12  (11 * Ysize)
#define YLINE13  (12 * Ysize)
#define YLINE14  (13 * Ysize)
#define YLINE15  (14 * Ysize)

#define YBOTTOM 479  // Bottom row of screen.

#endif


#if defined(TFTM09)
    #define X_CONST 800  // These setting are for the 7" display (ER-TFTM09-5) 800 x 480 pixels.
    #define Y_CONST 480  // These setting are for the 7" display (ER-TFTM09-5) 800 x 480 pixels.
    #define RA8875_FONTSCALE    1  // Font size is 1 or 16 wide x 32 high pixels.

    #if (RA8875_FONTSCALE == 0)
        #define Xsize 8
        #define Ysize 16

    #elif (RA8875_FONTSCALE == 1)
        #define Xsize 16
        #define Ysize 32

    #elif (RA8875_FONTSCALE == 2)
        #define Xsize 32
        #define Ysize 64

    #elif (RA8875_FONTSCALE == 3)
        #define Xsize 64
        #define Ysize 128
        #endif  // #if RA8875_FONTSCALE == 0

    #define YnumberOfRows (Y_CONST / Ysize)
    #define XnumberOfColumns (X_CONST / Xsize)


/*******************************************************************************************************************
 * These definitions are for 50 Rows of text when the display is in the Landscape mode. (X = 800, Y = 480).
 * Font size is 1 or 16 wide x 32 high pixels.
 *******************************************************************************************************************/
#define XLEFT    0  // Left side of screen.

#define XCOL1 0
#define XCOL2  Xsize
#define XCOL3  (2 * Xsize)
#define XCOL4  (3 * Xsize)
#define XCOL5  (4 * Xsize)
#define XCOL6  (5 * Xsize)
#define XCOL7  (6 * Xsize)
#define XCOL8  (7 * Xsize)
#define XCOL9  (8 * Xsize)
#define XCOL10  (9 * Xsize)
#define XCOL11  (10 * Xsize)
#define XCOL12  (11 * Xsize)
#define XCOL13  (12 * Xsize)
#define XCOL14  (13 * Xsize)
#define XCOL15  (14 * Xsize)
#define XCOL16  (15 * Xsize)
#define XCOL17  (16 * Xsize)
#define XCOL18  (17 * Xsize)
#define XCOL19  (18 * Xsize)
#define XCOL20  (19 * Xsize)
#define XCOL21  (20 * Xsize)
#define XCOL22  (21 * Xsize)
#define XCOL23  (22 * Xsize)
#define XCOL24  (23 * Xsize)

#define XMIDDLE 399  // Middle of the screen horizonal.

#define XCOL25  (24 * Xsize)
#define XCOL26  (25 * Xsize)
#define XCOL27  (26 * Xsize)
#define XCOL28  (27 * Xsize)
#define XCOL29  (28 * Xsize)
#define XCOL30  (29 * Xsize)
#define XCOL31  (30 * Xsize)
#define XCOL32  (31 * Xsize)
#define XCOL33  (32 * Xsize)
#define XCOL34  (33 * Xsize)
#define XCOL35  (34 * Xsize)
#define XCOL36  (35 * Xsize)
#define XCOL37  (36 * Xsize)
#define XCOL38  (37 * Xsize)
#define XCOL39  (38 * Xsize)
#define XCOL40  (39 * Xsize)
#define XCOL41  (40 * Xsize)
#define XCOL42  (41 * Xsize)
#define XCOL43  (42 * Xsize)
#define XCOL44  (43 * Xsize)
#define XCOL45  (44 * Xsize)
#define XCOL46  (45 * Xsize)
#define XCOL47  (46 * Xsize)
#define XCOL48  (47 * Xsize)
#define XCOL49  (48 * Xsize)
#define XCOL50  (49 * Xsize)

#define XRIGHT 799  // Extreme right of screen.

/*******************************************************************************************************************
 * These definitions are for 15 Rows of text when the display is in the Landscape mode. (X = 800, Y = 480).
 * Font size is 1 or 16 wide x 32 high pixels.
 *******************************************************************************************************************/
#define YTOP 0  // Top of screen.

#define YLINE1 0
#define YLINE2  Ysize
#define YLINE3  (2 * Ysize)
#define YLINE4  (3 * Ysize)
#define YLINE5  (4 * Ysize)
#define YLINE6  (5 * Ysize)
#define YLINE7  (6 * Ysize)

#define YMIDDLE 239  // Middle of the screen vertically
#define YLINE8  (7 * Ysize)
#define YLINE9  (8 * Ysize)
#define YLINE10  (9 * Ysize)
#define YLINE11  (10 * Ysize)
#define YLINE12  (11 * Ysize)
#define YLINE13  (12 * Ysize)
#define YLINE14  (13 * Ysize)
#define YLINE15  (14 * Ysize)

#define YBOTTOM 479  // Bottom row of screen.

#endif


#if defined(TFTM101)
    #define X_CONST 1024  // These setting are for the 10.1" display (ER-TFTM101-5) 1024 x 600 pixels.
    #define Y_CONST 600  // These setting are for the 10.1" display (ER-TFTM101-5) 1024 x 600 pixels.
    #define RA8875_FONTSCALE    0  // Font size is 1 or 16 wide x 32 high pixels.

    #if (RA8875_FONTSCALE == 0)
        #define Xsize 8
        #define Ysize 16

    #elif (RA8875_FONTSCALE == 1)
        #define Xsize 16
        #define Ysize 32

    #elif (RA8875_FONTSCALE == 2)
        #define Xsize 32
        #define Ysize 64

    #elif (RA8875_FONTSCALE == 3)
        #define Xsize 64
        #define Ysize 128
    #endif  // #if RA8875_FONTSCALE == 0

    #define YnumberOfRows (Y_CONST / Ysize)
    #define XnumberOfColumns (X_CONST / Xsize)


    /*******************************************************************************************************************
    * These definitions are for 50 Rows of text when the display is in the Landscape mode. (X = 1024, Y = 600).
    * Font size is 0 or 8 wide x 16 high pixels.
    *******************************************************************************************************************/
    #define XLEFT    0  // Left side of screen.

    #define XCOL1 0
    #define XCOL2  Xsize
    #define XCOL3  (2 * Xsize)
    #define XCOL4  (3 * Xsize)
    #define XCOL5  (4 * Xsize)
    #define XCOL6  (5 * Xsize)
    #define XCOL7  (6 * Xsize)
    #define XCOL8  (7 * Xsize)
    #define XCOL9  (8 * Xsize)
    #define XCOL10  (9 * Xsize)
    #define XCOL11  (10 * Xsize)
    #define XCOL12  (11 * Xsize)
    #define XCOL13  (12 * Xsize)
    #define XCOL14  (13 * Xsize)
    #define XCOL15  (14 * Xsize)
    #define XCOL16  (15 * Xsize)
    #define XCOL17  (16 * Xsize)
    #define XCOL18  (17 * Xsize)
    #define XCOL19  (18 * Xsize)
    #define XCOL20  (19 * Xsize)
    #define XCOL21  (20 * Xsize)
    #define XCOL22  (21 * Xsize)
    #define XCOL23  (22 * Xsize)
    #define XCOL24  (23 * Xsize)

    #define XCOL25  (24 * Xsize)
    #define XCOL26  (25 * Xsize)
    #define XCOL27  (26 * Xsize)
    #define XCOL28  (27 * Xsize)
    #define XCOL29  (28 * Xsize)
    #define XCOL30  (29 * Xsize)
    #define XCOL31  (30 * Xsize)
    #define XCOL32  (31 * Xsize)
    #define XCOL33  (32 * Xsize)
    #define XCOL34  (33 * Xsize)
    #define XCOL35  (34 * Xsize)
    #define XCOL36  (35 * Xsize)
    #define XCOL37  (36 * Xsize)
    #define XCOL38  (37 * Xsize)
    #define XCOL39  (38 * Xsize)
    #define XCOL40  (39 * Xsize)
    #define XCOL41  (40 * Xsize)
    #define XCOL42  (41 * Xsize)
    #define XCOL43  (42 * Xsize)
    #define XCOL44  (43 * Xsize)
    #define XCOL45  (44 * Xsize)
    #define XCOL46  (45 * Xsize)
    #define XCOL47  (46 * Xsize)
    #define XCOL48  (47 * Xsize)
    #define XCOL49  (48 * Xsize)
    #define XCOL50  (49 * Xsize)
    #define XCOL51  (50 * Xsize)
    #define XCOL52  (51 * Xsize)
    #define XCOL53  (52 * Xsize)
    #define XCOL54  (53 * Xsize)
    #define XCOL55  (54 * Xsize)
    #define XCOL56  (55 * Xsize)
    #define XCOL57  (56 * Xsize)
    #define XCOL58  (57 * Xsize)
    #define XCOL59  (58 * Xsize)
    #define XCOL60  (59 * Xsize)
    #define XCOL61  (60 * Xsize)
    #define XCOL62  (61 * Xsize)
    #define XCOL63  (62 * Xsize)
    #define XCOL64  (63 * Xsize)

    #define XMIDDLE 511  // Middle of the screen horizonal.

    #define XCOL65  (64 * Xsize)
    #define XCOL66  (65 * Xsize)
    #define XCOL67  (66 * Xsize)
    #define XCOL68  (67 * Xsize)
    #define XCOL69  (68 * Xsize)
    #define XCOL70  (69 * Xsize)
    #define XCOL71  (70 * Xsize)
    #define XCOL72  (71 * Xsize)
    #define XCOL73  (72 * Xsize)
    #define XCOL74  (73 * Xsize)
    #define XCOL75  (74 * Xsize)
    #define XCOL76  (75 * Xsize)
    #define XCOL77  (76 * Xsize)
    #define XCOL78  (77 * Xsize)
    #define XCOL79  (78 * Xsize)
    #define XCOL80  (79 * Xsize)
    #define XCOL81  (80 * Xsize)
    #define XCOL82  (81 * Xsize)
    #define XCOL83  (82 * Xsize)
    #define XCOL84  (83 * Xsize)
    #define XCOL85  (84 * Xsize)
    #define XCOL86  (85 * Xsize)
    #define XCOL87  (86 * Xsize)
    #define XCOL88  (87 * Xsize)
    #define XCOL89  (88 * Xsize)
    #define XCOL90  (89 * Xsize)
    #define XCOL91  (90 * Xsize)
    #define XCOL92  (91 * Xsize)
    #define XCOL93  (92 * Xsize)
    #define XCOL94  (93 * Xsize)
    #define XCOL95  (94 * Xsize)
    #define XCOL96  (95 * Xsize)
    #define XCOL97  (96 * Xsize)
    #define XCOL98  (97 * Xsize)
    #define XCOL99  (98 * Xsize)
    #define XCOL100  (99 * Xsize)
    #define XCOL101  (100 * Xsize)
    #define XCOL102  (101 * Xsize)
    #define XCOL103  (102 * Xsize)
    #define XCOL104  (103 * Xsize)
    #define XCOL105  (104 * Xsize)
    #define XCOL106  (105 * Xsize)
    #define XCOL107  (106 * Xsize)
    #define XCOL108  (107 * Xsize)
    #define XCOL109  (108 * Xsize)
    #define XCOL110  (109 * Xsize)
    #define XCOL111  (110 * Xsize)
    #define XCOL112  (111 * Xsize)
    #define XCOL113  (112 * Xsize)
    #define XCOL114  (113 * Xsize)
    #define XCOL115  (114 * Xsize)
    #define XCOL116  (115 * Xsize)
    #define XCOL117  (116 * Xsize)
    #define XCOL118  (117 * Xsize)
    #define XCOL119  (118 * Xsize)
    #define XCOL120  (119 * Xsize)
    #define XCOL121  (120 * Xsize)
    #define XCOL122  (121 * Xsize)
    #define XCOL123  (122 * Xsize)
    #define XCOL124  (123 * Xsize)
    #define XCOL125  (124 * Xsize)
    #define XCOL126  (125 * Xsize)
    #define XCOL127  (126 * Xsize)
    #define XCOL128  (127 * Xsize)
    #define XRIGHT 1023  // Extreme right of screen.

/*******************************************************************************************************************
 * These definitions are for 15 Rows of text when the display is in the Landscape mode. (X = 1024, Y = 600).
 * Font size is 0 or 8 wide x 16 high pixels.
 *******************************************************************************************************************/
#define YTOP 0  // Top of screen.

#define YLINE1 0
#define YLINE2  Ysize
#define YLINE3  (2 * Ysize)
#define YLINE4  (3 * Ysize)
#define YLINE5  (4 * Ysize)
#define YLINE6  (5 * Ysize)
#define YLINE7  (6 * Ysize)
#define YLINE8  (7 * Ysize)
#define YLINE9  (8 * Ysize)
#define YLINE10  (9 * Ysize)
#define YLINE11  (10 * Ysize)
#define YLINE12  (11 * Ysize)
#define YLINE13  (12 * Ysize)
#define YLINE14  (13 * Ysize)
#define YLINE15  (14 * Ysize)
#define YLINE16  (15 * Ysize)
#define YLINE17  (16 * Ysize)
#define YLINE18  (17 * Ysize)

#define YMIDDLE 299  // Middle of the screen vertically

#define YLINE19  (18 * Ysize)
#define YLINE20  (19 * Ysize)
#define YLINE21  (20 * Ysize)
#define YLINE22  (21 * Ysize)
#define YLINE23  (22 * Ysize)
#define YLINE24  (23 * Ysize)
#define YLINE25  (24 * Ysize)
#define YLINE26  (25 * Ysize)
#define YLINE27  (26 * Ysize)
#define YLINE28  (27 * Ysize)
#define YLINE29  (28 * Ysize)
#define YLINE30  (29 * Ysize)
#define YLINE31  (30 * Ysize)
#define YLINE32  (31 * Ysize)
#define YLINE33  (32 * Ysize)
#define YLINE34  (33 * Ysize)
#define YLINE35  (34 * Ysize)
#define YLINE36  (35 * Ysize)
#define YLINE37  (36 * Ysize)

#define YBOTTOM 599  // Bottom row of screen.

#endif


/*******************************************************************************************************************
 * Setting the sync provider to use for the Teensy.
 *
 *******************************************************************************************************************/
//#define timeSyncProviderTeensy
//#define timeSyncProviderGPS
#define timeSyncProviderDS3231
//#define timeSyncProviderInternet


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             Project/System dependent defines
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

/**********************************************************************************************************************
* CPU Restart macro
* 
**********************************************************************************************************************/
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

/**********************************************************************************************************************
* MIN/MAX/ABS macros
*
**********************************************************************************************************************/
#define MIN(a,b)            ((a<b)?(a):(b))
#define MAX(a,b)            ((a>b)?(a):(b))
#define ABS(x)                ((x>0)?(x):(-x))


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             UART Configurations
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             Rprintf Defines
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

#define RPRINTF_COMPLEX

#define RPRINTF_FLOAT


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                                             Variables
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/


//Do not add code below this line


#endif
