// radio1.ino
//
// GoodPrototyping radio1 rev.C5
// (c) 2016-2018 Jory Anick, jory@goodprototyping.com

// hardware 7.13.2016
// software 5.28.16 -
//
// 6.20.16 added alarm capability
// 6.21.16 added auto wake from sleep for alarm
// 10.8.16 hardware C5 update
// 2.9.18  tvout TCCR2A fixes
// 2.22.18 build 45 for public release
// 3.4.18  build 46, fixes flicker on low cost composite lcd displays

// This project compiles as-is in Arduino 1.8+
// Copy the contents of the supplied "arduino libraries" folder
// into your Arduino "libraries" directory

// to speed up compilation & upload:
// in file C:\arduino-1.8.5\hardware\arduino\avr\programmers.txt
//usbtinyisp.name=USBtinyISP
//usbtinyisp.protocol=usbtiny
//usbtinyisp.program.tool=avrdude
//usbtinyisp.program.extra_params=-V -B 1<-- disables verification, speeds up transfer to SCK period 1 usec (3x speedup)

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <inttypes.h>
#include <avr/wdt.h>
#include <Wire.h>
#include "radio1.h"
#include "radio1_tea5767n.h"
#include "radio1_digitalwritefast.h"
#include "radio1_timer5.h"   // interrupts for TDA7052A volume
#include "radio1_lcdmenu2.h" // menu system
#include "radio1_defs.h"     // consts, vars, defs, etc

char buf[40]; // universal buffer

// radio
TEA5767N radio = TEA5767N();
float frequency = 88.5f;
unsigned long last_UIactivityMS = 0;
uint8_t radioONflag = 0;
uint8_t systemVolumeLevel = 0;
unsigned long last_volumeAdjustmentMS = millis();
uint8_t radioSignalStrength = 0;
uint8_t radioTuningSensitivityLevel_LO_MED_HI = 1; // medium level by default

// composite video
TVout TV;

// radio1 specific
uint8_t videoONflag = 0;
uint8_t videoIStimedout = 0;
uint8_t videoTimeoutValueInMinutes = 30; // default value

// for triggering sleep via interrupt button
volatile uint8_t setSleep = 0;
// for detecting the way we woke from sleep
volatile uint8_t awokeBySleepButton = 0;

// working date/time for all functions
struct DateType CustomDate;

// days of week
const char * CONST_DAYSOFWEEK_LONGFMT[] = { dayStr1, dayStr2, dayStr3, dayStr4, dayStr5, dayStr6, dayStr7 };

// big numbers
const uint8_t * BigNums[] = { bignums_0, bignums_1, bignums_2, bignums_3, bignums_4, bignums_5, bignums_6, bignums_7, bignums_8, bignums_9 };

uint8_t alarmTriggered = 0; // default (off) alarm in progress
uint8_t alarmEnabled = 0;   // default (off) alarm value
uint8_t alarmHour = 255;    // default (unset) alarm settings
uint8_t alarmMinute = 255;  // default (unset) alarm settings

// pulsing led (default is visible)
static const uint8_t CONST_LEDDELAY = 50;
float pulseLedval = 50.0f;
uint8_t lastLedToggled = 1;

// buttons
uint8_t butpressed[5] = {0, 0, 0, 0, 0};
uint8_t button_press_enter = 0;
unsigned long gBUTTON_press_time = millis();
uint8_t BUTTON_press_time = CONST_DEFAULT_BUTTON_TIME;

// menu code /////////////////////////////////////////////////////////////////////////////////
uint8_t inScreen;
uint8_t appScreenMode = SCREENMODE_CLOCK;
static const uint8_t rotateScreens = 0;
static const uint8_t rotateScreenSeconds = 30;

menu top(MENUCONST_ROOT);
lcdmenu2 Root(top, LCD_rows, LCD_cols);

menu Item1(MENUCONST_APPNAME);
menu Item2(MENUCONST_SETTINGS);
menu Item21(MENUCONST_SETTIME);
menu Item22(MENUCONST_SETDATE);
menu Item23(MENUCONST_SETALARMTIME);
menu Item24(MENUCONST_SETDISPLAYTIMEOUT);
menu Item241(MENUCONST_SETDISPLAYTIMEOUT_NEVER);
menu Item242(MENUCONST_SETDISPLAYTIMEOUT_1MIN);
menu Item243(MENUCONST_SETDISPLAYTIMEOUT_3MIN);
menu Item244(MENUCONST_SETDISPLAYTIMEOUT_10MIN);
menu Item245(MENUCONST_SETDISPLAYTIMEOUT_30MIN);

menu Item25(MENUCONST_SETRADIOTUNING_SENSLVL);
menu Item251(MENUCONST_SETRADIOTUNING_SENSLVL_H);
menu Item252(MENUCONST_SETRADIOTUNING_SENSLVL_M);
menu Item253(MENUCONST_SETRADIOTUNING_SENSLVL_L);
menu Item26(MENUCONST_CLEARALLSETTINGS);
menu Item27(MENUCONST_DOBENCHMARKS);
menu Item3(MENUCONST_REBOOT);
menu Item4(MENUCONST_ABOUT);
// end of menu code

// for object editing
// alarm time
uint8_t alarmTimeEditHour;
uint8_t alarmTimeEditMinute;
uint8_t alarmTimeEditCursorPos;
// time
uint8_t timeEditHour;
uint8_t timeEditMinute;
uint8_t timeEditSecond;
uint8_t timeEditCursorPos;
// date
uint8_t dateEditDay;
uint8_t dateEditMonth;
int dateEditYear;
uint8_t dateEditCursorPos;
// end of timedate editing

// for timers
uint8_t LastCustomDateUpdateSec = 0; // last second the screen was updated
uint8_t  TimeCounter = 0; // for updating the universe every minute
unsigned long thismillis = 0; // universal uptime tick count
int ScreenModeTimeShowing = 0; // holds number of seconds the screen mode is active (to control scrolling in aspectsmodetext)
long long systemUptimeSeconds = 0; // holds total system uptime in seconds
uint8_t Xor1 = 0; // for flicker every second

// starfield vars
struct StarField_pixel3D
{
  uint8_t X;
  uint8_t Y;
  uint8_t Z;
};
static const uint8_t CONST_NUMSTARS = 25;
StarField_pixel3D StarField_star[CONST_NUMSTARS];
// end of starfield vars

// function prototypes for inline functions
void charlieleds_set_pinsPWM(uint8_t high_pin, uint8_t low_pin, uint8_t highval);
void charlieleds_setledPWM(uint8_t lednum, uint8_t ledval);
static void charlieleds_reset_pins();
uint8_t DotAtThisLocation(uint8_t i);
void MoveDotsLeft();
void Update(uint8_t refreshBG);
void ShowFrequencyAndVolume();
void radio_setVolumePWM();
float radio_getFrequency();
void radio_setSearchLevel();
void ShowTimeBig(int x, int y);
void resetButtonValues();

void setup()
{
  // initialize system hardware
  radio1_inithardware();

  // set up menus
  menuinit();

  // set default to app
  inScreen = MENUMODE_APP;

  // set up the initial UI for the mode (menu/app/settings/etc)
  InitMode();  
}

void InitMode()
{
  switch (inScreen)
  {
    case MENUMODE_MENU:
      menudisplay();
      break;

    case MENUMODE_APP:
      // reduces flicker on startup and mode changes
      TV.delay_frame(1);
      Update(1);
      break;

    case MENUMODE_SETALARMTIME:
      clsAndShowMenuLabel(MENUCONST_SETALARMTIME);

      // set default values
      setAlarmValuesForEdit();

      // show time for edit
      ShowAlarmTimeForEditing();

      // set cursor to edit hour first
      alarmTimeEditCursorPos = 1;
      break;

    case MENUMODE_SETTIME:
      clsAndShowMenuLabel(MENUCONST_SETTIME);

      // set default values
      setValuesForEdit();

      // show time for edit
      ShowTimeForEditing();

      // set cursor to edit hour first
      timeEditCursorPos = 1;
      break;

    case MENUMODE_SETDATE:
      clsAndShowMenuLabel(MENUCONST_SETDATE);

      // set default values
      setValuesForEdit();

      // show time for edit
      ShowDateForEditing();

      // set cursor to edit hour first
      dateEditCursorPos = 1;
      break;

    case MENUMODE_ABOUT:
      showAboutScreen();
      break;

    case MENUMODE_CLREEPROM:
      // clear eeprom
      clsAndShowMenuLabel(MENUCONST_CLEARALLSETTINGS);
      TV.printPGM(9, 28, CONST_ERASING);
      break;
    
    case MENUMODE_BENCHMARKS:
      // benchmark screen
      clsAndShowMenuLabel(MENUCONST_DOBENCHMARKS);
      break;
  }

  // reset all stored button values
  resetButtonValues();
}

void showAboutScreen()
{
  TV.delay_frame(1);

  clsAndShowMenuLabel(MENUCONST_ABOUT);

  radio1_GetAppVerString(buf);
  TV.print(9, 28, buf);
  TV.printPGM(9, 38, CONST_COPYRIGHTSTRING);

  TV.printPGM(9, 60, CONST_UPTIME);
  buildUptimeString(buf);
  TV.print(buf);

  TV.print(9, 80, radio1_memoryFree());
  TV.printPGM(CONST_BYTESFREE);
}

void buildUptimeString(char *outbuf)
{
  struct DateType d;
  char tmpbuf[10];
  long long tmpLngLng;

  long long timeUp = systemUptimeSeconds;
  static const long CONST_YEAROFSECONDS = 31557600L; //86400.0f * 365.25f;

  // get uptime parms the slow readable way
  tmpLngLng = timeUp / CONST_YEAROFSECONDS;
  d.yr = tmpLngLng;
  timeUp = timeUp - (d.yr * CONST_YEAROFSECONDS);

  tmpLngLng = timeUp / 86400L;
  d.dy = tmpLngLng;
  timeUp = timeUp - (d.dy * 86400L);

  tmpLngLng = timeUp / 3600L;
  d.hr = tmpLngLng;
  timeUp = timeUp - (d.hr * 3600L);

  tmpLngLng = timeUp / 60L;
  d.mn = tmpLngLng;
  timeUp = timeUp - (d.mn * 60L);

  outbuf[0] = 0;
  if (d.yr > 0)
  {
    itoa(d.yr, tmpbuf, 10);
    strcat(outbuf, tmpbuf);
    strcat_P(outbuf, CONST_YEARSPREFIX);
  }

  if (d.dy > 0)
  {
    itoa(d.dy, tmpbuf, 10);
    strcat(outbuf, tmpbuf);
    strcat_P(outbuf, CONST_DAYSPREFIX);
  }

  if (d.hr > 0)
  {
    itoa(d.hr, tmpbuf, 10);
    strcat(outbuf, tmpbuf);
    strcat_P(outbuf, CONST_HOURSPREFIX);
  }

  itoa(d.mn, tmpbuf, 10);
  strcat(outbuf, tmpbuf);
  strcat_P(outbuf, CONST_MINUTESPREFIX);
}

