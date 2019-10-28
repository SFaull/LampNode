// 
// 
// 

#include "nv.h"
#include "led.h"
#include <EEPROM.h>

void NvClass::init()
{

	/* Initialise EEPROM */
	EEPROM.begin(512);
	/*
	getColourFromMemory();
	setColourTarget(target_colour[0], target_colour[1], target_colour[2]);
	setTheMode((Modes)readEEPROM(MEM_MODE));

	if (readEEPROM(MEM_STANDBY) == 0)
	{
		setStandby(false);
		Serial.println("STANDBY FALSE");
	}
	else
	{
		setStandby(true);
		Serial.println("STANDBY TRUE");
	}

	brightness = readEEPROM(MEM_BRIGHTNESS);
	*/
}

void NvClass::writeEEPROM(int address, int val)
{
	if ((address < 512) && (address >= 0)) // make sure we are in range
	{
		EEPROM.write(address, val);
		EEPROM.commit();
	}
	else
	{
		Serial.print("Invalid EEPROM write address: ");
		Serial.println(address);
	}
}

int NvClass::readEEPROM(int address)
{
	if ((address < 512) && (address >= 0)) // make sure we are in range
	{
		int val;
		val = EEPROM.read(address);
		return val;
	}
	else
	{
		Serial.print("Invalid EEPROM read address: ");
		Serial.println(address);
	}
}


NvClass Nv;

