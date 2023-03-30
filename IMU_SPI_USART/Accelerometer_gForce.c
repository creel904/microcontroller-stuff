/*

 Name:          Thomas Creel
 Description:   uses lsm accelerometer to measure g forces

 */ 

/********************************DEPENDENCIES**********************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include "spi.h"
#include "lsm6ds3.h"
#include "lsm6ds3_registers.h"
#include "usart.h"

/*****************************FUNCTION DEFINITIONS*****************************/

volatile uint8_t accel_flag = 0;

int main(void)
{
    spi_init();
    
    interrupt_init();
    
    lsm6ds3_init();
    
    // enable ll interrupts globally
    //PMIC.CTRL = 0b00000001;
    PMIC.CTRL = PMIC_LOLVLEN_bm;
    
    sei();
    
    usartd0_init();
    
    uint8_t xyz_data[6];
    
    while(1)
    {
        if(accel_flag)
        {
            // load from register using lsmread
            xyz_data[0] = lsm6ds3_read(OUTX_L_XL);
            xyz_data[1] = lsm6ds3_read(OUTX_H_XL);
            xyz_data[2]  = lsm6ds3_read(OUTY_L_XL);
            xyz_data[3] = lsm6ds3_read(OUTY_H_XL);
            xyz_data[4]  =  lsm6ds3_read(OUTZ_L_XL);
            xyz_data[5] =  lsm6ds3_read(OUTZ_H_XL);
            
            usartd0_out_string((const char*) xyz_data);
    
            accel_flag = 0;
        }
    }
    
    while(1);
    
    return 0;
}


void spi_init(void)
{
    PORTF.OUTSET = SS_bm;                               // SS idles high in mode3
    PORTF.DIRSET = (SS_bm | MOSI_bm | SCK_bm);          // SS, MOSI, SCK outputs
    PORTF.DIRCLR = (MISO_bm);                           // MISO input
    
    // DORD is pin 5, defaulted to zero = MSB first, prescaler 4 gives 8MHz frequency
    SPIF.CTRL   =   SPI_PRESCALER_DIV4_gc           |   
                        SPI_MASTER_bm               |   
                        SPI_MODE_3_gc               |   
                        SPI_ENABLE_bm;
}

void spi_write(uint8_t data)
{
    /* Write to the relevant DATA register. */
    SPIF.DATA = data;

    /* Wait for relevant transfer to complete. */
    while(!(SPIF.STATUS & (1 << 7)));
}

uint8_t spi_read(void)
{
  /* Write some arbitrary data to initiate a transfer. */
  SPIF.DATA = 0x37;

  /* Wait for relevant transfer to be complete. */
  while(!(SPIF.STATUS & (1 << 7)));

    /* After the transmission, return the data that was received. */
    return SPIF.DATA;
}

void lsm6ds3_write(uint8_t reg_addr, uint8_t data)
{
    // enable slave (pull ss low)
    PORTF.OUTCLR = SS_bm;
    
    // enable the lsm6ds3 via the relevant chip select signal
    uint8_t var2 = (reg_addr | LSM6DS3_SPI_WRITE_STROBE_bm);
    
    // write desired address of lsm
    spi_write(var2);
    
    // write data to lsm
    spi_write(data);
    
    // disable slave (pull ss high)
    PORTF.OUTSET = SS_bm;
    
}

// returns a single byte of data from an LSM6SD3 register
// associated with the address `reg_addr`
uint8_t lsm6ds3_read(uint8_t reg_addr)
{
    PORTF.OUTCLR = SS_bm;           // enables cs/ss. idles high, active low
    
    uint8_t var3 = (reg_addr | LSM6DS3_SPI_READ_STROBE_bm);
    
    spi_write(var3);
    
    uint8_t var4 = spi_read();
    
    PORTF.OUTSET = SS_bm;           // disable ss. set it to idling high.
    
    return var4;
}

void interrupt_init(void)
{
    // port c int 1 mask select pin 6 as trigger the interrupt
    // PORTC.INT0MASK = 0b01000000;
    PORTC.INT0MASK = PIN6_bm;
    
    // set external interrupt pin as an input
    // PORTC.DIRCLR = 0b01000000;
    PORTC.DIRCLR = PIN6_bm;
    
    // set as low level priority interrupt
    //PORTC.INTCTRL = 0b00000001;
    PORTC.INTCTRL = PORT_INT0LVL_LO_gc;
    
    // set interrupt to trigger when PC6 at rising edge
    //PORTC.PIN6CTRL = 0b00000001;
    PORTC.PIN6CTRL = PORT_ISC_RISING_gc;
}

void lsm6ds3_init(void)
{
    // reset LSM6DS3 by setting CTRL3 bit 0 (SW_RESET) to 1. also, keep bit 2 (IF_INC) its default value of 1
    lsm6ds3_write(CTRL3_C, 0b00000101);
    
    // configure CTRL1_XL, CTRL9_XL, INT1_CTRL 
    lsm6ds3_write(CTRL9_XL, 0b00111000);            // enable X, Y, Z
    lsm6ds3_write(CTRL1_XL, 0b01010000);            // full-scale selection: 00 (+2g). 208Hz output data rate: 0101 (208 Hz)
    lsm6ds3_write(INT1_CTRL, 0b00000001);           // accelerometer set
}

ISR(PORTC_INT0_vect)
{
    // set Interrupt 1 Flag
    //PORTC.INTFLAGS = 0b00000001;
    PORTC.INTFLAGS = PORT_INT0IF_bm;
    
    accel_flag = 1;
}



void usartd0_init(void)
{
  /* Configure relevant TxD and RxD pins. */
    PORTD.OUTSET = PIN3_bm;
    PORTD.DIRSET = PIN3_bm;
    PORTD.DIRCLR = PIN2_bm;

  /* Configure baud rate. *//* At 2 MHz SYSclk, 5 BSEL, -6 BSCALE corresponds to 115200 bps */
    USARTD0.BAUDCTRLA = (uint8_t)BSEL;
    USARTD0.BAUDCTRLB = (uint8_t)((BSCALE << 4)|(BSEL >> 8));

  /* Configure remainder of serial protocol. */
  /* (In this example, a protocol with 8 data bits, no parity, and
   *  one stop bit is chosen.) */
    USARTD0.CTRLC = (USART_CMODE_ASYNCHRONOUS_gc |
                     USART_PMODE_DISABLED_gc     |
                     USART_CHSIZE_8BIT_gc)       &
                    ~USART_SBMODE_bm;

  /* Enable receiver and/or transmitter systems. */
    USARTD0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

  /* Enable interrupt (optional). */
    /* USARTD0.CTRLA = USART_RXCINTLVL_MED_gc; */
}

void usartd0_out_char(char c)
{
    while(!(USARTD0.STATUS & USART_DREIF_bm));
    USARTD0.DATA = c;
}

void usartd0_out_string(const char * str)
{
    for(uint8_t i = 0; i < 6; i++)
    {
        usartd0_out_char(str[i]);
    }
    
    //while(*str) usartd0_out_char(*(str++));
}

/***************************END OF FUNCTION DEFINITIONS************************/