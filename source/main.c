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

unsigned char eepromCurrScore EEMEM = 0;
unsigned char eepromHighScore EEMEM = 0;

typedef enum boolean { true,
                       false } boolean;

boolean CharacterJump;

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

///////////////////// GAME /////////////////////

enum GameState { Init,
                 InitWait,
                 StartGame,
                 EndGame,
                 RestartWait,
                 Reset } GameState;

boolean GameIsOver;
boolean GameInProgress;

unsigned char score;

int Game(int state) {
  unsigned char resetButton = (~PINC & 0x01) >> 4;

  switch (state) {
    case Init:
      state = (joyStickUp() == true) ? InitWait : Init;
      break;
    case InitWait:
      state = (joyStickUp() == true) ? InitWait : StartGame;
      break;
    case StartGame:
      state = (GameIsOver == true) ? EndGame : StartGame;
      break;
    case EndGame:
      state = (joyStickUp() == true) ? RestartWait : EndGame;
      break;
    case RestartWait:
      state = (joyStickUp() == true) ? RestartWait : Reset;
      break;
    case Reset:
      state = Init;
    default:
      break;
  }

  if (resetButton) state = Reset;

  switch (state) {
    case Init:
      StartScreen();
      eeprom_update_byte(&eepromCurrScore, 0);
      eeprom_update_byte(&eepromHighScore, 0);
      GameInProgress = false;
      GameIsOver = false;
      CharacterJump = false;
      break;
    case InitWait:
      break;
    case StartGame:
      GameInProgress = true;
      break;
    case EndGame:
      GameInProgress = false;
      EndScreen();
      break;
    case RestartWait:
      break;
    case Reset:
      StartScreen();

    default:
      break;
  }

  return state;
}

///////////////////// START/END SCREENS /////////////////////

void StartScreen() {
  nokia_lcd_set_cursor(13, 5);
  nokia_lcd_write_char('D', 2);
  nokia_lcd_write_char('I', 1);
  nokia_lcd_write_char('N', 1);
  nokia_lcd_write_char('O', 1);

  nokia_lcd_write_char(' ', 2);

  nokia_lcd_write_char('R', 2);
  nokia_lcd_write_char('U', 1);
  nokia_lcd_write_char('N', 1);

  nokia_lcd_set_cursor(12, 27);
  nokia_lcd_write_string("UP to start", 1);

  nokia_lcd_set_cursor(20, 40);
  nokia_lcd_write_char('Y', 1);
  nokia_lcd_write_char(' ', 1);
  nokia_lcd_write_char('Y', 1);
  nokia_lcd_write_char('Y', 1);
  nokia_lcd_write_char(' ', 1);
  nokia_lcd_write_char('X', 1);
  nokia_lcd_write_char(' ', 1);
  nokia_lcd_write_char('Y', 1);
}

void EndScreen() {
  nokia_lcd_set_cursor(10, 10);
  nokia_lcd_write_char('G', 2);
  nokia_lcd_write_char('A', 1);
  nokia_lcd_write_char('M', 1);
  nokia_lcd_write_char('E', 1);

  nokia_lcd_write_char(' ', 2);

  nokia_lcd_write_char('O', 2);
  nokia_lcd_write_char('V', 1);
  nokia_lcd_write_char('E', 1);
  nokia_lcd_write_char('R', 1);

  nokia_lcd_set_cursor(10, 35);
  nokia_lcd_write_string("RESTART? ", 1);
  nokia_lcd_write_char('X', 1);
  nokia_lcd_write_char(' ', 1);
  nokia_lcd_write_char('Y', 1);
}

int FailSound(int pos) {
  if (GameIsOver == true && pos < 4) {
    set_PWM(fail[pos]);
    return pos + 1;
  }

  return pos;
}

///////////////////// CHARACTER ///////////////////////

enum CharacterState { Ground,
                      Jump1,
                      Jump2,
                      Jump3,
                      Jump4,
                      Jump5 } CharacterState;

// boolean CharacterJump;
unsigned char jumpingHeights[] = {5, 10, 15, 10, 5};

