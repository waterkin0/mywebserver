#ifndef TIMER_H
#define TIMER_H

#define TIMEOUT 3

#include <signal.h>
#include <assert.h>
#include "../http/http.h"

class timer//定时器，即链表节点
{
public:
    timer() : prev(NULL), next(NULL) {}

    //定时器相关链接资源
    sockaddr_in address;
    int sockfd;

    time_t expire;//时间戳,定义超时时间

    //链表相关
    timer *prev;
    timer *next;
};

class timer_list{//定时器链表类.根据时间升序排列
    public:
    timer_list() : head(NULL), tail(NULL) {}
    ~timer_list() {
        timer *tmp = head;
        while (tmp) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    void add_timer(timer *t);
    void adjust_timer(timer *t);
    void del_timer(timer *t, bool dele);
    void tick();

private:
    timer *head;
    timer *tail;
};

class timer_util{//只是一堆工具函数的整合而已，你也可以不用放里面
    public:
        static timer_list timerlst;//链表的具体实现

        timer_util() {};
        ~timer_util() {};
        static int *pipefd;
        static void sig_handler(int sig);//信号处理函数
        static int epollfd;
        void add_sig(int sig, void(handler)(int), bool restart = true);//设置信号
        void timer_handler(bool isout);//定时处理
};

#endif