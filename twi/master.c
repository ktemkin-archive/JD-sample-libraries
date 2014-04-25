/*************************************************************************
* Title:    I2C master library using hardware TWI interface
* Author:   Peter Fleury <pfleury@gmx.ch>  http://jump.to/fleury
* File:     $Id: twimaster.c,v 1.3 2005/07/02 11:14:21 Peter Exp $
* Software: AVR-GCC 3.4.3 / avr-libc 1.2.3
* Target:   any AVR device with hardware TWI 
* Usage:    API compatible with I2C Software Library i2cmaster.h
**************************************************************************/
#include <inttypes.h>
#include <compat/twi.h>
#include "master.h"


/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
  #warning "You've attempted to use the TWI library without specifying an device clock speed (F_CPU). Assuming 16MHz."
  #define F_CPU 16000000UL
#endif

/* I2C clock in Hz */
#ifndef F_TWI
  #warning "You've attempted to use the TWI library without specifying an I2C clock speed (F_TWI). Assuming 100kHz."
  #define F_TWI  100000L
#endif


/**
 * -------------------------------------
 * Public API Functions
 * -------------------------------------
 */

/**
 * Performs a simple TWI write, and then returns the resultant status.
 *
 * @param data The byte of data to be transmitted...A
 * @return The I2C status value, which can be compared to the constants in <util/twi.h>.
 */ 
static uint8_t raw_twi_write(uint8_t data);

/**
 * Waits for any active TWI communications to complete.
 */  
static inline void wait_for_twi_operation_to_complete();


/**
 * -------------------------------------
 * Public API Functions
 * -------------------------------------
 */

/**
 * Sets up the I2C hardware interface, preparing the I2C hardware for
 * communications. Unless the TWI clock settings are adjusted, this
 * method need only be called once.
 *
 * @brief Sets up the I2C hardware interface.
 * @param void
 * @return none
 */ 
void set_up_twi_hardware()
{
  /* initialize TWI clock: 100 kHz clock, TWPS = 0 => prescaler = 1 */
 
  //Disable the TWI prescalar...
  TWSR = 0;

  //... and set up the I2C clock frequency.
  TWBR = ((F_CPU/F_TWI)-16)/2;  /* must be > 10 for stable operation */

}

/**
 * Begins a TWI packet intended to read from the provided address; or sends a repeated start condition.
 * (This transmits a start bit, an address bit, and a direction bit.)
 *
 * @param address The device's I2C address.
 * @retval 0 Returned if we can't communicate with the given device; e.g. if the device doesn't acknowledge communications.
 * @retval 1 Returned on success.
 */ 
uint8_t start_twi_read_from(uint8_t address) {
  return start_twi_communication(address, TWI_READ);
}

/**
 * Begins a TWI packet intended to write to the provided address; or sends a reapeated start condition.
 * (This transmits a start bit, an address bit, and a direction bit.)
 *
 * @param address The device's I2C address.
 * @retval 0 Returned if we can't communicate with the given device; e.g. if the device doesn't acknowledge communications.
 * @retval 1 Returned on success.
 */ 
uint8_t start_twi_write_to(uint8_t address) {
  return start_twi_communication(address, TWI_WRITE);
}

/**
 * Sends a TWI start condition.
 * 
 * @retval 0 Returned if we can't communicate with the given device; e.g. if the device doesn't acknowledge communications.
 * @retval 1 Returned on success.
 */ 
uint8_t send_twi_start_condition() {

  //Start the TWI connection.
  // The following Two Wire Control Register bits are set:
  //  TWEN:  Sets the Two Wire ENable bit, which must be written to start any TWI communication.
  //  TWSTA: Sets the Two Wire STArt bit, which specifies that we want to create a start condition.
  //  TWINT: Clears any existing Two Wire INTerrupts, allowing us to move forwad. 
  //         Confusingly enough, writing a '1' to this bit clears it.
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

  wait_for_twi_operation_to_complete();

	//Check to see if we were succesfully able to gain control of the bus,
  //indicated by the most significant five bits of the TW_STATUS register.
  uint8_t two_wire_status = TW_STATUS & 0xF8;
	return (two_wire_status == TW_START) || (two_wire_status == TW_REP_START);
}


/**
 * Begins an TWI communication to a given address in either read or write mode.
 * Sends a start bit, the target address, and a driection bit.
 *
 * You may prefer to use start_twi_read_from / start_twi_write_to.
 * 
 * @param address The device's I2C address.
 * @param direction The communication direction for the given TWI device. Should be either TWI_READ or TWI_WRITE.
 * @retval 0 Returned if we can't communicate with the given device; e.g. if the device doesn't acknowledge communications.
 * @retval 1 Returned on success.
 */
