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
  #warning "You've attempted to use the TWI library without specifying a device clock speed (F_CPU). Assuming 16MHz."
  #define F_CPU 16000000UL
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
void set_up_twi_hardware(uint32_t i2c_clock_speed)
{
  /* initialize TWI clock: 100 kHz clock, TWPS = 0 => prescaler = 1 */
 
  //Disable the TWI prescalar...
  TWSR = 0;

  //... and set up the I2C clock frequency.
  TWBR = ((F_CPU/i2c_clock_speed)-16)/2;  /* must be > 10 for stable operation */

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
  return start_twi_communication(address, Read);
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
  return start_twi_communication(address, Write);
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
uint8_t start_twi_communication(uint8_t address, TWIDataDirection direction)
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


/**
 * Attempts to start an I2C communication. If the device responds that it's
 * not available, retry until the device /is/ available.
 *
 * Use of this function is only appropriate in some limited circumstances,
 * but it is included as to be a complete implementation of the Master I2C library.
 */
void ensure_twi_communication(uint8_t address, TWIDataDirection direction)
{
    uint8_t twi_status;

    //Repeat until we're able to read succesfully.
    while(1)
    {
      
      //If we weren't able to start a TWI communication, retry.
    	if (!send_twi_start_condition()) {
        continue;  
      }
    
    	//Send the device's address, and the direction bit.
    	twi_status = raw_twi_write(address << 1 | direction);

      //If we recieved a Negative ACKnoledgement of either type 
      //(Master Transmit, SLAve Negative ACKnowledgement OR 
      // Master Receive, SLAve Negative ACKnowledgement), 
      // release the bus, and wait for the device to be ready.
    	if((twi_status == TW_MT_SLA_NACK ) || (twi_status == TW_MR_DATA_NACK)) {    	    
        //Abort the current TWI packet, and wait for the device to report that it's ready.
        end_twi_packet();     
    	}
      //Otherwise, mark the communication as complete, and stop.
      else {
        break; 
      }
    }
}


/** 
 * Terminates TWI communication with the given bus. 
 */
void end_twi_packet()
{
  //Terminate the active TWI packet by sending a stop condition.
  // The following Two Wire Control Register bits are set:
  //  TWEN:  Sets the Two Wire ENable bit, which must be written to start any TWI communication.
  //  TWSTO: Sets the Two Wire STOp bit, which specifies that we want to terminate our TWI communication.
  //  TWINT: Clears any existing Two Wire INTerrupts, allowing us to move forwad. 
  //         Confusingly enough, writing a '1' to this bit clears it.
  TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
  
  //Wait until we're no longer sending a Two Wire STOp condition.
  while(TWCR & (1 << TWSTO));
}


/**
 * Sends a single byte via the TWI interface.
 *
 * @param data The data to be transmitted via SPI.
 * @return data True iff the sent data is succesfully acknolwedged by a device.
 */ 
uint8_t send_via_twi(uint8_t data)
{	
  //Write the given byte to the TWI interface...
  uint8_t twi_status = raw_twi_write(data);

  //... and return true if we've recieved a Master Transmit DATA ACknowledgement.
	return (twi_status == TW_MT_DATA_ACK);
}



/**
 * Reads a single byte via TWI, and the requests more.
 *
 * @param read_mode Specifies the read mode for the given TWI communication, 
 *    which in turn specifies whether an additional byte is requested.
 * @return The byte read via TWI.
 */
uint8_t read_via_twi(TWIReadMode read_mode)
{
    //Ininitate a TWI read.
    // The following Two Wire Control Register bits are set:
    //  TWEN:  Sets the Two Wire ENable bit, which must be written to start any TWI communication.
    //  TWINT: Clears any existing Two Wire INTerrupts, allowing us to move forwad. 
    //         Confusingly enough, writing a '1' to this bit clears it.
    //  TWEA:  Sets the Two Wire Enable Acknowledge bit, which indicates that we're expecting more data.
    //
    //The lack of any other instruction (e.g. TWSTA) indicates that we're reading.
    TWCR = (1 << TWINT) | (1 << TWEN) | (read_mode << TWEA);
    wait_for_twi_operation_to_complete();

    return TWDR;
}

