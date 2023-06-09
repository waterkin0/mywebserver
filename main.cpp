#include <iostream>
#include <functional>
#include "setting/setting.h"
#include "base/webserver.h"


int main(){
    webserver server;
    server.init();
}

//两个问题1.第二次请求会没有返回，自己试，初步看是第二次的读取失败,目前是在*处readall返回了false：忘记resetfd了
//2.recv没有成功，错误88，意思是第一个参数不是建立连接时返回的文件描述符,这里都是0,也就是赋值失败了
    //暂时解决2
//用户端断开连接时可以删除对应套接字和epoll监听
//在recv读取客户端传来的数据时，只会得到第一次write的：其他的也得到了，但输出的时候默认在/0就停了，没输出后续的
//write了两次，本来以为是会在一次读取读取完毕，因为没想到服务器处理那么快，想的是会一起被一次读读到，结果还是用了两次，说明低估了服务器的速度
//很神奇的一件事，就是像上一行write两次只后第三次会阻塞住，可能就是发的太快了，因为当我在每两个write间sleep一点时间，就可以正常一直读取了，说明我又高估了其速度

//postman可以正常请求，浏览器不行，浏览器报文创建失败
//do_request对于浏览器可能会返回NO_RESOURCE或者FILE_REQUEST，当返回前者时，报文创建失败，服务器卡死
//原来是谷歌浏览器会自动请求一个ico文件，即图标，我这里没有就出现了NO_RESOURCE，但为啥会卡死呢

//返回的类型目前只能是text/html，后续应该会根据文件类型进行改变，改Content-Type:%s\r\n","text/html"

//在一个客户端链接后，不能进行新的连接，目前觉得问题是主线程的套接字也设置为oneshot了，还没测试

//在添加定时器后，运行会出现指针重复delete的情况，原因是当断开连接时，http指向的对应指针会delete一次，而链表又会delete一次
//但是情况不是上面的，而是write和read都delete了,原因是他们都adjust了，而偷懒adjust是先减去在加上，减去就会delete了，加也加了个空的
//删除函数加一个参数，决定是不是要delete

//并发出错
//先alive再不alive出错
//链表出现自己指向自己，赋值时出错,双向链表在增加节点时出错

//注意log的路径是相对于运行路径，而不是写在那个文件的路径，淦