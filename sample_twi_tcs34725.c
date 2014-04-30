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
 *  Sample code illustrating communications with the TSC-34725 via I2C,
 *  using a bus-pirate compatible I2C library.
 *
 *
 */

#include "twi/master.h"
#include "uart/stdio.h"

#include <util/delay.h>

/**
 * Simple data structure which defines a two-byte piece of data.
 *
 * This "union" allows us to access the two-byte data into two ways:
 * as a pair of low and high bytes (var.low and var.high), or as a
 * single 16-bit unsigned integer (var.full). 
 *
 * This is convenient for intepreting our light sensor data!
 */ 
union light_sensor_reading_union {

  uint16_t full;

  struct {
    uint8_t low;
    uint8_t high;
  };
};
typedef union light_sensor_reading_union light_sensor_reading;


/**
 * Small section of sample code, for the Atmega328p.
 */ 
int main() {

  uint8_t start_code, device_id;
  light_sensor_reading clear, red, green, blue;

  //Set up stdio over the device's UART.
  set_up_stdio_over_serial();

  //Set up the microcontrollers's I2C hardware, running at 100kHz.
  set_up_twi_hardware(100000);
  _delay_ms(1);

  //Enable the sensor's internal ADC.
  //Note that we're using almost exactly the same command as used when communicating
  //via Bus Pirate-- there's only one small difference: the last "r" has been replaced with an "s",
  //indicating that we expect no further data.
  perform_bus_pirate_twi_command("[ 0x52 0x80 0x03 [ 0x53 s ]", &start_code);

  //If the two LSBs of the start code were 0b11, we've started the device successfully!
  if(start_code == 0x03) {
    printf("Sensor enabled succesfully!\n");
  }

  //Read the device's ID. For this example, we'll use the special "w" syntax, a special form of "write"
  //which accepts the value to be transmitted as an argument. This allows convenient programmatic control
  //of values to be transmitted!
  perform_bus_pirate_twi_command("[ 0x52 w [ 0x53 s ]", 0x92, &device_id);
  printf("Read device ID: 0x%x\n", device_id);

  //Alternatively, one can implement the I2C communications manually, rather
  //than in bus pirate format. This is almost always faster!
  //
  //Realistically, you'd want these constants abstracted to somewhere else
  //in non-demonstation code.
  start_twi_write_to(0x29);
  send_via_twi(0x92);
  start_twi_read_from(0x29);
  device_id = read_via_twi(LastByte);
  end_twi_packet();
  printf("Re-read device ID: 0x%x\n", device_id);

  //And take repeated light sensor readings.
  while(1) {
    perform_bus_pirate_twi_command(
        "[ 0x52 0xB4 [ 0x53 rr rr rr rs  ]", 
        &clear.low, &clear.high,
        &red.low,   &red.high,
        &green.low, &green.high,
        &blue.low,  &blue.high);

    printf("Sensor readings (Clear, Red, Green, Blue): %5u, %5u, %5u, %5u\n", clear.full, red.full, green.full, blue.full);
    _delay_ms(100);
  }
  
  return 0;

}
