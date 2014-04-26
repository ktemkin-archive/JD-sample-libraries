/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 *
 *  EECE 387 Example Code
 *  Simple TWI (Two Wire Interface) library for TWI-enabled AVRs.
 *  
 *  Kyle J. Temkin <ktemkin@binghamton.edu>
 *  Peter Fleury <pfleury@gmx.ch>
 *
 *  This library is based on the I2C Master Library by Peter Fleury (http://jump.to/fleury),
 *  which is in turn based on the contents of the AVR300.
 */


#ifndef _TWI_MASTER_H__
#define _TWI_MASTER_H__

#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <compat/twi.h>
#include <util/delay.h>

#ifdef DOXYGEN
/**
 @brief Software Library for TWI Masters

 Basic routines for communicating with TWI slave devices. This single master 
 implementation is limited to one bus master on the TWI bus. 

 This I2c library is implemented as a compact assembler software implementation of the TWI protocol 
 which runs on any AVR (i2cmaster.S) and as a TWI hardware interface for all AVR with built-in TWI hardware (twimaster.c).
 Since the API for these two implementations is exactly the same, an application can be linked either against the
 software TWI implementation or the hardware TWI implementation.

 Use an appropriately sized pull-up resistor on the SDA and SCL pin. For testing, a 4.7k resistor is usually fine.
 
 @par API Usage Example
  The following code shows typical usage of this library. See example sample_twi_tsl2561.

 @code
  #include "twi/master.h"
  #include <util/delay.h>

  int main() {

    uint8_t device_id;

    //Set up the microcontrollers's I2C hardware, running at 100kHz.
    set_up_twi_hardware(100000);
    _delay_ms(1);

    //Read the device's ID-- simple, but less optimal method.
    perform_bus_pirate_twi_command("[ 0x72 0x8A [ 0x73 r ]", &device_id);

    //Read the device's ID-- more optimal method.
    start_twi_write_to(0x39);
    send_via_twi(0x8A);
    start_twi_read_from(0x39);
    device_id = read_via_twi(LastByte);
    end_twi_packet();
    printf("Re-read device ID: 0x%x\n", device_id);

    //Wait forever.
    while(1);
    return 0;

  }
 @endcode

*/
#endif /* DOXYGEN */

/**@{*/

#include <avr/io.h>

/**
 * Defines a direction constant used for reading from TWI devices.
 */

/**
 * Defines one of the two TWI data directions.
 */ 
enum TWIDataDirection_enum {
  Read = 1,
  Write = 0
};
typedef enum TWIDataDirection_enum TWIDataDirection;


/**
 * Defines the possible TWi read modes.
 */ 
enum TWIReadMode_enum {
  RequestMore = 1,
  NonLastByte = 1,
  LastByte = 0
};
typedef enum TWIReadMode_enum TWIReadMode;


/**
 * Sets up the TWI hardware interface, preparing the TWI hardware for
 * communications. Unless the TWI clock settings are adjusted, this
 * method need only be called once.
 *
 * @brief Sets up the TWI hardware interface.
 * @param uint32_t The clock speed for the TWI interface, in Hz.
 * @return none
 */ 
void set_up_twi_hardware(uint32_t i2c_clock_speed);

/**
 *
 * Begins a TWI packet intended to read from the provided address.
 * (This transmits a start bit, an address bit, and a direction bit.)
 *
 * This method should only be used for the first time sending a start bit.
 * To send a start bit in the middle of a packet, use restart_communication_as_read_from.
 *
 * @param address The device's TWI address.
 * @retval 0 Returned if we can't communicate with the given device; e.g. if the device doesn't acknowledge communications.
 * @retval 1 Returned on success.
 */ 
uint8_t start_twi_read_from(uint8_t address);

/**
 * Begins a TWI packet intended to write to the provided address.
 * (This transmits a start bit, an address bit, and a direction bit.)
 *
 * This method should only be used for the first time sending a start bit.
 * To send a start bit in the middle of a packet, use restart_communication_as_write_to.
 *
 * @param address The device's TWI address.
 * @retval 0 Returned if we can't communicate with the given device; e.g. if the device doesn't acknowledge communications.
 * @retval 1 Returned on success.
 */ 
uint8_t start_twi_write_to(uint8_t address);


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
uint8_t start_twi_communication(uint8_t address, TWIDataDirection direction);


/**
 * Attempts to start an TWI communication. If the device responds that it's
 * not available, retry until the device /is/ available.
 *
 * Use of this function is only appropriate in some limited circumstances,
 * but it is included as to be a complete implementation of the Master TWI library.
 */
void ensure_twi_communication(uint8_t address, TWIDataDirection direction);


/** 
 * Terminates TWI communication with the given bus. 
 */
void end_twi_packet();

 
/**
 * Sends a single byte via the TWI interface.
 *
 * @param data The data to be transmitted via SPI.
 * @return data True iff the sent data is succesfully acknolwedged by a device.
 */ 
uint8_t send_via_twi(uint8_t data);

/**
 * Reads a single byte via TWI, and the requests more.
 *
 * @param read_mode Specifies the read mode for the given TWI communication, 
 *    which in turn specifies whether an additional byte is requested.
 * @return The byte read via TWI.
 */
uint8_t read_via_twi(TWIReadMode read_mode);


/**
 * Performs a given bus pirate command.
 *
 * Supports the following features:
 * {}, [], R/r, 0-255, 0b, 0h, &
 *
 * See: http://dangerousprototypes.com/bus-pirate-manual/i2c-guide/ 
 *
 * An additional command is also implemented:
 * s: Requests a single byte, and then responds with a NAK.
 *
 * Important note! You'll need to replace your last "r" with an "s",
 * or the TWI library will "lock up", as the AVR views the transmission
 * as being incomplete.
 *
 * For each read, a pointer should be provided to a uint8_t target.
 * For example:
 *
 *   uint8_t hello;
 *   perform_bus_pirate_twi_command("[ 0x72 0x80 0x03 [ 0x73 s ]", &hello);
 *
 * would read a single byte to the variable hello.
 *
 * @param command The bus pirate command, as a null-terminated string.
 * @param ...     A single uint8_t * for each read command.
 *
 */ 
uint8_t perform_bus_pirate_twi_command(const char * command, ...);


/**@}*/
#endif
