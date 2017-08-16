//
// Created by xingzhao on 2017/8/16.
//

#include "Timer.h"

// cpp file //////////////////////////////////////////////////////////////////
#define _CRT_SECURE_NO_WARNINGS
//#include "config.h"
#include "Timer.h"
#include "TimerManager.h"
#ifdef _MSC_VER
# include <sys/timeb.h>
#else
# include <sys/time.h>
#endif



//////////////////////////////////////////////////////////////////////////
// Timer

Timer::Timer(TimerManager& manager)
        : manager_(manager)
        , heapIndex_(-1)
{
    pInstance = NULL;
}


Timer::~Timer()
{
    Stop();
}

void Timer::Start(unsigned int interval, Timer::TimerType timeType)
{
    Stop();
    interval_ = interval;
    timerType_ = timeType;
    expires_ = interval + TimerManager::GetCurrentMillisecs();
    manager_.AddTimer(this);
}

void Timer::Stop()
{
    if (heapIndex_ != -1)
    {
        manager_.RemoveTimer(this);
        heapIndex_ = -1;
    }
}

void Timer::OnTimer(unsigned long long now)
{
    if (timerType_ == Timer::CIRCLE)
    {
        expires_ = interval_ + now;
        manager_.AddTimer(this);
    }
    else
    {
        heapIndex_ = -1;
    }
    _callback(pInstance);
}