void loop()
{
  static unsigned long LastRTCCheckMS;
  uint8_t isNewSecond = 0;
  uint8_t updateThisRev = 0;
  static unsigned long lastDOTSmillis = 0;
  static uint8_t lastMinute = 0;

  // check sleep button
  if (setSleep == 1) doSleep();
  
  thismillis = radio1_Millis();
  if (thismillis < LastRTCCheckMS) LastRTCCheckMS = 0;  // 50 day rollover protection
  if (thismillis - LastRTCCheckMS > 100)
  {
    LastRTCCheckMS = thismillis;

    radio1_GetDateAndTimeFromRTC();
    
    // check signal strength if the radio is on
    if (radioONflag == 1)
      radioSignalStrength = radio.getSignalLevel();

    if (LastCustomDateUpdateSec != CustomDate.sc)
    {
      // set last update second
      LastCustomDateUpdateSec = CustomDate.sc;

      // flag new second
      isNewSecond = 1;

      // increase uptime;
      systemUptimeSeconds++;

      // TOGGLE XOR VALUE
      Xor1 = !Xor1;

      // check alarm
      if (alarmEnabled ==  1)
      {
        if (alarmTriggered == 0)
        {
          if (CustomDate.hr == alarmHour)
            if (CustomDate.mn == alarmMinute)
              if (CustomDate.sc == 0)
              {
                // we have an alarm
                alarm_setTriggered();
                InitMode();
              }
        }
      }

      if (videoONflag == 1)
      {
        // check for video timeout
        if (inScreen != MENUMODE_BENCHMARKS) // not when showing the benchmark screen
        {
          if (videoIStimedout == 0)
          {
            if (videoTimeoutValueInMinutes > 0)
            {
              if (alarmTriggered == 0)
              {
                if (thismillis > last_UIactivityMS + (videoTimeoutValueInMinutes * 60000))
                {
                  // time the video out
                  tv_shutdown();
                  videoIStimedout = 1;
                  // adjust volume for timed out video
                  if (radioONflag == 1)
                    radio_setVolumePWM();
                }
              } // alarmtriggered=0
            } // videotimeoutvalueinminutes=0
          } // videoistimedout==0
        } // != MENUMODE_BENCHMARKS
      } // videoonflag
    } // end of every second

    if (alarmTriggered == 1)
    {
      // alarm in progress, make noise
      if (CustomDate.sc % 2 == 0)
      {
        radio1_Tone(1100, 30);
        radio1_Tone(900, 20);
        radio1_Tone(500, 20);
      }
    }
  }

  // check keyboard every 20ms
  static unsigned long lastKBmillis = 0;
  if (thismillis < lastKBmillis) lastKBmillis = 0; // 50 day rollover protection
  if (thismillis - lastKBmillis > 20)
  {
    menu_buttoncheck();
    lastKBmillis = thismillis;
  }

  // process LED scrolling / tuning position / signal level every 15ms
  static unsigned long lastLEDmillis = 0;
  if (thismillis < lastLEDmillis) lastLEDmillis = 0; // 50 day rollover protection
  if (thismillis - lastLEDmillis > 15)
  {
    if (doLedScrollForStatus() == 2)
      radio_showTuningPositionLEDS();
    lastLEDmillis = thismillis;
  }

  if (radioONflag == 1)
  {
    static unsigned long lastAnalogVolumeReadMillis = 0;
    if (thismillis < lastAnalogVolumeReadMillis) lastAnalogVolumeReadMillis = 0; // 50 day rollover protection

    // read analog value every 50ms to speed things up
    if (thismillis - lastAnalogVolumeReadMillis > 50)
    {
      int newVolumeReading = 100 - (radio1_analogReadTwice(PIN_ANALOGVOLUME) / 10.24);
      if (systemVolumeLevel != newVolumeReading)
      {
        // set new volume
        systemVolumeLevel = newVolumeReading;
        // set last adjustment time
        last_volumeAdjustmentMS = thismillis;
        // adjust pwm to TDA volume pins 4
        radio_setVolumePWM();

        // update screen if we're in the app mode and the radio panel is showing
        if ((inScreen == MENUMODE_APP) && (appScreenMode == SCREENMODE_RADIO))
          updateThisRev = 1;
      }
      lastAnalogVolumeReadMillis = thismillis;
    }

    static float lastfrequency;
    if (lastfrequency != frequency)
    {
      lastfrequency = frequency;
      // mark for ui update
      updateThisRev = 1;
    }
  }
  
  // only process if the screen is on
  switch (inScreen)
  {
    case MENUMODE_MENU:
      menu_funccheck();
      break;

    case MENUMODE_APP:
      // all other modes run once a second on the timer
      if (isNewSecond == 1)
      {
        updateThisRev = 1;
        AppTimer_Timer();    //   run the timer
      }
      break;

    case MENUMODE_SETTIME:
      if (isNewSecond == 1) ShowTimeForEditing();
      break;

    case MENUMODE_SETDATE:
      if (isNewSecond == 1) ShowDateForEditing();
      break;

    case MENUMODE_SETALARMTIME:
      if (isNewSecond == 1) ShowAlarmTimeForEditing();
      break;

    case MENUMODE_ABOUT:
      // update every second for uptime
      if (isNewSecond == 1)
        if (videoIStimedout == 0)
          if (videoONflag == 1)
            if (lastMinute != CustomDate.mn) // once a minute update
              showAboutScreen();
      break;

    case MENUMODE_CLREEPROM:
      // Clear EEPROM (settings)
      radio1_ClearEEPROM();
      radio1_Tone(750, 100);
      radio1_Delay(1000);
      radio1_SoftwareReboot();
      break;
    
    case MENUMODE_BENCHMARKS:
      doBenchmarkPanel();
      break;
  } // switch

  // process consolidated update flag
  if (updateThisRev == 1)
    Update(0);

  // scroll the dots
  if (inScreen == MENUMODE_APP)
  {
    if (appScreenMode == SCREENMODE_CLOCK)
    {
      if (videoONflag == 1)
      {
        if (thismillis < lastDOTSmillis) lastDOTSmillis = 0; // 50 day rollover protection
        if (thismillis - lastDOTSmillis > 5)
        {
          lastDOTSmillis = thismillis;
          MoveDotsLeft();
        }
      }
    }
  }

  if (lastMinute != CustomDate.mn)
    lastMinute = CustomDate.mn;

  // this stops flicker
  if (videoONflag == 1)
    TV.delay_frame(1);
}

void menuinit()
{
  top.addChild(Item1);            // radio1
  top.addChild(Item2);            // settings
  Item2.addChild(Item21);         // set time
  Item2.addChild(Item22);         // set date
  Item2.addChild(Item23);         // set alarm time
  Item2.addChild(Item24);         // set display timeout
  Item24.addChild(Item241);       // set display timeout NEVER
  Item24.addChild(Item242);       // set display timeout 1 MIN
  Item24.addChild(Item243);       // set display timeout 3 MIN
  Item24.addChild(Item244);       // set display timeout 10 MIN
  Item24.addChild(Item245);       // set display timeout 30 MIN
  Item2.addChild(Item25);         // set radio tuning sensitivity
  Item25.addChild(Item251);       // HIGH
  Item25.addChild(Item252);       // MEDIUM
  Item25.addChild(Item253);       // LOW
  Item2.addChild(Item26);         // clear all settings + reboot
  Item2.addChild(Item27);         // do benchmarks
  top.addChild(Item3);            // reboot
  top.addChild(Item4);            // about
}

void menudisplay()
{
  Root.display();
}

void resetButtonValues()
{
  memset(&butpressed[0], 0, 5);
  button_press_enter = 0;
  gBUTTON_press_time = radio1_Millis();
  BUTTON_press_time = CONST_DEFAULT_BUTTON_TIME;
}

// button handler
void button(uint8_t which)
{
  TimeCounter = 0;  // reset mode switcher counter

  switch (inScreen)
  {
    case MENUMODE_APP:
      last_UIactivityMS = thismillis; // get last uptime at user activity
      switch (which)
      {
        case BtnMENU:
          // show menu
          TVScrollUp();
          inScreen = MENUMODE_MENU;
          InitMode();
          break;

        case BtnMODE:
          // toggle mode
          switch (appScreenMode)
          {
            case SCREENMODE_CLOCK:
            case SCREENMODE_RADIO:
              // increase screen mode
              advanceScreenMode();
              break;
          }
          break;

        case BtnBKWD:
          switch (appScreenMode)
          {
            case SCREENMODE_CLOCK:
              // toggles alarm enabled / disabled
              alarmEnabled = !alarmEnabled;

              // save to EEPROM for future recall
              radio1_SaveSettingValue(EEPROMLOCATION_ALARM_ENABLED, alarmEnabled);

              if (alarmEnabled == 1)
              {
                // enable the alarm

              } else
              {
                // disable any alarm
                alarmTriggered = 0;
              }
              break;

            case SCREENMODE_RADIO:
              if (radioONflag == 1)
              {
                if (BUTTON_press_time < CONST_DEFAULT_BUTTON_TIME - 20)
                {
                  radio.selectFrequency(frequency -= 0.1);
                  radio_showTuningPositionLEDS();
                  frequency = radio_getFrequency();
                }
              }
              break;
          }
          break;

        case BtnFWD:
          switch (appScreenMode)
          {
            case SCREENMODE_CLOCK:
              break;

            case SCREENMODE_RADIO:
              if (radioONflag == 1)
              {
                if (BUTTON_press_time < CONST_DEFAULT_BUTTON_TIME - 20)
                {
                  radio.selectFrequency(frequency += 0.1);
                  radio_showTuningPositionLEDS();
                  frequency = radio_getFrequency();
                }
              }
              break;
          }
          break;

        case BtnSELECT:
          // add or delete from memory
          if (radioONflag == 1)
          {
            if ((BUTTON_press_time < CONST_DEFAULT_BUTTON_TIME - 40) &&
                (BUTTON_press_time > CONST_DEFAULT_BUTTON_TIME - 60))
            {
              radiomemory_toggle();
              Update(0);
            }
          }

          break;
      }
      break;

    case MENUMODE_MENU:
      // menu mode
      switch (which)
      {
        case BtnBKWD:
          // up
          Root.goUp();
          break;

        case BtnFWD:
          // down
          Root.goDown();
          break;

        case BtnSELECT:
          // enter
          Root.goEnter();
          button_press_enter = 1;
          break;

        case BtnMENU:
          // menu
          Root.goBack();
          break;
      }
      break;

    case MENUMODE_SETALARMTIME:
      // set alarm time mode
      switch (which)
      {
        case BtnFWD:
          switch (alarmTimeEditCursorPos)
          {
            case 1:  // hour
              alarmTimeEditHour++;
              if (alarmTimeEditHour > 23)
                alarmTimeEditHour = 0;
              break;

            case 2:  // minute
              alarmTimeEditMinute++;
              if (alarmTimeEditMinute > 59)
                alarmTimeEditMinute = 0;
              break;
          }
          ShowAlarmTimeForEditing();
          break;

        case BtnBKWD:
          switch (alarmTimeEditCursorPos)
          {
            case 1: // hour
              if (alarmTimeEditHour > 0)
                alarmTimeEditHour--;
              else
                alarmTimeEditHour = 23;
              break;

            case 2: // minute
              if (alarmTimeEditMinute > 0)
                alarmTimeEditMinute--;
              else
                alarmTimeEditMinute = 59;
              break;
          }
          ShowAlarmTimeForEditing();
          break;

        case BtnSELECT:
          // advance position (hour edit to minute edit
          alarmTimeEditCursorPos++;
          if (alarmTimeEditCursorPos > 2)
          {
            // set alarm time
            alarmHour = alarmTimeEditHour;
            alarmMinute = alarmTimeEditMinute;

            // save to EEPROM for future recall
            radio1_SaveSettingValue(EEPROMLOCATION_ALARMHOUR,   alarmHour);
            radio1_SaveSettingValue(EEPROMLOCATION_ALARMMINUTE, alarmTimeEditMinute);

            // return to menu
            inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
            InitMode();
          }
          break;

        case BtnMENU:
          // return to menu
          inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
          InitMode();
          break;
      } // END OF SETALARMTIME
      break;

    case MENUMODE_SETTIME:
      // set time mode
      switch (which)
      {
        case BtnFWD:
          switch (timeEditCursorPos)
          {
            case 1: // hour
              timeEditHour++;
              if (timeEditHour > 23)
                timeEditHour = 0;
              break;

            case 2: // minute
              timeEditMinute++;
              if (timeEditMinute > 59)
                timeEditMinute = 0;
              break;

            case 3: // second
              timeEditSecond++;
              if (timeEditSecond > 59)
                timeEditSecond = 0;
              break;
          }
          ShowTimeForEditing();
          break;

        case BtnBKWD:
          switch (timeEditCursorPos)
          {
            case 1: // hour
              if (timeEditHour > 0)
                timeEditHour--;
              else
                timeEditHour = 23;
              break;

            case 2: // minute
              if (timeEditMinute > 0)
                timeEditMinute--;
              else
                timeEditMinute = 59;
              break;

            case 3: // second
              if (timeEditSecond > 0)
                timeEditSecond--;
              else
                timeEditSecond = 59;
              break;
          }
          ShowTimeForEditing();
          break;

        case BtnSELECT:
          // advance position (hour edit to minute edit to seconds edit
          timeEditCursorPos++;
          if (timeEditCursorPos > 3)
          {
            // set time
            radio1_SetRTCTime(timeEditHour, timeEditMinute, timeEditSecond);

            // return to menu
            inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
            InitMode();
          }
          break;

        case BtnMENU:
          // return to menu
          inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
          InitMode();
          break;
      } // end of settime
      break;

    case MENUMODE_SETDATE:
      // set date mode
      switch (which)
      {
        case BtnFWD:
          switch (dateEditCursorPos)
          {
            case 1: // month
              dateEditMonth++;
              if (dateEditMonth > 12)
                dateEditMonth = 1;
              break;

            case 2: // day
              dateEditDay++;
              if (dateEditDay > 31)
                dateEditDay = 1;
              break;

            case 3: // year
              dateEditYear++;
              break;
          }
          ShowDateForEditing();
          break;

        case BtnBKWD:
          switch (dateEditCursorPos)
          {
            case 1: // month
              if (dateEditMonth > 1)
                dateEditMonth--;
              else
                dateEditMonth = 12;
              break;

            case 2: // day
              if (dateEditDay > 1)
                dateEditDay--;
              else
                dateEditDay = 31;
              break;

            case 3: // year
              if (dateEditYear > 0) dateEditYear--;
              break;
          }
          ShowDateForEditing();
          break;

        case BtnSELECT:
          // advance position (month edit to day edit to year edit)
          dateEditCursorPos++;
          if (dateEditCursorPos > 3)
          {
            // set date
            radio1_SetRTCDate((int) dateEditMonth, (int) dateEditDay, (int) dateEditYear);

            // return to menu
            inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
            InitMode();
          }
          break;

        case BtnMENU:
          // return to menu
          inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
          InitMode();
          break;
      } // END OF SETDATE
      break;

    case MENUMODE_ABOUT:
      // about mode
      if (which == BtnMENU)
      {
        // return to menu
        inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
        InitMode();
      }
      break;

    case MENUMODE_CLREEPROM:
      // clear eeprom settings
      // we don't handle buttons, we wait until it's finished clearing
      break;
      
  case MENUMODE_BENCHMARKS:
      // benchmark screen
      // buttons exit out of it
      inScreen = MENUMODE_MENU; // pressing menu should always set the inscreen to menu
      InitMode();
      break;      
  }
}

