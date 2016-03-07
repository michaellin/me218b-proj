#ifndef InitHWModules
#define InitHWModules

void InitPWMLeft( void );
void InitPWMRight( void );
void InitMotorLines( void); //intializes PB0 and PB1 as dig out
void InitInputCapture( void );
void InitController( void );
void InitInputCapturePeriodLeft( void );
void InitInputCapturePeriodRight( void );
void InitOneShotLeft( void );
void InitOneShotRight( void );
void InitMotorVelocityPID( void );
void InitMotorPositionPID( void );
void InitLED(void);
void InitSPI(void);
void InitInputCaptureCityPeriod(void);
void InitShooterPWM( void );
void InitGameOneShot( void );

#endif
