#include <pthread.h>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string>

#define SQUARE(x) (x) * (x)
#define CUBE(x) (x) * (x) * (x)

const double G = 6.67384e-11;

struct Body
{
    double x, y, vx, vy, m;
};


inline void move_nth_body(int index);
void *worker(void *param);
void input_bodies(std::string filename);
void init_env(std::string input_file);
void write_to_file(std::string filename, std::string output_text);