// read buttons
void menu_buttoncheck()
{
  int i;
  int isDown;
  int kbdRead;
  float nextStoredFrequency;

  kbdRead = radio1_analogReadTwice(PIN_CHARLIEKBD);

  // process the state of each button
  for (i = 0; i < 5; i++)
  {
    isDown = 0;
    switch (i)
    {
      case 0:
        // menu
        if (kbdRead < 10) isDown = 1;
        break;
      case 1:
        // mode
        if ((kbdRead > 725) && (kbdRead < 735)) isDown = 1;
        break;
      case 2:
        // up
        if ((kbdRead > 345) && (kbdRead < 355)) isDown = 1;
        break;
      case 3:
        // down
        if ((kbdRead > 175) && (kbdRead < 185)) isDown = 1;
        break;
      case 4:
        // ok
        if ((kbdRead > 500) && (kbdRead < 510)) isDown = 1;
        break;
    }

    if (isDown == 1)
    {
      // button press during timeout restores video
      // button is otherwise ignored
      if (videoIStimedout == 1)
      {
        restoreTimedOutVideo();
        break; // break out of the for loop
      }

      // reset the xor's so any flickering doesn't distract the user
      Xor1 = 0;

      if (thismillis < gBUTTON_press_time) gBUTTON_press_time = 0; // fix for 50 day rollover

      // check time since last keypress
      if ( (thismillis - gBUTTON_press_time) >= BUTTON_press_time )
      {
        butpressed[i] = 1; // set button pressed
        // run button command
        button(i);
        // set last press time
        gBUTTON_press_time = thismillis;

        // reduce press time to scroll fast
        if (BUTTON_press_time >= 25)
          BUTTON_press_time = BUTTON_press_time - 10;
      }
      break; // break out of the for loop as we can only press one button at a time
    } // (isDown == 1)
    else
    {
      if (butpressed[i] == 1)
      {
        // was pressed and now released
        if (inScreen == MENUMODE_APP)
        {
          if (appScreenMode == SCREENMODE_RADIO)
          {
            if (BUTTON_press_time > CONST_DEFAULT_BUTTON_TIME - 30)
            {
              // basic button press, not held
              switch (i)
              {
                case BtnBKWD:
                  // new custom search
                  radio_searchUpDn(0);
                  break;

                case BtnFWD:
                  // new custom search
                  radio_searchUpDn(1);
                  break;

                case BtnSELECT:
                  // stored frequency hopping
                  // move position up
                  nextStoredFrequency = radiomemory_getNextStoredMemoryPosition();
                  if (nextStoredFrequency > -1)
                  {
                    frequency = nextStoredFrequency;
                    radio.selectFrequency(frequency);
                    radio_showTuningPositionLEDS();
                    SaveLastFrequencyToEEProm();
                  }
                  break;

                default:
                  break;
              }

              // get frequency
              radio_showTuningPositionLEDS();
              frequency = radio_getFrequency();
            } // BUTTON_press_time > CONST_DEFAULT_BUTTON_TIME - 30
          }
        }
        resetButtonValues();
      }
    }
  }
}

// function check
void menu_funccheck()
{
  if (button_press_enter == 1)
  {
    button_press_enter = 0;

    if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_APPNAME)
    {
      // back to the app
      TVScrollUp();
      inScreen = MENUMODE_APP; // set app mode
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETRADIOTUNING_SENSLVL_H)
    {
      radioTuningSensitivityLevel_LO_MED_HI = 2;
      if (radioONflag == 1) radio_setSearchLevel();
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_RADIO_AUTOTUNING_SENSVAL, radioTuningSensitivityLevel_LO_MED_HI);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETRADIOTUNING_SENSLVL_M)
    {
      radioTuningSensitivityLevel_LO_MED_HI = 1;
      if (radioONflag == 1) radio_setSearchLevel();
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_RADIO_AUTOTUNING_SENSVAL, radioTuningSensitivityLevel_LO_MED_HI);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETRADIOTUNING_SENSLVL_L)
    {
      radioTuningSensitivityLevel_LO_MED_HI = 0;
      if (radioONflag == 1) radio_setSearchLevel();
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_RADIO_AUTOTUNING_SENSVAL, radioTuningSensitivityLevel_LO_MED_HI);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETDISPLAYTIMEOUT_30MIN)
    {
      videoTimeoutValueInMinutes = 30;
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_VIDEOTIMEOUT_MINS, videoTimeoutValueInMinutes);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETDISPLAYTIMEOUT_10MIN)
    {
      videoTimeoutValueInMinutes = 10;
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_VIDEOTIMEOUT_MINS, videoTimeoutValueInMinutes);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETDISPLAYTIMEOUT_3MIN)
    {
      videoTimeoutValueInMinutes = 3;
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_VIDEOTIMEOUT_MINS, videoTimeoutValueInMinutes);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETDISPLAYTIMEOUT_1MIN)
    {
      videoTimeoutValueInMinutes = 1;
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_VIDEOTIMEOUT_MINS, videoTimeoutValueInMinutes);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETDISPLAYTIMEOUT_NEVER)
    {
      videoTimeoutValueInMinutes = 0;
      // save to eeprom
      radio1_SaveSettingValue(EEPROMLOCATION_VIDEOTIMEOUT_MINS, videoTimeoutValueInMinutes);
      Root.goBack();
      Root.goBack();
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETALARMTIME)
    {
      inScreen = MENUMODE_SETALARMTIME;
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETTIME)
    {
      inScreen = MENUMODE_SETTIME;
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_SETDATE)
    {
      inScreen = MENUMODE_SETDATE;
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_REBOOT)
      radio1_SoftwareReboot();
    else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_ABOUT)
    {
      inScreen = MENUMODE_ABOUT;
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_CLEARALLSETTINGS)
    {
      // clear eeprom
      inScreen =  MENUMODE_CLREEPROM;
      InitMode();
    } else if (Root.curfuncname == (const __FlashStringHelper*)MENUCONST_DOBENCHMARKS)
    {
      // benchmarks
      inScreen =  MENUMODE_BENCHMARKS;
      InitMode();
    }
  } // button = enter
}

// clears screen and shows a top level menu label
void clsAndShowMenuLabel(const prog_char * lbl)
{
  TV.clear_screen();

  // top line in menu
  TV.printPGM(6, 6, lbl);
  TV.draw_line(0, 14, SCREENWIDTH - 1, 14, WHITE);
}
////////////////////////////////////////////////////////////////////// END OF MENU CODE

__attribute__((always_inline)) static
void printBigNum(int x, int y, int theNum)
{ 
  if (theNum > -1)
    TV.bitmapfaster(x, y, BigNums[theNum], 11, 15);
  else
    // erase the block
    TV.draw_rectFastFill(x, y, 11, 15, BLACK);
}

// set time
void ShowTimeForEditing()
{
  int a = SCREENWID2 - 50;
  int b = 43;
  uint8_t dontShow = 0;

  // hour
  dontShow = ((timeEditCursorPos == 1) && (Xor1 == 1)) ? 1 : 0;
  if (dontShow == 0) printBigNum(a, b, (int)timeEditHour / 10); else printBigNum(a, b, -1);
  a = a + 13;
  if (dontShow == 0) printBigNum(a, b, (int)timeEditHour % 10); else printBigNum(a, b, -1);
  a = a + 13;

  TV.bitmapfaster(a, b, bignums_COLON, 11, 15);
  a = a + 13;

  // minute
  dontShow = ((timeEditCursorPos == 2) && (Xor1 == 1)) ? 1 : 0;
  if (dontShow == 0) printBigNum(a, b, (int)timeEditMinute / 10); else printBigNum(a, b, -1);
  a = a + 13;
  if (dontShow == 0) printBigNum(a, b, (int)timeEditMinute % 10); else printBigNum(a, b, -1);
  a = a + 13;

  TV.bitmapfaster(a, b, bignums_COLON, 11, 15);
  a = a + 13;

  // second
  dontShow = ((timeEditCursorPos == 3) && (Xor1 == 1)) ? 1 : 0;
  if (dontShow == 0) printBigNum(a, b, (int)timeEditSecond / 10); else printBigNum(a, b, -1);
  a = a + 13;
  if (dontShow == 0) printBigNum(a, b, (int)timeEditSecond % 10); else printBigNum(a, b, -1);
}

