/*************************************************************** 
 * TestingTFT.cpp, TestingTFT.h 
 * 
 * Basic graphics test for RA8876 based display
 * It also has a clock display.
 ***************************************************************/
#include <TestingTFT.h>


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                               Instantiate classes                                                             **
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

// This is the instantiation for the BME680 temp, pressure, humidity, air quality sensor.
// max 2 sensors on one bus, addresses can be:
// 118 or 119 Decimal == 0x76 -> 0x77  Hexidecimal.
// i2c speed can be 100kHz, 400kHz or 1Mhz.
BME680_Class BME680Sensor;  ///< Create an instance of the BME680 class.

// VALID ADDRESSES for MCP9808 are:
// max 8 sensors on one bus, addresses can be:
// 24 -> 31 Decimal == 0x18 -> 0x1F Hexidecimal.
// i2c speed can be 100kHz or 400kHz.
MCP9808 mcp9808TempSen(0x18);  // Create instance of the MCP9808 class.

// VALID ADDRESSES for TMP117 are:
// max 4 sensors on one bus, addresses can be:
// 0x48 -> 0x4B Hexidecimal.
TMP117 tmp117TempSen(0x48, &Wire2);  // Create instance of the TMP117 class.

// This is the instantiation for the RA8876 TFT display (10" or 7").
/// This is the form of the constructor:
/// RA8876_t3(CSp = 10, RSTp = 14, mosi_pin = 11, sclk_pin = 13, miso_pin = 12);
///
#define RA8876_CS 10
#define RA8876_RESET 14
#define BACKLITE 9 //External backlight control connected to this Arduino pin
RA8876_t3 tft = RA8876_t3(RA8876_CS, RA8876_RESET); //Using standard SPI pins

// This is the instantiation for the DS3231 Clock Chip.
DS3231RTC DS3231Clock;

// This is the intantiation for the Ublox GPS module attached to Serial port 1.
//   This Class is the uBlox GPS receiver Class.
//   Needs:
//     Hardware serial port the uBlox GPS Receiver is connected to Port (1, 2, 3) on Teensy 3.2.
// fix Type: 0: no fix, 1: dead reckoning only, 2: 2D-fix, 3: 3D-fix, 4: GNSS + dead reckoning combined, 5: time only fix
SFE_UBLOX_GNSS myGNSS;

bool GPS_GNSS_Present = false;


// This is the instantiation for the Periodic Timer class.
//PeriodicTimer t1; // generate a timer from the pool (Pool: 2xGPT, 16xTMR(QUAD), 20xTCK)

/*******************************************************************************************************************
 * IntervalTimer uses interrupts to call a function at a precise timing interval.
 * IntervalTimer will call your function from interrupt context. Because it can interrupt your program at
 * any moment, special design is necessary to share data with the rest of your program.
 * Up to 4 IntervalTimer objects may be active simultaneuously.  IntervalTimer is supported only on 32 bit Teensies.
 *
 *******************************************************************************************************************/
// Create and Instantiation three IntervalTimer objects.
IntervalTimer oneSecondTimer;               // One Second Timer Interrupt.
IntervalTimer oneHundredMillisecTimer;      // One Hundre Millisecond Timer Interrupt.
IntervalTimer fiveMillisecTimer;            // Five Millisecond Timer Interrupt.


/*******************************************************************************************************************
 * This is the Watch Dog Timer for the Teensy 4.x
 *
 *******************************************************************************************************************/
WDT_T4<WDT1> wdt;

WDT_timings_t config;


// This instantiates the class for reading and writing to the RTC EEPROM (AT24C32) 0 -> 0xFFF.
AT24C32 eepromMemory(7);


/*******************************************************************************************************************
 * This is LittleFS file system created in the QSPI RAM for the Teensy 4.x
 * This is used to store an entire days logging data before moving it to the SD card at Midnight
 *******************************************************************************************************************/
LittleFS_RAM qspiRamFileSystem;  // This is the QSPI Ram Chip soldered to the back of the Teensy 4.1.
EXTMEM char qspiRamDiskBuffer[1024 * 1024];  // This allocates 1Mb of the PSRAM for the RAM disk.

File qspiRamDataFile;  // This File object is for the RAMDisk Destination File where the data will be stored.
File qspiRamDirectory;  // This file object is for the RAMDisk Destination Directory where the Destination File would be created.

uint32_t qspiRamFileSystem_SIZE = (1024 * 1024); // probably more than enough...
uint64_t fileLength = 0; // This is the size of the file in the File System, used for status checks.

// Various Globals
const uint32_t lowOffset = 'a' - 'A';
const uint32_t lowShift = 13;
uint32_t errsLFS = 0, warnLFS = 0; // Track warnings or Errors
uint32_t lCnt = 0, LoopCnt = 0; // loop counters
uint64_t rdCnt = 0, wrCnt = 0; // Track Bytes Read and Written
int filecount = 0;
int loopLimit = 0; // -1 continuous, otherwise # to count down to 0
bool pauseDir = false;  // Start Pause on each off
bool showDir =  false;  // false Start Dir on each off
bool bDirVerify =  false;  // false Start Dir on each off
bool bWriteVerify = true;  // Verify on Write Toggle
bool bAutoFormat =  false;  // false Auto formatUnused() off
bool bCheckFormat =  false;  // false CheckFormat
bool bCheckUsed =  false;  // false CheckUsed
uint32_t res = 0; // for formatUnused

uint32_t record_count = 0;
bool write_data = false;

char FileNameChars[50];  // Declare a Char Array for the File Name of the LittleFS data file.
int NumChars = 0;  // This is the Number of characters in the file name.

char NewFileNameChars[50];
int NewFileNameNumChars = 0;  // This is the Number of characters in the new file name.
int NewfileNameDay = 1;         // This is a temporary fix to change the day of the month for testing.

const char RamDiskName[] = "TeensyRAMDisk";

char dataFileLine[500];  // This character array is for each record in the data file (Excel Row).
int numOfCharsInRecord = 0;  // This is the number of characters in each record of the data file (Excel Row).
uint32_t NumOfCharsWritten = 0;  // This is the Number of Characters written to the file when printing to the file.
uint64_t SizeOfCurrentFile = 0;

bool noLittleFS;  // This flag indicates whether or not the LittleFS file system is present and initialized.
bool diskFull = false;
bool success = 0;
volatile bool qspiRamDataFilePresent = false;

uint32_t spaceAvailToWrite = 0;
bool noDir = false;

uint64_t charsSpaceRemaingInFileSys = 0;



/*******************************************************************************************************************
 * This is MTP (Mass Transfer Protocol) for the Teensy 4.x
 *
 *******************************************************************************************************************/
// Add in MTPD objects
MTPStorage storage;


/**************************************************************************************************************************************
 * This is the SD class for the Teensy 4.x
 *
 * SDPath Paths and Working Directories:
 *
 *  Relative paths in %SdFat are resolved in a manner similar to Windows DOS and Most UNIX/Linux Systems.
 *
 *  Each instance of SdFat32, SdExFat, and SdFs has a current directory.  
 *  This directory is called the volume working directory, (vwd), Initially this directory is the root directory for the volume.  
 *
 *  The volume working directory is changed by calling the chdir(path).
 *
 *  The call sd.chdir("/2014") will change the volume working directory for sd to "/2014", assuming "/2014" exists.
 *  
 *  Relative paths for member functions are resolved by starting at the volume working directory.
 *  
 *  For example, the call sd.mkdir("April") will create the directory "/2014/April" assuming the volume working directory is "/2014".
 *  
 *  There is a current working directory, (cwd), that is used to resolve paths for file.open() calls.
 *  
 *  For a single SD card, the current working directory is always the volume working directory for that card.
 *  
 *  For multiple SD cards the current working directory is set to the volume working directory of a card by calling the chvol() member function.
 *  
 *  The chvol() call is like the Windows \<drive letter>: command.
 *
 *  The call sd2.chvol() will set the current working directory to the volume working directory for sd2.
 *  
 *  If the volume working directory for sd2 is "/music" the call file.open("BigBand.wav", O_READ);  will open "/music/BigBand.wav" on sd2. 
 * 
 ****************************************************************************************************************************************/
#define USE_BUILTIN_SDCARD

// Add in SD objects
SDClass sdSDIO;

File rootDirectory;                 // This file object is for the SD Card Destination Root directory (/).
File SDCardDataFile;                // This File object is for the SD Card Destination File where the data will be stored.
File sdCardDestinationDirectory;    // This File object is for the SD Card Destination Directory where the Destination File will be created.

const char SDDataDirectory[] = "EnvironmentalData";
char sdCardDestDirectory[25];  // This is the Destination directory on the SD Card with a / appended.
volatile bool sdCardRootDirectoryPresent = false;
volatile bool sdCardDestDirectoryPresent = false;

char sdCardDestFileName[25];  // This is the Destination File Name on the SD Card.
const char sdCardDiskName[] = "TeensySDCard";
volatile bool sdCardDestFileNamePresent = false;
volatile bool sdCardDestDirectoryOpen = false;

int myfs_index = 0;
uint32_t index_sdio_storage = -1;
uint32_t index_ramDisk_storage = -1;
bool sdio_previously_present;
bool ramDisk_previously_present = 0; 


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/// Prototypes
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

// NONE


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/// Define global variables, structures and constants:
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

/// These variables are for the eeprom Memory, they are the various locations in EEPROM where important info is stored.
const uint16_t rainMemoryLocation = 0x100;
const uint16_t TempCalOffsetLocation = 0x110;
const uint16_t TimeZoneSettingLocation = 0x130;
const uint16_t SleepTimeEEPROMMemoryLocation = 0x140;
const uint16_t WakeTimeEEPROMMemoryLocation = 0x150;
const uint16_t DayTimeScreenIntensityEEPROMMemoryLocation = 0x160;
const uint16_t NiteTimeScreenIntensityEEPROMMemoryLocation = 0x170;

/// These variables are for the measurement of the system sensors (MCP9808, BME680, and the TMP117).
volatile bool noBME680 = true, noMCP9808 = true, noTMP117 = true;
volatile float tmp117Temperature = 0;  // This is the temperature read from the TMP117 Temp sensor.

volatile bool BME680CalibrateFlag = false;
volatile bool MCP9808CalibrateFlag = false;

uint16_t StatuslineDisplayTime = 200;  // Setup status line display time delay.
uint16_t MainLoopStatuslineDisplayTime = 50;

/*******************************************************************************************************************
 * These variables are for the TimeZone functions.
 *******************************************************************************************************************/
// New Zealond Standard Time Zone ()
TimeChangeRule NDST = {"nzDST", First, Sun, Oct, 2, -660};   // UTC - 11 hours
TimeChangeRule NST = {"nwzST", First, Sun, Apr, 3, 720};      // UTC + 12 hours
Timezone nzTZ(NDST, NST);

// Solomon Standard Time Zone ()
TimeChangeRule SDST = {"slDST", First, Sun, Oct, 2, 720};    // UTC + 12 hours
TimeChangeRule SST = {"slnST", First, Sun, Apr, 3, 660};      // UTC + 11 hours
Timezone soIT(SDST, SST);

// Australia Central Time Zone ()
TimeChangeRule ACDST = {"acDST", First, Sun, Oct, 2, 630};  // UTC + 10:30 hours
TimeChangeRule ACT = {"aucST", First, Sun, Apr, 3, 570};      // UTC + 9:30 hours
Timezone ausCT(ACDST, ACT);

// Australia Eastern Time Zone (Sydney, Melbourne)
TimeChangeRule AEDST = {"aeDST", First, Sun, Oct, 2, 660};  // UTC + 11 hours
TimeChangeRule AET = {"aueST", First, Sun, Apr, 3, 600};      // UTC + 10 hours
Timezone ausET(AEDST, AET);

// Japan Standard Time (Tokyo).
TimeChangeRule JDST = {"jpDST", First, Sun, Oct, 2, 600};    // UTC + 10 hours.
TimeChangeRule JST = {"jpnST", First, Sun, Oct, 2, 540};      // UTC + 9 hours.
Timezone jpTZ(JDST, JST);

// China Standard Time (Bejing).
TimeChangeRule CTDST = {"chDST", First, Sun, Oct, 2, 540};  // UTC + 9 hours.
TimeChangeRule CTT = {"chiST", First, Sun, Oct, 2, 480};      // UTC + 8 hours.
Timezone chTZ(CTDST, CTT);

// Vietnam Standard Time (Hanoi).
TimeChangeRule VDST = {"vnDST", First, Sun, Oct, 2, 480};    // UTC + 8 hours.
TimeChangeRule VNST = {"vtnST", First, Sun, Oct, 2, 420};      // UTC + 7 hours.
Timezone vmTZ(VDST, VNST);

// Bangladesh Standard Time ().
TimeChangeRule BSDST = {"bgDST", First, Sun, Oct, 2, 420};  // UTC + 7 hours.
TimeChangeRule BST = {"bghST", First, Sun, Oct, 2, 360};      // UTC + 6 hours.
Timezone bgTZ(BSDST, BST);

// India Standard Time (New Delhi).
TimeChangeRule ISDST = {"inDST", First, Sun, Oct, 2, 390};  // UTC + 6:30 hours.
TimeChangeRule IST = {"indST", First, Sun, Oct, 2, 330};      // UTC + 5:30 hours.
Timezone inTZ(ISDST, IST);

// Pakistan Lahore Standard Time ().
TimeChangeRule PLDST = {"plDST", First, Sun, Oct, 2, 360};  // UTC + 6 hours.
TimeChangeRule PLT = {"pltST", First, Sun, Oct, 2, 300};      // UTC + 5 hours.
Timezone pkTZ(PLDST, PLT);

// Near East Standard Time ().
TimeChangeRule NEDST = {"neDST", First, Sun, Oct, 2, 540};  // UTC + 5 hours.
TimeChangeRule NET = {"netST", First, Sun, Oct, 2, 480};      // UTC + 4 hours.
Timezone neTZ(NEDST, NET);

// Middle East Standard Time ().
TimeChangeRule MEDST = {"meDST", First, Sun, Oct, 2, 270};  // UTC + 4:30 hours.
TimeChangeRule MET = {"metST", First, Sun, Oct, 2, 210};      // UTC + 3:30 hours.
Timezone meTZ(MEDST, MET);

// Eastern Afica Standard Time ().
TimeChangeRule EADST = {"eaDST", First, Sun, Oct, 2, 240};  // UTC + 4 hours.
TimeChangeRule EAT = {"eatST", First, Sun, Oct, 2, 180};      // UTC + 3 hours.
Timezone eaTZ(EADST, EAT);

// Moscow Standard Time (MSK, does not observe DST)
TimeChangeRule msk = {"ruMSK", Last, Sun, Mar, 1, 180};       // UTC + 3, Russia Standard Time
Timezone tzMSK(msk);

// Eastern European Time (Athens, Kiev)
TimeChangeRule EEST = {"EEDST", Last, Sun, Mar, 2, 180};     // UTC + 3, Eastern European Summer Time
TimeChangeRule EET = {"eetST", Last, Sun, Oct, 3, 120};      // UTC + 2, Eastern European Standard Time
Timezone eeTZ(EEST, EET);

