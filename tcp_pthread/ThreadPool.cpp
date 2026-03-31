#include "ThreadPool.hpp"
#include "defs.hpp"
#include <stdexcept>

ThreadPool::ThreadPool(size_t num_threads, Handler handler)
    : stop(false), handler_(handler)
{
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);

    threads.resize(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
    {
        if (pthread_create(&threads[i], nullptr, &ThreadPool::worker, this)
            != 0)
        {
            throw std::runtime_error("Failed to create thread");
        }
    }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::enqueue(MutexedSocket socket)
{
    pthread_mutex_lock(&mutex);
    tasks.push(std::move(socket));
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void ThreadPool::shutdown()
{
    pthread_mutex_lock(&mutex);
    stop = true;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    for (auto& t : threads)
    {
        pthread_join(t, nullptr);
    }
}

void* ThreadPool::worker(void* arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    while (true)
    {
        pthread_mutex_lock(&pool->mutex);

        while (pool->tasks.empty() && !pool->stop)
        {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        if (pool->stop && pool->tasks.empty())
        {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        auto socket = std::move(pool->tasks.front());
        pool->tasks.pop();

        pthread_mutex_unlock(&pool->mutex);

        // Process the socket
        pool->handler_(std::move(socket));
    }

    return nullptr;
}

static void handle_socket(StreamSocket&& socket)
{
}
