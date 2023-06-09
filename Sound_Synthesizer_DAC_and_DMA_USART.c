/*
Name:			Thomas Creel
Description:	sound synthesizer with triangle and sine waveforms, as well as
				12 keys
								
*/ 

extern void clock_init(void);

#include <avr/io.h>
#include <avr/interrupt.h>


// 32 MHz, 9600 bps
#define BSCALE -4
#define BSEL 3317

void dma_init(void);
void dac_init(void);
void tcc1_init(void);
void analog_init(void);
void usartd0_init(void);
void usartd0_out_char(char c);
void usartd0_out_string(const char * str);

// for fun
void tcc0_init(void);


volatile uint8_t data = 0;
volatile uint8_t dataflag = 0;
volatile uint8_t waveflag = 0;

char keys[12] =
{
	'W', '3', 'E', '4', 'R', 'T', '6', 'Y', '7', 'U', '8', 'I'
};

uint16_t notes[12] =
{
	0x77, 0x70, 0x6A, 0x64, 0x5E, 0x59, 0x54, 0x4F, 0x4B, 0x47, 0x43, 0x3F
};

// Mary had a little lamb
// E, D, C, D, E, E, E
// D, D, D, E, E, E
// E, D, C, D, E, E, E
// E, D, D, E, D, C
uint16_t song[23] =
{
	0x5E, 0x6A, 0x77, 0x5E, 0x5E, 0x5E,
	0x6A, 0x6A, 0x6A, 0x5E, 0x5E,
	0x5E, 0x6A, 0x77, 0x5E, 0x5E, 0x5E,
	0x5E, 0x6A, 0x6A, 0x5E, 0x6A, 0x77
};

uint16_t sinewave[256] =
{
	0x800,0x832,0x864,0x896,0x8c8,0x8fa,0x92c,0x95e,0x98f,0x9c0,0x9f1,0xa22,0xa52,0xa82,0xab1,0xae0,
	0xb0f,0xb3d,0xb6b,0xb98,0xbc5,0xbf1,0xc1c,0xc47,0xc71,0xc9a,0xcc3,0xceb,0xd12,0xd39,0xd5f,0xd83,
	0xda7,0xdca,0xded,0xe0e,0xe2e,0xe4e,0xe6c,0xe8a,0xea6,0xec1,0xedc,0xef5,0xf0d,0xf24,0xf3a,0xf4f,
	0xf63,0xf76,0xf87,0xf98,0xfa7,0xfb5,0xfc2,0xfcd,0xfd8,0xfe1,0xfe9,0xff0,0xff5,0xff9,0xffd,0xffe,
	0xfff,0xffe,0xffd,0xff9,0xff5,0xff0,0xfe9,0xfe1,0xfd8,0xfcd,0xfc2,0xfb5,0xfa7,0xf98,0xf87,0xf76,
	0xf63,0xf4f,0xf3a,0xf24,0xf0d,0xef5,0xedc,0xec1,0xea6,0xe8a,0xe6c,0xe4e,0xe2e,0xe0e,0xded,0xdca,
	0xda7,0xd83,0xd5f,0xd39,0xd12,0xceb,0xcc3,0xc9a,0xc71,0xc47,0xc1c,0xbf1,0xbc5,0xb98,0xb6b,0xb3d,
	0xb0f,0xae0,0xab1,0xa82,0xa52,0xa22,0x9f1,0x9c0,0x98f,0x95e,0x92c,0x8fa,0x8c8,0x896,0x864,0x832,
	0x800,0x7cd,0x79b,0x769,0x737,0x705,0x6d3,0x6a1,0x670,0x63f,0x60e,0x5dd,0x5ad,0x57d,0x54e,0x51f,
	0x4f0,0x4c2,0x494,0x467,0x43a,0x40e,0x3e3,0x3b8,0x38e,0x365,0x33c,0x314,0x2ed,0x2c6,0x2a0,0x27c,
	0x258,0x235,0x212,0x1f1,0x1d1,0x1b1,0x193,0x175,0x159,0x13e,0x123,0x10a,0xf2,0xdb,0xc5,0xb0,
	0x9c,0x89,0x78,0x67,0x58,0x4a,0x3d,0x32,0x27,0x1e,0x16,0xf,0xa,0x6,0x2,0x1,
	0x0,0x1,0x2,0x6,0xa,0xf,0x16,0x1e,0x27,0x32,0x3d,0x4a,0x58,0x67,0x78,0x89,
	0x9c,0xb0,0xc5,0xdb,0xf2,0x10a,0x123,0x13e,0x159,0x175,0x193,0x1b1,0x1d1,0x1f1,0x212,0x235,
	0x258,0x27c,0x2a0,0x2c6,0x2ed,0x314,0x33c,0x365,0x38e,0x3b8,0x3e3,0x40e,0x43a,0x467,0x494,0x4c2,
	0x4f0,0x51f,0x54e,0x57d,0x5ad,0x5dd,0x60e,0x63f,0x670,0x6a1,0x6d3,0x705,0x737,0x769,0x79b,0x7cd
};

