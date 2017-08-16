//
// Created by xingzhao on 2017/8/16.
//

#ifndef PROJECT_TIMERMANAGER_H
#define PROJECT_TIMERMANAGER_H
#include <vector>
#include "Timer.h"
#include "time.h"
#include "sys/time.h"

class TimerManager
{
    public:
    static unsigned long long GetCurrentMillisecs();
    void DetectTimers();
    private:
    friend class Timer;
    void AddTimer(Timer* timer);
    void RemoveTimer(Timer* timer);

    void UpHeap(size_t index);
    void DownHeap(size_t index);
    void SwapHeap(size_t, size_t index2);

    private:
    struct HeapEntry
    {
        unsigned long long time;
        Timer* timer;
    };
    std::vector<HeapEntry> heap_;
};

#endif //PROJECT_TIMERMANAGER_H
