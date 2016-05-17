// MAX7219 drivers common anode 8x 7-segment display so digits and segments are flipped
// Therefore MAX7219_REG_DIGIT0 holds segment A of all digits, MAX7219_REG_DIGIT1 holds segment B of all digits and so on.
//
// Pins
//
// PD0-PD7			4040 counter 8 low bits
// PC0-PC3			4040 counter 4 high bits
// A4				4040 reset, active high
// A5				or gate, measure on low
// 10				MAX7219 load
// 11,13			MAX7219 data & clock
//
// PC3 & PD2 have been swapped!

#include <SPI.h>
#include <util/delay.h>

// define max7219 registers
const byte MAX7219_REG_NOOP        = 0x0;
const byte MAX7219_REG_DIGIT0      = 0x1;
const byte MAX7219_REG_DIGIT1      = 0x2;
const byte MAX7219_REG_DIGIT2      = 0x3;
const byte MAX7219_REG_DIGIT3      = 0x4;
const byte MAX7219_REG_DIGIT4      = 0x5;
const byte MAX7219_REG_DIGIT5      = 0x6;
const byte MAX7219_REG_DIGIT6      = 0x7;
const byte MAX7219_REG_DIGIT7      = 0x8;
const byte MAX7219_REG_DECODEMODE  = 0x9;
const byte MAX7219_REG_INTENSITY   = 0xA;
const byte MAX7219_REG_SCANLIMIT   = 0xB;
const byte MAX7219_REG_SHUTDOWN    = 0xC;
const byte MAX7219_REG_DISPLAYTEST = 0xF;

const byte resetPin = A4; //4040 master reset, active high

const byte digits[] = {
	//ABCDEFG
	B01111110,  // 0
	B00110000,  // 1
	B01101101,	// 2
	B01111001,  // 3
	B00110011,	// 4
	B01011011,	// 5
	B01011111,  // 6
	B01110000,  // 7
	B01111111,  // 8
	B01111011,  // 9
};

void sendByte(const byte reg, const byte data) {    
	digitalWrite(10, LOW);
	SPI.transfer(reg);
	SPI.transfer(data);
	digitalWrite(10, HIGH); 
}

volatile uint16_t overflow = 0;

ISR(INT0_vect)
{
	// external interrupt on falling edge of PD2
	// 4040 counter has overflowed
	overflow++;
}

// the setup routine runs once when you press reset:
void setup() {                
	pinMode(10, OUTPUT);     
	pinMode(A5, OUTPUT);
	digitalWrite(A5, HIGH);

	SPI.begin();

	sendByte(MAX7219_REG_SCANLIMIT, 7);   // show all 7+1 digits
	sendByte(MAX7219_REG_DECODEMODE, 0);  // do not use binary coded decimal
	sendByte(MAX7219_REG_DISPLAYTEST, 0); // no display test

	// clear display
	for(byte col = 0; col < 8; col++)
		sendByte(col + 1, 0);

	sendByte(MAX7219_REG_INTENSITY, 15);  // character intensity: range: 0 to 15
	sendByte(MAX7219_REG_SHUTDOWN, 1);   // not in shutdown mode (ie. start it up)

	pinMode(resetPin, OUTPUT);
	digitalWrite(resetPin, LOW);

	// External Interrupt Control Register (EICRA)
	// Bits 0 and 1 set the trigger type:
	// 0 = low level
	// 1 = any logical chance
	// 2 = falling edge
	// 3 = rising edge
	EICRA &= 0xf0;	// clear bits 0-3
	EICRA |= 2;

	// External Interrupt Mask Register (EIMSK)
	// Bit 0 	Enable external interrupt 0 (INT0)
	// Bit 1	Enable external interrupt 1 (INT1)
	EIMSK |= 1; // enable INT0

	resetCounter();
}

void resetCounter() {
	digitalWrite(resetPin, HIGH);
	digitalWrite(resetPin, LOW);
}

uint16_t readCounter() {
	uint16_t cnt = (uint16_t)PIND | (((uint16_t)PINC & 0xf) << 8);

	// swap bits 2 and 11
	uint16_t bit2 = cnt & 4;
	uint16_t bit11 = cnt & 2048;
	cnt &= ~(4+2048);	// clear bits 2 and 11
	if(bit2)
		cnt |= 2048;
	if(bit11)
		cnt |= 4;

	return cnt;
}

