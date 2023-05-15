#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <memory>
#include <assert.h>
#include "log/log.h"
#include "timer/timer.h"
#include "http/http.h"
#include "threadpool/threadpool.h"

#include "../setting/setting.h"

class webserver{
    public:
        webserver();
        ~webserver();
        void init();
    private:
        http *users; //http链接的数组
        int epollfd; //epoll
        int myport; //端口号
        int listenfd; //创建套接字
        void listenevent(); //开始监听
        void dealevent();//处理事件
        void dealnewcon(); //处理新链接
        void dealread(int sockfd);//处理读
        void dealwrite(int sockfd);//处理写
        void dealtimer();//处理超时

        bool server_run = true;
        bool time_out = false;

        //超时相关
        int pipefd[2];//两个链接好的套接字，0读1写
        timer_util timeutil;

        //线程池相关
        threadpool<http> *pool;
        int thread_num;

        //epoll_event相关
        epoll_event events[MAX_EVENT_NUMBER];

        //数据库相关
        sqlconnect *sqlPool;
};

#endif