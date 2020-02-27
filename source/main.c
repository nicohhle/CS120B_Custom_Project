/*	Author: nnguy099
 *  	Partner(s) Name:
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include "nokia5110.h"
#include "pwm.h"

typedef enum boolean { true,
                       false } boolean;

///////////////////// SOUNDS /////////////////////

double sound[2] = {261.63, 130.81};
double fail[4] = {392.0, 369.99, 349.23, 329.63};

///////////////////// TIMER /////////////////////

volatile unsigned char TimerFlag = 0;

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

void TimerOn() {
  TCCR1B = 0x0B;
  OCR1A = 125;
  TIMSK1 = 0x02;
  TCNT1 = 0;

  _avr_timer_cntcurr = _avr_timer_M;

  SREG |= 0x80;
}

void TimerOff() {
  TCCR1B = 0x00;
}

void TimerISR() {
  TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
  _avr_timer_cntcurr--;
  if (_avr_timer_cntcurr == 0) {
    TimerISR();
    _avr_timer_cntcurr = _avr_timer_M;
  }
}

void TimerSet(unsigned long M) {
  _avr_timer_M = M;
  _avr_timer_cntcurr = _avr_timer_M;
}

///////////////////// TASK STRUCT /////////////////////

typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
} task;

task tasks[1];

const unsigned char tasksNum = 1;

//////////////////// JOYSTICK ////////////////////

unsigned short ADCValue;
unsigned const short joyStickUpValue = 700;
unsigned const short joyStickDownValue = 300;

void ADC_init() {
  ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

boolean joyStickUp() {
  ADCValue = ADC;
  return (ADCValue > joyStickUpValue) ? true : false;
}

boolean joyStickDown() {
  ADCValue = ADC;
  return (ADCValue < joyStickDownValue) ? true : false;
}

///////////////////// LIGHTS /////////////////////

enum TestState { Start,
                 Up,
                 Down,
                 Wait,
                 WaitForButtonRelease,
                 Reset } TestState;

void Test() {
  unsigned char resetButton = (~PINC) & 0x0F;

  switch (TestState) {  // transitions
    case Start:
      TestState = Wait;
      break;
    case Up:
      TestState = Wait;
      break;
    case Down:
      TestState = Wait;
      break;
    case Wait:
      if (resetButton)
        TestState = WaitForButtonRelease;
      else if (joyStickUp() == true)
        TestState = Up;
      else if (joyStickDown() == true)
        TestState = Down;
      break;
    case WaitForButtonRelease:
      TestState = (resetButton) ? WaitForButtonRelease : Reset;
      break;
    case Reset:
      TestState = Start;
      break;
    default:
      TestState = Start;
      break;
  }

  switch (TestState) {  // actions
    case Start:
      PORTB = 0x00;
      break;
    case Up:
      set_PWM(sound[0]);
      PORTB = 0x01;
      break;
    case Down:
      set_PWM(sound[1]);
      PORTB = 0x02;
      break;
    case Wait:
      set_PWM(0);
      PORTB = 0x00;
      break;
    case WaitForButtonRelease:
      PORTB = 0x00;
      break;
    case Reset:
      PORTB = 0x04;
      break;
    default:
      TestState = Start;
      break;
  }
}

///////////////////// INITIALIZE /////////////////////

// void Initialize() {
//   unsigned char i = 0;

//   tasks[i].period = 1;
//   tasks[i].elapsedTime = tasks[i].period;
//   tasks[i].state = Start;
//   tasks[i].TickFct = &Test;
// }

///////////////////// MAIN /////////////////////

int main(void) {
  DDRA = 0x00;
  PORTA = 0xFF;  // Joystick (Input)
  DDRC = 0x00;
  PORTC = 0xFF;  // Button (Input)

  DDRB = 0xFF;
  PORTB = 0x00;  // Buttons --> Nokia 5110 (Output)
  DDRD = 0xFF;
  PORTD = 0x00;  // LCD (Output)

  const unsigned long timerPeriod = 1;
  unsigned long elapsedTime = 0;
  TestState = Start;

  TimerSet(timerPeriod);
  TimerOn();

  ADC_init();
  PWM_on();

  // Initialize();

  while (1) {
    // unsigned char i;
    // for (i = 0; i < tasksNum; i++) {
    //   if (tasks[i].elapsedTime >= tasks[i].period) {
    //     tasks[i].state = tasks[i].TickFct(tasks[i].state);
    //     tasks[i].elapsedTime = 0;
    //   }
    //   tasks[i].elapsedTime += 1;
    // }

    if (elapsedTime >= 100) {
      Test();
      elapsedTime = 0;
    }

    while (!TimerFlag)
      ;
    TimerFlag = 0;

    elapsedTime += timerPeriod;

    // PORTB = 0x01;
  }
  return 1;
}