uint16_t trianglewave[256] =
{
	0x20,0x40,0x60,0x80,0xa0,0xc0,0xe0,0x100,0x120,0x140,0x160,0x180,0x1a0,0x1c0,0x1e0,0x200,
	0x220,0x240,0x260,0x280,0x2a0,0x2c0,0x2e0,0x300,0x320,0x340,0x360,0x380,0x3a0,0x3c0,0x3e0,0x400,
	0x420,0x440,0x460,0x480,0x4a0,0x4c0,0x4e0,0x500,0x520,0x540,0x560,0x580,0x5a0,0x5c0,0x5e0,0x600,
	0x620,0x640,0x660,0x680,0x6a0,0x6c0,0x6e0,0x700,0x720,0x740,0x760,0x780,0x7a0,0x7c0,0x7e0,0x800,
	0x81f,0x83f,0x85f,0x87f,0x89f,0x8bf,0x8df,0x8ff,0x91f,0x93f,0x95f,0x97f,0x99f,0x9bf,0x9df,0x9ff,
	0xa1f,0xa3f,0xa5f,0xa7f,0xa9f,0xabf,0xadf,0xaff,0xb1f,0xb3f,0xb5f,0xb7f,0xb9f,0xbbf,0xbdf,0xbff,
	0xc1f,0xc3f,0xc5f,0xc7f,0xc9f,0xcbf,0xcdf,0xcff,0xd1f,0xd3f,0xd5f,0xd7f,0xd9f,0xdbf,0xddf,0xdff,
	0xe1f,0xe3f,0xe5f,0xe7f,0xe9f,0xebf,0xedf,0xeff,0xf1f,0xf3f,0xf5f,0xf7f,0xf9f,0xfbf,0xfdf,0xfff,
	0xfdf,0xfbf,0xf9f,0xf7f,0xf5f,0xf3f,0xf1f,0xeff,0xedf,0xebf,0xe9f,0xe7f,0xe5f,0xe3f,0xe1f,0xdff,
	0xddf,0xdbf,0xd9f,0xd7f,0xd5f,0xd3f,0xd1f,0xcff,0xcdf,0xcbf,0xc9f,0xc7f,0xc5f,0xc3f,0xc1f,0xbff,
	0xbdf,0xbbf,0xb9f,0xb7f,0xb5f,0xb3f,0xb1f,0xaff,0xadf,0xabf,0xa9f,0xa7f,0xa5f,0xa3f,0xa1f,0x9ff,
	0x9df,0x9bf,0x99f,0x97f,0x95f,0x93f,0x91f,0x8ff,0x8df,0x8bf,0x89f,0x87f,0x85f,0x83f,0x81f,0x800,
	0x7e0,0x7c0,0x7a0,0x780,0x760,0x740,0x720,0x700,0x6e0,0x6c0,0x6a0,0x680,0x660,0x640,0x620,0x600,
	0x5e0,0x5c0,0x5a0,0x580,0x560,0x540,0x520,0x500,0x4e0,0x4c0,0x4a0,0x480,0x460,0x440,0x420,0x400,
	0x3e0,0x3c0,0x3a0,0x380,0x360,0x340,0x320,0x300,0x2e0,0x2c0,0x2a0,0x280,0x260,0x240,0x220,0x200,
	0x1e0,0x1c0,0x1a0,0x180,0x160,0x140,0x120,0x100,0xe0,0xc0,0xa0,0x80,0x60,0x40,0x20,0x0
};


