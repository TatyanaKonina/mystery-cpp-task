#include "task.h"


int dt;
int iters;
int num_thread;
bool gui, finsish = false;
double Gmm;
Body *bodies, *new_bodies;
int num_body;
int queuing_jobs = 0, num_done = 0;
pthread_mutex_t queuing;  // мьютекс - механизм синхронизации потоков
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


void *worker(void *param)
{
    while (true)
    {
        // блокировка доступа к очереди (когда модифицируем переменную -- или ++)
        pthread_mutex_lock(&queuing);
        // проверка: следует ли продолжать ожидание
        while (!finsish && queuing_jobs <= 0)
            // Функция pthread_cond_wait блокирует thread на неопределенное время. Если условие так никогда и не будет выполнено, то нить никогда не возобновится.
            // Поскольку в функции pthread_cond_wait предусмотрена точка отмены thread, единственный способ выхода из этой
            // тупиковой ситуации - отменить блокированную нить, если такая отмена разрешена.

            // ожидать остальных (блокировать поток, пока не появится processing)
            // ждем сигнал от другого треда который может пеменять или нет условие
            pthread_cond_wait(&processing, &queuing);
             /* Equivalent to:
            pthread_mutex_unlock(&queuing);
            wait for signal processing
            pthread_mutex_lock(&queuing);
             */
        int i = --queuing_jobs;
        // активизация взаимной блокировки - в противном случае
        // из процедуры сможет выйти только один thread
        pthread_mutex_unlock(&queuing);
        if (finsish)
            break;
        move_nth_body(i);
        pthread_mutex_lock(&queuing);
        num_done++;
        if (num_done >= num_body)
            // Функция pthread_cond_signal активизирует по крайней мере однин thread, который в данное время ожидает выполнения определенного условия.
            // Thread для активизации выбирается в соответствии с наивысшим приоритетом планирования;
            // wait очнется и провкерит условие
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
    dt = atoi(argv[1]);  // шаг по времени
    iters = atoi(argv[2]);  // число итераций
    num_thread = atoi(argv[3]);  // число потоков
    std::string input_file = argv[4]; // файл с переменными для рассчетов
    std::string output_text;
    // clock_t begin;
    double start_t, end_t;

    init_env(input_file);

    GET_TIME(start_t);

    pthread_t workers[num_thread];  // задаем кол-во потоков
    pthread_mutex_init(&queuing, NULL);  // Перед использованием инициализируем мьютекс (это механизм изоляции, используемый сервером баз данных для синхронизации доступа нескольких потоков к совместно используемым ресурсам)
    pthread_cond_init(&iter_fin, NULL); // Переменные условия позволяют приостановить выполнение треда, пока не наступит заданное событие или не будет соблюдено некоторое условие.
    pthread_cond_init(&processing, NULL);

    for (int i = 0; i < num_thread; ++i)
        pthread_create(&workers[i], NULL, worker, NULL);  // starts a new thread in the calling process

    for (int i = 0; i < iters; ++i)
    {

        // begin = clock();

        output_text = output_text + std::to_string(i * dt);

        for (int i = 0; i < num_body; ++i)
        {
            Body &t = bodies[i];
            output_text = output_text + ';';
            output_text = output_text + std::to_string(t.x) + ';' + std::to_string(t.y);
        }
        output_text = output_text + "\n";

        // блокировка доступа к очереди
        pthread_mutex_lock(&queuing);
        queuing_jobs = num_body, num_done = 0;
        // Функция pthread_cond_broadcast активизирует все threads, которые ожидают выполнения заданного условия.
        // Однако после завершения функции thread можно вновь заблокировать до тех пор, пока не будет выполнено то же самое условие.
        // транслирует сигнал всем ждушим тредам
        pthread_cond_broadcast(&processing);

        pthread_cond_wait(&iter_fin, &queuing);  // Эти функции атомарно освобождают мьютекс и заставляют вызывающий поток блокироваться по условной переменной cond ; атомарно здесь означает «атомарно в отношении доступа другого потока к мьютексу, а затем к условной переменной».
        /* Equivalent to:
            pthread_mutex_unlock(&queuing);
            wait for signal iter_fin
            pthread_mutex_lock(&queuing);
        */
        pthread_mutex_unlock(&queuing);  // должна освободить объект мьютекса, на который ссылается мьютекс
        Body *t = new_bodies;
        new_bodies = bodies;
        bodies = t;
    }

    write_to_file("output.csv", output_text);
    finsish = true;
    pthread_mutex_lock(&queuing);
    pthread_cond_broadcast(&processing);
    pthread_mutex_unlock(&queuing);

    for (int j = 0; j < num_thread; ++j)
        pthread_join(workers[j], NULL);  //  блокирует вызывающий поток, пока указанный поток не завершится.

    // TODO: ОБЪЯСНИТЬ ЗАЧЕМ !!!!
    pthread_mutex_destroy(&queuing);
    pthread_cond_destroy(&iter_fin);
    pthread_cond_destroy(&processing);

    // free(workers); /////////////////////////// ??????
    GET_TIME(end_t);

    std::cout << "TIME: " << end_t - start_t;

    return 0;
}