#define F_CPU 8000000

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "i2cmaster.h"

#define RTC 0xDE

// Array of digits to display on each tube
uint8_t tubes[6];

// Periodically fired interrupt (~16 kHz)
ISR(PCINT1_vect) {
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
			tube_index++;
			if(tube_index == 6) tube_index = 0;
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

uint8_t read_rtc(uint8_t address) {
	uint8_t result;
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(address);
	i2c_rep_start(RTC+I2C_READ);
	result = i2c_readNak();
	i2c_stop();
	return result;
}

void init_rtc() {
	uint8_t seconds_reg;
	// Start oscillator
	seconds_reg = read_rtc(0x00);
	seconds_reg |= 0b10000000;
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(0x00);
	i2c_write(seconds_reg);
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

void reset_seconds() {
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0x80);
	i2c_stop();
	tubes[4] = 0;
	tubes[5] = 0;
}

void increment_minutes() {
	uint8_t mins, tens, units;
	mins = read_rtc(0x01);
	units = mins & 0b00001111;
	tens = mins >> 4;
	units++;
	if(units == 10) {
		units = 0;
		tens++;
	}
	if(tens == 6) tens = 0;
	mins = units | (tens << 4);
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(0x01);
	i2c_write(mins);
	i2c_stop();
	update_minutes(tubes);
}

void increment_hours() {
	uint8_t hours, tens, units;
	hours = read_rtc(0x02);
	units = hours & 0b00001111;
	tens = (hours >> 4) & 0b00000011;
	units++;
	if(units == 10) {
		units = 0;
		tens++;
	}
	if(tens == 2 && units == 5) {
		units = 0;
		tens = 0;
	}
	hours = units | (tens << 4);
	i2c_start_wait(RTC+I2C_WRITE);
	i2c_write(0x02);
	i2c_write(hours);
	i2c_stop();
	update_hours(tubes);
}

uint8_t handle_buttons(uint8_t old_buttons) {
	uint8_t new_buttons, raw_buttons, count;
	count = 0;
	new_buttons = PINC | 0b11111000;
	_delay_ms(10);
	while(count < 5) {
		raw_buttons = PINC | 0b11111000;
		if(raw_buttons == new_buttons) {
			count++;
		} else {
			count = 0;
			new_buttons = raw_buttons;
		}
		_delay_ms(10);
	}
	if(new_buttons != old_buttons) {
		if(!(new_buttons & 0b00000001)) reset_seconds();
		if(!(new_buttons & 0b00000010)) increment_minutes();
		if(!(new_buttons & 0b00000100)) increment_hours();
	}
	return new_buttons;
}

int main() {

	uint8_t buttons = 0b00000111;

	// Set port directions
	DDRB = 0b00111111;
	DDRD = 0b11111110;
	PORTC = 0b00000111;
	// Start RTC oscillator
	i2c_init();
	init_rtc();

	update_seconds(tubes);
	update_minutes(tubes);
	update_hours(tubes);
	setup_interrupts();

	while(1) {
		update_seconds(tubes);
		update_minutes(tubes);
		update_hours(tubes);
		buttons = handle_buttons(buttons);
	}

	return 0;
}
