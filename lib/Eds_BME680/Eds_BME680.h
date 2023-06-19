/*! @file Eds_BME680.h

@mainpage Arduino Library to access and control a Bosch BME680 Environmental Sensor

@section Eds_BME680_section Description

Class definition header for the Bosch BME680 temperature / humidity / pressure / air quality sensor.
The device is described at https://www.bosch-sensortec.com/bst/products/all_products/BME680 and the
datasheet is available from Bosch at
https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BME680-DS001-00.pdf
\n\n

The BME680 can use either SPI or I2C for communications. This library supports I2C at various bus
speeds as well as SPI using either the standard Arduino hardware SPI or software SPI. Depending upon
the pin configuration a BME680 can have 2 distinct I2C addresses - either 0x76 or 0x77.\n\n

The most recent version of the library is available at https://github.com/Zanduino/BME680 and
extensive documentation of the library as well as descriptions of the various example programs are
described in the project's wiki pages located at https://github.com/Zanduino/BME680/wiki
\n
The BME680 is an extremely small physical package that is so tiny as to be impossible to solder at
home, hence it will be used as part of a third-party breakout board. There are several such boards
available at this time, for example:\n

Company  | Link
-------  | ----------
SparkFun | https://www.sparkfun.com/products/14570
BlueDot  | https://www.bluedot.space/sensor-boards/bme680/
Adafruit |
https://learn.adafruit.com/adafruit-BME680-humidity-barometric-pressure-temperature-sensor-breakout

\n
Bosch supplies sample software that runs on various platforms, including the Arduino family; this
can be downloaded at https://github.com/BoschSensortec/BSEC-Arduino-library. This software is part
of the Bosch "BSEC" (Bosch Sensortec Environmental Cluster) framework and is somewhat bulky and
rather unwieldy for typical Arduino applications, it won't run on most Arduinos and the only example
is for the Arduino Mega 2560 (due to the memory required), hence the decision to make this compact
and rather less abstract library that will run on typical Arduino hardware.

@section license GNU General Public License v3.0

This program is free software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version. This program is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should
have received a copy of the GNU General Public License along with this program.  If not, see
<http://www.gnu.org/licenses/>.

*/

#include "Arduino.h"  // Arduino data type definitions

#include <SPI.h>   // Standard SPI library
#include <Wire.h>  // Standard I2C "Wire" library


#ifndef BME680_h
  #define BME680_h  ///< Guard code definition for the header
  #define CONCAT_BYTES(msb, lsb) (((uint16_t)msb << 8) | (uint16_t)lsb)  ///< combine msb & lsb bytes
  #ifndef _BV
    #define _BV(bit) (1 << (bit))  ///< This macro isn't pre-defined on all platforms
  #endif

  /***************************************************************************************************
  ** Declare publically visible constants used in the class **
  ***************************************************************************************************/
  #ifndef I2C_MODES                           //   If the I2C_Modes haven't been declared yet
    #define I2C_MODES                         ///< Guard code definition for the I2C modes
    const uint32_t I2C_STANDARD_MODE{100000};     ///< Default normal I2C 100KHz speed
    const uint32_t I2C_FAST_MODE{400000};         ///< Fast mode
    const uint32_t I2C_FAST_MODE_PLUS{1000000};   ///< Really fast mode
    const uint32_t I2C_HIGH_SPEED_MODE{3400000};  ///< Turbo mode
  #endif
const uint32_t SPI_HERZ{500000};  ///< SPI speed in Hz

/***************************************************************************************************
** Declare enumerated types used in the class                                                     **
***************************************************************************************************/
/*! @brief  Enumerate the sensor type */
enum sensorTypes { TemperatureSensor, HumiditySensor, PressureSensor, GasSensor, UnknownSensor };
/*! @brief  Enumerate the Oversampling types */
enum oversamplingTypes
{
  SensorOff,
  Oversample1,
  Oversample2,
  Oversample4,
  Oversample8,
  Oversample16,
  UnknownOversample
};

/*! @brief  Enumerate the iir filter types */
enum iirFilterTypes { IIROff, IIR2, IIR4, IIR8, IIR16, IIR32, IIR64, IIR128, UnknownIIR };

class BME680_Class
{
  /*!
   * @class BME680_Class
   * @brief Main BME680 class for the temperature / humidity / pressure sensor
   */
 public:
    BME680_Class();                           // Class constructor (unused)
    ~BME680_Class();                          // Class destructor (unused)
    bool    begin();                          // Start using I2C Communications
    bool    begin(const uint32_t i2cSpeed, TwoWire *theWire = &Wire);   // I2C with a non-default speed
    bool    begin(const uint32_t i2cSpeed, const uint8_t i2cAddress, TwoWire *theWire = &Wire);  // Set speed and I2C Addr.

    bool    begin(const uint8_t chipSelect);  // Start using either I2C or HW-SPI
    bool    begin(const uint8_t chipSelect, const uint8_t mosi, const uint8_t miso, const uint8_t sck);  // Start using software SPI

