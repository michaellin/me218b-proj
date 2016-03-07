/****************************************************************************
 Module
     ES_Configure.h
 Description
     This file contains macro definitions that are edited by the user to
     adapt the Events and Services framework to a particular application.
 Notes
     
 History
 When           Who     What/Why
 -------------- ---     --------
 10/21/13 20:54 jec      lots of added entries to bring the number of timers
                         and services up to 16 each
 08/06/13 14:10 jec      removed PostKeyFunc stuff since we are moving that
                         functionality out of the framework and putting it
                         explicitly into the event checking functions
 01/15/12 10:03 jec      started coding
*****************************************************************************/

#ifndef CONFIGURE_H
#define CONFIGURE_H

/****************************************************************************/
// The maximum number of services sets an upper bound on the number of 
// services that the framework will handle. Reasonable values are 8 and 16
// corresponding to an 8-bit(uint8_t) and 16-bit(uint16_t) Ready variable size
#define MAX_NUM_SERVICES 16

/****************************************************************************/
// This macro determines that nuber of services that are *actually* used in
// a particular application. It will vary in value from 1 to MAX_NUM_SERVICES
#define NUM_SERVICES 9

/****************************************************************************/
// These are the definitions for Service 0, the lowest priority service.
// Every Events and Services application must have a Service 0. Further 
// services are added in numeric sequence (1,2,3,...) with increasing 
// priorities
// the header file with the public function prototypes
#define SERV_0_HEADER "MapKeys.h"
// the name of the Init function
#define SERV_0_INIT InitMapKeys
// the name of the run function
#define SERV_0_RUN RunMapKeys
// How big should this services Queue be?
#define SERV_0_QUEUE_SIZE 2

/****************************************************************************/
// The following sections are used to define the parameters for each of the
// services. You only need to fill out as many as the number of services 
// defined by NUM_SERVICES
/****************************************************************************/
// These are the definitions for Service 1
#if NUM_SERVICES > 1
// the header file with the public function prototypes
#define SERV_1_HEADER "Master.h"
// the name of the Init function
#define SERV_1_INIT InitMasterSM
// the name of the run function
#define SERV_1_RUN RunMasterSM
// How big should this services Queue be?
#define SERV_1_QUEUE_SIZE 5
#endif

/****************************************************************************/
// These are the definitions for Service 2
#if NUM_SERVICES > 2
// the header file with the public function prototypes
#define SERV_2_HEADER "ButtonDebounce.h"
// the name of the Init function
#define SERV_2_INIT InitButtonDB
// the name of the run function
#define SERV_2_RUN RunButtonDB
// How big should this services Queue be?
#define SERV_2_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 3
#if NUM_SERVICES > 3
// the header file with the public function prototypes
#define SERV_3_HEADER "CommService.h"
// the name of the Init function
#define SERV_3_INIT InitCommSM
// the name of the run function
#define SERV_3_RUN RunCommSM
// How big should this services Queue be?
#define SERV_3_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 4
#if NUM_SERVICES > 4
// the header file with the public function prototypes
#define SERV_4_HEADER "CaptureCityService.h"
// the name of the Init function
#define SERV_4_INIT InitCaptureCityService
// the name of the run function
#define SERV_4_RUN RunCaptureCityService
// How big should this services Queue be?
#define SERV_4_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 5
#if NUM_SERVICES > 5
// the header file with the public function prototypes
#define SERV_5_HEADER "WireCheckerService.h"
// the name of the Init function
#define SERV_5_INIT InitWireCheckerService
// the name of the run function
#define SERV_5_RUN RunWireCheckerService
// How big should this services Queue be?
#define SERV_5_QUEUE_SIZE 5
#endif

