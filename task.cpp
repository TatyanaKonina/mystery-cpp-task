#include <pthread.h>
#include <cmath>
#include <chrono>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace std;


#define INFO(str)                      \
    do                                 \
    {                                  \
        std::cout << str << std::endl; \
    } while (false)
#endif


const double G = 6.67384e-11;
#define SQUARE(x) (x) * (x)
#define CUBE(x) (x) * (x) * (x)
struct Body
{
    double x, y, vx, vy;
    friend std::ostream &operator<<(std::ostream &out, const Body &b)
    {
        out << "(" << b.x << ", " << b.y << ")"
            << "Vx= " << b.vx << ", Vy= " << b.vy;
        return out;
    }
};
bool gui, finsish = false;
double mass, t, Gmm, angle;
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
        double dx = b.x - a.x, dy = b.y - a.y,
               radius_cube_sqrt = CUBE(sqrt(SQUARE(dx) + SQUARE(dy))) + 10e-7;
        f_sum_x += Gmm * dx / radius_cube_sqrt;
        f_sum_y += Gmm * dy / radius_cube_sqrt;
    }
    new_a.vx = a.vx + f_sum_x * t / mass;
    new_a.vy = a.vy + f_sum_y * t / mass;
    new_a.x = a.x + new_a.vx * t;
    new_a.y = a.y + new_a.vy * t;
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
        input >> t.x >> t.y >> t.vx >> t.vy;
    }
    input.close();
}
void init_env(int count, const char **argv)
{
    double len;
    num_thread = 2, iters = 300;
    mass = 1, t = 1, angle = 0;
    // gui = true
    // if (gui) {
    //     xmin = -1, ymin = -1;
    //     len = 2.5, window_len = 500;
    //     mf = (double) window_len / len;
    //     init_window(window_len);
    // }
    input_bodies("test1.txt");
}
// void draw_points(int mode)
// {
//     if (mode == 0)
//         XClearWindow(display, window);
//     XSetForeground(display, gc, whitecolor);
//     for (int i = 0; i < num_body; ++i)
//     {
//         Body &t = bodies[i];
//         XDrawPoint(display, window, gc, (t.x - xmin) * mf, (t.y - ymin) * mf);
//     }
//     XFlush(display);
// }

int main(int argc, char const **argv)
{
    init_env(argc, argv);

    pthread_t workers[num_thread];
    pthread_mutex_init(&queuing, NULL);
    pthread_cond_init(&iter_fin, NULL);
    pthread_cond_init(&processing, NULL);

    for (int i = 0; i < num_thread; ++i)
        pthread_create(&workers[i], NULL, worker, NULL);

    Gmm = G * mass * mass;
    for (int i = 0; i < iters; ++i)
    {
        // if (gui)
        //     draw_points(0);

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