    uint8_t setOversampling(const uint8_t sensor,  const uint8_t sampling = UINT8_MAX) const;  // Set enum sensorType Oversampling and return current value.
    bool    setGas(uint16_t GasTemp, uint16_t GasMillis) const;  // Gas heating temperature and time
    uint8_t setIIRFilter(const uint8_t iirFilterSetting = UINT8_MAX) const;  // Set IIR Filter.
    void    set_offset_temp(float toffset);     // Set the calibration offset temp to correct the offset temp of the sensor. 
    void    set_offset_hum(float hoffset);      // Set the calibration offset humidity to correct the offset humidity of the sensor.
    float   get_offset_temp();                  // Retrieve the calibration offset temp used to correct the offset temp of the sensor.
    float   get_offset_hum();                   // Retrieve the calibration offset humidity used to correct the humidity temp of the sensor.

    uint8_t getSensorData(int32_t &temp, int32_t &hum, int32_t &press, int32_t &gas, const bool waitSwitch = true);     // Get most recent readings in integer format.
    //uint8_t getSensorDataFlt(float &temp, float &hum, float &press, float &gas, const bool waitSwitch = true);        // Get the most recent readings in Float Format.

    bool    isIAQValid();
    float   getSeaLevel();
    float   getAlt(void);   
    float   getCalibAlt();
    float   getTVoc(void);
    float   absHum(float temp, float hum);
    float   dewPoint(float temp, float hum);

    uint8_t getI2CAddress() const;                        // Return the I2C Address of the BME680
    void    reset();                                      // Reset the BME680
    bool    measuring() const;                            ///< true if currently measuring
    void    triggerMeasurement() const;                   ///< trigger a measurement.

 private:                                               //
  bool     commonInitialization();                      ///< Common initialization code
  uint8_t  readByte(const uint8_t addr) const;          ///< Read byte from register address
  uint8_t  readSensors(const bool waitSwitch);          ///< read the registers in one burst
  void     waitForReadings() const;                     ///< Wait for readings to finish
  void     getCalibration();                            ///< Load calibration from registers
  uint8_t  _I2CAddress = 0;                             ///< Default is I2C address is unknown
  uint16_t _I2CSpeed   = 0;                             ///< Default is I2C speed is unknown.
  TwoWire *_wire;                                       /// The I2C bus abstraction name.

  uint8_t  _cs, _sck, _mosi, _miso;                     ///< Hardware and software SPI pins
  uint8_t  _H6, _P10, _res_heat_range;                  ///< unsigned configuration vars
  int8_t   _H3, _H4, _H5, _H7, _G1, _G3, _T3, _P3, _P6, _P7, _res_heat,
      _rng_sw_err;                                            ///< signed configuration vars
  uint16_t _H1, _H2, _T1, _P1;                                ///< unsigned 16bit configuration vars
  int16_t  _G2, _T2, _P2, _P4, _P5, _P8, _P9;                 ///< signed 16bit configuration vars
  int32_t  _tfine, _Temperature, _Pressure, _Humidity, _Gas;  ///< signed 32bit configuration vars

  /*!
   @section Template functions
   Declare the getData and putData methods as template functions. All device I/O is done through
   these two functions regardless of whether I2C, hardware SPI or software SPI is being used. The
   two functions are designed so that only the address and a variable are passed in and the
   functions determine the size of the parameter variable and reads or writes that many bytes. So
   if a read is called using a character array[10] then 10 bytes are read, if called with a int8
   then only one byte is read. The return value, if used, is the number of bytes read or written.
   Since this is implemented by using template function definitions, they need to be defined in
   this header file rather than in the c++ program library file. The "getData()" function is
   currently not used in the library directly, but the function "readByte()" is used which calls
   the getData().  The "putData()" is called directly in the code.
  */


