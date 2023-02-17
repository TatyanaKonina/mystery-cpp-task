#include <pthread.h>
#include <cmath>
// #include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <time.h> //....
#include "timer.h"

#define SQUARE(x) (x) * (x)
#define CUBE(x) (x) * (x) * (x)

const double G = 6.67384e-11;

struct Body
{
    double x, y, vx, vy, m;
};


inline void move_nth_body(int);
void *worker(void *);
void input_bodies(std::string);
void init_env(std::string);
void write_to_file(std::string, std::string);