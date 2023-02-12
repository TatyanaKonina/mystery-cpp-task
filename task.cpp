#include <pthread.h>
#include <cmath>
#include <chrono>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace std;

const double G = 6.67384e-11;
#define SQUARE(x) (x) * (x)
#define CUBE(x) (x) * (x) * (x)
#ifndef INFO
#define INFO(str)                      \
    do                                 \
    {                                  \
        std::cout << str << std::endl; \
    } while (false)
#else
#endif
struct Body
{
    double x, y, vx, vy, m;
    friend std::ostream &operator<<(std::ostream &out, const Body &b)
    {
        out << "(" << b.x << ", " << b.y << ")"
            << "Vx= " << b.vx << ", Vy= " << b.vy
            << "M= " << b.m;
        return out;
    }
};
bool gui, finsish = false;
double t, Gmm, angle;
Body *bodies, *new_bodies;
int num_thread, iters, num_body;

int queuing_jobs = 0, num_done = 0;
pthread_mutex_t queuing;
pthread_cond_t processing, iter_fin;

inline void move_nth_body(int index)
{
    Body &a = bodies[index], &new_a = new_bodies[index];
    double f_sum_x = 0, f_sum_y = 0;
    for (int i = 0; i < num_body; ++i)
    {
        if (index == i)
            continue;
        Body &b = bodies[i];
        Gmm = G * b.m * a.m;
        double dx = b.x - a.x, dy = b.y - a.y,
               radius_cube_sqrt = CUBE(sqrt(SQUARE(dx) + SQUARE(dy))) + 10e-7;
        f_sum_x += Gmm * dx / radius_cube_sqrt;
        f_sum_y += Gmm * dy / radius_cube_sqrt;
    }
    new_a.vx = a.vx + f_sum_x * t / a.m;
    new_a.vy = a.vy + f_sum_y * t / a.m;
    new_a.x = a.x + new_a.vx * t;
    new_a.y = a.y + new_a.vy * t;
    new_a.m = a.m;
}
void *worker(void *param)
{
    while (true)
    {
        pthread_mutex_lock(&queuing);
        while (!finsish && queuing_jobs <= 0)
            pthread_cond_wait(&processing, &queuing);
        int i = --queuing_jobs;
        pthread_mutex_unlock(&queuing);
        if (finsish)
            break;
        move_nth_body(i);
        pthread_mutex_lock(&queuing);
        num_done++;
        if (num_done >= num_body)
            pthread_cond_signal(&iter_fin);
        pthread_mutex_unlock(&queuing);
    }
    return 0;
}
void input_bodies(string filename)
{
    ifstream input;
    input.open(filename);
    input >> num_body;
    bodies = new Body[num_body];
    new_bodies = new Body[num_body];

    for (int i = 0; i < num_body; ++i)
    {
        Body &t = bodies[i];
        input >> t.m >> t.x >> t.y >> t.vx >> t.vy;
    }
    input.close();
}
void init_env(int count, const char **argv)
{
    double len;
    num_thread = 2, iters = 10, t = 1, angle = 0;

    input_bodies("test1.txt");
}

int main(int argc, char const **argv)
{
    init_env(argc, argv);

    pthread_t workers[num_thread];
    pthread_mutex_init(&queuing, NULL);
    pthread_cond_init(&iter_fin, NULL);
    pthread_cond_init(&processing, NULL);

    for (int i = 0; i < num_thread; ++i)
        pthread_create(&workers[i], NULL, worker, NULL);

    for (int i = 0; i < iters; ++i)
    {
        for (int i = 0; i < num_body; ++i)
        {
            Body &t = bodies[i];
            INFO(i << " " << t.x << " " << t.y);
        }
        pthread_mutex_lock(&queuing);
        queuing_jobs = num_body, num_done = 0;
        pthread_cond_broadcast(&processing);
        pthread_cond_wait(&iter_fin, &queuing);
        pthread_mutex_unlock(&queuing);
        Body *t = new_bodies;
        new_bodies = bodies;
        bodies = t;
    }
    finsish = true;
    pthread_mutex_lock(&queuing);
    pthread_cond_broadcast(&processing);
    pthread_mutex_unlock(&queuing);
    for (int j = 0; j < num_thread; ++j)
        pthread_join(workers[j], NULL);

    return 0;
}