// Central European Time (Frankfurt, Paris, Rome)
TimeChangeRule CEST = {"ceDST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"cetST", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone ceTZ(CEST, CET);

// United Kingdom (London, Belfast)
TimeChangeRule BDST = {"ukDST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"ukGMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone UK(BDST, GMT);

// UTC
TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};     // UTC
Timezone UTC(utcRule);

// Eastern Atlantic Time Zone ( Cape Verde )
TimeChangeRule eaDST = {"eaDST", Second, Sun, Mar, 2, -120}; // UTC - 2 hours
TimeChangeRule eaT = {"eatST", First, Sun, Nov, 2, -60};       // UTC - 1 hours
Timezone ealtTZ(eaDST, eaT);

// Brazil Eastern Standard Time (Fortaleza).
TimeChangeRule BEDST = {"BEDST", First, Sun, Oct, 2, -180};  // UTC - 3 hours.
TimeChangeRule BET = {"BrEST", First, Sun, Oct, 2, -120};      // UTC - 2 hours.
Timezone beTZ(BEDST, BET);

// Argentina Time Zone (Buenos Aires, Montevideo, Rio de Janerio )
TimeChangeRule AGT = {"ArGST", First, Sun, Jan, 2, -180};         // UTC - 3 hours
Timezone agTZ(AGT,AGT);

// Atlantic Time Zone ( San Juan, Puerto Rico )
TimeChangeRule ATDST = {"ATDST", Second, Sun, Mar, 2, -180};    // UTC - 3 hours
TimeChangeRule ATST = {"ATST", First, Sun, Nov, 2, -240};      // UTC - 4 hours
Timezone atTZ(ATDST, ATST);

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDST", Second, Sun, Mar, 2, -240};     // UTC - 4 hours
TimeChangeRule usEST = {"usEST", First, Sun, Nov, 2, -300};       // UTC - 5 hours
Timezone usET(ATDST, usEST);

// US Central Time Zone (Chicago, Houston)
TimeChangeRule usCDT = {"CDST", Second, Sun, Mar, 2, -300};     // UTC - 5
TimeChangeRule usCST = {"usCST", First, Sun, Nov, 2, -360};       // UTC - 6
Timezone usCT(usCDT, usCST);

// US Mountain Time Zone (Denver, Salt Lake City)
TimeChangeRule usMDT = {"MDST", Second, Sun, Mar, 2, -360};     // UTC - 6
TimeChangeRule usMST = {"usMST", First, Sun, Nov, 2, -420};       // UTC - 7
Timezone usMT(usMDT, usMST);

// Arizona is US Mountain Time Zone but does not use DST
Timezone usAZ(usMST);  // UTC - 7

// US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDST", Second, Sun, Mar, 2, -420};     // UTC - 7
TimeChangeRule usPST = {"usPST", First, Sun, Nov, 2, -480};       // UTC - 8
Timezone usPT(usPDT, usPST);

// Alaska Time Zone (Nome)
TimeChangeRule ADST = {"ADST", Second, Sun, Mar, 2, -480};      // UTC - 8
TimeChangeRule AST = {"usAST", First, Sun, Nov, 2, -540};         // UTC - 9
Timezone alTZ(ADST, AST);

// Hawaii is Hawaiian Time Zone but does not use DST
TimeChangeRule usHST = {"usHST", Last, Sun, Mar, 1,  -600};       // UTC - 10 (no daylight saving time).
Timezone usHT(usHST);

// Midway Islands Time Zone ()
TimeChangeRule MIDST = {"MIDST", Second, Sun, Mar, 2, -600};    // UTC - 10
TimeChangeRule MIT = {"MIIST", First, Sun, Nov, 2, -660};         // UTC - 11
Timezone miTZ(MIDST, MIT);


Timezone* timezones[] = { &nzTZ, &soIT, &ausCT, &ausET, &jpTZ, &chTZ, &vmTZ, &bgTZ, &inTZ, &pkTZ, &neTZ, &meTZ, &eaTZ, &tzMSK,
                 &eeTZ, &ceTZ, &UK, &UTC, &ealtTZ, &beTZ, &agTZ, &atTZ, &usET, &usCT, &usMT, &usAZ, &usPT, &alTZ, &usHT, &miTZ };  // 31 total time zones

Timezone* tz;  //pointer to the time zone.
TimeChangeRule* tcr;  //pointer to the time change rule struct, use to get TZ abbrev
uint8_t tzIndex = 20;  //indexes the timezones[] array, start with Argentina Time.

bool UTCTimeCorrected = false;


/***********************************************************************************************************************

***********************************************************************************************************************/

/*******************************************************************************************************************
* This Variable is set to the GPS Completion Code to indicate whether or not the GPS data has been properly received
* or not.
*******************************************************************************************************************/
gpsCompletionCodes gpsCodes;  // These are the Return codes from the GPS device to determine errors or good data.

/*******************************************************************************************************************
* This Flag is set to indicate it is time to update the Master Clock (DS3231) with the time from the GPS Clock.
*******************************************************************************************************************/
volatile bool updateGPSFlag = false;

/*******************************************************************************************************************
* This Variable is set to the color of the Dot at the Top Middle of the TFT to indicate whether of not the DS3231
* Clock has been set to the GPS Clock accuracy.
*******************************************************************************************************************/
volatile dotColorSelection statusDotColor = DOT_NONE;  // This varible holds the color of the RTC corrected by GPS Status Dot.

/*******************************************************************************************************************
* This number is set to the number of times the GPS receive code gets a BAD_PACKET error.
*******************************************************************************************************************/
volatile uint8_t numberOfBadPackets = 0;

/*******************************************************************************************************************
* This flag is to inform the system that this is the first run of the GPS system.
*******************************************************************************************************************/
volatile bool firstRun = true;

/*******************************************************************************************************************
* This flag is to inform the system that this is the first run of the GPS Start Up Routine.
*******************************************************************************************************************/
volatile bool initial_Startup = true;  // The first time set true.


///< Forward function declaration with default value for sea level.
float altitude(const int32_t press, const float seaLevel = 1013.25);


/*******************************************************************************************************************
 * Usage with 32-Bit "time_t"
 * Time uses a special time_t variable type, which is the number of seconds elapsed since 1970.
 * Using time_t lets you store or compare times as a single number, rather that dealing with 6 numbers and details
 * like the number of days in each month and leap years.
 *
 *******************************************************************************************************************/
time_t lastTimeSetByClock = 0;  // This time is set by the clock reading routine.
tmElements_t tm;                // Create a TimeElements struct for the time data to be fetched from the DS3231 IC.
DateTimeFields DateTimeFlds;    // Create a DateTimeFields struct for the time and date data, this is slightly different than TimeElements.
/*
When using the new DateTimeFields in MTP, please keep in mind it is slightly different than TimeElements. It follows C
library struct tm format. The month is 0-based, not 1-12 as in TimeElements. The year is offset by 1900, not 1970.

Code:
 DateTimeFields follows C library "struct tm" convention, but uses much less memory
*/

/*
typedef struct 
{
        uint8_t sec;   // 0-59
        uint8_t min;   // 0-59
        uint8_t hour;  // 0-23
        uint8_t wday;  // 0-6, 0=sunday
        uint8_t mday;  // 1-31
        uint8_t mon;   // 0-11
        uint8_t year;  // 70 -> 206, 70=1970, 206=2106
} DateTimeFields;
*/


/*******************************************************************************************************************
 * This Variable is for determining whether or not the Teensy Clock has been set (true) or not (false).
 *******************************************************************************************************************/
bool pctime = false;


/*******************************************************************************************************************
 * These definitions are for the Clock functions.
 *******************************************************************************************************************/
// Define array for Clockface:
int16_t clockPos[3] = {0, 0, 50}; //x size, y size, r Radius

// Define array for Time in hours, minutes, and seconds.
uint8_t currentTime[3] = {12, 30, 0}; //hh, mm, ss

// Define a variable for the Day of the Week.
uint8_t currentDayOfWeek = 0;

// Define array for Day, Month, and Year.
int currentDate[3] = {1, 1, 2022}; // Day, Month, Year

// Define colors for background, face, hh hand, mm hand, and ss hand.
const uint16_t clockColors[5] = {BLACK, BLACK, CYAN, GREEN, RED};

// Define array with positions for hour hand X, hour hand Y, minute hand X, minute hand Y, second hand X, second hand Y
uint16_t oldPos[6] = {0, 0, 0, 0, 0, 0};

bool century = false;
bool h12Flag = false;
bool pmFlag = false;
byte alarmDay, alarmHour, alarmMinute, alarmSecond, alarmBits;
bool alarmDy, alarmH12Flag, alarmPmFlag;

volatile byte timeToDimTheScreen = 23;      // This is the time of day to dim the screen for night viewing.
volatile byte TimeToWakeTheScreen = 7;      // This is the time of day to set the screen to full brightness.
volatile bool coldStart = false;
volatile bool nightTimeFlag = false;        // This means it is day time, set true for night time.
volatile bool itsMidnight = false;          // This flag means that the clock just struck midnight.
volatile bool finishedPreferences = false;  // This flag means that the preferences sections has just finished.
volatile bool startUp = true;               // This flag means that the system has just started up.
volatile bool colorChangeOccured = false;    // This flag means that the screen color changed just occured.
volatile bool preferencesInProgressFlag = false;

volatile bool fiveMSFlag = false;
volatile bool oneHundredMSFlag = false;
volatile bool oneSecFlag = false;
volatile uint16_t twoSecCount = 0;
volatile uint16_t tenSecCount = 0;
volatile uint16_t thirtySecCount = 0;
volatile uint16_t twoMinuteCount = 0;
volatile uint16_t twelveHourCount = 0;
//volatile uint32_t twentyFourHourCount = 0;

uint16_t textXsize = 0;
uint16_t fontXsize = 0;  // This is the Horizontal size of the current Font.
uint16_t Twidth = 0;
	
uint16_t textYsize = 0;
uint16_t fontYsize = 0;  // This is the Vertical size of the current font.
uint16_t Theight = 0;

/*
#define BLACK		0x0000
#define WHITE		0xffff
#define RED			0xf800
#define LIGHTRED	0xfc10
#define CRIMSON		0x8000
#define GREEN		0x07e0
#define PALEGREEN	0x87f0
#define DARKGREEN	0x0400
#define BLUE		0x001f
#define LIGHTBLUE	0x051f
#define SKYBLUE		0x841f
#define DARKBLUE	0x0010
#define YELLOW		0xffe0
#define LIGHTYELLOW	0xfff0
#define DARKYELLOW	0x8400 // mustard
#define CYAN		0x07ff
#define LIGHTCYAN	0x87ff
#define DARKCYAN	0x0410
#define MAGENTA		0xf81f
#define VIOLET		0xfc1f
#define BLUEVIOLET	0x8010
#define ORCHID		0xA14
*/

// Array of Simple RA8876 Basic Colors
PROGMEM uint16_t myColors[] =
{
	0x0000,
	0xffff,
	0xf800,
	0xfc10,
	0x8000,
	0x07e0,
	0x87f0,
	0x0400,
	0x001f,
	0x051f,
	0x841f,
	0x0010,
	0xffe0,
	0xfff0,
	0x8400,
	0x07ff,
	0x87ff,
	0x0410,
	0xf81f,
	0xfc1f,
	0x8010,
	0xA145
};

int w, h;  // These are the defined width and height that is used to calculate the size of the TFT screen.


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
///                               Define Functions that will be used.                                             **
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/

/*******************************************************************************************************************
 * These are the defintions of the UBX_NAV_PVT_data_t data Struct:
 * 
 * iTOW; // GPS time of week of the navigation epoch: ms
 * year; // Year (UTC)
 * month; // Month, range 1..12 (UTC)
 * day;   // Day of month, range 1..31 (UTC)
 * hour;  // Hour of day, range 0..23 (UTC)
 * min;   // Minute of hour, range 0..59 (UTC)
 * sec;   // Seconds of minute, range 0..60 (UTC)
 * 
 * validDate : 1;     // 1 = valid UTC Date
 * validTime : 1;     // 1 = valid UTC time of day
 * fullyResolved : 1; // 1 = UTC time of day has been fully resolved (no seconds uncertainty).
 * validMag : 1;      // 1 = valid magnetic declination
 *
 * tAcc;   // Time accuracy estimate (UTC): ns
 * nano;    // Fraction of second, range -1e9 .. 1e9 (UTC): ns
 * fixType; // GNSSfix Type:
 *                          0: no fix
 *                          1: dead reckoning only
 *                          2: 2D-fix
 *                          3: 3D-fix
 *                          4: GNSS + dead reckoning combined
 *                          5: time only fix
 *
 * gnssFixOK : 1; // 1 = valid fix (i.e within DOP & accuracy masks)
 * diffSoln : 1;  // 1 = differential corrections were applied
 * psmState : 3;
 * headVehValid : 1; // 1 = heading of vehicle is valid, only set if the receiver is in sensor fusion mode
 * carrSoln : 2;     // Carrier phase range solution status:
                                // 0: no carrier phase range solution
                                // 1: carrier phase range solution with floating ambiguities
                                // 2: carrier phase range solution with fixed ambiguities
 *
 * reserved : 5;
 * confirmedAvai : 1; // 1 = information about UTC Date and Time of Day validity confirmation is available
 * confirmedDate : 1; // 1 = UTC Date validity could be confirmed
 * confirmedTime : 1; // 1 = UTC Time of Day could be confirmed
 *
 * numSV;    // Number of satellites used in Nav Solution
 * lon;      // Longitude: deg * 1e-7
 * lat;      // Latitude: deg * 1e-7
 * height;   // Height above ellipsoid: mm
 * hMSL;     // Height above mean sea level: mm
 * hAcc;    // Horizontal accuracy estimate: mm
 * vAcc;    // Vertical accuracy estimate: mm
 * velN;     // NED north velocity: mm/s
 * velE;     // NED east velocity: mm/s
 * velD;     // NED down velocity: mm/s
 * gSpeed;   // Ground Speed (2-D): mm/s
 * headMot;  // Heading of motion (2-D): deg * 1e-5
 * sAcc;    // Speed accuracy estimate: mm/s
 * headAcc; // Heading accuracy estimate (both motion and vehicle): deg * 1e-5
 * pDOP;    // Position DOP * 0.01
 *
 * invalidLlh : 1; // 1 = Invalid lon, lat, height and hMSL
 *
 * reserved1[5];
 * headVeh; // Heading of vehicle (2-D): deg * 1e-5
 * magDec;  // Magnetic declination: deg * 1e-2
 * magAcc; // Magnetic declination accuracy: deg * 1e-2
 *
*/

// Callback: printPVTdata will be called when new NAV PVT data arrives
// See u-blox_structs.h for the full definition of UBX_NAV_PVT_data_t
//         _____  You can use any name you like for the callback. Use the same name when you call setAutoPVTcallback
//        /                  _____  This _must_ be UBX_NAV_PVT_data_t
//        |                 /               _____ You can use any name you like for the struct
//        |                 |              /
//        |                 |              |
void printPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
    /*********************************************************************************
    * GNSSfix Type:
    *   0: no fix
    *   1: dead reckoning only
    *   2: 2D-fix
    *   3: 3D-fix
    *   4: GNSS + dead reckoning combined
    *   5: time only fix
    *********************************************************************************/
    gpsUBXDataStruct.fixType = ubxDataStruct->fixType;  // Get the fix type from the Struct.
    if (gpsUBXDataStruct.fixType == NO_FIX)
        {
            #if defined (DEBUG54)
                Serial.println(F("Fix Type: NO FIX..."));  // No fix means there is no valid data.
            #endif
            if (coldStart == true)
                {
                    coldStart = false;
                }
            gpsUBXDataStruct.valid = false;  // This means the data in the Data Struct is not valid.
            return;  // Return as there is no point in printing all the other info as it is not valid.
        }
    else
        {
            #if defined (DEBUG54)
                Serial.print(F("Fix Type: "));
                switch (gpsUBXDataStruct.fixType)
                    {
                        case NO_FIX:
                            Serial.println(F("NO FIX"));  // Print the Heading.
                            break;
                        case DEAD_RECKONING_ONLY:
                            Serial.println(F("DR ONLY"));  // Print the Heading.
                            break;
                        case TWO_D_FIX:
                            Serial.println(F("TWO_D_FIX"));  // Print the Heading.
                            break;
                        case THREE_D_FIX:
                            Serial.println(F("THREE_D_FIX"));  // Print the Heading.
                            break;
                        case GNSS_DR_COMBINED:
                            Serial.println(F("GNSS_DR_COMBINED"));  // Print the Heading.
                            break;
                        case TIME_FIX_ONLY:
                            Serial.println(F("TIME_FIX_ONLY"));  // Print the Heading.
                            break;
                                                        
                        default:
                            Serial.println(F("BAD PACKET"));  // Print the Heading.
                            break;
                    }
            #endif
        }
    // This section copies the GPS info from the library struct (which is very volitle) to the local struct.
    gpsUBXDataStruct.hour = ubxDataStruct->hour;                                // Copy the hours to local struct.
    gpsUBXDataStruct.min = ubxDataStruct->min;                                  // Copy the minutes to local struct.
    gpsUBXDataStruct.sec = ubxDataStruct->sec;                                  // Copy the seconds to local struct.
    gpsUBXDataStruct.iTOW = ubxDataStruct->iTOW % 1000;                         // Copy the milliseconds to local struct with adjustments.
    gpsUBXDataStruct.tAcc = ubxDataStruct->tAcc;                                // Copy the tAcc to local struct.
    gpsUBXDataStruct.nano = ubxDataStruct->nano;                                // Copy the nano seconds to local struct.
    gpsUBXDataStruct.month = ubxDataStruct->month;                              // Copy the month to local struct.
    gpsUBXDataStruct.day = ubxDataStruct->day;                                  // Copy the day to local struct.
    gpsUBXDataStruct.year = ubxDataStruct->year;                                // Copy the year to local struct.
    gpsUBXDataStruct.lat = ((float)ubxDataStruct->lat / 10000000l);             // Copy the latitude to local struct with adjustments.
    gpsUBXDataStruct.lon = ((float)ubxDataStruct->lon / 10000000l);             // Copy the longitude to local struct with adjustments.
    gpsUBXDataStruct.hMSL = ((float)ubxDataStruct->hMSL * 0.00328084F);         // Copy the height above mean sea level to local struct with adjustments.
    gpsUBXDataStruct.numSV = ubxDataStruct->numSV;                              // Copy the number of Sats for fix to local struct.
    gpsUBXDataStruct.validTime = ubxDataStruct->valid.bits.validTime;           // Copy the Validity Code for Time to loacl struct.
    gpsUBXDataStruct.validDate = ubxDataStruct->valid.bits.validDate;           // Copy the Validity Code for Date to local struct.
    gpsUBXDataStruct.fullyResolved = ubxDataStruct->valid.bits.fullyResolved;   // Copy the Validity Code for fully Resolved to local struct.
    gpsUBXDataStruct.gSpeed = ((float)ubxDataStruct->gSpeed  * 0.001F);         // Copy the Speed (In mm/sec, so convert to m/sec) to local struct.
    gpsUBXDataStruct.headMot = ubxDataStruct->headMot;                          // Copy the Heading of Motion to local struct.
    gpsUBXDataStruct.headVeh = ubxDataStruct->headVeh;                          // Copy the Vehicle Heading to local struct.
    gpsUBXDataStruct.magDec = ubxDataStruct->magDec;                            // Copy the Magnetic Declination to local struct.

    gpsUBXDataStruct.valid = true;  // This means the data in the Data Struct is valid.

    #if defined (DEBUG54)
        Serial.print(F("Time: ")); // Print the time
        if (gpsUBXDataStruct.hour < 10) Serial.print(F("0")); // Print a leading zero if required
        Serial.print(gpsUBXDataStruct.hour);
        Serial.print(F(":"));
        if (gpsUBXDataStruct.min < 10) Serial.print(F("0")); // Print a leading zero if required
        Serial.print(gpsUBXDataStruct.min);
        Serial.print(F(":"));
        if (gpsUBXDataStruct.sec < 10) Serial.print(F("0")); // Print a leading zero if required
        Serial.print(gpsUBXDataStruct.sec);
        Serial.print(F("."));
        if (gpsUBXDataStruct.iTOW < 100) Serial.print(F("0")); // Print the trailing zeros correctly
        if (gpsUBXDataStruct.iTOW < 10) Serial.print(F("0"));
        Serial.print(gpsUBXDataStruct.iTOW);
        Serial.println(F(" UTC or GMT"));

        Serial.print(F("mm/dd/year: ")); // Print the month
        if (gpsUBXDataStruct.month < 10) Serial.print(F("0")); // Print a leading zero if required
        Serial.print(gpsUBXDataStruct.month);
        Serial.print(F("/"));
        if (gpsUBXDataStruct.day < 10) Serial.print(F("0")); // Print a leading zero if required
        Serial.print(gpsUBXDataStruct.day);
        Serial.print(F("/"));
        Serial.println(gpsUBXDataStruct.year);

        Serial.print(F("Lat: "));
        Serial.println(gpsUBXDataStruct.lat, 6);

        Serial.print(F("Long: "));
        Serial.println( gpsUBXDataStruct.lon, 6);

        Serial.print(F("Altitude above Mean Sea Level: "));
        Serial.print( gpsUBXDataStruct.hMSL, 2);
        Serial.println(F(" Feet"));

        Serial.print(F("Number of Sats Used: "));
        Serial.println(gpsUBXDataStruct.numSV);
        Serial.println(F("\n"));
    #endif
    

    if (coldStart == true)
        {
            coldStart = false;
        }
}


float altitude(const int32_t press, const float seaLevel)
    {
        /*!****************************************************************************************
            @brief      This converts a pressure measurement into a height in meters
            @details    The corrected sea-level pressure can be passed into the function if it is known,
                        otherwise the standard atmospheric pressure of 1013.25hPa is used (see
                        https://en.wikipedia.org/wiki/Atmospheric_pressure) for details.
            @param[in]  press    Pressure reading from BME680
            @param[in]  seaLevel Sea-Level pressure in millibars
            @return     floating point altitude in meters.
        *******************************************************************************************/

        static float Altitude = 44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));  // Convert into meters. 
        return (Altitude);
    }  // of method altitude()
 

/***************************************************************************************************************
* This function draws the Major and Minor tick marks in the circle.
***************************************************************************************************************/
void roundGaugeTickMarks(uint16_t x, uint16_t y, uint16_t r, int from, int to, float dev, int degrees , uint16_t color)
{
    float dsec;
    int i;
    for (i = from; i <= to; i += degrees)
    {
        dsec = i * (PI / 180);
        tft.drawLine(
                     x + (cos(dsec) * (r / dev)) + 1,
                     y + (sin(dsec) * (r / dev)) + 1,
                     x + (cos(dsec) * r) + 1,
                     y + (sin(dsec) * r) + 1,
                     color);
    }
}


/***************************************************************************************************************
* This function draws the Clock Face, White circle on black backround with major and minor tick marks, and the
* 12, 3, 6, and 9 around the inside of the clock lines.
* Needs:
*       pos[]:  Position array (16 bit integer):
*                       // Now set up the clock face:
*                       clockPos[0] = (tft.width() / 2);  // Clock center position in x direction (pixels).
*                       clockPos[1] = tft.height() / 2;  // Clock center position in y direction (pixels).
*                       clockPos[2] = (tft.height() / 2) - 60;  // Clock radius in pixels (600 / 2 - 60 = 240 pixels).
*       from:  (unsigned 16 bit integer),
*       to:  (unsigned 16 bit integer),
*       Circle_color:  The color of the inside of the circle,
*       Major_color:  The Major tick marks color,
*       Minor_color:  The Minor tick marks color.
* Where:
*       X Position array has 3 numbers, starting x position, starting y position, and the radius of the circle.
*       from:  is the first tick mark in degrees from the top center (12 o'clock position).
*       to:  is the last tick mark in degrees from the top center (12 o'clock position).
* Returns:  Nothing
*
***************************************************************************************************************/
void drawGauge(int16_t pos[], uint16_t from, uint16_t to, uint16_t Circle_color , uint16_t Major_color , uint16_t Minor_color)
{
    tft.drawCircle(pos[0], pos[1], pos[2], Circle_color); //draw instrument container
    tft.drawCircle(pos[0], pos[1], pos[2] + 1, Circle_color); //draw instrument container
    roundGaugeTickMarks(pos[0], pos[1], pos[2], from, to, 1.2, 30, Major_color);  // Draw Major Ticks every 30 degrees.
    if (pos[2] > 15) roundGaugeTickMarks(pos[0], pos[1], pos[2], from + 6, to - 6, 1.1, 6, Minor_color); // Draw minor ticks every 6 degrees.
    
    /// @brief Print the 12, 9 ,3, 6 on the clock face. 
    tft.setCursor ((pos[0] - 10 ), (pos[1] - pos[2]) + 50 );  // Size is set for 16 x 32 pixels.
    tft.print(F("12"));
    tft.setCursor (((pos[0] - pos[2]) + 44 ), (pos[1] - 6));
    tft.print(F("9"));
    tft.setCursor (((pos[0] + pos[2]) - 52 ), (pos[1] - 6));
    tft.print(F("3"));
    tft.setCursor ((pos[0] - 4 ) , (pos[1] + pos[2]) - 58 );
    tft.print(F("6"));
}


/********************************************************************************************************************
* This function draws the clock hands where ever you specify.
* These definitions are for the Clock functions.
* // Define array for Clockface:
* int16_t clockPos[3] = {0, 0, 50}; //x size, y size, r Radius
*
* //Define array for Time in hours, minutes, and seconds.
* uint8_t currentTime[3] = {0, 0, 0}; //hh, mm, ss
* int currentDate[3] = {0, 0, 0}; // Day, Month, Year
* // Define colors for background, face, hh hand, mm hand, and ss hand.
* const uint16_t clockColors[5] = {RA8875_BLACK, RA8875_BLACK, RA8875_CYAN, RA8875_BLUE, RA8875_RED};
*
* // Define array with positions for hour hand X, hour hand Y, minute hand X, minute hand Y, second hand X, second hand Y
* uint16_t oldPos[6] = {0, 0, 0, 0, 0, 0};
*
* // Need variable for the current time.
* unsigned long targetTime = 0;
*
*********************************************************************************************************************/
void drawClockHands(int16_t pos[], uint8_t curTime[], const uint16_t colors[])
{
    
    uint8_t hlen = (pos[2] / 2 - pos[2] / 12);
    uint8_t mlen = pos[2] / 2 - pos[2] / 4;
    uint8_t slen = pos[2] / 2 - pos[2] / 4;
    float hx = cos(((curTime[0] * 30 + (curTime[1] * 6 + (curTime[2] * 6) * 0.01666667) * 0.0833333) - 90) * 0.0174532925);
    float hy = sin(((curTime[0] * 30 + (curTime[1] * 6 + (curTime[2] * 6) * 0.01666667) * 0.0833333) - 90) * 0.0174532925);
    float mx = cos(((curTime[1] * 6 + (curTime[2] * 6) * 0.01666667) - 90) * 0.0174532925);
    float my = sin(((curTime[1] * 6 + (curTime[2] * 6) * 0.01666667) - 90) * 0.0174532925);
    float sx = cos(((curTime[2] * 6) - 90) * 0.0174532925);
    float sy = sin(((curTime[2] * 6) - 90) * 0.0174532925);
    
    // Erase just old hand positions
    tft.drawLine(oldPos[0], oldPos[1], pos[0] + 1, pos[1] + 1, colors[1]);
    tft.drawLine(oldPos[2], oldPos[3], pos[0] + 1, pos[1] + 1, colors[1]);
    tft.drawLine(oldPos[4], oldPos[5], pos[0] + 1, pos[1] + 1, colors[1]);
    // Draw new hand positions
    tft.drawLine(hx * (pos[2] - hlen) + pos[0] + 1, hy * (pos[2] - hlen) + pos[1] + 1, pos[0] + 1, pos[1] + 1, colors[2]);
    tft.drawLine(mx * (pos[2] - mlen) + pos[0] + 1, my * (pos[2] - mlen) + pos[1] + 1, pos[0] + 1, pos[1] + 1, colors[3]);
    tft.drawLine(sx * (pos[2] - slen) + pos[0] + 1, sy * (pos[2] - slen) + pos[1] + 1, pos[0] + 1, pos[1] + 1, colors[4]);
    tft.fillCircle(pos[0] + 1, pos[1] + 1, 3, colors[4]);
    
    // Update old x&y coords
    oldPos[4] = sx * (pos[2] - slen) + pos[0] + 1;
    oldPos[5] = sy * (pos[2] - slen) + pos[1] + 1;
    oldPos[2] = mx * (pos[2] - mlen) + pos[0] + 1;
    oldPos[3] = my * (pos[2] - mlen) + pos[1] + 1;
    oldPos[0] = hx * (pos[2] - hlen) + pos[0] + 1;
    oldPos[1] = hy * (pos[2] - hlen) + pos[1] + 1;
}


/***************************************************************************************************************
* This function puts the time, date, and Day of the Week on the screen where ever you specify on the TFT.
* We are not using 12 Hr time (Boolean am/pm is false), we are using military time.
* clockPos[0] - (fontXsize * 5)) , YLINE15, currentTime[0], currentTime[1], currentTime[2], currentDayOfWeek,
* currentDate[0], currentDate[1], currentDate[2], false);
***************************************************************************************************************/
void drawPrintTime(uint16_t x, uint16_t y, uint8_t h, uint8_t m, uint8_t s, uint8_t day, int dd, int mm, int yr, bool ampm)
{
    // First lets clear the previous date, time, time zone, and day of the week.
    // The backround color is black which blanks out the previous text.
    if ( (itsMidnight == true) || (finishedPreferences == true) || startUp == true || colorChangeOccured == true )
        {
            tft.setCursor( x , YLINE35 );  // Step down to the bottom to clear the Day.
            tft.print(F("         "));  // Clear the line (9 spaces) from the previous day of the week.
        }

    // Clear the previous date info from the tft screen depending on how many digits there are.
    if ( (dd < 10) && (mm < 10) )
        {
            tft.setCursor ( ( x + (fontXsize) ), YLINE26 );  // Set cursor to center the date on screen.
            tft.print("        ");  // Clear the line (8 spaces) from the previous date.
        }
    else if ( (dd < 10) && (mm >= 10) )
        {
            tft.setCursor ( ( x + (fontXsize / 2) ), YLINE26 );  // Set cursor to center the date on screen.
            tft.print("         ");  // Clear the line (9 spaces) from the previous date.
        }
    else
        {
            tft.setCursor ( x, YLINE26 );  // Set cursor to center the date on screen.
            tft.print("          ");  // Clear the line (10 spaces) from the previous date.
        }

    tft.setCursor (x + fontXsize, YLINE12);  // Step up four lines to clear the old time.
    tft.print("        ");  // Clear the line (8 spaces) from the previous time.

    tft.setCursor ( ( x + ( (fontXsize * 2) + (fontXsize / 2) ) ), YLINE14 );  // Step up 3 lines to clear the time zone.
    tft.print("     ");  // Clear the line (5 spaces) from the previous time zone.
    

    // Now set the backround color to transparent with the correct color of text.
    if (nightTimeFlag == false)
        {
            tft.setTextColor(WHITE, BLACK);    
        }
    else
        {
            tft.setTextColor(RED, BLACK);
        }

    // Lets print the date:
    // First move the cursor so the date is centered in the clock face.
    if ( (dd < 10) && (mm < 10) )
        {
            tft.setCursor ( ( x + fontXsize ), YLINE26 );  // Set cursor to center the date on screen.
        }
    else if ( (dd < 10) && (mm >= 10) )
        {
            tft.setCursor ( ( x + (fontXsize / 2) ), YLINE26 );  // Set cursor to center the date on screen.
        }
    else
        {
            tft.setCursor ( x, YLINE26 );  // Set cursor to center the date on screen.
        }
    // Now that the cursor is located correctly now print the date on the tft screen.
    tft.print(mm);
    tft.write('/');
    tft.print(dd);
    tft.write('/');
    tft.print(yr);

    // Now lets print the time in 24 hr format.
    tft.setCursor (x + fontXsize, YLINE12);
    if (ampm == true)
        {
            if (h > 12)
                {
                    if (h < 22) tft.print('0');
                    tft.print (h - 12);
                }
            else
                {
                    if (h < 10) tft.print('0');
                    tft.print (h);
                }
        }
    else
        {
            if (h < 10) tft.print('0');
            tft.print (h);
        }
    
    tft.print (':');
    if (m < 10) tft.print('0');
    tft.print (m);
    tft.print (':');
    if (s < 10) tft.print('0');
    tft.print (s);
    
    if (ampm == true)
        {
            if (h > 12)
                {
                    tft.print(" pm");
                }
            else
                {
                    tft.print (" am");
                }
        }
    
    // Now lets print the time zone.
    
    #if defined (DEBUG51)
        Serial.print(F("This is the drawPrintTime Sub. This is the size of the tcr -> abbrev: "));
        Serial.println(sizeof(tcr -> abbrev), DEC);
    #endif

    tft.setCursor ( ( x + ( (fontXsize * 2) + (fontXsize / 2) ) ),YLINE14 );
    tft.print(tcr -> abbrev);
    

    // Now let us print the day of the week at the bottome row of the screen.
    if ( (itsMidnight == true) || (finishedPreferences == true) || startUp == true || colorChangeOccured == true )
        {
            // Now lets print the day of the week (Sunday -> Saturday)
            switch (day)  // Day can be 1 -> 7 inclusive.
                {
                    case 0:  // This means the day of the week was not initialized.
                        tft.setCursor( x , YLINE35 );  // Step down to the bottom to print the Day.
                        tft.print(F("Bad Day"));  // Clear the line (9 spaces) first to remove the remnants from previous print.
                        break;
            
                    case 1:  // This is Sunday (6 Characters)
                        tft.setCursor( x + fontXsize , YLINE35 );  // Step down to the bottom to print the Day.
                        tft.print(F("Sunday"));
                        break;
            
                    case 2:  // This is Monday (6 Characters)
                        tft.setCursor( x + fontXsize , YLINE35);
                        tft.print(F("Monday"));
                        break;
            
                    case 3:  // This is Tuesday (7 Characters)
                        tft.setCursor( x + ( (fontXsize / 2) * 3 ) , YLINE35);
                        tft.print(F("Tuesday"));
                        break;
            
                    case 4:  // This is Wednesday (9 Characters)
                        tft.setCursor( x - fontXsize, YLINE35);
                        tft.print(F("Wednesday"));
                        break;
            
                    case 5:  // This is Thursday (8 Characters)
                        tft.setCursor( x , YLINE35);
                        tft.print(F("Thursday"));
                        break;
            
                    case 6:  // This is Friday (6 Characters)
                        tft.setCursor( x + fontXsize , YLINE35);
                        tft.print(F("Friday"));
                        break;
            
                    case 7:  // This is Saturday (8 Characters)
                        tft.setCursor( x , YLINE35);
                        tft.print(F("Saturday"));
                        break;
            
                    default:
                        tft.setCursor( x , YLINE35 );  // Step down to the bottom to print the Day.
                        tft.print(F("Bad Day"));  // Clear the line (9 spaces) first to remove the remnants from previous print.
                        break;
                }
            itsMidnight = false;  // It is midnight only for 1 second after that the date and day do not change until the next midnight.
            finishedPreferences = false;  // We have used this flag so do not need it until another preference is set.
            startUp = false;  // We are finished with this flags after the initial startup.
            colorChangeOccured = false;  // We are finished with this flag after the text color changed.
        }

    /// @brief FIX THIS!!!!!
    /*
    //tft.setFontScale(0);  // This is the small font on the screen.
    tft.setTextColor(CRIMSON, BLACK);  // This sets the font color to orange and the backround to black.
    // This puts the Build TimeStamp just above the day of the week at the bottom of the screen.
    tft.setCursor( ( clockPos[0] - ( fontXsize * 10 ) ), ( YLINE15 - 12 ), false);  // Size is set for 8 x 16 pixels.
    tft.print("Bld " + String(BUILD_TIMESTAMP) );  // Print Actual build string.
    */
    /// @brief FIX THIS!!!!!
    //tft.setFontScale(1);  // This is the regular font (16x x 32y) on the screen.

    // Now lets return to the normal color text and backround (BLACK).
    if (nightTimeFlag == false)
        {
            tft.setTextColor(WHITE, BLACK);    
        }
    else
        {
            tft.setTextColor(RED, BLACK);
        }
    
}


