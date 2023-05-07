#ifndef THREADPOOL_H
#define THREADPOOL_H
#define MAX_THREADPOOL_NUM 10 //线程池的线程数
#define MAX_REQUESTS_NUM 10000 //请求队列中允许的最大请求数

#include <queue>
#include <vector>
#include <cstdio>
#include <iostream>
#include "../sqlconnect.h"
#include "lock.h"
#include <sstream>
#include <typeinfo>
using namespace std;

template<typename T>
class threadpool{
    private:
        vector<thread> threads;       //描述线程池的数组，其大小为MAX_THREADPOOL_NUM
        queue<T*> requests; //请求队列,先进先出
        mutex mux;       //保护请求队列的互斥锁
        condition_variable cv;            //条件变量，确定是否有请求要处理
        sqlconnect *connPool;  //数据库链接
        bool is_run; //线程池是否在运行
        void work(); //每个线程运行的部分
    public:
        threadpool(sqlconnect *connPool);
        ~threadpool();
        bool appendrequest(T* t); //增加请求
        void stop(); //停止线程池
        void start(); //开始线程池
};

template<typename T>
threadpool<T>::threadpool(sqlconnect *connPool) : connPool(connPool), is_run(true) {
    start();
}

template<typename T>
threadpool<T>::~threadpool() {
    stop();
}

template<typename T>
void threadpool<T>::work(){
    // auto id = this_thread::get_id();
    // stringstream ss;
    // ss << id;
    // string mystring = ss.str();
    // string tmp = "thread " + mystring + " begin to work!";
    // cout << tmp << endl;
    while(is_run){
        std::unique_lock<std::mutex> lock(mux);
        cv.wait(lock); //线程被创建，但是开始等待条件变量给的信号，lock自动解锁直到被唤醒
        if(requests.empty()) continue;
        T* t = requests.front();
        //对t进行处理
        t->process();
        //cout << "!!!线程开始运行指定程序" << endl;
        requests.pop();
    }
    // tmp =  "thread " + mystring + " going to die!";
    // cout << tmp << endl;
}

template<typename T>
bool threadpool<T>::appendrequest(T* t){
    if (is_run){
        std::unique_lock<std::mutex> lock(mux);
        if (requests.size() >= MAX_REQUESTS_NUM){
            lock.unlock();
            return false;
        }
        requests.push(t);
        cv.notify_one();
        return true;
    }
    return false;
}

template<typename T>
void threadpool<T>::start(){
    is_run = true;
    for (int i = 0; i < MAX_THREADPOOL_NUM; i++){
        threads.push_back(thread(&threadpool::work, this));
    }
}

template<typename T>
void threadpool<T>::stop(){
    {
        std::unique_lock<std::mutex> lock(mux);
        is_run = false;
        cv.notify_all();
    }
    for (std::thread &thd : threads){
        if (thd.joinable()){
            thd.join();
        }
    }
    cout<< "线程池已被清空" << endl;
}

#endif