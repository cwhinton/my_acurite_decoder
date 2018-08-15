#include <Arduino.h>
#include "RFM69_Helper.h";
#include <SPI.h>
#include "rfm69_constants.h"
#include <RFM69.h>

SPISettings SPI_settings(2000000, MSBFIRST, SPI_MODE0);

RFM69_Helper::RFM69_Helper(int selPin){
  _selPin = selPin;
}

//
// talk to the RFM69
uint16_t RFM69_Helper::xfer16(uint16_t cmd)
{
  uint16_t reply;

  SPI.beginTransaction(SPI_settings);

  if (_selPin != -1)
    digitalWrite(_selPin, LOW);
  reply = SPI.transfer(cmd >> 8) << 8;
  reply |= SPI.transfer(cmd & 0xFF);
  if (_selPin != -1)
    digitalWrite(_selPin, HIGH);

  SPI.endTransaction();

  return reply;
}

uint8_t RFM69_Helper::read(uint8_t reg)
{
  uint8_t reply;
  
  SPI.beginTransaction(SPI_settings);
  if (_selPin != -1)
    digitalWrite(_selPin, LOW);
    
  SPI.transfer(reg);
  reply = SPI.transfer(0x00);
    
  if (_selPin != -1)
    digitalWrite(_selPin, HIGH);
    
  SPI.endTransaction();
  
  return reply;
}

void RFM69_Helper::write(uint8_t reg, uint8_t val)
{
  SPI.beginTransaction(SPI_settings);
  if (_selPin != -1)
    digitalWrite(_selPin, LOW);
    
  SPI.transfer(reg | 0x80); // write bit
  SPI.transfer(val);
    
  if (_selPin != -1)
    digitalWrite(_selPin, HIGH);
    
  SPI.endTransaction();
}

void RFM69_Helper::initOOK () {
//  uint8_t dev_id = rfm69_read(RegVersion);
//  if (dev_id != 0x00 || dev_id != 0xff)
//    return;

  write(RegOpMode, RegOpModeStandby);
  write(RegDataModul, RegDataModulContinuous | RegDataModulOOK); // Set continuous OOK mode
  RegBitrateSet(8000); // 8.0kb/s
  RegFrfSet(433920000); // fundamental frequency = 433.92MHz (really 433.920044 MHz)
  
  write(RegRxBw, RegRxBwDccFreq4 | RegRxBwOOK50k); // 4% DC cancellation; 50k bandwidth in OOK mode
  write(RegLna, RegLnaZ200 | RegLnaGainSelect12db); // 200 ohm, -12db

  write(RegOokPeak,
   RegOokThreshPeak | RegOokThreshPeakStep0d5 | RegOokThreshPeakDec1c );

  write(RegOpMode, RegOpModeRX);
  write(RegAfcFei, RegAfcFeiAfcClear);
}