/***************************************************************************************************************
*       This function prints a Red or Green Dot just below the 12 of the Clock Face on the TFT Screen.
*       This indicates whether or not the RTC clock has been set to UTC from the Gps
*   Needs:
*       Xpos - The X position on the TFT which is the center of the Dot.
*       Ypos - The Y position on the TFT which is the center of the Dot.
*       dotColor - The color of the Dot
*
*   Returns:
*       Nothing
***************************************************************************************************************/
void printUTCStatusDotOnTFT(uint16_t xPos, uint16_t yPos, uint16_t dotColor )
{
    tft.fillCircle(xPos, yPos, 6, dotColor);
}


/*******************************************************************************************************************
* This Function blanks out a rectangle shaped box on the TFT monitor, which effectivly blanks out section of the display.
*
* Needs:
*   xPos:       Starting X Position.
*   yPos:       Starting Y Position.
*   Width:      Width of the blanking rectangle.
*   Height:     Height of the blanking rectangle.  
* Returns:
*   Nothing.
*******************************************************************************************************************/
void clearTFTSensorData(uint16_t xPos, uint16_t yPos, uint16_t Width, uint16_t Height)
    {
        tft.fillRect(xPos, yPos, Width, Height, BLACK);
    }


/*******************************************************************************************************************
* This Function prints "GPS Present", or "No GPS" on the TFT monitor.
*
* Needs:
*   xPos:       Starting X Position.
*   yPos:       Starting Y Position.
*   GpsPresent: GPS present or not.  
* Returns:
*   Nothing.
*******************************************************************************************************************/
void tftPrintGPSPresentHeader(uint16_t xPos, uint16_t yPos, bool GpsPresent)
    {
        clearTFTSensorData(xPos, yPos, XRIGHT - xPos, ( yPos + (fontYsize * 2) ) );
        if (GpsPresent == true)
            {
                tft.setCursor (xPos, yPos);
                tft.print(F("GPS Present"));
            }
        else
            {
                tft.setCursor (xPos, yPos);
                tft.print(F("No GPS"));
            }
    }


/*******************************************************************************************************************
* This Function prints "No Valid Data" on the TFT monitor.
*
* Needs:
*   xPos:       Starting X Position.
*   yPos:       Starting Y Position.
* Returns:
*   Nothing.
*******************************************************************************************************************/
void tftPrintGPSNoValidData(uint16_t xPos, uint16_t yPos)
    {
        clearTFTSensorData( xPos, yPos, ( XRIGHT - xPos ), ( yPos + (fontYsize * 12) ) );  // Clear al the GPS Data
        tft.setCursor(xPos, yPos);
        tft.print(F("No Valid Data"));
    }


/*******************************************************************************************************************
* This Function prints data from the GPS/GNSS:
*   Fix Type,
*   Number of *Satellites being used,
*   Latitude to 6 decimal places,
*   Longitude to 6 decimal places,
*   Altitude,
*   Speed,
*   Time,
*   Date.
*
*   on the TFT monitor.
* Needs:
*   X Position to start printing
*   Y Position to start printing
* Returns:
*   Nothing.
*******************************************************************************************************************/
void printGPSPositionOnTFT(uint16_t x, uint16_t y, uint8_t validityCode)
    {
        clearTFTSensorData(x, y, XRIGHT - x, ( y + (fontYsize * 12) ) );
        //blankOutGPSPositionOnTFT(x, y);  // Blank out the entire GPS Data from the TFT.
    
        // Move Cursor to initial coordinates
        //tft.setFontScale(0);  // Set Font to normal size (8W x 16T pixels).
    
        tft.setCursor (x, y);

        tft.print(F("FixType: "));  // Print the Heading.
        switch (validityCode)
        {
            case NO_FIX:
                tft.print(F("NO FIX"));  // Print the Heading.
                break;
                        
            case DEAD_RECKONING_ONLY:
                tft.print(F("DR ONLY"));  // Print the Heading.
                break;
                        
            case TWO_D_FIX:
                tft.print(F("2D-Fix"));  // Print the Fix Type.
                break;
                        
            case THREE_D_FIX:
                tft.print(F("3D-Fix"));  // Print the Fix Type.
                break;
                        
            case GNSS_DR_COMBINED:
                tft.print(F("GNSS_DR_COMBINED"));  // Print the Fix Type.
                break;
                        
            case TIME_FIX_ONLY:
                tft.print(F("TIME FIX ONLY"));  // Print the Heading.
                break;

            default:
                tft.print(F("NO FIX"));  // Print the Heading.
                break;
        }

        tft.setCursor ( x, ( y + ( Ysize * 2 ) ) );  // Step down one line (Y) to print the number of Sats.
        tft.print(F("Sats: "));  // Print the Heading.
        tft.print(gpsUBXDataStruct.numSV, DEC);  // Print the Number of Satellites Used.
    
        tft.setCursor ( x, ( y + ( 2 * ( Ysize * 2 ) ) ) );  // Step down two lines (Y) to print the Longitude.
        tft.print(F("Lat: "));  // Print the Heading.
        tft.print( ( ((float)gpsUBXDataStruct.lat) ), 6);  // Print the Latitude.
    
        tft.setCursor ( x, (y + (3 * ( Ysize * 2 )) ) );  // Step down three lines (Y) to print the Longitude.
        tft.print(F("Lon: "));  // Print the Heading.
        tft.print( ( ((float)gpsUBXDataStruct.lon) ), 6);  // Print the Longitude (the value received has been multiplied by 1e7 to avoid decimal places).
    
        tft.setCursor ( x, (y + (4 * ( Ysize * 2 )) ) );  // Step down four lines (Y) to print the Altitude.
        tft.print(F("Alt: "));  // Print the Heading.
        tft.print( ( ((float)gpsUBXDataStruct.hMSL) ), 1 );  // Print the Altitude (the value is in millimeters), to convert millimeters to feet divide the value by 304.8.
    
        tft.setCursor ( x, (y + (5 * ( Ysize * 2 )) ) );  // Step down five lines (Y) to print the speed.
        tft.print(F("Spd: "));  // Print the Speed in MPH.
        tft.print( ( 2.23694F * ((float)gpsUBXDataStruct.gSpeed) ), 1 );  // Convert the speed value in m/sec by 2.23694 to get miles per hour.
            
        tft.setCursor ( x, (y + (6 * ( Ysize * 2 )) ) );  // Step down six lines (Y) to print the time.
        tft.print(F("Time: "));  // Print the Time.
        if (gpsUBXDataStruct.hour < 10) tft.print(F("0")); // Print a leading zero if required
        tft.print(gpsUBXDataStruct.hour);

        tft.print(F(":"));

        if (gpsUBXDataStruct.min < 10) tft.print(F("0")); // Print a leading zero if required
        tft.print(gpsUBXDataStruct.min);

       tft.print(F(":"));

        if (gpsUBXDataStruct.sec < 10) tft.print(F("0")); // Print a leading zero if required
        tft.print(gpsUBXDataStruct.sec);

        /*
        Serial.print(F("."));

        if (gpsUBXDataStruct.iTOW < 100) tft.print(F("0")); // Print the trailing zeros correctly
        if (gpsUBXDataStruct.iTOW < 10) tft.print(F("0"));
        tft.print(gpsUBXDataStruct.iTOW);
        tft.println(F(" UTC or GMT"));
        */

        tft.setCursor ( x, (y + (7 * ( Ysize * 2 )) ) );  // Step down seven lines (Y) to print the date.
        tft.print(F("Date: "));  // Print the Date.
        if (gpsUBXDataStruct.month < 10) tft.print(F("0")); // Print a leading zero if required
        tft.print(gpsUBXDataStruct.month);

        tft.print(F("/"));

        if (gpsUBXDataStruct.day < 10) tft.print(F("0")); // Print a leading zero if required
        tft.print(gpsUBXDataStruct.day);

        tft.print(F("/"));

        tft.println(gpsUBXDataStruct.year);
         
    }


/***************************************************************************************************************
* This function updates the Location Data on the TFT from the GPS.
*
*******************************************************************************************************
*               High level Commands, for the user ~ UBLOX lib. v1.0.2 2018-03-20 *
*******************************************************************************************************
*     NOTE: gps.command(Boolean) == (true)Print Message Acknowledged on USB Serial Monitor
* end();                            // Disables Teensy serial communication, to re-enable, call begin(Baud)
* Poll_CFG_Port1(bool);             // Polls the configuration for one I/O Port, I/O Target 0x01=UART1
* Poll_NAV_PVT();                   // Polls UBX-NAV-PVT    (0x01 0x07) Navigation Position Velocity Time Solution
* Poll_NAV_POSLLH();                // Polls UBX-NAV-POSLLH (0x01 0x02) Geodetic Position Solution
* Poll_NAV_ATT();                   // Polls UBX-NAV-ATT    (0x01 0x05) Attitude Solution
*
* ### Periodic Auto Update ON,OFF Command ###
* Ena_NAV_PVT(bool);                // Enable periodic auto update NAV_PVT
* Dis_NAV_PVT(bool);                // Disable periodic auto update NAV_PVT
* Ena_NAV_ATT(bool);                // Enable periodic auto update NAV_ATT
* Dis_NAV_ATT(bool);                // Disable periodic auto update NAV_ATT
* Ena_NAV_POSLLH(bool);             // Enable periodic auto update NAV_POSLLH
* Dis_NAV_POSLLH(bool);             // Disable periodic auto update NAV_POSLLH
*
* #### u-blox Switch off all NMEA MSGs ####
* Dis_all_NMEA_Child_MSGs(bool);    // Disable All NMEA Child Messages Command
*
* ### High level Command Generator ###
* SetGPSbaud(uint32_t baud, bool)   // Set UBLOX GPS Port Configuration Baud rate
* SetNAV5(uint8_t dynModel, bool)   // Set Dynamic platform model Navigation Engine Settings (0:portable, 3:pedestrian, Etc)
* SetRATE(uint16_t measRate, bool)  // Set Navigation/Measurement Rate Settings (100ms=10.00Hz, 200ms=5.00Hz, 1000ms=1.00Hz, Etc)
*
*   UBX-NAV-PVT --  Navigation Position Velocity Time Solution
*   ### UBX Protocol, Class NAV 0x01, ID 0x07 ###
*   UBX-NAV-PVT (0x01 0x07)    (Payload U-blox-M8=92, M7&M6=84)
*   iTOW                       ///< [ms], GPS time of the navigation epoch
*   utcYear                    ///< [year], Year (UTC)
*   utcMonth                   ///< [month], Month, range 1..12 (UTC)
*   utcDay                     ///< [day], Day of month, range 1..31 (UTC)
*   utcHour                    ///< [hour], Hour of day, range 0..23 (UTC)
*   utcMin                     ///< [min], Minute of hour, range 0..59 (UTC)
*   utcSec                     ///< [s], Seconds of minute, range 0..60 (UTC)
*   valid                      ///< [ND], Validity flags
*   tAcc                       ///< [ns], Time accuracy estimate (UTC)
*   utcNano                    ///< [ns], Fraction of second, range -1e9 .. 1e9 (UTC)
*   fixType                    ///< [ND], GNSSfix Type: 0: no fix, 1: dead reckoning only, 2: 2D-fix, 3: 3D-fix, 4: GNSS + dead reckoning combined, 5: time only fix
*   flags                      ///< [ND], Fix status flags
*   flags2                     ///< [ND], Additional flags
*   numSV                      ///< [ND], Number of satellites used in Nav Solution
*   lon                        ///< [deg], Longitude
*   lat                        ///< [deg], Latitude
*   height                     ///< [m], Height above ellipsoid
*   hMSL                       ///< [m], Height above mean sea level
*   hAcc                       ///< [m], Horizontal accuracy estimate
*   vAcc                       ///< [m], Vertical accuracy estimate
*   velN                       ///< [m/s], NED north velocity
*   velE                       ///< [m/s], NED east velocity
*   velD                       ///< [m/s], NED down velocity
*   gSpeed                     ///< [m/s], Ground Speed (2-D)
*   heading                    ///< [deg], Heading of motion (2-D)
*   sAcc                       ///< [m/s], Speed accuracy estimate
*   headingAcc                 ///< [deg], Heading accuracy estimate (both motion and vehicle)
*   pDOP                       ///< [ND], Position DOP
*   headVeh                    ///< [deg], Heading of vehicle (2-D)             #### NOTE: u-blox8 only ####
*   --- magDec, magAcc --- TODO TEST
*   magDec                     ///< [deg], Magnetic declination                 #### NOTE: u-blox8 only ####
*   magAcc                     ///< [deg], Magnetic declination accuracy        #### NOTE: u-blox8 only ####
*
* Needs:
*   Nothing
* Returns:
*       GNSSfix Type:
*       0: No fix
*       1: Dead reckoning only
*       2: 2D-fix
*       3: 3D-fix
*       4: GNSS + dead reckoning combined
*       5: Time only fix
***************************************************************************************************************/
uint8_t updatePositionFromGPS(void)
{
    uint8_t fixType;

    // GNSSfix Type:
    //  0: no fix
    //  1: dead reckoning only
    //  2: 2D-fix
    //  3: 3D-fix
    //  4: GNSS + dead reckoning combined
    //  5: time only fix
    //  6: Bad Ublox Packet
    

    fixType = (gpsUBXDataStruct.fixType );  // Get fixtype from the Struct.
        
        #if defined(DEBUG47)
            Serial.print(F("uBlox Fix Type: "));
            Serial.println(fixType, HEX);  /// Print the fixType code to the terminal.
        #endif

    // Clear the TFT GPS data from the TFT Screen.
    clearTFTSensorData( ( XRIGHT - ( Xsize * 31) ), ( YMIDDLE ), XRIGHT - ( XRIGHT - ( Xsize * 31) ), ( ( YMIDDLE ) + (fontYsize * 12) ) );

    switch (fixType)
        {
            case NO_FIX:
                tft.setCursor(( XRIGHT - ( Xsize * 31) ), ( YMIDDLE ) );
                tft.print(F("NO FIX"));  // Print the Heading.
                numberOfBadPackets = 0;
                return NO_FIX;
                        
            case DEAD_RECKONING_ONLY:
                tft.setCursor(( XRIGHT - ( Xsize * 31) ), ( YMIDDLE ) );
                tft.print(F("DR ONLY"));  // Print the Heading.
                numberOfBadPackets = 0;
                return DEAD_RECKONING_ONLY;
                        
            case TWO_D_FIX:
                printGPSPositionOnTFT( ( XRIGHT - ( Xsize * 31 ) ), ( YMIDDLE ), fixType );  // Print the Lat, Long, etc on the TFT.
                numberOfBadPackets = 0;
                return TWO_D_FIX;
                        
            case THREE_D_FIX:
                printGPSPositionOnTFT( ( XRIGHT - ( Xsize * 31 ) ), ( YMIDDLE ), fixType  );  // Print the Lat, Long, etc on the TFT.
                numberOfBadPackets = 0;
                return THREE_D_FIX;
                        
            case GNSS_DR_COMBINED:
                printGPSPositionOnTFT( ( XRIGHT - ( Xsize * 31 ) ), ( YMIDDLE ), fixType  );  // Print the Lat, Long, etc on the TFT.
                numberOfBadPackets = 0;
                return GNSS_DR_COMBINED;
                        
            case TIME_FIX_ONLY:
                tft.setCursor(( XRIGHT - ( Xsize * 31) ), ( YMIDDLE ) );
                tft.print(F("TIME FIX ONLY"));  // Print the Heading.
                numberOfBadPackets = 0;
                return TIME_FIX_ONLY;

            default:
                numberOfBadPackets = 0;
                return NO_FIX;
        }
}


/***************************************************************************************************************
* This function updates the internal clock of the DS3231 Clock from the GPS Time.
*
*******************************************************************************************************
*               High level Commands, for the user ~ UBLOX lib. v1.0.2 2018-03-20 *
*******************************************************************************************************
*     NOTE: gps.command(Boolean) == (true)Print Message Acknowledged on USB Serial Monitor
* end();                            // Disables Teensy serial communication, to re-enable, call begin(Baud)
* Poll_CFG_Port1(bool);             // Polls the configuration for one I/O Port, I/O Target 0x01=UART1
* Poll_NAV_PVT();                   // Polls UBX-NAV-PVT    (0x01 0x07) Navigation Position Velocity Time Solution
* Poll_NAV_POSLLH();                // Polls UBX-NAV-POSLLH (0x01 0x02) Geodetic Position Solution
* Poll_NAV_ATT();                   // Polls UBX-NAV-ATT    (0x01 0x05) Attitude Solution
*
* ### Periodic Auto Update ON,OFF Command ###
* Ena_NAV_PVT(bool);                // Enable periodic auto update NAV_PVT
* Dis_NAV_PVT(bool);                // Disable periodic auto update NAV_PVT
* Ena_NAV_ATT(bool);                // Enable periodic auto update NAV_ATT
* Dis_NAV_ATT(bool);                // Disable periodic auto update NAV_ATT
* Ena_NAV_POSLLH(bool);             // Enable periodic auto update NAV_POSLLH
* Dis_NAV_POSLLH(bool);             // Disable periodic auto update NAV_POSLLH
*
* #### u-blox Switch off all NMEA MSGs ####
* Dis_all_NMEA_Child_MSGs(bool);    // Disable All NMEA Child Messages Command
*
* ### High level Command Generator ###
* SetGPSbaud(uint32_t baud, bool)   // Set UBLOX GPS Port Configuration Baud rate
* SetNAV5(uint8_t dynModel, bool)   // Set Dynamic platform model Navigation Engine Settings (0:portable, 3:pedestrian, Etc)
* SetRATE(uint16_t measRate, bool)  // Set Navigation/Measurement Rate Settings (100ms=10.00Hz, 200ms=5.00Hz, 1000ms=1.00Hz, Etc)
*
*   UBX-NAV-PVT --  Navigation Position Velocity Time Solution
*   ### UBX Protocol, Class NAV 0x01, ID 0x07 ###
*   UBX-NAV-PVT (0x01 0x07)    (Payload U-blox-M8=92, M7&M6=84)
*   iTOW                       ///< [ms], GPS time of the navigation epoch
*   utcYear                    ///< [year], Year (UTC)
*   utcMonth                   ///< [month], Month, range 1..12 (UTC)
*   utcDay                     ///< [day], Day of month, range 1..31 (UTC)
*   utcHour                    ///< [hour], Hour of day, range 0..23 (UTC)
*   utcMin                     ///< [min], Minute of hour, range 0..59 (UTC)
*   utcSec                     ///< [s], Seconds of minute, range 0..60 (UTC)
*   valid                      ///< [ND], Validity flags
*   tAcc                       ///< [ns], Time accuracy estimate (UTC)
*   utcNano                    ///< [ns], Fraction of second, range -1e9 .. 1e9 (UTC)
*   fixType                    ///< [ND], GNSSfix Type: 0: no fix, 1: dead reckoning only, 2: 2D-fix, 3: 3D-fix, 4: GNSS + dead reckoning combined, 5: time only fix
*   flags                      ///< [ND], Fix status flags
*   flags2                     ///< [ND], Additional flags
*   numSV                      ///< [ND], Number of satellites used in Nav Solution
*   lon                        ///< [deg], Longitude
*   lat                        ///< [deg], Latitude
*   height                     ///< [m], Height above ellipsoid
*   hMSL                       ///< [m], Height above mean sea level
*   hAcc                       ///< [m], Horizontal accuracy estimate
*   vAcc                       ///< [m], Vertical accuracy estimate
*   velN                       ///< [m/s], NED north velocity
*   velE                       ///< [m/s], NED east velocity
*   velD                       ///< [m/s], NED down velocity
*   gSpeed                     ///< [m/s], Ground Speed (2-D)
*   heading                    ///< [deg], Heading of motion (2-D)
*   sAcc                       ///< [m/s], Speed accuracy estimate
*   headingAcc                 ///< [deg], Heading accuracy estimate (both motion and vehicle)
*   pDOP                       ///< [ND], Position DOP
*   headVeh                    ///< [deg], Heading of vehicle (2-D)             #### NOTE: u-blox8 only ####
*   --- magDec, magAcc --- TODO TEST
*   magDec                     ///< [deg], Magnetic declination                 #### NOTE: u-blox8 only ####
*   magAcc                     ///< [deg], Magnetic declination accuracy        #### NOTE: u-blox8 only ####
*
* Needs:
*   Nothing
* Returns:
*   boolean:
*       false if success.
*       true if error.
***************************************************************************************************************/
bool updateTimeFromGPS(void)
{
    uint8_t WeekDay;
    uint8_t validityCode;
    time_t tempTimeTStorage;

    #if defined(DEBUG28)
        Serial.println("\nThis is the updateTimeFromGPS function...");
        Serial.print(F("This is the Size of the gpsUBXDataStruct Struct: "));
        Serial.println(sizeof(gpsUBXDataStruct));  // Print the size of the uBloxData Struct.
    #endif

    WeekDay = tm.Wday;  // Save the Week Day in this temp location.
    
    #if defined(DEBUG28)
        Serial.println(F("Checking the validity code..."));
    #endif

    validityCode = ( (gpsUBXDataStruct.fixType) & (0x0F) );  // Strip off unneeded bits.
    if( (validityCode > 0x01) && (validityCode < 0x06) )  // These codes mean the time & date are accurate.
        {
            #if defined(DEBUG28)
                Serial.print(F("Found Good Validity Code: "));
                Serial.println(validityCode, HEX);

                Serial.print(F("This is the Hour reported from the GPS: "));
                Serial.println(gpsUBXDataStruct.hour, DEC);

                Serial.print(F("This is the Minute reported from the GPS: "));
                Serial.println(gpsUBXDataStruct.min, DEC);

                Serial.print(F("This is the Seconds reported from the GPS: "));
                Serial.println(gpsUBXDataStruct.sec, DEC);

                Serial.print(F("This is the Month reported from the GPS: "));
                Serial.println(gpsUBXDataStruct.month, DEC);

                Serial.print(F("This is the Day reported from the GPS: "));
                Serial.println(gpsUBXDataStruct.day, DEC);

                Serial.print(F("This is the Year reported from the GPS: "));
                Serial.println(gpsUBXDataStruct.year, DEC);

                Serial.println(F("Now putting the Time and Date into the tm elements struct..."));
            #endif
                    
                    tm.Hour = gpsUBXDataStruct.hour;
                    tm.Minute = gpsUBXDataStruct.min;
                    tm.Second = (gpsUBXDataStruct.sec);
                    
                    tm.Day = gpsUBXDataStruct.day;
                    tm.Month = gpsUBXDataStruct.month;
                    tm.Year = CalendarYrToTm(gpsUBXDataStruct.year);  // This converts the 4 digit year of the GPS Module to 2 digits for the DS3231.
            
                    /*----------------------------------------------------------------------*
                     * Convert the given UTC time to local time, standard or                *
                     * daylight time, as appropriate.                                       *
                     *----------------------------------------------------------------------*/
                    #if defined(DEBUG28)
                        Serial.println(F("Now Converting tmElements struct into a Time_T (UNIX Time)..."));
                    #endif
                    tempTimeTStorage = makeTime(tm);  // Converts tmElements struct into a Time_T (UNIX Time), LastTimeSetByClock is a Time_T (UNIX Time) variable.

                    #if defined(DEBUG28)
                        Serial.println(F("Now converting the clock Time to local time..."));
                    #endif
                    tempTimeTStorage = (*tz).toLocal(tempTimeTStorage, &tcr);  // This will convert the clock Time to local time.

                    #if defined(DEBUG28)
                        Serial.println(F("Now Converting the Time_t Unix Time to a tmElements struct..."));
                    #endif
                    breakTime(tempTimeTStorage, tm);  // This Converts the Time_t Unix Time to a tmElements struct.

                    #if defined(DEBUG28)
                        Serial.println(F("This is the Time and Date after conversion..."));

                        Serial.print(F("This is the Hour:Minutes:Seconds after conversion: "));
                        Serial.print(tm.Hour, DEC);
                        Serial.print(F(":"));
                        Serial.print(tm.Minute, DEC);
                        Serial.print(F(":"));
                        Serial.println(tm.Second, DEC);

                        Serial.print(F("This is the Day/Month/Year after conversion: "));
                        Serial.print(tm.Month, DEC);
                        Serial.print(F("/"));
                        Serial.print(tm.Day, DEC);
                        Serial.print(F("/"));
                        Serial.println(tm.Year, DEC);
                    #endif
            
                    tm.Wday = WeekDay;
            
                    #if defined(DEBUG28)                   
                        Serial.print(F("Valid Date flag: "));
                        Serial.print(gpsUBXDataStruct.validDate,BIN);   ///< [s], Seconds of minute, range 0..60 (UTC)
                        Serial.println(F(" BIN"));

                        Serial.print(F("Valid Time flag: "));
                        Serial.print(gpsUBXDataStruct.validTime,BIN);   ///< [s], Seconds of minute, range 0..60 (UTC)
                        Serial.println(F(" BIN"));

                        Serial.print(F("Fully Resolved flag: "));
                        Serial.print(gpsUBXDataStruct.fullyResolved,BIN);   ///< [s], Seconds of minute, range 0..60 (UTC)
                        Serial.println(F(" BIN"));
                    #endif
            
                    // and configure the DS1307 RTC with this info
                    if (!RTC.write(tm))
                        {
                            #if defined(DEBUG28)
                                Serial.println(F("No DS3231 Present on the Bus!\n"));
                            #endif
                            return true;
                        }
            
                    statusDotColor = DOT_GREEN;
                    UTCTimeCorrected = true;
                    #if defined(DEBUG28)
                        Serial.println(F("Setting DS3231 Time from GPS was Successful!"));
                        Serial.print(F("statusDotColor is: "));
                        Serial.println(statusDotColor);
                        Serial.print(F("UTCTimeCorrected flag: "));
                        Serial.println(UTCTimeCorrected);
                        Serial.println(F(""));
                    #endif
                    numberOfBadPackets = 0;
                    return false;  // Success in setting time from GPS!
        }
    else
        {
            statusDotColor = DOT_YELLOW;
            UTCTimeCorrected = false;
            #if defined(DEBUG28)
                Serial.println(F("uBlox Data is NOT Valid!"));
                Serial.print(F("statusDotColor is: "));
                Serial.println(statusDotColor);
                Serial.print(F("UTCTimeCorrected flag: "));
                Serial.println(UTCTimeCorrected);
                Serial.println(F(""));
            #endif
            return true;
        }

}


