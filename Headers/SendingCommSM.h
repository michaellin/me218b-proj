#ifndef SendingCommSM_H
#define SendingCommSM_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { WaitingInSend, SendingAllBytes} SendingState_t ;


// Public Function Prototypes

ES_Event RunSendingCommSM( ES_Event CurrentEvent );
void StartSendingCommSM ( ES_Event CurrentEvent );
SendingState_t QuerySendingCommSM ( void );

//Public helpers
uint8_t RR_Read(void);
uint8_t RS_Read(void);
uint8_t SS1_Read(void);
uint8_t SS2_Read(void);
uint8_t SS3_Read(void);

#endif

