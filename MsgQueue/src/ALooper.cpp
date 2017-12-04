/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ALooper"

#ifdef VS_COMPILE
    #include <time.h>
#else
    #include <sys/time.h>
#endif

#include <chrono>

#include "ALooper.h"

#include "AHandler.h"
#include "ALooperRoster.h"
#include "AMessage.h"


ALooperRoster gLooperRoster;

#ifdef VS_COMPILE

#include <Windows.h>
int
gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;

	return (0);
}
#endif


// static
int64_t ALooper::GetNowUs() {
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    return int64_t(t.tv_sec) * 1000000000LL + int64_t(t.tv_usec) * 1000LL;
}

ALooper::ALooper() {
    is_run_ = true;
    is_stoped_ = true;
    mThread = NULL;
}

ALooper::~ALooper() {
    stop();

    // Since this looper is "dead" (or as good as dead by now),
    // have ALooperRoster unregister any handlers still registered for it.
    gLooperRoster.unregisterStaleHandlers();
}

void ALooper::setName(const char *name) {
    mName = name;
}

ALooper::handler_id ALooper::registerHandler(const std::shared_ptr<AHandler> &handler) {
    return gLooperRoster.registerHandler(shared_from_this(), handler);
}

void ALooper::unregisterHandler(handler_id handlerID) {
    gLooperRoster.unregisterHandler(handlerID);
}

status_t ALooper::start(
        bool runOnCallingThread) {
    if (runOnCallingThread) {
        {
            std::lock_guard<std::mutex> lck(mLock);

            if (mThread != NULL) {
                return INVALID_OPERATION;
            }
        }

        run();
        return OK;
    }
    std::lock_guard<std::mutex> lck(mLock);
    if (mThread != NULL) {
        return INVALID_OPERATION;
    }
    mThread = new std::thread([=] { this->run(); });
    return OK;
}

status_t ALooper::stop() {
    if (mThread == NULL) {
        return INVALID_OPERATION;
    }
    is_run_ = false;

    // 监视线程函数真正结束
    while(!is_stoped_) {
        std::unique_lock<std::mutex> lck(mLock);
        mQueueChangedCondition.notify_one();
        mQueueChangedCondition.wait_for(lck, std::chrono::milliseconds(100));
    }

    if (mThread->joinable()) {
        mThread->join();
    }
    delete mThread;
    mThread = NULL;
    return OK;
}

void ALooper::post(const std::shared_ptr<AMessage> &msg, int64_t delayUs) {
    std::unique_lock<std::mutex> lck(mLock);

    int64_t whenUs;
    if (delayUs > 0) {
        whenUs = GetNowUs() + delayUs;
    } else {
        whenUs = GetNowUs();
    }

    std::list<Event>::iterator it = mEventQueue.begin();
    while (it != mEventQueue.end() && (*it).mWhenUs <= whenUs) {
        ++it;
    }

    Event event;
    event.mWhenUs = whenUs;
    event.mMessage = msg;

    if (it == mEventQueue.begin()) {
        mQueueChangedCondition.notify_one();
    }

    mEventQueue.insert(it, event);
}

void ALooper::run() {
    while (is_run_) {
        is_stoped_ = false;

        if (loop() == false) {
            break;
        }
    }

    is_stoped_ = true;
    mQueueChangedCondition.notify_one();
}

bool ALooper::loop() {
    Event event;
    {
        std::unique_lock<std::mutex> lck(mLock);
        if (mThread == NULL) {
            return false;
        }
        if (mEventQueue.empty()) {
            mQueueChangedCondition.wait(lck);
            return true;
        }
        int64_t whenUs = (*mEventQueue.begin()).mWhenUs;
        int64_t nowUs = GetNowUs();

        if (whenUs > nowUs) {

            int64_t delayUs = whenUs - nowUs;
            mQueueChangedCondition.wait_for(lck, std::chrono::microseconds(delayUs));
            return true;
        }

        event = *mEventQueue.begin();
        mEventQueue.erase(mEventQueue.begin());
    }

    gLooperRoster.deliverMessage(event.mMessage);
    // NOTE: It's important to note that at this point our "ALooper" object
    // may no longer exist (its final reference may have gone away while
    // delivering the message). We have made sure, however, that loop()
    // won't be called again
    return true;

}

