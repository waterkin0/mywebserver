#include "timer.h"
#include <memory>

//静态变量定义
int *timer_util::pipefd = 0;
int timer_util::epollfd = 0;
timer_list timer_util::timerlst;
//链表实现
void timer_list::add_timer(timer *t) {
    if(!t) return;
    if(!head) {
        head = tail = t;
        return;
    }
    timer* tmp = new timer;//创建虚拟头结点，用于遍历
     timer* tmp2 = tmp;//用于后续释放内存
    tmp->next = head;
    while(tmp->next->expire < t->expire) {//下一个节点小于目标节点，向后遍历
        tmp = tmp->next;
        if(tmp->next == NULL) {//下一个节点为空，说明到尾了
            break;
        }
    }
    //双链表插入
    // tmp->next = t;
    // t->prev = tmp;
    // t->next = tmp->next;//自己指自己了,艹
    t->next = tmp->next;
    t->prev = tmp;
    tmp->next = t;
    if(t->next == NULL)//说明插入的是尾部
        tail = t;
    else
        t->next->prev = t;
    delete tmp2;
}

void timer_list::del_timer(timer *t, bool dele) { //删除定时器
    if(!t || !head) return;
    if(t == head && t == tail) {//唯一节点
        if(dele) delete t;
        head = NULL;
        tail = NULL;
        return;
    }
    if(t == head) { //删除头部
        head = t->next;
        head->prev = NULL;
        if(dele) delete t;
        return;
    }
    if(t == tail) { //删除尾部
        tail = t->prev;
        tail->next = NULL;
        if(dele) delete t;
        return;
    }
    t->prev->next = t->next;
    t->next->prev = t->prev;
    if(dele) delete t;
}

void timer_list::adjust_timer(timer *t) { //定时器发生改变，重新对该定时器排序,删掉再加入就好
    del_timer(t, false);
    add_timer(t);
}

void timer_list::tick() {
    if (!head) return;
    time_t cur = time(NULL);
    timer *tmp = head;
    while (tmp) {//从头依次检查
        if (cur < tmp->expire)
            break;
        LOG_WRITE("time out, client: " + to_string(tmp->sockfd) + " disconnect, usernum: " + to_string(http::user_num));
        removefd(timer_util::epollfd, tmp->sockfd);
        head = head->next;
        if (head)
            head->prev = NULL;
        delete tmp;
        tmp = head;//这步让tmp->next指向了指向自身
    }
}

//工具类实现
void timer_util::sig_handler(int sig) {//信号处理
    int save_errno = errno;//保证本函数中间如果产生errno也不会导致程序某错判断出错
    int msg = sig;
    //将信号值从管道写端写入，传输char
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void timer_util::add_sig(int sig, void(handler)(int), bool restart) {
    //创建sigaction结构体变量
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));

    //信号处理函数中仅仅发送信号值，不做对应逻辑处理
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    //将所有信号添加到信号集中，即函数期间屏蔽所有信号
    sigfillset(&sa.sa_mask);

    //执行sigaction函数
    assert(sigaction(sig, &sa, NULL) != -1);
}

void timer_util::timer_handler(bool isout) {
    if(!isout) return;
    timerlst.tick();
    alarm(TIMEOUT);
}