// set date
void ShowDateForEditing()
{
  int a = SCREENWID2 - 60;
  int b = 43;
  int c;
  int d;
  uint8_t dontShow = 0;

  // month
  dontShow = ((dateEditCursorPos == 1) && (Xor1 == 1)) ? 1 : 0;
  if (dontShow == 0) printBigNum(a, b, (int)dateEditMonth / 10); else printBigNum(a, b, -1);
  a = a + 13;
  if (dontShow == 0) printBigNum(a, b, (int)dateEditMonth % 10); else printBigNum(a, b, -1);
  a = a + 14;

  TV.printPGM(a, b + 6, CONST_FWDSLASH);
  a = a + 6;

  // day
  dontShow = ((dateEditCursorPos == 2) && (Xor1 == 1)) ? 1 : 0;
  if (dontShow == 0) printBigNum(a, b, (int)dateEditDay / 10); else printBigNum(a, b, -1);
  a = a + 13;
  if (dontShow == 0) printBigNum(a, b, (int)dateEditDay % 10); else printBigNum(a, b, -1);
  a = a + 14;

  TV.printPGM(a, b + 6, CONST_FWDSLASH);
  a = a + 6;

  // year
  dontShow = ((dateEditCursorPos == 3) && (Xor1 == 1)) ? 1 : 0;
  c = dateEditYear;
  d = c / 1000;

  if (dontShow == 0) printBigNum(a, b, d); else printBigNum(a, b, -1);
  a = a + 13;

  c = c - d * 1000;
  d = c / 100;
  if (dontShow == 0) printBigNum(a, b, d); else printBigNum(a, b, -1);
  a = a + 13;

  c = c - d * 100;
  d = c / 10;
  if (dontShow == 0) printBigNum(a, b, d); else printBigNum(a, b, -1);
  a = a + 13;

  if (dontShow == 0) printBigNum(a, b, (int)c % 10); else printBigNum(a, b, -1);
}

void inline ShowTimeBig(int x, int y)
{
  static uint8_t OLDdigit1;
  uint8_t h;
  uint8_t isMorning = 0;
  uint8_t digit1 = 0;
  uint8_t xpos;
  uint8_t ypos;
  int a;

  h = CustomDate.hr;
  if (h < 12)
  {
    isMorning = 1;
    if (h == 0) h = 12;
  } else {
    h = h - 12;
    if (h == 0) h = 12;
    isMorning = 0;
  }
  digit1 = (((int) h / 10) > 0) ? 1 : 0;

  if (digit1 == 0)
    x = x + 7;

  // clear time display region
  // if digit count changes
  if (digit1 != OLDdigit1)
    TV.draw_rectFastFill(x - 1, y - 11, 102, 26, BLACK);

  // back up old digit value
  OLDdigit1 = digit1;

  // show large time
  xpos = x;
  ypos = y;
  if (digit1 == 1)
  {
    printBigNum(xpos, ypos, (int) h / 10);
    xpos = xpos + 12;
  }
  printBigNum(xpos, ypos, (int) h % 10); // mod 10
  xpos = xpos + 11;

  TV.bitmapfaster(xpos, ypos - 1, bignums_COLON, 11, 15);

  xpos = xpos + 10;
  printBigNum(xpos, ypos, (int) CustomDate.mn / 10);
  xpos = xpos + 12;
  printBigNum(xpos, ypos, (int) CustomDate.mn % 10); // mod 10
  xpos = xpos + 11;

  if (Xor1 == 1)
    TV.bitmapfaster(xpos, ypos - 1, bignums_COLON, 11, 15);
  else
    TV.draw_rectFastFill(xpos + 1, ypos - 1, 4, 15, BLACK);

  xpos = xpos + 10;
  printBigNum(xpos, ypos, (int) CustomDate.sc / 10);
  xpos = xpos + 12;
  printBigNum(xpos, ypos, (int) CustomDate.sc % 10); // mod 10

  xpos = xpos + 11;
  ypos++;
  if (isMorning == 1)
    TV.printPGM(xpos + 4, ypos, CONST_AM);
  else
    TV.printPGM(xpos + 4, ypos, CONST_PM);

  ypos = ypos + 9;
  xpos = xpos + 4;

  // weekday name
  h = radio1_Weekday(CustomDate.mo, CustomDate.dy, CustomDate.yr);
  // large name
  if (h<7)
    TV.printPGM(x, y - 10, CONST_DAYSOFWEEK_LONGFMT[h]);

  // more compact code instead of using snprintf
  buf[0] = 0;
  char vnum[5];

  // month
  if (CustomDate.mo < 10) strcat_P(buf, CONST_ZERO);
  itoa(CustomDate.mo, vnum, 10);
  strcat(buf, vnum);
  strcat_P(buf, CONST_FWDSLASH);

  // day
  if (CustomDate.dy < 10) strcat_P(buf, CONST_ZERO);
  itoa(CustomDate.dy, vnum, 10);
  strcat(buf, vnum);
  strcat_P(buf, CONST_FWDSLASH);

  // year
  if (CustomDate.yr < 1000) strcat_P(buf, CONST_ZERO);
  if (CustomDate.yr < 100)  strcat_P(buf, CONST_ZERO);
  if (CustomDate.yr < 10)   strcat_P(buf, CONST_ZERO);
  itoa(CustomDate.yr, vnum, 10);
  strcat(buf, vnum);

  if (digit1 == 1)
    a = x + 61;
  else
    a = x + 49;

  TV.print(a, y - 10, buf);  
}

uint8_t doLedScrollForStatus()
{
  static uint8_t scrollPos = 0;
  static uint8_t scrollDir = 0;
  static unsigned long lastLedScrollStartMillis = 0;
  static unsigned long lastIntervalMillis = 0;

  // scroll the led while idle and the last activity is at least 5 seconds past
  if (thismillis - last_UIactivityMS > 5000)
  {
    // every 30 sec to begin a new led scroll effect
    if (thismillis < lastLedScrollStartMillis) lastLedScrollStartMillis = 0; // fix for 50 day rollover
    if (thismillis > lastLedScrollStartMillis + 30000)
    {
      // every CONST_LEDDELAY ms to switch leds in the effect (non-blocking)
      if (thismillis < lastIntervalMillis) lastIntervalMillis = 0; // fix for 50 day rollover
      if (thismillis < lastIntervalMillis + CONST_LEDDELAY)
        return 0;

      // set last interval ms
      lastIntervalMillis = thismillis;

      if (scrollDir == 3)
      {
        // we're finished
        lastLedScrollStartMillis = thismillis;

        // set the values
        scrollPos = 0;
        scrollDir = 0;
        // return processed
        return 1;
      }

      // initial setup
      if ((scrollPos == 0) && (scrollDir == 0))
      {
        scrollPos = lastLedToggled;
        scrollDir = 1;
      } else if (scrollDir == 1)
      {
        // go up from where we are
        scrollPos++;
        if (scrollPos == 7)
        {
          scrollDir = 0;
          scrollPos = 5;
        }
      } else if (scrollDir == 0)
      {
        // go dn from where we are
        scrollPos--;
        if (scrollPos == 0)
        {
          scrollDir = 2;
          scrollPos = 1;
        }
      } else if (scrollDir == 2)
      {
        // go up from where we are
        scrollPos++;
        if (scrollPos >= lastLedToggled)
        {
          // finished
          scrollDir = 3;
          scrollPos = lastLedToggled;
        }
      }

      // set the led
      uint8_t currentPulseLEDval = (int)pulseLedval;
      if (currentPulseLEDval < 1) currentPulseLEDval = 1;

      charlieleds_setledPWM(scrollPos, currentPulseLEDval);
      return 0;
    }
  }

  if (radioONflag == 1)
  {
    if (thismillis < last_UIactivityMS) last_UIactivityMS = 0; // 50 day rollover protection
    if (thismillis - last_UIactivityMS < 2000)
    {
      if (thismillis - last_UIactivityMS < 1000)
        radio_showTuningPositionLEDS();
      else
        radio_showSignalLevelInCharlieleds();
      return 1;
    }
  }

  // default, no leds were modified
  return 2;
}

void powerled_init()
{
  uint8_t a;

  pinModeFast(PIN_LED2, OUTPUT);
  for (a = 0; a < 4; a++)
  {
    TCCR4A &= ~_BV(COM4C1); //analogWrite(PIN_LED2, 0);
    radio1_Delay(CONST_LEDDELAY);
    analogWrite(PIN_LED2, 5);
    radio1_Delay(CONST_LEDDELAY);
  }
}

void charlieleds_init()
{
  uint8_t a;
  uint8_t currentPulseLEDval;

  currentPulseLEDval = (int)pulseLedval;
  if (currentPulseLEDval < 1) currentPulseLEDval = 1;

  // go up from where we are
  for (a = lastLedToggled; a < 7; a++)
  {
    // fast exit if sleep button pressed
    if (setSleep == 1) return;

    charlieleds_setledPWM(a, currentPulseLEDval);
    radio1_Delay(CONST_LEDDELAY);
  }

  // go all the way down
  for (a = 5; a > 0; a--)
  {
    // fast exit if sleep button pressed
    if (setSleep == 1) return;

    charlieleds_setledPWM(a, currentPulseLEDval);
    radio1_Delay(CONST_LEDDELAY);
  }

  // go back up to where we were originally
  for (a = 2; a <= lastLedToggled; a++)
  {
    // fast exit if sleep button pressed
    if (setSleep == 1) return;

    charlieleds_setledPWM(a, currentPulseLEDval);
    radio1_Delay(CONST_LEDDELAY);
  }
}

void charlieleds_countup()
{
  uint8_t a;

  for (a = 1; a < 7; a++)
  {
    charlieleds_setledPWM(a, LED_MAX_ILLUM);
    radio1_Delay(CONST_LEDDELAY);
  }
}

void charlieleds_countdown()
{
  uint8_t a;

  for (a = 6; a > 0; a--)
  {
    charlieleds_setledPWM(a, LED_MAX_ILLUM);
    radio1_Delay(CONST_LEDDELAY);
  }
}

void charlieleds_set_pinsPWM(uint8_t high_pin, uint8_t low_pin, uint8_t highval)
{
  // sanity check on highval is already done

  charlieleds_reset_pins();
  pinModeFast(high_pin, OUTPUT);
  pinModeFast(low_pin, OUTPUT);
  analogWrite(high_pin, highval);
  digitalWriteFast(low_pin, LOW);
}

void charlieleds_setledPWM(uint8_t lednum, uint8_t ledval)
{
  // sanity check
  if (ledval < 0)
    ledval = 0;
  else if (ledval > LED_MAX_ILLUM)
    ledval = LED_MAX_ILLUM;

  switch (lednum)
  {
    case 6:
      charlieleds_set_pinsPWM(PIN_CHARLIELED_3, PIN_CHARLIELED_2, ledval);
      break;
    case 5:
      charlieleds_set_pinsPWM(PIN_CHARLIELED_2, PIN_CHARLIELED_3, ledval);
      break;
    case 4:
      charlieleds_set_pinsPWM(PIN_CHARLIELED_3, PIN_CHARLIELED_1, ledval);
      break;
    case 3:
      charlieleds_set_pinsPWM(PIN_CHARLIELED_1, PIN_CHARLIELED_3, ledval);
      break;
    case 2:
      charlieleds_set_pinsPWM(PIN_CHARLIELED_2, PIN_CHARLIELED_1, ledval);
      break;
    case 1:
      charlieleds_set_pinsPWM(PIN_CHARLIELED_1, PIN_CHARLIELED_2, ledval);
      break;
  }
}

__attribute__((always_inline)) static
void charlieleds_reset_pins()
{
  //pinModeFast(PIN_CHARLIELED_1, INPUT);
  DDRE &= ~(1 << (5));
  PORTE &= ~(1 << (5));
    
  //pinModeFast(PIN_CHARLIELED_2, INPUT);
  DDRG &= ~(1 << (5));
  PORTG &= ~(1 << (5));
    
  //pinModeFast(PIN_CHARLIELED_3, INPUT);
  DDRE &= ~(1 << (3));
  PORTE &= ~(1 << (3));
}

