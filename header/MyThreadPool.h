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
        Worker():Terminate(false), isWorking(false), Pool(nullptr) {}
        pthread_t ThreadID{};
        bool Terminate; // 判断线程是否终止
        bool isWorking; // 判断线程是否在工作
        MyThreadPool *Pool;
    };

public:
    MyThreadPool()=default;
    MyThreadPool(ui maxWorker, ui maxJob);
    ~MyThreadPool();

    void AddThreads(ui count); // 添加线程

public:
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
            if (attempt>=maxAttempts)std::this_thread::yield(); // 极端情况
        }
        ~Locker() {pthread_mutex_unlock(mutex);}

        pthread_mutex_t *mutex;
    };

private:
    ui maxWorker{};
    ui maxJob{};
    ui freeWorker{}; // 空闲线程数
    Queue<Job*> taskList; // 任务队列
    Vector<Worker> workers;
    pthread_mutex_t mutex{};
    pthread_cond_t cond{};
    pthread_cond_t mainCond{}; // 用于控制主线程等待
};

#endif //MYTHREADPOOL_H