int main(void)
{
	clock_init();
	dma_init();
	dac_init();
	tcc1_init();
	analog_init();
	
	PMIC_CTRL = PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
	sei();
	
	usartd0_init();
	
	
	// for fun
	tcc0_init();
	
	while(1)
	{
		// Depending on button press of 's' choose between sinewave and trianglewave
		if(dataflag)
		{
			dataflag = 0;
			// if 's' switch between sinewave and trianglewave
			if(data == 's')
			{
				// poll DMA status for when its ready to be reset
				while(!((DMA.INTFLAGS & DMA_CH1TRNIF_bm) >> 1));
				
				dma_init();

			}
			
			uint16_t ms50 = 1562;
			uint16_t ms125 = 3906;
			uint16_t ms250 = 7812;
			uint16_t ms500 = 15625;
			uint16_t ms1000 = 31250;
			
			for(uint8_t i = 0; i < 12; i++)
			{
				if(data == keys[i])
				{
					TCC1.PER = notes[i];
					
					TCC0.CNT = 0;
					TCC0.PER = ms250;
					
					while(!(TCC0.INTFLAGS & TC0_OVFIF_bm));
					TCC0.INTFLAGS |= TC0_OVFIF_bm;
					
					TCC1.PER = 0;
					
				}
			}
			
			/* JUST FOR FUN */
			// Plays mary had a little lamb
			
			if(data == 'Q')
			{
				for(int i = 0; i < 23; i++)
				{
					TCC0.CNT = 0;
					TCC0.PER = ms500;
				
					TCC1.PER = song[i];
					
					while(!(TCC0.INTFLAGS & TC0_OVFIF_bm));
					TCC0.INTFLAGS |= TC0_OVFIF_bm;
					
					TCC0.CNT = 0;
					TCC0.PER = ms125;
					if( i == 7 | i == 10)
					{
						TCC0.PER = ms250;
					}
					
					TCC1.PER = 0;
					
					while(!(TCC0.INTFLAGS & TC0_OVFIF_bm));
					TCC0.INTFLAGS |= TC0_OVFIF_bm;	
				}
				TCC1.PER = 0;
			}

		}
	}
}

void analog_init(void)
{
	// POWER_DOWN_L PC7, make false (drive high), for analog backpack prevent shutdown
	PORTC.OUTSET = PIN7_bm;
	
	// Set PC7 as an output
	PORTC.DIRSET = PIN7_bm;
}


