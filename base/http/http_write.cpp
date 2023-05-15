#include "http.h"

//定义响应信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

bool http::write() {
    int temp = 0;
    if (myhttp.sendsize_now == 0) {//要写的内容为0
        resetfd(epollfd, sockfd, EPOLLIN);
        init();
        return true;
    }
    while (1) {//正常写
        temp = writev(sockfd, iv, iv_size);
        int iovc_add = 0;//iovc偏移，后续断点重续用
        if (temp > 0) { //正常发送
            //更新已发送字节
            myhttp.sendsize_all += temp;
            iovc_add = myhttp.sendsize_all - writesize_now;
            //这个值可能为负数，因为它代表是当前的位置与报文尾(也是文件头)的差距，即此时发送的字节可能小于报文长度，但不用担心，这种情况进不到下面用到这个参数的情况
        }
        if (temp == -1) { //出现错误
            if (errno == EAGAIN) { //错误是缓冲区已满
                if (myhttp.sendsize_all >= iv[0].iov_len) {//当前已经把头发完了，不需要再发头了
                    //不再继续发送头部信息
                    iv[0].iov_len = 0;
                    iv[1].iov_base = myhttp.file_address + iovc_add;
                    iv[1].iov_len = myhttp.sendsize_now;
                }
                //继续发送第一个iovec头部信息的数据
                else {
                    iv[0].iov_base = writebuf + myhttp.sendsize_now;
                    iv[0].iov_len = iv[0].iov_len - myhttp.sendsize_all;
                }
                resetfd(epollfd, sockfd, EPOLLOUT);
                return true;
            }
            if (myhttp.file_address) {//错误不是缓冲区已满，未知，关闭mmap映射
                munmap(myhttp.file_address, myhttp.file_stat.st_size);
                myhttp.file_address = 0;
            }
            return false;
        }
        myhttp.sendsize_now -= temp;
        if (myhttp.sendsize_now <= 0)//发送完毕
        {
            //cout << "发送完毕" << endl;
            if (myhttp.file_address) {//关闭mmap映射
                munmap(myhttp.file_address, myhttp.file_stat.st_size);
                myhttp.file_address = 0;
            }
            resetfd(epollfd,sockfd,EPOLLIN);
            //浏览器的请求为长连接
            if(myhttp.alive) {
                //重新初始化HTTP对象
                init();
                return true;
            }
            else {
                return false;
            }
        }
    }
}

bool http::process_write(HTTP_CODE ret) {
    switch(ret){
        case INTERNAL_ERROR://服务器内部错误,500
            add_stateline(500,error_500_title);//状态行//状态行
            //消息报头
            add_headers(strlen(error_500_form));
            if(!add_content(error_500_form))
                return false;
            break;
        case BAD_REQUEST://请求报文出错或者资源不存在,404
        case NO_RESOURCE:
            add_stateline(404,error_404_title);//状态行//状态行
            //消息报头
            add_headers(strlen(error_404_form));
            if(!add_content(error_404_form))
                return false;
            break;
        case FORBIDDEN_REQUEST://没权限访问,403
            add_stateline(403,error_403_title);//状态行//状态行
            //消息报头
            add_headers(strlen(error_403_form));
            if(!add_content(error_403_form))
                return false;
            break;
        case FILE_REQUEST://文件存在可以正常请求,200
            add_stateline(200,ok_200_title);
            //如果请求的资源存在
            if(myhttp.file_stat.st_size != 0) {
                add_headers(myhttp.file_stat.st_size);
                //第一个iovec指针指向响应报文缓冲区，长度指向m_write_idx
                iv[0].iov_base = writebuf;
                iv[0].iov_len= writesize_now;
                //第二个iovec指针指向mmap返回的文件指针，长度指向文件大小
                iv[1].iov_base = myhttp.file_address;
                iv[1].iov_len = myhttp.file_stat.st_size;
                iv_size = 2;
                //发送的全部数据为响应报文头部信息和文件大小
                myhttp.sendsize_now = writesize_now + myhttp.file_stat.st_size;
                return true;
            }
            else {
                //如果请求的资源大小为0，则返回空白html文件
                const char* ok_string="<html><body></body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string))
                    return false;
            }
            break;
        default:
            return false;
    }
    //除FILE_REQUEST状态外，其余状态只申请一个iovec，指向响应报文缓冲区
    iv[0].iov_base = writebuf;
    iv[0].iov_len = writesize_now;
    iv_size=1;
    myhttp.sendsize_now = writesize_now;
    return true;
}

bool http::add_response(const char *format, ...) {
    if (writesize_now >= WRITE_SIZE){
        LOG_WRITE("writebuf max");
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(writebuf + writesize_now, WRITE_SIZE - 1 - writesize_now, format, arg_list);
    writesize_now += len;
    va_end(arg_list);
    return true;
}

bool http::add_stateline(int status,const char* title) {
    return add_response("%s %d %s\r\n","HTTP/1.1",status,title);
}

//添加消息报头，具体的添加文本长度、连接状态和空行
bool http::add_headers(int content_len) {
    add_response("Content-Length:%d\r\n",content_len);//响应报文的长度
    add_response("Connection:%s\r\n",(myhttp.alive == true)?"keep-alive":"close");//连接是否持续
    //add_response("Content-Type:%s\r\n","text/html");//返回类型
    add_response("%s","\r\n");//空行
    return true;
}

//添加文本content
bool http::add_content(const char* content) {
    return add_response("%s",content);
}