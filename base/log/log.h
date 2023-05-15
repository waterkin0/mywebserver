#ifndef LOG_H
#define LOG_H

#include "../../setting/setting.h"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <queue>

using namespace std;

class log{
    private:
        enum LOG_TYPE{ERROR, WARM, INFO, DEBUG};//四种日志类型
        log() {is_run = true; log_init();}
        ~log() {is_run = false;}
        queue<string> log_queue;//异步写入的缓冲队列
        thread* mythread;//处理异步写入的线程
        condition_variable cv;//条件变量
        mutex mux;       //保护请求队列的互斥锁
        tm* time_now;
        void log_work(); //写入日志的线程
        bool is_run;
        ofstream myfd;//写入文件流
        void newday();//判断是否是新的一天
        string ready(string t);//将写入内容格式化
    public:
        static log* getlog();
        void log_write(string t);
        void log_init(); //初始化
};

#define LOG_WRITE(format) if(LOG_OPEN){log::getlog()->log_write(format);}

#endif