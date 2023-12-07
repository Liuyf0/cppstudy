/* 为什么管道写端要非阻塞？
 * send是将信息发送给套接字缓冲区，如果缓冲区满了，则会阻塞
 * 这时候会进一步增加信号处理函数的执行时间，为此，将其修改为非阻塞。
 * 没有对非阻塞返回值处理，如果阻塞是不是意味着这一次定时事件失效了？
 * 是的，但定时事件是非必须立即处理的事件，可以允许这样的情况发生。
 * */
#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "log.h"

//连接资源结构体成员需要用到定时器类
//需要前向声明
class util_timer;

//连接资源
struct client_data
{
    //客户端socket地址
    sockaddr_in address;

    //socket文件描述符
    int sockfd;

    //定时器
    util_timer* timer;
};

//定时器类
class util_timer
{
public:
    util_timer() : prev( NULL ), next( NULL ){}

public:
    //超时时间
    time_t expire; 
    //回调函数
    void (*cb_func)( client_data* );
    //连接资源
    client_data* user_data;
    //前向定时器
    util_timer* prev;
    //后继定时器
    util_timer* next;
};

//定时器容器类
class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();           //常规销毁链表

    void add_timer(util_timer *timer);
    //调整定时器，任务发生变化时，调整定时器在链表中的位置
    void adjust_timer(util_timer *timer);
    //删除定时器
    void del_timer(util_timer *timer);
    void tick();

private:
    //添加定时器，内部调用私有成员add_timer
    void add_timer(util_timer *timer, util_timer *lst_head);

    //头尾结点
    util_timer *head;
    util_timer *tail;
};

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

//定时器回调函数
void cb_func(client_data *user_data);

#endif