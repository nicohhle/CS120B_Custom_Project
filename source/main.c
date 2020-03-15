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
#include "obstacles.h"
#include "pwm.h"

// unsigned char eepromCurrScore EEMEM = 0;
unsigned char score = 0;
unsigned char eepromHighScore EEMEM = 0;

typedef enum boolean { true,
                       false } boolean;

boolean characterJump;

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

task tasks[4];
const unsigned char numTasks = 4;

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
                 PassGame,
                 RestartWait,
                 Reset } GameState;

boolean gameIsOver;
boolean gameInProgress;
boolean gameDone;

unsigned char score;

int Game(int state) {
  unsigned char resetButton = (~PINC & 0x01) >> 4;

  switch (state) {  // transitions
    case Init:
      if (joyStickUp() == true) state = InitWait;
      break;
    case InitWait:
      if (joyStickUp() == true) state = StartGame;
      break;
    case StartGame:
      if (gameIsOver == true)
        state = EndGame;
      else if (gameDone == true)
        state = PassGame;
      break;
    case EndGame:
      if (joyStickUp() == true) state = RestartWait;
      break;
    case PassGame:
      if (joyStickUp() == true) state = RestartWait;
      break;
    case RestartWait:
      if (joyStickUp() == true)
        state = RestartWait;
      else
        state = Reset;
      break;
    case Reset:
      state = Init;
      break;
    default:
      break;
  }

  if (resetButton) state = Reset;

  switch (state) {  // actions
    case Init:
      StartScreen();
      // score = 0;
      // eeprom_update_byte(&eepromHighScore, 0);
      // gameInProgress = false;
      // gameIsOver = false;
      // characterJump = false;
      break;
    case InitWait:
      score = 1;
      break;
    case StartGame:
      score = 2;
      gameInProgress = true;
      break;
    case EndGame:
      // gameInProgress = false;
      gameIsOver = true;
      EndScreen();
      break;
    case PassGame:
      gameInProgress = false;
      PassScreen();
      break;
    case RestartWait:
      break;
    case Reset:
      score = 3;
      Initialize(false);
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
  nokia_lcd_set_cursor(7, 10);
  nokia_lcd_write_char('G', 2);
  nokia_lcd_write_char('A', 1);
  nokia_lcd_write_char('M', 1);
  nokia_lcd_write_char('E', 1);

  nokia_lcd_write_char(' ', 2);

  nokia_lcd_write_char('O', 2);
  nokia_lcd_write_char('V', 1);
  nokia_lcd_write_char('E', 1);
  nokia_lcd_write_char('R', 1);

  nokia_lcd_set_cursor(17, 29);
  nokia_lcd_write_string("RESTART? ", 1);

  nokia_lcd_set_cursor(30, 40);
  nokia_lcd_write_char('X', 1);
  nokia_lcd_write_char(' ', 1);
  nokia_lcd_write_char('Y', 1);
}

void PassScreen() {
  nokia_lcd_set_cursor(7, 10);
  nokia_lcd_write_char('G', 2);
  nokia_lcd_write_char('A', 1);
  nokia_lcd_write_char('M', 1);
  nokia_lcd_write_char('E', 1);

  nokia_lcd_write_char(' ', 2);

  nokia_lcd_write_char('P', 2);
  nokia_lcd_write_char('A', 1);
  nokia_lcd_write_char('S', 1);
  nokia_lcd_write_char('S', 1);

  nokia_lcd_set_cursor(17, 29);
  nokia_lcd_write_string("RESTART? ", 1);

  nokia_lcd_set_cursor(30, 40);
  nokia_lcd_write_char('X', 1);
  nokia_lcd_write_char(' ', 1);
  nokia_lcd_write_char('Y', 1);
}

int FailSound(int pos) {
  if (gameIsOver == true && pos < 4) {
    set_PWM(fail[pos]);
    return pos + 1;
  }
}

///////////////////// CHARACTER ///////////////////////

enum CharacterState { Ground,
                      Jump1,
                      Jump2,
                      Jump3,
                      Jump4,
                      Jump5 } CharacterState;

// boolean characterJump;
unsigned char jumpingHeights[] = {5, 10, 15, 10, 5};

int Character(int state) {
  if (gameInProgress == false) return Ground;

  unsigned char height;

  switch (state) {  // transitions
    case Ground:
      if (joyStickUp() == true) state = Jump1;
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
      characterJump = false;
      height = 0;
      break;
    default:
      characterJump = true;
      if (state == Jump1)
        set_PWM(sound[0]);
      else if (state == Jump5)
        set_PWM(sound[1]);

      height = jumpingHeights[state - 1];
      break;
  }

  nokia_lcd_set_cursor(10, 40 - height);
  nokia_lcd_write_char('X', 1);

  return state;
}

///////////////////// OBSTACLES /////////////////////
int PlayGame(int pos) {
  if (gameInProgress == false) {
    return -6;
  }

  if (pos >= 60) gameDone = true;

  MakeObstacle(pos);

  if (pos < 0)
    return pos + 1;
  else if (pos >= 60)
    return pos;

  return pos + 1;
}

signed char obstacles[] = {-1, -1, -1, -1, -1};

unsigned const char initialPos = 79;

void MakeObstacle(int pos) {
  if ((pos - 1) >= 0) {
    if (gameObstacles[pos - 1] == 1 && (pos - 1) < 60) {
      if (characterJump == true) {
        score = score + 1;
      } else {
        gameIsOver = true;
      }
    }
  }

  // 6 to wait
  if ((pos + 6) >= 0 && (pos + 6) < 60) {
    if (gameObstacles[pos + 6] == 1) {
      for (int i = 0; i < 60; ++i) {
        if (obstacles[i] < 0) {
          obstacles[i] = 79;
          break;
        }
      }
    }
  }
}

int MoveObstacle(int state) {
  if (gameInProgress == false) return 0;
  for (int i = 0; i < 60; ++i) {
    obstacles[i] = DrawObstacle(obstacles[i]);
  }

  return 0;
}

int DrawObstacle(int pos) {
  if (pos <= 0) return -1;
  nokia_lcd_set_cursor(pos, 40);
  nokia_lcd_write_char('Y', 1);

  // 5 is the speed for pixel
  return pos - 5;
}

///////////////////// DISPLAY SCORE /////////////////////

// unsigned char curr = 0;
unsigned char high = 0;
enum DisplayStates { display };

int Display(int state) {
  static unsigned char prev = 100;
  // curr = eeprom_read_byte(&eepromCurrScore);
  high = eeprom_read_byte(&eepromHighScore);
  switch (state) {
    case display:
      if (high != prev) {
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
        // LCD_WriteData(curr + '0');
        LCD_WriteData(score + '0');

        prev = high;
      }
      break;
  }
  return state;
}

///////////////////// INITIALIZE /////////////////////

void Initialize(boolean goingOn) {
  score = 0;
  eeprom_update_byte(&eepromHighScore, 0);
  gameInProgress = false;
  gameIsOver = false;
  gameDone = false;

  if (goingOn == false) {
    score = 0;
  }

  characterJump = false;

  set_PWM(0);

  for (unsigned char j = 0; j < 5; ++j) {
    obstacles[j] = -1;
  }

  unsigned char i = 0;

  // Controls game
  tasks[i].period = 1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].state = Init;
  tasks[i].TickFct = &Game;

  i++;

  // Controls character
  tasks[i].period = 1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].state = Ground;
  tasks[i].TickFct = &Character;

  i++;

  // Display score + high score
  tasks[i].period = 1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].state = display;
  tasks[i].TickFct = &Display;

  i++;

  // Sound for end of game
  tasks[i].period = 1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].state = 0;
  tasks[i].TickFct = &FailSound;

  i++;

  // Where produce obstacles happens
  tasks[i].period = 2;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].state = -6;
  tasks[i].TickFct = &MoveObstacle;

  i++;

  // Move obstacles along screen
  tasks[i].period = 1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].state = 0;
  tasks[i].TickFct = &MoveObstacle;
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

  ADC_init();
  LCD_init();
  nokia_lcd_init();
  PWM_on();

  const unsigned long timerPeriod = 50;
  // unsigned long elapsedTime = 0;

  // static task task1, task2, task3, task4;
  // task* tasks[] = {&task1, &task2, &task3, &task4};
  // const unsigned short numTasks = sizeof(tasks) / sizeof(task*);

  // task1.state = 0;
  // task1.period = 50;
  // task1.elapsedTime = task1.period;
  // task1.TickFct = &Game;

  // task2.state = 0;
  // task2.period = 50;
  // task2.elapsedTime = task2.period;
  // task2.TickFct = &Character;

  // task3.state = 0;
  // task3.period = 50;
  // task3.elapsedTime = task3.period;
  // task3.TickFct = &Display;

  // task4.state = 0;
  // task4.period = 50;
  // task4.elapsedTime = task4.period;
  // task4.TickFct = &FailSound;

  // ------------------------------------------

  // LCD_ClearScreen();
  // LCD_DisplayString(1, "test");
  // LCD_DisplayString(1, "Score : ");

  // nokia_lcd_clear();
  // StartScreen();
  // EndScreen();
  // nokia_lcd_set_cursor(55, 40);
  // nokia_lcd_write_char('X', 1);
  // nokia_lcd_set_cursor(75, 40);
  // nokia_lcd_write_char('Y', 1);

  // gameInProgress = false;
  // gameIsOver = true;
  // characterJump = false;

  Initialize(false);

  while (1) {
    nokia_lcd_clear();

    // unconditional border display
    int j;
    for (j = 0; j < 83; ++j) {
      nokia_lcd_set_pixel(j, 0, 1);
      nokia_lcd_set_pixel(j, 47, 1);
    }
    nokia_lcd_render();

    // unsigned char i;
    // for (i = 0; i < numTasks; i++) {
    //   if (tasks[i]->elapsedTime >= tasks[i]->period) {
    //     tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
    //     tasks[i]->elapsedTime = 0;
    //   }
    //   tasks[i]->elapsedTime += timerPeriod;
    // }

    unsigned char i;
    for (i = 0; i < numTasks; i++) {
      if (tasks[i].elapsedTime >= tasks[i].period) {
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
      }
      tasks[i].elapsedTime += 1;
    }

    nokia_lcd_render();

    while (!TimerFlag)
      ;
    TimerFlag = 0;

    // nokia_lcd_render();
  }
  return 1;
}
