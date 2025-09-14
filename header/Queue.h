#ifndef QUEUE_H
#define QUEUE_H

#include <mutex>
#include <stdexcept>

template<class T>
class Queue {
    using ui=unsigned int;

    public:
        Queue(); // 构造函数
        ~Queue(); // 析构函数
        void Push(const T& val); // 入队
        void Pop(); // 出队
        T& Front(); // 返回队首元素
        T& Back(); // 返回队尾元素
        [[nodiscard]] bool Empty()const; // 判断队列是否为空
        [[nodiscard]] ui Size()const; // 返回队列大小
        void Clear(); // 清空队列

    private:
        T* Data;
        ui Head; // 队列头指针
        ui queueSize;
        ui Capacity; // 队列容量
        mutable std::mutex mutex;
};

template<class T>
Queue<T>::Queue(): Head(0), queueSize(0), Capacity(1) {
    Data=new T[Capacity];
}

template<class T>
Queue<T>::~Queue() {
    std::lock_guard lock(mutex);
    delete[] Data;
}

template<class T>
void Queue<T>::Push(const T& val) {
    std::lock_guard lock(mutex);
    if(queueSize==Capacity) { // 队列满
        T* NewData=new T[Capacity*2]; // 扩容
        for(ui i=Head, len=Head+Capacity;i<len;++i)
            NewData[i-Head]=Data[i%Capacity];
        delete[] Data;
        Data=NewData;
        Head=0;
        Capacity*=2;
    }
    Data[((queueSize++)+Head)%Capacity]=val;
}

template<class T>
void Queue<T>::Pop() {
    std::lock_guard lock(mutex);
    if (Empty())throw std::underflow_error("Queue is empty");
    Head=(Head+1)%Capacity;
    --queueSize;
}

template<class T>
T& Queue<T>::Front() {
    if (Empty()) throw std::underflow_error("Queue is empty");
    return Data[Head];
}

template<class T>
T& Queue<T>::Back() {
    if (Empty()) throw std::underflow_error("Queue is empty");
    return Data[(Head+queueSize-1)%Capacity];
}

template<class T>
bool Queue<T>::Empty()const {
    return queueSize==0;
}

template<class T>
unsigned int Queue<T>::Size()const {
    return queueSize;
}

template<class T>
void Queue<T>::Clear() {
    std::lock_guard lock(mutex);
    delete[] Data;
    Data=new T[1];
    Capacity=1;
    Head=queueSize=0;
}

#endif //QUEUE_H
