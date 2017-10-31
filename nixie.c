#define F_CPU 8000000

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "i2cmaster.h"

#define RTC 0xDE

// Array of digits to display on each tube
volatile uint8_t tubes[6] = {0,2,3,4,5,6};


// PORTB values for turning on individual anodes
const uint8_t anodes[6] = {
	0b00100000,
	0b00010000,
	0b00001000,
	0b00000100,
	0b00000010,
	0b00000001
};

// PORTD values for turning on individual cathodes
const uint8_t digits[10] = {
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

// Periodically fired interrupt (~16 kHz)
ISR(PCINT1_vect) {
	// Counts visits to this ISR
	static uint8_t frame_tick;
	// Currently active tube
	static uint8_t tube_index;
	switch(frame_tick) {
		case 0:
			// Turn on cathode for the active tube
			PORTB = anodes[tube_index];
			break;
		case 30:
			// Blank all anodes
			PORTB = 0x00;
			break;
		case 35:
			// Update cathode values for the next tube
			tube_index = tube_index + 1;
			if(tube_index == 6) {
				tube_index = 0;
			}
			PORTD = digits[tubes[tube_index]];
			break;
		case 40:
			frame_tick = 255; // Increment below will roll over to zero
			break;
		default:
			break;
	}
	frame_tick++;
}

void init_rtc() {

	// Start oscillator
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0x80);
	i2c_stop();

	// Enable 4kHz squarewave output
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(0x07);
	i2c_write(0x42);
	i2c_stop();
}

void setup_interrupts() {
	PCICR = _BV(PCIE1);	// Enable pin change interrupt 1
	PCMSK1 = _BV(PCINT11);	// Set mask
	sei();			// Enable ints
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

void update_hours(uint8_t *tubes) {
	uint8_t time;
	time = read_rtc(0x02);
	tubes[1] = (uint8_t) (time & 0x0F);
	tubes[0] = (uint8_t) ((time >> 4) & 0x03);
}

int main() {

	// Set port directions
	DDRB = 0b00111111;
	DDRD = 0b11111110;

	// Start RTC oscillator
	i2c_init();
	init_rtc();

//	update_seconds(tubes);
//	update_minutes(tubes);
//	update_hours(tubes);
//	tubes[5] = 1;
	setup_interrupts();

	while(1) {
		tubes[3] ++;
		_delay_ms(1000);
//		if(tubes[5] == 10) tubes[5] = 0;
		//update_seconds(tubes);
		//update_minutes(tubes);
//		update_hours(tubes);
	}

	return 0;
}
