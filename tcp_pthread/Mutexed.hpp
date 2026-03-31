#pragma once
#include <memory>
#include <pthread.h>
#include <utility>

template <typename T> class Mutexed
{
public:
    class MutexedRef
    {
    public:
        MutexedRef(T& obj_ref, pthread_mutex_t* mut_ptr)
            : obj_ref_(obj_ref)
            , mutex_ptr_(mut_ptr)
        {
            pthread_mutex_lock(mutex_ptr_);
        }

        T& operator*()
        {
            return obj_ref_;
        }

        T* operator->()
        {
            return std::addressof(obj_ref_);
        }

        ~MutexedRef()
        {
            if (mutex_ptr_)
            {
                pthread_mutex_unlock(mutex_ptr_);
            }
        }

    private:
        pthread_mutex_t* mutex_ptr_ { nullptr };
        T& obj_ref_; // изза этого не можем скопировать Ref. И хорошо
    };

    Mutexed(T&& obj)
        : obj_(std::forward<T>(obj))
    {
        pthread_mutex_init(&mutex_, nullptr);
    }

    /**
     * Не допускайте ситуации, когда MutexedRef живет дольше, чем Mutexed. В
     * идеале, не сохраняйте никуда MutexedRef и пользуйтесь всегда
     * Mutexed::get()::operator*()
     */
    MutexedRef get() { return MutexedRef(obj_, &mutex_); }

    ~Mutexed()
    {
        pthread_mutex_destroy(&mutex_);
    }


private:
    pthread_mutex_t mutex_;
    T obj_;
};
