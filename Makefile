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

CC=avr-gcc
CFLAGS=-mmcu=atmega328 -DF_CPU=16000000L -ggdb 
LDFLAGS=-mmcu=atmega328

sample_twi_tsl2561.hex: sample_twi_tsl2561
	avr-objcopy -O ihex $^ $^.hex

clean:
	rm -f **/*.o **/*.hex *.o *.hex
	find . -perm +100 -type f -delete

sample_twi_tsl2561: sample_twi_tsl2561.o twi/master.o
sample_twi_tsl2561.o: sample_twi_tsl2561.c twi/master.h
twi/master.o: twi/master.c twi/master.h
