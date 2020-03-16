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
signed char obstacles[] = {-1, -1, -1, -1, -1};

typedef enum boolean { true,
                       false } boolean;

boolean characterJump;

///////////////////// SOUNDS /////////////////////

double sound[2] = {261.63, 130.81};
double fail[7] = {466.16, 440.0, 415.3, 440.0, 415.3, 369.99, 0};

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

enum GameState {
  Init,
  InitWait,
  StartGame,
  EndGame,
  PassGame,
  RestartWait,
  Reset
} GameState;

boolean gameIsOver;
boolean gameInProgress;
boolean gameDone;

unsigned char score;

int Game(int state) {
  unsigned char resetButton = PINA & 0x10;

  switch (state) {  // transitions
    case Init:
      score = 0;
      // eeprom_update_byte(&eepromHighScore, 0);
      gameInProgress = false;
      gameIsOver = false;
      gameDone = false;
      characterJump = false;

      set_PWM(0);

      unsigned char j = 0;
      for (j = 0; j < 5; ++j) {
        obstacles[j] = -1;
      }
      state = InitWait;
      break;
    case InitWait:
      StartScreen();
      if (joyStickUp() == true)
        state = StartGame;

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
      state = InitWait;
      break;
    default:
      state = Init;
      break;
  }

  if (resetButton) {
    eeprom_update_byte(&eepromHighScore, 0);
    state = Reset;
  }

  switch (state) {  // actions
    case Init:
      break;
    case InitWait:
      // score = 1;
      break;
    case StartGame:
      // score = 2;
      gameInProgress = true;
      break;
    case EndGame:
      gameInProgress = false;
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
      // StartScreen();
      score = 0;
      gameInProgress = false;
      gameIsOver = false;
      gameDone = false;

      characterJump = false;

      set_PWM(0);

      unsigned char j = 0;
      for (j = 0; j < 5; ++j) {
        obstacles[j] = -1;
      }

    default:
      break;
  }

  return state;
}

///////////////////// SCREENS /////////////////////

void StartScreen() {
  nokia_lcd_clear();

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
  nokia_lcd_clear();

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
  nokia_lcd_clear();

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
  if (gameIsOver == true && pos < 7) {
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
  nokia_lcd_clear();

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
      if (state == Jump1 || state == Jump2 || state == Jump3)
        set_PWM(sound[0]);
      else
        set_PWM(0);

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

  if (pos >= 50) gameDone = true;

  MakeObstacle(pos);

  if (pos < 0)
    return pos + 1;
  else if (pos >= 50)
    return pos;

  return pos + 1;
}

// signed char obstacles[] = {-1, -1, -1, -1, -1};

unsigned const char initialPos = 79;

void MakeObstacle(int pos) {
  if ((pos - 1) >= 0 && (pos - 1) < 50) {
    if (gameObstacles[pos - 1] == 1) {
      if (characterJump == true) {
        score = score + 1;
      } else {
        gameIsOver = true;
      }
    }
  }

  // 6 to wait
  if ((pos + 6) >= 0 && (pos + 6) < 50) {
    if (gameObstacles[pos + 6] == 1) {
      for (int i = 0; i < 5; ++i) {
        if (obstacles[i] < 0) {
          obstacles[i] = initialPos;
          break;
        }
      }
    }
  }
}

int MoveObstacle(int state) {
  if (gameInProgress == false) return 0;

  for (int i = 0; i < 5; ++i) {
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
      if (score != prev) {
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
        if (high < score) eeprom_update_byte(&eepromHighScore, score);
        high = eeprom_read_byte(&eepromHighScore);
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

        prev = score;
      }
      break;
  }
  return state;
}

///////////////////// MAIN /////////////////////

//GameState state = Start;

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

  const unsigned long timerPeriod = 10;

  TimerSet(timerPeriod);
  TimerOn();

  // unsigned long elapsedTime = 0;

  static task task1, task2, task3, task4, task5, task6;
  task* tasks[] = {&task1, &task2, &task3, &task4, &task5, &task6};
  const unsigned short numTasks = sizeof(tasks) / sizeof(task*);

  task1.state = 0;
  task1.period = 50;
  task1.elapsedTime = task1.period;
  task1.TickFct = &Character;

  task2.state = -6;
  task2.period = 100;
  task2.elapsedTime = task2.period;
  task2.TickFct = &PlayGame;

  task3.state = 0;
  task3.period = 50;
  task3.elapsedTime = task3.period;
  task3.TickFct = &MoveObstacle;

  task4.state = 0;
  task4.period = 50;
  task4.elapsedTime = task4.period;
  task4.TickFct = &Game;

  task5.state = 0;
  task5.period = 50;
  task5.elapsedTime = task5.period;
  task5.TickFct = &Display;

  task6.state = 0;
  task6.period = 50;
  task6.elapsedTime = task6.period;
  task6.TickFct = &FailSound;

  //-------------------------------------------------
  // task2.state = 0;
  // task2.period = 1;
  // task2.elapsedTime = task2.period;
  // task2.TickFct = &Game;

  // task3.state = 0;
  // task3.period = 1;
  // task3.elapsedTime = task3.period;
  // task3.TickFct = &Display;

  // task4.state = 0;
  // task4.period = 1;
  // task4.elapsedTime = task4.period;
  // task4.TickFct = &FailSound;

  // task5.state = -6;
  // task5.period = 2;
  // task5.elapsedTime = task5.period;
  // task5.TickFct = &PlayGame;

  // task6.state = 0;
  // task6.period = 1;
  // task6.elapsedTime = task6.period;
  // task6.TickFct = &MoveObstacle;

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
  // nokia_lcd_render();

  // score = 0;
  // eeprom_update_byte(&eepromHighScore, 0);
  // gameInProgress = false;
  // gameIsOver = false;
  // gameDone = false;

  // characterJump = false;

  // TESTING
  // long gameElapsedTime = 0;
  // long gamePeriod = 1000;

  while (1) {
    // nokia_lcd_clear();

    // unconditional border display
    int j;
    for (j = 0; j < 83; ++j) {
      nokia_lcd_set_pixel(j, 0, 1);
      nokia_lcd_set_pixel(j, 47, 1);
    }
    nokia_lcd_render();

    int i;
    for (i = 0; i < 6; i++) {
      if (tasks[i]->elapsedTime >= tasks[i]->period) {
        tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
        tasks[i]->elapsedTime = 0;
      }
      tasks[i]->elapsedTime += timerPeriod;
    }

    nokia_lcd_render();

    while (!TimerFlag)
      ;
    TimerFlag = 0;
  }
  return 1;
}
