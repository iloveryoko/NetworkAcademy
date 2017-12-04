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

#ifndef A_LOOPER_H_

#define A_LOOPER_H_

#ifdef __APPLE__
#include <string>
#include <sstream>
#include <iostream>
#endif

#include "ABase.h"
#include <mutex>
#include <list>
#include <thread>
#include <condition_variable>


class AHandler;

class AMessage;

class ALooper : public std::enable_shared_from_this<ALooper> {
public:
    typedef int32_t event_id;
    typedef int32_t handler_id;

    ALooper();

    // Takes effect in a subsequent call to start().
    void setName(const char *name);

    handler_id registerHandler(const std::shared_ptr<AHandler> &handler);

    void unregisterHandler(handler_id handlerID);

    status_t start(bool runOnCallingThread = false);

    status_t stop();

    static int64_t GetNowUs();

    int getEventQueueSize() {
        std::unique_lock<std::mutex> lck(mLock);
        return mEventQueue.size();
    };

    virtual ~ALooper();

private:
    friend class ALooperRoster;

    struct Event {
        int64_t mWhenUs;
        std::shared_ptr<AMessage> mMessage;
    };

    std::mutex mLock;
    std::condition_variable mQueueChangedCondition;

    bool is_run_;
    bool is_stoped_;

    std::string mName;

    std::list<Event> mEventQueue;

    std::thread *mThread;

    void post(const std::shared_ptr<AMessage> &msg, int64_t delayUs);

    bool loop();

    void run();

    DISALLOW_EVIL_CONSTRUCTORS(ALooper);
};


#endif  // A_LOOPER_H_
