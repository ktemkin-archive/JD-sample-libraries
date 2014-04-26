/**
 * EECE 387 Example Code
 * AVR Standard I/O over UART: Universal Asynchronous Receiver / Transmitter 
 *
 * Allows you to use printf/scanf using your AVR and a USB-to-serial converter,
 * like the Bus Pirate. If you're about to use this to gather debug information,
 * you might want to see if the Logic Analyzer isn't a better fit to your needs.
 */

#ifndef __UART_STDIO_H__
#define __UART_STDIO_H__

//If you do not specify F_CPU at the compile time (e.g. on the GCC command line),
//assume 8MHz.
#ifndef F_CPU
  #warning "You've attempted to use the UART library without specifying a device clock speed (F_CPU). Assuming 8MHz."
  #define F_CPU 8000000UL
#endif

//If you do not specify BAUD at the compile time (e.g. on the GCC command line),
//assume 19.2k-baud. Unfortunately, the AVR libraries don't allow us to set this dynamically.
#ifndef BAUD
  #define BAUD 115200
#endif

#include <avr/io.h>
#include <util/setbaud.h>
#include <stdio.h>


/**
 * Sets up serial communications at 19200 baud (a measure
 * of communications frequency) with 8-bit data packets, 
 * no parity, 1 stop bit, and no flow control; and sets up
 * special "standard pipes" (stdin/stderr/stdout) for use
 * with printf and scanf.
 *
 * Most of these match the defaults for the Bus Pirate.
 */ 
void set_up_stdio_over_serial();

/**
 * Sets up serial communications at 19200 baud (a measure
 * of communications frequency) with 8-bit data packets, 
 * no parity, 1 stop bit, and no flow control; but does
 * not set up any of the standard I/O functions.
 *
 * Most of these match the defaults for the Bus Pirate.
 */ 
void initialize_uart();

/**
 * Sends a single character directly over the UART.
 * If the UART is busy, this function will wait ('block')
 * until it is free.
 */
void send_via_uart(char c);

/**
 * Receives a single character over the UART.
 * If no characters have been receieved, this function will wait
 * until the device receives at least one.
 */
char receieve_via_uart();

#endif
