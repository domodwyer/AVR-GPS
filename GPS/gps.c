/**
	@date 2013/07/08
	
	@brief GPS with the Teensy 2.0 micro-controller

	An application to read from an <a href='http://www.gtop-tech.com/en/product/MT3339_GPS_Module_04.html'>GlobalTop PA6H GPS module</a>.
	Supports NEMA parsing, and uses dynamic memory allocation and linked lists to provide a reliable state machine.
	
	Here's how it works;
		Init all the vars, start up the USART and wait for input.
		
		When the GPS module sends a character, the interrupt for USART rx is called,
		our code puts the character into a buffer (of size NMEA_BUFFER_LEN) and 
		checks to see if it was a newline? (this indicates the end of a NMEA sentance). 
		If it is, we now have a full NMEA sentance in our buffer, if not, carry on till we do 
 		(and don't overflow).
		
		Now we create a nmeaData instance in memory, set the nmeaString pointer to our buffer,
		and make ourselves a new buffer instance.
		
		Next we update the 'next' attribute of the nmeaData the main loop is using (gpsData) to
		point at our new sentence. This triggers the main loop into processing the previous item.
		
		We move gpsData along to the new instance, process and free the old instance, and wait to
		start again.
		
		In theory we should never miss a sentance as they're all buffered, though if we're too slow
		reading the nmeaData items we'll fill the memory and crash...
	
	Notes:
		If at any point the LED on pin 6 lights up, a malloc() has failed in the ISR and our
		memory is full, we're probably crashing at this point - at least we know why and can debug.
		
		We could move the filter to be inside the USART rx interrupt to save on malloc'd memory
		but it seems fine and really we want to keep the ISR as small (quick) as possible. It's 
		probably best this way round.
		
		I'm testing on a GlobalTop PA6H GPS module (using a MediaTek MT3339 chipset) which supports
		selective output, so you could easily filter unused NMEA sentences on the gps side, removing
		the need to filter on the mcu side. Enable only GPRMC in this case by sending: 
			$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29
		to the unit. To re-enable all NMEA strings send:
			$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28
			
		Update rate can also be changed in a similar way.
	
	Connections:
		+5v to +5v, GND to GND, TX on GPS to PD2(RX)
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h> 
#include <string.h> 
#include <stdlib.h>
#include <stdio.h>


#include "gps.h"
#include "../lib/common.h"
#include "../lib/usb_debug_only.h"
#include "../lib/usart/usart.h"

///Serial baudrate config
#define USART_BAUDRATE 9600

///Maximum length of a NEMA string for buffer allocation
#define NMEA_BUFFER_LEN 85

/*
		Globals
*/
///Pointer to our current RX buffer
volatile char *buffer;

///Counter for our current position in the buffer.
///We don't check that (buffer_index < bufferSize) because we know the NEMA limits
volatile uint8_t buffer_index = 0;

///Pointer to our linked list of NEMA strings
nmeaData *gpsData;

