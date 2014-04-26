JD-sample-libraries
===================

Dirt-simple sample libraries for EECE 387. These libraries are intended to be minimalist samples-- they don't take advantage of more complex hardware features such as interrupt synchronization.


Libraries
-----------

- A <i>Two Wire Interface ("I2C") master</i> library, which allows simple control of an I2C device using a bus-pirate-like syntax. See <a href="http://ktemkin.github.io/JD-sample-libraries/master_8h.html">the documentation for <code>twi/master.h</code></a>, or the samples below.
- A <i>uart-over-stdio</i> library, which is conveneint for simple serial monitors. See <a href="http://ktemkin.github.io/JD-sample-libraries/stdio_8h.html">The documentation for <code>uart/stdio.h</code>, or the samples below.</a>


Example
-------------

A simple TWI interface using <a href="http://dangerousprototypes.com/bus-pirate-manual/i2c-guide/">Bus Pirate</a>-like syntax, and stdio over serial. (Compatible with Arduino boards!)

```C
#include "uart/stdio.h"
#include "twi/master.h"

int main() {

  uint8_t reading_low, reading_high;

  //Set up an I2c communication at 100kHz.
  set_up_twi_hardware(100000);

  //And take repeated light sensor readings.
  while(1) {
    perform_bus_pirate_twi_command("[ 0x72 0xAC [ 0x73 r s ]", &reading_low, &reading_high);
    printf("Sensor reading: %d\n", (reading_high << 8) | reading_low);
    _delay_ms(100);
  }
  
}

```

Samples
---------

For more detailed examples, see the top-level code files. Annotated source:

- <a href="http://ktemkin.github.io/JD-sample-libraries/sample__twi__tsl2561_8c.html"> TSL 2561 Demo
- <a href="http://ktemkin.github.io/JD-sample-libraries/sample__uart__stdio_8c.html"> Serial UART Demo


