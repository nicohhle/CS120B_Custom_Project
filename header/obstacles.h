#include <avr/io.h>

unsigned const char numObstacles = 60;
unsigned const char gameObstacles[60] = {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
                                         1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};