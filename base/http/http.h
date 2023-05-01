#ifndef HTTP_H
#define HTTP_H

//同意ET模式了
//设置读取文件的名称m_real_file大小
#define FILENAME_SIZE 200
//设置读缓冲区m_read_buf大小
#define READ_SIZE 2048
//设置写缓冲区m_write_buf大小
#define WRITE_SIZE 1024

#include <netinet/in.h>
#include <sys/epoll.h>
#include <iostream>
#include <fcntl.h> //fcntl
#include <errno.h> //errno
#include <unistd.h>//close
#include <string.h>//memset

using namespace std;

class http{
    private:
        //报文的请求方法，本项目只用到GET和POST
        enum METHOD{GET,POST,HEAD,PUT,DELETE,TRACE,OPTIONS,CONNECT,PATH};
        //主状态机的状态
        enum CHECK_STATE{CHECK_STATE_REQUESTLINE,CHECK_STATE_HEADER,CHECK_STATE_CONTENT};
        //报文解析的结果
        enum HTTP_CODE{NO_REQUEST,GET_REQUEST,BAD_REQUEST,NO_RESOURCE,FORBIDDEN_REQUEST,FILE_REQUEST,INTERNAL_ERROR,CLOSED_CONNECTION};
        //从状态机的状态
        enum LINE_STATUS{LINE_OK,LINE_BAD,LINE_OPEN};
        int sockfd;
        sockaddr_in address;
        char recvbuf[READ_SIZE];//存储读取的请求报文数据
        int readsize_now;//现在读取的长度
        void init();
    public:
        http() {}
        ~http() {}
        static int epollfd;//epollfd
        static int user_num; //当前用户(链接)个数
        int getsockfd();
        void init(int sockfd,const sockaddr_in &addr);
        bool readall();
};

int http::epollfd = -1;
int http::user_num = 0;
int setnonblocking(int fd)   //设置套接字为非阻塞套接字
{
    int old_option = fcntl( fd, F_GETFL );//取得fd的文件状态标志
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );//设置为非阻塞套接字
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;//定义监听的事件类型
    event.data.fd = fd; 
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    /* 开启EPOLLONESHOT，因为我们希望每个socket在任意时刻都只被一个线程处理 */
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event); //四个参数分别是1.要操作内核的文件描述符，即epoll_create返回的，2.操作类型，3.所要操作用户的文件描述符，4.指定监听类型
    setnonblocking(fd);//将这个套接字变为非阻塞
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void resetfd(int epollfd,int fd){//重置事件
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http::getsockfd(){
    return sockfd;
}

void http::init() {
    readsize_now = 0;
    memset(recvbuf, '\0', READ_SIZE);
}

void http::init(int sockfd,const sockaddr_in &addr){
    this->sockfd = sockfd;//套接字编号
    this->address = addr;//ip地址等信息
    cout << "创建了cilentfd,套接字编号为：" <<sockfd << endl;
    addfd(epollfd, sockfd);
    init();
}

//ET边缘触发模式，要一次全部读完
bool http::readall(){
    cout << "开始读数据：" << sockfd << endl;
    if(readsize_now >= READ_SIZE){
        resetfd(epollfd, sockfd);
        return false;
    }
    while(true){
        int byte_read = recv(sockfd, recvbuf + readsize_now, READ_SIZE - readsize_now, 0);
        if(byte_read == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK)//对非阻塞socket而言,EAGAIN不是一种错误,在这里是读完了
               break;
            return false;
        }
        else if(byte_read==0){
            return false;
        }
        cout << recvbuf + readsize_now << endl;
        readsize_now += byte_read;//修改m_read_idx的读取字节数
    }
    return true;
}

#endif