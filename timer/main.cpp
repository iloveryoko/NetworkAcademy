#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>
//#include "thread_pool.c"
#include <iostream>
#include <thread>
#include "Timer.h"
#include "TimerManager.h"

using namespace std;

TimerManager g_tm;
void TimerHandler(void *data)
{
    timeval tv;
    ::gettimeofday(&tv, 0);
    std::cout << "TimerHandler " << tv.tv_sec << std::endl;
}

void TimerThreadFunc() {
    while (true)
    {
        g_tm.DetectTimers();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


int main(int argc, char *argv[])
{

    Timer t(g_tm);
    t.pInstance = NULL;
    t._callback = &TimerHandler;
    t.Start(1000,  Timer::TimerType::CIRCLE);
    std::thread *timer_thread = new std::thread([=] {TimerThreadFunc();});

    if (timer_thread->joinable()) {
        timer_thread->join();
    }
    delete timer_thread;
    std::cin.get();
    return 0;
}