void radio_init()
{
  // defines for setting and clearing register bits
  #ifndef cbi
    #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
  #endif
  #ifndef sbi
    #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
  #endif
  
  // sets 4 prescaler bits on ADC
  cbi(ADCSRA, ADPS2);
  sbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  
  // set up TDA volume pins first and set zero volume
  pinModeFast(PIN_AMPVOLUME_R, OUTPUT);
  pinModeFast(PIN_AMPVOLUME_L, OUTPUT);
  digitalWriteFast(PIN_AMPVOLUME_R, LOW);
  digitalWriteFast(PIN_AMPVOLUME_L, LOW);

  // fire up radio and amps
  pinModeFast(PIN_RADIOPOWER, OUTPUT);
  digitalWriteFast(PIN_RADIOPOWER, HIGH);
  radio1_Delay(10);
  
  // do this instead on C5 hardware
  radio.setStandByOff();

  radio.mute();
  radio.setSoftMuteOn();
  radio.setStereoReception();
  radio.setStereoNoiseCancellingOn();
  radio.setHighCutControlOn();

  // set search level
  radio_setSearchLevel();

  radio1_Delay(10);

  radio.selectFrequency(frequency);

  // wait for the frequency to be tuned so we don't get squelch
  while (!radio.isReady())
  {};

  radio.turnTheSoundBackOn();

  // read radio position
  frequency = radio_getFrequency();

  // now set the TDA volume pin PWM (rev.B/C only)

  // back up original value
  uint8_t origVolumeLevel = systemVolumeLevel;
  // ramp up
  for (int a = 0; a < systemVolumeLevel; a += 3)
  {
    systemVolumeLevel = a;
    radio_setVolumePWM();
    radio1_Delay(2);
  }
  // restore original value
  systemVolumeLevel = origVolumeLevel;
  radio_setVolumePWM();

  // set radio initialized
  radioONflag = 1;
}

float radiomemory_getNextStoredMemoryPosition()
{
  int didfind = -1;
  static uint8_t lastPosition = 0;
  uint8_t numSearches = 0;
  float storedFrequency;

  do
  {
    lastPosition++;
    if (lastPosition > 9) lastPosition = 0;

    // get stored frequency for memory position
    storedFrequency = radio1_GetSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_REAL + (lastPosition * 2), 0);
    storedFrequency += ((float)radio1_GetSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_DEC + (lastPosition * 2), 0) / 10.0f);
    if ((storedFrequency >= 88.0f) && (storedFrequency <= 108.0f))
    {
      // got one
      didfind = 1;
      break;
    }
    numSearches++;
  } while (numSearches <= 10);

  if (didfind > -1)
    return storedFrequency;
  else
    return -1;
}

int radiomemory_getMemoryPositionForCurrentFrequency()
{
  int didfind = -1;

  for (int i = 0; i < 10; i++)
  {
    // get stored frequency for memory position
    float storedFrequency = radio1_GetSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_REAL + (i * 2), 0);
    storedFrequency += ((float)radio1_GetSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_DEC + (i * 2), 0) / 10.0f);
    if (storedFrequency == frequency)
    {
      // match
      didfind = i;
      break;
    }
  }
  return didfind;
}

void ShowMemoryStatusForCurrentFrequency()
{
  char buf[6];

  // get memory position for current radio frequency
  int didfind = radiomemory_getMemoryPositionForCurrentFrequency();
  
  if (didfind > -1)
  {
    // got one, show the memory position
    strcpy_P(buf, CONST_MEM); // "MEM "
    buf[4] = didfind + 48;
  } else
  {
    // no memory position, just clear the region
    memset(&buf[0], 32, 5); // 5 spaces
  }
  buf[5] = 0;
  TV.print(SCREENWID2 - ((4 * 5) / 2), 16, buf);
}

void radiomemory_toggle()
{
  // lookup current frequency

  uint8_t i;
  int didfind = radiomemory_getMemoryPositionForCurrentFrequency();

  if (didfind > -1)
  {
    // delete the storage record
    radio1_SaveSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_REAL + (didfind * 2), 255);
    radio1_SaveSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_DEC + (didfind * 2), 255);
  }
  else
  {
    // create a new preset for this frequency

    // find first unused record
    for (i = 0; i < 10; i++)
    {
      if (radio1_GetSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_REAL + (i * 2), 0) == 0)
      {
        // found one
        didfind = i;
        break;
      }
    }

    if (didfind > -1)
    {
      // store it

      // integer part
      uint8_t fr = (int)frequency;

      // decimal part (uses dtostrf for accuracy)
      char fq[8];
      uint8_t decimalDigit;

      dtostrf(frequency, 3, 1, fq);

      if ((fq[0] == '1') && (fq[3] == '.')) // if (frequency >= 100.0f)
        decimalDigit = fq[4] - 48;
      else
        decimalDigit = fq[3] - 48;

      radio1_SaveSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_REAL + (didfind * 2), fr);
      radio1_SaveSettingValue(EEPROMLOCATION_RADIO_PRESET_BASE_FREQUENCY_DEC + (didfind * 2), decimalDigit);
    }
  }
}

void radio_setSearchLevel()
{
  switch (radioTuningSensitivityLevel_LO_MED_HI)
  {
    case 0:
      radio.setSearchLowStopLevel();
      break;

    case 1:
    default:
      radio.setSearchMidStopLevel();
      break;

    case 2:
      radio.setSearchHighStopLevel();
      break;
  }
}

// this function exists because the tuner
// may return uneven freqencies, ie: 91.29 instead of 91.3.
float radio_getFrequency()
{
  float rawFreq = radio.readFrequencyInMHz();
  long rawIntpart = floor(rawFreq);
  float rawDecimal = rawFreq - rawIntpart;
  
  uint8_t secondDecimal = floor(((rawDecimal * 10.0f) - floor(rawDecimal * 10)) * 10.0f);
  if (secondDecimal >= 5)
    return rawIntpart + ((floor(rawDecimal * 10.0f) + 1) / 10.0f);
  else
    return rawIntpart + (floor(rawDecimal * 10.0f) / 10.0f);
}

void inline radio_setVolumePWM()
{  
  int newLevel = (int)( (25.0 * 6.5) + ((float)systemVolumeLevel * 9.5) );
  if (newLevel > 950) newLevel = 950;
  
  // we compensate for added volume due to timed out video
  // this is an artifact of the C5 hardware design
  if (videoIStimedout == 1) newLevel -=25;
  if (newLevel < 0) newLevel = 0;

  // SET PWM FOR VOLUME LEVEL (TDA7052A)
  Timer5.pwm(PIN_AMPVOLUME_L, newLevel, 30); //  left amp pin 44 - duty (1024 is 100%) - 30us
  Timer5.pwm(PIN_AMPVOLUME_R, newLevel, 30); // right amp pin 45 - duty (1024 is 100%) - 30us
}

void radio_shutdown()
{
  // quickly fade the volume out
  // ramp down
  for (int a = systemVolumeLevel; a > 0; a -= 3)
  {
    systemVolumeLevel = a;
    radio_setVolumePWM();
    radio1_Delay(2);
  }

  // turn off the radio power
  // C5 hardware behaves differently, do not uncomment
  // digitalWriteFast(PIN_RADIOPOWER, LOW);
  // pinModeFast(PIN_RADIOPOWER, INPUT); // set pin to input to reduce power consumption
  
  // do this instead on C5 hardware
  radio.setStandByOn();

  // disable the amp volume control pwm on both pins
  Timer5.disablePwm(PIN_AMPVOLUME_L);
  Timer5.disablePwm(PIN_AMPVOLUME_R);
  digitalWriteFast(PIN_AMPVOLUME_L, LOW);
  digitalWriteFast(PIN_AMPVOLUME_R, LOW);
  pinModeFast(PIN_AMPVOLUME_L, INPUT);
  pinModeFast(PIN_AMPVOLUME_R, INPUT);
  // stop the volume timer
  Timer5.stop();

  // set flag
  radioONflag = 0;
}

// displays signal strength to the 6 charlieplexed leds
void radio_showSignalLevelInCharlieleds()
{
  uint8_t z;

  // show signal level (max 10, only 6 leds)
  uint8_t signal_level = floor(((float)radioSignalStrength) / 2.5f);
  if (signal_level > 6)
    signal_level = 6;
  else if (signal_level < 1)
    signal_level = 1;

  // get time division within second and led position
  static uint8_t lastledposition = 0;
  lastledposition++;
  if (lastledposition > signal_level) lastledposition = 1;

  // brightness based on position within signal level
  z = floor(LED_MAX_ILLUM * (float)((float)lastledposition / (float)signal_level));
  charlieleds_setledPWM(lastledposition, z);
}

void radio_showTuningPositionLEDS()
{
  static const float CONST_FREQSPREAD = 108.0 - 88.0;
  static const float CONST_FADE_INTERVAL = 0.13;
  static const int CONST_HOLDMAXVAL = 500;
  static int holdMaxValueCount = 0;
  static uint8_t lastsec = 0;
  static uint8_t dir = 1;

  // get frequency once a second
  if (CustomDate.sc != lastsec)
  {
    if (radioONflag == 1)
    {
      lastsec = CustomDate.sc;
      if (radio.isReady()) frequency = radio_getFrequency();
    }
  }

  float freqPos = frequency;
  if (freqPos > 108.0) freqPos = 108.0;
  else if (freqPos < 88.0) freqPos = 88.0;

  freqPos = (freqPos - 88.0) / CONST_FREQSPREAD;
  freqPos = 1 + floor(6.0 * freqPos);
  if (freqPos > 6.0)
    freqPos = 6;

  lastLedToggled = freqPos;

  charlieleds_setledPWM((int)freqPos, pulseLedval);

  if (dir == 1)
  {
    pulseLedval = pulseLedval + CONST_FADE_INTERVAL;
    if (pulseLedval > LED_MAX_ILLUM)
    {
      pulseLedval = LED_MAX_ILLUM;
      // delay when we reach the maximum illumination
      holdMaxValueCount++;
      if (holdMaxValueCount > CONST_HOLDMAXVAL)
      {
        // reset
        holdMaxValueCount = 0;
        dir = 0;
      }
    }
  }
  else
  {
    pulseLedval = pulseLedval - CONST_FADE_INTERVAL;
    if (pulseLedval < 1)
    {
      pulseLedval = 0;
      dir = 1;
    }
  }
}

// test if -0.1 or +0.1 is better in signal
// 6.3.16, 6.9.16
void radio_fixFrequencyForSignalCenter(uint8_t searchUp1Dn0)
{
  static const uint8_t CONST_NUMSEARCHES = 3;
  uint8_t signalValueForThisFrequency[CONST_NUMSEARCHES] = {0};
  float searchOffset;
  uint8_t i;

  // choose 100khz offset value based on the search up or down
  if (searchUp1Dn0 == 1)
    searchOffset = 0.1f;
  else
    searchOffset = -0.1f;

  frequency += -searchOffset;

  // search offset * CONST_NUMSEARCHES for strongest signal
  for (i = 0; i < CONST_NUMSEARCHES; i++)
  {
    radio.selectFrequency(frequency + (searchOffset * i)); // 0 will be the starting frequency
    // get signal level twice (necessary!)
    signalValueForThisFrequency[i] = radio.getSignalLevel();
    TV.delay_frame(1);
    signalValueForThisFrequency[i] = radio.getSignalLevel();
  }

  // find strongest signal in our array
  float strongestFrequency = 0;
  uint8_t strongestSignalValue = 0;

  for (i = 0; i < CONST_NUMSEARCHES; i++)
  {
    if (signalValueForThisFrequency[i] > strongestSignalValue)
    {
      strongestFrequency = frequency + (searchOffset * i);
      strongestSignalValue = signalValueForThisFrequency[i];
    }
  }

  // now assign the frequency
  if (strongestFrequency != frequency)
  {
    frequency =  strongestFrequency;

    // reset or change to the new frequency
    radio.selectFrequency(frequency);
  }

  ShowFrequencyAndVolume();

  // save last frequency to eeprom
  SaveLastFrequencyToEEProm();
}

