/****************************************************************************
 
  Header file for template CaptureCityServiec

 ****************************************************************************/

#ifndef CaptureCityService_H
#define CaptureCityService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { WaitingCapture, RequestingChange1, QueryingChange1, ReadingFirstResponse, 
              RequestingChange2, QueryingChange2, ReadingSecondResponse,CapturedState } CaptureCity_t ;


// Public Function Prototypes

bool InitCaptureCityService ( uint8_t Priority );
bool PostCaptureCityService( ES_Event ThisEvent );
ES_Event RunCaptureCityService( ES_Event ThisEvent );

uint8_t OurTeamColor(void); //Red = 0, blue = 1
//void TurnOnTeamColor( void );

							
#endif /* FSMTemplate_H */

