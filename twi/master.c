/*
 * EECE 387 Example Code
 * Simple TWI (Two Wire Interface) library for TWI-enabled AVRs.
 * 
 * Kyle J. Temkin <ktemkin@binghamton.edu>
 * Peter Fleury <pfleury@gmx.ch>
 *
 * This library is based on the I2C Master Library by Peter Fleury (http://jump.to/fleury),
 * which is in turn based on the contents of the AVR300.
 */

#include "master.h"


/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
  #warning "You've attempted to use the TWI library without specifying a device clock speed (F_CPU). Assuming 8MHz."
  #define F_CPU 8000000UL
#endif


/**
 * -------------------------------------
 * Private API Functions
 * -------------------------------------
 */

/**
 * Performs a simple TWI write, and then returns the resultant status.
 *
 * @param data The byte of data to be transmitted...A
 * @return The TWI status value, which can be compared to the constants in <util/twi.h>.
 */ 
static uint8_t raw_twi_write(uint8_t data);

/**
 * Waits for any active TWI communications to complete.
 */  
static inline void wait_for_twi_operation_to_complete();

/**
 * Given a TWI prescaler value, determines the amount of clock periods necessary to
 * reach a given frequency.
 */ 
static inline uint32_t clock_periods_from_prescaler(uint32_t target_speed, uint32_t prescaler) {
 
  //This function solves the following equation for TWBR (the Two Wire Bitrate Register value),
  //while avoiding floating point math. Solutions are truncated to the nearest integer.
  //
  //   target_speed = F_CPU / (16 + 2 * TWBR *(4 ^ prescaler))
  //
  //This is useful for computing the a prescalar / TWBR pair that allows a given clock frequency.
  return (F_CPU / ((2 << (2 * prescaler)) * target_speed)) - (8  >> (prescaler * 2));
}


/**
 * -------------------------------------
 * Public API Functions
 * -------------------------------------
 */

/**
 * Sets up the TWI hardware interface, preparing the TWI hardware for
 * communications. Unless the TWI clock settings are adjusted, this
 * method need only be called once.
 *
 * @brief Sets up the TWI hardware interface.
 * @param twi_clock_Speed The clock speed for the TWI interface, in Hz. 
 *    You must ensure that ((F_CPU / TWI_CLOCk_SPEED) - 16)/ 2 is less than 256, or this function won't work!
 * @return none
 */ 
void set_up_twi_hardware(uint32_t twi_bitrate)
{

  uint8_t  prescaler = -1;
  uint32_t clock_periods;

  //Compute an appropriate Two Wire prescaler value, which will
  //determine the rate of the main TWI clock speed.
  //
  //The TWI clock speed is defined as follows:
  //
  // F_CPU /(16 + 2 * TWBR *(4 ^ prescaler))
  //
  // Due to hardware register sizes, two condition have to be met:
  //  1) The prescaler has to be less than four, as to fit in a two-bit register; and
  //  2) The "constant" TWBR must be between 0-255, inclusive.
  //
  //  We'll iterate through prescaler values until we find one that allows us to satisfy condition #2.
  //  
  do {
    prescaler += 1;
    clock_periods = clock_periods_from_prescaler(twi_bitrate, prescaler); 
  } 
  while(clock_periods > 255 && prescaler < 3);

  //Apply the prescaler we've found.
  TWSR &= ~0x03;
  TWSR |= prescaler;

  //... and apply the Two Wire Bitrate Register (TWBR) value that we've determined.
  TWBR = clock_periods;

}

/**
 * Begins a TWI packet intended to read from the provided address; or sends a repeated start condition.
 * (This transmits a start bit, an address bit, and a direction bit.)
 *
 * @param address The device's TWI address.
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
 * @param address The device's TWI address.
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
 * @param address The device's TWI address.
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
 * @return The TWI status value, which can be compared to the constants in <util/twi.h>.
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
 * Attempts to start an TWI communication. If the device responds that it's
 * not available, retry until the device /is/ available.
 *
 * Use of this function is only appropriate in some limited circumstances,
 * but it is included as to be a complete implementation of the Master TWI library.
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

/**
 * Performs a given bus pirate command.
 *
 * Supports the following features:
 * {}, [], R/r, 0-255, 0b, 0h, &
 *
 * See: http://dangerousprototypes.com/bus-pirate-manual/i2c-guide/ 
 *
 * Two additional commands are also implemented:
 *  s: Reads a single byte, and then responds with a NAK.
 *  w: Transmits a single byte, provided as an argument. This allows programatic 
 *     control of transmission.
 *
 * Important note! You'll need to replace your last "r" with an "s",
 * or the TWI library will "lock up", as the AVR views the transmission
 * as being incomplete.
 *
 * For each read, a pointer should be provided to a uint8_t target.
 * For example:
 *
 * @code
 *   uint8_t hello;
 *   perform_bus_pirate_twi_command("[ 0x72 0x80 0x03 [ 0x73 s ]", &hello);
 * @endcode
 *
 * would read a single byte to the variable hello.
 *
 * For each _write_, a (non-pointer) uint8_t should be provided.
 * For example:
 *
 * @code
 *   perform_bus_pirate_twi_command("[ 0x72 0x80 w ]", 0x55);
 * @endcode 
 *
 * would send 0x72 (the address and write bit), then 0x80, and then 0x55.
 *
 *
 * @param command The bus pirate command, as a null-terminated string.
 * @param ...     A single uint8_t * for each read command, or a single uint8_t for each write.
 *
 */ 
