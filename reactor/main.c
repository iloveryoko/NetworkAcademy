#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include "thread_pool.h"
//#include "thread_pool.c"
#define MAX_EVENT_NUMBER  1000
#define SIZE    1024
#define MAX     10

//从主线程向工作线程数据结构
struct fd
{
    int epollfd;
    int sockfd ;
};

//用户说明
struct user
{
    int  sockfd ;   //文件描述符
    char client_buf [SIZE]; //数据的缓冲区
};
struct user user_client[MAX];  //定义一个全局的客户数据表


//由于epoll设置的EPOLLONESHOT模式，当出现errno =EAGAIN,就需要重新设置文件描述符（可读）
void reset_oneshot (int epollfd , int fd)
{
    struct epoll_event event ;
    event.data.fd = fd ;
    event.events = EPOLLIN|EPOLLET|EPOLLONESHOT ;
    epoll_ctl (epollfd , EPOLL_CTL_MOD, fd , &event);

}
//向epoll内核事件表里面添加可写的事件
int addreadfd (int epollfd , int fd , int oneshot)
{
    struct epoll_event  event ;
    event.data.fd = fd ;
    event.events |= ~ EPOLLIN ;
    event.events |= EPOLLOUT ;
    event.events |= EPOLLET;
    if (oneshot)
    {
        event.events |= EPOLLONESHOT ; //设置EPOLLONESHOT

    }
    epoll_ctl (epollfd , EPOLL_CTL_MOD ,fd , &event);

}
//群聊函数
int groupchat (int epollfd , int sockfd , char *buf)
{

    int i = 0 ;
    for ( i  = 0 ; i < MAX ; i++)
    {
        if (user_client[i].sockfd == sockfd)
        {
            continue ;
        }
        strncpy (user_client[i].client_buf ,buf , strlen (buf)) ;
        addreadfd (epollfd , user_client[i].sockfd , 1);

    }

}
//接受数据的函数，也就是线程的回调函数
int funcation (void *args)
{
    int sockfd = ((struct fd*)args)->sockfd ;
    int epollfd =((struct fd*)args)->epollfd;
    char buf[SIZE];
    memset (buf , '\0', SIZE);

    printf ("Thread handling data on fd :%d\n", sockfd);

    //由于我将epoll的工作模式设置为ET模式，所以就要用一个循环来读取数据，防止数据没有读完，而丢失。
    while (1)
    {
        int ret = recv (sockfd ,buf , SIZE-1 , 0);
        if (ret == 0)
        {
            close (sockfd);
            break;
        }
        else if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                reset_oneshot (epollfd, sockfd);  //重新设置（上面已经解释了）
                break;
            }
        }
        else
        {
            printf ("Read data is %s\n", buf);
            //sleep (5);
            groupchat (epollfd , sockfd, buf );
        }


    }
    printf ("End thread receive  data on fd : %d\n", sockfd);

}
//这是重新注册，将文件描述符从可写变成可读
int addagainfd (int epollfd , int fd)
{
    struct epoll_event event;
    event.data.fd = fd ;
    event.events  |= ~EPOLLOUT ;
    event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
    epoll_ctl (epollfd , EPOLL_CTL_MOD , fd , &event);
}
//与前面的解释一样
int reset_read_oneshot (int epollfd , int sockfd)
{
    struct epoll_event  event;
    event.data.fd = sockfd ;
    event.events = EPOLLOUT |EPOLLET |EPOLLONESHOT ;
    epoll_ctl (epollfd, EPOLL_CTL_MOD , sockfd , &event);
    return 0 ;

}

