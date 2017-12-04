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
#define LOG_TAG "ALooperRoster"

#include <stdio.h>
#include "ALooperRoster.h"

#include "AHandler.h"
#include "AMessage.h"


ALooperRoster::ALooperRoster()
        : mNextHandlerID(1),
          mNextReplyID(1) {
}

ALooper::handler_id ALooperRoster::registerHandler(
        const std::shared_ptr<ALooper> &looper, const std::shared_ptr<AHandler> &handler) {
    std::lock_guard<std::mutex> lck(mLock);
    if (handler->id() != 0) {
        //CHECK(!"A handler must only be registered once.");
        return INVALID_OPERATION;
    }

    HandlerInfo info;
    info.mLooper = looper;
    info.mHandler = handler;
    ALooper::handler_id handlerID = mNextHandlerID++;
    mHandlers.insert(std::pair<ALooper::handler_id, HandlerInfo>(handlerID, info));
    //ALOGW("ALooper::handler_id ALooperRoster::registerHandler %d\n",handlerID);

    handler->setID(handlerID);
    return handlerID;
}

void ALooperRoster::unregisterHandler(ALooper::handler_id handlerID) {
    std::lock_guard<std::mutex> lck(mLock);
    //ALOGI("ALooperRoster unregisterHandler handlerID=%x\n",handlerID);

    std::map<ALooper::handler_id, HandlerInfo>::iterator iter;
    iter = mHandlers.find(handlerID);
    if (iter == mHandlers.end()) {
        return;
    }

    const HandlerInfo &info = iter->second;

    std::shared_ptr<AHandler> handler = NULL;
    handler = info.mHandler.lock();
    if (handler != NULL) {
        handler->setID(0);
    }

    mHandlers.erase(iter);
}

void ALooperRoster::unregisterStaleHandlers() {
    std::lock_guard<std::mutex> lck(mLock);
    std::map<ALooper::handler_id, HandlerInfo>::iterator iter;
    for (iter = mHandlers.begin(); iter != mHandlers.end();) {
        std::shared_ptr<ALooper> looper = NULL;
        looper = iter->second.mLooper.lock();
        if (looper == NULL) {
            iter = mHandlers.erase(iter);
        } else {
            iter++;
        }
    }
}

status_t ALooperRoster::postMessage(
        const std::shared_ptr<AMessage> &msg, int64_t delayUs) {
    std::lock_guard<std::mutex> lck(mLock);
    return postMessage_l(msg, delayUs);
}

status_t ALooperRoster::postMessage_l(
        const std::shared_ptr<AMessage> &msg, int64_t delayUs) {
    std::map<ALooper::handler_id, HandlerInfo>::iterator iter;
    iter = mHandlers.find(msg->target());
    if (iter == mHandlers.end()) {
        return -ENOENT;;
    }

    const HandlerInfo &info = iter->second;

    std::shared_ptr<ALooper> looper = NULL;
    looper = info.mLooper.lock();
    if (looper == NULL) {
        mHandlers.erase(iter);
        return -ENOENT;
    }


    looper->post(msg, delayUs);

    return OK;
}

void ALooperRoster::deliverMessage(const std::shared_ptr<AMessage> &msg) {
    std::shared_ptr<AHandler> handler = NULL;
    {
        std::lock_guard<std::mutex> lck(mLock);
        std::map<ALooper::handler_id, HandlerInfo>::iterator iter;
        iter = mHandlers.find(msg->target());
        if (iter == mHandlers.end()) {
            return;
        }
        const HandlerInfo &info = iter->second;
        handler = info.mHandler.lock();
        if (handler == NULL) {
            mHandlers.erase(iter);
            return;
        }

    }
    handler->onMessageReceived(msg);
}

std::shared_ptr<ALooper> ALooperRoster::findLooper(ALooper::handler_id handlerID) {
    std::lock_guard<std::mutex> lck(mLock);

    std::map<ALooper::handler_id, HandlerInfo>::iterator iter;
    iter = mHandlers.find(handlerID);
    if (iter == mHandlers.end()) {
        return NULL;
    }
    std::shared_ptr<ALooper> looper = NULL;
    looper = iter->second.mLooper.lock();
    if (looper == NULL) {
        mHandlers.erase(iter);
        return NULL;
    }

    return looper;
}

status_t ALooperRoster::postAndAwaitResponse(
        const std::shared_ptr<AMessage> &msg, std::shared_ptr<AMessage> *response) {
    std::unique_lock<std::mutex> lck(mLock);

    int32_t replyID = mNextReplyID++;

    msg->setInt32("replyID", replyID);

    status_t err = postMessage_l(msg, 0 /* delayUs */);

    if (err != OK) {
        (*response)->clear();
        return err;
    }

    std::map<uint32_t, std::shared_ptr<AMessage>>::iterator iter;
    while ((iter = mReplies.find(replyID)) == mReplies.end()) {
        mRepliesCondition.wait(lck);
    }
    *response = iter->second;
    mReplies.erase(iter);

    return OK;
}

void ALooperRoster::postReply(uint32_t replyID, const std::shared_ptr<AMessage> &reply) {
    std::unique_lock<std::mutex> lck(mLock);
    std::map<uint32_t, std::shared_ptr<AMessage>>::iterator iter;
    iter = mReplies.find(replyID);
    if (iter != mReplies.end()) {
        return;
    }
    mReplies.insert(std::pair<uint32_t, std::shared_ptr<AMessage>>(replyID, reply));
    mRepliesCondition.notify_all();
}

