/**
 * EECE 387 Example Code
 * AVR Standard I/O over UART: Universal Asynchronous Receiver / Transmitter 
 *
 * Allows you to use printf/scanf using your AVR and a USB-to-serial converter,
 * like the Bus Pirate. If you're about to use this to gather debug information,
 * you might want to see if the Logic Analyzer isn't a better fit to your needs.
 *
 * Simple sample code.
 */

#include <util/delay.h>
#include "uart/stdio.h"

int main() {

  unsigned x = 0;

  //You'll need to call this function before you can use
  //printf and scanf via the Bus Pirate.
  set_up_stdio_over_serial(); 

  while(1) {
    printf("I've sent this message %d times.\n", x++);
    _delay_ms(1000);
  }

}