uint32_t measureFrequency() {
	overflow = 0;

	// enable counting
	digitalWrite(A5, LOW);
	//resetCounter();

	// reset counter
	PORTC |= 16;
	PORTC &= ~16;

	// count how many times the counter overflows during 1 second interval
	// i.e. how many times the MSB changes from 1 to 0


	// 1 second delay
	/*
	for(uint16_t i = 0; i < 62500; i++) {
		// 256 nops = 256 cycles
		__asm__(
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		);
	}
	*/

	delay(1000);
	// 1 second delay v2

	// stop counting
	//digitalWrite(A5, HIGH);
	PORTC |= 32;		// set A5 high

	uint32_t freq = readCounter() + (uint32_t)overflow * 4096L;

	// delay(1000) does not take exactly 1000ms, so let's fudge the result
	// fudge factor computed empirically from a few crystals
	freq = (double)freq * 0.9999143407383538;

	//0.9999030094080874 // 1 MHz
	//0.9999180067234487 // 1 MHz
	//0.9996393998748198 // 14.31818 MHz
	//0.9999220060835254 // 20 MHz

	return freq;

}

void loop() {
	uint32_t n = measureFrequency();  

	// extract digits
	byte d0 = digits[(n/10000000L) % 10L];
	byte d1 = digits[(n/ 1000000L) % 10L];
	byte d2 = digits[(n/  100000L) % 10L];
	byte d3 = digits[(n/   10000L) % 10L];
	byte d4 = digits[(n/    1000L) % 10L];
	byte d5 = digits[(n/     100L) % 10L];
	byte d6 = digits[(n/      10L) % 10L];
	byte d7 = digits[(n          ) % 10L];

	// segments A is stored in MAX7219 reg 0
	// segments B is stored in MAX7219 reg 1
	// ...
	// decimal points go to MAX7219 reg 7
	sendByte(MAX7219_REG_DIGIT0, ((d7 & 64)<<1) | ((d0 & 64)   ) | ((d1 & 64)>>1) | ((d2 & 64)>>2) | ((d3 & 64)>>3) | ((d4 & 64)>>4) | ((d5 & 64)>>5) | ((d6 & 64)>>6));
	sendByte(MAX7219_REG_DIGIT1, ((d7 & 32)<<2) | ((d0 & 32)<<1) | ((d1 & 32)   ) | ((d2 & 32)>>1) | ((d3 & 32)>>2) | ((d4 & 32)>>3) | ((d5 & 32)>>4) | ((d6 & 32)>>5));
	sendByte(MAX7219_REG_DIGIT2, ((d7 & 16)<<3) | ((d0 & 16)<<2) | ((d1 & 16)<<1) | ((d2 & 16)   ) | ((d3 & 16)>>1) | ((d4 & 16)>>2) | ((d5 & 16)>>3) | ((d6 & 16)>>4));
	sendByte(MAX7219_REG_DIGIT3, ((d7 &  8)<<4) | ((d0 &  8)<<3) | ((d1 &  8)<<2) | ((d2 &  8)<<1) | ((d3 &  8)   ) | ((d4 &  8)>>1) | ((d5 &  8)>>2) | ((d6 &  8)>>3));
	sendByte(MAX7219_REG_DIGIT4, ((d7 &  4)<<5) | ((d0 &  4)<<4) | ((d1 &  4)<<3) | ((d2 &  4)<<2) | ((d3 &  4)<<1) | ((d4 &  4)   ) | ((d5 &  4)>>1) | ((d6 &  4)>>2));
	sendByte(MAX7219_REG_DIGIT5, ((d7 &  2)<<6) | ((d0 &  2)<<5) | ((d1 &  2)<<4) | ((d2 &  2)<<3) | ((d3 &  2)<<2) | ((d4 &  2)<<1) | ((d5 &  2)   ) | ((d6 &  2)>>1));
	sendByte(MAX7219_REG_DIGIT6, ((d7 &  1)<<7) | ((d0 &  1)<<6) | ((d1 &  1)<<5) | ((d2 &  1)<<4) | ((d3 &  1)<<3) | ((d4 &  1)<<2) | ((d5 &  1)<<1) | ((d6 &  1)   ));

	//delay(1);
}
