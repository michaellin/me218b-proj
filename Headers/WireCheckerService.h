#ifndef WireCheckerService_H
#define WireCheckerService_H

#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// Public Function Prototypes

ES_Event RunWireCheckerService( ES_Event CurrentEvent );
bool PostWireCheckerService( ES_Event ThisEvent );
bool InitWireCheckerService ( uint8_t Priority );
void GetWireSensorVals( float returnArray[2]);
bool bothWiresFound (void);

#endif

