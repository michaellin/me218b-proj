#ifndef DrivingHelper
#define DrivingHelper

#include <stdint.h>

void SetDutyLeft(int Duty);
void SetDutyRight(int Duty);

void SetForwardRPMLeft(float RPMToSet);
void SetForwardRPMRight(float RPMToSet);
void SetBackwardRPMLeft(float RPMToSet);
void SetBackwardRPMRight(float RPMToSet);

#endif /* DrivingHelper */
