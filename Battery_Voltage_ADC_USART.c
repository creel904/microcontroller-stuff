/*

Name:			Thomas Creel
Description:	print the raw CdS cell data to be used with serialplot
				if press 'C' then poll Cds
				if press 'J' then poll J3 header
								
*/ 
#include <avr/io.h>
#include <avr/interrupt.h>

#define BSEL     (5)
#define BSCALE   (-6)

// global variables
volatile uint8_t conversion_flag = 0;
volatile uint8_t input_flag = 0;
volatile int16_t result = 0;
volatile float voltage = 0.0;
volatile uint8_t data = 0;

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

void adc_init(void)
{
	// set in0+ and in0- as inputs 
	PORTA.DIRCLR = PIN4_bm | PIN5_bm;
	
	// set cds+ and cds- as inputs
	PORTA.DIRCLR = PIN1_bm | PIN6_bm;
	
	// signed (set conmode), 12-bit right adjusted (00 res)
	// normal (not freerun) mode (0)
	ADCA.CTRLB = ADC_CURRLIMIT_NO_gc | ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
	
	// 2.5 V voltage reference, used AREFA PA0, also set bandgap enable since we arent using it
	ADCA.REFCTRL = (ADC_REFSEL_AREFB_gc | ADC_BANDGAP_bm);
	
	// interrupt when complete, set as low-level
	ADCA.CH0.INTCTRL = (ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_LO_gc);

	// select channel 0 in event system
	ADCA.EVCTRL = ADC_EVSEL_0123_gc | ADC_EVACT_CH0_gc | ADC_SWEEP_0_gc;
	

	// select differential input with gain
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	
	// muxpos = CdS+ PA1 
	// muxneg = CdS- PA6
	ADCA.CH0.MUXCTRL =	 ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN6_gc;
	
	// enable adc
	ADCA.CTRLA = ADC_ENABLE_bm;
}

void tcc0_init(void)
{
	// set .01 second timer with prescaler of 1024
	float mytimer = .01;
	TCC0.PER = ( mytimer * (2000000 / 1024)  );
	
	// overflow on tcc0
	EVSYS.CH0MUX = EVSYS_CHMUX_TCC0_OVF_gc;

	// choose prescaler
	TCC0.CTRLA = TC_CLKSEL_DIV1024_gc;
}

// set conversion_flag, clear tc0 overflow interrupt flag
ISR(ADCA_CH0_vect)
{
	result = ADCA.CH0.RES;
	
	voltage = (((float) result)*2.5)/2048.0;
	
	TCC0_INTFLAGS = TC0_OVFIF_bm;
	
	conversion_flag = 1;
}

//
ISR(USARTD0_RXC_vect)
{
	data = USARTD0.DATA;
	input_flag = 1;
}


void print_raw(void)
{
	uint8_t resultlow = result;
	uint8_t resulthigh = (result >> 8);
	
	// big endian is high byte first, then low byte
	usartd0_out_char(resulthigh);
	usartd0_out_char(resultlow);
}


int main(void)
{
	tcc0_init();
	adc_init();
	
	// enable up to medium level interrupts
	PMIC_CTRL = PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
	sei();
	
	usartd0_init();

	
	while(1)
	{
		if(input_flag)
		{
			// if 'C' use CdS cell
			if(data == 'C')
			{
				ADCA.CH0.MUXCTRL =	 ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN6_gc;
			}
			// if 'J' use J3 jumper
			else if(data == 'J')
			{
				ADCA.CH0.MUXCTRL =	 ADC_CH_MUXPOS_PIN4_gc | ADC_CH_MUXNEG_PIN5_gc;
			}
		}
	
		if(conversion_flag)
		{
			
			print_raw();
			
			conversion_flag = 0;
		}

	}
	
	return 0;
}

