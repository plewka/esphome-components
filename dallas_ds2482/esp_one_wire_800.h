#pragma once

#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include <vector>
#include "ds2482_defs.h"

#include <stddef.h>
#include <inttypes.h>

// Chose between a table based CRC (flash expensive, fast)
// or a computed CRC (smaller, slow)
#define ONEWIRE_CRC8_TABLE 			1

#define DS2482_COMMAND_RESET		0xF0	// Device reset

#define DS2482_COMMAND_SRP			0xE1 	// Set read pointer
#define DS2482_POINTER_STATUS		0xF0
#define DS2482_STATUS_BUSY			(1<<0)
#define DS2482_STATUS_PPD			(1<<1)
#define DS2482_STATUS_SD			(1<<2)
#define DS2482_STATUS_LL			(1<<3)
#define DS2482_STATUS_RST 			(1<<4)
#define DS2482_STATUS_SBR			(1<<5)
#define DS2482_STATUS_TSB 			(1<<6)
#define DS2482_STATUS_DIR 			(1<<7)
#define DS2482_POINTER_DATA			0xE1
#define DS2482_POINTER_CONFIG		0xC3
#define DS2482_CONFIG_APU			(1<<0)
#define DS2482_CONFIG_SPU			(1<<2)
#define DS2482_CONFIG_1WS			(1<<3)

#define DS2482_COMMAND_WRITECONFIG	0xD2
#define DS2482_COMMAND_CHANNELSEL   0xC3
#define DS2482_COMMAND_RESETWIRE	0xB4
#define DS2482_COMMAND_WRITEBYTE	0xA5
#define DS2482_COMMAND_READBYTE		0x96
#define DS2482_COMMAND_SINGLEBIT	0x87
#define DS2482_COMMAND_TRIPLET		0x78

#define WIRE_COMMAND_SKIP			0xCC
#define WIRE_COMMAND_SELECT			0x55
#define WIRE_COMMAND_SEARCH			0xF0

//#define DS2482_ADDRESS 0x18

#define DS2482_ERROR_TIMEOUT		(1<<0)
#define DS2482_ERROR_SHORT			(1<<1)
#define DS2482_ERROR_CONFIG			(1<<2)

namespace esphome {
namespace dallas {

typedef enum {
    READ_ROM = 0x33, // DS18B20
    MATCH_ROM = 0x55, // DS18B20
    SEARCH_ROM = 0xF0, // DS18B20
    SKIP_ROM = 0xCC, // DS18B20 *
    ALARM_SEARCH = 0xEC, // DS18B20
    CONVERT_T = 0x44, // DS18B20
    WRITE_SCRATCHPAD = 0x4E,  // DS18B20
    READ_SCRATCHPAD = 0xBE,  // DS18B20
    COPY_SCRATCHPAD = 0x48,  // DS18B20
    RECALL_EE = 0xB8,  // DS18B20
    READ_POWER_SUPPLY = 0xB4,   // DS18B20
    RESUME = 0xA5,
    OVERDRIVE_SKIP = 0x3C,
    OVERDRIVE_MATCH = 0x69,
} one_wire_rom_commands;

extern const uint8_t ONE_WIRE_ROM_SELECT;
extern const int ONE_WIRE_ROM_SEARCH;

class ESPOneWire800 : public i2c::I2CDevice{
 public:
//  explicit ESPOneWire800(InternalGPIOPin *pin, my_dev_t *dev);
  explicit ESPOneWire800();  
        bool reset();
        
  /** Reset the bus, should be done before all write operations.
   *
   * Takes approximately 1ms.
   *
   * @return Whether the operation was successful.
   */

//	uint8_t getAddress(); // unused
//	uint8_t getError();
//	uint8_t checkPresence();

	bool deviceReset();
	void setReadPointer(uint8_t readPointer);
	uint8_t readStatus();
	uint8_t readData();
	uint8_t waitOnBusy();
	uint8_t readConfig();
	void writeConfig(uint8_t config);
	void setStrongPullup();
        uint8_t setChannel(uint8_t ch);
	void clearStrongPullup();
	uint8_t wireReset();
	void wireWriteByte(uint8_t data, uint8_t power = 0);
	uint8_t wireReadByte();
	void wireWriteBit(uint8_t data, uint8_t power = 0);
	uint8_t wireReadBit();
	void wireSkip();
	void wireSelect(const uint8_t rom[8]);
        void wireSelect(const uint64_t rom);
	
	void wireResetSearch();
	uint8_t wireSearch(uint8_t *address);

	static uint8_t crc8(const uint8_t *addr, uint8_t len);

  /// Helper that wraps search in a std::vector.
  std::vector<uint64_t> search_vec();

 protected:
	void writeI2CByte(uint8_t);   // remapped
	void writeI2CByte2(uint8_t data0, uint8_t data1);
	uint8_t readI2CByte();	// remapped

	uint8_t mError;
    uint8_t buffer_data[2];
    uint8_t buffer_len;
	uint8_t searchAddress[8];
	uint8_t searchLastDiscrepancy;
	uint8_t searchLastDeviceFlag;


  /// Helper to get the internal 64-bit unsigned rom number as a 8-bit integer pointer.
  inline uint8_t *rom_number8_();

  uint8_t last_discrepancy_{0};
  bool last_device_flag_{false};
  uint64_t rom_number_{0};

}; // ESPOneWire800

}  // namespace dallas
}  // namespace esphome
