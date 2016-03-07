/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef Driving_H
#define Driving_H

// State definitions for use with the query function
typedef enum { Waiting2Drive, WireFollow, Turning, DisplaceFwdBwd, Sweeping} DrivingState_t ;

// Public Function Prototypes

ES_Event RunDrivingSM( ES_Event CurrentEvent );
void StartDrivingSM ( ES_Event CurrentEvent );

DrivingState_t QueryDrivingSM ( void );

#endif /*Driving_H */

