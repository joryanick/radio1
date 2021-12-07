// radio1_digitalwritefast.h
//
// GoodPrototyping radio1 rev.C5

#include <arduino.h>

// speedups for ATMega 2560
#define BIT_READ(value, bit) (((value) >> (bit)) & 0x01)
#define BIT_SET(value, bit) ((value) |= (1UL << (bit)))
#define BIT_CLEAR(value, bit) ((value) &= ~(1UL << (bit)))
#define BIT_WRITE(value, bit, bitvalue) (bitvalue ? BIT_SET(value, bit) : BIT_CLEAR(value, bit))

#if !defined(digitalPinToPortReg)
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    // arduino mega pins
    #define digitalPinToPortReg(P) \
    (((P) >= 22 && (P) <= 29) ? &PORTA : \
    ((((P) >= 10 && (P) <= 13) || ((P) >= 50 && (P) <= 53)) ? &PORTB : \
    (((P) >= 30 && (P) <= 37) ? &PORTC : \
    ((((P) >= 18 && (P) <= 21) || (P) == 38) ? &PORTD : \
    ((((P) >= 0 && (P) <= 3) || (P) == 5) ? &PORTE : \
    (((P) >= 54 && (P) <= 61) ? &PORTF : \
    ((((P) >= 39 && (P) <= 41) || (P) == 4) ? &PORTG : \
    ((((P) >= 6 && (P) <= 9) || (P) == 16 || (P) == 17) ? &PORTH : \
    (((P) == 14 || (P) == 15) ? &PORTJ : \
    (((P) >= 62 && (P) <= 69) ? &PORTK : &PORTL))))))))))
    
    #define digitalPinToDDRReg(P) \
    (((P) >= 22 && (P) <= 29) ? &DDRA : \
    ((((P) >= 10 && (P) <= 13) || ((P) >= 50 && (P) <= 53)) ? &DDRB : \
    (((P) >= 30 && (P) <= 37) ? &DDRC : \
    ((((P) >= 18 && (P) <= 21) || (P) == 38) ? &DDRD : \
    ((((P) >= 0 && (P) <= 3) || (P) == 5) ? &DDRE : \
    (((P) >= 54 && (P) <= 61) ? &DDRF : \
    ((((P) >= 39 && (P) <= 41) || (P) == 4) ? &DDRG : \
    ((((P) >= 6 && (P) <= 9) || (P) == 16 || (P) == 17) ? &DDRH : \
    (((P) == 14 || (P) == 15) ? &DDRJ : \
    (((P) >= 62 && (P) <= 69) ? &DDRK : &DDRL))))))))))
    
    #define digitalPinToPINReg(P) \
    (((P) >= 22 && (P) <= 29) ? &PINA : \
    ((((P) >= 10 && (P) <= 13) || ((P) >= 50 && (P) <= 53)) ? &PINB : \
    (((P) >= 30 && (P) <= 37) ? &PINC : \
    ((((P) >= 18 && (P) <= 21) || (P) == 38) ? &PIND : \
    ((((P) >= 0 && (P) <= 3) || (P) == 5) ? &PINE : \
    (((P) >= 54 && (P) <= 61) ? &PINF : \
    ((((P) >= 39 && (P) <= 41) || (P) == 4) ? &PING : \
    ((((P) >= 6 && (P) <= 9) || (P) == 16 || (P) == 17) ? &PINH : \
    (((P) == 14 || (P) == 15) ? &PINJ : \
    (((P) >= 62 && (P) <= 69) ? &PINK : &PINL))))))))))
    
    #define __digitalPinToBit(P) \
    (((P) >=  7 && (P) <=  9) ? (P) - 3 : \
    (((P) >= 10 && (P) <= 13) ? (P) - 6 : \
    (((P) >= 22 && (P) <= 29) ? (P) - 22 : \
    (((P) >= 30 && (P) <= 37) ? 37 - (P) : \
    (((P) >= 39 && (P) <= 41) ? 41 - (P) : \
    (((P) >= 42 && (P) <= 49) ? 49 - (P) : \
    (((P) >= 50 && (P) <= 53) ? 53 - (P) : \
    (((P) >= 54 && (P) <= 61) ? (P) - 54 : \
    (((P) >= 62 && (P) <= 69) ? (P) - 62 : \
    (((P) == 0 || (P) == 15 || (P) == 17 || (P) == 21) ? 0 : \
    (((P) == 1 || (P) == 14 || (P) == 16 || (P) == 20) ? 1 : \
    (((P) == 19) ? 2 : \
    (((P) == 5 || (P) == 6 || (P) == 18) ? 3 : \
    (((P) == 2) ? 4 : \
    (((P) == 3 || (P) == 4) ? 5 : 7)))))))))))))))
    
    // 15 PWM
    #define __digitalPinToTimer(P) \
    (((P) == 13 || (P) ==  4) ? &TCCR0A : \
    (((P) == 11 || (P) == 12) ? &TCCR1A : \
    (((P) == 10 || (P) ==  9) ? &TCCR2A : \
    (((P) ==  5 || (P) ==  2 || (P) ==  3) ? &TCCR3A : \
    (((P) ==  6 || (P) ==  7 || (P) ==  8) ? &TCCR4A : \
    (((P) == 46 || (P) == 45 || (P) == 44) ? &TCCR5A : 0))))))
    #define __digitalPinToTimerBit(P) \
    (((P) == 13) ? COM0A1 : (((P) ==  4) ? COM0B1 : \
    (((P) == 11) ? COM1A1 : (((P) == 12) ? COM1B1 : \
    (((P) == 10) ? COM2A1 : (((P) ==  9) ? COM2B1 : \
    (((P) ==  5) ? COM3A1 : (((P) ==  2) ? COM3B1 : (((P) ==  3) ? COM3C1 : \
    (((P) ==  6) ? COM4A1 : (((P) ==  7) ? COM4B1 : (((P) ==  8) ? COM4C1 : \
    (((P) == 46) ? COM5A1 : (((P) == 45) ? COM5B1 : COM5C1))))))))))))))
  #else
    // Standard Arduino Pins
    #define digitalPinToPortReg(P) \
    (((P) >= 0 && (P) <= 7) ? &PORTD : (((P) >= 8 && (P) <= 13) ? &PORTB : &PORTC))
    #define digitalPinToDDRReg(P) \
    (((P) >= 0 && (P) <= 7) ? &DDRD : (((P) >= 8 && (P) <= 13) ? &DDRB : &DDRC))
    #define digitalPinToPINReg(P) \
    (((P) >= 0 && (P) <= 7) ? &PIND : (((P) >= 8 && (P) <= 13) ? &PINB : &PINC))
    #define __digitalPinToBit(P) \
    (((P) >= 0 && (P) <= 7) ? (P) : (((P) >= 8 && (P) <= 13) ? (P) - 8 : (P) - 14))
    
    #if defined(__AVR_ATmega8__)
      // 3 PWM
      #define __digitalPinToTimer(P) \
      (((P) ==  9 || (P) == 10) ? &TCCR1A : (((P) == 11) ? &TCCR2 : 0))
      #define __digitalPinToTimerBit(P) \
      (((P) ==  9) ? COM1A1 : (((P) == 10) ? COM1B1 : COM21))
    #else  //168,328
      // 6 PWM
      #define __digitalPinToTimer(P) \
      (((P) ==  6 || (P) ==  5) ? &TCCR0A : \
      (((P) ==  9 || (P) == 10) ? &TCCR1A : \
      (((P) == 11 || (P) ==  3) ? &TCCR2A : 0)))
      #define __digitalPinToTimerBit(P) \
      (((P) ==  6) ? COM0A1 : (((P) ==  5) ? COM0B1 : \
      (((P) ==  9) ? COM1A1 : (((P) == 10) ? COM1B1 : \
      (((P) == 11) ? COM2A1 : COM2B1)))))
    #endif  //defined(__AVR_ATmega8__)
  #endif  //mega
