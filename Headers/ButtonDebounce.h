#ifndef ButtonService_H
#define ButtonService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum { Debouncing , Ready2Sample } ButtonState_t ;

// Public Function Prototypes

bool InitButtonDB ( uint8_t Priority );
bool CheckButtonEvents ( void );
bool PostButton( ES_Event ThisEvent );
ES_Event RunButtonDB( ES_Event ThisEvent );


#endif
