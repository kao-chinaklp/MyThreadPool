#ifndef QUEUE_H
#define QUEUE_H

typedef unsigned int ui;

template<class T>
class Queue {

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
        T* Data; // 队列头指针
        ui Head; // 队列头指针
        ui Tail; // 队列尾指针
        ui Capacity; // 队列容量
};

template<class T>
Queue<T>::Queue(): Head(0), Tail(0) {
    Capacity=1; // 默认容量为1
    Data=new T[Capacity];
}

template<class T>
Queue<T>::~Queue() {
    delete[] Data;
}

template<class T>
void Queue<T>::Push(const T& val) {
    if((Tail+1)%Capacity==Head) { // 队列满
        T* NewData=new T[Capacity*2]; // 扩容
        for(ui i=0; i<Capacity; ++i) {
            NewData[i]=Data[(Head+i)%Capacity];
        }
        delete[] Data;
        Data=NewData;
        Head=0;
        Tail=Capacity;
        Capacity*=2;
    }
    Data[Tail]=val;
    Tail=(Tail+1)%Capacity;
}

template<class T>
void Queue<T>::Pop() {
    if(Head!=Tail) {
        Head=(Head+1)%Capacity;
    }
}

template<class T>
T& Queue<T>::Front() {
    return Data[Head];
}

template<class T>
T& Queue<T>::Back() {
    return Data[(Tail-1+Capacity)%Capacity];
}

template<class T>
bool Queue<T>::Empty()const {
    return Head==Tail;
}

template<class T>
unsigned int Queue<T>::Size()const {
    return (Tail-Head+Capacity)%Capacity;
}

template<class T>
void Queue<T>::Clear() {
    Head=Tail=0;
}

#endif //QUEUE_H
