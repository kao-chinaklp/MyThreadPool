#include "MyThreadPool.h"

MyThreadPool::MyThreadPool(const ui maxWorker): maxWorker(maxWorker) {
    if(maxWorker<1)return; // 线程数和任务数必须大于0
    if(pthread_cond_init(&cond, nullptr)!=0)return; // 初始化条件变量
    if(pthread_cond_init(&cacheCond, nullptr)!=0)return;
    if(pthread_mutex_init(&mutex, nullptr)!=0)return; // 初始化互斥锁
    if(pthread_mutex_init(&counterMutex, nullptr)!=0)return;

    // 初始化worker
    workers.Resize(maxWorker+1);
    for(ui i=0;i<=maxWorker;++i) {
        workers[i].Pool=this;
        if(pthread_create(&workers[i].ThreadID, nullptr, Run, &workers[i])!=0) {
            workers.Clear();
            return; // 创建线程失败
        }
    }

    terminate=false;
    freeWorker=maxWorker;
}

MyThreadPool::~MyThreadPool() {
    StopAll();
    workers.Clear();
    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&cacheCond);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&counterMutex);
}

void* MyThreadPool::CacheProcess(void* data) {
    while (true) {
        Locker locker(&mutex);
        while (taskList.Empty()) {
            if (terminate) return nullptr;
            pthread_cond_wait(&cacheCond, &mutex);
        }
        while (freeWorker==0) pthread_cond_wait(&cacheCond, &mutex);
        pthread_cond_signal(&cond); // 唤醒一个工作线程
    }
}

void MyThreadPool::StopAll() {
    terminate=true;
    pthread_cond_signal(&cacheCond); // 处理没处理完的任务
    pthread_cond_broadcast(&cond); // 唤醒所有线程

    // 等待所有线程结束
    for (const Worker& i:workers)
        pthread_join(i.ThreadID, nullptr);
}

void* MyThreadPool::Run(void* data) {
    const auto* worker=static_cast<Worker*>(data);

    if(worker==nullptr||worker->Pool==nullptr)return nullptr;
    if (worker->ThreadID==worker->Pool->workers[0].ThreadID)
        return worker->Pool->CacheProcess(data);
    return worker->Pool->ThreadLoop(data);
}

void* MyThreadPool::ThreadLoop(void* data) {
    while(true) {
        auto* locker=new Locker(&mutex);
        if (!locker->locked) {
            delete locker;
            continue;
        }
        while (taskList.Empty()&&!terminate) pthread_cond_wait(&cond, &mutex);

        if (terminate&&taskList.Empty()) {
            delete locker;
            break;
        }

        auto job=taskList.Front();
        taskList.Pop();
        delete locker;

        auto* counterLocker=new Locker(&counterMutex);
        --freeWorker;
        delete counterLocker;

        if(job!=nullptr) {
            job->Function(job->Data);

            free(job->Data);
            delete job;
        }
        counterLocker=new Locker(&counterMutex);
        ++freeWorker;
        delete counterLocker;

        pthread_cond_signal(&cacheCond);
    }

    return nullptr;
}

void MyThreadPool::PushJob(void (*Function)(void*), void* Data) {
    if(Function==nullptr)return;
    auto job=new Job;

    job->Function=Function;
    job->Data=Data;

    auto* locker=new Locker(&mutex);
    while (!locker->locked) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        delete locker;
        locker=new Locker(&mutex);
    }

    taskList.Push(job);
    delete locker;
    if (freeWorker>0) pthread_cond_signal(&cond);
    else pthread_cond_signal(&cacheCond);
}
