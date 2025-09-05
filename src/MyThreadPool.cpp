#include "MyThreadPool.h"

#include <mutex>

MyThreadPool::MyThreadPool(const ui maxWorker, const ui maxJob): maxWorker(maxWorker), maxJob(maxJob) {
    if(maxWorker<1||maxJob<1)return; // 线程数和任务数必须大于0
    if(pthread_cond_init(&cond, nullptr)!=0)return; // 初始化条件变量
    if(pthread_cond_init(&controlCond, nullptr)!=0)return;
    if(pthread_cond_init(&mainCond, nullptr)!=0)return; // 初始化条件变量
    if(pthread_mutex_init(&mutex, nullptr)!=0)return; // 初始化互斥锁

    // 初始化worker
    workers.Resize(maxWorker+1);
    for(ui i=0;i<maxWorker;++i) {
        workers[i].Pool=this;
        if(pthread_create(&workers[i].ThreadID, nullptr, Run, &workers[i])!=0) {
            workers.Clear();
            return; // 创建线程失败
        }
        if(pthread_detach(workers[i].ThreadID)) {
            workers.Clear();
            return; // 分离线程失败
        }
        workers[i].Terminate=false;
    }

    freeWorker=maxWorker;
}

MyThreadPool::~MyThreadPool() {
    StopAll();
    workers.Clear();
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

void MyThreadPool::AddThreads(ui count) {
    if(count==0)return;
    const ui oldSize=workers.Size();
    workers.Resize(oldSize+count);

    for(ui i=oldSize;i<workers.Size();++i) {
        workers[i].Pool=this;
        if(const int ret=pthread_create(&workers[i].ThreadID, nullptr, Run, &workers[i]);ret!=0) {
            workers.Resize(oldSize);
            return; // 创建线程失败
        }
        if(pthread_detach(workers[i].ThreadID)) {
            workers.Resize(oldSize);
            return; // 分离线程失败
        }
        workers[i].Terminate=false;
    }

    Locker* locker=new Locker(&mutex);
    while (!locker->locked) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        delete locker;
        locker=new Locker(&mutex);
    }
    freeWorker+=count;
}

void* MyThreadPool::Control(void* data) {
    auto* worker=static_cast<Worker*>(data);

    worker->isWorking=true;
    while (true) {
        auto* locker=new Locker(&mutex);
        while (taskList.Empty()) {
            if (terminate) {
                delete locker;
                worker->isWorking=false;
                pthread_cond_wait(&mainCond, &mutex); // 通知主线程
                return nullptr;
            }
            pthread_cond_wait(&controlCond, &mutex); // 由PushJob()唤醒
        }
        while (freeWorker==0) pthread_cond_wait(&controlCond, &mutex);
        pthread_cond_signal(&cond);
    }
}

void MyThreadPool::StopAll() {
    terminate=true;
    pthread_cond_broadcast(&controlCond); // 唤醒控制线程

    for (const Worker& i:workers)
        while (i.isWorking) pthread_cond_wait(&mainCond, &mutex);
}

void* MyThreadPool::Run(void* data) {
    const auto* worker=static_cast<Worker*>(data);

    if(worker==nullptr||worker->Pool==nullptr)return nullptr;
    if (worker->ThreadID==worker->Pool->workers[0].ThreadID)
        return worker->Pool->Control(data);
    return worker->Pool->ThreadLoop(data);
}

void* MyThreadPool::ThreadLoop(void* data) {
    auto* worker=static_cast<Worker*>(data);
    while(true) {
        const auto* locker=new Locker(&mutex);
        if (!locker->locked)continue;
        pthread_cond_wait(&cond, &mutex);

        --freeWorker;
        auto job=taskList.Front();
        if(job!=nullptr) {
            taskList.Pop();
            delete locker;

            worker->isWorking=true;
            job->Function(job->Data);
            worker->isWorking=false;

            free(job->Data);
            free(job);
        }
        else delete locker;
        ++freeWorker;
    }
}

void MyThreadPool::PushJob(void (*Function)(void*), void* Data) {
    if(Function==nullptr)return;
    auto job=new Job;

    job->Function=Function;
    job->Data=Data;

    Locker* locker=new Locker(&mutex);
    while (!locker->locked) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        delete locker;
        locker=new Locker(&mutex);
    }

    if(taskList.Size()>=maxJob) {
        free(job->Data);
        free(job);
        delete locker;
        return;
    }

    taskList.Push(job);
    delete locker;
    pthread_cond_signal(&controlCond); // 唤醒线程
}
