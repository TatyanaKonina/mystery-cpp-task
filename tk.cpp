#include "task.h"

int dt;
int iters;
int num_thread;
bool gui, finsish = false;
double Gmm;
Body *bodies, *new_bodies;
int num_body;
int queuing_jobs = 0, num_done = 0;
pthread_mutex_t queuing; // мьютекс - механизм синхронизации потоков
pthread_cond_t processing, iter_fin;
// nanoseconds total_time;;
double total_time;

// ALGO START
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
    new_a.vx = a.vx + f_sum_x * dt / a.m;
    new_a.vy = a.vy + f_sum_y * dt / a.m;
    new_a.x = a.x + new_a.vx * dt;

    new_a.y = a.y + new_a.vy * dt;
    new_a.m = a.m;
}

// у нас есть воркеры - это потоки
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

void input_bodies(std::string filename)
{
    std::ifstream input;
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

void init_env(std::string input_file)
{
    input_bodies(input_file);
}

void write_to_file(std::string filename, std::string output_text)
{
    std::ofstream output;
    output.open(filename);
    output << output_text;
    output.close();
}

// RENAME NAMES OF VARS!!!!
int main(int argc, char const **argv)
{
    dt = atoi(argv[1]);               // шаг по времени
    iters = atoi(argv[2]);            // число итераций
    num_thread = atoi(argv[3]);       // число потоков
    std::string input_file = argv[4]; // файл с переменными для расчетов
    std::string output_text;

    double start_t, end_t;

    init_env(input_file);

    pthread_t workers[num_thread]; // задаем кол-во потоков
    // просто все инитим
    // мьютекс чтоб закрывать критически важную секцию - очередь
    pthread_mutex_init(&queuing, NULL);
    // кондишен на последнее вычисление в итерации
    pthread_cond_init(&iter_fin, NULL);
    // кондишен на последнюю итерацию
    pthread_cond_init(&processing, NULL);

    // создаем столько тредов  сколько задали
    for (int i = 0; i < num_thread; ++i)
        pthread_create(&workers[i], NULL, worker, NULL);
    GET_TIME(start_t);
    for (int i = 0; i < iters; ++i)
    {

        output_text = output_text + std::to_string(i * dt);

        for (int i = 0; i < num_body; ++i)
        {
            Body &t = bodies[i];
            output_text = output_text + ';';
            output_text = output_text + std::to_string(t.x) + ';' + std::to_string(t.y);
        }
        output_text = output_text + "\n";

        // есть общий ресурс - очередь задач и мы ее постоянно лочим чтоб никто ее не портил
        pthread_mutex_lock(&queuing);
        // пушим задачи в очередь
        queuing_jobs = num_body, num_done = 0;
        // разбудить всех воркеров
        // чтобы они начали выполнять задачи
        pthread_cond_broadcast(&processing);

        // главный поток будет ждать пока все задачи не будут выполнены
        // позже он будет разбужен последним воркером в этой итерации
        // и таким образом пойдет на новый цикл итераций
        pthread_cond_wait(&iter_fin, &queuing);

        pthread_mutex_unlock(&queuing);
        Body *t = new_bodies;
        new_bodies = bodies;
        bodies = t;
    }
    GET_TIME(end_t);
    finsish = true;
    pthread_mutex_lock(&queuing);
    // посылаем сигнал что мы закончили последнюю итерацию
    pthread_cond_broadcast(&processing);
    pthread_mutex_unlock(&queuing);

    for (int j = 0; j < num_thread; ++j)
        pthread_join(workers[j], NULL); //  блокирует вызывающий поток, пока указанный поток не завершится.

    // дестроим что создали
    pthread_mutex_destroy(&queuing);
    pthread_cond_destroy(&iter_fin);
    pthread_cond_destroy(&processing);

    std::cout << "TIME: " << end_t - start_t;
    write_to_file("output.csv", output_text);
    return 0;
}

// всеми локами и анлоками мы просто закрываем критически важную секцию - очередь задач