// Замер времени работы операций БД: добавление, поиск по ключу,
// удаление по ключу, поиск по неключевому полю (линейный).
#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include "db.h"

using namespace std::chrono;

const string BENCH_FILE = "bench.db";

double msElapsed(high_resolution_clock::time_point start) {
    auto end = high_resolution_clock::now();
    return duration<double, std::milli>(end - start).count();
}

int main() {
    std::vector<int> sizes = {100, 1000, 5000, 10000, 20000};

    std::cout << "N\tAdd_total_ms\tAdd_avg_us\tFind_total_ms(1000 searches)\tFind_avg_us\tDelete_total_ms(1000 del)\tDelete_avg_us\tLinearScan_ms(full file)\n";

    for (int N : sizes) {
        // чистая новая БД для каждого N
        g_index.clear();
        createDatabase(BENCH_FILE);

        // --- замер добавления N записей ---
        auto t0 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) {
            Record r;
            r.id = i;
            snprintf(r.name, sizeof(r.name), "Item%d", i);
            r.quantity = i % 100;
            r.price = (float)(i % 1000) + 0.5f;
            r.category = 'A' + (i % 5);
            r.deleted = false;
            r.next_free = -1;
            addRecord(BENCH_FILE, r);
        }
        double addTotalMs = msElapsed(t0);

        // --- замер поиска по ключу: 1000 случайных существующих id ---
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0, N - 1);
        int searchCount = (N < 1000) ? N : 1000;

        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < searchCount; i++) {
            int id = dist(rng);
            Record found;
            findByKey(BENCH_FILE, id, found);
        }
        double findTotalMs = msElapsed(t1);

        // --- замер удаления по ключу: 1000 (или N, если меньше) случайных id ---
        int delCount = (N < 1000) ? N / 2 : 1000;
        std::vector<int> toDelete;
        for (int i = 0; i < delCount; i++) toDelete.push_back(i); // первые delCount id гарантированно есть

        auto t2 = high_resolution_clock::now();
        for (int id : toDelete) {
            deleteByKey(BENCH_FILE, id);
        }
        double delTotalMs = msElapsed(t2);

        // --- замер линейного прохода по всему файлу (поиск по неключевому полю) ---
        auto t3 = high_resolution_clock::now();
        findByCategory(BENCH_FILE, 'Z'); // заведомо несуществующая категория -> гарантированно читает весь файл
        double scanMs = msElapsed(t3);

        std::cout << N << "\t"
                  << addTotalMs << "\t"
                  << (addTotalMs * 1000.0 / N) << "\t"
                  << findTotalMs << "\t"
                  << (findTotalMs * 1000.0 / searchCount) << "\t"
                  << delTotalMs << "\t"
                  << (delTotalMs * 1000.0 / delCount) << "\t"
                  << scanMs
                  << "\n";
    }

    return 0;
}
