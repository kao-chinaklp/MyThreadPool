#include "MyThreadPool.h"

#include <cstdlib>
#include <cstring>

MyThreadPool::MyThreadPool(int numWorkers, ui maxJobs) {
    if(numWorkers<1||maxJobs<1)return; // 线程数和任务数必须大于0
    if(pthread_cond_init(&JobsCond, nullptr)!=0)return; // 初始化条件变量
    if(pthread_mutex_init(&JobsMutex, nullptr)!=0)return; // 初始化互斥锁

    // 初始化worker
    Workers=new Worker[numWorkers];
    for(int i=1;i<=numWorkers;++i) {
        Workers[i].Pool=this;
        if(const int ret=pthread_create(&Workers[i].ThreadID, nullptr, Run, &Workers[i]); ret!=0) {
            delete[] Workers;
            return; // 创建线程失败
        }
        if(pthread_detach(Workers[i].ThreadID)) {
            delete[] Workers;
            return; // 分离线程失败
        }
        Workers[i].Terminate=false;
    }
}

MyThreadPool::~MyThreadPool() {
    for(int i=1;i<=SumThread;++i) {
        Workers[i].Terminate=true;
    }

    pthread_cond_broadcast(&JobsCond);
    pthread_mutex_lock(&JobsMutex);
    pthread_mutex_unlock(&JobsMutex);

    delete[] Workers;
    pthread_cond_destroy(&JobsCond);
    pthread_mutex_destroy(&JobsMutex);
}


int MyThreadPool::PushJob(void (*Function)(void *), void *Data) {
    auto* job=static_cast<struct Job*>(malloc(sizeof(struct Job)));
    if(job==nullptr)return -1;

    memset(job, 0, sizeof(struct Job));
    job->Data=malloc(sizeof(Data));
    memcpy(job->Data, Data, sizeof(job->Data));
    job->Data=Data;

    AddJob(job);

    return 1;
}


bool MyThreadPool::AddJob(Job* job) {
    pthread_mutex_lock(&JobsMutex);
    if(JobsList.Size()>=MaxJobs) {
        pthread_mutex_unlock(&JobsMutex);
        return false;
    }

    JobsList.Push(job);
    pthread_cond_signal(&JobsCond); // 唤醒线程
    pthread_mutex_unlock(&JobsMutex);
    return true;
}


void* MyThreadPool::Run(void *Data) {
    auto* worker=static_cast<Worker *>(Data);
    worker->Pool->ThreadLoop(worker);
    return nullptr;
}

void MyThreadPool::ThreadLoop(void *Data) {
    auto* worker=static_cast<Worker *>(Data);
    while(true) {
        pthread_mutex_lock(&JobsMutex);
        while(JobsList.Size()==0) {
            if(worker->Terminate)break;
            pthread_cond_wait(&JobsCond,&JobsMutex);
        }

        // 获取任务
        Job* job=JobsList.Front();
        JobsList.Pop();
        pthread_mutex_unlock(&JobsMutex);

        FreeThread--;
        worker->IsWorking=true;
        // 执行任务
        job->Function(job->Data);
        worker->IsWorking=false;
        FreeThread++;

        free(job->Data);
        free(job);
    }

    free(worker);
    pthread_exit(nullptr);
}