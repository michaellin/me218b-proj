/****************************************************************************
 
  Header file for template DetectCityFrequencyService

 ****************************************************************************/

#ifndef DetectCityFrequency_H
#define DetectCityFrequency_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitCityFrequency, CheckingCityFrequency} DetectCityFrequency_t ;

// Public Function Prototypes

bool InitDetectCityFrequencyService ( uint8_t Priority );
bool PostDetectCityFrequencyService( ES_Event ThisEvent );
ES_Event RunDetectCityFrequencyService( ES_Event ThisEvent );

uint32_t GetValidCityPeriod (void);	

#endif



