// radio1_toolbox.h
//
// GoodPrototyping radio1 rev.C5
// (c) 2016-2018 Jory Anick, jory@goodprototyping.com

#ifndef radio1_toolbox_h
#define radio1_toolbox_h

#include "radio1_clock.h"
#include "radio1_timefunctions.h"

  // hardware
  //////////////////////////////

  // digital pins
  #define PIN_SLEEP        2  // sleep button
  
  #define PIN_CHARLIELED_1 3  // charlieplexed led pin 1
  #define PIN_CHARLIELED_2 4  // charlieplexed led pin 2
  #define PIN_CHARLIELED_3 5  // charlieplexed led pin 3

  #define PIN_LED2         8  // main power led
  #define PIN_SPK          10 // piezo speaker
  #define PIN_RESET        30 // internal reset button for automating reboots
  
  #define PIN_MONITORPOWER 42 // powers TV

  #define PIN_AMPVOLUME_L  44 // pwm (TIMER5) volume output for left  TDA7052A pin 4
  #define PIN_AMPVOLUME_R  45 // pwm (TIMER5) volume output for right TDA7052A pin 4

  // analog pins
  #define PIN_ANALOGVOLUME A1 // volume potentiometer  
  #define PIN_RTCPOWER     A2 // powers DS3231
  #define PIN_RADIOPOWER   A3 // powers TEA5767 & TDA7052As    
  #define PIN_CHARLIEKBD   A7 // 5 button external keyboard
  
  #define LED_MAX_ILLUM    5  // led maximum PWM value to save power by reducing brightness

  // functions
  ///////////////////////////////////////////////
  
  // get free memory in bytes
  int     radio1_memoryFree();
  
  // eeprom functions
  void    radio1_CheckEEPROMFormatted();
  void    radio1_ClearEEPROM();
  void    radio1_SaveSettingValue(int, uint8_t);
  uint8_t radio1_GetSettingValue(int, uint8_t);
  
  // date functions
  uint8_t radio1_Weekday(uint8_t, uint8_t, int);
  
  // rtc functions
  void    radio1_SetRTCDate(uint8_t, uint8_t, int);
  void    radio1_SetRTCTime(uint8_t, uint8_t, uint8_t);
  void    radio1_GetDateAndTimeFromRTC();
  
  // misc functions
  void    radio1_SoftwareReboot();
  void    radio1_GetAppVerString(char *);
  void    radio1_Tone(int, int);
  void    radio1_Delay(int);
  unsigned long radio1_Millis();
  
  int     radio1_analogReadFast(byte ADCpin);
  int     radio1_analogReadTwice(uint8_t);
 
#endif

// end of radio1_toolbox.h
