/****************************************************************************
 
  Header file for template ShootingService

 ****************************************************************************/

#ifndef ShootingService_H
#define ShootingService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { Waiting2ShootState, ShootingServoDownState, ShootingServoUpState, 
              CServoDownState, CServoUpState, CServoMiddleState } ShootingService_t ;


// Public Function Prototypes

bool InitShootingService ( uint8_t Priority );
bool PostShootingService( ES_Event ThisEvent );
ES_Event RunShootingService( ES_Event ThisEvent );
							

#endif /* ShootingService_H */