/***************************************************************************************************************
* This function prints Humidity, Pressure, Air Quality, Altitude, and Temperature for the inside Sensor
*        at the top right of the TFT Screen.
*       TODO: Convert from Imperial (F) to Metric (C).
***************************************************************************************************************/
void drawPrintInsideTempPresHum(uint16_t xPos, uint16_t yPos, float Temperature, float Baro_Pressure, float Relative_Humidity, float Gas_Sensor, float Altitude)
{
    clearTFTSensorData(xPos, yPos, XRIGHT - xPos, ( yPos + (fontYsize * 9) ) );

    tft.setCursor (xPos, yPos);  // Set the cursor to the first printing position on TFT.
    
    tft.print(F("Temp: "));  // Print Temperature on TFT with trailing space.
    //tft.print(F("            "));  // Clear the previous (12 possible spaces) write data with spaces.
    //tft.setCursor (xPos + (fontXsize * 10), yPos);  // Move to the new print place (7 characters from starting point xPos).
    tft.print(Temperature, 2);  // Print the temperature with two decimal places.
    tft.print(F(" deg F"));  // Tack on degrees F to the line.
    
    tft.setCursor (xPos, yPos + (fontYsize * 2));
    tft.print(F("Pres: "));
    //tft.print(F("     "));  // Clear the previous (5 possible spaces) write data with spaces.
    //tft.setCursor (xPos + (fontXsize * 10), yPos + (fontYsize * 2));
    tft.print(Baro_Pressure, 2);
    tft.print(F(" InHg"));
    
    tft.setCursor(xPos, yPos + (fontYsize * 4));
    tft.print(F("Hum: "));
    //tft.print(F("        "));  // Clear the previous (8 possible spaces) write data with spaces.
    //tft.setCursor (xPos + (fontXsize * 10), yPos + (fontYsize * 4));
    tft.print(Relative_Humidity, 2);
    tft.print(F(" %"));
    
    tft.setCursor(xPos, yPos + (fontYsize * 6));
    tft.print(F("Gas: "));
    //tft.print(F("            "));  // Clear the previous (8 possible spaces) write data with spaces.
    //tft.setCursor (xPos + (fontXsize * 10), yPos + (fontYsize * 6));
    tft.print(Gas_Sensor, 1);
    tft.print(F(" KOhms"));
        
    tft.setCursor(xPos, yPos + (fontYsize * 8));
    tft.print(F("Alt: "));
    //tft.print(F("        "));  // Clear the previous (8 possible spaces) write data with spaces.
    //tft.setCursor (xPos + (fontXsize * 10), yPos + (fontYsize * 8));
    tft.print(Altitude, 1);
    tft.print(F(" Ft"));
}


/*******************************************************************************************************************
* This function gets the sensor data from the BME680 Temperature, Barometric Pressure, and Relative Humidity Sensor.
* Pressure is returned in the SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar. Often times barometric pressure
* is reported in millibar or inches-mercury. For future reference 1 pascal =0.000295333727 inches of mercury, or
* 1 inch Hg = 3386.39 Pascal. So if you take the pascal value of say 100734 and divide by 3389.39 you'll get 29.72
* inches-Hg.
*******************************************************************************************************************/
void getBME680SensorData(void)
{
    int32_t tempReadTemperature, tempReadhumidity, tempReadPressure, gasReadResitance;

    //Start with temperature, as that data is needed for accurate compensation.
    //Reading the temperature updates the compensators of the other functions
    //in the background.
    #if defined(DEBUG42)
        Serial.println(F("\nThis is the BME680 getSensorData function."));
    #endif
    
    BME680Sensor.getSensorData(tempReadTemperature, tempReadhumidity, tempReadPressure, gasReadResitance, true);
    
    #if defined(DEBUG42)
        Serial.print(F("This is the getSensorData function..."));
    #endif
    
    SensorStruct.BME680TemperatureC = ( ( ( (float)tempReadTemperature ) / 100) );  // Read in °C and place in C spot in Struct.
    #if defined(DEBUG42)
        Serial.print(F("This is the getSensorData function.  This is the (uncalibrated) Temperature in °C from the BME280 Sensor: "));
        Serial.println(tempReadTemperature,2);
    #endif

    SensorStruct.BME680TemperatureF = ( ( ( ( (float)tempReadTemperature ) / 100) * 1.8) + 32 );  // Convert °C to °F.
    #if defined(DEBUG42)
        Serial.print(F("This is the getSensorData function.  This is the (uncalibrated) Temperature in °F from the BME280 Sensor: "));
        Serial.println(tempReadTemperature,2);
    #endif
    
    SensorStruct.BME680Baro_Pressure = ( ( ( (float)tempReadPressure ) ) / 3389.39);  // Convert from Pascals to InHg.
    #if defined(DEBUG42)
        Serial.print(F("This is the getSensorData function.  This is the (unadjusted) Converted Baro Pressure in InHg from the BME280 Sensor: "));
        Serial.println(BME680SensorStruct.Baro_Pressure,2);
    #endif
    
    SensorStruct.BME680Relative_Humidity = ( ( (float)tempReadhumidity ) / 1000 );
    #if defined(DEBUG42)
        Serial.print(F("This is the getSensorData function.  This is the (unadjusted) Relative Humidity from the BME280 Sensor: "));
        Serial.println(tempReadhumidity,2);
    #endif
    
    SensorStruct.BME680gasSensor = ( (float)( gasReadResitance / 1000 ) );  // Change to Kohms.
    #if defined(DEBUG42)
        Serial.print(F("This is the getSensorData function.  This is the (unadjusted) Gas Sensor reading in KOhms from the BME280 Sensor: "));
        Serial.println(gasReadResitance,2);
    #endif

    SensorStruct.BME680altitude = ( (altitude(tempReadPressure) * 3.281F ));  // Convert altitude from meters to Feet.

    SensorStruct.BME680DataValid = true;  // Set the data valid flag (Producer).
}


/*!****************************************************************************************
     @brief      Print the data from the MCP9808 sensor on the serial monitor.
    @details    
    @param[in]  None.  
    @return     None.
*******************************************************************************************/ 
void processMCP9808Data()
    {
        #if defined(DEBUG52)
            Serial.println(F("This is the displayMCP9808Data function."));
            Serial.println(F("\nReading MCP9808 Temp Data:"));
        #endif
        
        SensorStruct.mcp9808TempC = mcp9808TempSen.getTemperature();
        SensorStruct.mcp9808TempF = ( (SensorStruct.mcp9808TempC  * 1.8) + 32.0);
        #if defined(DEBUG52)
            Serial << ("Ambient Temperature: ") << SensorStruct.mcp9808TempC << ("C    ") << SensorStruct.mcp9808TempF  << ("F") << endl;
        #endif

        SensorStruct.mcp9808ConfigReg = mcp9808TempSen.getConfigRegister();
        #if defined(DEBUG52)
            Serial << ("\nReading Config Register in HEX: ") << _HEX(SensorStruct.mcp9808ConfigReg) << endl;
        #endif

        SensorStruct.mcp9808Resolution = mcp9808TempSen.getResolution();
        #if defined(DEBUG52)
            Serial << ("Reading Resolution Register in HEX: ") << _HEX(SensorStruct.mcp9808Resolution) << endl;
        #endif

        SensorStruct.mcp9808ManufID = mcp9808TempSen.getManufacturerID();
        #if defined(DEBUG52)
            Serial << ("Reading Mfr ID in HEX: ") << _HEX(SensorStruct.mcp9808ManufID) << endl;
        #endif

        SensorStruct.mcp9808DeviceID = mcp9808TempSen.getDeviceID();
        #if defined(DEBUG52)
            Serial << ( "Reading Device ID in HEX: ") << _HEX(SensorStruct.mcp9808DeviceID) << endl;
        #endif

        SensorStruct.mcp9808Revision = mcp9808TempSen.getRevision();
        #if defined(DEBUG52)
            Serial << ( "Reading Device Rev in HEX: ") << _HEX(SensorStruct.mcp9808Revision) << endl;
        #endif

        SensorStruct.mcp9808TempOffset = mcp9808TempSen.getOffset();
        #if defined(DEBUG52)
            Serial << ( "Reading Temperature Offset Calibration: ") << (float)(SensorStruct.mcp9808TempOffset) << endl;
        #endif

        SensorStruct.mcp9808TupperC = mcp9808TempSen.getTupper();
        SensorStruct.mcp9808TupperF = ( (SensorStruct.mcp9808TupperC  * 1.8) + 32.0);
        SensorStruct.mcp9808TUpperStatus = mcp9808TempSen.getStatus();
        #if defined(DEBUG52)  
            Serial << ("Upper Limit: ") << SensorStruct.mcp9808TupperC << ("C  ") << SensorStruct.mcp9808TupperF << ("F  Alert: ") << _HEX(SensorStruct.mcp9808TUpperStatus) << endl;
        #endif

        SensorStruct.mcp9808TlowerC = mcp9808TempSen.getTlower();
        SensorStruct.mcp9808TlowerF = ( (SensorStruct.mcp9808TlowerC  * 1.8) + 32.0);
        SensorStruct.mcp9808TlowerStatus = mcp9808TempSen.getStatus();
        #if defined(DEBUG52)  
            Serial << ("Lower Limit: ") << SensorStruct.mcp9808TlowerC << ("C  ") << SensorStruct.mcp9808TlowerF << ("F  Alert: ") << _HEX(SensorStruct.mcp9808TlowerStatus) << endl;
        #endif

        SensorStruct.mcp9808TCriticalC = mcp9808TempSen.getTcritical();
        SensorStruct.mcp9808TCriticalF = ( (SensorStruct.mcp9808TCriticalC  * 1.8) + 32.0);
        SensorStruct.mcp9808TCriticalStatus = mcp9808TempSen.getStatus();
        #if defined(DEBUG52)  
            Serial << ("Reading Critical Limit: ") << SensorStruct.mcp9808TCriticalC << ("C  ") << SensorStruct.mcp9808TCriticalF << ("F  Alert: ") << _HEX(SensorStruct.mcp9808TCriticalStatus) << endl;
        #endif

        SensorStruct.mcp9808DataValid = true;  // Set Valid Data flag true (Producer).

    }


void processTMP117Data(void)
    {
        SensorStruct.tmp117TempC = tmp117TempSen.getTemperature();
        SensorStruct.tmp117TempF = ( (SensorStruct.tmp117TempC * 1.8) + 32);
        SensorStruct.tmp117AlertType = tmp117TempSen.getAlertType(); 
        #if defined(DEBUG53)
            Serial << ("\nTMP117 Temperature : ") << SensorStruct.tmp117TempC << ("C  ") << SensorStruct.tmp117TempF << ("F") << endl;
            if (SensorStruct.tmp117AlertType == TMP117_ALERT::NOALERT)
                {
                    Serial.println(F("No Alert Occured...\n"));
                }
            else if (SensorStruct.tmp117AlertType == TMP117_ALERT::HIGHALERT)
                {
                    Serial.println(F("High Temperature Alert Occured...\n"));
                }
            else if (SensorStruct.tmp117AlertType == TMP117_ALERT::LOWALERT)
                {
                    Serial.println(F("Low Temperature Alert Occured...\n"));
                }
        #endif

        SensorStruct.tmp117DataValid = true;  // Set Valid Data flag true (Producer).
    }


/*
*  This sub Calibrates the temperture Sensor Mcp9808 using the TMP117 very high accuracy Temp Sensor (Temperature Reference). 
*/
void calibrateMCP9808TempSensor(void)
    {
        float tmpDifference;

        #if defined(DEBUG_CALIBRATE_MCP9808_SENSOR)
            Serial.println(F("\nCalibrating the MCP9808 Temp Sensor..."));
        #endif
        if (SensorStruct.tmp117TempC > SensorStruct.mcp9808TempC)
            {
                // This means that the tmp117 temp is more than the MCP9808 temp so will have add (positive number) the difference from the MCP9808.
                tmpDifference = (SensorStruct.tmp117TempC - SensorStruct.mcp9808TempC);
                mcp9808TempSen.setOffset(tmpDifference);
                #if defined(DEBUG_CALIBRATE_MCP9808_SENSOR)
                    Serial.print(F("Calibrating BME680 Temp Sensor, tmp117 is more than MCP9808  - Temp Difference: "));
                    Serial.println(tmpDifference, 5);
                #endif
                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

            }
        else
                // This means that the tmp117 temp is less than the MCP9808 temp so will have subtract the difference from the MCP9808.
            {
                tmpDifference = (SensorStruct.tmp117TempC - SensorStruct.mcp9808TempC);
                mcp9808TempSen.setOffset(tmpDifference);
                #if defined(DEBUG_CALIBRATE_MCP9808_SENSOR)
                    Serial.print(F("Calibrating BME680 Temp Sensor, tmp117 is less than MCP9808 - Temp Difference: "));
                    Serial.println(tmpDifference, 5);
                #endif
                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

            }
        #if defined(DEBUG_CALIBRATE_MCP9808_SENSOR)
            Serial.println(F(""));
        #endif    
    }


/*
*  This sub Calibrates the temperature Sensor BMP680 using the TMP117 very high accuracy Temp Sensor as a Temperature Reference. 
*/
void calibrateBME680TempSensor(void)
    {
        float tempDifference, bmeCalibOffset, temp;

        #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
            Serial.println(F("\nCalibrating the BME680 Temp Sensor..."));
        #endif
        if (SensorStruct.tmp117TempC > SensorStruct.BME680TemperatureC)  // Should be a positive number.
            {
                // This means that the tmp117 temp is more than the BME680 temp so will have add (positive number) the difference from the BMP680.
                tempDifference = (SensorStruct.tmp117TempC - SensorStruct.BME680TemperatureC);

                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                    Serial.println(F("The tmp117 is more than the bme680..."));
                    Serial.printf("The temp difference is: %f\n", tempDifference);
                #endif

                bmeCalibOffset = BME680Sensor.get_offset_temp();
                if (bmeCalibOffset == tempDifference)
                    {
                        #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                            Serial.println(F("Temp Difference is equal to current calibration offset, no adjusment is required..."));
                            Serial.printf("The current Calibration Offset is: %f, the temp difference between the TMP117 and BME680 is: %f...\n", bmeCalibOffset, tempDifference);
                        #endif
                        BME680CalibrateFlag = false;
                        return;
                    }
                else if(bmeCalibOffset > tempDifference)  // The BME Library offset adjustment is larger than the temp difference between the tmp117 and the bme680.
                    {
                        #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                            Serial.println(F("Current calibration offset is more than the Temp Difference, adjusting..."));
                            Serial.printf("The current Calibration Offset is: %f, the temp difference between the TMP117 and BME680 is: %f...\n", bmeCalibOffset, tempDifference);
                        #endif

                        temp = (bmeCalibOffset - tempDifference);  // Subtract the temp difference from the Calibration Offset and put into the Temporary space.

                        if (bmeCalibOffset > 0)  // The BME680 Calibration Offset number is positive
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is positive, subtracting the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset - temp);  // Subtract the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;   
                            }
                        else if (bmeCalibOffset < 0)  // The BME680 Calibration Offset number is negative.
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is negative, adding the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset + temp);  // Add the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                        else  // The BME680 Calibration Offset number is zero
                            {
                                BME680Sensor.set_offset_temp(bmeCalibOffset - temp);  // Subtract the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                    }
                else
                    {
                        #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                            Serial.println(F("Current calibration offset is less than the Temp Difference..."));
                            Serial.printf("The current Calibration Offset is: %f, the temp difference between the TMP117 and BME680 is: %f...\n", bmeCalibOffset, tempDifference);
                        #endif

                        temp = (tempDifference - bmeCalibOffset);  // Subtract the Calibration Offset from the temp difference and put into the Temporary space.

                        if (bmeCalibOffset > 0)  // The BME680 Calibration Offset number is positive
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is positive, subtracting the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("This is the new Offset_temp: %f...\n", (bmeCalibOffset + temp));
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset + temp);  // Subtract the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;   
                            }
                        else if (bmeCalibOffset < 0)  // The BME680 Calibration Offset number is negative.
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is negative, adding the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("This is the new Offset_temp: %f...\n", (bmeCalibOffset - temp));
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset - temp);  // Add the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                        else  // The BME680 Calibration Offset number is zero
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is zero, adding the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("This is the new Offset_temp: %f...\n", (bmeCalibOffset + temp));
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset + temp);  // Add the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                    }

                
            }
        else
                // This means that the tmp117 temp is less than the BME 680 temp so will have subtract the difference from the BMP680.
            {
                tempDifference = (SensorStruct.tmp117TempC - SensorStruct.BME680TemperatureC);  // Should be a negative number.

                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                    Serial.println(F("The tmp117 is less than the bme680..."));
                    Serial.printf("The temp difference is: %f\n", tempDifference);
                #endif

                bmeCalibOffset = BME680Sensor.get_offset_temp();

                if (bmeCalibOffset == tempDifference)
                    {
                        #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                            Serial.println(F("Temp Difference is equal to current calibration offset, no adjusment is required..."));
                            Serial.printf("The current Calibration Offset is: %f, the temp difference between the TMP117 and BME680 is: %f...\n", bmeCalibOffset, tempDifference);
                        #endif

                        BME680CalibrateFlag = false;
                        return;
                    }
                else if(bmeCalibOffset > tempDifference)  // The BME Library offset adjustment is larger than the temp difference between the tmp117 and the bme680.
                    {
                        #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                            Serial.println(F("Current calibration offset is more than the Temp Difference, adjusting..."));
                        #endif

                        temp = (bmeCalibOffset - tempDifference);  // Subtract the temp difference (negative value) from the Calibration Offset and put into the Temporary space.

                        if (bmeCalibOffset > 0)  // The BME680 Calibration Offset number is positive
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is positive, subtracting the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("The current Calibration Offset is: %f, the updated calib offset is: %f...\n", bmeCalibOffset, (bmeCalibOffset + temp));
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset + temp);  // Subtract the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;   
                            }
                        else if (bmeCalibOffset < 0)  // The BME680 Calibration Offset number is negative.
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is negative, adding the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("The current Calibration Offset is: %f, the updated calib offset is: %f...\n", bmeCalibOffset, (bmeCalibOffset - temp));
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset - temp);  // Add the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                        else  // The BME680 Calibration Offset number is zero
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is zero, subtracting the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("The current Calibration Offset is: %f, the updated calib offset is: %f...\n", bmeCalibOffset, (-temp));
                                #endif
                                BME680Sensor.set_offset_temp(-temp);  // temp is the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                    }
                else
                    {
                        #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                            Serial.println(F("Current calibration offset is less than the Temp Difference..."));
                            Serial.printf("The current Calibration Offset is: %f, the temp difference between the TMP117 and BME680 is: %f...\n", bmeCalibOffset, tempDifference);
                        #endif

                        temp = (tempDifference - bmeCalibOffset);  // Subtract the Calibration Offset from the temp difference and put into the Temporary space.

                        if (bmeCalibOffset > 0)  // The BME680 Calibration Offset number is positive
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is positive, subtracting the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("This is the new Offset_temp: %f...\n", (bmeCalibOffset - temp));
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset - temp);  // Subtract the difference from the Calibration Offset. 
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;  
                            }
                        else if (bmeCalibOffset < 0)  // The BME680 Calibration Offset number is negative.
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is negative, adding the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("This is the new Offset_temp: %f...\n", (bmeCalibOffset + temp));
                                #endif
                                BME680Sensor.set_offset_temp(bmeCalibOffset + temp);  // Add the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                        else  // The BME680 Calibration Offset number is zero
                            {
                                #if defined(DEBUG_CALIBRATE_BME680_SENSOR)
                                    Serial.println(F("Current calibration offset value is zero, adding the difference between calib offset and the temp difference..."));
                                    Serial.printf("The current Calibration Offset is: %f, the difference between calib offset and the temp difference is: %f...\n", bmeCalibOffset, temp);
                                    Serial.printf("This is the new Offset_temp: %f...\n", (temp));
                                #endif
                                BME680Sensor.set_offset_temp(temp);  // Add the difference from the Calibration Offset.
                                // Now put the calibration offset number in the EEPROM for cold startup temperature correction.

                                BME680CalibrateFlag = false;
                                return;    
                            }
                    }
            }
    }
 

