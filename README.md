#AVR-GPS

An example dive into the world of AVR micro-controllers - using the [Atmel AT90USB1286](http://www.atmel.com/devices/AT90USB1286.aspx) paired with a [GlobalTop PA6H](http://www.gtop-tech.com/en/product/MT3339_GPS_Module_04.html) GPS module.

The example code takes streaming [NEMA 0183](https://en.wikipedia.org/wiki/NMEA_0183) messages over the serial port using the AVR's hardware interrupts, building a linked list of messages to be parsed.

From there, the main routine decodes each message in the linked list into it's constituent parts (well, not all of them) and then does... nothing. That bit is up to you!

##Build Instructions
Install [CrossPack](https://www.obdev.at/products/crosspack/index.html) with either the official installer or homebrew (`brew install Caskroom/cask/crosspack-avr`). Don't forget to set your path (if installing using homebrew, you should `export PATH=$PATH:/usr/local/CrossPack-AVR/bin`).

- **Build:** run `make` to build the firmware
- **Flash:** edit the Makefile to reflect your avrdude settings and run `make flash`.
- **Clean:** run `make clean` to remove the compiled files from the working directory.

I use a cheap Chinese USBASP programmer purchased for next to nothing on a well known auction site - it works brilliantly!

##Improvements
Many! This is a brief PoC cobbled together as a learning exercise. Saying that, the first thing I would do would filter for messages starting [`$GPRMC`](http://aprs.gids.nl/nmea/#rmc) (positional messages) in the serial ISR routine, perhaps checking the buffer after we hit 6 characters and just waiting for `\n` if it doesn't match. Also the GPS module supports selective messages, but that seems too easy for a learning exercise.

Or you know, _actually check the checksum_. Bet it's there for a reason... It's not even difficult, a simple bitwise exclusive OR over the NMEA message!

##Debug Note
I'm using the HID [USB library](https://www.pjrc.com/teensy/usb_debug_only.html) provided by PJRC, and the accompanying [`hid_listen`](https://www.pjrc.com/teensy/hid_listen.html) program to grab debug messages sent over the USB port.
