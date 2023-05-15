#ifndef HTTP_H
#define HTTP_H

//统一ET模式了
#include "../../setting/setting.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <iostream>
#include <fcntl.h> //fcntl
#include <errno.h> //errno
#include <unistd.h>//close
#include <string.h>//memset
#include <sys/types.h>
#include <sys/stat.h>//stat
#include <sys/mman.h>//mmap
#include <stdarg.h>//va_list
#include <sys/uio.h>//iovc

using namespace std;

//报文的请求方法
enum METHOD{GET,POST};
//主状态机的状态
enum CHECK_STATE{CHECK_STATE_REQUESTLINE,CHECK_STATE_HEADER,CHECK_STATE_CONTENT};
//报文解析的结果
enum HTTP_CODE{NO_REQUEST,GET_REQUEST,NO_RESOURCE,BAD_REQUEST,FORBIDDEN_REQUEST,FILE_REQUEST,INTERNAL_ERROR};
//从状态机的状态
enum LINE_STATE{LINE_OK,LINE_BAD,LINE_OPEN};

int setnonblocking(int fd); //设置套接字为非阻塞套接字
void addfd(int epollfd, int fd, bool oneshot);
void removefd(int epollfd, int fd);
void resetfd(int epollfd, int fd, int ev);//重置事件

#include "../timer/timer.h"

class timer;

class http_mes{//用来存解析出的http请求信息
    public:
        char* url; //后续返回请求的前部分
        METHOD method;//请求方法
        int readsize_header;//已经读取的头的行数
        bool alive;//是否保持链接
        int contentsize;//内容长度
        char* host;//头部的HOST
        char* postdata;//post内的信息
        char* file_address; //请求的文件的地址(如果请求的是文件的话)
        struct stat file_stat;//文件具体的信息，例如权限

        int sendsize_now;//现在需要发送的数据大小，包括报文和文件大小
        int sendsize_all;//本次套接字已经发送的大小

        http_mes() {
            url = 0;
            readsize_header = 0;
            contentsize = 0;
            alive = false;
            host = 0;
            postdata = 0;
            file_address = 0;
            sendsize_now = 0;
            sendsize_all = 0;
        }
};

class http{
    private:
        int sockfd;
        sockaddr_in address;
        //网页地址
        const char* html_root="/home/ubuntu/TinyWebServer/mywebserver/html";
        char httproot_now[FILENAME_SIZE]; //后续处理的网页根目录
        char readbuf[READ_SIZE];//存储读取的请求报文数据
        int readsize_now;//现在读取的长度
        char writebuf[WRITE_SIZE];//存储写入的请求报文数据
        int writesize_now;//现在写入的长度
        int checksize_now;//现在处理到的地方
        CHECK_STATE mainstate;//主状态机状态
        http_mes myhttp;
        struct iovec iv[2];//写的缓存区
        int iv_size;//缓存区大小
        timer* t;//对应定时器
        void init();
        char* get_line(int n);//返回当前处理的地址
        HTTP_CODE process_read(); //读取http
        bool process_write(HTTP_CODE ret); //返回报文
        HTTP_CODE parse_request_line(char *text);//解析请求行
        HTTP_CODE parse_headers(char *text);//解析请求头
        HTTP_CODE parse_content(char *text);//解析请求内容
        LINE_STATE parse_line();//从状态机，判断并处理一行
        HTTP_CODE do_request();//对请求进行处理
        bool add_response(const char* format,...);//变参va_list，减少重复代码
        bool add_stateline(int status,const char* title);//添加状态行
        bool add_headers(int content_len);//添加头，包括后面的空行
        bool add_content(const char* content);//添加文本
        
    public:
        http() {}
        ~http() {}
        static int epollfd;//epollfd
        static int user_num; //当前用户(链接)个数
        int getsockfd();
        void init(int sockfd,const sockaddr_in &addr);
        bool readall();//读取
        bool write();//正式开写
        void process();
        void add_timer(timer* t);
        timer* get_timer();
};

#endif