/*
*  This Sub checks the TMP117 high accuracy Temp Sensor (Temperature Reference) output and rounds the temperature values to 2 decimal places from
*  the Sensor Struct against the values from other two temperature Sensors (BME680 and MCP9808) by comparison.
*  If the rounded (It needs the <math.h> library) values are not the same it sets the Calibrate flag.
*
*  WHEN MEASURING TEMPERATURES TWO DECIMAL PLACES IS MORE THAN ADEQUATE FOR MOST PURPOSES.
*
*  It uses this formula to round to the nearest 2 decimal places:
*  float nearest = roundf(val * 100) / 100;  // Result: 37.78
*
*  These other formulas round to 2 decimal places down:
*  float rounded_down = floorf(val * 100) / 100;  // Result: 37.77
*
*  And round to 2 decimal places up:
*  float rounded_up = ceilf(val * 100) / 100;  // Result: 37.78
*/
void compareTMP117WithOtherTempSensors(void)
    {
        #if defined(DEBUG_COMPARETMP117_WITHOTHERS)
            Serial.println(F("\nThis is the Compare TMP117 with Other Temp Sensors Sub..."));
            Serial << ("Rounding the 3 floats from the Sensor Struct to 2 decimal places...")  << endl;
        #endif

        float nearesttmp117Temp, nearestBME680Temp, nearestmcp9808Temp;  // Need some temporary floats.
        
        nearesttmp117Temp = (roundf(SensorStruct.tmp117TempC * 100) / 100);  // Result will be rounded to nearest 2 decimal places

        nearestBME680Temp = (roundf(SensorStruct.BME680TemperatureC * 100) / 100);  // Result will be rounded to nearest 2 decimal places.

        nearestmcp9808Temp = (roundf(SensorStruct.mcp9808TempC * 100) / 100);  // Result will be rounded to nearest 2 decimal places

        #if defined(DEBUG_COMPARETMP117_WITHOTHERS)
            Serial.print(F("This is the Temperature in C from the Sensor Struct array for the TMP117: "));
            Serial.println(SensorStruct.tmp117TempC,5);
            Serial.print(F("This is the Temperature in C from the Sensor Struct array for the TMP117 rounded to 2 decimal places: "));
            Serial.println(nearesttmp117Temp, 5);
            Serial.println(F(""));

            Serial.print(F("This is the Temperature in C from the Sensor Struct array for the BME680: "));
            Serial.println(SensorStruct.BME680TemperatureC, 5);
            Serial.print(F("This is the Temperature in C from the Sensor Struct array or the BME680 rounded to 2 decimal places: "));
            Serial.println(nearestBME680Temp, 5);
            Serial.println(F(""));

            Serial.print(F("This is the Temperature in C from the Sensor Struct array for the MCP9808: "));
            Serial.println(SensorStruct.mcp9808TempC, 5);
            Serial.print(F("This is the Temperature in C from the Sensor Struct array for the MCP9808 rounded to 2 decimal places: "));
            Serial.println(nearestmcp9808Temp, 5);
            Serial.println(F(""));
        #endif

        #if defined(DEBUG_COMPARETMP117_WITHOTHERS)
            Serial.println(F("Comparing the TMP117 with the BME680 Temp Sensor..."));
        #endif
        if (nearesttmp117Temp != nearestBME680Temp)
            {
                BME680CalibrateFlag = true;
                #if defined(DEBUG_COMPARETMP117_WITHOTHERS)
                    Serial.println(F("TMP117 Temp Sensor is different than the BME680 setting BME680 Calibrate flag true...\n"));
                #endif
            }

        #if defined(DEBUG_COMPARETMP117_WITHOTHERS)
            Serial.println(F("Comparing the TMP117 with the MCP9808 Temp Sensor..."));
        #endif
        if (nearesttmp117Temp != nearestmcp9808Temp)
            {
                MCP9808CalibrateFlag = true;
                #if defined(DEBUG_COMPARETMP117_WITHOTHERS)
                    Serial.println(F("TMP117 Temp Sensor is different than the BME680 setting MCP9808 Calibrate flag true...\n"));
                #endif
            }
        
        
    }


void clearStatusLine(void)
    {
        tft.printStatusLine(0,myColors[1],myColors[0],"                                                                                                                             ");
    }


/****************************************************************************************************************************
*
****************************************************************************************************************************/
bool continueLogggingDataToRamDisk()
    {
        if( qspiRamFileSystem.usedSize() == qspiRamFileSystem.totalSize() )
            {
                #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
		            Serial.printf("\n\n\tWARNING: DISK FULL >>>>>  Bytes Used: %llu, Bytes Total:%llu\n\n", qspiRamFileSystem.usedSize(), qspiRamFileSystem.totalSize());
                #endif
		        diskFull = true;

                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[2],myColors[0],"WARNING: DISK FULL >>>>>");
                delay(MainLoopStatuslineDisplayTime);
	         }

        if ( (noLittleFS == false) && (write_data == true) && (diskFull == false))
            {
                #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                    Serial.println(F("\nThis is the continueLoggingDataToRamDisk function..."));
                    Serial.println(F("Creating the data line for the CSV file..."));
                    Serial.println(F("Now checking if the data file on the RAMDisk is open for write..."));
                #endif

                if (!qspiRamDataFile)  // If the Data File is not open than open It before trying to write to it.
                    {
                        #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                            Serial.println(F("File is not open, So opening it..."));
                        #endif
                        qspiRamDataFile = qspiRamFileSystem.open(FileNameChars, FILE_WRITE);

                        if (!qspiRamDataFile)
                            {
                                #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                                    Serial.println(F("Error - File did not open when requested to open..."));
                                #endif

                                return false; 
                            }
                    }
                else
                    {
                        #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                            Serial.println(F("File is open and ready for writing..."));
                         #endif
                    }
                
                #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                    Serial.println(F("Now creating the actual data line to be written from the Sensor Struct..."));
                 #endif
                
                NumChars = sprintf(dataFileLine,"%u:%u:%u,%3.2f,%3.2f,%2.2f,%2.2f,%3.2f,%5.2f,%3.2f,%3.2f,%3.2f,%3.2f\r", currentTime[0], currentTime[1], currentTime[2], SensorStruct.BME680TemperatureC, SensorStruct.BME680TemperatureF, SensorStruct.BME680Baro_Pressure, SensorStruct.BME680Relative_Humidity, SensorStruct.BME680gasSensor, SensorStruct.BME680altitude, SensorStruct.mcp9808TempC, SensorStruct.mcp9808TempF, SensorStruct.tmp117TempC, SensorStruct.tmp117TempF);

                #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                    Serial.print(F("Size of the assembled character string for the File Data: "));
                    Serial.println(NumChars);

                    Serial.print(F("File Line assembled for this record: "));
                    Serial.println(dataFileLine);
                #endif
                
                NumOfCharsWritten = qspiRamDataFile.write(dataFileLine);

                #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                    Serial.print(F("Number of Characters written to file: "));
                    Serial.println(NumOfCharsWritten);
                #endif

                if (NumOfCharsWritten == 0)
                    {
                        #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                            Serial.println(F("Error, number of characters written to file is zero..."));
                        #endif

                        clearStatusLine();  // Clear any leftovers in the status line.
                        tft.printStatusLine(0,myColors[2],myColors[0],"Error, number of characters written to file is zero...");
                        delay(MainLoopStatuslineDisplayTime);
                        write_data = false;
                        return false;
                    }
                else
                    {
                        record_count++;

                        #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                            Serial.println(F("Printed the data line to the File..."));
                            Serial.print(F("The Data File record count is now: "));
                            Serial.print(record_count);
                            Serial.println(F(" Records"));
                            Serial.printf("The file is now this size (In bytes) Total: %llu\n", qspiRamDataFile.size());
                        #endif

                        qspiRamDataFile.flush();  // Sent data to file from buffer.
                        return true;
                    }
            }
        else
            {
                #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                    Serial.println(F("Error, Problem with file or disk..."));
                #endif

                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[2],myColors[0],"Error, Problem with file or disk...");
                delay(MainLoopStatuslineDisplayTime);
                
                if (noLittleFS == true)
                    {
                        #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                            Serial.println(F("noLittleFS is true..."));
                        #endif

                        clearStatusLine();  // Clear any leftovers in the status line.
                        tft.printStatusLine(0,myColors[2],myColors[0],"noLittleFS is true...");
                        delay(MainLoopStatuslineDisplayTime);
                    }
                if (write_data == false)
                    {
                        #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                            Serial.println(F("write_data is false..."));
                        #endif

                        clearStatusLine();  // Clear any leftovers in the status line.
                        tft.printStatusLine(0,myColors[2],myColors[0],"write_data is false...");
                        delay(MainLoopStatuslineDisplayTime);
                    }
                if (diskFull == true)
                    {
                        #if defined(DEBUG_LOGGINGDATA_TO_RAMDISK_SUB)
                            Serial.println(F("diskFull is true..."));
                        #endif

                        clearStatusLine();  // Clear any leftovers in the status line.
                        tft.printStatusLine(0,myColors[2],myColors[0],"diskFull is true...");
                        delay(MainLoopStatuslineDisplayTime);
                    }
                return false;              
            } 
    }


void stopLoggingDataToRamDisk()
    {
        #if defined(DEBUG_STOPLOGGINGDATATO_DISK_SUB)
            Serial.println(F("\nStopped Logging Data!!!"));
        #endif
        write_data = false;
        
        qspiRamDataFile.close();  // Closes the data file.

        #if defined(DEBUG_STOPLOGGINGDATATO_DISK_SUB)
            Serial.printf("Records written = %d\n\n", record_count);
        #endif

        record_count = 0;  // Reset the record count for next batch of data.
    }


/*********************************************************************************************************************
*   This Subroutine prints the number of spaces you specify to the Serial Port.
*   Needs:
*       num        : The number of spaces you specify.
*   Returns:
*       Nothing
**********************************************************************************************************************/
void printSpaces(int num)
	{
  		for (int i=0; i < num; i++)
  			{
    			Serial.print(" ");
  			}
	}


/*********************************************************************************************************************
*   This Subroutine prints the Time and Data to the Serial Port.
*   Needs:
*       tm        : (DateTimeFields) type of struct. (Not to be confused with a (tmElements_t) type of struct)
*   Returns:
*       Nothing
**********************************************************************************************************************/
void printTime(const DateTimeFields tm)
	{
  		const char *months[12] = 
  			{
    			"January","February","March","April","May","June",
    			"July","August","September","October","November","December"
  			};
  			
  		if (tm.hour < 10) Serial.print('0');
  		
  		Serial.print(tm.hour);
  		Serial.print(':');
  		
  		if (tm.min < 10) Serial.print('0');
  		
  		Serial.print(tm.min);
  		Serial.print("  ");
  		Serial.print(tm.mon < 12 ? months[tm.mon] : "???");
  		Serial.print(" ");
  		Serial.print(tm.mday);
  		Serial.print(", ");
  		Serial.print(tm.year + 1900);
	}


/*********************************************************************************************************************
*   This Subroutine prints the contents of the directory you specify.
*   Needs:
*       dir                     : A directory File object which is the source of the data being printed.
*       dirName                 : A Directory Name in a char[] format.
*       numSpaces               : The number of spaces to print before the file or directory found in the directory.
*   Returns:
*       Nothing
**********************************************************************************************************************/
void printDirectory(File dir, int numSpaces)
	{
        #if defined(DEBUG_PRINT_DIRECTORY_SUB)
            Serial.printf("    This is the print Directory Sub...\n");
        #endif

   		while(true)
   			{
     			File entry = dir.openNextFile();
     			if (! entry)
     				{
                        #if defined(DEBUG_PRINT_DIRECTORY_SUB)
       					    Serial.println(F("    ** no more files **"));
                        #endif

       					break;
     				}
     				
     			printSpaces(numSpaces);
                //tempName = entry.name();
     			Serial.print(entry.name());
     			
     			if (entry.isDirectory())
     				{
       					Serial.println("/");
       					printDirectory(entry, numSpaces + 2);
     				}
     			else
     				{
       					// files have sizes, directories do not
       					int n = log10f(entry.size());
       					if (n < 0) n = 10;
       					if (n > 10) n = 10;
       						{
       							printSpaces(50 - numSpaces - strlen(entry.name()) - n);
       						}
       						
       					Serial.print("  ");
       					Serial.print(entry.size(), DEC);
       					
                        DateTimeFields datetime;
       					if (entry.getModifyTime(datetime))
       						{
         						printSpaces(4);
         						printTime(datetime);
       						}
       						
       					Serial.println();
     				}
     				
     			entry.close();
  			 }
	}


/*********************************************************************************************************************
*   This Subroutine copies the contents of a RAMDISK data file to the SD Card into the directory you specify.
*   Needs:
*       sourceDirectory         : A File object which is the source of the data being transfered.
*       DestinationDirectory    : A character array pointer pointing to the destination directory.
*       DestinationFile         : A character array pointer pointing to the destination file name.
*   Returns:
*       True if transfer was complete.
*       False if transfer failed.
**********************************************************************************************************************/
bool copyDataFileFromRAMDiskToSDCard(char * SourceFileName = NULL, const char * DestinationDirectory = NULL,  const char * DestinationFile = NULL)
    {
        char destinationSDCardDir[60];

        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
            Serial.println(F("Destination to the SD Card, do not Initilize the File System, do not Initialize the File, into a Subdirectory yes..."));
        #endif

        if (DestinationDirectory == NULL)
            {
                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			              Serial.println(F("Error, Destination Directory is NULL...\n"));
                #endif
                return false;
            }

        strcpy( destinationSDCardDir, DestinationDirectory );

        if (sdCardDestDirectoryPresent == false)
            {
                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                    Serial.printf("This is the Destination Directory: %s\n", destinationSDCardDir);
                    Serial.println(F("Checking to see if the Destination Directory exists on the SD Card already..."));
                #endif

                if ( !sdSDIO.exists(destinationSDCardDir) )
                    {  // If the directory does not exist than create it.
                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                            Serial.println(F("The destination directory does not exist, creating it now..."));
                        #endif
                        if (!sdSDIO.mkdir( destinationSDCardDir ))
                            {
                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                                    Serial.println(F("Error Creating the Sub Directory Failed...\n"));
                                #endif
                                sdCardDestDirectoryPresent = false;
                                return false;   
                            }
                        else
                            {
                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                                    Serial.println(F("Created the Subdirectory..."));
                                #endif
                                sdCardDestDirectoryPresent = true;
                            }
                    }
                else
                    {
                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                            Serial.println(F("The Sub Directory exists..."));
                        #endif
                        sdCardDestDirectoryPresent = true;
                    }    
            }
        else
            {
                #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                    Serial.println(F("Changing to and Opening the SD Card Destination Directory on the SD Card..."));
                #endif

                sprintf(sdCardDestDirectory, "/%s", SDDataDirectory );  // Insert the "/" before the SD Card Dest directory name.

                // Need to change to the Destination Directory before we can create a file inside it.
                if (!sdSDIO.sdfs.chdir(sdCardDestDirectory))
                    {
                        // The SD Card Destination Directory did not open.
                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                            Serial.printf("Error - Changing to the Sub Directory %s Failed...\n", sdCardDestDirectory );
                        #endif
                        sdCardDestDirectoryOpen = false;
                        return false;
                    }
                else
                    {
                        // Changing to the SD Card Destination Directory.
                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                            Serial.printf("Changing to the Sub Directory %s was successful...\n", sdCardDestDirectory );
                        #endif
                        /*
                        // Now opening the SD Card Destination Directory (subdirectory)
                        sdCardDestinationDirectory = sdSDIO.open(sdCardDestDirectory, FILE_WRITE);
                        if (!sdCardDestinationDirectory)
                            {
                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                                    Serial.printf("Opening the Sub Directory %s Failed...\n", sdCardDestDirectory );
                                #endif

                            sdCardDestDirectoryOpen = false;
                            }
                        else
                            {
                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                                    Serial.printf("Opened the Sub Directory %s...\n", sdCardDestDirectory );
                                #endif
                        */
                            sdCardDestDirectoryOpen = true;
                            //}
                    }
                /*
                #if defined(SDCARD_PRINT_DIRECTORY)
                    if (sdCardDestDirectoryOpen == true)
                        {
                            Serial.println(F("Printing the contents of the Destination Directory..."));
                            printDirectory(sdCardDestinationDirectory, 4);  // Print the current directory to Serial (USB).
                        }
                    else
                        {
                            Serial.printf("Error - The Sub Directory %s is not open...\n", sdCardDestDirectory );
                        }
                #endif
                */
            }

        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			Serial.println(F("Creating the total file path for the Destination File..."));
        #endif

        //strcpy (destinationSDCardFile, DestinationFile );  // Add the file name to the destination

        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
            Serial.printf("This is the total file path for the Destination File: %s\n", DestinationFile);
        #endif

        if ( (sdCardDestDirectoryOpen == true) && (sdCardDestDirectoryPresent == true))
            {
                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			        Serial.println(F("Creating the destination file for Write on the SD Card..."));
                #endif

                SDCardDataFile = sdSDIO.open(DestinationFile, FILE_WRITE_BEGIN);
                if (!SDCardDataFile)
                    {
                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			                Serial.println(F("Error - Creating the destination file Failed...\n"));
                        #endif
                        sdCardDestFileNamePresent = false;
                        return false;   
                    }
                    else
                        {
                           #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			                    Serial.println(F("Successfully created the destination file..."));
                            #endif
                            sdCardDestFileNamePresent = true; 
                        }    
            }
        
        if ( (sdCardDestFileNamePresent == true) )
            {
                fileLength = SDCardDataFile.size();

                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                    Serial.print(DestinationFile);
                    Serial.print(" Started with ");
                    Serial.print(fileLength);
                    Serial.println(" bytes\n");
                #endif

                //  Destination file on sd card is open and ready for transfer.
                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			        Serial.println(F("Destination file on sd card is open and ready for transfer..."));
                    Serial.println(F("Checking for the existance of the source file, is it open??..."));
                #endif

                if (qspiRamDataFilePresent == true)
                    {
                        if (!qspiRamDataFile)  // Is the source file open?
                            {
                                // No the source file is not open, so open it.
                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			                        Serial.println(F("The source file is not open, Open it first..."));
                                #endif

                                qspiRamDataFile = qspiRamFileSystem.open(SourceFileName, FILE_READ);  // Opens the data file for read.
                                if ( !qspiRamDataFile )
                                    {
                                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			                                Serial.println(F("Error opening Ram Data Source file...\n"));
                                        #endif
                                        ramDisk_previously_present = false;
                                        qspiRamDataFilePresent = false;

                                        return false; 
                                    }
                                else
                                    {
                                        ramDisk_previously_present = true;
                                        // Source file in the PSRAM Ram Disk is ready for transfer.
                                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
			                                Serial.println(F("Source file in the PSRAM Ram Disk is ready for transfer..."));
                                        #endif
    
		                                uint64_t sourcefileSize, sizeOfByteCount = 0, transferSize = 1;
		                                char transferBuffer[512];

		                                sourcefileSize = qspiRamDataFile.size();

                                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
					                        Serial.printf("Source File Size is: %llu Bytes in size.\n\n", sourcefileSize );
                                            Serial.println(F("STARTING the Copy from LittleFS to SD Card Transfer..."));
                                        #endif

		                                while ( qspiRamDataFile.available() )
			                                {
				                                if ( sourcefileSize < sizeOfByteCount )
                                                    {
                                                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                                                            Serial.printf("Transfer of Data from %c to the SD Card is Complete\n", qspiRamDataFile.name() );
                                                        #endif

                                                        break;  // This means that the transfer is completed.
                                                    } 
				                                if ( (sourcefileSize - sizeOfByteCount) >= sizeof(transferBuffer) )
					                                {
						                                transferSize = sizeof(transferBuffer);
					                                }
				                                else
					                                {
						                                transferSize = (sourcefileSize - sizeOfByteCount);
					                                }

				                                qspiRamDataFile.read( transferBuffer , transferSize );
				                                SDCardDataFile.write( transferBuffer , transferSize );
				                                sizeOfByteCount += transferSize;
                                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                                                    Serial.print(F("."));  // Print progress indicators.
                                                #endif
			                                }
                                        #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                                            Serial.println(F(""));  // Print CRLF.
                                        #endif

                                        SDCardDataFile.flush();  // Flush the data to the SD Card

		                                if (sourcefileSize != sizeOfByteCount )
			                                {
                                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
					                                Serial.print(F("File Size Error in Source File..."));
					                                Serial.println( qspiRamDataFile.name() );
                                                    Serial.println(F("Closing the source file and destination files...\n"));
                                                #endif
                                                qspiRamDataFile.close();
                                                SDCardDataFile.close();
                                                return false;
			                                }
                                        else
                                            {
                                                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
					                                Serial.print(F("Closing the source and destination files...\n"));
                                                #endif
                                                qspiRamDataFile.close();
                                                SDCardDataFile.close();
                                                return true;
                                            }
                                    }               
                            }
                    }
            }
        else
            {
                #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
					Serial.print(F("Error - No File was created in the Destination Directory: "));
					Serial.println( DestinationFile );
                    Serial.println(F("Closing the source file...\n"));
                #endif
                qspiRamDataFile.close();
                return false;
            }
        return false;
    }


