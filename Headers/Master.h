/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef Master_H
#define Master_H


// Public Function Prototypes

ES_Event RunMasterSM( ES_Event CurrentEvent );
void StartMasterSM ( ES_Event CurrentEvent );
bool PostMasterSM( ES_Event ThisEvent );
bool InitMasterSM ( uint8_t Priority );

#endif /*Master_H */
