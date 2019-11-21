// 
// 
// 

#include "config.h"
#include <EEPROM.h>

#define START_ADDRESS 0x40


/**
 * @brief Configuration data
 *
 * This is a RAM shadow of the data in NV memory
*/
config_t ConfigClass::config;

/**
 * @brief Configuration data
 *
 * This is a RAM shadow of the data in NV memory
*/
int ConfigClass::init()
{
	EEPROM.begin(CONFIGURE_SIZE/* + START_ADDRESS*/);
	EEPROM.get(START_ADDRESS, config.bytes);

	size_t length = sizeof(config_t);
	int i;

	Serial.println("LOAD");
	/* get one byte at a time from the eeprom and compare to shadow copy, if it differs write the shadow copy to EEPROM */
	for (i = 0; i < (length - 1); i++)
	{
		Serial.printf("%d \t %d\r\n", i, config.bytes[i]);
	}

	return checkNV();
}

void ConfigClass::update()
{
	size_t length = sizeof(config_t);
	int i;

	/* get one byte at a time from the eeprom and compare to shadow copy, if it differs write the shadow copy to EEPROM */
	for (i = 0; i < (length - 1); i++)
	{
		//EEPROM.update(START_ADDRESS + i, config.bytes[i]);

		byte data;
		EEPROM.get(START_ADDRESS + i, data);
		if (data != config.bytes[i])
			EEPROM.write(START_ADDRESS + i, config.bytes[i]);
	}
}

void ConfigClass::save()
{
	/*
	 *	EEPROM.put() allows writing of any data type to the Arduino EEPROM.
	 *	This uses the EEPROM.update() function so only changed values are written
	 */
	config.nv.checksum = getChecksum();

	EEPROM.put(START_ADDRESS, config.bytes);

	size_t length = sizeof(config_t);
	int i;

	Serial.println("SAVE");
	/* get one byte at a time from the eeprom and compare to shadow copy, if it differs write the shadow copy to EEPROM */
	for (i = 0; i < (length - 1); i++)
	{
		Serial.printf("%d \t %d\r\n", i, config.bytes[i]);
	}
	//update();
}

/**
 * @brief Get a reference to the configuration data
 *
 * The obtained reference is intentionally read only
 * Use this for normally access
 */
const config_nv_t* ConfigClass::getReference(void)
{
	return &config.nv;
}

/**
 * @brief Get a reference to the configuration data
 *
 * The obtained reference has read and write access
 *
 * @warning Use this only in calibration and set up routines
 *
 * @return A read and write pointer to the configuration data;
 */
config_nv_t* ConfigClass::getWriteableReference(void)
{
	return &config.nv;
}

/**
 * @brief Calculate the checksum
 */
uint8_t  ConfigClass::getChecksum(void)
{
	/* checksum - last byte in config memory */
	uint8_t sum = 0;
	size_t length = sizeof(config_t);
	int i;

	for (i = 0; i < (length - 1); i++)
	{
		sum += config.bytes[i];
	}
	return sum;
}


/**
 *	@brief Run sanity checks on the EEPROM content
 *
 *	If any value is deemed out of range it will be replaced with
 *		the corresponding default for the display type
 */
int ConfigClass::checkNV(void)
{
	int error = 0;
	const config_data_t* defaults;

	defaults = &config_defaults;

	// Top level failures - flag these before trying to continue
	if (config.nv.header.signature != CONFIGURE_SIGNATURE)
	{
		config.nv.header.signature = CONFIGURE_SIGNATURE;
		error = -1;
	}

	if (config.nv.header.version != CONFIGURE_VERSION)
	{
		config.nv.header.version = CONFIGURE_VERSION;
		error = -2;
	}

	if (config.nv.checksum != getChecksum())
	{
		error = -3;
	}

	// Check each config value for validity and replace with default if necessary

	/* check standby */
	if (config.nv.configuration.standby < 0
		|| config.nv.configuration.standby > 1)
	{
		config.nv.configuration.standby = defaults->standby;
		error = -4;
	}

	/* no real need to check colour or brightness... */
	/* empty */

	/* check mode */
	if (config.nv.configuration.mode < 0 
		|| config.nv.configuration.mode > 4)
	{
		config.nv.configuration.mode = defaults->mode;
		error = -5;
	}

	if (error < 0) // if we have altered contents, CRC needs recalculating
	{
		config.nv.checksum = getChecksum();
		Serial.println("EEPROM Modified to defaults");
	}

	save();	// Save any updates to NV

	return error;
}


ConfigClass Config;