void SaveLastFrequencyToEEProm()
{
  // integer part
  uint8_t fr = (int)frequency;

  // decimal part (uses dtostrf for accuracy)
  char fq[8];
  uint8_t decimalDigit;

  dtostrf(frequency, 3, 1, fq);

  if ((fq[0] == '1') && (fq[3] == '.')) // if (frequency >= 100.0f)
    decimalDigit = fq[4] - 48;
  else
    decimalDigit = fq[3] - 48;

  radio1_SaveSettingValue(EEPROMLOCATION_LASTFREQUENCY_REAL, fr);
  radio1_SaveSettingValue(EEPROMLOCATION_LASTFREQUENCY_DEC, decimalDigit);
}

// custom search
void radio_searchUpDn(uint8_t searchUp1_Dn0)
{
  radio.mute();

  // default move, so we don't end up in the wake of our current station
  if (searchUp1_Dn0 == 1)
    frequency += 0.2;
  else
    frequency -= 0.2;

  while (1)
  {
    // break the search if a key is pressed
    if (radio1_analogReadFast(PIN_CHARLIEKBD) < 1000) break;

    // break the search if the sleep button was pressed
    if (setSleep == 1) return; // <-- returns without restoring the volume

    if (searchUp1_Dn0 == 1)
    {
      frequency += 0.1;
      if (frequency > 108.0) frequency = 88.0;
    } else
    {
      frequency -= 0.1;
      if (frequency < 88.0) frequency = 108.0;
    }

    radio.selectFrequency(frequency);

    // update UI if necessary
    if (videoONflag == 1)
      ShowFrequencyAndVolume();

    radio_showTuningPositionLEDS();

    // check the signal level twice (necessary!)
    radioSignalStrength = radio.getSignalLevel();
    TV.delay_frame(1);
    radioSignalStrength = radio.getSignalLevel();

    if (radioSignalStrength >= ((radioTuningSensitivityLevel_LO_MED_HI + 1) * 4)) // low=4, med=8, hi=12
    {
      // we have a station acceptable for our tuning sensitivity,
      // re-center the frequency based on the signal level
      radio_fixFrequencyForSignalCenter(searchUp1_Dn0);
      break;
    }
  }
  radio.turnTheSoundBackOn();
}

// make sure the i2c devices are properly connected
void i2cdetect()
{
  uint8_t didSetPosition = 0;

  Wire.begin();

  // find radio
  Wire.beginTransmission(0x60);
  if (Wire.endTransmission() != 0)
  {
    // radio problem, bad solder joint?
    TV.printPGM(SCREENWID2 - ((10 * 4) / 2), SCREENHEIGHT - 17, MENUCONST_APPNAME);
    TV.printPGM(CONST_1SPACE);
    TV.printPGM(CONST_ERR);
    didSetPosition = 1;
  }

  charlieleds_setledPWM(3, LED_MAX_ILLUM);

  // find RTC
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() != 0)
  {
    // rtc problem, could be a dead 3v battery
    if (didSetPosition == 0)
      TV.printPGM(SCREENWID2 - ((9 * 4) / 2), SCREENHEIGHT - 17, CONST_CLOCK);
    else
      TV.printPGM(CONST_CLOCK);

    TV.printPGM(CONST_ERR);
    // show check battery message
    TV.printPGM(SCREENWID2 - ((13 * 4) / 2), SCREENHEIGHT - 9, CONST_CHECKBATTERY);
  }

  charlieleds_setledPWM(4, LED_MAX_ILLUM);

  radio1_Delay(1500);
}  // end of I2CDETECT

void clock_init()
{
  // power on clock
  pinModeFast(PIN_RTCPOWER, OUTPUT);
  digitalWriteFast(PIN_RTCPOWER, HIGH);
  delayMicroseconds(100);

  RTC.initialize();
  radio1_Delay(10);

#define FORCETIMESET 0 // change to fix settings until we have the menu up

  setSyncProvider(RTC.get);

  if ((timeStatus() != timeSet) || (FORCETIMESET == 1))
  {
    // set the default date/time
    time_t t;
    tmElements_t DStimestruct;

    DStimestruct.Year = CalendarYrToTm(2016);
    DStimestruct.Month = 10;
    DStimestruct.Day = 10;
    DStimestruct.Hour = 0;
    DStimestruct.Minute = 0;
    DStimestruct.Second = 0;
    t = makeTime(DStimestruct);

    RTC.set(t); // use the time_t value to ensure correct weekday is set
    setTime(t);
  }

  // populate the custom time structure
  radio1_GetDateAndTimeFromRTC();
}

time_t tLast;
void clock_checktick()
{
  time_t timeT = RTC.get();
  if (timeT == tLast)
    return;

  tLast = timeT;

  uint8_t sec = (second(timeT) % 6) + 1;
  charlieleds_setledPWM(sec, LED_MAX_ILLUM);
}

void tv_shutdown()
{
  if (videoONflag == 0) return;

  TVFadeOut();

  PORTL &= ~(1 << (7)); // digitalWriteFast(PIN_MONITORPOWER, LOW);
  pinModeFast(PIN_MONITORPOWER, INPUT); // set pin to input to reduce power consumption

  TV.end();
  videoONflag = 0;
}

void tv_init()
{
  uint8_t i;

  if (videoONflag == 1)
    return;

  // turn it on
  pinModeFast(PIN_MONITORPOWER, OUTPUT);
  PORTL |= (1 << (7)); //digitalWriteFast(PIN_MONITORPOWER, HIGH);

  // prepare tv library
  i = TV.begin(NTSC, SCREENWIDTH, SCREENHEIGHT);

  // if TV returns 4 there isn't enough memory for it to begin
  if (i == 4)
    beepErrorLoop(); // don't proceed past this error beeping

  TV.clear_screen();
  TV.select_font(font4x6);
  
  // reset video timeout
  videoIStimedout = 0;
  // set video on
  videoONflag = 1;
}

void beepErrorLoop()
{
  uint8_t i;

  while (true) // don't proceed past this error beeping
  {
    for (i = 0; i < 2; i++)
    {
      tone(PIN_SPK, 700, 100);
      delay (400);
    }
    radio1_Delay(3000);
  }
}

void ShutOffADC()
{
  ACSR = (1 << ACD);                      // disable A/D comparator
  ADCSRA = (0 << ADEN);                   // disable A/D converter
  DIDR0 = 0x3f;                           // disable all A/D inputs (ADC0-ADC5)
  DIDR1 = 0x03;                           // disable AIN0 and AIN1
}

// callback for the sleep button while running
// just sets a flag that we catch in the main loop
void isr_sleepbtn()
{
  // shut off sleep button
  detachInterrupt(0);
  setSleep = 1;
}

// shut down everything and put the board to sleep
// if the alarm is enabled, turn on the watchdog timer and wake up every 8 seconds to check the remaining interval
// if no match, go back to sleep
// interval = hours + minutes remaining to alarm / 8 sec
//
void doSleep()
{
  long remainingWakesIfAlarmEnabled = 0;

  // reset
  setSleep = 0;
  awokeBySleepButton = 0;

  // shut off radio first
  if (radioONflag == 1) radio_shutdown();

  // show the sleep screen
  if (videoONflag == 1)
  {
    TVScrollUp();
    TV.clear_screen();
    TV.bitmapfaster(SCREENWID2 - 16, SCREENHEI2 - 16, SleepImg, 32, 32);
  }

  // flash powerled
  powerled_init();

  // shut off power led
  TCCR4A &= ~_BV(COM4C1); //digitalWriteFast(PIN_LED2, LOW);
  pinModeFast(PIN_LED2, INPUT);

  radio1_Delay(100);

  // shut off tv
  if (videoONflag == 1) tv_shutdown();

  // shut off clock
  digitalWriteFast(PIN_RTCPOWER, LOW);
  pinModeFast(PIN_RTCPOWER, INPUT); // set pin as input to reduce power

  charlieleds_countdown();
  charlieleds_reset_pins();

  // necessary debouncing
  radio1_Delay(500);

  // if alarm is enabled, set up the remaining wakes
  if (alarmEnabled == 1)
  {
    // calculate current time in minutes
    long cTime = (CustomDate.hr * 60) + CustomDate.mn;
    // make seconds
    cTime = (cTime * 60) + CustomDate.sc;

    // calculate alarm time in minutes
    long aTime = (alarmHour * 60) + alarmMinute;
    // make seconds
    aTime = aTime * 60;

    // calculate difference in seconds
    long diffSec = (cTime > aTime) ? 86400 - (cTime - aTime)  : aTime - cTime;
    // get the divisions of 8 seconds, this is how many times we need to wake up until our alarm is ready
    remainingWakesIfAlarmEnabled = floor((float)diffSec / 8.0f);
    if (remainingWakesIfAlarmEnabled < 1) remainingWakesIfAlarmEnabled = 1;
  }

gobacktosleep:
  setIdle();

  // returned from sleep

  // if we awoke by the sleep button being pressed then ignore the alarm wakes interval
  if (awokeBySleepButton == 0)
  {
    // if alarm enabled, check to see if ready
    // otherwise go back to sleep
    if (alarmEnabled == 1)
    {
      remainingWakesIfAlarmEnabled--;
      if (remainingWakesIfAlarmEnabled > 0)
        goto gobacktosleep;
      else
        // set alarm triggered flag (we check it again further down)
        alarmTriggered = 1;
    }
  }

  // reinitialize hardware
  radio1_inithardware();

  if (appScreenMode == SCREENMODE_RADIO)
    openRadioMode();

  // reset flickers
  Xor1 = 0;

  // now that the hardware is initialized, check the alarm state
  // and enable things as necessary
  if ((alarmEnabled == 1) && (alarmTriggered == 1))
    alarm_setTriggered();

  // set up the initial UI for the mode (menu/app/settings/etc)
  InitMode();

  // reset last UI activity
  last_UIactivityMS = radio1_Millis();
}

// callback to sleep button interrupt
void wake()
{
  // set how we woke up
  awokeBySleepButton = 1;
}

