#ifndef GPS_H_MH9WCYIG
#define GPS_H_MH9WCYIG

/**
	@file
	
	@brief Header file defining data structures and functions
*/

///Linked list of NEMA strings
struct _nmeaData {
	///Pointer to the NEMA string
	char *nmeaString;
	///Pointer to the next NEMA object
	struct _nmeaData * next; 
};

typedef volatile struct _nmeaData nmeaData;

///Struct to store fixes as addressible parts rather than a string - 26 bytes
struct _fixData {
	///N/S direction
	char latDir; 
	///E/W direction
	char lonDir;
	///Date as ddmmyy 
	uint32_t date;
	///Time as hhmmss UTC
	uint32_t time; 
	///First portion of the latitude decimal
	uint32_t lat1; 
	///Second portion of the latitude decimal
	uint32_t lat2; 
	///First portion of the longitude decimal
	uint32_t lon1; 
	///Second portion of the longitude decimal
	uint32_t lon2; 
};

typedef struct _fixData fixData;

//Functions
int parseGPRMC(char *item);

#endif /* end of include guard: GPS_H_MH9WCYIG */