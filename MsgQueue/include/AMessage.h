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

#ifndef A_MESSAGE_H_

#define A_MESSAGE_H_

#include "ABase.h"
#include "ALooper.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>



class AMessage : public std::enable_shared_from_this<AMessage> {
public:
    AMessage(uint32_t what = 0, ALooper::handler_id target = 0);

    void setWhat(uint32_t what);

    uint32_t what() const;

    void setTarget(ALooper::handler_id target);

    ALooper::handler_id target() const;

    void clear();

    void setInt32(const char *name, int32_t value);

    void setInt64(const char *name, int64_t value);

    void setSize(const char *name, size_t value);

    void setFloat(const char *name, float value);

    void setDouble(const char *name, double value);

    void setPointer(const char *name, void *value);

    void setString(const char *name, const char *s, ssize_t len = -1);

    void setMessage(const char *name, const AMessage &obj);


    bool findInt32(const char *name, int32_t *value) const;

    bool findInt64(const char *name, int64_t *value) const;

    bool findSize(const char *name, size_t *value) const;

    bool findFloat(const char *name, float *value) const;

    bool findDouble(const char *name, double *value) const;

    bool findPointer(const char *name, void **value) const;

//    bool findString(const char *name, std::string *value) const;

    bool findString(const char *name, char *value) const;

    void post(int64_t delayUs = 0);

    // Posts the message to its target and waits for a response (or error)
    // before returning.
    status_t postAndAwaitResponse(std::shared_ptr<AMessage> *response);

    // If this returns true, the sender of this message is synchronously
    // awaiting a response, the "replyID" can be used to send the response
    // via "postReply" below.
    bool senderAwaitsResponse(uint32_t *replyID) const;

    void postReply(uint32_t replyID);

    // Performs a deep-copy of "this", contained messages are in turn "dup'ed".
    // Warning: RefBase items, i.e. "objects" are _not_ copied but only have
    // their refcount incremented.
    std::shared_ptr<AMessage> dup() const;

    //std::string debugString(int32_t indent = 0) const;

    enum Type {
        kTypeInt32,
        kTypeInt64,
        kTypeSize,
        kTypeFloat,
        kTypeDouble,
        kTypePointer,
        kTypeString,
        kTypeMessage
    };

    size_t countEntries() const;

    const char *getEntryNameAt(size_t index, Type *type) const;


    virtual ~AMessage();

private:
    uint32_t mWhat;
    ALooper::handler_id mTarget;


    struct Item {
        union {
            int32_t int32Value;
            int64_t int64Value;
            size_t sizeValue;
            float floatValue;
            double doubleValue;
            void *ptrValue;
//            std::string *stringValue;
            char stringValue[256];
        } u;
        const char *mName;
        Type mType;
    };

    enum {
        kMaxNumItems = 64
    };
    Item mItems[kMaxNumItems];
    size_t mNumItems;

    Item *allocateItem(const char *name);

    void freeItem(Item *item);

    const Item *findItem(const char *name, Type type) const;


    DISALLOW_EVIL_CONSTRUCTORS(AMessage);
};


#endif  // A_MESSAGE_H_
