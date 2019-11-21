// config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

/** @brief The expected value of the signature */
#define CONFIGURE_SIGNATURE		(0xDEADBEEF)

/** @brief Version of this configuration format */
#define CONFIGURE_VERSION		(0x01)

/** @brief Size of EEPROM structure */
#define CONFIGURE_SIZE			14


typedef struct
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
}rgb_t;

typedef struct
{
	uint32_t signature;		// 4 bytes
	uint8_t version;		// 1 byte
} config_header_t;

typedef struct
{
	uint8_t standby;		// 1 byte
	uint8_t brightness;		// 1 byte
	rgb_t colour;		// 3 bytes
	uint8_t mode;		// 1 bytes
	uint8_t reserved[2];	// 2 bytes
} config_data_t;

typedef struct
{
	config_header_t header;					// 5 bytes
	config_data_t configuration;			// 8 bytes
	uint8_t checksum;						// 1 bytes
}config_nv_t;

typedef union
{
	uint8_t bytes[CONFIGURE_SIZE];
	config_nv_t nv;							// 22 bytes total
} config_t;



const config_data_t config_defaults = {
	.standby = 1,
	.brightness = 150,
	.colour = {255, 255, 255},
	.mode = 0 // (COLOUR)
};


class ConfigClass
{
 protected:
	static config_t config;
	

 public:
	int init();
	void update();
	void save();
	const config_nv_t* getReference(void);
	config_nv_t* getWriteableReference(void);
	uint8_t  getChecksum(void);

private:
	int checkNV(void);

	/* add get/set functions */
};

extern ConfigClass Config;

#endif

