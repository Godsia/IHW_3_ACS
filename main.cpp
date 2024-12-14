#include <iostream>
#include <pthread.h>
#include <queue>
#include <vector>
#include <unistd.h>
#include <random>
#include <cstdlib> // Для atoi

using namespace std;

struct Task {
    int program_id;
    bool is_correct; // Статус программы: корректна или требует исправления
};

// Указатели на очереди, мьютексы и условные переменные
queue<Task>* task_queues;
pthread_mutex_t* mutexes;
pthread_cond_t* conditions;

// Генератор случайных чисел
random_device rd;
mt19937 gen(rd());
uniform_int_distribution<> random_correct(0, 1); // Результат проверки: 0 - неправильно, 1 - правильно

int PROGRAMMER_COUNT; // Количество программистов

void* programmer_thread(void* arg) {
    int id = *(int*)arg; // ID программиста
    int program_count = 0;

    while (true) {
        // Написание программы
        sleep(1); // Симуляция написания программы
        program_count++;
        cout << "Программист " << id << " написал программу " << program_count << "." << endl;

        // Выбор случайного проверяющего
        uniform_int_distribution<> random_reviewer(1, PROGRAMMER_COUNT - 1);
        int reviewer_id = (id + random_reviewer(gen)) % PROGRAMMER_COUNT;

        // Отправка программы на проверку
        Task new_task = {program_count, true}; // Изначально программа считается правильной
        pthread_mutex_lock(&mutexes[reviewer_id]);
        task_queues[reviewer_id].push(new_task);
        pthread_cond_signal(&conditions[reviewer_id]); // Уведомление проверяющего
        pthread_mutex_unlock(&mutexes[reviewer_id]);

        // Ожидание результата проверки
        bool is_correct = false;
        while (!is_correct) {
            sleep(1); // Симуляция ожидания проверки
            pthread_mutex_lock(&mutexes[id]);

            // Проверка очереди задач
            if (!task_queues[id].empty()) {
                Task task = task_queues[id].front();
                task_queues[id].pop();

                is_correct = task.is_correct;
                if (!is_correct) {
                    cout << "Программист " << id << " получил ошибку в программе " << task.program_id << " и исправляет её." << endl;
                } else {
                    cout << "Программист " << id << " получил одобрение программы " << task.program_id << " и начинает писать новую." << endl;
                }
            }

            pthread_mutex_unlock(&mutexes[id]);
        }
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Использование: ./program <количество_программистов>" << endl;
        return 1;
    }

    // Считываем количество программистов из аргументов командной строки
    PROGRAMMER_COUNT = atoi(argv[1]);
    if (PROGRAMMER_COUNT < 2) {
        cerr << "Количество программистов должно быть не менее 2." << endl;
        return 1;
    }

    // Инициализация очередей, мьютексов и условных переменных
    task_queues = new queue<Task>[PROGRAMMER_COUNT];
    mutexes = new pthread_mutex_t[PROGRAMMER_COUNT];
    conditions = new pthread_cond_t[PROGRAMMER_COUNT];

    for (int i = 0; i < PROGRAMMER_COUNT; i++) {
        pthread_mutex_init(&mutexes[i], nullptr);
        pthread_cond_init(&conditions[i], nullptr);
    }

    pthread_t* threads = new pthread_t[PROGRAMMER_COUNT];
    int* ids = new int[PROGRAMMER_COUNT];

    // Создание потоков программистов
    for (int i = 0; i < PROGRAMMER_COUNT; i++) {
        ids[i] = i;
        pthread_create(&threads[i], nullptr, programmer_thread, &ids[i]);
    }

    // Ожидание завершения потоков (бесконечный цикл - программа должна быть остановлена вручную)
    for (int i = 0; i < PROGRAMMER_COUNT; i++) {
        pthread_join(threads[i], nullptr);
    }

    // Уничтожение мьютексов, условных переменных и освобождение памяти
    for (int i = 0; i < PROGRAMMER_COUNT; i++) {
        pthread_mutex_destroy(&mutexes[i]);
        pthread_cond_destroy(&conditions[i]);
    }
    delete[] task_queues;
    delete[] mutexes;
    delete[] conditions;
    delete[] threads;
    delete[] ids;

    return 0;
}
