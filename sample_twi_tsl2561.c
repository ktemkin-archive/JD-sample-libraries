/**
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
 *  ----
 *  
 *  Sample code illustrating communications with the TSL-2561 via I2C,
 *  using a modified version of Peter Fleury's I2C Master Library. 
 *
 *
 */

#include "twi/master.h"
#include <util/delay.h>

//The I2C device address and read/write select bits, for the Read 
//and Write bits, respectively.
static const uint8_t device_address = 0x39;


union light_sensor_reading_union {
  uint16_t full_word;
  struct {
    uint8_t high;
    uint8_t low;
  };
};
typedef union light_sensor_reading_union light_sensor_reading;


/**
 * Small section of sample code, for the Atmega328p.
 */ 
int main() {

  uint8_t start_code;
  light_sensor_reading reading;

  //Set up the microcontrollers's I2C hardware.
  set_up_twi_hardware();
  _delay_ms(1);

  //Turn on the sensor...
  //[ 0x72 0x80 [ 0x73 r ]
  start_twi_write_to(device_address);
  i2c_write(0x80);
  i2c_write(0x03);
  start_twi_read_from(device_address);
  start_code = i2c_readNak();
  i2c_stop();

  while(1) {

    //... and taken an ADC reading.
    //[ 0x72 0xAC [ 0x73 r r ]
    start_twi_write_to(device_address);
    i2c_write(0xAC);
    start_twi_read_from(device_address);
    reading.low  = i2c_readAck();
    reading.high = i2c_readNak();
    i2c_stop();

  }
  

  return 0;

}
