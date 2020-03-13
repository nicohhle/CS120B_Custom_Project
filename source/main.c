/*	Author: nnguy099
 *  	Partner(s) Name:
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/eeprom.h>
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Nokia_5110.h"
#include "io.h"
#include "pwm.h"

unsigned char eepromScore EEMEM = 0;

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

///////////////////// START/END SCREENS /////////////////////

void StartScreen() {
  nokia_lcd_set_cursor(0, 0);
  nokia_lcd_write_char('D', 2);
  nokia_lcd_write_char('I', 1);
  nokia_lcd_write_char('N', 1);
  nokia_lcd_write_char('O', 1);

  nokia_lcd_set_cursor(10, 20);
  nokia_lcd_write_char('R', 2);
  nokia_lcd_write_char('U', 1);
  nokia_lcd_write_char('N', 1);
}

void EndScreen() {
  nokia_lcd_set_cursor(5, 5);
  nokia_lcd_write_char('G', 2);
  nokia_lcd_write_char('A', 1);
  nokia_lcd_write_char('M', 1);
  nokia_lcd_write_char('E', 1);

  nokia_lcd_set_cursor(15, 35);
  nokia_lcd_write_char('O', 2);
  nokia_lcd_write_char('V', 1);
  nokia_lcd_write_char('E', 1);
  nokia_lcd_write_char('R', 1);

  nokia_lcd_set_cursor(35, 0);
  nokia_lcd_write_string("Move up to start", 1);
}

///////////////////// TEST /////////////////////

enum TestState { Start,
                 Up,
                 Down,
                 Wait,
                 WaitForButtonRelease,
                 ResetTest };

int Test(int TestState) {
  unsigned char resetButton = (PINA & 0x10);

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
      else
        TestState = Wait;
      break;
    case WaitForButtonRelease:
      TestState = (resetButton) ? WaitForButtonRelease : ResetTest;
      break;
    case ResetTest:
      TestState = Start;
      break;
    default:
      TestState = Start;
      break;
  }

  switch (TestState) {  // actions
    case Start:
      // PORTB = 0x00;
      displayString("start");
      break;
    case Up:
      if (eeprom_read_byte(&eepromScore) < 9) {
        set_PWM(sound[0]);
        eeprom_update_byte(&eepromScore, eeprom_read_byte(&eepromScore) + 1);
      }
      // nokia_lcd_write_string("Pressed UP", 1);
      displayString("up");
      // LCD_DisplayString(17, "up");
      // PORTB = 0x01;
      break;
    case Down:
      if (eeprom_read_byte(&eepromScore) > 0) {
        set_PWM(sound[1]);
        eeprom_update_byte(&eepromScore, eeprom_read_byte(&eepromScore) - 1);
      }
      // nokia_lcd_write_string("Pressed DOWN", 1);
      displayString("down");
      // LCD_DisplayString(17, "down");
      // PORTB = 0x02;
      break;
    case Wait:
      set_PWM(0);
      // PORTB = 0x00;
      break;
    case WaitForButtonRelease:
      // nokia_lcd_write_string("Waiting for RESET", 1);
      displayString("wait");
      // LCD_ClearScreen();
      // PORTB = 0x00;
      break;
    case ResetTest:
      // nokia_lcd_write_string("RESET", 1);
      eeprom_update_byte(&eepromScore, 0);
      displayString("reset");
      // LCD_ClearScreen();
      // LCD_DisplayString(1, "RESET");
      // PORTB = 0x04;
      break;
    default:
      TestState = Start;
      break;
  }
  return TestState;
}
unsigned char temp = 0;
enum DisplayStates { display };
int Display(int state) {
  static unsigned char prev = 100;
  temp = eeprom_read_byte(&eepromScore);
  switch (state) {
    case display:
      if (temp != prev) {
        LCD_DisplayString(1, "Score : ");
        LCD_Cursor(9);
        LCD_WriteData(temp + '0');
        prev = temp;
      }
      break;
  }
  return state;
}

void displayString(char* string) {
  if (strcmp(string, "start") == 0) {
    nokia_lcd_set_cursor(0, 0);
    nokia_lcd_write_char('Y', 1);
  } else if (strcmp(string, "up") == 0) {
    nokia_lcd_set_cursor(0, 10);
    nokia_lcd_write_string("UP", 1);
  } else if (strcmp(string, "down") == 0) {
    nokia_lcd_set_cursor(0, 10);
    nokia_lcd_write_string("DOWN", 1);
  } else if (strcmp(string, "wait") == 0) {
    nokia_lcd_set_cursor(0, 10);
    nokia_lcd_write_string("WAIT", 1);
  } else if (strcmp(string, "reset") == 0) {
    nokia_lcd_set_cursor(0, 10);
    nokia_lcd_write_string("RESET", 1);
  } else {
    nokia_lcd_clear();
  }
}

///////////////////// MAIN /////////////////////

int main(void) {
  DDRA = 0x00;
  PORTA = 0x00;  // Joystick + Button (Input)

  DDRB = 0xFF;
  PORTB = 0x00;  // Nokia 5110 (Output)
  DDRC = 0xFF;
  PORTC = 0x00;
  DDRD = 0xFF;
  PORTD = 0x00;  // LCD (Output)

  const unsigned long timerPeriod = 50;
  // unsigned long elapsedTime = 0;

  static task task1, task2;
  task* tasks[] = {&task1, &task2};
  const unsigned short numTasks = sizeof(tasks) / sizeof(task*);

  task1.state = 0;
  task1.period = 200;
  task1.elapsedTime = task1.period;
  task1.TickFct = &Test;

  task2.state = 0;
  task2.period = 50;
  task2.elapsedTime = task2.period;
  task2.TickFct = &Display;

  TimerSet(timerPeriod);
  TimerOn();

  ADC_init();
  LCD_init();
  nokia_lcd_init();
  PWM_on();

  LCD_ClearScreen();
  // LCD_DisplayString(1, "test");
  // LCD_DisplayString(1, "Score : ");
  // Initialize(false);

  nokia_lcd_clear();
  StartScreen();
  nokia_lcd_set_cursor(55, 40);
  nokia_lcd_write_char('X', 1);
  nokia_lcd_set_cursor(75, 40);
  nokia_lcd_write_char('Y', 1);
  nokia_lcd_render();

  while (1) {
    // nokia_lcd_clear();
    unsigned char i;
    for (i = 0; i < numTasks; i++) {
      if (tasks[i]->elapsedTime >= tasks[i]->period) {
        tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
        tasks[i]->elapsedTime = 0;
      }
      tasks[i]->elapsedTime += timerPeriod;
    }

    // if (elapsedTime >= 200) {
    //   // nokia_lcd_clear();
    //   Test();
    //   elapsedTime = 0;
    // }

    while (!TimerFlag)
      ;
    TimerFlag = 0;

    // elapsedTime += timerPeriod;

    // nokia_lcd_render();
  }
  return 1;
}