 /*!
 * @brief     Template for reading from I2C or SPI using any data type
 * @details   As a template it can support compile-time data type definitions
 * @param[in] addr Memory address
 * @param[in] value Data Type "T" to read
 * @return    Size of data read in bytes
 */
  template <typename T>
  uint8_t &getData(const uint8_t addr, T &value) const
  {
    uint8_t *      bytePtr    = (uint8_t *)&value;  // Pointer to structure beginning
    static uint8_t structSize = sizeof(T);          // Number of bytes in structure
    if (_I2CAddress)                                // Using I2C if address is non-zero
        {                                               //
             _wire->beginTransmission(_I2CAddress);          // Address the I2C device
            _wire->write(addr);                             // Send register address to read
            _wire->endTransmission();                       // Close transmission
            _wire->requestFrom(_I2CAddress, sizeof(T));     // Request 1 byte of data
            structSize = _wire->available();                // Use the actual number of bytes
            for (uint8_t i = 0; i < structSize; i++)
            	{
            		*bytePtr++ = _wire->read();  // loop for each byte to be read.
            	}
        }
    else
        {
            if (_sck == 0)  // if sck is zero then hardware SPI
                {
                    SPI.beginTransaction(SPISettings(SPI_HERZ, MSBFIRST, SPI_MODE0));  // Start the SPI transaction
                    digitalWrite(_cs, LOW);                           // Tell BME680 to listen up
                    SPI.transfer(addr | 0x80);                        // bit 7 is high, so read a byte
                    for (uint8_t i = 0; i < structSize; i++)
                        {
                            *bytePtr++ = SPI.transfer(0);  // loop for each byte to be read
                            digitalWrite(_cs, HIGH);         // Tell BME680 to stop listening
                            SPI.endTransaction();            // End the transaction
                        }
                }
            else
                {                           // otherwise we are using soft SPI
                    int8_t  i, j;                    // Loop variables
                    uint8_t reply;                   // return byte for soft SPI transfer
                    digitalWrite(_cs, LOW);          // Tell BME680 to listen up
                    for (j = 7; j >= 0; j--)         // First send the address byte
                        {
                            digitalWrite(_sck, LOW);                          // set the clock signal
                            digitalWrite(_mosi, ((addr) | 0x80) & (1 << j));  // set the MOSI pin state
                            digitalWrite(_sck, HIGH);                         // reset the clock signal
                        }                                                   // of for-next each bit
                    for (i = 0; i < structSize; i++)                    // Loop for each byte to read
                        {                                                   //
                            reply = 0;                                        // reset our return byte
                            for (j = 7; j >= 0; j--)                          // Now read the data at that byte
                                {                                                 //
                                    reply <<= 1;                                    // shift buffer one bit left
                                    digitalWrite(_sck, LOW);                        // set and reset the clock signal
                                    digitalWrite(_sck, HIGH);                       // pin to get the next MISO bit
                                    if (digitalRead(_miso)) reply |= 1;             // read the MISO bit, add to reply
                                }                                                 // of for-next each bit
                            *bytePtr++ = reply;                               // Add byte just read to return data
                        }                                                   // of for-next each byte to be read
                    digitalWrite(_cs, HIGH);                            // Tell BME680 to stop listening
                }  // of  if-then-else we are using hardware SPI
        }    // of if-then-else we are using I2C
    return (structSize);
  }  // of method getData()


/*!
* @brief     Template for writing to I2C or SPI using any data type
* @details   As a template it can support compile-time data type definitions
* @param[in] addr Memory address
* @param[in] value Data Type "T" to write
* @return    Size of data written in bytes
*/
  template <typename T>
  uint8_t &putData(const uint8_t addr, const T &value) const
  {
    const uint8_t *bytePtr    = (const uint8_t *)&value;  // Pointer to structure beginning
    static uint8_t structSize = sizeof(T);                // Number of bytes in structure
    if (_I2CAddress)                                      // Using I2C if address is non-zero
        {                                                     //
            _wire->beginTransmission(_I2CAddress);                // Address the I2C device
            _wire->write(addr);                                   // Send register address to write
            for (uint8_t i = 0; i < sizeof(T); i++)
                {
                    _wire->write(*bytePtr++);  // loop for each byte to be written
                    _wire->endTransmission();    // Close transmission
                }
        }
    else
        {
            if (_sck == 0)  // if sck is zero then hardware SPI
                {
                    SPI.beginTransaction(SPISettings(SPI_HERZ, MSBFIRST, SPI_MODE0));  // start the SPI transaction
                    digitalWrite(_cs, LOW);                           // Tell BME680 to listen up
                    SPI.transfer(addr & ~0x80);                       // bit 7 is low, so write a byte
                    for (uint8_t i = 0; i < structSize; i++)
                        {
                            SPI.transfer(*bytePtr++);
                        }  // loop for each byte to be written
                    digitalWrite(_cs, HIGH);  // Tell BME680 to stop listening
                    SPI.endTransaction();     // End the transaction
                }
            else  // Otherwise soft SPI is used
                {
                    int8_t  i, j;                     // Loop variables
                    uint8_t reply;                    // return byte for soft SPI transfer
                    for (i = 0; i < structSize; i++)  // Loop for each byte to read
                        {
                            reply = 0;                // reset our return byte
                            digitalWrite(_cs, LOW);   // Tell BME680 to listen up
                            for (j = 7; j >= 0; j--)  // First send the address byte
                                {
                                    digitalWrite(_sck, LOW);                         // set the clock signal
                                    digitalWrite(_mosi, (addr & ~0x80) & (1 << j));  // set the MOSI pin state
                                    digitalWrite(_sck, HIGH);                        // reset the clock signal
                                }                                                  // of for-next each bit
                            for (j = 7; j >= 0; j--)                           // Now read the data at that byte
                                {
                                    reply <<= 1;                               // shift buffer one bit left
                                    digitalWrite(_sck, LOW);                   // set the clock signal
                                    digitalWrite(_mosi, *bytePtr & (1 << j));  // set the MOSI pin state
                                    digitalWrite(_sck, HIGH);                  // reset the clock signal
                                }                                            // of for-next each bit
                            digitalWrite(_cs, HIGH);                     // Tell BME680 to stop listening
                            bytePtr++;                                   // go to next byte to write
                        }                                              // of for-next each byte to be read
                }                                                // of  if-then-else we are using hardware SPI
        }                                                  // of if-then-else we are using I2C
    return (structSize);
  }  // of method putData()
};   // of BME680 class definition
#endif
