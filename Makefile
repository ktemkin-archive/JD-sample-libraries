#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.*
#
# ----
# 
# Sample code illustrating communications with the TSL-2561 via I2C,
# using a modified version of Peter Fleury's I2C Master Library. 
#

#
# Basic setup. You should ensure these match your uC's configuration.
# 

#The name of the microcontroller you're working with.
DEVICE=atmega328p

#The CPU frequency you're working with.
#For an Arduino Uno, this should be 16MHz (16000000UL).
#For an atmega328p using an internal oscillator, this should be either 
#1MHz (1000000UL) if the CLKDIV8 fuse is set (default) or or 
#8MHz (1000000UL) if the fuse has not been set.
F_CPU=16000000UL

#The baud rate of any relevant serial transmissions. If you're using
#an Arduino Uno, this should be 115200. If you're using the bpserial
#scriptm, this should be 9600.
BAUD=115200UL

#
# Define the C compiler parameters, as used by the implicit rules for
# compiling C.
#
CC=avr-gcc
LDFLAGS=-mmcu=${DEVICE}
CFLAGS=-mmcu=${DEVICE} -DF_CPU=${F_CPU} -DBAUD=${BAUD} -ggdb  -Wall -Wextra -std=gnu11 -Os

#
# Compilation rules:
#

all: sample_twi_tcs34725.hex sample_twi_tsl2561.hex sample_uart_stdio.hex

#TWI Sample: TSL2561
sample_twi_tsl2561: sample_twi_tsl2561.o twi/master.o uart/stdio.o
sample_twi_tsl2561.o: sample_twi_tsl2561.c twi/master.h uart/stdio.h

#TWI Sample: TCS34725
sample_twi_tcs34725: sample_twi_tcs34725.o twi/master.o uart/stdio.o
sample_twi_tcw34725.o: sample_twi_tcs34725.c twi/master.h uart/stdio.h

#UART stdio sample
sample_uart_stdio: sample_uart_stdio.o uart/stdio.o
sample_uart_stdio.o: sample_uart_stdio.c uart/stdio.h

#Libraries
twi/master.o: twi/master.c twi/master.h
uart/stdio.o: uart/stdio.c uart/stdio.h

#General rules

%.hex: %
	avr-objcopy -O ihex $^ $^.hex
	echo
	avr-size --mcu=${DEVICE} --format=avr $^
	echo $^
	echo

clean:
	rm -f **/*.o **/*.hex *.o *.hex
	find . -perm +100 -type f -delete
