#ifndef MYTHREADPOOL_H
#define MYTHREADPOOL_H

#include <pthread.h>
#include <thread>

#include "Queue.h"
#include "Vector.h"

class MyThreadPool {
    public:
        struct Job {
            void (*Function)(void *); // 任务函数
            void* Data;
        };

        struct Worker {
            Worker():Terminate(false), Pool(nullptr) {}
            pthread_t ThreadID{};
            bool Terminate; // 判断线程是否终止
            MyThreadPool *Pool;
        };

    public:
        MyThreadPool()=default;
        explicit MyThreadPool(ui maxWorker);
        ~MyThreadPool();

    public:
        void* CacheProcess(void* data); // 控制线程池运行

        void StopAll(); // 停止所有线程

        static void* Run(void* data); // 执行任务

        void* ThreadLoop(void* data); // 线程循环

        void PushJob(void (*Function)(void*), void* Data);

    protected:
        struct Locker {
            explicit Locker(pthread_mutex_t *mutex):mutex(mutex) {
                ui attempt=1;
                constexpr ui maxAttempts=1000;
                while(pthread_mutex_trylock(mutex)!=0&&attempt<maxAttempts)
                    std::this_thread::sleep_for(std::chrono::microseconds(attempt<<=1)); // 等待锁
                if (attempt>=maxAttempts) locked=false; // 极端情况
                else locked=true;
            }
            ~Locker() {pthread_mutex_unlock(mutex);}

            bool locked;
            pthread_mutex_t *mutex;
        };

    private:
        ui maxWorker{};
        ui freeWorker{}; // 空闲线程数
        bool terminate{};

        Queue<Job*> taskList; // 任务队列
        Vector<Worker> workers;
        pthread_t controlThreadID{};
        pthread_mutex_t mutex{};
        pthread_mutex_t counterMutex{};
        pthread_cond_t cond{};
        pthread_cond_t cacheCond{}; // 用于控制线程池运行
};

#endif //MYTHREADPOOL_H
