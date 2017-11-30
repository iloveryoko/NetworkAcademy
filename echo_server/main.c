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
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include "thread_pool.h"
#include <getopt.h>
//#include "thread_pool.c"
#define MAX_EVENT_NUMBER  1000
#define SIZE    1024
#define MAX_CLIENT_NUMBER     1000
#define LOOP_COUNT 10000
int client_send_count = 0;
thpool_t  *thpool ;  //线程池

//从主线程向工作线程数据结构
struct thread_args
{
    int epollfd;
    int sockfd ;
    char *ip;
    int port;
    int data;
};

//用户说明
struct user
{
    int  sockfd ;   //文件描述符
    char client_buf [SIZE]; //数据的缓冲区
};
struct user user_client[MAX_CLIENT_NUMBER];  //定义一个全局的客户数据表

int64_t getSystemTimeMs() {
//    struct timeval t;
//    t.tv_sec = t.tv_usec = 0;
//    gettimeofday(&t, NULL);
//    return int64_t(t.tv_sec) * 1000LL + int64_t(t.tv_usec/1000L);

    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME,&ts) == 0){
        return ts.tv_sec * 1000LL + (ts.tv_nsec/1000000LL);
    }
    return 0;
}

//由于epoll设置的EPOLLONESHOT模式，当出现errno =EAGAIN,就需要重新设置文件描述符（可读）
int reset_epoll_in_oneshot(int epollfd, int fd)
{
    printf("FD [%d] reset_epoll_in_oneshot()\n", fd);
    struct epoll_event event ;
    event.data.fd = fd ;
    event.events = EPOLLIN|EPOLLET|EPOLLONESHOT ;
    int ret = epoll_ctl (epollfd , EPOLL_CTL_MOD, fd , &event);
    if (ret != 0) {
        char err_msg[256] = {0};
        sprintf(err_msg, "FD [%d] reset_epoll_in_oneshot():epoll_ctl() perror", fd);
        perror(err_msg);
    }
    return ret;
}
//向epoll内核事件表里面添加可写的事件
int add_epoll_out_fd(int epollfd, int fd, int oneshot)
{
    printf("FD [%d] add_epoll_out_fd(%d)\n", fd, oneshot);
    struct epoll_event  event ;
    event.data.fd = fd ;
    event.events |= ~ EPOLLIN ;
    event.events |= EPOLLOUT ;
    event.events |= EPOLLET;
    if (oneshot)
    {
        event.events |= EPOLLONESHOT ; //设置EPOLLONESHOT

    }
    int ret = epoll_ctl (epollfd , EPOLL_CTL_ADD ,fd , &event);
    if (ret != 0) {
        char err_msg[256] = {0};
        sprintf(err_msg, "FD [%d] add_epoll_out_fd():epoll_ctl() perror", fd);
        perror(err_msg);
    }
    return  ret;
}
//群聊函数
int groupchat (int epollfd , int sockfd , char *buf)
{

    int i = 0 ;
    for ( i  = 0 ; i < MAX_CLIENT_NUMBER ; i++)
    {
        if (user_client[i].sockfd == sockfd)
        {
            continue ;
        }
        strncpy (user_client[i].client_buf ,buf , strlen (buf)) ;
        add_epoll_out_fd(epollfd, user_client[i].sockfd, 1);

    }

}

