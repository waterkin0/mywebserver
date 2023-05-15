#include "log.h"

log* log::getlog(){
    static log tmp;
    return &tmp;
}

void log::log_init() {
    mythread = new thread(&log::log_work, this);
    time_t now = time(NULL);//获取当前时间
    time_now = localtime(&now);
    string time_string = to_string((time_now->tm_year + 1900) * 10000 + (time_now->tm_mon + 1)*100 + time_now->tm_mday);
    myfd.open(LOG_FILE + "/" + time_string + ".log", ios::out | ios::app);
}

void log::log_work() {
    while(is_run){
        std::unique_lock<std::mutex> lock(mux);
        cv.wait(lock); //线程被创建，但是开始等待条件变量给的信号，lock自动解锁直到被唤醒
        if(log_queue.empty()) continue;
        newday();
        while(!log_queue.empty()) {
            string t = log_queue.front();
            t = ready(t);
            myfd << t << endl;
            myfd.flush();
            log_queue.pop();
        }
        //对t进行处理,即日志写入
    }
}

void log::newday() {
    time_t now = time(NULL);//获取当前时间
    tm* local_time = localtime(&now);
    if(local_time->tm_year != time_now->tm_year && local_time->tm_mon != time_now->tm_mon && local_time->tm_mday != time_now->tm_mday) {
        myfd.close();
        string time_string = to_string((time_now->tm_year + 1900) * 10000 + (time_now->tm_mon + 1)*100 + time_now->tm_mday);
        myfd.open(LOG_FILE + '/' + time_string + ".log", ios::out | ios::app);//重开文件
    }
}

string log::ready(string t) {
    time_t tmp = time(NULL);
    tm* now = localtime(&tmp);
    char tstr[20];
    strftime(tstr, sizeof(tstr), "%T", now);
    t = "[" + (string)tstr + "]" + t;
    return t;
}

void log::log_write(string t) {
    if (is_run){
        std::unique_lock<std::mutex> lock(mux);
        if (log_queue.size() >= MAXQUEUE_SIZE){//队列太长了
            lock.unlock();
            return;
        }
        log_queue.push(t);
        cv.notify_one();
    }
}