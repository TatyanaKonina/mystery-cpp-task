#include "task.h"

int dt;
int iters;
int num_thread;
bool gui, finsish = false;
double Gmm;
Body *bodies, *new_bodies;
int num_body;
int queuing_jobs = 0, num_done = 0;
pthread_mutex_t queuing;
pthread_cond_t processing, iter_fin;
double total_time;


inline void move_nth_body(int index) {
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
    new_a.vx = a.vx + f_sum_x * dt / a.m;
    new_a.vy = a.vy + f_sum_y * dt / a.m;
    new_a.x = a.x + new_a.vx * dt;

    new_a.y = a.y + new_a.vy * dt;
    new_a.m = a.m;
}

void *worker(void *param)
{
    while (true)
    {
        pthread_mutex_lock(&queuing);
        // проверка: следует ли продолжать ожидание
        while (!finsish && queuing_jobs <= 0)
            // ожидаем конца
            pthread_cond_wait(&processing, &queuing);

        int i = --queuing_jobs;

        // блочим критически важную секцию а именно изменение координат точек
        pthread_mutex_unlock(&queuing);
        if (finsish)
            break;
        move_nth_body(i);
        pthread_mutex_lock(&queuing);
        // во время вычисления мы обновляем кол-во задач которые мы выполнили
        num_done++;
        if (num_done >= num_body)
            // сигналим что мы все посчитали на этой итерации
            pthread_cond_signal(&iter_fin);
        pthread_mutex_unlock(&queuing);
    }
    return 0;
}

void input_bodies(std::string filename) {
    std::ifstream input;
    input.open(filename);
    input >> num_body;
    bodies = new Body[num_body];
    new_bodies = new Body[num_body];

    for (int i = 0; i < num_body; ++i) {
        Body &t = bodies[i];
        input >> t.m >> t.x >> t.y >> t.vx >> t.vy;
    }
    input.close();
}

void init_env(std::string input_file) {
    input_bodies(input_file);
}

void write_to_file(std::string filename, std::string output_text) {
    std::ofstream output;
    output.open(filename);
    output << output_text;
    output.close();
}


int main(int argc, char const **argv) {
    dt = atoi(argv[1]);
    iters = atoi(argv[2]);
    num_thread = atoi(argv[3]);
    std::string input_file = argv[4];
    std::string output_file = argv[5];
    std::string output_text;

    double start_t, end_t;

    init_env(input_file);

    pthread_t workers[num_thread];

    pthread_mutex_init(&queuing, NULL);
    pthread_cond_init(&iter_fin, NULL);
    pthread_cond_init(&processing, NULL);

    for (int i = 0; i < num_thread; ++i)
        pthread_create(&workers[i], NULL, worker, NULL);

    GET_TIME(start_t);

    for (int i = 0; i < iters; ++i) {
        output_text = output_text + std::to_string(i * dt);

        for (int i = 0; i < num_body; ++i){
            Body &t = bodies[i];
            output_text = output_text + ';';
            output_text = output_text + std::to_string(t.x) + ';' + std::to_string(t.y);
        }
        output_text = output_text + "\n";

        pthread_mutex_lock(&queuing);
        queuing_jobs = num_body, num_done = 0;
        pthread_cond_broadcast(&processing);
        pthread_cond_wait(&iter_fin, &queuing);
        pthread_mutex_unlock(&queuing);

        Body *t = new_bodies;
        new_bodies = bodies;
        bodies = t;
    }

    GET_TIME(end_t);

    finsish = true;
    pthread_mutex_lock(&queuing);
    pthread_cond_broadcast(&processing);
    pthread_mutex_unlock(&queuing);

    for (int j = 0; j < num_thread; ++j)
        pthread_join(workers[j], NULL);

    pthread_mutex_destroy(&queuing);
    pthread_cond_destroy(&iter_fin);
    pthread_cond_destroy(&processing);

    std::cout << "TIME: " << end_t - start_t;
    write_to_file(output_file, output_text);
    return 0;
}
