//#include <Wire.h>
#include "Arduino.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esp_one_wire_800.h"


namespace esphome {
namespace dallas {

static const char *const TAG = "dallas.one_wire_ds2482";

ESPOneWire800::ESPOneWire800() {}

bool HOT IRAM_ATTR ESPOneWire800::reset() {
 ESP_LOGCONFIG(TAG, "Reset");
 return (readI2CByte()) ? 0 : -1; // presence
}

void IRAM_ATTR ESPOneWire800::writeI2CByte(uint8_t data)
{
	buffer_data[0] = data;
	write(buffer_data, 1);
}

void IRAM_ATTR ESPOneWire800::writeI2CByte2(uint8_t data0, uint8_t data1)
{
	buffer_data[0] = data0;
    buffer_data[1] = data1;

	write(buffer_data, 2);
}

uint8_t IRAM_ATTR ESPOneWire800::readI2CByte()
{
	read(buffer_data,1);
	return buffer_data[0];
}

// Performs a global reset of device state machine logic. Terminates any ongoing 1-Wire communication.
bool IRAM_ATTR ESPOneWire800::deviceReset()
{
	writeI2CByte(DS2482_COMMAND_RESET);
	return true;
}

// Sets the read pointer to the specified register. Overwrites the read pointer position of any 1-Wire communication command in progress.
void IRAM_ATTR ESPOneWire800::setReadPointer(uint8_t readPointer)
{
	writeI2CByte2(DS2482_COMMAND_SRP, readPointer);
}

// Read the status register
uint8_t IRAM_ATTR ESPOneWire800::readStatus()
{
	setReadPointer(DS2482_POINTER_STATUS);
	return readI2CByte();
}

// Read the data register
uint8_t IRAM_ATTR ESPOneWire800::readData()
{
	setReadPointer(DS2482_POINTER_DATA);
	return readI2CByte();
}

// Read the config register
uint8_t IRAM_ATTR ESPOneWire800::readConfig()
{
	setReadPointer(DS2482_POINTER_CONFIG);
	return readI2CByte();
}

void IRAM_ATTR ESPOneWire800::setStrongPullup()
{
	writeConfig(readConfig() | DS2482_CONFIG_SPU);
}

void IRAM_ATTR ESPOneWire800::clearStrongPullup()
{
	//writeConfig(readConfig() & !DS2482_CONFIG_SPU);
	writeConfig(readConfig() &~DS2482_CONFIG_SPU);
}

// Churn until the busy bit in the status register is clear
uint8_t IRAM_ATTR ESPOneWire800::waitOnBusy()
{
	uint8_t status;

	for(int i=1000; i>0; i--)
	{
		status = readStatus();
		if (!(status & DS2482_STATUS_BUSY))
			break;
		delayMicroseconds(20);
	}

	// if we have reached this point and we are still busy, there is an error
	if (status & DS2482_STATUS_BUSY)
		mError = DS2482_ERROR_TIMEOUT;

	// Return the status so we don't need to explicitly do it again
	return status;
}

// Write to the config register
void IRAM_ATTR ESPOneWire800::writeConfig(uint8_t config)
{
	waitOnBusy();

//	// Write the 4 bits and the complement 4 bits
    writeI2CByte2(DS2482_COMMAND_WRITECONFIG, config | (~config)<<4);

	// This should return the config bits without the complement
	if (readI2CByte() != config)
		mError = DS2482_ERROR_CONFIG;
}

// Generates a 1-Wire reset/presence-detect cycle (Figure 4) at the 1-Wire line. The state
// of the 1-Wire line is sampled at tSI and tMSP and the result is reported to the host 
// processor through the Status Register, bits PPD and SD.
uint8_t IRAM_ATTR ESPOneWire800::wireReset()
{
	waitOnBusy();
	// Datasheet warns that reset with SPU set can exceed max ratings
	clearStrongPullup();

	waitOnBusy();

	writeI2CByte(DS2482_COMMAND_RESETWIRE);

	uint8_t status = waitOnBusy();

	if (status & DS2482_STATUS_SD)
	{
		mError = DS2482_ERROR_SHORT;
	}

	return (status & DS2482_STATUS_PPD) ? true : false;
}

// Writes a single data byte to the 1-Wire line.
void IRAM_ATTR ESPOneWire800::wireWriteByte(uint8_t data, uint8_t power)
{
	waitOnBusy();
	if (power)
		setStrongPullup();

    writeI2CByte2(DS2482_COMMAND_WRITEBYTE,data);
}

// Generates eight read-data time slots on the 1-Wire line and stores result in the Read Data Register.
uint8_t IRAM_ATTR ESPOneWire800::wireReadByte()
{
	waitOnBusy();
	writeI2CByte(DS2482_COMMAND_READBYTE);
	waitOnBusy();
	return readData();
}

// Generates a single 1-Wire time slot with a bit value “V” as specified by the bit byte at the 1-Wire line
// (see Table 2). A V value of 0b generates a write-zero time slot (Figure 5); a V value of 1b generates a 
// write-one time slot, which also functions as a read-data time slot (Figure 6). In either case, the logic
// level at the 1-Wire line is tested at tMSR and SBR is updated.
void IRAM_ATTR ESPOneWire800::wireWriteBit(uint8_t data, uint8_t power)
{
	waitOnBusy();
	if (power)
		setStrongPullup();
	
    writeI2CByte2(DS2482_COMMAND_SINGLEBIT, data ? 0x80 : 0x00);	
}

// As wireWriteBit
uint8_t IRAM_ATTR ESPOneWire800::wireReadBit()
{
	wireWriteBit(1);
	uint8_t status = waitOnBusy();
	return status & DS2482_STATUS_SBR ? 1 : 0;
}

// 1-Wire skip
void IRAM_ATTR ESPOneWire800::wireSkip()
{
	wireWriteByte(WIRE_COMMAND_SKIP);
}

void IRAM_ATTR ESPOneWire800::wireSelect(const uint8_t rom[8])
{
	wireWriteByte(WIRE_COMMAND_SELECT);
	for (int i=0;i<8;i++)
		wireWriteByte(rom[i]);
}

void IRAM_ATTR ESPOneWire800::wireSelect(const uint64_t rom)
{
	wireWriteByte(WIRE_COMMAND_SELECT);
	for (int i=0;i<8;i++)
	{
		wireWriteByte(((rom>>(8*i))&0xff)); 
	}
}

//  1-Wire reset seatch algorithm
void IRAM_ATTR ESPOneWire800::wireResetSearch()
{
	searchLastDiscrepancy = 0;
	searchLastDeviceFlag = 0;

	for (int i = 0; i < 8; i++)
	{
		searchAddress[i] = 0;
	}

}

// Set the channel on the DS2482-800
uint8_t IRAM_ATTR ESPOneWire800::setChannel(uint8_t ch){
  uint8_t w[] = {0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87};
  uint8_t r[] = {0xb8, 0xb1, 0xaa, 0xa3, 0x9c, 0x95, 0x8e, 0x87};
  waitOnBusy();
    
    writeI2CByte2(DS2482_COMMAND_CHANNELSEL,w[ch]);

     ESP_LOGD(TAG, "Channel Set: %d", ch);

  waitOnBusy();
  return readI2CByte() == r[ch];
}

// Perform a search of the 1-Wire bus
uint8_t IRAM_ATTR ESPOneWire800::wireSearch(uint8_t *address)
{
	uint8_t direction;
	int8_t last_zero=-1; //

	if (searchLastDeviceFlag)
		return 0;

	if (!wireReset())
		return 0;

	waitOnBusy();

	wireWriteByte(WIRE_COMMAND_SEARCH);

	for(uint8_t i=0;i<64;i++)
	{
		int searchByte = i / 8; 
		int searchBit = 1 << i % 8;

		if (i < searchLastDiscrepancy)
			direction = searchAddress[searchByte] & searchBit;
		else
			direction = i == searchLastDiscrepancy;

		waitOnBusy();

	    writeI2CByte2(DS2482_COMMAND_TRIPLET, direction ? 0x80 : 0x00 );

		uint8_t status = waitOnBusy();

		uint8_t id = status & DS2482_STATUS_SBR;
		uint8_t comp_id = status & DS2482_STATUS_TSB;
		direction = status & DS2482_STATUS_DIR;

		if (id && comp_id)
		{
			return 0;
		}
		else
		{
			if (!id && !comp_id && !direction)
			{
				last_zero = i;
			}
		}

		if (direction)
			searchAddress[searchByte] |= searchBit;
		else
			searchAddress[searchByte] &= ~searchBit;

	}

	searchLastDiscrepancy = last_zero;

	if (last_zero == -1)
		searchLastDeviceFlag = 1;

	for (uint8_t i=0; i<8; i++)
		address[i] = searchAddress[i];

	return 1;
}

#if ONEWIRE_CRC8_TABLE
// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t PROGMEM dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t IRAM_ATTR ESPOneWire800::crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
		crc = pgm_read_byte(dscrc_table + (crc ^ *addr++));
	}
	return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
uint8_t IRAM_ATTR ESPOneWire800::crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
#endif

std::vector<uint64_t> ESPOneWire800::search_vec() {
  std::vector<uint64_t> res;

  unsigned char romids[8];
  uint64_t romid;
  int idx = 0;

 	  int ret = wireSearch(romids);
  
	    romid = (uint64_t)romids[7]<<56|(uint64_t)romids[6]<<48
		      | (uint64_t)romids[5]<<40|(uint64_t)romids[4]<<32
			  | (uint64_t)romids[3]<<24|(uint64_t)romids[2]<<16
			  | (uint64_t)romids[1]<<8 |(uint64_t)romids[0]<<0;
 	 
	  if (ret==true) 
	  {
		res.push_back(romid);
		ESP_LOGI(TAG, "New Sensor: %2x:%2x:%2x:%2x:%2x:%2x:%2x:%2x",  romids[7], romids[6], romids[5], romids[4], romids[3], romids[2], romids[1], romids[0]);
	  }
	  else
        ESP_LOGCONFIG(TAG, "No Sensor found");


	  while(ret){

	    ret = wireSearch(romids);    
	    romid = (uint64_t)romids[7]<<56|(uint64_t)romids[6]<<48
		      | (uint64_t)romids[5]<<40|(uint64_t)romids[4]<<32
			  | (uint64_t)romids[3]<<24|(uint64_t)romids[2]<<16
			  | (uint64_t)romids[1]<<8 |(uint64_t)romids[0]<<0;

	    if (ret==true) 
		{
			res.push_back(romid);
		ESP_LOGI(TAG, "New Sensor: %2x:%2x:%2x:%2x:%2x:%2x:%2x:%2x",  romids[7], romids[6], romids[5], romids[4], romids[3], romids[2], romids[1], romids[0]);

		}

	  }
  return res;
 }

  } // namespace esphome
}   // namespace dallas
