#pragma once
#include <cstddef>
#include "defs.hpp"
#include <pthread.h>
#include <queue>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(size_t num_threads, Handler handler); 

    ~ThreadPool(); 

    void enqueue(SharedSocket socket);

    void shutdown();

private:
    std::vector<pthread_t> threads;
    std::queue<SharedSocket> tasks;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool stop;

    static void* worker(void* arg);

    Handler handler_;
};