void dma_init(void)
{
	if(waveflag == 0)
	{
		waveflag = 1;
	}
	else if(waveflag == 1)
	{
		waveflag = 0;
	}
	
	// Reset DMAC
	DMA.CTRL = DMA_RESET_bm;
	
	// Single burst, repeat block, 2 bursts per block
	DMA.CH1.CTRLA |=  DMA_CH_BURSTLEN_2BYTE_gc | DMA_CH_SINGLE_bm | DMA_CH_REPEAT_bm;

	// Reload source when done, increment address from src, releast dest, inc dest
	DMA.CH1.ADDRCTRL = DMA_CH_SRCRELOAD_BLOCK_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_BURST_gc | DMA_CH_DESTDIR_INC_gc;
	
	// CH1 triggers event system
	DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_EVSYS_CH1_gc;
	
	// Depending on button press of 's' choose between sinewave (0) and trianglewave (1)
	if(waveflag == 1)
	{
		// Total bytes in block transfer
		DMA.CH1.TRFCNT = (uint16_t)(sizeof(sinewave));
		
		// Configuring source address as sinewave location
		DMA_CH1_SRCADDR0 = (uint8_t)((uintptr_t)sinewave);
		DMA_CH1_SRCADDR1 = (uint8_t)((uintptr_t)sinewave >> 8);
		DMA_CH1_SRCADDR2 = (uint8_t)(((uint32_t)((uintptr_t)sinewave))>>16);
	}
	else if(waveflag == 0)
	{
		// Total bytes in block transfer
		DMA.CH1.TRFCNT = (uint16_t)(sizeof(trianglewave));
		
		// Configuring source address as sinewave location
		DMA_CH1_SRCADDR0 = (uint8_t)((uintptr_t)trianglewave);
		DMA_CH1_SRCADDR1 = (uint8_t)((uintptr_t)trianglewave >> 8);
		DMA_CH1_SRCADDR2 = (uint8_t)(((uint32_t)((uintptr_t)trianglewave))>>16);
	}
	
	// Configuring destination address as DAC DATA register
	DMA_CH1_DESTADDR0 = (uint8_t)((uintptr_t)&DACA_CH1DATA);
	DMA_CH1_DESTADDR1 = (uint8_t)((uintptr_t)&DACA_CH1DATA >> 8);
	DMA_CH1_DESTADDR2 = (uint8_t)(((uint32_t)((uintptr_t)&DACA_CH1DATA))>>16);
	
	// Enable DMA
	DMA.CH1.CTRLA |= DMA_CH_ENABLE_bm;
	DMA.CTRL |= DMA_ENABLE_bm;
}

void dac_init(void){
	// PA3 output
	PORTA.DIRSET = PIN3_bm;
	
	// Channel 1
	DACA.CTRLB =  DAC_CHSEL_SINGLE1_gc;
	
	// 2.5VREF
	DACA.CTRLC = DAC_REFSEL_AREFB_gc;
	
	// Enable DAC
	DACA.CTRLA =  DAC_CH1EN_bm | DAC_ENABLE_bm ;
}


void tcc1_init(void)
{
	// 32MHz, 0 Hz starting frequency, prescaler 1
	TCC1.PER =  0;
	
	// Prescaler
	TCC1.CTRLA = TC_CLKSEL_DIV1_gc;
	
	// Event System uses TCC1 overflow as signal
	EVSYS.CH1MUX = EVSYS_CHMUX_TCC1_OVF_gc;
}

void tcc0_init(void)
{
	// .3 second timer
	TCC0.PER = 18750;
	
	TCC0.CTRLA = TC_CLKSEL_DIV1024_gc;
}

void usartd0_init(void)
{	
  /* Configure relevant TxD and RxD pins. */
	PORTD.OUTSET = PIN3_bm;
	PORTD.DIRSET = PIN3_bm;
	PORTD.DIRCLR = PIN2_bm;

  /* Configure baud rate. */
	USARTD0.BAUDCTRLA = (uint8_t)BSEL;
	USARTD0.BAUDCTRLB = (uint8_t)((BSCALE << 4)|(BSEL >> 8));

  /* Configure remainder of serial protocol. */
  /* (In this example, a protocol with 8 data bits, no parity, and
   *  one stop bit is chosen.) */
	USARTD0.CTRLC =	(USART_CMODE_ASYNCHRONOUS_gc |
					 USART_PMODE_DISABLED_gc  	 |
					 USART_CHSIZE_8BIT_gc)       &
					~USART_SBMODE_bm;

  /* Enable receiver and/or transmitter systems. */
	USARTD0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

  /* Enable interrupt as medium level. */
	USARTD0.CTRLA = USART_RXCINTLVL_MED_gc; 
}

void usartd0_out_char(char c)
{
	while(!(USARTD0.STATUS & USART_DREIF_bm));
	USARTD0.DATA = c;
}

void usartd0_out_string(const char * str)
{
	while(*str) usartd0_out_char(*(str++));
}

ISR(USARTD0_RXC_vect)
{
	dataflag = 1;
	data = USARTD0.DATA;
	usartd0_out_char(data);
}