/****************************************************************************/
// These are the definitions for Service 6
#if NUM_SERVICES > 6
// the header file with the public function prototypes
#define SERV_6_HEADER "ShootingService.h"
// the name of the Init function
#define SERV_6_INIT InitShootingService
// the name of the run function
#define SERV_6_RUN RunShootingService
// How big should this services Queue be?
#define SERV_6_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 7
#if NUM_SERVICES > 7
// the header file with the public function prototypes
#define SERV_7_HEADER "DetectCityFrequencyService.h"
// the name of the Init function
#define SERV_7_INIT InitDetectCityFrequencyService
// the name of the run function
#define SERV_7_RUN RunDetectCityFrequencyService
// How big should this services Queue be?
#define SERV_7_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 8
#if NUM_SERVICES > 8
// the header file with the public function prototypes
#define SERV_8_HEADER "LEDService.h"
// the name of the Init function
#define SERV_8_INIT InitLEDService
// the name of the run function
#define SERV_8_RUN RunLEDService
// How big should this services Queue be?
#define SERV_8_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 9
#if NUM_SERVICES > 9
// the header file with the public function prototypes
#define SERV_9_HEADER "TestHarnessService9.h"
// the name of the Init function
#define SERV_9_INIT InitTestHarnessService9
// the name of the run function
#define SERV_9_RUN RunTestHarnessService9
// How big should this services Queue be?
#define SERV_9_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 10
#if NUM_SERVICES > 10
// the header file with the public function prototypes
#define SERV_10_HEADER "TestHarnessService10.h"
// the name of the Init function
#define SERV_10_INIT InitTestHarnessService10
// the name of the run function
#define SERV_10_RUN RunTestHarnessService10
// How big should this services Queue be?
#define SERV_10_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 11
#if NUM_SERVICES > 11
// the header file with the public function prototypes
#define SERV_11_HEADER "TestHarnessService11.h"
// the name of the Init function
#define SERV_11_INIT InitTestHarnessService11
// the name of the run function
#define SERV_11_RUN RunTestHarnessService11
// How big should this services Queue be?
#define SERV_11_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 12
#if NUM_SERVICES > 12
// the header file with the public function prototypes
#define SERV_12_HEADER "TestHarnessService12.h"
// the name of the Init function
#define SERV_12_INIT InitTestHarnessService12
// the name of the run function
#define SERV_12_RUN RunTestHarnessService12
// How big should this services Queue be?
#define SERV_12_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 13
#if NUM_SERVICES > 13
// the header file with the public function prototypes
#define SERV_13_HEADER "TestHarnessService13.h"
// the name of the Init function
#define SERV_13_INIT InitTestHarnessService13
// the name of the run function
#define SERV_13_RUN RunTestHarnessService13
// How big should this services Queue be?
#define SERV_13_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 14
#if NUM_SERVICES > 14
// the header file with the public function prototypes
#define SERV_14_HEADER "TestHarnessService14.h"
// the name of the Init function
#define SERV_14_INIT InitTestHarnessService14
// the name of the run function
#define SERV_14_RUN RunTestHarnessService14
// How big should this services Queue be?
#define SERV_14_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 15
#if NUM_SERVICES > 15
// the header file with the public function prototypes
#define SERV_15_HEADER "TestHarnessService15.h"
// the name of the Init function
#define SERV_15_INIT InitTestHarnessService15
// the name of the run function
#define SERV_15_RUN RunTestHarnessService15
// How big should this services Queue be?
#define SERV_15_QUEUE_SIZE 3
#endif


/****************************************************************************/
// Name/define the events of interest
// Universal events occupy the lowest entries, followed by user-defined events
typedef enum {  ES_NO_EVENT = 0,
                ES_ERROR,  /* used to indicate an error from the service */
                ES_INIT,   /* used to transition from initial pseudo-state */
                ES_NEW_KEY, /* signals a new key received from terminal */
                ES_TIMEOUT, /* signals that the timer has expired */
                ES_ENTRY,
                ES_ENTRY_HISTORY,
                ES_EXIT,
                /* User-defined events start here */
								/* Commands to Driving State Machine */
								ES_OkToWaitCapture, //Sent from game service to capture city service when game service is ready to ignore city so that capture service doesnt get stuck in state
								ES_Stuck,
								ES_IgnoreCity, //Tells game service to not respond to AT_City events sent from capture city service until we leave a city 
								ES_DriveForward,//Sent from navigation to driving
								ES_DriveBackward,//Sent from navigation to driving
								ES_TurnLeft,//Sent from navigation to driving
								ES_TurnRight, //Sent from navigation to driving
								ES_WireFollow, //Sent to Driving to go into wire following
								ES_FindWire,  //Sent to Driving to start navigating to find a wire
								ES_StopMotors,//When we want to go back into waiting to drive in Driving.c
								ES_SweepWire, //For finding wire
								ES_SweepingDone, //When done finding wire
								/* Events from Driving */
								ES_FinishedDriving, //Sent from Driving to indicate target position reached
								/* Events exchanged with ShootingService */
								ES_BallShot, //Sent from ShootingService to navigation 
								ES_StartShooting, //Posted from driving to shooting service
								/* Events used by Comm */
								ES_SendCMD,
								ES_SendByte, //10
								ES_SPIWriteDone, // Posted by CommService to indicate that a whole 5 byte transaction has finished
								ES_EOT,
								ES_ReadingDone,
								ES_SPIWrite,
								/* Events in GameService */
								ES_StartCampaign, //In gameService moves us to Campaigning state
								ES_GameOver,    //In gameService moves us to WaitingforCampaign state
								/* Event posted by CaptureCityService */
							  ES_GotCityName,
								/* Event posted by DetectCityFrequency */
								ES_AtCity, //Posted to CaptureCityService and GameService whenever we detect a valid city frequency
								ES_NotAtCity,
								ES_StartCaptureCity, //Posted by GameService to CaptureCityService to do the city capture actions
								/* Event posted by Debounce Button to switch teams */
								ES_SwitchTeam,
								/* Event posted to Navigation to know that Wire has been found or Wire has been lost*/
								ES_WireFound, //When wire sensor detects high enough voltage to follow wire
								ES_WireLost,  //wire sensor does not detect high enough voltage while trying to find wire
								ES_Sweep4Wire,
								ES_FoundRightWire,
								ES_FoundLeftWire,
								//Other Events
								ES_Celebrate,
								ES_StopCelebrate,
								ES_LOCK,
								ES_UNLOCK,
								ES_BUTTON_DOWN,
								ES_BUTTON_UP} ES_EventTyp_t ;

