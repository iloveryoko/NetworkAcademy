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

#ifndef A_LOOPER_ROSTER_H_

#define A_LOOPER_ROSTER_H_

#include "ALooper.h"
#include <mutex>
#include <map>
#include <condition_variable>


class ALooperRoster {
public:
    ALooperRoster();

    ALooper::handler_id registerHandler(
            const std::shared_ptr<ALooper> &looper, const std::shared_ptr<AHandler> &handler);

    void unregisterHandler(ALooper::handler_id handlerID);

    void unregisterStaleHandlers();

    status_t postMessage(const std::shared_ptr<AMessage> &msg, int64_t delayUs = 0);

    void deliverMessage(const std::shared_ptr<AMessage> &msg);

    status_t postAndAwaitResponse(
            const std::shared_ptr<AMessage> &msg, std::shared_ptr<AMessage> *response);

    void postReply(uint32_t replyID, const std::shared_ptr<AMessage> &reply);

    std::shared_ptr<ALooper> findLooper(ALooper::handler_id handlerID);

private:
    struct HandlerInfo {
        std::weak_ptr<ALooper> mLooper;
        std::weak_ptr<AHandler> mHandler;
    };

    std::mutex mLock;
    std::map<ALooper::handler_id, HandlerInfo> mHandlers;
    ALooper::handler_id mNextHandlerID;
    uint32_t mNextReplyID;
    std::condition_variable mRepliesCondition;

    std::map<uint32_t, std::shared_ptr<AMessage>> mReplies;

    status_t postMessage_l(const std::shared_ptr<AMessage> &msg, int64_t delayUs);

    DISALLOW_EVIL_CONSTRUCTORS(ALooperRoster);
};


#endif  // A_LOOPER_ROSTER_H_
