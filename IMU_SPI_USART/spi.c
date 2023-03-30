/*------------------------------------------------------------------------------
  spi.c --
  
  Description:
    Provides useful definitions for manipulating the relevant SPI
    module of the ATxmega128A1U. 

------------------------------------------------------------------------------*/

/********************************DEPENDENCIES**********************************/

#include <avr/io.h>
#include "spi.h"

/*****************************END OF DEPENDENCIES******************************/


/*****************************FUNCTION DEFINITIONS*****************************/


void spi_init(void)
{
  /* Initialize the relevant SPI output signals to be in an "idle" state.
   * Refer to the relevant timing diagram within the LSM6DS3 datasheet.
   * (You may wish to utilize the macros defined in `spi.h`.) */
    PORTF.OUTSET = SS_bm;                               // SS idles high
  

  /* Configure the pin direction of relevant SPI signals. */
    PORTF.DIRSET = (SS_bm | MOSI_bm | SCK_bm);      // SS, MOSI, SCK outputs
    PORTF.DIRCLR = (MISO_bm);                       // MISO input
    
    /* Set the other relevant SPI configurations. */    // DORD is pin 5, defaulted to zero = MSB first, prescaler 4 gives 8MHz frequency
    SPIF.CTRL   =   SPI_PRESCALER_DIV4_gc           |   
                        SPI_MASTER_bm               |   
                        SPI_MODE_0_gc               |   
                        SPI_ENABLE_bm;
}

void spi_write(uint8_t data)
{
    /* Write to the relevant DATA register. */
    SPIF.DATA = data;

    /* Wait for relevant transfer to complete. */
    while(!(SPIF.STATUS & (1 << 7)));

  /* In general, it is probably wise to ensure that the relevant flag is
   * cleared at this point, but, for our contexts, this will occur the 
   * next time we call the `spi_write` (or `spi_read`) routine. 
   * Really, because of how the flag must be cleared within
   * ATxmega128A1U, it would probably make more sense to have some single 
   * function, say `spi_transceive`, that both writes and reads 
   * data, rather than have two functions `spi_write` and `spi_read`,
   * but we will not concern ourselves with this possibility
   * during this semester of the course. */
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

/***************************END OF FUNCTION DEFINITIONS************************/