/********************************************************************************************************************************
*   This Subroutine renames the existing RAMDISK data file, clears the data, and opens it to contine logging Environmental Data.
*   Needs:
*       Nothing
*
*   Returns:
*       True if successful.
*       False if unsuccessful.
********************************************************************************************************************************/
bool createNewDataFileAndStartLogging()
    {
        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
            Serial.println(F("This is the createNewDataFileAndStartLogging Sub..."));
        #endif

        #if defined(DEBUG_DAYOFMONTH_FOR_TESTING)
            NewfileNameDay++;  // increment the day by one.

            sprintf(NewFileNameChars, "EdsEnvSysData-%d-%d-%d.csv", currentDate[1], NewfileNameDay, currentDate[2]  );  // Fill the FileNameChars array with a new file name.

            // This section is strictly for testing of the copying files from RamDisk to SD Card.

            if (NewfileNameDay > 30)
                {
                    NewfileNameDay = 1;
                }
        #else
            sprintf(NewFileNameChars, "EdsEnvSysData-%d-%d-%d.csv", currentDate[1], currentDate[0], currentDate[2] );  // Fill the FileNameChars array with a new file name.
        #endif

        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
            Serial.printf("Checking if %s is Open and ready for write...\n", FileNameChars);
        #endif

        if (!qspiRamDataFile)
            {
                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                    Serial.printf("%s is not Open, Now Opening it...\n", FileNameChars);
                #endif
                qspiRamDataFile = qspiRamFileSystem.open(FileNameChars, FILE_WRITE);

                if (!qspiRamDataFile)
                {
                    #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                        Serial.printf("Could not open the File %s...\n", FileNameChars);
                    #endif
                    return false;
                }  
            }

        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
            Serial.println(F("Now truncating the Original file..."));  
        #endif

        if(qspiRamDataFile.truncate() )  // This should remove all data in the original file.
            {
                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                    Serial.println(F("Data in File has been removed, ready to begin filling again..."));
                    Serial.println(F("Resetting the Record Count to zero..."));
                    Serial.printf("Size of File is now: %llu\n", qspiRamDataFile.size());
                #endif
            }
        else
            {
                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                    Serial.println(F("Error - File truncate failed...\n"));  
                #endif
                return false;  // Error truncating failed.
            }

        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
            Serial.printf("Now doing file flush to send data to the disk...\n" );
        #endif
        qspiRamDataFile.flush();
        
        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
            Serial.printf("Now changing the name to %s for the RAMDISK data file...\n", NewFileNameChars );
        #endif

        if(!qspiRamFileSystem.rename(FileNameChars, NewFileNameChars))
            {
                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                    Serial.println(F("Error Renaming the existing file to a new Name...\n"));
                #endif

                return false;
            } 
        else
            {
                // Opens the file for Write.
                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                    Serial.printf("Opening the newly renamed file: %s for write...\n", NewFileNameChars);
                #endif

                qspiRamDataFile = qspiRamFileSystem.open(NewFileNameChars, FILE_WRITE);
                noLittleFS = false;

                if(qspiRamDataFile)
                    {
                        record_count = 0;  // Reseting the Record count to zero.

                        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                            Serial.println(F("Now creating the CSV header line at the top of the File..."));
                        #endif
                    
                        NumChars = sprintf(dataFileLine,"  Time  ,BME680TempC,BME680TempC,BME680BarPres,BME680RHumdity,BME680GasSensor,BME680Altitude,MCP9808TempC,MCP9808TempF,TMP117TempC,TMP117TempF\r");

                        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                            Serial.print(F("Size of the assembled character array for the 1st line of the File: "));
                            Serial.println(NumChars);
                            Serial.println(F("This is the Line of characters to put into the File:"));
                            Serial.println(dataFileLine);
                        #endif

                        NumOfCharsWritten = qspiRamDataFile.write(dataFileLine);

                        qspiRamDataFile.flush();

                        SizeOfCurrentFile = qspiRamDataFile.size();

                        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                            Serial.printf("Number of characters written to File: %u\n", NumOfCharsWritten);
                            Serial.printf("Size of File now: %llu\n", SizeOfCurrentFile);
                        #endif        
                        
                        if (NumOfCharsWritten == 0)
                            {
                                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                                    Serial.println(F("Error, the number of characters written is zero..."));
                                #endif
                                write_data = false;
                                return false;
                            }
                        else if (SizeOfCurrentFile == 0)
                            {
                                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                                    Serial.println(F("Error, the size of the file is zero..."));
                                #endif
                                write_data = false;
                                return false;
                            }
                        else
                            {
                                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                                    Serial.println(F("Printed the header line to the File..."));
                                #endif
                                
                                record_count++;

                                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                                    Serial.print("The Data File record count is now: ");
                                    Serial.print(record_count);
                                    Serial.println(" Records");
                                #endif
                                
                                fileLength = qspiRamDataFile.size();
                                    
                                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                                    Serial.printf("%s is Now %llu Bytes.\n", NewFileNameChars, fileLength );
                                #endif

                                write_data = true;

                                #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                                    Serial.println("\nLogging Data!!!\n");
                                #endif
                                
                                sprintf(FileNameChars, NewFileNameChars);  // Fill the NameChars array with a new file name.

                                //index_ramDisk_storage = MTP.addFilesystem(qspiRamFileSystem, "qspiRamFileSystem");

                                return true;
                            }
                    }
                else
                    {
                        #if defined(DEBUG_CREATE_NEW_DATAFILE_AND_STARTLOGGING_SUB)
                            Serial.println(F("Newly renamed File did not open..."));
                        #endif
                        
                        return false;
                    }
            }
    }
     

void oneSecCallback(void)
    {
        oneSecFlag = true;
    }


/*********************************************************************************************************************
*   This is the 5 Milli Seconds Callback subroutine, it is called by an interrupt.
*   Needs:
*       Nothing
*   Returns:
*       Nothing
**********************************************************************************************************************/
void fiveMSCallback(void)
    {
        fiveMSFlag = true;
    }


/*********************************************************************************************************************
*   This is the 100 Milli Seconds Callback subroutine, it is called by an interrupt.
*   Needs:
*       Nothing
*   Returns:
*       Nothing
**********************************************************************************************************************/
void oneHundredMSCallback(void)
    {
        oneHundredMSFlag = true;
    }

