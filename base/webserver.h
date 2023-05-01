#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "http/http.h"
#include "threadpool/threadpool.h"
#include <assert.h>

#define MAX_FD 65535//最大文件描述数
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define MY_PORT 1234//端口号

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
        void dealread(int sockfd);
        void dealwrite(int sockfd);

        //线程池相关
        threadpool<http> *pool;
        int thread_num;

        //epoll_event相关
        epoll_event events[MAX_EVENT_NUMBER];

        //数据库相关
        sqlconnect *m_connPool;
};

webserver::webserver() {
    //http类对象
    pool = new threadpool<http>(m_connPool);
    users = new http[MAX_FD];
    myport = MY_PORT;
}

webserver::~webserver() {
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;
}

void webserver::init() {
    cout << "服务器初始化..." << endl;
    listenevent();
    dealevent();
}

void webserver::listenevent() {//创建套接字，链接ip端口并开始监听，创建事件表并连接
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  	// AF_INET :   表示使用 IPv4 地址		可选参数
    // SOCK_STREAM 表示使用面向连接的数据传输方式，
    // IPPROTO_TCP 表示使用 TCP 协议

    //将套接字和IP、端口绑定
    struct sockaddr_in serv_addr; //用sockaddr_in进行初始化，bind时转化为sockaddr
    memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //允许所有ip地址链接
    serv_addr.sin_port = htons(myport);  //端口 htons()作用是将端口号由主机字节序转换为网络字节序的整数值
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));/* SO_REUSEADDR 允许端口被重复使用 */
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); /* 绑定socket和它的地址 */

    //进入监听状态，等待用户发起请求
    listen(listenfd, 20);

    //创建内核事件表
    epollfd = epoll_create(5);
    http::epollfd = epollfd;
    assert(epollfd != -1);
    addfd(epollfd, listenfd);
}

void webserver::dealevent() {
    cout << "服务器初始化完成，开始运行" << endl;
    bool server_run = true;
    int n = 0;
    while(server_run) {
        cout << "新的循环:"<< n++ << endl;
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);//1.要操作的内核文件描述符，即 epoll_create 的返回值2.存储内核事件表中得到的检测事件集合3.内核 events 的最大 size，4.指定超时时间
        /* 遍历数组处理这些已经就绪的事件 */
        for(int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;  // 事件表中就绪的socket文件描述符
            // cout << events[i].events << endl;
            if(sockfd == listenfd) {  // 确认socket文件是listenfd的，并处理
                dealnewcon();
            }
            else if (events[i].events & EPOLLRDHUP) {//客户端断开连接
                cout << "客户端断开连接" << endl;
                removefd(epollfd, sockfd);
                close(sockfd);
            }
            else if (events[i].events & EPOLLIN) {//处理读数据,有连接也会触发
                dealread(sockfd);
            }
            else if (events[i].events & EPOLLOUT) {//处理写数据
                dealwrite(sockfd);
            }
        }
    }
    cout << "服务器停止" << endl;
}

void webserver::dealnewcon() {//将fd转为clientfd操作并epoll_wait使其在新循环内操作
    cout << "处理新数据" << endl;
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    /* ET模式，需要循环接收数据 */
    while(1){
        int clientfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);
        if(clientfd < 0) break;
        if(http::user_num >= MAX_FD){//超过限制
            break;
        }
        // char str[] = "Hello World!";
        // write(clientfd, str, sizeof(str));
        users[clientfd].init(clientfd, client_address);
    }
    cout << "处理完毕" << endl;
}

void webserver::dealread(int sockfd) {
    if(users[sockfd].readall()){ //*
        pool->appendrequest(users + sockfd);
    }
}

void webserver::dealwrite(int sockfd) {
    cout << "处理写数据,其套接字为:" << sockfd  << endl;
    cout << "处理完毕" << endl;
}

#endif