int client_epoll_in_thread_func(void *args) {
    int sockfd = ((struct thread_args*)args)->sockfd ;
    int epollfd =((struct thread_args*)args)->epollfd;
    char buf[SIZE];
    memset (buf , '\0', SIZE);

    printf ("Thread handling data on fd [%d]\n", sockfd);

    //由于我将epoll的工作模式设置为ET模式，所以就要用一个循环来读取数据，防止数据没有读完，而丢失。
    while (1)
    {
        int ret = recv (sockfd ,buf , SIZE-1 , 0);
        if (ret == 0)
        {
            char err_msg[256] = {0};
            sprintf(err_msg, "FD [%d] server_epoll_in_thread_func():recv()", sockfd);
            perror(err_msg);
            printf("FD [%d] Peer closed, close fd\n", sockfd);
            close(sockfd);
            break;
        }
        else if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                reset_epoll_in_oneshot(epollfd, sockfd);  //重新设置（上面已经解释了）
                break;
            }
        }
        else
        {
            printf ("FD [%d] read data: [%s]\n", sockfd, buf);
            //sleep (5);

            close(sockfd);
            printf ("FD [%d] closed\n", sockfd);
            break;
            //strncpy (user_client[sockfd].client_buf , buf , strlen (buf)) ;
            //printf("Server set epoll out on fd [%d]\n", user_client[sockfd].sockfd);
            //reset_read_oneshot (epollfd , user_client[sockfd].sockfd );
            //groupchat (epollfd , sockfd, buf );
        }


    }
    printf ("End thread receive data on fd :[%d]\n", sockfd);
}

//接受数据的函数，也就是线程的回调函数
int server_epoll_in_thread_func(void *args)
{
    int sockfd = ((struct thread_args*)args)->sockfd ;
    int epollfd =((struct thread_args*)args)->epollfd;
    char buf[SIZE];
    memset (buf , '\0', SIZE);

    printf ("Thread handling data on fd [%d]\n", sockfd);

    //由于我将epoll的工作模式设置为ET模式，所以就要用一个循环来读取数据，防止数据没有读完，而丢失。
    while (1)
    {
        int ret = recv (sockfd ,buf , SIZE-1 , 0);
        if (ret == 0)
        {
            char err_msg[256] = {0};
            sprintf(err_msg, "FD [%d] server_epoll_in_thread_func():recv()", sockfd);
            perror(err_msg);
            printf("FD [%d] Peer closed, close fd\n", sockfd);
            close(sockfd);
            break;
        }
        else if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                reset_epoll_in_oneshot(epollfd, sockfd);  //重新设置（上面已经解释了）
                break;
            }
        }
        else
        {
            printf ("FD [%d] read data: [%s]\n", sockfd, buf);
            //sleep (5);
            strncpy (user_client[sockfd].client_buf , buf , strlen (buf)) ;
            mod_fd_epoll_in2out (epollfd , user_client[sockfd].sockfd );
            break;
            //groupchat (epollfd , sockfd, buf );
        }
    }
    printf ("End thread receive  data on fd [%d]\n", sockfd);

}
//这是重新注册，将文件描述符从可写变成可读
int mod_fd_epoll_out2in(int epollfd, int fd)
{
    printf("FD [%d] mod_fd_epoll_out2in\n", fd);
    struct epoll_event event;
    event.data.fd = fd ;
    event.events  |= ~EPOLLOUT ;
    event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
    int ret = epoll_ctl (epollfd , EPOLL_CTL_MOD , fd , &event);
    if (ret != 0) {
        char err_msg[256] = {0};
        sprintf(err_msg, "FD [%d] mod_fd_epoll_out2in():epoll_ctl()", fd);
        perror(err_msg);
    }
    return ret;
}

int mod_fd_epoll_in2out(int epollfd, int fd)
{
    printf("FD [%d] mod_fd_epoll_in2out()\n", fd);
    struct epoll_event event;
    event.data.fd = fd ;
    event.events  |= ~EPOLLIN ;
    event.events = EPOLLOUT|EPOLLET|EPOLLONESHOT;
    int ret = epoll_ctl (epollfd , EPOLL_CTL_MOD , fd , &event);
    if (ret != 0) {
        char err_msg[256] = {0};
        sprintf(err_msg, "FD [%d] mod_fd_epoll_in2out():epoll_ctl()", fd);
        perror(err_msg);
    }
    return  ret;
}