/*********************************************************************************************************************
*   This is the WDT (Watch Dog Timer) Callback subroutine, it is called by an interrupt.
*   Needs:
*       Nothing
*   Returns:
*       Nothing
**********************************************************************************************************************/
void WDTCallback()
    {
        #if defined(DEBUG46)
            Serial << ("FEED THE DOG SOON, OR RESET!...")  << endl;
        #endif
    }


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/// System Setup routine.
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
void setup()
{
    volatile uint8_t numberOfSeconds = 0;
    uint8_t status = 0; // initialize the hardware

    Serial.begin(115200);
    Serial.print(CrashReport);

    long unsigned debug_start = millis ();
    while(!Serial && ((millis () - debug_start) <= 5000UL))
        {
            yield();
        }

    #if defined(DEBUG)
        Serial.println(F("\nSetup Starting the program.."));
    #endif

    Serial.println("Project version: " + String(VERSION));  // Print Project version.
    Serial.println("Build timestamp:" + String(BUILD_TIMESTAMP));  // Print build timestamp

    
    /*******************************************************************************************************************
    *                                           Setup the DS3231 Time Keeper IC.
    *******************************************************************************************************************/

   #if defined(DEBUG)
        Serial.println(F("Setting up the DS3231 Time Keeper IC..."));
    #endif

    // Start the I2C0 interface
	Wire.begin();  // This is for the DS3231 Time Keeper IC, and the AT24C32 EEPROM located on the DS3231 Clock Module.
    //Wire.setClock(400000UL);  // Set clock speed for Wire 0 to 400KHz to speed up the transactions.

    /*******************************************************************************************************************
    * Get current time from DS3231 and put into variables curentTime[0] (hh), [1] (mm), & [2] (ss).
    *******************************************************************************************************************/
    if (DS3231Clock.read(tm))
        {
            lastTimeSetByClock = makeTime(tm);  // Converts tmElements array into a Time_T (UNIX Time), LastTimeSetByClock is a Time_T (UNIX Time) variable.
                
            currentTime[0]      = (tm.Hour);
            currentTime[1]      = (tm.Minute);
            currentTime[2]      = (tm.Second);
            currentDate[0]      = (tm.Day);
            currentDate[1]      = (tm.Month);
            currentDate[2]      = (tmYearToCalendar(tm.Year));
            currentDayOfWeek    = (tm.Wday);  // tmElements uses day of the week from 1 -> 7.

            breakTime( lastTimeSetByClock, DateTimeFlds);  // This converts the UNIX Time to DateTimeFields format.
        }
    else
        {
            if (DS3231Clock.chipPresent())
                {
                    Serial.println(F("The DS3231 is stopped.  Please run the SetTime"));
                    Serial.println(F("example to initialize the time and begin running."));
                    Serial.println();
                }
            else
                {
                    Serial.println(F("DS3231 read error!  Please check the circuitry."));
                    Serial.println();
                }
        }
    
    #if defined(DEBUG)
        Serial.println(F("Finished setting up the DS3231 Time Keeper IC..."));
    #endif

    /********************************************************************************************************************
     *             Get Temp Sensor Calibration Offset Data from EEPROM and put in RAM.
     * This is where the info concerning the Temp Sensor Calibration Offset in EEPROM is placed in the appropriate variables in RAM.
     * The AT24C32 has 4096 bytes of memory storage (address 0 -> 0xFFF).
     ********************************************************************************************************************/
    #if defined(DEBUG)
        Serial.println(F("Inputting the permanent values from EEPROM to Memory..."));
    #endif

    float j = eepromMemory.readFloat(TempCalOffsetLocation);  // First 4 bytes is Temp Cal offset in float.
    #if defined(DEBUG40)  // This is the Reading Startup Values from EEPROM debug flag.
        Serial.print(F("Read EEPROM Memory Temp Sensor Calibration Offset, Temp Sensor Calibration Offset in Floating Point: "));
        Serial.println(j, 2);
        Serial.print(F("Temp Sensor Calibration Offset in float: "));
        Serial.println(j,2);
    #endif
    if ( (j > -5.0F) && (j < 5.0F) )
        {
            BME680Sensor.set_offset_temp(j);  // Sets the BME680 Temperature offset to number from EEPROM in degrees C.
        }
    else if (__isnanf(j))  // If the number in EEPROM is bogus set it to zero
        {
            j = 0;  // Set the floating point number to zero.
            eepromMemory.writeFloat(TempCalOffsetLocation, j);  // Set the Temp Sensor Offset Location to zero.
            #if defined(DEBUG40)  // This the the Reading Startup Values from EEPROM debug flag.
                Serial.println(F("Read EEPROM Memory Temp Sensor Calibration Offset, Temp Sensor Calibration Offset Number is NAN!\n"));
            #endif
        }
    else
        {
            j = 0;  // Set the floating point number to zero.
            eepromMemory.writeFloat(TempCalOffsetLocation, j);  // Set the Temp Sensor Offset Location to zero.
            #if defined(DEBUG40)  // This is the Reading Startup Values from EEPROM debug flag.
                Serial.println(F("Read EEPROM Memory Temp Sensor Calibration Offset, Temp Sensor Calibration Offset Number is out of range!\n"));
            #endif    
        }


    /********************************************************************************************************************
     *                  Get Time Zone Setting from EEPROM and put in RAM.
     * This is where the info concerning the Time Zone Setting in EEPROM is placed in the appropriate variables in RAM.
     * The AT24C32 has 4096 bytes of memory storage (address 0 -> 0xFFF).
     ********************************************************************************************************************/
    uint8_t k = eepromMemory.read(TimeZoneSettingLocation);  // This is Time zone in uint8_t.
    time_t temp_t;

    #if defined(DEBUG40)  // This the the Reading Startup Values from EEPROM debug flag.
        Serial.print(F("Read EEPROM Memory Time Zone Setting, This is the time zone number from EEPROM: "));
        Serial.println(k);

        Serial.print(F("Read EEPROM Memory Time Zone Setting, This is the size of the tzIndex: "));
        Serial.println( (sizeof(timezones) / 4) );  // The timezones array is an array of addresses.
    #endif
    
    if ( (k >= 0) && (k <= (sizeof(timezones) / 4) ) )  // This checks for the validity of the time zone.
        {
            tzIndex = k;
            tz = timezones[tzIndex];  // This variable is used to determine which time zone we are using.
            temp_t = ((*tz).toLocal(lastTimeSetByClock, &tcr));
            #if defined(DEBUG40)  // This the the Reading Startup Values from EEPROM debug flag.
                Serial.print(F("Read EEPROM Memory Time Zone Setting, set time to: "));
                Serial.println(tcr -> abbrev);
            #endif
        }
    else
        {
            tz = timezones[tzIndex];  // Set the time zone to the default.
            temp_t = ((*tz).toLocal(lastTimeSetByClock, &tcr));
            eepromMemory.write(TimeZoneSettingLocation, k);  // Fix number in EEPROM if its bad.
            #if defined(DEBUG40)  // This the the Reading Startup Values from EEPROM debug flag.
                Serial.print(F("Read EEPROM Memory Time Zone Setting, EEPROM Number was not valid, setting to: "));
                Serial.println(tcr -> abbrev);
            #endif
        }
    
    #if defined(DEBUG)
        Serial.println(F("Setup finished EEPROM setup.."));
    #endif


/*******************************************************************************************************************
*                                            Setup the TFT.
*******************************************************************************************************************/
    /// I'm guessing most copies of this display are using external PWM
    /// backlight control instead of the internal RA8876 PWM.
    /// Connect a Teensy pin to pin 14 on the display.
    /// Can use analogWrite() but I suggest you increase the PWM frequency first so it doesn't sing.
    pinMode(BACKLITE, OUTPUT);
    digitalWrite(BACKLITE, HIGH);
    
    #if defined(DEBUG)
        Serial.println(F("Setting up the TFT..."));
    #endif
    
    //	tft.begin(20000000);
    if (!tft.begin(28000000UL))  // Set the SPI speed to 28MHz.
        {
            #if defined(DEBUG0)
                Serial.println(F("TFT Inialize failed"));
            #endif
            
        }
    tft.graphicMode(true);
    tft.setTextCursor(0,0);
    tft.setFont(Arial_18);
    tft.setTextColor(WHITE);
   
    tft.setRotation(0);
    w = tft.width()-1;
    h = tft.height()-STATUS_LINE_HEIGHT-1;
    int16_t getTwidth(void);
	int16_t getTheight(void);

    fontXsize = tft.getFontWidth();
    Serial.print(F("fontXsize is: "));
    Serial.println(fontXsize, DEC);

    textXsize = tft.getTextX();
    Serial.print(F("textXsize is: "));
    Serial.println(textXsize, DEC);

    Twidth = tft.getTwidth();
    Serial.print(F("Twidth is: "));
    Serial.println(Twidth, DEC);

    fontYsize = tft.getFontHeight();
    Serial.print(F("fontYsize is: "));
    Serial.println(fontYsize, DEC);
    
    textYsize = tft.getTextY();
    Serial.print(F("textYsize is: "));
    Serial.println(textYsize, DEC);

    Theight = tft.getTheight();
    Serial.print(F("Theigth is: "));
    Serial.println(Theight, DEC);

    Serial.println(F("\n"));

    tft.fillScreen(BLACK);


    // Now set up the clock face:
    clockPos[0] = (tft.width() / 2);  // Clock center position in x direction (pixels).
    clockPos[1] = tft.height() / 2;  // Clock center position in y direction (pixels).
    clockPos[2] = (tft.height() / 2) - 60;  // Clock radius in pixels (600 / 2 - 60 = 240 pixels).
    
    oldPos[0] = clockPos[0];
    oldPos[1] = clockPos[1];
    oldPos[2] = clockPos[0];
    oldPos[3] = clockPos[1];
    oldPos[4] = clockPos[0];
    oldPos[5] = clockPos[1];

    drawGauge(clockPos, 0, 360,WHITE, WHITE, WHITE);  // This draws the the round clock face with the major & minor tick marks.

    /// Define array for Time in hours, minutes, and seconds: currentTime[3] = {12, 30, 0}; //hh, mm, ss
    /// Define array for Day, Month, and Year: currentDate[3] = {1, 1, 2022}; // Day, Month, Year
    drawClockHands(clockPos, currentTime, clockColors);

    /// Print the day, data, time, and time zone on the TFT using starting x position of (tft width / 2), starting y position of YLINE15 (14 * Ysize)
    drawPrintTime(( clockPos[0] - (fontXsize * 7)) , YLINE13, currentTime[0], currentTime[1], currentTime[2], currentDayOfWeek, currentDate[0], currentDate[1], currentDate[2], false);

    #if defined(DEBUG)
        Serial.println(F("Drew the clock face it should be in the center of the screen..."));
        clearStatusLine();  // Clear any leftovers in the status line.       
        tft.printStatusLine(0,myColors[1],myColors[0],"Drew the clock face it should be in the center of the screen...");
        delay(StatuslineDisplayTime);  
    #endif


/*******************************************************************************************************************
*                                       Setup the BME680 Sensor.
* This sensor is located on the Wire2 bus, pins 24 and 25 of the Teensy4.1.
*******************************************************************************************************************/            
    numberOfSeconds = 0;

    #if defined(DEBUG)
        Serial.println(F("\nInitializing BME680 Temp, Humdity, Baro Pressure, and Air Quality sensor..."));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Initializing BME680 Temp, Humdity, Baro Pressure, and Air Quality sensor...");
        delay(StatuslineDisplayTime);  
    #endif
    

   	while ( ( (status = BME680Sensor.begin(I2C_STANDARD_MODE, 0x77,  &Wire2)) == false ) && (numberOfSeconds < 10) )
        {
            #if defined (DEBUG0)
            	Serial.print(F("-  Unable to find BME680.\n"));
            	Serial << "checking for connection status, status: " << status << endl;
                Serial.println(F("Waiting 1 second and trying again"));
            #endif
            delay(1000);
            numberOfSeconds++;
       	}
        
    if (status != 0)
        {
            #if defined(DEBUG)
                Serial.println(F("- Setting 16x oversampling for all sensors"));
            #endif
   	 	    BME680Sensor.setOversampling(TemperatureSensor, Oversample16);  // Use enumerated type values
    	    BME680Sensor.setOversampling(HumiditySensor, Oversample16);     // Use enumerated type values
    	    BME680Sensor.setOversampling(PressureSensor, Oversample16);     // Use enumerated type values

            #if defined(DEBUG)
                Serial.println(F("- Setting IIR filter to a value of 4 samples"));
            #endif
		    BME680Sensor.setIIRFilter(IIR4);  // Use enumerated type values.

            #if defined(DEBUG)
                Serial.println(F("- Setting gas measurement to 320 Deg C for 150ms"));  // "°C" symbols
            #endif
		    BME680Sensor.setGas(320, 150);  // 320°c for 150 milliseconds.

            #if defined(DEBUG)
                Serial.println(F("Finished initializing BME680 sensor...\n"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[1],myColors[0],"Finished initializing BME680 sensor...");
                delay(StatuslineDisplayTime);
            #endif
            
            noBME680 = false;  // This means the BME680 is present and functioning.
        }
    else
        {
            #if defined(DEBUG)
                Serial.print(F("- Tried to establish contact with the BME680 10 Times over 10 Seconds - Setting noBME680 true!\n"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[1],myColors[0],"- Tried to establish contact with the BME680 10 Times over 10 Seconds - Setting noBME680 true!...");
                delay(StatuslineDisplayTime);
            #endif

            noBME680 = true;  // This means the BME680 is not present and non-functional.    
        }


/*******************************************************************************************************************
*                     Setup the MCP9808 High Accuracy Temp Sensor.
* This sensor is located on the Wire1 bus, pins 16 and 17 of the Teensy4.1.
*******************************************************************************************************************/

        #if defined(DEBUG)
            Serial.print(F("\nInitializing MCP9808 High Accuracy Temp sensor\n"));
            clearStatusLine();  // Clear any leftovers in the status line.
            tft.printStatusLine(0,myColors[1],myColors[0],"Initializing MCP9808 High Accuracy Temp sensor...");
            delay(StatuslineDisplayTime);
        #endif     
        

        status = mcp9808TempSen.setAddress(0x18, &Wire1);  // Returns false if address is out of range.
        if (status == false)
            {
                #if defined(DEBUG0)
                    Serial << "I2C address is out of range, status: " << status << endl;
                #endif
                
            }
        
        delay(1000);

        numberOfSeconds = 0;
        while ( ( (status = mcp9808TempSen.isConnected()) != 0) && (numberOfSeconds < 10))
            {
                #if defined (DEBUG0)
                    Serial.println(F("- MCP9808 sensor is not responding\n"));
                    Serial << "Checking for connection status = 0, Status: " << status << endl;
                    Serial.println(F("Waiting 1 second and trying again"));
                #endif

                #if defined(DEBUG)
                    Serial << "Checking for connection status = 0, Status: " << status << endl;
                #endif
                
                delay(1000);
                numberOfSeconds++;
            }
        if (status == 0)
            {
                #if defined(DEBUG)
                    Serial.println(F("- Setting Upper Temperature Limit to 50 Deg C..."));
                #endif
                mcp9808TempSen.setTupper( 50.0F );  // 50C (122F)
                delayMicroseconds(100);

                #if defined(DEBUG)
                    Serial.println(F("- Setting Lower Temperature Limit to -10Deg C..."));
                #endif
                mcp9808TempSen.setTlower( -10.0F );  // -10C (14F)
                delayMicroseconds(100);

                #if defined(DEBUG)
                    Serial.println(F("- Setting Critical Temperature Limit to 60 Deg C..."));
                #endif
                mcp9808TempSen.setTcritical( 60.0F );  // 60C (140F).
                delayMicroseconds(100);

                #if defined(DEBUG)
                    Serial.println(F("- Setting Temperature Resolution to .0625 Deg C..."));
                #endif
                mcp9808TempSen.setResolution(3);        // Set Resolution to .0625° C).
                delayMicroseconds(100);

                #if defined(DEBUG)
                    Serial.println(F("- Finished Initializing MCP9808 Temp Sensor\n"));
                    clearStatusLine();  // Clear any leftovers in the status line.
                    tft.printStatusLine(0,myColors[1],myColors[0],"- Finished Initializing MCP9808 Temp Sensor...");
                    delay(StatuslineDisplayTime);
                #endif
                
                noMCP9808 = false;  // This means the MCP9808 is present and functioning.
            }
        else
            {
                #if defined(DEBUG)
                    Serial.print(F("- Tried to establish contact with the MCP9808 10 Times over 10 Seconds - Setting noMCP9808 true!\n"));
                    clearStatusLine();  // Clear any leftovers in the status line.
                    tft.printStatusLine(0,myColors[1],myColors[0],"- Tried to establish contact with the MCP9808 10 Times over 10 Seconds - Setting noMCP9808 true!...");
                    delay(StatuslineDisplayTime);
                #endif     

                noMCP9808 = true;  // This means the MCP9808 is not present and non-functional.
            } 


/*******************************************************************************************************************
*                     Setup TMP17 very High Accuracy Temp Sensor.
* This sensor is located on the Wire1 bus, pins 16 and 17 of the Teensy4.1.
* These are the possible Commands:
*   uint8_t   isConnected() - Should return 0 if device detected, otherwise any other number means bad.          
*   init( void (*_newDataCallback) (void) )
*   uint16_t    update() - Returns the contents of the configuration register.
*   softReset() - Resets the sensor to factory settings.

*   setAlertMode(TMP117_PMODE mode)
*       Possible Modes:
*           THERMAL
*           ALERT
*           DATA
*   setCallBack( void (*_newDataCallback)(void) ) - Set the callback function address without setting any other options.
*   setAlertCallback( void (*allert_callback)(void), uint8_t pin ) - This sets the interrupt pin used for the CallBack.
*   setAlertTemperature( double lowtemp, double hightemp ) - Sets the alert temperatures inside the TMP117.
*   setConvMode( TMP117_CMODE cmode) - Sets the Conversion Mode.
*       Possible Conversion Modes:
*           CONTINUOUS
*           SHUTDOWN
*           ONESHOT
*   setConvTime( TMP117_CONVT convtime ) - This command sets the temperature conversion time.
*       Possile Conversion Times in Seconds:
*           C15mS5
*           C125mS
*           C250mS
*           C500mS
*           C1S
*           C4S
*           C8S
*           C16S
*   setAveraging( TMP117_AVE ave ) - Sets the temperature averaging Mode.
*       Possible Averaging Modes:
*           NOAVE
*           AVE8
*           AVE32
*           AVE64
*
*   setOffsetTemperature( double offset ) - Sets the Offset of the Temperture sensor for Calibration.
*   setTargetTemperature( double target ) - Sets the Target Temperature.

*   double    getTemperature() - Gets the actual latest measured Temperature from the sensor.
*   uint16_t  getDeviceID() - Gets the Device ID burned into the internal EEPROM.
*   uint16_t  getDeviceRev() - Gets the Device Revision number burned into the internal EEPROM.
*   double    getOffsetTemperature() - Gets the offset temperture set for the Sensor.
*   TMP117_ALERT getAlertType() - Gets the Alert Type generated by the sensor during the last reading.
*       Possible Alert Types:
*           NOALERT
*           HIGHALERT
*           LOWALERT
    
*   writeEEPROM( uint16_t data, uint8_t eeprom_nr ) - Writes into the internal EEPROM of the TMP117.
*   uint16_t  readEEPROM( uint8_t eeprom_nr ) - Reads from the internal EEPROM of the TMP117.
*   uint16_t  readConfig() - Reads the internal 16 bit configuration register.
*   printConfig() - Prints internal 16 bit configuration register in user readable format.
*******************************************************************************************************************/
    
    #if defined(DEBUG)
        Serial.println(F("\nInitializing TMP117 very High Accuracy Temp sensor..."));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Initializing TMP117 very High Accuracy Temp sensor...");
        delay(StatuslineDisplayTime); 
    #endif
    
 
    numberOfSeconds = 0;
    while ( ( (status = tmp117TempSen.isConnected()) != 0) && (numberOfSeconds < 10))
        {
            #if defined (DEBUG0)
                Serial.println(F("- TMP117 sensor is not responding\n"));
                Serial << "Checking for connection status = 0, Status: " << status << endl;
                Serial.println(F("Waiting 1 second and trying again"));
            #endif
            delay(1000);
            numberOfSeconds++;
        }

    if (status == 0)
        {
            #if defined(DEBUG)
                Serial.println(F("- Setting Convert Time to 500 milliseconds..."));
            #endif
            tmp117TempSen.setConvTime(TMP117_CONVT::C500mS);
            delayMicroseconds(10);

            #if defined(DEBUG)
                Serial.println(F("- Setting Number of Averages to 32..."));
            #endif
            tmp117TempSen.setAveraging(TMP117_AVE::AVE32);
            delayMicroseconds(10);

            #if defined(DEBUG)
                Serial.println(F("- Setting the Alert Mode to DATA..."));
            #endif
            tmp117TempSen.setAlertMode(TMP117_PMODE::DATA);
            delayMicroseconds(10);

            #if defined(DEBUG)
                Serial.println(F("- Setting the Offset Temperature to 0..."));
            #endif
            tmp117TempSen.setOffsetTemperature(0);
            delayMicroseconds(10);

            #if defined(DEBUG)
                Serial.println(F("- Setting Convert Mode to Continuous..."));
            #endif
            tmp117TempSen.setConvMode(TMP117_CMODE::CONTINUOUS);

            #if defined(DEBUG)
                Serial.println(F("\n- Setting the Callback Function..."));
            #endif
            tmp117TempSen.setCallBack(processTMP117Data);
            
            #if defined(DEBUG)
                Serial.println(F("- Finished Initializing TMP117 Temp Sensor\n"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[1],myColors[0],"- Finished Initializing TMP117 Temp Sensor...");
                delay(StatuslineDisplayTime);
            #endif
            
            noTMP117 = false;  // This means the TMP117 is present and working.
        }
    else
        {
            #if defined(DEBUG)
                Serial.print(F("- Tried to establish contact with the TMP117 10 Times over 10 Seconds - Setting noTMP117 true!\n"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[1],myColors[0],"- Tried to establish contact with the TMP117 10 Times over 10 Seconds - Setting noTMP117 true!...");
                delay(StatuslineDisplayTime);
            #endif

            noTMP117 = true;  // This means the TMP117 is missing and non-functional.
        } 

/*******************************************************************************************************************
*                                Setup the GNSS (GPS) to work on Serial port 7.
*******************************************************************************************************************/
    //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial
    //Assume that the U-Blox GNSS is running at 9600 baud (the default) or at 230400 baud.
    // Initialize the Serial port
    // bool SFE_UBLOX_GNSS::begin(Stream &serialPort, uint16_t maxWait, bool assumeSuccess)
    //Loop until we're in sync and then ensure it's at 230400 baud.

    #if defined(DEBUG)
        Serial.println(F("Setting up the GNSS (GPS) to work on Serial port 1..."));
        Serial.println(F("Trying to find GPS Module for 1 minute, otherwise No GPS is Available..."));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setting up the GNSS (GPS) to work on Serial port 1...");
        delay(StatuslineDisplayTime);
    #endif
    
    debug_start = millis();

    do
        {
            #if defined(DEBUG)
                Serial.println(F("GNSS: trying 230400 baud\n"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[1],myColors[0],"Starting GNSS: trying 230400 baud...");
                delay(StatuslineDisplayTime);
            #endif
            

            Serial7.begin(230400);
            
            delay(10);
            if (myGNSS.begin(Serial7) == true)
                {
                    #if defined(DEBUG)
                        Serial.println(F("230400 baud is working\n"));
                        clearStatusLine();  // Clear any leftovers in the status line.
                        tft.printStatusLine(0,myColors[1],myColors[0],"230400 baud is working...");
                        delay(StatuslineDisplayTime);
                    #endif
                    
                    coldStart = true;
                    break;
                }

            #if defined(DEBUG)
                Serial.println(F("230400 baud failed"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[2],myColors[11],"230400 baud failed...");
                delay(StatuslineDisplayTime);
            #endif
            
            Serial7.end();
            delay(10);

            #if defined(DEBUG)
                Serial.println(F("GNSS: trying 9600 baud"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[1],myColors[0],"Starting GNSS: trying 9600 baud...");
                delay(StatuslineDisplayTime);
            #endif
            
            Serial7.begin(9600);
            delay(10);
            if (myGNSS.begin(Serial7) == true)
                {
                    #if defined(DEBUG)
                        Serial.println(F("GNSS:Cconnected at 9600 baud, switching to 230400\n"));
                        clearStatusLine();  // Clear any leftovers in the status line.
                        tft.printStatusLine(0,myColors[1],myColors[0],"Starting GNSS: Connected at 9600 baud, switching to 230400...");
                        delay(StatuslineDisplayTime);
                    #endif
                    myGNSS.setSerialRate(230400);  // This only sets the baud rate for the GPS module.
                    Serial7.end();
                    delay(10);
                    Serial7.begin(230400);  // Now set the Serial7 port to the same speed.
                    delay(10);
                    coldStart = true;
                    break;  // Break out of the do loop.
                }
            else
                {
                    #if defined(DEBUG)
                        Serial.println(F("GNSS: 9600 baud didn't work lets do a factory reset and wait 5 seconds and try again.\n"));
                        clearStatusLine();  // Clear any leftovers in the status line.
                        tft.printStatusLine(0,myColors[2],myColors[11],"GNSS: 9600 baud didn't work lets do a factory reset and wait 5 seconds and try again...");
                        delay(StatuslineDisplayTime);
                    #endif
                    myGNSS.factoryReset();
                    Serial7.end();
                    delay(500); //Wait a bit before trying again.
                }
        } while( ( (millis() - debug_start) <= 60000UL) );  // Try for 1 Minute, if fail then jump out.

    if ( ( (millis() - debug_start) <= 60000UL) )
        {
            #if defined(DEBUG)
                Serial.println(F("GNSS serial connected...\n"));
                clearStatusLine();  // Clear any leftovers in the status line.
                tft.printStatusLine(0,myColors[1],myColors[0],"GNSS serial connected...");
                delay(StatuslineDisplayTime);   
            #endif

            GPS_GNSS_Present = true;

            if(!myGNSS.setUART1Output(COM_TYPE_UBX))                    //Set the UART port to output UBX only
                {
                    #if defined(DEBUG)
                        Serial.println(F("Set UART Output failed...\n"));   
                    #endif 
                }
            //myGNSS.saveConfiguration();                           //Save the current settings to flash and BBR
            //myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);    //Save (only) the communications port settings to flash and BBR
            if(!myGNSS.setNavigationFrequency(2))                       //Produce two solutions per second
                {
                    #if defined(DEBUG)
                        Serial.println(F("Set Navigation Frequency failed...\n"));   
                    #endif
                }

            if(!myGNSS.setAutoPVTcallbackPtr(&printPVTdata))            // Enable automatic NAV PVT messages with callback to printPVTdata.
                {
                    #if defined(DEBUG)
                        Serial.println(F("Set Auto PVT callback Ptr failed...\n"));   
                    #endif
                }    
        }
    else
        {
            #if defined(DEBUG)
                Serial.println(F("No GNSS serial connected...\n"));   
            #endif
            GPS_GNSS_Present = false;  // No GPS is present
        }


    /*******************************************************************************************************************
    *                     Setup the Timer to generate Interrupts at 1 second interval.
    *******************************************************************************************************************/
    //t1.begin(oneSecCallback, 1'000'000); // 1s


    /****************************************************************************************************
    * IntervalTimer is generally recommended for use only in libraries and advanced applications
    * which require highly precise timing.  Up to 4 IntervalTimer objects may be active simultaneuously.
    * The interval is specified in microseconds, which may be an integer or floating point number,
    * for more highly precise timing.
    * elif (F_CPU == 96,000,000) for Teensy 3.2
    * #define F_BUS 48,000,000
    * (Largest number for unsigned int = 4,294,967,295 decimal)
    * MAX_PERIOD = UINT32_MAX / (F_BUS / 1000000.0)
    * The function requires two input variables:
    * 1. The name of the function to be executed.
    * 2. The interval in microseconds.  The largest number allowed is: 89,478,485 or 89 seconds
    *
    * This function returns true if sucessful.
    * False is returned if all hardware resources are busy, or used by other IntervalTimer objects.
    *
    *****************************************************************************************************/
    
    status = oneSecondTimer.begin(oneSecCallback, 1000000);  // One Second ISR to run every second.
    if (status == false)
        {
            #if defined(DEBUG)
                Serial.println(F("oneSecondTimer initialization failed"));
            #endif
        }

    status = oneHundredMillisecTimer.begin(oneHundredMSCallback, 100000);  // One Hundred Millisecond ISR to run every 100 milliseconds.
    if (status == false)
        {
            #if defined(DEBUG_STARTUP_CODE)
                Serial.println(F("oneHundredMillisecTimer initialization failed"));
            #endif
        }
 
    status = fiveMillisecTimer.begin(fiveMSCallback, 5000);  // Five MS Timer ISR to run every 5 milliseconds.
    if (status == false)
        {
            #if defined(DEBUG_STARTUP_CODE)
                Serial.println(F("oneHundredMillisecTimer initialization failed"));
            #endif
        }

    #if defined(DEBUG)
        Serial.println(F("Setup finished interval timers setup...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setup finished interval timers setup...");
        delay(StatuslineDisplayTime);
    #endif
    

    #if defined(DEBUG)
        Serial.println(F("Setting up the Software Version on the TFT Screen..\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setting up the Software Version on the TFT Screen...");
        delay(StatuslineDisplayTime);
    #endif
    //tft.setFontScale(0);  // This is the small font on the screen.
    tft.setTextColor(CYAN, BLACK);  // Set the text color to orange with black backround.
    // This puts the Software Version just below the MASTER title at the top of the tft.
    tft.setCursor( ( clockPos[0] - ( Xsize * 12 ) ), ( YLINE3 ), false);  // Size is set for 8 x 16 pixels.
    tft.print("SWversion: " + String(VERSION));  // Print Project version.
    #if defined(DEBUG)
        Serial.println(F("Setup finished putting the software version on the TFT...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setup finished putting the software version on the TFT...");
        delay(StatuslineDisplayTime);
    #endif

    #if defined(DEBUG)
        Serial.println(F("Setting up the UTC Status Dot on the TFT...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setting up the UTC Status Dot on the TFT...");
        delay(StatuslineDisplayTime);
    #endif
    statusDotColor = DOT_RED;  // Set the color indicator of the status dot to red.
    printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), RED);  // Print RED dot to indicate beginning status of the corrected UTC Time from the GPS.
    #if defined(DEBUG)
        Serial.println(F("Setup finished placing the UTC Status Dot on the TFT...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setup finished placing the UTC Status Dot on the TFT...");
        delay(StatuslineDisplayTime);
    #endif

    #if defined(DEBUG)
        Serial.println(F("Setting up the Watch Dog Timer...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setting up the Watch Dog Timer...");
        delay(StatuslineDisplayTime);
    #endif

    config.trigger = 12; /* in seconds, 0->128, trigger is how long before the watchdog callback fires */
    config.timeout = 15; /* in seconds, 0->128, timeout is how long before not feeding will the watchdog reset */
    config.callback = WDTCallback;

    wdt.begin(config);

    #if defined(DEBUG)
        Serial.println(F("Setup finished the Watch Dog Timer setup...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setup finished the Watch Dog Timer setup...");
        delay(StatuslineDisplayTime);
    #endif

    wdt.feed();  // Kick the dog every 10 seconds, its set to reset the teensy in 15 seconds.

    MTP.begin();

    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
        Serial.println(F("Initializing LittleFS in the Onboard QSPI RAM..."));
        Serial.println(F("Creating the character string for the File Name..."));
    #endif

    NumChars = sprintf( FileNameChars,"EdsEnvSysData-%d-%d-%d.csv", currentDate[1], currentDate[0], currentDate[2] );

    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
        Serial.print(F("Size of the assembled character string for the File Name: "));
        Serial.println(NumChars);
        Serial.print(F("File Name assembled for today: "));
        Serial.println(FileNameChars);
        Serial.println("");           
    #endif

    // See if you are able to create a RAM disk in the space alotted.
    // qspiRamDiskBuffer = is the name of the array created, sizeof(qspiRamDiskBuffer) is how large the 
    // array is, in our case 1MB.
    // Returns true if File System is created, formatted with quick format, and mounted.
    if (!qspiRamFileSystem.begin(qspiRamDiskBuffer, qspiRamFileSystem_SIZE) )
        {
            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                Serial.printf("Error initialization %s\n", "qspiRamFileSystem");
                Serial.println(F("Cannot use the LittleFS File System, it failed to start up properly..."));           
            #endif

            noLittleFS = true;            
        }
    else
        {
            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                    Serial.println(F("LittleFS has been created, formatted with quick format, and mounted..."));           
            #endif

            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                Serial.printf("qspiRamFileSystem Name is: %s\n", qspiRamFileSystem.getMediaName() );
            #endif
            
            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                Serial.println(F("LittleFS Has been formatted and Initialized, this is the size of the LittleFS file system created:"));
                Serial.printf("    Bytes Used in QSPI RAM: %llu, Bytes Total: %llu\n\n", qspiRamFileSystem.usedSize(), qspiRamFileSystem.totalSize());           
            #endif

            if( qspiRamFileSystem.usedSize() == qspiRamFileSystem.totalSize() )
                {
                    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
		                Serial.printf("\n\n\tWARNING: DISK FULL >>>>>  Bytes Used: %llu, Bytes Total:%llu\n\n", qspiRamFileSystem.usedSize(), qspiRamFileSystem.totalSize());           
                    #endif                  

		            diskFull = true;
	            }
            else
                {
                    noLittleFS = false;

                    // Creates a file if not present, FILE_WRITE_BEGIN will create the file.
                    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                        Serial.println(F("Creating the File..."));           
                    #endif

                    qspiRamDataFile = qspiRamFileSystem.open(FileNameChars, FILE_WRITE_BEGIN);

                    fileLength = qspiRamDataFile.size();

                    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                        Serial.printf("%s started with %llu bytes\n", FileNameChars, fileLength );
                        Serial.println(F("Now checking if the Data File qspiRamDataFile exists...\n"));
                    #endif

                    if (qspiRamDataFile)
                        {
                            qspiRamDataFilePresent = true;
                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.printf("File %s exists and is ready for writing...\n", FileNameChars );           
                            #endif

                            write_data = true;

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.println(F("Now creating the CSV header line at the top of the File..."));           
                            #endif

                    
                            NumChars = sprintf(dataFileLine,"  Time  ,BME680TempC,BME680TempC,BME680BarPres,BME680RHumdity,BME680GasSensor,BME680Altitude,MCP9808TempC,MCP9808TempF,TMP117TempC,TMP117TempF\r");

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.print(F("Size of the assembled character array for the 1st line of the File: "));
                                Serial.println(NumChars);
                                Serial.println(F("This is the Line of characters to put into the File:"));
                                Serial.println(dataFileLine);
                            #endif

                            qspiRamDataFile.write(dataFileLine);  // Now write data to file.

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.println(F("Printed the header line to the File..."));
                            #endif
                                            
                            record_count++;

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.print("The Data File record count is now: ");
                                Serial.print(record_count);
                                Serial.println(" Records");
                            #endif

                            qspiRamDataFile.flush();  // Write data to file.

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.printf("The file is now this size (In bytes) Total: %llu\n", qspiRamDataFile.size());
                                Serial.println("\nLogging Data!!!\n");
                            #endif

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.println(F("Adding the RAMDISK file system to the MTP File System..."));
                            #endif

                            index_sdio_storage = MTP.addFilesystem(qspiRamFileSystem, RamDiskName);  // This line adds the RAMDISK to MTP and gives it a name.

                            if (index_sdio_storage >= 1)
                                {
                                    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                        Serial.printf("The MTP File System Count is %u...\n\n", index_sdio_storage );
                                    #endif
                                }
                            else
                                {
                                    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                        Serial.println(F("The MTP File System is less than 1, No MTP File System was created...\n\n"));
                                    #endif
                                }       
                        }
                    else
                        {
                            qspiRamDataFilePresent = false;
                            write_data = false;

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.println(F("Error, The Data File qspiRamDataFile does not exist...\n"));
                            #endif

                            #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                Serial.println(F("Adding the RAMDISK file system to the MTP File System..."));
                            #endif
                            index_sdio_storage = MTP.addFilesystem(qspiRamFileSystem, RamDiskName);  // This line adds the RAMDISK to MTP and gives it a name.

                            if (index_sdio_storage >= 1)
                                {
                                    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                        Serial.printf("The MTP File System Count is %u...\n\n", index_sdio_storage );
                                    #endif
                                }
                            else
                                {
                                    #if defined(DEBUG_STARTUP_LITTLEFS_CODE)
                                        Serial.println(F("The MTP File System is less than 1, No MTP File System was created.../n"));
                                    #endif
                                }        
                        }
                }
        }

    #if defined(DEBUG)
        Serial.println(F("Setup finished the LittleFS Setup...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setup finished the LittleFS Setup...");
        delay(StatuslineDisplayTime);
    #endif

    /*******************************************************************************************************************************
    *           This is important information concerning the SD and SdFat file systems operation.
    * 
    * You can access the main SdFat filesystem with "SD.sdfs".  You may wish to use SD.sdfs.begin() to cause SdFat to access
    * the SD card using faster drivers than the default.  You may also wish to open files as FsFile to gain access to SdFat's
    * special file functions which are not available using the simpler SD File.
    *
    * Access the built in SD card on Teensy 3.5, 3.6, 4.1 using a FIFO:  ok = SD.sdfs.begin(SdioConfig(FIFO_SDIO))
    * Access the built in SD card on Teensy 3.5, 3.6, 4.1 using DMA (maybe faster):  ok = SD.sdfs.begin(SdioConfig(DMA_SDIO))
    * 
    * After the SD card is initialized, you can access it using the ordinary SD library functions, regardless of whether it was
    * initialized by SD library SD.begin() or SdFat library SD.sdfs.begin().
    * 
    * You can also access the SD card with SdFat's functions:  SD.sdfs.ls() [List files and directories]  root.ls(LS_R | LS_DATE | LS_SIZE)
    * 
    * You can access files using SdFat which uses "FsFile" for open files:  FsFile myfile = SD.sdfs.open("datalog.bin", O_WRITE | O_CREAT)
    * FsFile offers more capability than regular SD "File".
    * As shown in this example, you can truncate tiles:  myfile.truncate()  You can also pre-allocate a file on the SD card
    * (if it does not yet have any data, the reason we truncate first):  myfile.preAllocate(40*1024*1024))
    * Pre-allocation impoves the speed of writes within the already allocated space while data logging or performing other small writes.
    * 
    * You can also use regular SD functions, even to access the same file.  Just remember to close the SdFat FsFile before
    * opening as a regular SD File.
    * 
    * When mixing SD and SdFat file access, remember for writing that SD defaults to appending if you open with FILE_WRITE.
    * You must use FILE_WRITE_BEGIN if you wish to overwrite the file from the start.
    * 
    * With SdFat, O_WRITE or O_RDWR starts overwriting from the beginning.  You must add O_AT_END if you wish to append.
    *******************************************************************************************************************************/

    // Always add an SD file system
    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
        Serial.println(F("Initalize the SD Card before using it..."));
    #endif

    sdio_previously_present = sdSDIO.begin(BUILTIN_SDCARD);

    #if defined(FORMAT_SD_CARD_BEFORE_USING)
        Serial.println(F("Formatting the SD Card Before use..."));
        sdSDIO.format(1);  // Format the card before using it.
        Serial.println(F(""));   
    #endif

    #if defined(SDCARD_PRINT_DIRECTORY)

        /*******************************************************************************************
        * List the directory contents of a directory.
        *
        * \param[in] pr Print stream for list.
        *
         * \param[in] path directory to list.
        *
        * \param[in] flags The inclusive OR of
        *
        * LS_DATE - %Print file modification date
        *
        * LS_SIZE - %Print file size.
        *
        * LS_R - Recursive list of subdirectories.
        *
        * \return true for success or false for failure.
         *******************************************************************************************/

        Serial.println(F("Listing of Files and Directories on the SD Card..."));
        sdSDIO.sdfs.ls("/", LS_R);  // List the Files and directories on the SD Card
    #endif

    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
        Serial.println(F("\nCreating the SD Card Destination Directory Name..."));
    #endif
    sprintf(sdCardDestDirectory, "/%s", SDDataDirectory );  // Insert the SD Card Dest directory name.

    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
        Serial.printf( "This is the SD Card Destination Directory to be created: %s\n", sdCardDestDirectory );
        Serial.println(F("If the SD Card Destination Directory is missing create it..."));
    #endif

    if( !sdSDIO.exists(sdCardDestDirectory) )  // Check for the existance of the SDCard Destination Directory.
        {
            if(!sdSDIO.mkdir( sdCardDestDirectory ))  // Create the Destination Directory.
                {
                    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                        Serial.println(F("Creating the Destination Directory Failed..."));
                    #endif
                     sdCardDestDirectoryPresent = false; 
                }
            else
                {
                    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                        Serial.println(F("The SD Card Destination Directory was created..."));
                    #endif

                    sdCardDestDirectoryPresent = true;
                }
        }
    else
        {
            #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                Serial.println(F("The SD Card Destination Directory exists..."));
            #endif

            sdCardDestDirectoryPresent = true;
        }
    
    if(sdCardDestDirectoryPresent == true)
        {
            #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                Serial.println(F("Changing to and Opening the SD Card Destination Directory on the SD Card..."));
            #endif

            // Need to change to the Destination Directory before we can create a file inside it.
            if (!sdSDIO.sdfs.chdir(sdCardDestDirectory))
                {
                    // The SD Card Destination Directory did not open.
                    #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                        Serial.printf("Error - Changing to the Sub Directory %s Failed...\n", sdCardDestDirectory );
                    #endif
                    sdCardDestDirectoryOpen = false;
                }
            
            else
                {
                    // Changing to the SD Card Destination Directory.
                    #if defined(DEBUG_COPY_RAMDISK_TO_SDCARD_SUB)
                        Serial.printf("Changing to the Sub Directory %s was successful...\n", sdCardDestDirectory );
                    #endif

                    sdCardDestDirectoryOpen = true;
                }

            if (sdCardDestDirectoryOpen == true)
                {
                    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                        Serial.println(F("Now checking to see if the Destination File is present..."));
                    #endif

                    if ( sdSDIO.exists(FileNameChars) )
                        {
                            #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                                Serial.println(F("Destination File exists, removing it to make room for a new one..."));
                            #endif

                            if ( !sdSDIO.remove(FileNameChars) )
                                {
                                    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                                        Serial.println(F("Removing the destination File failed..."));
                                    #endif
                                }
                            else
                                {
                                    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                                        Serial.println(F("Destination File was removed..."));
                                    #endif
                                }
                            
                        }
                    else
                        {
                            #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                                Serial.println(F("Destination File does not exist..."));
                            #endif
                        }   
                }
            
            
            #if defined(CREATE_SDCARD_FILE_IN_SUBDIRECTORY)
                #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                    Serial.println(F("Create the SD Card Destination File on the SD Card..."));
                #endif

                sprintf(sdCardDestFileName, "%s", FileNameChars );  // The file will be created in the subdirectory.
                #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                    Serial.printf( "This is the File Name path created: %s\n", sdCardDestFileName );
                #endif

                #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                    Serial.println(F("If the SD Card Destination File is missing create it..."));
                #endif

                if ( !sdSDIO.exists(sdCardDestFileName) )
                    {
                        SDCardDataFile = sdSDIO.open( sdCardDestFileName, FILE_WRITE_BEGIN );
                        if ( !SDCardDataFile )
                            {
                                #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                                    Serial.println(F("Creating and opening the SD Card Destination File Failed...\n"));
                                #endif
                                bool sdCardDestFileNamePresent = false;

                            }
                        else
                            {
                                #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                                    Serial.println(F("Created and Opened the SD Card Destination File...\n"));
                                #endif
                                bool sdCardDestFileNamePresent = true;

                            } 
                    }
                else
                    {
                        #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                            Serial.println(F("The SD Card Destination File exists..."));
                        #endif
                    }
            #endif
        }

    #if defined(DEBUG_SD_CARD_STARTUP_CODE)
        Serial.println(F("Adding the SD Card file system to the MTP File System..."));
    #endif
    index_sdio_storage = MTP.addFilesystem(sdSDIO, sdCardDiskName);  // This line adds the SD Card to MTP and gives it a name.
    if (index_sdio_storage >= 1)
        {
            #if defined(DEBUG_SD_CARD_STARTUP_CODE)
              Serial.printf("The MTP File System Count is %u...\n\n", index_sdio_storage );
            #endif
        }
    else
        {
            #if defined(DEBUG_SD_CARD_STARTUP_CODE)
                Serial.println(F("The MTP File System is less than 1, No MTP File System was created.../n"));
            #endif
        }

    #if defined(DEBUG)
        Serial.println(F("\nSetup finished the SD Card Setup...\n"));
        clearStatusLine();  // Clear any leftovers in the status line.
        tft.printStatusLine(0,myColors[1],myColors[0],"Setup finished the SD Card Setup...");
        delay(StatuslineDisplayTime);
    #endif

    clearStatusLine();  // Clear any leftovers in the status line.
    tft.printStatusLine(0,myColors[1],myColors[0],"End of Setup...");
    delay(StatuslineDisplayTime);
    clearStatusLine();  // Clear any leftovers in the status line.
    
}  // End of Setup


/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/*  @brief      Arduino method for the main program loop
    @details    This is the main program for the Arduino IDE, it is an infinite loop and keeps on repeating.
    @return   void
*/
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
/*ΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩΩ*/
void loop()
    {
        if (oneSecFlag == true)
            {
                
                // These events occur every second.
                /*******************************************************************************************************************
                * Get current time from DS3231 and put into variables curentTime[0] (hh), [1] (mm), & [2] (ss).
                *******************************************************************************************************************/
                if (DS3231Clock.read(tm))
                    {
                        lastTimeSetByClock = makeTime(tm);  // Converts tmElements array into a Time_T (UNIX Time), LastTimeSetByClock is a Time_T (UNIX Time) variable.

                        currentTime[0]      = (tm.Hour);
                        currentTime[1]      = (tm.Minute);
                        currentTime[2]      = (tm.Second);
                        currentDate[0]      = (tm.Day);
                        currentDate[1]      = (tm.Month);
                        currentDate[2]      = (tmYearToCalendar(tm.Year));
                        currentDayOfWeek    = (tm.Wday);  // Put the Day of the Week into the currentDayOfWeek Variable.

                        breakTime( lastTimeSetByClock, DateTimeFlds );  // This converts the UNIX Time to DateTimeFields format.
                        DateTimeFlds.wday = (tm.Wday - 1);  // DateTimeFields uses 0 -> 6 for Week days.
                    }
                else
                    {
                        #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                            clearStatusLine();  // Clear any leftovers in the status line.
                            tft.printStatusLine(0,myColors[2],myColors[11],"Error - No DS3231 Present, no time to display...");  // Print this in Red.
                            delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds.
                        #endif

                        if (DS3231Clock.chipPresent())
                            {
                                Serial.println(F("The DS3231 is stopped.  Please run the SetTime"));
                                Serial.println(F("example to initialize the time and begin running.\n"));
                            }
                        else
                            {
                                Serial.println(F("DS3231 read error!  Please check the circuitry.\n"));
                            }
                    }

                            
                if ((currentTime[0] >= timeToDimTheScreen) || (currentTime[0] < TimeToWakeTheScreen))
                    {  // Night Time Brightness is lower, with red text and a red clock face.
                        //tft.brightness(nightTimeScreenBrightness);  // Set the TFT brightness to the night time setting.
                        tft.textColor(RED, BLACK);
                        tft.setTextColor(RED);  // Set text color, text background color is black.
                        tft.setBackGroundColor(BLACK);
                        nightTimeFlag = true;  // It's night time.
                        if (currentTime[0] == timeToDimTheScreen)
                            {
                                colorChangeOccured = true;
                            }
                        drawGauge(clockPos, 0, 360, RED, RED, RED);  // This draws the the round clock face with the major & minor tick marks.
                        switch (statusDotColor)
                            {
                                case DOT_RED:
                                    printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), RED);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                                        
                                case DOT_YELLOW:
                                    printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), YELLOW);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                                        
                                case DOT_GREEN:
                                    printUTCStatusDotOnTFT( ( XMIDDLE  ), ( YTOP + ( Ysize * 9 ) ), GREEN);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                                        
                                default:
                                    printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), BLACK);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                            }
                    }
                else
                    {  // Day Time brightness is full power with white text and a white clock face
                        //tft.brightness(dayTimeScreenBrightness);
                        tft.textColor(WHITE, BLACK);
                        tft.setTextColor(WHITE);  // Set text color, text background color is black.
                        tft.setBackGroundColor(BLACK);
                        nightTimeFlag = false;  // It's day time.
                        if (currentTime[0] == TimeToWakeTheScreen )
                            {
                                colorChangeOccured = true;
                            }
                        drawGauge(clockPos, 0, 360,WHITE, WHITE, WHITE);  // This draws the the round clock face with the major & minor tick marks.
                        switch (statusDotColor)
                            {
                                case DOT_RED:
                                    printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), RED);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                                        
                                case DOT_YELLOW:
                                    printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), YELLOW);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                                        
                                case DOT_GREEN:
                                     printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), GREEN);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                                        
                                default:
                                    printUTCStatusDotOnTFT( ( XMIDDLE ), ( YTOP + ( Ysize * 9 ) ), BLACK);  // Print the status of the corrected UTC Time from the GPS.
                                    break;
                            }
                    }
                    if (currentTime[2] == 0)  // Seconds = zero.
                        {
                            if (currentTime[1] == 0)  // Minutes = zero.
                                {
                                    if (currentTime[0] == 0)  // Hours = zero, It is Midnight 00:00:00.
                                        {
                                            itsMidnight = true;  // The clock has just struck midnight.

                                            #if defined(DEBUG_LOOP_MIDNIGHT_TIME)
                                                Serial.println(F("\nMain Loop - Now stopping the logging of data and moving the Environmental data file to SD Card..."));      
                                            #endif

                                            stopLoggingDataToRamDisk();  // This stops the logging and closes the existing data file.

                                            if(!copyDataFileFromRAMDiskToSDCard(FileNameChars, SDDataDirectory, FileNameChars))  // This moves the data file to the SD Card, the source file name and destination file name are the same.
                                                {
                                                    #if defined(DEBUG_LOOP_MIDNIGHT_TIME )
                                                        Serial.println(F("Error - Copying data file to SD Card from RAMDISK...\n"));
                                                    #endif
                                                }
                                            else
                                                {
                                                    #if defined(DEBUG_LOOP_MIDNIGHT_TIME )
                                                        Serial.println(F("Copying data file to SD Card from RAMDISK was successful...\n"));
                                                    #endif
                                                }
                        

                                            #if defined(DEBUG_LOOP_MIDNIGHT_TIME )
                                                Serial.println(F("Now creating a new Environmental data file in QSPI-RAM...\n"));                                            
                                            #endif

                                            if(!createNewDataFileAndStartLogging())  // This creates the new data file and begins logging again.
                                                {
                                                    #if defined(DEBUG_LOOP_MIDNIGHT_TIME )
                                                        Serial.println(F("Error - Creating new data file to log data...\n"));
                                                    #endif
                                                }
                                            else
                                                {
                                                    MTP.send_DeviceResetEvent();  // Make sure the MTP system updates the new file name and file copied to the SD Card.
                                                    #if defined(DEBUG_LOOP_MIDNIGHT_TIME )
                                                        Serial.println(F("Creating new data file was successful...\n"));                                            
                                                    #endif
                                                }

                                            //insideDailyHighTemp = sensorState.Temperature;  // Set inside Daily High Temp to ambient at midnight.
                                            //insideDailyLowTemp = sensorState.Temperature;  // Set inside Daily Low Temp to ambient at midnight.
                        
                                            //outsideDailyHighTemp = WeatherDataStruct.Temperature;  // Set outside Daily High Temp to ambient at midnight.
                                            //outsideDailyLowTemp = WeatherDataStruct.Temperature;  // Set outside Daily Low Temp to ambient at midnight.
                                        }
                                }
                        }
                    #if defined(USINGGPS)
                        if ( (currentTime[0] == 2) && (GPS_GNSS_Present == true) )  // Hours = two, It is 2AM (02:00).
                            {
                                updateGPSFlag = true;  // Setting this flag means that the next 2 second interrupt comes around, it will update the RTC from the GPS.
                            }
                    #endif

                drawClockHands(clockPos, currentTime, clockColors);
                drawPrintTime(( clockPos[0] - (fontXsize * 7)) , YLINE13, currentTime[0], currentTime[1], currentTime[2], currentDayOfWeek, currentDate[0], currentDate[1], currentDate[2], false);

                if (noTMP117 == false)
                    {
                        SensorStruct.tmp117DataValid = false;
                        #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                            clearStatusLine();  // Clear any leftovers in the status line.
                            tft.printStatusLine(0,myColors[1],myColors[0],"Processing the data from TMP117...");
                        #endif
                        tmp117TempSen.update();  // This class call checks for any update on the temperature from the TMP117 Temp Sensor, if it finds one it executes the callback routine.
                    }
                
                if (noBME680 == false)
                    {
                        SensorStruct.BME680DataValid = false;
                        #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                            clearStatusLine();  // Clear any leftovers in the status line.
                            tft.printStatusLine(0,myColors[1],myColors[0],"Processing the data from BME680...");
                        #endif
                        getBME680SensorData();  // Sets valid flag if data is valid.
                    }

                if (noMCP9808 == false)
                    {
                        SensorStruct.mcp9808DataValid = false;
                        #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                            clearStatusLine();  // Clear any leftovers in the status line.
                            tft.printStatusLine(0,myColors[1],myColors[0],"Processing the data from MCP9808...");
                        #endif
                        processMCP9808Data();  // Puts the info out to the USB Serial port if debug 52 is set otherwise it puts the data into the Struct.
                    }
                #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                    clearStatusLine();  // Clear any leftovers in the status line.
                #endif
                // End of one second updates.

                // Begin Two second updates.
                twoSecCount++;
                if (twoSecCount >= 2)
                    {
                        #if defined(USINGGPS)
                            #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                clearStatusLine();  // Clear any leftovers in the status line.
                                tft.printStatusLine(0,myColors[1],myColors[0],"Updating the position from GPS...");
                                delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds
                            #endif

                            if ( (gpsUBXDataStruct.valid == true) && (GPS_GNSS_Present == true))
                                {
                                    // Put "GPS Present" on the TFT.
                                    tftPrintGPSPresentHeader( ( XRIGHT - ( Xsize * 26) ), ( YMIDDLE - (Ysize * 2) ), true );

                                    updatePositionFromGPS();
                                    #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                        switch (gpsUBXDataStruct.fixType)
                                            {
                                                case NO_FIX:
                                                    Serial.println(F("NO FIX"));  // Print the Heading.
                                                    break;
                                                case DEAD_RECKONING_ONLY:
                                                    Serial.println(F("DR ONLY"));  // Print the Heading.
                                                    break;
                                                case TWO_D_FIX:
                                                    Serial.println(F("TWO_D_FIX"));  // Print the Heading.
                                                    break;
                                                case THREE_D_FIX:
                                                    Serial.println(F("THREE_D_FIX"));  // Print the Heading.
                                                    break;
                                                case GNSS_DR_COMBINED:
                                                    Serial.println(F("GNSS_DR_COMBINED"));  // Print the Heading.
                                                    break;
                                                case TIME_FIX_ONLY:
                                                    Serial.println(F("TIME_FIX_ONLY"));  // Print the Heading.
                                                    break;
                                                        
                                                default:
                                                    Serial.println(F("BAD PACKET"));  // Print the Heading.
                                                    break;
                                            }
                                    #endif

                                    if (preferencesInProgressFlag == false)
                                        {
                                            #if defined(USINGGPS)
                                                if (updateGPSFlag == true)  //This means that it is 2AM in the morning, which is the time to update the internal Clock.
                                                    {
                                                        #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                                            clearStatusLine();  // Clear any leftovers in the status line.
                                                            tft.printStatusLine(0,myColors[1],myColors[0],"Updating the DS3231 Clock Time from GPS...");
                                                            delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds
                                                        #endif

                                                        if (updateTimeFromGPS())  // This gets the GMT Time from the GPS attached to Serial port 1.
                                                            {
                                                                #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                                                    clearStatusLine();  // Clear any leftovers in the status line.
                                                                    tft.printStatusLine(0,myColors[1],myColors[0],"Updating the time from GPS Failed...");
                                                                    delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds.
                                                                #endif

                                                                #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                                                    Serial.println(F("Error: Did not update DS3231 Clock with GPS time!"));
                                                                    Serial.print(F("statusDotColor is: "));
                                                                    Serial.println(statusDotColor);
                                                                    Serial.print(F("UTCTimeCorrected flag: "));
                                                                    Serial.println(UTCTimeCorrected);
                                                                #endif
                                                            }
                                                        else
                                                            {
                                                                #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                                                    Serial.println(F("Updated DS3231 Clock with GPS time!"));
                                                                    Serial.print(F("statusDotColor is: "));
                                                                    Serial.println(statusDotColor);
                                                                    Serial.print(F("UTCTimeCorrected flag: "));
                                                                    Serial.println(UTCTimeCorrected);
                                                                #endif
                                                            }
                                
                                                        #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                                            switch (gpsUBXDataStruct.fixType)
                                                                {
                                                                    case NO_FIX:
                                                                        Serial.println(F("NO FIX"));  // Print the Heading.
                                                                        break;
                                                                    case DEAD_RECKONING_ONLY:
                                                                        Serial.println(F("DR ONLY"));  // Print the Heading.
                                                                        break;
                                                                    case TWO_D_FIX:
                                                                        Serial.println(F("TWO_D_FIX"));  // Print the Heading.
                                                                        break;
                                                                    case THREE_D_FIX:
                                                                        Serial.println(F("THREE_D_FIX"));  // Print the Heading.
                                                                        break;
                                                                    case GNSS_DR_COMBINED:
                                                                        Serial.println(F("GNSS_DR_COMBINED"));  // Print the Heading.
                                                                        break;
                                                                    case TIME_FIX_ONLY:
                                                                        Serial.println(F("TIME_FIX_ONLY"));  // Print the Heading.
                                                                        break;
                                                        
                                                                    default:
                                                                        Serial.println(F("BAD PACKET"));  // Print the Heading.
                                                                        break;
                                                                }
                                                        #endif
                                                        updateGPSFlag = false;
                                                    }
                                    
                                            #endif
                                        }
                                }
                            else
                                {
                                    // No valid data from GPS, or No GPS is present.

                                    if (GPS_GNSS_Present == true)
                                        {
                                            #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                                Serial.println(F("No Valid Data from the GPS Module..."));
                                            #endif
                                            // Put "GPS Present" and "No Valid Data" on the TFT.
                                            tftPrintGPSPresentHeader( ( XRIGHT - ( Xsize * 26) ), ( YMIDDLE - (Ysize * 2) ), true );
                                            tftPrintGPSNoValidData( ( XRIGHT - ( Xsize * 31 ) ), ( YMIDDLE ) );
                                        }
                                    else
                                        {
                                            #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                                Serial.println(F("No GPS Module is present..."));
                                            #endif
                                            // Print "No GPS" to the TFT and blank all GPS Data.
                                            tftPrintGPSPresentHeader( ( XRIGHT - ( Xsize * 20) ), ( YMIDDLE - (Ysize * 2) ), false );
                                            clearTFTSensorData( ( XRIGHT - ( Xsize * 31 ) ), YMIDDLE , XRIGHT - ( XRIGHT - ( Xsize * 31 ) ), ( YMIDDLE + (fontYsize * 12) ) );
                                        }
                                }
                            
                            
                        #endif
                        
                        
                        twoSecCount = 0;
                    }  // End of two seconds updates.

                // Begin the ten second updates.                        
                tenSecCount++;
                if (tenSecCount >= 10)
                    {
                        #if defined(DEBUG46)  // This is the WatchDog debug flag.
                            wdt.feed();  // Kick the dog every 10 seconds, its set to reset the teensy in 12 seconds.
                            Serial.println("Reset the WatchDog Timer");
                        #else
                            wdt.feed();  // Kick the dog every 10 seconds, its set to reset the teensy in 12 seconds.
                        #endif

                        if (noBME680 == false)
                            {
                                if (SensorStruct.BME680DataValid == true)
                                    {
                                        drawPrintInsideTempPresHum(XRIGHT - (Xsize * 31), YTOP, SensorStruct.BME680TemperatureF, 
                                        SensorStruct.BME680Baro_Pressure, SensorStruct.BME680Relative_Humidity, SensorStruct.BME680gasSensor, SensorStruct.BME680altitude);

                                        //drawPrintInsideHighLowTempsTFT( ( XRIGHT - (Xsize * 31) ),  ( YTOP + (Ysize * 9) ) );  // Print the days high and low on the TFT
                                    } 

                                if( continueLogggingDataToRamDisk() )  // Returns false is there is a problem.
                                    {
                                        #if defined(DEBUG_LOOP_TEN_SECOND_COUNT)
                                            Serial.printf("\nLogged sensor data to LittleFS file, Total Number of Records in File: %d...\n", record_count);
                                        #endif

                                        #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                            clearStatusLine();  // Clear any leftovers in the status line.
                                            tft.printStatusLine(0,myColors[1],myColors[0],"Logged sensor data to LittleFS file...");
                                        #endif
                                    }
                                else
                                    {
                                        #if defined(DEBUG_LOOP_TEN_SECOND_COUNT)
                                            Serial.println("Error, Sensor data not logged to LittleFS file...");
                                        #endif

                                        #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                            clearStatusLine();  // Clear any leftovers in the status line.
                                            tft.printStatusLine(0,myColors[2],myColors[0],"Error, Sensor data not logged to LittleFS file...");
                                        #endif
                                    }
                            }
                        
                    
                        tenSecCount = 0;
                    }  // End of ten second updates.

                // Begin thirty second updates.
                thirtySecCount++;
                if (thirtySecCount >= 30)
                    {
                        #if defined(USINGGPS)
                            if ( (UTCTimeCorrected == false) && (gpsUBXDataStruct.validTime == 1) && (gpsUBXDataStruct.validDate == 1) && (gpsUBXDataStruct.fullyResolved == 1) )
                                {
                                    #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)   
                                        clearStatusLine();  // Clear any leftovers in the status line.
                                        tft.printStatusLine(0,myColors[1],myColors[0],"Updating the time from GPS...");
                                        delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds.
                                    #endif

                                    #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                        Serial.print(F("The Clock has not been corrected yet, Correcting..."));

                                        Serial.print(F("UTCTimeCorrected Flag: "));
                                        Serial.println(UTCTimeCorrected);

                                        Serial.print(F("gpsUBXDataStruct.validTime: "));
                                        Serial.println(gpsUBXDataStruct.validTime, BIN);

                                        Serial.print(F("gpsUBXDataStruct.validDate: "));
                                        Serial.println(gpsUBXDataStruct.validDate, BIN);

                                        Serial.print(F("gpsUBXDataStruct.fullyResolved: "));
                                        Serial.println(gpsUBXDataStruct.fullyResolved);

                                    #endif

                                    if (updateTimeFromGPS())  // This gets the GMT Time from the GPS attached to Serial port 1.
                                        {
                                            #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                                clearStatusLine();  // Clear any leftovers in the status line.
                                                tft.printStatusLine(0,myColors[1],myColors[0],"Updating the time from GPS Failed...");
                                                delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds.
                                            #endif

                                            #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                                Serial.println(F("Error: Did not update DS3231 Clock with GPS time!"));
                                                Serial.print(F("statusDotColor is: "));
                                                Serial.println(statusDotColor);
                                                Serial.print(F("UTCTimeCorrected flag: "));
                                                Serial.println(UTCTimeCorrected);
                                            #endif
                                        }
                                    else
                                        {
                                            #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                                Serial.println(F("Updated DS3231 Clock with GPS time!"));
                                                Serial.print(F("statusDotColor is: "));
                                                Serial.println(statusDotColor);
                                                Serial.print(F("UTCTimeCorrected flag: "));
                                                Serial.println(UTCTimeCorrected);
                                            #endif
                                        }
                                       
                                }
                            else
                                {
                                    #if defined(DEBUG_GPS_LOOP_MESSAGES)
                                        Serial.print(F("UTCTimeCorrected flag: "));
                                        Serial.println(UTCTimeCorrected);

                                        Serial.print(F("gpsUBXDataStruct.validTime: "));
                                        Serial.println(gpsUBXDataStruct.validTime, BIN);

                                        Serial.print(F("gpsUBXDataStruct.validDate: "));
                                        Serial.println(gpsUBXDataStruct.validDate, BIN);

                                        Serial.print(F("gpsUBXDataStruct.fullyResolved: "));
                                        Serial.println(gpsUBXDataStruct.fullyResolved);
                                    #endif
                                }
                        #endif

                        thirtySecCount = 0;
                    }

            // Begin with two minute updates.
             twoMinuteCount++;
                if (twoMinuteCount >= (60 * 2))  // This happens every 2 minutes.
                    {

                        twoMinuteCount = 0;
                    }  // End of two minute events.

                // Begin twelve hour count updates.
                twelveHourCount++;
                if (twelveHourCount >= (60 * 60 * 12))  // This happens every twelve hours.
                    {
                        // This is where the calibration belongs after debugging.
                        #if defined(DEBUG_LOOP_TEMPSENSORS_CALIBRATION)
                            Serial.println(F("Checking for TMP117 Present Flag..."));
                        #endif
                        if ( (noTMP117 == false) && (noBME680 == false) && (noMCP9808 == false) )
                            {
                                #if defined(DEBUG_LOOP_TEMPSENSORS_CALIBRATION)
                                    Serial.println(F("TMP117 is Present..."));
                                    Serial.println(F("Now doing the Compare TMP117 with the other Temp Sensors..."));
                                #endif

                                if ( (SensorStruct.BME680DataValid == true) && (SensorStruct.mcp9808DataValid == true) && (SensorStruct.tmp117DataValid == true))
                                    {
                                        compareTMP117WithOtherTempSensors();

                                        if (BME680CalibrateFlag == true)
                                            {
                                                #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                                    clearStatusLine();  // Clear any leftovers in the status line.
                                                    tft.printStatusLine(0,myColors[1],myColors[0],"Calibrating the BME680...");
                                                    delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds.
                                                #endif

                                                #if defined(DEBUG_LOOP_TEMPSENSORS_CALIBRATION)
                                                    Serial.println(F("BME680 Calibrate Flag is true, now doing the Calibrate for the BME680...\n"));
                                                #endif
                                                calibrateBME680TempSensor();
                                            }
                                        else
                                            {
                                                #if defined(DEBUG_LOOP_TEMPSENSORS_CALIBRATION)
                                                    Serial.println(F("BME680 Calibrate Flag is false, no Calibrate is necessary...\n"));
                                                #endif
                                            }
                                        
                                        if (MCP9808CalibrateFlag == true)
                                            {
                                                #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                                    clearStatusLine();  // Clear any leftovers in the status line.
                                                    tft.printStatusLine(0,myColors[1],myColors[0],"Calibrating the MCP9808...");
                                                    delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds.
                                                #endif

                                                #if defined(DEBUG_LOOP_TEMPSENSORS_CALIBRATION)
                                                    Serial.println(F("MCP9808 Calibrate Flag is true, now doing the Calibrate for the MCP9808...\n"));
                                                #endif
                                                calibrateMCP9808TempSensor();
                                            }
                                        else
                                            {
                                                #if defined(DEBUG_LOOP_TEMPSENSORS_CALIBRATION)
                                                    Serial.println(F("MCP9808 Calibrate Flag is false, no Calibrate is necessary...\n"));
                                                #endif    
                                            }
                                    }
                            }
                        else
                            {
                                #if defined(DEBUG_LOOP_TFTSTATUS_LINE_INFO)
                                    clearStatusLine();  // Clear any leftovers in the status line.
                                    tft.printStatusLine(0,myColors[1],myColors[0],"No TMP117, No Calibration Possible...");
                                    delay(MainLoopStatuslineDisplayTime);  // About .2 Seconds.
                                #endif

                                #if defined(DEBUG_LOOP_TEMPSENSORS_CALIBRATION)
                                    Serial.println(F("TMP117 is not Present, No Calibration possible...\n"));
                                #endif
                            }
                        
                        twelveHourCount = 0;
                    }  // End of twelve hour events.

                oneSecFlag = false;
            }  // End of one second events.

        if (fiveMSFlag == true)
            {
                if ( !myGNSS.checkUblox() ) // Check for the arrival of new data and process it.
                    {
                        Serial.println(F("Error, myGNSS checkUblox returned bad Comm Type...\n"));
                    }
                //Serial.print(F("."));
                
                fiveMSFlag = false;
            }
        
        if (oneHundredMSFlag == true)
            {
                MTP.loop();  // This is mandatory to be placed in the loop code.
                
                oneHundredMSFlag = false;
            }
        
        myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

    }

