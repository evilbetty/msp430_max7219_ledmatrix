// 
// Controlling an 8x8 Led Matrix with an MSP430 Launchpad and a MAX7219.
// This code is for an msp430g2553 and uses USCI_A for hardware SPI.
// 

#include "msp430.h"
#include <stdint.h>

// Uncomment if you are using a MAX7221, leave commented for MAX7219
//#define USE_MAX7221

// Port1 USCI pins and CS/Load pin
#define SPI_SIMO	BIT2
#define SPI_CLK		BIT4
#define SPI_CS		BIT3	// Load or /CS

// MAX7219 Register addresses
#define MAX_NOOP	0x00
#define MAX_DIGIT0	0x01
#define MAX_DIGIT1	0x02
#define MAX_DIGIT2	0x03
#define MAX_DIGIT3	0x04
#define MAX_DIGIT4	0x05
#define MAX_DIGIT5	0x06
#define MAX_DIGIT6	0x07
#define MAX_DIGIT7	0x08
#define MAX_DECODEMODE	0x09
#define MAX_INTENSITY	0x0A
#define MAX_SCANLIMIT	0x0B
#define MAX_SHUTDOWN	0x0C
#define MAX_DISPLAYTEST	0x0F


// Function prototypes
void spi_init();
void spi_max(unsigned char address, unsigned char data);


// 8x8 number font (not complete yet): number[digit][row]
const uint8_t number[10][8] = {
	{ 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c },	// 0
	{ 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c },	// 1
	{ 0x3c, 0x66, 0x06, 0x06, 0x0c, 0x18, 0x30, 0x7e },	// 2
	{ 0x3c, 0x66, 0x06, 0x1e, 0x1e, 0x06, 0x66, 0x3c },	// 3
};


// Program start
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD; 	// Disable WDT
	DCOCTL = CALDCO_1MHZ; 		// 1 Mhz DCO
	BCSCTL1 = CALBC1_1MHZ;

	// Setup Port1 pins
	P1DIR |= SPI_SIMO + SPI_CLK + SPI_CS;
	#ifdef USE_MAX7221
	P1OUT |= SPI_CS;		// MAX7221 Chip Select is inactive high
	#endif

	spi_init();			// Init USCI in SPI mode

	// Initialise MAX7219 with 8x8 led matrix
	spi_max(MAX_NOOP, 0x00); 	// NO OP (seems needed after power on)
	spi_max(MAX_SCANLIMIT, 0x07); 	// Enable all digits (always needed for current/8 per row)
	spi_max(MAX_INTENSITY, 0x08); 	// Display intensity (0x00 to 0x0F)
	spi_max(MAX_DECODEMODE, 0);	// No BCD decoding for led matrix
	// Clear all rows/digits
	spi_max(MAX_DIGIT0, 0);
	spi_max(MAX_DIGIT1, 0);
	spi_max(MAX_DIGIT2, 0);
	spi_max(MAX_DIGIT3, 0);
	spi_max(MAX_DIGIT4, 0);
	spi_max(MAX_DIGIT5, 0);
	spi_max(MAX_DIGIT6, 0);
	spi_max(MAX_DIGIT7, 0);
	spi_max(MAX_SHUTDOWN, 1); 	// Wake oscillators/display up


	// Ready to start displaying something!

	// Some vars
	uint8_t row, framecounter;

	// Loop forever
	while (1) 
	{
		// Loop through some frames
		for (framecounter=0; framecounter<4; framecounter++)
		{
			// Load all 8 row/digit registers with data from number[framecounter]
			for (row=0; row<8; row++)
			{
				spi_max(MAX_DIGIT0+row, number[framecounter][row]);
			}
			__delay_cycles(500000);	// Wait a bit before showing next frame
		}
	}

}


// Send 16 bits as: xxxxaaaadddddddd (ignore, address, data)
// and use active low Chip Select or active high Load pulse
// depending if the IC is a MAX7221 or MAX7219
void spi_max(uint8_t address, uint8_t data)
{
#ifdef USE_MAX7221
	P1OUT &= ~(SPI_CS);			// MAX7221 uses proper /CS
#endif
	UCA0TXBUF = address & 0b00001111;	// Send 4bit address as byte
	while (UCA0STAT & UCBUSY);		// Wait until done
	UCA0TXBUF = data;			// Send byte of data
	while (UCA0STAT & UCBUSY);
	P1OUT |= SPI_CS;			// /CS inactive or Load high
#ifndef USE_MAX7221
	P1OUT &= ~(SPI_CS);			// MAX7219 pulses Load high
#endif
}


// Enable harware SPI
void spi_init()
{
	UCA0CTL1 |= UCSWRST; 		// USCI in Reset State (for config)
	// Leading edge + MSB first + Master + Sync mode (spi)
	UCA0CTL0 = UCCKPH + UCMSB + UCMST + UCSYNC;
	UCA0CTL1 |= UCSSEL_2; 		// SMCLK as clock source
	UCA0BR0 |= 0x01; 		// SPI speed (same as SMCLK)
	UCA0BR1 = 0;
	P1SEL |= SPI_SIMO + SPI_CLK;	// Set port pins for USCI
	P1SEL2 |= SPI_SIMO + SPI_CLK;
	UCA0CTL1 &= ~UCSWRST; 		// Clear USCI Reset State
}