//发送读的数据
int readfun (void *args)
{
    int sockfd = ((struct fd *)args)->sockfd ;
    int epollfd= ((struct fd*)args)->epollfd ;

    int ret = send (sockfd, user_client[sockfd].client_buf , strlen (user_client[sockfd].client_buf), 0); //发送数据
    if (ret == 0 )
    {

        close (sockfd);
        printf ("发送数据失败\n");
        return -1 ;
    }
    else if (ret == EAGAIN)
    {
        reset_read_oneshot (epollfd , sockfd);
        printf("send later\n");
        return -1;
    }
    memset (&user_client[sockfd].client_buf , '\0', sizeof (user_client[sockfd].client_buf));
    addagainfd (epollfd , sockfd);//重新设置文件描述符

}
//套接字设置为非阻塞
int setnoblocking (int fd)
{
    int old_option = fcntl (fd, F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl (fd , F_SETFL , new_option);
    return old_option ;
}

int addfd (int epollfd , int fd , int oneshot)
{
    struct epoll_event  event;
    event.data.fd = fd ;
    event.events = EPOLLIN|EPOLLET ;
    if (oneshot)
    {
        event.events |= EPOLLONESHOT ;

    }
    epoll_ctl (epollfd , EPOLL_CTL_ADD ,fd ,  &event);
    setnoblocking (fd);
    return 0 ;
}



int main(int argc, char *argv[])
{
    struct sockaddr_in  address ;
    const char *ip = "127.0.0.1";
    int port  = 7777 ;

    memset (&address , 0 , sizeof (address));
    address.sin_family = AF_INET ;
    inet_pton (AF_INET ,ip , &address.sin_addr);
    address.sin_port =htons( port) ;


    int listenfd = socket (AF_INET, SOCK_STREAM, 0);
    assert (listen >=0);
    int reuse = 1;
    setsockopt (listenfd , SOL_SOCKET , SO_REUSEADDR , &reuse , sizeof (reuse)); //端口重用，因为出现过端口无法绑定的错误
    int ret = bind (listenfd, (struct sockaddr*)&address , sizeof (address));
    assert (ret >=0 );

    ret = listen (listenfd , 5);
    assert (ret >=0);


    struct epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create (5); //创建内核事件描述符表
    assert (epollfd != -1);
    addfd (epollfd , listenfd, 0);

    thpool_t  *thpool ;  //线程池
    thpool = thpool_init (5) ; //线程池的一个初始化

    while (1)
    {
        int ret = epoll_wait (epollfd, events, MAX_EVENT_NUMBER , -1);//等待就绪的文件描述符，这个函数会将就绪的复制到events的结构体数组中。
        if (ret < 0)
        {
            printf ("poll failure\n");
            break ;
        }
        int i =0  ;
        for ( i = 0 ; i < ret ; i++ )
        {
            int sockfd = events[i].data.fd ;

            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address ;
                socklen_t  client_length = sizeof (client_address);
                int connfd = accept (listenfd , (struct sockaddr*)&client_address,&client_length);
                user_client[connfd].sockfd = connfd ;
                memset (&user_client[connfd].client_buf , '\0', sizeof (user_client[connfd].client_buf));
                addfd (epollfd , connfd , 1);//将新的套接字加入到内核事件表里面。
                printf ("New Connection\n");
            }
            else if (events[i].events & EPOLLIN)
            {
                printf("EPOLLIN\n");
                struct fd    fds_for_new_worker ;
                fds_for_new_worker.epollfd = epollfd ;
                fds_for_new_worker.sockfd = sockfd ;

                thpool_add_work (thpool, (void*)funcation ,&fds_for_new_worker);//将任务添加到工作队列中
            }else if (events[i].events & EPOLLOUT)
            {
                printf("EPOLLOUT\n");
                struct  fd   fds_for_new_worker ;
                fds_for_new_worker.epollfd = epollfd ;
                fds_for_new_worker.sockfd = sockfd ;
                thpool_add_work (thpool, (void*)readfun , &fds_for_new_worker );//将任务添加到工作队列中
            }

        }

    }

    thpool_destory (thpool);
    close (listenfd);
    return EXIT_SUCCESS;
}