#define F_CPU 1000000

#include <avr/io.h>
#include <util/delay.h>

int main(void) {

	// Set port directions
	DDRB = 0b00111111;
	DDRD = 0b11111110;

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
		// Wait for tube capacitance to discharge...
		_delay_us(150);
		// Update values for the next tube
		index += 1;
		if(index == 6) {
			index = 0;
			rounds += 1;
			// Incrememt counter every so many-rounds
			if(rounds == 50) {
				rounds = 0;
				tubes[5] += 1;
				// Carry...
				if(tubes[5] == 10) { tubes[5] = 0; tubes[4] += 1; }
				if(tubes[4] == 10) { tubes[4] = 0; tubes[3] += 1; }
				if(tubes[3] == 10) { tubes[3] = 0; tubes[2] += 1; }
				if(tubes[2] == 10) { tubes[2] = 0; tubes[1] += 1; }
				if(tubes[1] == 10) { tubes[1] = 0; tubes[0] += 1; }
				if(tubes[0] == 10) { tubes[0] = 0; }
			}
		}
	}

	return 0;
}