int Character(int state) {
  if (GameInProgress == false) return Ground;

  unsigned char height;

  switch (state) {  // transitions
    case Ground:
      state = (joyStickUp() == true) ? Jump1 : Ground;
      break;
    case Jump5:
      state = Ground;
      break;
    default:
      state = state + 1;
      break;
  }

  switch (state) {  // actions
    case Ground:
      CharacterJump = false;
      height = 0;
      break;
    default:
      CharacterJump = true;
      if (state == Jump1)
        set_PWM(sound[0]);
      else if (state == Jump5)
        set_PWM(sound[1]);

      height = jumpingHeights[state - 1];
      break;
  }

  nokia_lcd_set_cursor(10, 39 - height);
  nokia_lcd_write_char('X', 1);

  return state;
}

///////////////////// DISPLAY SCORE /////////////////////

unsigned char curr = 0;
unsigned char high = 0;
enum DisplayStates { display };

int Display(int state) {
  static unsigned char prev = 100;
  curr = eeprom_read_byte(&eepromCurrScore);
  high = eeprom_read_byte(&eepromHighScore);
  switch (state) {
    case display:
      if (curr != prev) {
        LCD_Cursor(1);
        // LCD_DisplayString(1, "High Score : ");
        LCD_WriteData('H');
        LCD_Cursor(2);
        LCD_WriteData('i');
        LCD_Cursor(3);
        LCD_WriteData('g');
        LCD_Cursor(4);
        LCD_WriteData('h');
        LCD_Cursor(6);
        LCD_WriteData('S');
        LCD_Cursor(7);
        LCD_WriteData('c');
        LCD_Cursor(8);
        LCD_WriteData('o');
        LCD_Cursor(9);
        LCD_WriteData('r');
        LCD_Cursor(10);
        LCD_WriteData('e');
        LCD_Cursor(12);
        LCD_WriteData(':');
        LCD_Cursor(14);
        LCD_WriteData(high + '0');

        LCD_Cursor(17);
        // LCD_DisplayString(1, "Score : ");
        LCD_WriteData('S');
        LCD_Cursor(18);
        LCD_WriteData('c');
        LCD_Cursor(19);
        LCD_WriteData('o');
        LCD_Cursor(20);
        LCD_WriteData('r');
        LCD_Cursor(21);
        LCD_WriteData('e');
        LCD_Cursor(23);
        LCD_WriteData(':');
        LCD_Cursor(25);
        LCD_WriteData(curr + '0');

        prev = curr;
      }
      break;
  }
  return state;
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

  static task task1, task2, task3, task4;
  task* tasks[] = {&task1, &task2, &task3, &task4};
  const unsigned short numTasks = sizeof(tasks) / sizeof(task*);

  task1.state = Init;
  task1.period = 100;
  task1.elapsedTime = task1.period;
  task1.TickFct = &Game;

  task2.state = 0;
  task2.period = 50;
  task2.elapsedTime = task2.period;
  task2.TickFct = &Character;

  task3.state = 0;
  task3.period = 50;
  task3.elapsedTime = task3.period;
  task3.TickFct = &Display;

  task4.state = Ground;
  task4.period = 50;
  task4.elapsedTime = task4.period;
  task4.TickFct = &FailSound;

  ADC_init();
  LCD_init();
  nokia_lcd_init();
  PWM_on();

  // LCD_ClearScreen();
  // LCD_DisplayString(1, "test");
  // LCD_DisplayString(1, "Score : ");

  // nokia_lcd_clear();
  StartScreen();
  // EndScreen();
  // nokia_lcd_set_cursor(55, 40);
  // nokia_lcd_write_char('X', 1);
  // nokia_lcd_set_cursor(75, 40);
  // nokia_lcd_write_char('Y', 1);
  int i;
  for (i = 0; i < 80; ++i) {
    nokia_lcd_set_pixel(i, 47, 1);
  }
  nokia_lcd_render();

  GameInProgress = false;
  GameIsOver = false;
  CharacterJump = false;

  while (1) {
    nokia_lcd_clear();
    unsigned char i;
    for (i = 0; i < numTasks; i++) {
      if (tasks[i]->elapsedTime >= tasks[i]->period) {
        tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
        tasks[i]->elapsedTime = 0;
      }
      tasks[i]->elapsedTime += timerPeriod;
    }

    while (!TimerFlag)
      ;
    TimerFlag = 0;

    nokia_lcd_render();
  }
  return 1;
}
