//
// Created by xingzhao on 2017/8/16.
//

#ifndef PROJECT_TIMER_H
#define PROJECT_TIMER_H

#include <vector>
#include <stddef.h>
class TimerManager;

class Timer
{
public:
    enum TimerType { ONCE, CIRCLE };

    Timer(TimerManager& manager);
    ~Timer();

    //template<typename Fun>
    void *pInstance;
    void (*_callback)(void *pInstance);
//    timer->pInstance = (void *)this;
//    timer->_callback = &CLASSS::static_func;
    void Start(unsigned int interval, Timer::TimerType timeType);
    void Stop();

private:
    void OnTimer(unsigned long long now);

private:
    friend class TimerManager;
    TimerManager& manager_;
    TimerType timerType_;
    //boost::function<void(void)> timerFun_;
    unsigned interval_;
    unsigned long long expires_;

    size_t heapIndex_;
};

#endif //PROJECT_TIMER_H
