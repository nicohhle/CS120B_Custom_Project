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
                 Reset } TestState;

int Test(int state) {
  switch (state) {  // transitions
    case Start:
      state = Wait;
      break;
    case Up:
      state = Wait;
      break;
    case Down:
      state = Wait;
    case Wait:
      if (joyStickUp() == true)
        state = Up;
      else if (joyStickDown() == true)
        state = Down;
    case Reset:
      state = Wait;
    default:
      break;
  }

  switch (state) {  // actions
    case Start:
      break;
    case Up:
      PORTB = 0x01;
      break;
    case Down:
      PORTB = 0x02;
      break;
    case Wait:
      if (joyStickUp() == true) {
        PORTB = 0x01;
      } else if (joyStickDown() == true) {
        PORTB = 0x02;
      }
      break;
    case Reset:
      PORTB = 0x00;
      break;
    default:
      break;
  }
}

///////////////////// INITIALIZE /////////////////////

void Initialize() {
  unsigned char i = 0;

  tasks[i].period = 1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].state = Start;
  tasks[i].TickFct = &Test;
}

///////////////////// MAIN /////////////////////

int main(void) {
  DDRA = 0x00;
  PORTA = 0xFF;  // Joystick (Input)
  DDRC = 0x00;
  PORTC = 0xFF;  // Button (Input)

  DDRB = 0xFF;
  PORTB = 0x00;  // Nokia 5110 (Output)
  // DDRD = 0xFF; PORTD = 0x00; // LCD (Output)

  ADC_init();
  PWM_on();

  PORTA = 0x01;

  // Initialize();

  while (1) {
    unsigned char i;
    for (i = 0; i < tasksNum; i++) {
      if (tasks[i].elapsedTime >= tasks[i].period) {
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
      }
      tasks[i].elapsedTime += 1;
    }
  }
  return 1;
}