/**
	@brief Parse a NEMA string (of type GPRMC)
	@param *item Pointer to the NEMA string
*/
int parseGPRMC(char *item) {
	
	/*/ DEBUG - print NMEA string out
	for(int x=0;x<NMEA_BUFFER_LEN;x++) {
		usb_debug_putchar(item->nmeaString[x]);
		if(item->nmeaString[x] == '\n') break;
	}*/
	
	//DEBUG - output buffer
	char output[NMEA_BUFFER_LEN] = "";
	
	//Init fix data object
	fixData fix;
	memset(&fix, 0x00, sizeof(fixData));
	
	//Process the NMEA string - split it up around the commas and
	//pull out the values we're interested in
	uint8_t pos = 0;
	uint8_t decimalPos = 0;
	char *res = strtok(item, ",");	
	while(res != NULL) {
		pos++;
		switch (pos) {
			case 1:
				//header
				break;
			case 2:
				//time
				fix.time = atol(res);
				break;
			case 3:
				//valid fix? 0x41 is "A"
				if(res[0] != 0x41) return 1;
				break;
			case 4:
				//Copy latitude to fixData element
				for(decimalPos=0;decimalPos<strlen(res);decimalPos++) {
					//Look for "."
					if(res[decimalPos] == 0x2E)
						break;
				}
				
				//Check we found it - because we start counting decimalPos at 0, and string length from 1,
				//these will only match if we reached the end of the string.
				if(decimalPos == strlen(res)) return 1;
				
				//Read first half
				res[decimalPos] = 0x00;
				fix.lat1 = atol(res);
				fix.lat2 = atol(&res[decimalPos+1]);
				break;
			case 5:
				//Add North / South to fixData element
				memcpy(&fix.latDir, res, sizeof(char));
				break;
			case 6:
				//Copy longitude to fixData element
				for(decimalPos=0;decimalPos<strlen(res);decimalPos++) {
					//Look for "."
					if(res[decimalPos] == 0x2E)
						break;
				}
				
				//Check we found it
				if(decimalPos == strlen(res)) return 1;
				
				//Read first half
				res[decimalPos] = 0x00;
				fix.lon1 = atol(res);
				fix.lon2 = atol(&res[decimalPos+1]);
				break;
			case 7:
				//Add East / West to fixData element
				memcpy(&fix.lonDir, res, sizeof(char));
				break;
			case 8:
				//Speed in knots
				break;
			case 9:
				//Track angle
				break;
			case 10:
				//date
				fix.date = atol(res);
				break;
			case 11:
				//Magnetic variation
				break;
			case 12:
				//Magnetic variation N/S/E/W
				break;
			case 13:
				//checksum
				break;
			default:
				break;
		}
		
		//Next element
		res = strtok(NULL, ",");
	}
	
	//Here you would extend the application to use the GPS data as required
	
	//DEBUG - build output string
	sprintf(output, "%sDate: %lu (%lu)\n", output, fix.date, fix.time);
	sprintf(output, "%sLat: %lu.%lu%c\n", output, fix.lat1, fix.lat2, fix.latDir);
	sprintf(output, "%sLon: %lu.%lu%c\n", output, fix.lon1, fix.lon2, fix.lonDir);
	
	//DEBUG - print output string
	for(int i=0; i < NMEA_BUFFER_LEN; i++) {
		usb_debug_putchar(output[i]);
		if(output[i] == '\0') {usb_debug_putchar('\n');break;}
	}
	
	return 0;
}

/**
	@brief Start point for the application
*/
int main(void) {
	CPU_PRESCALE(CPU_16MHz);  // run at 16 MHz
	
	//Init linked list, global buffer
	gpsData = malloc(sizeof(nmeaData));
	gpsData->next = 0;
	buffer = malloc(sizeof(char)*NMEA_BUFFER_LEN);
	
	//Init hardware
	usb_init();
	_delay_ms(80);
	USARTinit(USART_BAUDRATE);
	
	while (1) {
		//New entry?
		if(gpsData->next != 0) {
			//There is! Move root along, process item
			cli();
				volatile nmeaData *item	= gpsData;
				gpsData = gpsData->next;
			sei();
			
			//Filter messages, only interested in GPRMC strings
			if(strncmp(item->nmeaString, "$GPRMC", 6) == 0)
				parseGPRMC(item->nmeaString);
			
			free((void *)item->nmeaString);
			free((void *)item);
		}
	}
}

/**
	@brief Handle USART interrupts
	This function is a time-critical parser of recieved USART data, and is called one character at a time.
*/
ISR(USART1_RX_vect) {
	uint8_t c;

	c = UDR1;
	buffer[buffer_index] = c;
	buffer_index++;
	
	if(c == '\n') {
		//We now have a full NMEA sentence
		//Get to the end of the linked list
		volatile nmeaData *item = gpsData;
		if(item != 0)
			while(item->next != 0)
				item = item->next;
		
		//Malloc a new item, move into it
		item->next = (nmeaData *)malloc(sizeof(nmeaData));
		
		//Memory full?
		if(item->next == NULL) {
			//Miss this message
			TEENSY_LED_ON;
			buffer_index = 0;
			return;
		}
			
		item = item->next;
		
		//Set the nmeaString pointer to our completed NMEA sentence, and malloc a new buffer
		item->nmeaString = buffer;
		buffer = malloc(sizeof(char)*NMEA_BUFFER_LEN);
		
		//Check we're not filling the memory
		if(buffer == NULL) {
			//Full! put the buffer pointer back...
			buffer = item->nmeaString;
			//Reset the index pointer
			buffer_index = 0;
			//Light up the PANIC-NOW led
			TEENSY_LED_ON;
			return;
		}
		
		//Set the next pointer to 0
		item->next = 0;
		
		//Reset position counter
		buffer_index = 0;
	}
}	
