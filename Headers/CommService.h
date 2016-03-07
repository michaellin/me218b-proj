#ifndef CommService_H
#define CommService_H

// State definitions for use with the query function
typedef enum { WaitingComm, SendingComm } CommState_t ;

// Public Function Prototypes

ES_Event RunCommSM( ES_Event CurrentEvent );
void StartCommSM ( ES_Event CurrentEvent );
bool PostCommSM( ES_Event ThisEvent );
bool InitCommSM ( uint8_t Priority );

#endif /*CommService_H */
