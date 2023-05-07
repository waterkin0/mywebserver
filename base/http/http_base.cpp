#include "http.h"

int http::epollfd = -1;
int http::user_num = 0;
int setnonblocking(int fd)   //设置套接字为非阻塞套接字
{
    int old_option = fcntl( fd, F_GETFL );//取得fd的文件状态标志
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );//设置为非阻塞套接字
    return old_option;
}

void addfd(int epollfd, int fd, bool oneshot) {
    epoll_event event;//定义监听的事件类型
    event.data.fd = fd; 
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(oneshot) event.events |= EPOLLONESHOT;
    //开启EPOLLONESHOT，因为我们希望每个子socket在任意时刻都只被一个线程处理,而主线程那个则不需要，如果将主线程也oneshot了，那么将无法进入产生第二次链接
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event); //四个参数分别是1.要操作内核的文件描述符，即epoll_create返回的，2.操作类型，3.所要操作用户的文件描述符，4.指定监听类型
    setnonblocking(fd);//将这个套接字变为非阻塞
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void resetfd(int epollfd, int fd, int ev){//重置事件
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http::getsockfd(){
    return sockfd;
}

void http::init() {
    readsize_now = 0;
    writesize_now = 0;
    checksize_now = 0;
    iv_size = 0;
    mainstate = CHECK_STATE_REQUESTLINE;
    memset(readbuf, '\0', READ_SIZE);
    memset(writebuf, '\0', WRITE_SIZE);
    memset(httproot_now, '\0', FILENAME_SIZE);
}

void http::init(int sockfd,const sockaddr_in &addr){
    this->sockfd = sockfd;//套接字编号
    this->address = addr;//ip地址等信息
    this->t = 0;
    user_num++;
    //cout << "新的客户：" << sockfd << "当前用户数"<< ++user_num << endl;
    addfd(epollfd, sockfd, true);
    init();
}

//ET边缘触发模式，要一次全部读完
bool http::readall(){
    // cout << "开始读数据：" << sockfd << endl;
    if(readsize_now >= READ_SIZE){
        return false;
    }
    while(true){
        int byte_read = recv(sockfd, readbuf + readsize_now, READ_SIZE - readsize_now, 0);
        if(byte_read == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK)//对非阻塞socket而言,EAGAIN不是一种错误,在这里是读完了
               break;
            return false;
        }
        else if(byte_read==0){
            return false;
        }
        // cout << recvbuf + readsize_now << endl;
        readsize_now += byte_read;//修改m_read_idx的读取字节数
    }
    return true;
}

void http::process() { 
    HTTP_CODE ret = process_read();//读取报文
    if(ret == NO_REQUEST) {//请求不完整，继续监听
        resetfd(epollfd, sockfd, EPOLLIN);
        return;
    }
    bool write_ret = process_write(ret);//创建返回报文
    if (!write_ret) {
        removefd(epollfd, sockfd);
        sockfd = -1;
        user_num--;
    }
    resetfd(epollfd, sockfd, EPOLLOUT);
}

void http::add_timer(timer* t){
    this->t = t;
}

timer* http::get_timer(){
    return this->t;
}