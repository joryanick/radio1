// radio1_toolbox.ino
//
// GoodPrototyping radio1 rev.C5
// (c) 2016-2018 Jory Anick, jory@goodprototyping.com

#include <Wire.h>
#include "radio1_toolbox.h"
#include "radio1_eeprom.h"
#include "radio1_clock.h"

int radio1_memoryFree()
{
extern unsigned long __bss_end;
extern void *__brkval;

  int freeValue;
  freeValue = 0;

  if ((unsigned long)__brkval == 0)
    freeValue = ((unsigned long)&freeValue) - ((unsigned long)&__bss_end);
  else
    freeValue = ((unsigned long)&freeValue) - ((unsigned long)__brkval);

  return freeValue;
} //end radio1_memoryFree()

// registry code /////////////////////////////////////////////////////////////  
void radio1_CheckEEPROMFormatted()
{
  // clear eeprom if format bit is not set
 if (EEPROM.read(510) != 127)
   radio1_ClearEEPROM();
}

void radio1_ClearEEPROM()
{
int i;
  
  // reset all 512 bytes
  for (i = 0; i < 512; i++)
    EEPROM.write(i, 255); // write all FF's to every memory block
  
  // set formatted
  EEPROM.write(510, 127);
}

void radio1_SaveSettingValue(int settingNum, uint8_t theValue)
{      
  EEPROM.write(settingNum,theValue);
}

// fetches an entry from the saved eeprom or returns the default if no entry (value was 0)
uint8_t radio1_GetSettingValue(int settingNum, uint8_t defaultRtn)
{
uint8_t a;
  
  a = EEPROM.read(settingNum);
  if (a == 255)
    return (defaultRtn);
  else
    return (a);
}
// end of registry code ///////////////////////////////////////////////////

uint8_t radio1_Weekday(uint8_t mo, uint8_t dy, int yr)
{
uint16_t mm;
uint16_t yy;
uint8_t a;

  // Calculate day of the week    
  mm = mo;
  yy = yr;
  if (mm < 3)
  {
    mm += 12;
    yy -= 1;
  }
  
  a = (dy + (2 * mm) + (6 * (mm + 1)/10) + yy + (yy/4) - (yy/100) + (yy/400) + 1) % 7;
  return(a);
}

// rtc functions
void inline radio1_SetRTCTime(uint8_t h, uint8_t m, uint8_t s)
{ 
  tmElements_t DStimestruct;
  
  DStimestruct.Year = CalendarYrToTm(CustomDate.yr);
  DStimestruct.Month = CustomDate.mo;
  DStimestruct.Day = CustomDate.dy;
  DStimestruct.Hour = h;
  DStimestruct.Minute = m;
  DStimestruct.Second = s;
  RTC.write(DStimestruct);
 
  CustomDate.hr = h;
  CustomDate.mn = m;
  CustomDate.sc = s;
}

void inline radio1_SetRTCDate(uint8_t mo, uint8_t d, int y)
{
  tmElements_t DStimestruct;
  
  DStimestruct.Year = CalendarYrToTm(y);
  DStimestruct.Month = mo;
  DStimestruct.Day = d;
  DStimestruct.Hour = CustomDate.hr;
  DStimestruct.Minute = CustomDate.mn;
  DStimestruct.Second = CustomDate.sc;
  RTC.write(DStimestruct);

  CustomDate.yr = y;
  CustomDate.mo = mo;
  CustomDate.dy = d;
}

void radio1_GetDateAndTimeFromRTC()
{ 
  time_t timeT = RTC.get();
  CustomDate.yr = year(timeT);
  CustomDate.mo = month(timeT);
  CustomDate.dy = day(timeT);
  CustomDate.hr = hour(timeT);
  CustomDate.mn = minute(timeT);
  CustomDate.sc = second(timeT);
}
// end rtc ///////////////////////////////////////////////////////////////////