// new sleep code 6.18.16
void setIdle()
{
  // temporary clock source variable
  unsigned char clockSource = 0;

  // set up sleep button to wake
  attachInterrupt(0, wake, LOW);  // wake up on low level

  // turn off brown-out enable in software
  #define BODS   7  // BOD Sleep bit in MCUCR
  #define BODSE  2  // BOD Sleep enable bit in MCUCR
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);

  // disable timer2
  if (TCCR2B & CS22) clockSource |= (1 << CS22);
  if (TCCR2B & CS21) clockSource |= (1 << CS21);
  if (TCCR2B & CS20) clockSource |= (1 << CS20);

  // remove the clock source to shutdown Timer2
  TCCR2B &= ~(1 << CS22);
  TCCR2B &= ~(1 << CS21);
  TCCR2B &= ~(1 << CS20);

  power_timer2_disable();

  // disable ADC
  //ShutOffADC();
  ADCSRA &= ~(1 << ADEN);
  power_adc_disable();

  // turn off various modules
  power_all_disable();

  // disable all timers
  power_timer5_disable();
  power_timer4_disable();
  power_timer3_disable();
  power_timer1_disable();
  power_timer0_disable();
  power_spi_disable();
  //power_usart3_disable();
  power_usart2_disable();
  power_usart1_disable();
  power_usart0_disable();
  power_twi_disable();

  // if the alarm is enabled, set up the watchdog timer
  // timer wakes from sleep every 8 seconds to check the time for an alarm
  if (alarmEnabled == 1)
    setup_watchdog(WDTO_8S); // watchdog new

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
  sei();
  sleep_cpu(); // nighty nite

  // - * -

  // we wake up here
  sleep_disable();
  detachInterrupt(0); // stop LOW interrupt

  // if the alarm is enabled, disable the watchdog timer
  if (alarmEnabled == 1)
  {
    wdt_reset(); // watchdog new
    wdt_disable();
  }

  sei();

  // turn the adc back on
  power_adc_enable();
  ADCSRA |= (1 << ADEN);

  // enable timer2
  if (clockSource & CS22) TCCR2B |= (1 << CS22);
  if (clockSource & CS21) TCCR2B |= (1 << CS21);
  if (clockSource & CS20) TCCR2B |= (1 << CS20);
  power_timer2_enable();

  // enable timers
  power_timer5_enable();
  power_timer4_enable();
  power_timer3_enable();
  power_timer1_enable();
  power_timer0_enable();

  // enable spi, uarts & i2c
  power_spi_enable();
  //power_usart3_enable();
  power_usart2_enable();
  power_usart1_enable();
  power_usart0_enable();
  power_twi_enable();
}
// end of new sleep code

// main hardware init
void radio1_inithardware()
{
  static uint8_t notFirstBoot = 0;

  // main power led
  powerled_init();

  charlieleds_setledPWM(1, LED_MAX_ILLUM);

  // set up speaker
  pinModeFast(PIN_SPK, OUTPUT);

  // set up reset trigger pin
  pinModeFast(PIN_RESET, INPUT);

  // set up analog tuning input
  pinModeFast(PIN_ANALOGVOLUME, INPUT);

  // set up mosfet power to radio
  //
  // in rev.C5 this must be turned on before
  // initializing any other i2c device
  pinModeFast(PIN_RADIOPOWER, OUTPUT);
  digitalWriteFast(PIN_RADIOPOWER, HIGH);

  // set up mosfet power for TV
  pinModeFast(PIN_MONITORPOWER, OUTPUT);
  PORTL &= ~(1 << (7)); //digitalWriteFast(PIN_MONITORPOWER, LOW);

  // turn on TV
  tv_init();

  radio1_Delay(200);

  if (notFirstBoot == 0)
  {
    // show the boot logo
    TV.bitmapfaster(SCREENWID2 - (96 / 2), SCREENHEI2 - (64 / 2), bootLogo, 96, 64);

    charlieleds_setledPWM(2, LED_MAX_ILLUM);

    radio1_Delay(1000);

    i2cdetect();

    // scroll screen up
    TVScrollUp();

    // set not first boot
    notFirstBoot = 1;
  } else
    TV.clear_screen();

  // init clock
  charlieleds_setledPWM(5, LED_MAX_ILLUM);
  clock_init();

  // read the alarm values from eeprom
  alarmEnabled = radio1_GetSettingValue(EEPROMLOCATION_ALARM_ENABLED, 0);
  if (alarmEnabled > 1) alarmEnabled = 0; // sanity check
  alarmHour = radio1_GetSettingValue(EEPROMLOCATION_ALARMHOUR, 7); // default 7am alarm value
  if (alarmHour > 23) alarmHour = 7;
  alarmMinute = radio1_GetSettingValue(EEPROMLOCATION_ALARMMINUTE, 0);
  if (alarmMinute > 59) alarmMinute = 0;

  // read the last frequency value from eeprom (default = 88.5)
  frequency = radio1_GetSettingValue(EEPROMLOCATION_LASTFREQUENCY_REAL, 88);
  frequency += ((float)radio1_GetSettingValue(EEPROMLOCATION_LASTFREQUENCY_DEC, 5) / 10.0f);
  // sanity check
  if ((frequency < 88.0f) || (frequency > 108.0f))
    frequency = 88.5f;

  // read the stored video timeout setting
  videoTimeoutValueInMinutes = radio1_GetSettingValue(EEPROMLOCATION_VIDEOTIMEOUT_MINS, 30); // default 30 minutes

  // read the stored radio tuning sensitivity
  radioTuningSensitivityLevel_LO_MED_HI = radio1_GetSettingValue(EEPROMLOCATION_RADIO_AUTOTUNING_SENSVAL, 1); // default medium level

  charlieleds_setledPWM(6, LED_MAX_ILLUM);
  radio1_Delay(200);

  // charlieplexed leds
  charlieleds_init();
  charlieleds_setledPWM(1, LED_MAX_ILLUM);

  // set sleep button
  pinModeFast(PIN_SLEEP, INPUT_PULLUP);
  attachInterrupt(0, isr_sleepbtn, LOW); // set sleep button interrupt

  // reset all stored button values
  memset(&butpressed[0], 0, 5);
  gBUTTON_press_time = radio1_Millis();
}

void ShowFrequencyAndVolume()
{
  int showVol = 0;

  if (thismillis < last_volumeAdjustmentMS) last_volumeAdjustmentMS = 0; // 50 day rollover protection
  if (radio1_Millis() - last_volumeAdjustmentMS < 4000)
    showVol = 1;

  // get signal strength before clearing screen to avoid jitter
  uint8_t signalStr = (radioSignalStrength + 1) / 4; // src=0-15 = out=1-4

  // removes flicker
  TV.delay_frame(1);

  // header
  TV.draw_rectFastFill(1, 1, SCREENWIDTH - 1, 5, BLACK);
  TV.printPGM(1, 1, MENUCONST_APPNAME);

  // write volume buffer
  if (showVol == 1)
  {
    static const uint8_t VOLPOINTS = 30;
    uint8_t volval = systemVolumeLevel / (100 / VOLPOINTS);
    uint8_t xpos;

    if (volval > 0)
    {
      volval = volval / 2;
      // left lines
      for (int i = 0; i < volval; i++)
      {
        xpos = (SCREENWID2 - (4 * 3)) - (i * 2);
        TV.draw_line(xpos, 1, xpos, 5, WHITE);
      }
      // right lines
      for (int i = 0; i < volval; i++)
      {
        xpos = (SCREENWID2 + (4 * 3)) + (i * 2) - 3;
        TV.draw_line(xpos, 1, xpos, 5, WHITE);
      }
      // draw  centered 'VOL'
      TV.printPGM(SCREENWID2 - ((3 * 4) / 2), 1, CONST_VOL);
    } else
    {
      // draw  centered 'MUTE'
      TV.printPGM(SCREENWID2 - ((4 * 4) / 2), 1, CONST_MUTE);
    }
  }

  if (alarmEnabled == 1)
  {
    // draw alarm icon
    TV.printPGM(SCREENWIDTH - 23, 1, CONST_ALARMICON);
  }

  // show signal strength bars
  if (signalStr > 0) TV.draw_rectFastFill(SCREENWIDTH - 15, 4, 2, 1, WHITE);
  if (signalStr > 1) TV.draw_rectFastFill(SCREENWIDTH - 11, 3, 2, 2, WHITE);
  if (signalStr > 2) TV.draw_rectFastFill(SCREENWIDTH - 7,  2, 2, 3, WHITE);
  if (signalStr > 3) TV.draw_rectFastFill(SCREENWIDTH - 3,  1, 2, 4, WHITE);

  // draw separator line
  TV.draw_line(1, 8, SCREENWIDTH, 8, WHITE);

  // radio frequency
  ShowBigFrequency();

  // show mode
  TV.printPGM(SCREENWID2 - ((2 * 4) / 2), 45, CONST_FM);

  // draw separator line
  TV.draw_line(1, 60, SCREENWIDTH, 60, WHITE);
}

void ShowBigFrequency()
{
  int posInDstrtof = 0;
  uint8_t isOver100 = 0;

  // kludges for avr floating point crappyness
  char fq[8];
  dtostrf(frequency, 3, 1, fq);

  if ((fq[0] == '1') && (fq[3] == '.')) isOver100 = 1;

  uint8_t fpos = (isOver100 == 1) ? ((13 * 4) / 2) + 3 : ((13 * 3) / 2) + 3;

  uint8_t ypos = 25;
  uint8_t xpos = SCREENWID2 - fpos;

  // zero the draw region
  TV.draw_rectFastFill(0, ypos, SCREENWIDTH - 1, 15, BLACK);

  // 100s digit
  if (isOver100 == 1)
  {
    printBigNum(xpos, ypos, 1);
    xpos += 13;
    posInDstrtof = 1;
  }

  // first or second digit
  printBigNum(xpos, ypos, fq[posInDstrtof] - 48);
  xpos += 13;
  posInDstrtof++;

  // second or third digit
  printBigNum(xpos, ypos, fq[posInDstrtof] - 48);
  xpos += 13;
  posInDstrtof += 2;

  // decimal separator
  TV.draw_circle(xpos + 1, ypos + 13, 1, WHITE, 1);
  xpos += 5;

  // decimal digit
  printBigNum(xpos, ypos, fq[posInDstrtof] - 48);
}

void TVFadeOut()
{
  int i, j;

  // fade out
  for (i = 0; i < SCREENWIDTH; i++)
    for (j = 0; j < SCREENHEIGHT; j++)
    {
      TV.set_pixel(i, j, BLACK);
      delayMicroseconds(1);
    }
}

void TVScrollUp()
{
  int i;

  // scroll up
  TV.draw_line(0, SCREENHEIGHT - 1, SCREENWIDTH - 1, SCREENHEIGHT - 1, WHITE); // for effect
  for (i = 0; i <= SCREENHEIGHT / 4; i++)
  {
    TV.shift(4, 0); // shift(uint8_t distance, uint8_t direction)
    if ((i % 3) == 0) radio1_Delay(1);
  }
  TV.clear_screen();
}

void TVScrollDn()
{
  int i;

  // scroll dn
  TV.draw_line(0, 0, SCREENWIDTH - 1, 0, WHITE); // for effect
  for (i = 0; i <= SCREENHEIGHT / 4; i++)
  {
    TV.shift(4, 1); // shift(uint8_t distance, uint8_t direction)
    if ((i % 3) == 0) radio1_Delay(1);
  }
  TV.clear_screen();
}

void TVOpenCloseAppWindow(uint8_t direction1open)
{
  int z;

  if (direction1open == 1)
  {
    // open
    for (z = 0; z <= 60; z += 4)
    {
      TV.draw_rectFastFill(0, 0, SCREENWIDTH, z, BLACK);
      TV.draw_line(0, z + 1, SCREENWIDTH, z + 1, WHITE);
      radio1_Delay(10);
    }
    TV.draw_rectFastFill(0, 0, SCREENWIDTH, 59, BLACK);
  }
  else
  {
    // close
    for (z = 59; z >= 0; z -= 4)
    {
      TV.draw_rectFastFill(0, z, SCREENWIDTH, 59, BLACK);
      TV.draw_line(0, z - 1, SCREENWIDTH, z - 1, WHITE);
      radio1_Delay(10);
    }
    TV.draw_rectFastFill(0, 0, SCREENWIDTH, 59, BLACK);
  }
}

