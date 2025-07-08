#ifndef MYTHREADPOOL_H
#define MYTHREADPOOL_H

#include <pthread.h>
#include <thread>

#include "Queue.h"

class MyThreadPool {
    private:
        typedef unsigned int ui;

        struct Worker {
            pthread_t ThreadID;
            bool Terminate; // 判断线程是否终止
            bool IsWorking; // 判断线程是否在工作
            MyThreadPool *Pool;
        };

        struct Job {
            void (*Function)(void *); // 任务函数
            void* Data;
        };

        struct MutexLocker {
            pthread_mutex_t* Mutex;
            explicit MutexLocker(pthread_mutex_t* mutex):Mutex(mutex) {
                ui attempt=1;
                constexpr ui maxAttempts=1000;
                while(pthread_mutex_trylock(Mutex)!=0&&attempt<maxAttempts)
                    std::this_thread::sleep_for(std::chrono::microseconds(attempt<<=1)); // 等待锁
                if (attempt>=maxAttempts)std::this_thread::yield(); // 极端情况，阻塞
            }
            ~MutexLocker() {pthread_mutex_unlock(Mutex);}
        };

    public:
        MyThreadPool(int numWorkers, ui maxJobs); // 初始化
        ~MyThreadPool(); // 析构函数
        int PushJob(void (*Function)(void *), void *Data); // 添加任务

    private:
        bool AddJob(Job* job); // 添加任务
        static void* Run(void* Data); // 回调函数
        void ThreadLoop(void* Data);

    private:
        Worker* Workers; // 线程队列
        Queue<Job*> JobsList; // 任务队列
        ui MaxJobs{}; // 最大任务数
        ui SumThread{}; // 线程数
        ui FreeThread{}; // 空闲线程数
        pthread_cond_t JobsCond{}; // 任务队列条件变量
        pthread_mutex_t JobsMutex{}; // 任务队列锁
};

#endif //MYTHREADPOOL_H