//与前面的解释一样
int reset_epoll_out_oneshot (int epollfd , int sockfd)
{
    struct epoll_event  event;
    event.data.fd = sockfd ;
    event.events = EPOLLOUT |EPOLLET |EPOLLONESHOT ;
    epoll_ctl (epollfd, EPOLL_CTL_MOD , sockfd , &event);
    perror("reset_epoll_out_oneshot");
    return 0 ;

}

//发送读的数据
int client_epoll_out_thread_func(void *args)
{
    int sockfd = ((struct thread_args *)args)->sockfd ;
    int epollfd= ((struct thread_args*)args)->epollfd ;

    int ret = send (sockfd, user_client[sockfd].client_buf , strlen (user_client[sockfd].client_buf), 0); //发送数据
    client_send_count++;
    printf("Send [%d] times\n", client_send_count);
    if (ret == 0 )
    {
        if (errno == EAGAIN)
        {
            reset_epoll_out_oneshot (epollfd , sockfd);
            printf("FD [%d] send later\n", sockfd);
            return -1;
        } else {
            char err_msg[256] = {0};
            sprintf(err_msg, "FD [%d] send()", sockfd);
            perror(err_msg);
            close (sockfd);
            return -1 ;
        }
    } else if (ret > 0) {
        printf("FD [%d] sent [%d] bytes: [%s]\n", sockfd, ret, user_client[sockfd].client_buf);
        memset (&user_client[sockfd].client_buf , '\0', sizeof (user_client[sockfd].client_buf));
        mod_fd_epoll_out2in(epollfd, sockfd);//重新设置文件描述符
    }
    return 0;
}

