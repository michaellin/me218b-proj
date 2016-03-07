#ifndef LEDService_H
#define LEDService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"

// Public Function Prototypes

bool InitLEDService ( uint8_t Priority );
bool PostLEDService( ES_Event ThisEvent );
ES_Event RunLEDService( ES_Event ThisEvent );

// Public Prototypes
void SetLEDHigh(uint8_t);
void SetLEDLow(uint8_t);
void InitLEDStrip(void);

void SetRedLED( void);
void SetBlueLED( void);

#endif /* LEDService_H */