#endif  //#if !defined(digitalPinToPortReg)

#define __atomicWrite__(A,P,V) \
  if ( (int)(A) < 0x40) { bitWrite(*(A), __digitalPinToBit(P), (V) );}  \
else {                                                         \
  uint8_t register saveSreg = SREG;                          \
  cli();                                                     \
  bitWrite(*(A), __digitalPinToBit(P), (V) );                   \
  SREG=saveSreg;                                             \
} 

#ifndef digitalWriteFast
  #define digitalWriteFast(P, V) \
  do {                       \
  if (__builtin_constant_p(P) && __builtin_constant_p(V))   __atomicWrite__((uint8_t*) digitalPinToPortReg(P),P,V) \
  else  digitalWrite((P), (V));         \
  }while (0)
#endif  //#ifndef digitalWriteFast

#ifndef noAnalogWrite
  #define noAnalogWrite(P) \
  	do {if (__builtin_constant_p(P) )  __atomicWrite((uint8_t*) __digitalPinToTimer(P),P,0) \
  		else turnOffPWM((P));   \
  } while (0)
#endif

#ifndef digitalReadFast
	#define digitalReadFast(P) ( (int) _digitalReadFast_((P)) )
	#define _digitalReadFast_(P ) \
	(__builtin_constant_p(P) ) ? ( \
	( BIT_READ(*digitalPinToPINReg(P), __digitalPinToBit(P))) ) : \
	digitalRead((P))