uint8_t start_twi_communication(uint8_t address, uint8_t direction)
{
  uint8_t twi_status;

  //Attempt to send a start condition. 
  //If we fail to take control of the bus, return false.
  if(!send_twi_start_condition()) {
    return false;
  }

  //Send the TWI device address and the data direction bit.
  twi_status = raw_twi_write(address << 1 | direction);

  //Return true if the resultant status indicatied that the Master Transmitted, SLAve ACKnowledged,
  //or if the status wwas Master Read, SLAve ACKnowledged.
	return (twi_status == TW_MT_SLA_ACK) || (twi_status == TW_MR_SLA_ACK);
}

/**
 * Performs a simple TWI write, and then returns the resultant status.
 *
 * @param data The byte of data to be transmitted...A
 * @return The I2C status value, which can be compared to the constants in <util/twi.h>.
 */ 
static uint8_t raw_twi_write(uint8_t data) {

	//Send the device's address and direction by populating the
  //Two Wire Data Register.
	TWDR = data;

  //Initiate the actual transmission.
  // The following Two Wire Control Register bits are set:
  //  TWEN:  Sets the Two Wire ENable bit, which must be written to start any TWI communication.
  //  TWINT: Clears any existing Two Wire INTerrupts, allowing us to move forwad. 
  //         Confusingly enough, writing a '1' to this bit clears it.
	TWCR = (1 << TWINT) | (1 << TWEN);

  wait_for_twi_operation_to_complete();

	// check value of TWI Status Register. Mask prescaler bits.
	return TW_STATUS & 0xF8;

} 

/**
 * Waits for any active TWI communications to complete.
 */  
static inline void wait_for_twi_operation_to_complete() {
	//Wait for the current operation to be finished, as indicated by the
  //Two Wire INTerrupt flag being set to '1'.
  while(!(TWCR & (1 << TWINT)));
}


/*************************************************************************
 Issues a start condition and sends address and transfer direction.
 If device is busy, use ack polling to wait until device is ready
 
 Input:   address and transfer direction of I2C device
*************************************************************************/
void i2c_start_wait(unsigned char address)
{
    uint8_t   twst;


    while ( 1 )
    {
	    // send START condition
	    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    
    	// wait until transmission completed
    	while(!(TWCR & (1<<TWINT)));
    
    	// check value of TWI Status Register. Mask prescaler bits.
    	twst = TW_STATUS & 0xF8;
    	if ( (twst != TW_START) && (twst != TW_REP_START)) continue;
    
    	// send device address
    	TWDR = address;
    	TWCR = (1<<TWINT) | (1<<TWEN);
    
    	// wail until transmission completed
    	while(!(TWCR & (1<<TWINT)));
    
    	// check value of TWI Status Register. Mask prescaler bits.
    	twst = TW_STATUS & 0xF8;
    	if ( (twst == TW_MT_SLA_NACK )||(twst ==TW_MR_DATA_NACK) ) 
    	{    	    
    	    /* device busy, send stop condition to terminate write operation */
	        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	        
	        // wait until stop condition is executed and bus released
	        while(TWCR & (1<<TWSTO));
	        
    	    continue;
    	}
    	//if( twst != TW_MT_SLA_ACK) return 1;
    	break;
     }

}/* i2c_start_wait */



/*************************************************************************
 Terminates the data transfer and releases the I2C bus
*************************************************************************/
void i2c_stop(void)
{
    /* send stop condition */
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	
	// wait until stop condition is executed and bus released
	while(TWCR & (1<<TWSTO));

}/* i2c_stop */


/*************************************************************************
  Send one byte to I2C device
  
  Input:    byte to be transfered
  Return:   0 write successful 
            1 write failed
*************************************************************************/
unsigned char i2c_write( unsigned char data )
{	
    uint8_t   twst;
    
	// send data to the previously addressed device
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);

	// wait until transmission completed
	while(!(TWCR & (1<<TWINT)));

	// check value of TWI Status Register. Mask prescaler bits
	twst = TW_STATUS & 0xF8;
	if( twst != TW_MT_DATA_ACK) return 1;
	return 0;

}/* i2c_write */


/*************************************************************************
 Read one byte from the I2C device, request more data from device 
 
 Return:  byte read from I2C device
*************************************************************************/
unsigned char i2c_readAck(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while(!(TWCR & (1<<TWINT)));    

    return TWDR;

}/* i2c_readAck */


/*************************************************************************
 Read one byte from the I2C device, read is followed by a stop condition 
 
 Return:  byte read from I2C device
*************************************************************************/
unsigned char i2c_readNak(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	
    return TWDR;

}/* i2c_readNak */