uint8_t perform_bus_pirate_twi_command(const char * command, ...) {


  //Stores the current radix, which is decimal by default.
  uint8_t radix = 10;
  uint8_t to_transmit = 0, is_transmission = 0, read_count = 0;

  //Set up handling of the variadic arguments, which are used
  //for reading.
  va_list variadic_arguments;
  uint8_t * read_target = 0;
  va_start(variadic_arguments, command);

  //Process each argument in the command.
  for(; *command; ++command) {
    switch(*command) {
  
      //If we have an open brace, issue a TWI start condition.
      case '{':
      case '[':
        send_twi_start_condition();
        break;

      //If we have a close brace, issue a TWI stop condition.
      case '}':
      case ']':
        end_twi_packet();
        break;
   
      //If we have an 'x', switch the active radix to hex.
      case 'x':
        radix = 16;
        break;

      //If we have a 'b', switch the active radix to binary.
      case 'b':
        radix = 2;
        break;


      //Read a single byte from the TWI device, and then
      //send an acknowledgement, indicating that we expect further data.
      case 'r':
      case 'R':
      case 's':
      case 'S':
        {

          TWIReadMode read_mode;

          //If we're been provided the 's' command,
          //set the read mode to "last byte", which sends
          //a negative acknowledgement to the device.
          if(*command == 's' || *command == 'S') {
            read_mode = LastByte;
          } 
          //Otherwise, send an ACK, indiacting that we
          //would like additional data.
          else {
            read_mode = RequestMore;
          }
          
          //Get the address that the read is targeting...
          read_target = va_arg(variadic_arguments, uint8_t *);

          //... and then perform the read.
          *read_target = read_via_twi(read_mode);

          //... increment the number of total reads, and continue.
          ++read_count;
          break;
        }

      // Send a single byte via the TWI interface, which should be
      // provided as an argument.
      case 'w':
      case 'W':

        //Since we're writing, this is a transmission.
        is_transmission = 1;

        //Read the byte that should be transmitted...
        to_transmit = va_arg(variadic_arguments, unsigned int);

        //... and "roll" directly into the transmit case.

      //If we have a delimiter, handle any actions that have been queued.
      case ' ':
      case ',':
        if(is_transmission) {
          send_via_twi(to_transmit);
        }

        //Reset our state to the default.
        radix = 10;
        is_transmission = to_transmit = 0;
        break;

      //Delay 1us.
      case '&':
        _delay_us(1);
        break;

      //In all other cases, check to see if we have a piece of a literal.
      default:
        {
         
          uint8_t numeric_value;
          uint8_t is_lowercase_hex = (*command >= 'a') && (*command <= 'f') && (radix == 16);
          uint8_t is_uppercase_hex = (*command >= 'A') && (*command <= 'F') && (radix == 16);
          uint8_t is_decimal_digit = (*command >= '0') && (*command <= '9') && (radix >= 10);
          uint8_t is_valid_binary  = (*command >= '0') && (*command <= '1') && (radix ==  2);

          //If we have a number format, convert the number to a raw numeric value.
          if(is_lowercase_hex) {
            numeric_value = *command - 'a' + 10;
          }
          else if(is_uppercase_hex) {
            numeric_value = *command - 'A' + 10;
          }
          else if(is_decimal_digit || is_valid_binary) {
            numeric_value = *command - '0';
          } 
          //Otherwise, the value is nonsensical, and should be skipped.
          else {
            break; 
          }
        
          //Since we have a valid value to transmit, mark this as a transmission.
          //This is idempotent, so it doesn't matter if this is run multiple times.
          is_transmission = 1;

          //Move the existing number over by a single radix place, "making room"
          //to add the new number to the right...
          to_transmit *= radix;

          //... and then add the new value in the vacated spot.
          to_transmit += numeric_value;

          break;
        }
    }
  }

  //Halt parsing of varaidic arguments.
  va_end(variadic_arguments);

  //Return the number of reads performed.
  return read_count;
}
