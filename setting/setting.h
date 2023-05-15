#ifndef SETTING_H
#define SETTING_H

//webserver相关
#define MAX_FD 65535//最大文件描述数,即用户数
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define MY_PORT 1234//端口号

//http相关
#define FILENAME_SIZE 200//设置读取文件的名称m_real_file大小
#define READ_SIZE 2048//设置读缓冲区m_read_buf大小
#define WRITE_SIZE 1024//设置写缓冲区m_write_buf大小

//日志相关
#define MAXQUEUE_SIZE 20 //日志队列大小
#define LOG_SIZE 300 //日志最大行数
#define LOG_FILE (string)"log"//日志文件的文件夹目录，默认生成的日志名称是当天时间
#define LOG_OPEN 0//日志开启

//线程相关
#define MAX_THREADPOOL_NUM 10 //线程池的线程数
#define MAX_REQUESTS_NUM 10000 //请求队列中允许的最大请求数

//定时器相关
#define TIMEOUT 3

#endif