// misc functions
void inline radio1_SoftwareReboot()
{
  // assumes pin is tied to RST
  analogWrite(PIN_RESET, LOW);
}

void inline radio1_GetAppVerString(char * s)
{
  // "APPNAME V1"
  // fetch from progmem
  strcpy_P(s, MENUCONST_APPNAME);
  strcat_P(s, CONST_VERSIONPREFIX);

  char vnum[5];
  itoa(VERSION_NUMBER, vnum, 10);
  strcat(s, vnum);
}

// end of misc functions ///////////////////////

void DebugPrint(char *msg)
{
  TV.end();
  Serial.begin(9600);
  Serial.println(msg);
  Serial.end();
  
  int i = TV.begin(NTSC, SCREENWIDTH, SCREENHEIGHT);
}

char *ftoa(char *a, float f, int precision)
{
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
  
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

void TVdebug(char *varname, long value)
{
char buf2[80];

  TV.draw_rectFastFill(1, 50, SCREENWIDTH, 10, BLACK); 
  snprintf(buf2,80, "DBG: %s = %ld", varname, value);
  TV.print(1,50,buf2);
  radio1_Delay(1000);
}

void DebugPrint(float val, byte precision)
{
  ftoa(buf, val, precision);
  DebugPrint(buf);
}

void DebugPrintFloatOld(float val, byte precision)
{
  // prints val with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimial places
  // example: printfloat( 3.1415, 2); // prints 3.14 (two decimal places)

  TV.end();
  Serial.begin(9600);
  
  // prints val with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimial places
  // example: lcdPrintfloat( 3.1415, 2); // prints 3.14 (two decimal places)
 
  if (val < 0.0)
  {
    Serial.print('-');
    val = -val;
  }

  Serial.print (int(val));  //prints the int part
  if (precision > 0)
  {
    Serial.print("."); // print the decimal point
    unsigned long frac;
    unsigned long mult = 1;
    byte padding = precision -1;
    while(precision--)
      mult *=10;
 
    if(val >= 0)
     frac = (val - int(val)) * mult;
    else
     frac = (int(val)- val ) * mult;
    unsigned long frac1 = frac;
    while( frac1 /= 10 )
     padding--;
    while(  padding--)
     Serial.print("0");
     
    Serial.println(frac,DEC) ;
  }
  int i = TV.begin(NTSC, SCREENWIDTH, SCREENHEIGHT);
}

void DebugPrint(int f)
{  
    sprintf(buf, "%d", f);
    DebugPrint((char *) buf);
}

void radio1_Delay(int duration)
{
  // if TV object is initialized use TV delay function otherwise use standard timer delay
  if (videoONflag == 1)
    TV.delay(duration);
  else
    delay(duration);
}

void radio1_Tone(int frequency, int duration)
{
  // if TV object is initialized use TV delay function otherwise use standard timer delay
  if (videoONflag == 1)
    TV.tone(frequency, duration);
  else
    tone(PIN_SPK, frequency, duration);
}

inline unsigned long radio1_Millis()
{
  if (videoONflag == 1)
    return TV.millis();
  else
    return millis();
}

// new from avdweb.nl
inline int radio1_analogReadFast(byte ADCpin) // inline functions must appear in header
{  
  // uses ADC set to 4 prescaler bits in radio_inithardware
  return analogRead(ADCpin);

// orig
/*
  byte prescalerBits = 4;
  byte ADCSRAOriginal = ADCSRA;
  ADCSRA = (ADCSRA & B11111000) | prescalerBits;
  int adc = analogRead(ADCpin);
  ADCSRA = ADCSRAOriginal;
  return adc;
*/
}

// this routine reads the ADC twice to get more reliable values
int radio1_analogReadTwice(uint8_t pin)
{
  radio1_analogReadFast(pin); // discard the first reading
  return radio1_analogReadFast(pin); // get the reading to keep
}

// end of radio1_toolbox.ino
