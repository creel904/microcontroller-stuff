/*------------------------------------------------------------------------------
  lsm6ds3.c --
  

------------------------------------------------------------------------------*/

/********************************DEPENDENCIES**********************************/

#include <avr/io.h>
#include "spi.h"
#include "lsm6ds3.h"
#include "lsm6ds3_registers.h"

/*****************************END OF DEPENDENCIES******************************/


/*****************************FUNCTION DEFINITIONS*****************************/

// write a single byte of data, `data`, to the address
// `reg_addr`, which is meant to be associated with an
// LSM6DS3 register.
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
    PORTF.OUTCLR = SS_bm;
    
    uint8_t var3 = (reg_addr | LSM6DS3_SPI_READ_STROBE_bm);
    
    spi_write(var3);
    
    uint8_t var4 = spi_read();
    
    PORTF.OUTSET = SS_bm;
    
    return var4;
}
/***************************END OF FUNCTION DEFINITIONS************************/