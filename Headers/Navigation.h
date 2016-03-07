/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef Navigation_H
#define Navigation_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { DrivingState, SendingShootingAngleState, SendingShootingPositionState, 
WaitingForReloadState, Returning2PositionState,Returning2AngleState, SlowTurnInFindingWire, 
ChangeDirInFindingWire,StuckState_Backup,StuckState_Turn, WireLostDriving, WireLostSweeping} NavigationState_t ;


// Public Function Prototypes

ES_Event RunNavigationSM( ES_Event CurrentEvent );
void StartNavigationSM ( ES_Event CurrentEvent );
NavigationState_t QueryNavigationSM ( void );

#endif /*Navigation_H */

