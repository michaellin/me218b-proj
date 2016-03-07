/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef GameService_H
#define GameService_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Waiting4Campaign, Campaigning_StopAtCity, Campaigning_DontStopAtCity, ShootingInCampaign,Campaigning_CapturingCity } GameServiceState_t ;


// Public Function Prototypes

ES_Event RunGameServiceSM( ES_Event CurrentEvent );
void StartGameServiceSM ( ES_Event CurrentEvent );
GameServiceState_t QueryGameServiceSM ( void );
bool GetCampaignState( void);

#endif /*SHMTemplate_H */

