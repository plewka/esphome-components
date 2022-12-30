

#ifndef DS2482_DEFS_H_
#define DS2482_DEFS_H_



/** Type definitions */
/*!
 * Generic communication function pointer
 * @param[in] dev_id: Place holder to store the id of the device structure
 *                    Can be used to store the index of the Chip select or
 *                    I2C address of the device.
 * @param[in] reg_addr:	Used to select the register the where data needs to
 *                      be read from or written to.
 * @param[in/out] reg_data: Data array to read/write
 * @param[in] len: Length of the data array
 */
typedef int8_t (*ds2482_com_fptr_t)(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len);

/*!
 * @brief BME680 device structure
 */
struct	ds2482_dev {
	/*! Chip Id */
	//uint8_t chip_id;
	/*! Device Id */
	//uint8_t dev_id;
	/*! SPI/I2C interface */
	//enum bme680_intf intf;
	/*! Memory page used */
	//uint8_t mem_page;
	/*! Ambient temperature in Degree C */
	//int8_t amb_temp;
	/*! Sensor calibration data */
	//struct bme680_calib_data calib;
	/*! Sensor settings */
	//struct bme680_tph_sett tph_sett;
	/*! Gas Sensor settings */
	//struct bme680_gas_sett gas_sett;
	/*! Sensor power modes */
	//uint8_t power_mode;
	/*! New sensor fields */
	//uint8_t new_fields;
	/*! Store the info messages */
	//uint8_t info_msg;
	/*! Bus read function pointer */
	ds2482_com_fptr_t read;
	/*! Bus write function pointer */
	ds2482_com_fptr_t write;
	/*! delay function pointer */
	//bme680_delay_fptr_t delay_ms;
	/*! Communication function result */
	//int8_t com_rslt;
};

typedef struct{
	uint64_t address;
	uint8_t channel;
}address_channel;

typedef struct{
	struct ds2482_dev *dev;
}my_dev_t;

#endif
