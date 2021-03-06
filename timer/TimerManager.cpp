//
// Created by xingzhao on 2017/8/16.
//

#include "TimerManager.h"
//////////////////////////////////////////////////////////////////////////
// TimerManager
#include "stdio.h"

void TimerManager::AddTimer(Timer* timer)
{
    printf("AddTimer: [%x], interval_: [%d], expires_[%llu]\n", timer, timer->interval_, timer->expires_ / 1000);
    timer->heapIndex_ = heap_.size();
    HeapEntry *entry = new HeapEntry();
    entry->time = timer->expires_;
    entry->timer = timer;
    heap_.push_back(entry);
    UpHeap(heap_.size() - 1);
}

void TimerManager::RemoveTimer(Timer* timer)
{
    size_t index = timer->heapIndex_;
    if (!heap_.empty() && index < heap_.size())
    {
        if (index == heap_.size() - 1)
        {
            HeapEntry *entry = heap_.back();
            heap_.pop_back();
            delete entry;
        }
        else
        {
            SwapHeap(index, heap_.size() - 1);
            heap_.pop_back();
            size_t parent = (index - 1) / 2;
            if (index > 0 && heap_[index]->time < heap_[parent]->time)
                UpHeap(index);
            else
                DownHeap(index);
        }
    }
}

void TimerManager::DetectTimers()
{
    unsigned long long now = GetCurrentMillisecs();

    while (!heap_.empty() && heap_[0]->time <= now)
    {
        Timer* timer = heap_[0]->timer;
        RemoveTimer(timer);
        timer->OnTimer(now);
    }
}

void TimerManager::UpHeap(size_t index)
{
    printf("UpHeap index [%d]\n", index);
    size_t parent = 0;
    if (index < 1) {
        parent = 0;
    } else {
        (index - 1) / 2;
    }
    while (index > 0 && heap_[index]->time < heap_[parent]->time)
    {
        SwapHeap(index, parent);
        index = parent;
        parent = (index - 1) / 2;
    }
}

void TimerManager::DownHeap(size_t index)
{
    printf("DownHeap index [%d]\n", index);
    size_t child = index * 2 + 1;
    while (child < heap_.size())
    {
        size_t minChild = (child + 1 == heap_.size() || heap_[child]->time < heap_[child + 1]->time)
                          ? child : child + 1;
        if (heap_[index]->time < heap_[minChild]->time)
            break;
        SwapHeap(index, minChild);
        index = minChild;
        child = index * 2 + 1;
    }
}

void TimerManager::SwapHeap(size_t index1, size_t index2)
{
    HeapEntry *tmp = heap_[index1];
    heap_[index1] = heap_[index2];
    heap_[index2] = tmp;
    heap_[index1]->timer->heapIndex_ = index1;
    heap_[index2]->timer->heapIndex_ = index2;
}


unsigned long long TimerManager::GetCurrentMillisecs()
{
#ifdef _MSC_VER
    _timeb timebuffer;
    _ftime(&timebuffer);
    unsigned long long ret = timebuffer.time;
    ret = ret * 1000 + timebuffer.millitm;
    return ret;
#else
    timeval tv;
    ::gettimeofday(&tv, 0);
    unsigned long long ret = tv.tv_sec;
    return ret * 1000 + tv.tv_usec / 1000;
#endif
}