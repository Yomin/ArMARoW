// THIS FILE WAS GENERATED FROM FILE "radio.portmap" BY AVR-HALIB PORTMAP GENERATOR 0.1
// DO NOT EDIT THIS FILE CAUSE CHANGES MAY BE OVERWRITTEN. CHANGE "radio.portmap" INSTEAD!
#include "avr-halib/avr/portmap.h"
#pragma once

struct RadioPortMap		// portmap for atmega1281
{
	union
	{
		struct		// pin interrupt: d 4;
		{
			uint8_t __pad0 [0x29];
			uint8_t : 4;
			bool pin : 1;		// PIND (0x29), bit 4
			uint8_t : 7;
			bool ddr : 1;		// DDRD (0x2a), bit 4
			uint8_t : 7;
			bool port : 1;		// PORTD (0x2b), bit 4
		} interrupt;
		struct		// pin sleep: b 4;
		{
			uint8_t __pad0 [0x23];
			uint8_t : 4;
			bool pin : 1;		// PINB (0x23), bit 4
			uint8_t : 7;
			bool ddr : 1;		// DDRB (0x24), bit 4
			uint8_t : 7;
			bool port : 1;		// PORTB (0x25), bit 4
		} sleep;
		struct		// pin reset: b 5;
		{
			uint8_t __pad0 [0x23];
			uint8_t : 5;
			bool pin : 1;		// PINB (0x23), bit 5
			uint8_t : 7;
			bool ddr : 1;		// DDRB (0x24), bit 5
			uint8_t : 7;
			bool port : 1;		// PORTB (0x25), bit 5
		} reset;
		struct		// pin cs: b 0;
		{
			uint8_t __pad0 [0x23];
			bool pin : 1;		// PINB (0x23), bit 0
			uint8_t : 7;
			bool ddr : 1;		// DDRB (0x24), bit 0
			uint8_t : 7;
			bool port : 1;		// PORTB (0x25), bit 0
		} cs;
		struct		// pin sck: b 1;
		{
			uint8_t __pad0 [0x23];
			uint8_t : 1;
			bool pin : 1;		// PINB (0x23), bit 1
			uint8_t : 7;
			bool ddr : 1;		// DDRB (0x24), bit 1
			uint8_t : 7;
			bool port : 1;		// PORTB (0x25), bit 1
		} sck;
		struct		// pin mosi: b 2;
		{
			uint8_t __pad0 [0x23];
			uint8_t : 2;
			bool pin : 1;		// PINB (0x23), bit 2
			uint8_t : 7;
			bool ddr : 1;		// DDRB (0x24), bit 2
			uint8_t : 7;
			bool port : 1;		// PORTB (0x25), bit 2
		} mosi;
		struct		// pin miso: b 3;
		{
			uint8_t __pad0 [0x23];
			uint8_t : 3;
			bool pin : 1;		// PINB (0x23), bit 3
			uint8_t : 7;
			bool ddr : 1;		// DDRB (0x24), bit 3
			uint8_t : 7;
			bool port : 1;		// PORTB (0x25), bit 3
		} miso;
	};
};