void TVOpenWindow(uint8_t direction1open)
{
  uint8_t x;     uint8_t xdiv = SCREENWID2 / 20;
  uint8_t y;     uint8_t ydiv = SCREENHEI2 / 20;
  uint8_t z;

  y = SCREENHEI2 - 1;
  x = SCREENWID2 - 1;

  if (direction1open == 1)
  {
    for (z = 0; z <= 20; z++)
    {
      TV.draw_rect(x - (z * xdiv), y - (z * ydiv), (z * xdiv) * 2, (z * ydiv) * 2, WHITE, -1);
      radio1_Delay(10);
      TV.clear_screen();
    }
  } else
  {
    for (z = 20; z > 0; z--)
    {
      radio1_Delay(10);
      TV.clear_screen();
    }
  }
}

void Update(uint8_t refreshBG)
{
  // main update is done here every minute

  if (videoONflag == 0)
    return;

  if (refreshBG == 1)
    TV.clear_screen();

  static byte lastsec = 0;
  if (lastsec != CustomDate.sc)
  {
    ShowTimeBig(SCREENWID2 - 50, SCREENHEIGHT - 19);
    lastsec = CustomDate.sc;
  }
  
  switch (appScreenMode)
  {
    case SCREENMODE_CLOCK:
      showOrHideAlarmIcon();
      break;

    case SCREENMODE_RADIO:
      ShowFrequencyAndVolume();
      ShowMemoryStatusForCurrentFrequency();
      break;
  }

  // set up scrolling dots after drawing the screen
  if (refreshBG == 1)
    SetUpDots();
}

void setValuesForEdit()
{
  time_t timeT = RTC.get();

  // for editing
  timeEditHour   = hour(timeT);
  timeEditMinute = minute(timeT);
  timeEditSecond = second(timeT);
  dateEditDay    = day(timeT);
  dateEditMonth  = month(timeT);
  dateEditYear   = year(timeT);
}

void setAlarmValuesForEdit()
{
  // for alarm time editing
  if (alarmHour == 255)
  {
    // default 7am alarm
    alarmTimeEditHour   = 7;
    alarmTimeEditMinute = 0;
  } else
  {
    // load current alarm settings
    alarmTimeEditHour   = alarmHour;
    alarmTimeEditMinute = alarmMinute;
  }
}

// set alarm time
void ShowAlarmTimeForEditing()
{
  int a = SCREENWID2 - 33;
  int b = 43;
  uint8_t dontShow = 0;

  // hour
  dontShow = ((alarmTimeEditCursorPos == 1) && (Xor1 == 1)) ? 1 : 0;
  if (dontShow == 0) printBigNum(a, b, (int)alarmTimeEditHour / 10); else printBigNum(a, b, -1);
  a = a + 13;
  if (dontShow == 0) printBigNum(a, b, (int)alarmTimeEditHour % 10); else printBigNum(a, b, -1);
  a = a + 13;

  TV.bitmapfaster(a, b, bignums_COLON, 11, 15);
  a = a + 13;

  // minute
  dontShow = ((alarmTimeEditCursorPos == 2) && (Xor1 == 1)) ? 1 : 0;
  if (dontShow == 0) printBigNum(a, b, (int)alarmTimeEditMinute / 10); else printBigNum(a, b, -1);
  a = a + 13;
  if (dontShow == 0) printBigNum(a, b, (int)alarmTimeEditMinute % 10); else printBigNum(a, b, -1);
  a = a + 13;
}

void showOrHideAlarmIcon()
{
  if (alarmEnabled == 1)
  {
    // show the alarm icon
    TV.bitmapfaster(SCREENWIDTH - 19, 1, alarmIcon, 16, 16);

    // show the alarm time under it
    char buf1[7];
    itoa(alarmHour, buf1, 10);
    strcat_P(buf1, CONST_COLON);

    if (alarmMinute < 10) strcat_P(buf1, CONST_ZERO);
    char buf2[3];
    itoa(alarmMinute, buf2, 10);
    strcat(buf1, buf2);

    // calculate center
    uint8_t charpos = 4; // 4 digits by default 0:00
    if (alarmHour > 9) charpos++; // add extra position if tens digit occupied
    charpos = charpos * 4; // pixels per char
    charpos = charpos / 2; // middle
    TV.print((SCREENWIDTH - 11) - charpos, 19, buf1);
  }
  else
  {
    TV.draw_rectFastFill(SCREENWIDTH - 21, 1, 21, 16 + 10, BLACK);
  }
}

// alarm triggering turns the radio on, restores the video if it was timed out,
// and enables the piezo beeping in the main loop
void alarm_setTriggered()
{
  // set flag if not already
  alarmTriggered = 1;

  // set radio mode and turn it on
  if (radioONflag == 0)
    radio_init();

  if (videoIStimedout == 1)
  {
    restoreTimedOutVideo();
    TVOpenWindow(1);
  }

  appScreenMode = SCREENMODE_RADIO;
}

// dots /////////////////////////////////////////////////////////////////////////
void resetDots()
{
  for (uint8_t i = 0; i < CONST_NUMSTARS; i++)
  {
    StarField_star[i].X = 0;
    StarField_star[i].Y = 0;
    StarField_star[i].Z = 0;
  }
}

void SetUpDots()
{
  uint8_t i;

  // create a sane pixel map to start
  for (i = 0; i < CONST_NUMSTARS; i++)
  {
    while (1)
    {
      StarField_star[i].X = random(1, SCREENWIDTH);
      StarField_star[i].Y = random(1, SCREENHEIGHT);

      // we have a valid pixel (no matches to other pixels)
      if (DotAtThisLocation(i) == 0) break;
    }

    // get current pixel value at new location
    StarField_star[i].Z = TV.get_pixel(StarField_star[i].X, StarField_star[i].Y);
  }
}

void SetNewDotPos(int i)
{
  // set new position & search for duplicate
  StarField_star[i].X = SCREENWIDTH - 1;

  while (1)
  {
    // random y
    StarField_star[i].Y = random(1, SCREENHEIGHT - 1);
    if (DotAtThisLocation(i) == 0) break;
  }
}

uint8_t DotAtThisLocation(uint8_t i)
{
  uint8_t z;

  // search for dupe
  for (z = 0; z < CONST_NUMSTARS; z++)
  {
    if (z != i)
    {
      if (StarField_star[z].X == StarField_star[i].X)
      {
        if (StarField_star[z].Y == StarField_star[i].Y)
        {
          // dupe
          return 1;
        }
      }
    }
  }
  return 0;
}

void MoveDotsLeft()
{
  uint8_t i;

  // only process if the array was initialized (0 is not allowed)
  if (StarField_star[0].X == 0)
    return;

  for (i = 0; i < CONST_NUMSTARS; i++)
  {
    // restore old pixel
    TV.set_pixel(StarField_star[i].X, StarField_star[i].Y, StarField_star[i].Z);

    if (StarField_star[i].X <= 2)
      SetNewDotPos(i);
    else
    {
      // now scroll this dot at two velocities
      if ((StarField_star[i].Y & 1) == 0) // odd by 1 pixel
        StarField_star[i].X--;
      else
        StarField_star[i].X -= 2; // even by 2 pixels
    }

    // get pixel value (it will be the 'old' value when scrolling)
    StarField_star[i].Z = TV.get_pixel(StarField_star[i].X, StarField_star[i].Y);

    // draw the dot
    TV.set_pixel(StarField_star[i].X, StarField_star[i].Y, WHITE);
  }
}
// end of dots code ///////////////////////////////

void AppTimer_Timer()
{
  if (videoONflag == 1)
  {
    // flip through different modes as a timed event
    if (rotateScreens == 1)
    {
      TimeCounter++;
      if (TimeCounter == rotateScreenSeconds)
      {
        TimeCounter = 0;

        // switch screen mode
        advanceScreenMode();
        ScreenModeTimeShowing = 0;
      }
    } // rotatescreens
    else
    {
      // new
      TimeCounter++;
      if (TimeCounter == 30)
      {
        TimeCounter = 0;
      }
    }
  } // videoONflag
}

void openRadioMode()
{
  TVScrollDn();
  if (radioONflag == 0) radio_init();
}

void closeRadioMode()
{
  TVScrollUp();
  if (radioONflag == 1) radio_shutdown();
}

void advanceScreenMode()
{
  if (appScreenMode == SCREENMODE_RADIO)
  {
    closeRadioMode();
    appScreenMode = SCREENMODE_CLOCK;
  } else
  {
    openRadioMode();
    appScreenMode = SCREENMODE_RADIO;
  }
  Update(1);
}

// watchdog new
// 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms, 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii)
{
  byte bb;
  if (ii > 9 ) ii = 9;
  bb = ii & 7;
  if (ii > 7) bb |= (1 << 5);
  bb |= (1 << WDCE);
  
  MCUSR &= ~(1 << WDRF);
  // start timed sequence
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect)
{

}
// end of watchdog new

void restoreTimedOutVideo()
{
  // any button press restores the video
  tv_init();
  
  // fix the volume if the radio is on
  if (radioONflag == 1)
    radio_setVolumePWM();
  
  InitMode();
}

// check performance
void doBenchmarkPanel()
{
  int cursor_x, cursor_y, a, linefontsize;
  float d, dbltmp, highresTimer;
  char fq[8];
  long runcount = 0;

  // show menu
  TV.clear_screen();

  // draw menu header & line
  TV.printPGM(6, 6, CONST_BENCHMARKS);
  TV.draw_line(0, 14, SCREENWIDTH - 1, 14, WHITE);

  linefontsize = 9;
  cursor_x = 0;
  cursor_y = 14;
  
  // screen printing
  cursor_y += linefontsize;  
  
  runcount = 1000;
  highresTimer = radio1_Millis();
  for (dbltmp = 0; dbltmp < runcount; dbltmp++)
  {
    // make a random string from displayable characters
    for(int i=0; i<10; i++) 
      buf[i]=random(33,128);
    buf[9]=0;
    // draw it
    TV.print(cursor_x, cursor_y, buf);
  }
  highresTimer = radio1_Millis() - highresTimer;  
  highresTimer = highresTimer / (float)runcount;
  dtostrf(highresTimer, 6, 1, fq);
  strcpy_P(buf, CONST_BENCHPRINT);
  strcat(buf, fq);
  // show improvement  
  strcat_P(buf, CONST_1SPACE);
  highresTimer = (float)((long)(highresTimer*10.0f)) / 10.f; // strip trailing decimal positions 2+
  strcat_P(buf, CONST_OPENBRACKET);
  highresTimer = 7.7f / highresTimer;
  dtostrf(highresTimer, 2, 1, fq);
  strcat(buf, fq);
  strcat_P(buf, CONST_CLOSEBRACKET);
  TV.print(cursor_x, cursor_y, buf);

  radio1_Delay(1000);
  
  // break the search if a key is pressed
  if (radio1_analogReadFast(PIN_CHARLIEKBD) < 1000) return;  
  
  // graphics timings
  cursor_y += linefontsize;
  runcount = 700;
  highresTimer = radio1_Millis();
  for (d = 0; d < 700; d++)
    for (a = 0; a < 9; a++)
      TV.bitmapfaster(SCREENWIDTH-12, SCREENHEIGHT-16, BigNums[a], 11, 15);
  highresTimer = radio1_Millis() - highresTimer;
  highresTimer = highresTimer / (float)(runcount*9);
  dtostrf(highresTimer, 6, 1, fq);
  strcpy_P(buf, CONST_BENCHBITMAP);
  strcat(buf, fq);
  // show improvement  
  strcat_P(buf, CONST_1SPACE);
  highresTimer = (float)((long)(highresTimer*10.0f)) / 10.f; // strip trailing decimal positions 2+
  strcat_P(buf, CONST_OPENBRACKET);
  highresTimer = 1.2f / highresTimer;
  dtostrf(highresTimer, 2, 1, fq);
  strcat(buf, fq);
  strcat_P(buf, CONST_CLOSEBRACKET);
  TV.print(cursor_x, cursor_y, buf);
  
  radio1_Delay(3000);
}
// end of radio1.ino