/****************************************************************************/
// These are the definitions for the Distribution lists. Each definition
// should be a comma separated list of post functions to indicate which
// services are on that distribution list.
#define NUM_DIST_LISTS 1
#if NUM_DIST_LISTS > 0 
#define DIST_LIST0 PostMapKeys, PostMasterSM
#endif
#if NUM_DIST_LISTS > 1 
#define DIST_LIST1 PostTemplateFSM, TemplateFSM
#endif
#if NUM_DIST_LISTS > 2 
#define DIST_LIST2 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 3 
#define DIST_LIST3 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 4 
#define DIST_LIST4 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 5 
#define DIST_LIST5 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 6 
#define DIST_LIST6 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 7 
#define DIST_LIST7 PostTemplateFSM
#endif

/****************************************************************************/
// This are the name of the Event checking funcion header file. 
#define EVENT_CHECK_HEADER "EventCheckers.h"

/****************************************************************************/
// This is the list of event checking functions 
#define EVENT_CHECK_LIST Check4Keystroke, CheckButtonEvents


/****************************************************************************/
// These are the definitions for the post functions to be executed when the
// corresponding timer expires. All 16 must be defined. If you are not using
// a timer, then you should use TIMER_UNUSED
// Unlike services, any combination of timers may be used and there is no
// priority in servicing them
#define TIMER_UNUSED ((pPostFunc)0)
#define TIMER0_RESP_FUNC PostMasterSM
#define TIMER1_RESP_FUNC PostMasterSM
#define TIMER2_RESP_FUNC PostCommSM
#define TIMER3_RESP_FUNC PostCaptureCityService
#define TIMER4_RESP_FUNC PostCaptureCityService
#define TIMER5_RESP_FUNC PostMasterSM
#define TIMER6_RESP_FUNC PostWireCheckerService
#define TIMER7_RESP_FUNC PostMasterSM
#define TIMER8_RESP_FUNC PostMasterSM
#define TIMER9_RESP_FUNC PostShootingService
#define TIMER10_RESP_FUNC PostDetectCityFrequencyService
#define TIMER11_RESP_FUNC PostDetectCityFrequencyService
#define TIMER12_RESP_FUNC PostMasterSM
#define TIMER13_RESP_FUNC PostButton
#define TIMER14_RESP_FUNC PostMasterSM
#define TIMER15_RESP_FUNC PostLEDService

/****************************************************************************/
// Give the timer numbers symbolc names to make it easier to move them
// to different timers if the need arises. Keep these definitions close to the
// definitions for the response functions to make it easier to check that
// the timer number matches where the timer event will be routed
// These symbolic names should be changed to be relevant to your application 

//#define SERVICE0_TIMER 15
#define DrivingTimer 0
#define DB_TIMER 13
#define PacTimer 2
#define QueryTimer 3
#define CaptureTimer 4
#define WireFindingTimer 5
#define WireCheckTimer 6
#define OneShotLeftTimer      7
#define OneShotRightTimer     8
#define ShootingTimer 9
#define CityFrequencyTimer 10
#define CityFrequencyOneShot     11
#define CampaignTimer 1
#define GameStatusTimer 12
#define MakeSureNotStuckTimer 14
#define CELEB_TIMER 15

//#define ProtectionSwitchState 16 Not using this anymore


#endif /* CONFIGURE_H */