#endif

// for ATMega2560
__attribute__((always_inline))
static inline void pinModeFast(uint8_t pin, uint8_t mode)
{
  if(pin == 0 && mode == INPUT) {
    DDRE &= ~(1 << (0));
    PORTE &= ~(1 << (0));
  } else if(pin == 0 && mode == INPUT_PULLUP) {
    DDRE &= ~(1 << (0));
    PORTE |= (1 << (0));
  } else if(pin == 0) DDRE |= (1 << (0));
  else if(pin == 1 && mode == INPUT) {
    DDRE &= ~(1 << (1));
    PORTE &= ~(1 << (1));
  } else if(pin == 1 && mode == INPUT_PULLUP) {
    DDRE &= ~(1 << (1));
    PORTE |= (1 << (1));
  } else if(pin == 1) DDRE |= (1 << (1));
  else if(pin == 2 && mode == INPUT) {
    DDRE &= ~(1 << (4));
    PORTE &= ~(1 << (4));
  } else if(pin == 2 && mode == INPUT_PULLUP) {
    DDRE &= ~(1 << (4));
    PORTE |= (1 << (4));
  } else if(pin == 2) DDRE |= (1 << (4));
  else if(pin == 3 && mode == INPUT) {
    DDRE &= ~(1 << (5));
    PORTE &= ~(1 << (5));
  } else if(pin == 3 && mode == INPUT_PULLUP) {
    DDRE &= ~(1 << (5));
    PORTE |= (1 << (5));
  } else if(pin == 3) DDRE |= (1 << (5));
  else if(pin == 4 && mode == INPUT) {
    DDRG &= ~(1 << (5));
    PORTG &= ~(1 << (5));
  } else if(pin == 4 && mode == INPUT_PULLUP) {
    DDRG &= ~(1 << (5));
    PORTG |= (1 << (5));
  } else if(pin == 4) DDRG |= (1 << (5));
  else if(pin == 5 && mode == INPUT) {
    DDRE &= ~(1 << (3));
    PORTE &= ~(1 << (3));
  } else if(pin == 5 && mode == INPUT_PULLUP) {
    DDRE &= ~(1 << (3));
    PORTE |= (1 << (3));
  } else if(pin == 5) DDRE |= (1 << (3));
  else if(pin == 6 && mode == INPUT) {
    DDRH &= ~(1 << (3));
    PORTH &= ~(1 << (3));
  } else if(pin == 6 && mode == INPUT_PULLUP) {
    DDRH &= ~(1 << (3));
    PORTH |= (1 << (3));
  } else if(pin == 6) DDRH |= (1 << (3));
  else if(pin == 7 && mode == INPUT) {
    DDRH &= ~(1 << (4));
    PORTH &= ~(1 << (4));
  } else if(pin == 7 && mode == INPUT_PULLUP) {
    DDRH &= ~(1 << (4));
    PORTH |= (1 << (4));
  } else if(pin == 7) DDRH |= (1 << (4));
  else if(pin == 8 && mode == INPUT) {
    DDRH &= ~(1 << (5));
    PORTH &= ~(1 << (5));
  } else if(pin == 8 && mode == INPUT_PULLUP) {
    DDRH &= ~(1 << (5));
    PORTH |= (1 << (5));
  } else if(pin == 8) DDRH |= (1 << (5));
  else if(pin == 9 && mode == INPUT) {
    DDRH &= ~(1 << (6));
    PORTH &= ~(1 << (6));
  } else if(pin == 9 && mode == INPUT_PULLUP) {
    DDRH &= ~(1 << (6));
    PORTH |= (1 << (6));
  } else if(pin == 9) DDRH |= (1 << (6));
  else if(pin == 10 && mode == INPUT) {
    DDRB &= ~(1 << (4));
    PORTB &= ~(1 << (4));
  } else if(pin == 10 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (4));
    PORTB |= (1 << (4));
  } else if(pin == 10) DDRB |= (1 << (4));
  else if(pin == 11 && mode == INPUT) {
    DDRB &= ~(1 << (5));
    PORTB &= ~(1 << (5));
  } else if(pin == 11 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (5));
    PORTB |= (1 << (5));
  } else if(pin == 11) DDRB |= (1 << (5));
  else if(pin == 12 && mode == INPUT) {
    DDRB &= ~(1 << (6));
    PORTB &= ~(1 << (6));
  } else if(pin == 12 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (6));
    PORTB |= (1 << (6));
  } else if(pin == 12) DDRB |= (1 << (6));
  else if(pin == 13 && mode == INPUT) {
    DDRB &= ~(1 << (7));
    PORTB &= ~(1 << (7));
  } else if(pin == 13 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (7));
    PORTB |= (1 << (7));
  } else if(pin == 13) DDRB |= (1 << (7));
  else if(pin == 14 && mode == INPUT) {
    DDRJ &= ~(1 << (1));
    PORTJ &= ~(1 << (1));
  } else if(pin == 14 && mode == INPUT_PULLUP) {
    DDRJ &= ~(1 << (1));
    PORTJ |= (1 << (1));
  } else if(pin == 14) DDRJ |= (1 << (1));
  else if(pin == 15 && mode == INPUT) {
    DDRJ &= ~(1 << (0));
    PORTJ &= ~(1 << (0));
  } else if(pin == 15 && mode == INPUT_PULLUP) {
    DDRJ &= ~(1 << (0));
    PORTJ |= (1 << (0));
  } else if(pin == 15) DDRJ |= (1 << (0));
  else if(pin == 16 && mode == INPUT) {
    DDRH &= ~(1 << (1));
    PORTH &= ~(1 << (1));
  } else if(pin == 16 && mode == INPUT_PULLUP) {
    DDRH &= ~(1 << (1));
    PORTH |= (1 << (1));
  } else if(pin == 16) DDRH |= (1 << (1));
  else if(pin == 17 && mode == INPUT) {
    DDRH &= ~(1 << (0));
    PORTH &= ~(1 << (0));
  } else if(pin == 17 && mode == INPUT_PULLUP) {
    DDRH &= ~(1 << (0));
    PORTH |= (1 << (0));
  } else if(pin == 17) DDRH |= (1 << (0));
  else if(pin == 18 && mode == INPUT) {
    DDRD &= ~(1 << (3));
    PORTD &= ~(1 << (3));
  } else if(pin == 18 && mode == INPUT_PULLUP) {
    DDRD &= ~(1 << (3));
    PORTD |= (1 << (3));
  } else if(pin == 18) DDRD |= (1 << (3));
  else if(pin == 19 && mode == INPUT) {
    DDRD &= ~(1 << (2));
    PORTD &= ~(1 << (2));
  } else if(pin == 19 && mode == INPUT_PULLUP) {
    DDRD &= ~(1 << (2));
    PORTD |= (1 << (2));
  } else if(pin == 19) DDRD |= (1 << (2));
  else if(pin == 20 && mode == INPUT) {
    DDRD &= ~(1 << (1));
    PORTD &= ~(1 << (1));
  } else if(pin == 20 && mode == INPUT_PULLUP) {
    DDRD &= ~(1 << (1));
    PORTD |= (1 << (1));
  } else if(pin == 20) DDRD |= (1 << (1));
  else if(pin == 21 && mode == INPUT) {
    DDRD &= ~(1 << (0));
    PORTD &= ~(1 << (0));
  } else if(pin == 21 && mode == INPUT_PULLUP) {
    DDRD &= ~(1 << (0));
    PORTD |= (1 << (0));
  } else if(pin == 21) DDRD |= (1 << (0));
  else if(pin == 22 && mode == INPUT) {
    DDRA &= ~(1 << (0));
    PORTA &= ~(1 << (0));
  } else if(pin == 22 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (0));
    PORTA |= (1 << (0));
  } else if(pin == 22) DDRA |= (1 << (0));
  else if(pin == 23 && mode == INPUT) {
    DDRA &= ~(1 << (1));
    PORTA &= ~(1 << (1));
  } else if(pin == 23 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (1));
    PORTA |= (1 << (1));
  } else if(pin == 23) DDRA |= (1 << (1));
  else if(pin == 24 && mode == INPUT) {
    DDRA &= ~(1 << (2));
    PORTA &= ~(1 << (2));
  } else if(pin == 24 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (2));
    PORTA |= (1 << (2));
  } else if(pin == 24) DDRA |= (1 << (2));
  else if(pin == 25 && mode == INPUT) {
    DDRA &= ~(1 << (3));
    PORTA &= ~(1 << (3));
  } else if(pin == 25 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (3));
    PORTA |= (1 << (3));
  } else if(pin == 25) DDRA |= (1 << (3));
  else if(pin == 26 && mode == INPUT) {
    DDRA &= ~(1 << (4));
    PORTA &= ~(1 << (4));
  } else if(pin == 26 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (4));
    PORTA |= (1 << (4));
  } else if(pin == 26) DDRA |= (1 << (4));
  else if(pin == 27 && mode == INPUT) {
    DDRA &= ~(1 << (5));
    PORTA &= ~(1 << (5));
  } else if(pin == 27 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (5));
    PORTA |= (1 << (5));
  } else if(pin == 27) DDRA |= (1 << (5));
  else if(pin == 28 && mode == INPUT) {
    DDRA &= ~(1 << (6));
    PORTA &= ~(1 << (6));
  } else if(pin == 28 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (6));
    PORTA |= (1 << (6));
  } else if(pin == 28) DDRA |= (1 << (6));
  else if(pin == 29 && mode == INPUT) {
    DDRA &= ~(1 << (7));
    PORTA &= ~(1 << (7));
  } else if(pin == 29 && mode == INPUT_PULLUP) {
    DDRA &= ~(1 << (7));
    PORTA |= (1 << (7));
  } else if(pin == 29) DDRA |= (1 << (7));
  else if(pin == 30 && mode == INPUT) {
    DDRC &= ~(1 << (7));
    PORTC &= ~(1 << (7));
  } else if(pin == 30 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (7));
    PORTC |= (1 << (7));
  } else if(pin == 30) DDRC |= (1 << (7));
  else if(pin == 31 && mode == INPUT) {
    DDRC &= ~(1 << (6));
    PORTC &= ~(1 << (6));
  } else if(pin == 31 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (6));
    PORTC |= (1 << (6));
  } else if(pin == 31) DDRC |= (1 << (6));
  else if(pin == 32 && mode == INPUT) {
    DDRC &= ~(1 << (5));
    PORTC &= ~(1 << (5));
  } else if(pin == 32 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (5));
    PORTC |= (1 << (5));
  } else if(pin == 32) DDRC |= (1 << (5));
  else if(pin == 33 && mode == INPUT) {
    DDRC &= ~(1 << (4));
    PORTC &= ~(1 << (4));
  } else if(pin == 33 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (4));
    PORTC |= (1 << (4));
  } else if(pin == 33) DDRC |= (1 << (4));
  else if(pin == 34 && mode == INPUT) {
    DDRC &= ~(1 << (3));
    PORTC &= ~(1 << (3));
  } else if(pin == 34 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (3));
    PORTC |= (1 << (3));
  } else if(pin == 34) DDRC |= (1 << (3));
  else if(pin == 35 && mode == INPUT) {
    DDRC &= ~(1 << (2));
    PORTC &= ~(1 << (2));
  } else if(pin == 35 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (2));
    PORTC |= (1 << (2));
  } else if(pin == 35) DDRC |= (1 << (2));
  else if(pin == 36 && mode == INPUT) {
    DDRC &= ~(1 << (1));
    PORTC &= ~(1 << (1));
  } else if(pin == 36 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (1));
    PORTC |= (1 << (1));
  } else if(pin == 36) DDRC |= (1 << (1));
  else if(pin == 37 && mode == INPUT) {
    DDRC &= ~(1 << (0));
    PORTC &= ~(1 << (0));
  } else if(pin == 37 && mode == INPUT_PULLUP) {
    DDRC &= ~(1 << (0));
    PORTC |= (1 << (0));
  } else if(pin == 37) DDRC |= (1 << (0));
  else if(pin == 38 && mode == INPUT) {
    DDRD &= ~(1 << (7));
    PORTD &= ~(1 << (7));
  } else if(pin == 38 && mode == INPUT_PULLUP) {
    DDRD &= ~(1 << (7));
    PORTD |= (1 << (7));
  } else if(pin == 38) DDRD |= (1 << (7));
  else if(pin == 39 && mode == INPUT) {
    DDRG &= ~(1 << (2));
    PORTG &= ~(1 << (2));
  } else if(pin == 39 && mode == INPUT_PULLUP) {
    DDRG &= ~(1 << (2));
    PORTG |= (1 << (2));
  } else if(pin == 39) DDRG |= (1 << (2));
  else if(pin == 40 && mode == INPUT) {
    DDRG &= ~(1 << (1));
    PORTG &= ~(1 << (1));
  } else if(pin == 40 && mode == INPUT_PULLUP) {
    DDRG &= ~(1 << (1));
    PORTG |= (1 << (1));
  } else if(pin == 40) DDRG |= (1 << (1));
  else if(pin == 41 && mode == INPUT) {
    DDRG &= ~(1 << (0));
    PORTG &= ~(1 << (0));
  } else if(pin == 41 && mode == INPUT_PULLUP) {
    DDRG &= ~(1 << (0));
    PORTG |= (1 << (0));
  } else if(pin == 41) DDRG |= (1 << (0));
  else if(pin == 42 && mode == INPUT) {
    DDRL &= ~(1 << (7));
    PORTL &= ~(1 << (7));
  } else if(pin == 42 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (7));
    PORTL |= (1 << (7));
  } else if(pin == 42) DDRL |= (1 << (7));
  else if(pin == 43 && mode == INPUT) {
    DDRL &= ~(1 << (6));
    PORTL &= ~(1 << (6));
  } else if(pin == 43 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (6));
    PORTL |= (1 << (6));
  } else if(pin == 43) DDRL |= (1 << (6));
  else if(pin == 44 && mode == INPUT) {
    DDRL &= ~(1 << (5));
    PORTL &= ~(1 << (5));
  } else if(pin == 44 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (5));
    PORTL |= (1 << (5));
  } else if(pin == 44) DDRL |= (1 << (5));
  else if(pin == 45 && mode == INPUT) {
    DDRL &= ~(1 << (4));
    PORTL &= ~(1 << (4));
  } else if(pin == 45 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (4));
    PORTL |= (1 << (4));
  } else if(pin == 45) DDRL |= (1 << (4));
  else if(pin == 46 && mode == INPUT) {
    DDRL &= ~(1 << (3));
    PORTL &= ~(1 << (3));
  } else if(pin == 46 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (3));
    PORTL |= (1 << (3));
  } else if(pin == 46) DDRL |= (1 << (3));
  else if(pin == 47 && mode == INPUT) {
    DDRL &= ~(1 << (2));
    PORTL &= ~(1 << (2));
  } else if(pin == 47 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (2));
    PORTL |= (1 << (2));
  } else if(pin == 47) DDRL |= (1 << (2));
  else if(pin == 48 && mode == INPUT) {
    DDRL &= ~(1 << (1));
    PORTL &= ~(1 << (1));
  } else if(pin == 48 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (1));
    PORTL |= (1 << (1));
  } else if(pin == 48) DDRL |= (1 << (1));
  else if(pin == 49 && mode == INPUT) {
    DDRL &= ~(1 << (0));
    PORTL &= ~(1 << (0));
  } else if(pin == 49 && mode == INPUT_PULLUP) {
    DDRL &= ~(1 << (0));
    PORTL |= (1 << (0));
  } else if(pin == 49) DDRL |= (1 << (0));
  else if(pin == 50 && mode == INPUT) {
    DDRB &= ~(1 << (3));
    PORTB &= ~(1 << (3));
  } else if(pin == 50 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (3));
    PORTB |= (1 << (3));
  } else if(pin == 50) DDRB |= (1 << (3));
  else if(pin == 51 && mode == INPUT) {
    DDRB &= ~(1 << (2));
    PORTB &= ~(1 << (2));
  } else if(pin == 51 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (2));
    PORTB |= (1 << (2));
  } else if(pin == 51) DDRB |= (1 << (2));
  else if(pin == 52 && mode == INPUT) {
    DDRB &= ~(1 << (1));
    PORTB &= ~(1 << (1));
  } else if(pin == 52 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (1));
    PORTB |= (1 << (1));
  } else if(pin == 52) DDRB |= (1 << (1));
  else if(pin == 53 && mode == INPUT) {
    DDRB &= ~(1 << (0));
    PORTB &= ~(1 << (0));
  } else if(pin == 53 && mode == INPUT_PULLUP) {
    DDRB &= ~(1 << (0));
    PORTB |= (1 << (0));
  } else if(pin == 53) DDRB |= (1 << (0));
  else if(pin == 54 && mode == INPUT) {
    DDRF &= ~(1 << (0));
    PORTF &= ~(1 << (0));
  } else if(pin == 54 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (0));
    PORTF |= (1 << (0));
  } else if(pin == 54) DDRF |= (1 << (0));
  else if(pin == 55 && mode == INPUT) {
    DDRF &= ~(1 << (1));
    PORTF &= ~(1 << (1));
  } else if(pin == 55 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (1));
    PORTF |= (1 << (1));
  } else if(pin == 55) DDRF |= (1 << (1));
  else if(pin == 56 && mode == INPUT) {
    DDRF &= ~(1 << (2));
    PORTF &= ~(1 << (2));
  } else if(pin == 56 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (2));
    PORTF |= (1 << (2));
  } else if(pin == 56) DDRF |= (1 << (2));
  else if(pin == 57 && mode == INPUT) {
    DDRF &= ~(1 << (3));
    PORTF &= ~(1 << (3));
  } else if(pin == 57 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (3));
    PORTF |= (1 << (3));
  } else if(pin == 57) DDRF |= (1 << (3));
  else if(pin == 58 && mode == INPUT) {
    DDRF &= ~(1 << (4));
    PORTF &= ~(1 << (4));
  } else if(pin == 58 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (4));
    PORTF |= (1 << (4));
  } else if(pin == 58) DDRF |= (1 << (4));
  else if(pin == 59 && mode == INPUT) {
    DDRF &= ~(1 << (5));
    PORTF &= ~(1 << (5));
  } else if(pin == 59 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (5));
    PORTF |= (1 << (5));
  } else if(pin == 59) DDRF |= (1 << (5));
  else if(pin == 60 && mode == INPUT) {
    DDRF &= ~(1 << (6));
    PORTF &= ~(1 << (6));
  } else if(pin == 60 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (6));
    PORTF |= (1 << (6));
  } else if(pin == 60) DDRF |= (1 << (6));
  else if(pin == 61 && mode == INPUT) {
    DDRF &= ~(1 << (7));
    PORTF &= ~(1 << (7));
  } else if(pin == 61 && mode == INPUT_PULLUP) {
    DDRF &= ~(1 << (7));
    PORTF |= (1 << (7));
  } else if(pin == 61) DDRF |= (1 << (7));
  else if(pin == 62 && mode == INPUT) {
    DDRK &= ~(1 << (0));
    PORTK &= ~(1 << (0));
  } else if(pin == 62 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (0));
    PORTK |= (1 << (0));
  } else if(pin == 62) DDRK |= (1 << (0));
  else if(pin == 63 && mode == INPUT) {
    DDRK &= ~(1 << (1));
    PORTK &= ~(1 << (1));
  } else if(pin == 63 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (1));
    PORTK |= (1 << (1));
  } else if(pin == 63) DDRK |= (1 << (1));
  else if(pin == 64 && mode == INPUT) {
    DDRK &= ~(1 << (2));
    PORTK &= ~(1 << (2));
  } else if(pin == 64 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (2));
    PORTK |= (1 << (2));
  } else if(pin == 64) DDRK |= (1 << (2));
  else if(pin == 65 && mode == INPUT) {
    DDRK &= ~(1 << (3));
    PORTK &= ~(1 << (3));
  } else if(pin == 65 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (3));
    PORTK |= (1 << (3));
  } else if(pin == 65) DDRK |= (1 << (3));
  else if(pin == 66 && mode == INPUT) {
    DDRK &= ~(1 << (4));
    PORTK &= ~(1 << (4));
  } else if(pin == 66 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (4));
    PORTK |= (1 << (4));
  } else if(pin == 66) DDRK |= (1 << (4));
  else if(pin == 67 && mode == INPUT) {
    DDRK &= ~(1 << (5));
    PORTK &= ~(1 << (5));
  } else if(pin == 67 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (5));
    PORTK |= (1 << (5));
  } else if(pin == 67) DDRK |= (1 << (5));
  else if(pin == 68 && mode == INPUT) {
    DDRK &= ~(1 << (6));
    PORTK &= ~(1 << (6));
  } else if(pin == 68 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (6));
    PORTK |= (1 << (6));
  } else if(pin == 68) DDRK |= (1 << (6));
  else if(pin == 69 && mode == INPUT) {
    DDRK &= ~(1 << (7));
    PORTK &= ~(1 << (7));
  } else if(pin == 69 && mode == INPUT_PULLUP) {
    DDRK &= ~(1 << (7));
    PORTK |= (1 << (7));
  } else if(pin == 69) DDRK |= (1 << (7));
}

// end of radio1_digitalwritefast.h
