#define F_CPU 1000000

#include <avr/io.h>
#include <util/delay.h>

#include "i2cmaster.h"

#define RTC 0xDE

void init_rtc() {

	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0x80);
	i2c_stop();

}

uint8_t read_rtc(uint8_t address) {
	uint8_t result;
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(address);
	i2c_rep_start(RTC+I2C_READ);
	result = i2c_readNak();
	i2c_stop();
	return result;
}

void update_seconds(uint8_t tubes[6]) {
	uint8_t time;
	time = read_rtc(0x00);
	tubes[5] = time & 0x0F;
	tubes[4] = (time >> 4) & 0x07;
}

void update_minutes(uint8_t tubes[6]) {
	uint8_t time;
	time = read_rtc(0x01);
	tubes[3] = time & 0x0F;
	tubes[2] = time >> 4;
}

void update_hours(uint8_t tubes[6]) {
	uint8_t time;
	time = read_rtc(0x02);
	tubes[1] = time & 0x0F;
	tubes[0] = (time >> 4) & 0x03;
}

int main() {

	// Set port directions
	DDRB = 0b00111111;
	DDRD = 0b11111110;

	// Start RTC oscillator
	i2c_init();
	init_rtc();

	// Counter for "rounds" through all six tubes
	int8_t rounds = 0;

	// PORTB values for turning on individual anodes
	int8_t anodes[6] = {
		0b00100000,
		0b00010000,
		0b00001000,
		0b00000100,
		0b00000010,
		0b00000001
	};

	// PORTD values for turning on individual cathodes
	int8_t digits[10] = {
		0b10111100,
		0b01111100,
		0b01111010,
		0b01110110,
		0b01101110,
		0b01011110,
		0b10011110,
		0b10101110,
		0b10110110,
		0b10111010
	};

	// Array of digits to display on each tube
	uint8_t tubes[6] = {0, 0, 0, 0, 0, 0};	

	// Index of active tube
	uint8_t index=0;

	while(1) {
		// Turn on cathode for the active tube
		PORTD = digits[tubes[index]];
		// Turn on anode for active tube
		PORTB = anodes[index];
		// Leave it on (i.e. display digit)
		_delay_ms(3);
		// Blank all anodes
		PORTB = 0x00;
		// Update values for the next tube
		index += 1;
		if(index == 6) {
			index = 0;
			rounds += 1;
			switch(rounds) {
				case 5:
					update_seconds(tubes);
					break;
				case 10:
					update_minutes(tubes);
					break;
				case 15:
					update_hours(tubes);
					break;
				case 20:
					rounds = 0;
					break;
				default:
					_delay_us(50);
					break;
			}
		}
	}

	return 0;
}