int server_epoll_out_thread_func(void *args)
{
    int sockfd = ((struct thread_args *)args)->sockfd ;
    int epollfd= ((struct thread_args*)args)->epollfd ;

    int ret = send (sockfd, user_client[sockfd].client_buf , strlen (user_client[sockfd].client_buf), 0); //发送数据
    if (ret == 0 )
    {
        if (errno == EAGAIN)
        {
            reset_epoll_out_oneshot (epollfd , sockfd);
            printf("FD [%d] send later\n", sockfd);
            return -1;
        } else {
            char err_msg[256] = {0};
            sprintf(err_msg, "FD [%d] send()", sockfd);
            perror(err_msg);
            close (sockfd);
            return -1 ;
        }
    } else if (ret > 0) {
        printf("FD [%d] sent [%d] bytes\n", sockfd, ret);
        printf("FD [%d] epoll out to in\n", sockfd);
        memset (&user_client[sockfd].client_buf , '\0', sizeof (user_client[sockfd].client_buf));
        //mod_fd_epoll_out2in(epollfd, sockfd);//重新设置文件描述符
    }
    return 0;
}
//套接字设置为非阻塞
int setnoblocking (int fd)
{
    int old_option = fcntl (fd, F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl (fd , F_SETFL , new_option);
    return old_option ;
}

int add_epoll_in_fd(int epollfd, int fd, int oneshot)
{
    printf("FD [%d] add_epoll_in_fd(%d)\n", fd, oneshot);
    struct epoll_event  event;
    event.data.fd = fd ;
    event.events = EPOLLIN|EPOLLET ;
    if (oneshot)
    {
        event.events |= EPOLLONESHOT ;

    }
    int ret = epoll_ctl (epollfd , EPOLL_CTL_ADD ,fd ,  &event);
    if (ret != 0) {
        char err_msg[256] = {0};
        sprintf(err_msg, "FD [%d] add_epoll_in_fd():epoll_ctl() perror", fd);
        perror(err_msg);
    }
    setnoblocking (fd);
    return 0 ;
}

int connect_thread_func (void *args) {
    struct sockaddr_in address;
    memset (&address , 0 , sizeof (address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(((struct thread_args *) args)->ip);
    address.sin_port =htons( ((struct thread_args *) args)->port) ;
    int sock_fd = ((struct thread_args *) args)->sockfd;
    int epollfd = ((struct thread_args *) args)->epollfd;

    int ret = 0;
    while ((ret = connect(sock_fd, (struct sockaddr*)&address, sizeof(address))) != 0 ){
        if (errno == EINPROGRESS) {
            //usleep(50);
            //continue;
            char err_msg[256] = {0};
            sprintf(err_msg, "FD [%d] client_thread_func():connect() perror", sock_fd);
            perror(err_msg);
            break;
        } else {
            char err_msg[256] = {0};
            sprintf(err_msg, "FD [%d] client_thread_func():connect() perror", sock_fd);
            perror(err_msg);
            break;
        }
        //continue;
    }
    //if (ret == 0) {
    printf("FD [%d] Connect success\n", sock_fd);
    sprintf(user_client[sock_fd].client_buf, "%06d", ((struct thread_args *) args)->data);
    ret = add_epoll_out_fd(epollfd, sock_fd, 1);
    //}

}


//接受数据的函数，也就是线程的回调函数
int client_thread_func (void *args) {
    struct sockaddr_in address;
    //int sockfd = ((struct thread_args *) args)->sockfd;
    //int epollfd = ((struct thread_args *) args)->epollfd;
    char buf[SIZE];
    memset(buf, '\0', SIZE);

    for (int i = 0; i < LOOP_COUNT; i ++) {
        int sock_fd = 0;
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            printf("socket create error\n");
        }

        //add_epoll_in_fd(epollfd, sock_fd, 0);

        if (sock_fd >= MAX_CLIENT_NUMBER) {
            printf("Client reach limit, hold on\n");
            close(sock_fd);
            usleep(1000);
            i--;
            continue;
        }
        printf("FD [%d] socket created, add to thread pool\n", sock_fd);

//        memset (&address , 0 , sizeof (address));
//        address.sin_family = AF_INET;
//        address.sin_addr.s_addr = inet_addr(((struct thread_args *) args)->ip);
//        address.sin_port =htons( ((struct thread_args *) args)->port) ;

        int epollfd = ((struct thread_args *) args)->epollfd;
        struct thread_args  *fds_for_new_worker ;
        fds_for_new_worker = (struct thread_args *) calloc(1, sizeof(struct thread_args));
        fds_for_new_worker->epollfd = ((struct thread_args *) args)->epollfd ;
        fds_for_new_worker->sockfd = sock_fd ;
        fds_for_new_worker->ip = ((struct thread_args *) args)->ip ;
        fds_for_new_worker->port = ((struct thread_args *) args)->port ;
        fds_for_new_worker->data = i;

        thpool_add_work(thpool, (void *) connect_thread_func, fds_for_new_worker);//将任务添加到工作队列中

        //把socket和socket地址结构联系起来
//        int ret = 0;
//        while ((ret = connect(sock_fd, (struct sockaddr*)&address, sizeof(address))) != 0 ){
//            if (errno == EINPROGRESS) {
//                //usleep(50);
//                //continue;
//                char err_msg[256] = {0};
//                sprintf(err_msg, "FD [%d] client_thread_func():connect() perror", sock_fd);
//                perror(err_msg);
//                break;
//            } else {
//                char err_msg[256] = {0};
//                sprintf(err_msg, "FD [%d] client_thread_func():connect() perror", sock_fd);
//                perror(err_msg);
//                break;
//            }
//            //continue;
//        }
//        //if (ret == 0) {
//            printf("FD [%d] Connect success\n", sock_fd);
//            strcpy(user_client[sock_fd].client_buf, "123456");
//            ret = add_epoll_out_fd(epollfd, sock_fd, 1);
//        //}
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in address ;
    const char *ip = "127.0.0.1";
    int port  = 7777 ;
    int ch;

    int server_client = 1;
    char *arg_ip = NULL;
    char *arg_port = NULL;
    char short_opts[] = "sc";
    char *long_opt_arg = NULL;
    struct option long_opts[] = {
            {"server", no_argument, &server_client, 1},    // getopt_long返回值为0，server保存为1
            {"client", no_argument, &server_client, 2},    // getopt_long返回值为0
            {"host", required_argument, 0, 'h'},  // getopt_long返回值为0,必须有参数
            {"port", required_argument, 0, 'p'},
            {NULL, 0, NULL, 0}
    };

    while((ch = getopt_long (argc, argv, short_opts, long_opts, NULL)) != -1)
    {
        switch (ch)
        {
            case 0:
                break;
            case 'h':
                arg_ip = strdup(optarg);
                break;
            case 'p':
                arg_port = strdup(optarg);
                break;
            case '?':
                printf("Unknown option: %c\n",(char)optopt);
                break;
        }
    }

    if (arg_ip != NULL) {
        ip = arg_ip;
    }

    if (arg_port != NULL) {
        int arg_port_int = atoi(arg_port);
        if (arg_port_int > 0) {
            port = arg_port_int;
        }
    }

    int sock_fd = 0;
    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create (5); //创建内核事件描述符表
    assert (epollfd != -1);
    thpool = thpool_init (5) ; //线程池的一个初始化

    if (server_client == 1) {
        // create server socket
        // create epoll fd
        // add socket to epoll
        // epoll server wait

        memset (&address , 0 , sizeof (address));
        address.sin_family = AF_INET ;
        inet_pton (AF_INET ,ip , &address.sin_addr);
        address.sin_port = htons( port) ;


        sock_fd = socket (AF_INET, SOCK_STREAM, 0);
        assert (listen >=0);
        int reuse = 1;
        setsockopt (sock_fd , SOL_SOCKET , SO_REUSEADDR , &reuse , sizeof (reuse)); //端口重用，因为出现过端口无法绑定的错误
        int ret = bind (sock_fd, (struct sockaddr*)&address , sizeof (address));
        assert (ret >=0 );

        ret = listen (sock_fd , 5);
        assert (ret >=0);
        add_epoll_in_fd(epollfd, sock_fd, 0);

        while (1)
        {
            int ret = epoll_wait (epollfd, events, MAX_EVENT_NUMBER , -1);//等待就绪的文件描述符，这个函数会将就绪的复制到events的结构体数组中。
            if (ret < 0)
            {
                if (errno != EINTR) {
                    printf ("poll failure\n");
                    break ;
                }
            } else {
                printf("poll [%d] events\n", ret);
            }
            int i =0  ;
            for ( i = 0 ; i < ret ; i++ )
            {
                int sockfd = events[i].data.fd ;
                if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                    printf("FD [%d] EPOLL hup/error\n", sockfd);
                } else if (sockfd == sock_fd) {
                    struct sockaddr_in client_address ;
                    socklen_t  client_length = sizeof (client_address);
                    int connfd = accept (sock_fd , (struct sockaddr*)&client_address,&client_length);
                    user_client[connfd].sockfd = connfd ;
                    //memset (&user_client[connfd].client_buf , '\0', sizeof (user_client[connfd].client_buf));
                    add_epoll_in_fd(epollfd, connfd, 1);//将新的套接字加入到内核事件表里面。
                    printf ("FD [%d] New Connection: FD [%d]\n", sockfd, connfd);
                }
                else if (events[i].events & EPOLLIN)
                {
                    printf("FD [%d] EPOLLIN\n", sockfd);
                    struct thread_args  *fds_for_new_worker ;
                    fds_for_new_worker = (struct thread_args *) calloc(1, sizeof(struct thread_args));
                    fds_for_new_worker->epollfd = epollfd ;
                    fds_for_new_worker->sockfd = sockfd ;

                    thpool_add_work(thpool, (void *) server_epoll_in_thread_func, fds_for_new_worker);//将任务添加到工作队列中
                }else if (events[i].events & EPOLLOUT)
                {
                    printf("FD [%d] EPOLLOUT\n", sockfd);
                    struct thread_args  *fds_for_new_worker ;
                    fds_for_new_worker = (struct thread_args *) calloc(1, sizeof(struct thread_args));
                    fds_for_new_worker->epollfd = epollfd ;
                    fds_for_new_worker->sockfd = sockfd ;

                    thpool_add_work(thpool, (void *) server_epoll_out_thread_func, fds_for_new_worker);//将任务添加到工作队列中
                }

            }

        }
    } else if (server_client == 2) {
        // for loop {
        //     add task to thread pool {
        //         create client socket
        //         create epoll fd
        //         add socket to epoll
        //         socket connect. send.
        //     }
        //
        // }
        // epoll server
        //
        struct thread_args  fds_client_thread ;
        fds_client_thread.epollfd = epollfd ;
        fds_client_thread.sockfd = 0 ;
        fds_client_thread.ip = ip;
        fds_client_thread.port = port;
        pthread_t client_thread;
        int64_t start_time = getSystemTimeMs();
        pthread_create (&client_thread , NULL , (void *) client_thread_func , (void *)&fds_client_thread);

        while (1)
        {

            int ret = epoll_wait (epollfd, events, MAX_EVENT_NUMBER , -1);//等待就绪的文件描述符，这个函数会将就绪的复制到events的结构体数组中。
            if (ret < 0)
            {
                if (errno != EINTR) {
                    printf ("poll failure\n");
                    break ;
                }
            } else {
                printf("poll [%d] events\n", ret);
            }
            int i =0  ;
            for ( i = 0 ; i < ret ; i++ )
            {
                int sockfd = events[i].data.fd ;
                if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                    printf("FD [%d] EPOLL hup/error\n", sockfd);
                } else if (sockfd == -1) {
                    struct sockaddr_in client_address ;
                    socklen_t  client_length = sizeof (client_address);
                    int connfd = accept (sock_fd , (struct sockaddr*)&client_address,&client_length);
                    user_client[connfd].sockfd = connfd ;
                    memset (&user_client[connfd].client_buf , '\0', sizeof (user_client[connfd].client_buf));
                    add_epoll_in_fd(epollfd, connfd, 1);//将新的套接字加入到内核事件表里面。
                    printf ("FD [%d] New Connection: FD [%d]\n", sockfd, connfd);
                } else if (events[i].events & EPOLLIN) {
                    printf("FD [%d] EPOLLIN\n", sockfd);
                    struct thread_args  *fds_for_new_worker ;
                    fds_for_new_worker = (struct thread_args *) calloc(1, sizeof(struct thread_args));
                    fds_for_new_worker->epollfd = epollfd ;
                    fds_for_new_worker->sockfd = sockfd ;

                    thpool_add_work(thpool, (void *) client_epoll_in_thread_func, fds_for_new_worker);//将任务添加到工作队列中
                } else if (events[i].events & EPOLLOUT) {
                    printf("FD [%d] EPOLLOUT\n", sockfd);
                    struct  thread_args  *fds_for_new_worker ;
                    fds_for_new_worker = (struct thread_args *) calloc(1, sizeof(struct thread_args));
                    fds_for_new_worker->epollfd = epollfd ;
                    fds_for_new_worker->sockfd = sockfd ;
                    thpool_add_work(thpool, (void *) client_epoll_out_thread_func, fds_for_new_worker);//将任务添加到工作队列中
                }

            }
            if (client_send_count == LOOP_COUNT) {
                break;
            }
        }
        int64_t end_time = getSystemTimeMs();
        printf("Client receive [%d] times in [%lld] ms\n", client_send_count, end_time - start_time);
    }

    thpool_destory (thpool);
    close (sock_fd);
    return EXIT_SUCCESS;
}