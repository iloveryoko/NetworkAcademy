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

#include "AMessage.h"
#include "ALooperRoster.h"


extern ALooperRoster gLooperRoster;

AMessage::AMessage(uint32_t what, ALooper::handler_id target)
        : mWhat(what),
          mTarget(target),
          mNumItems(0) {
}

AMessage::~AMessage() {
    clear();
}

void AMessage::setWhat(uint32_t what) {
    mWhat = what;
}

uint32_t AMessage::what() const {
    return mWhat;
}

void AMessage::setTarget(ALooper::handler_id handlerID) {
    mTarget = handlerID;
}

ALooper::handler_id AMessage::target() const {
    return mTarget;
}

void AMessage::clear() {
    for (size_t i = 0; i < mNumItems; ++i) {
        Item *item = &mItems[i];
        freeItem(item);
    }
    mNumItems = 0;
}

void AMessage::freeItem(Item *item) {
    switch (item->mType) {
        case kTypeString: {
//            delete item->u.stringValue;
            break;
        }

        case kTypeMessage: {
            /*Todo
            if (item->u.refValue != NULL) {
                item->u.refValue->decStrong(this);
            }*/
            break;
        }

        default:
            break;
    }
}

AMessage::Item *AMessage::allocateItem(const char *name) {
    if (mNumItems >= kMaxNumItems) {
        return NULL;
    }

    int i = 0;
    while (i < mNumItems && (strcmp(mItems[i].mName, name) != 0)) {
        ++i;
    }

    Item *item;

    if (i < mNumItems) {
        item = &mItems[i];
        freeItem(item);
        item = NULL;
    } else {
        i = mNumItems++;
        item = &mItems[i];
        item->mName = name;
    }

    return item;
}

const AMessage::Item *AMessage::findItem(
        const char *name, Type type) const {

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item *item = &mItems[i];

        if (strcmp(item->mName, name) == 0) {
            return item->mType == type ? item : NULL;
        }
    }

    return NULL;
}

#define BASIC_TYPE(NAME, FIELDNAME, TYPENAME)                             \
void AMessage::set##NAME(const char *name, TYPENAME value) {            \
    Item *item = allocateItem(name);                                    \
                                                                        \
    item->mType = kType##NAME;                                          \
    item->u.FIELDNAME = value;                                          \
}                                                                       \
                                                                        \
bool AMessage::find##NAME(const char *name, TYPENAME *value) const {    \
    const Item *item = findItem(name, kType##NAME);                     \
    if (item) {                                                         \
        *value = item->u.FIELDNAME;                                     \
        return true;                                                    \
    }                                                                   \
    return false;                                                       \
}

BASIC_TYPE(Int32, int32Value, int32_t)

BASIC_TYPE(Int64, int64Value, int64_t)

BASIC_TYPE(Size, sizeValue, size_t)

BASIC_TYPE(Float, floatValue, float)

BASIC_TYPE(Double, doubleValue, double)

BASIC_TYPE(Pointer, ptrValue, void *)

#undef BASIC_TYPE

void AMessage::setString(
        const char *name, const char *s, ssize_t len) {
    Item *item = allocateItem(name);
    item->mType = kTypeString;
//    item->u.stringValue = new std::string(s, len <= 0 ? strlen(s) : len);
    strcpy(item->u.stringValue, s);
}


//bool AMessage::findString(const char *name, std::string *value) const {
//    const Item *item = findItem(name, kTypeString);
//    if (item) {
//        *value = *item->u.stringValue;
//        return true;
//    }
//    return false;
//}

bool AMessage::findString(const char *name, char *value) const {
    const Item *item = findItem(name, kTypeString);
    if (item) {
        strcpy(value, item->u.stringValue);
        return true;
    }
    return false;
}

void AMessage::post(int64_t delayUs) {
    gLooperRoster.postMessage(shared_from_this(), delayUs);
}

status_t AMessage::postAndAwaitResponse(std::shared_ptr<AMessage> *response) {
    return gLooperRoster.postAndAwaitResponse(shared_from_this(), response);
}

void AMessage::postReply(uint32_t replyID) {
    gLooperRoster.postReply(replyID, shared_from_this());
}

bool AMessage::senderAwaitsResponse(uint32_t *replyID) const {
    int32_t tmp;
    bool found = findInt32("replyID", &tmp);

    if (!found) {
        return false;
    }

    *replyID = static_cast<uint32_t>(tmp);

    return true;
}

std::shared_ptr<AMessage> AMessage::dup() const {
    std::shared_ptr<AMessage> msg(new AMessage(mWhat, mTarget));
    msg->mNumItems = mNumItems;

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item *from = &mItems[i];
        Item *to = &msg->mItems[i];

        to->mName = from->mName;
        to->mType = from->mType;

        switch (from->mType) {
            case kTypeString: {
                strcpy(to->u.stringValue, from->u.stringValue);
                break;
            }
            default: {
                to->u = from->u;
                break;
            }
        }
    }

    return msg;
}

static void appendIndent(std::string *s, int32_t indent) {
    static const char kWhitespace[] =
            "                                        "
                    "                                        ";
    s->append(kWhitespace, indent);
}

static bool isFourcc(uint32_t what) {
    return isprint(what & 0xff)
           && isprint((what >> 8) & 0xff)
           && isprint((what >> 16) & 0xff)
           && isprint((what >> 24) & 0xff);
}

/*
 * std::string AMessage::debugString(int32_t indent) const {
    std::string s = "AMessage(what = ";

    std::string tmp;
    if (isFourcc(mWhat)) {
        tmp = sprintf(
                "'%c%c%c%c'",
                const_cast<char *> (mWhat >> 24),
                const_cast<char *> ((mWhat >> 16) & 0xff),
                const_cast<char *> ((mWhat >> 8) & 0xff),
                const_cast<char *> (mWhat & 0xff));
    } else {
        tmp = sprintf("0x%08x", mWhat);
    }
    s.append(tmp);

    if (mTarget != 0) {
        tmp = sprintf(", target = %d", mTarget);
        s.append(tmp);
    }
    s.append(") = {\n");

    for (size_t i = 0; i < mNumItems; ++i) {
        const Item &item = mItems[i];

        switch (item.mType) {
            case kTypeInt32:
                tmp = sprintf(
                        "int32_t %s = %d", item.mName, item.u.int32Value);
                break;
            case kTypeInt64:
                tmp = sprintf(
                        "int64_t %s = %lld", item.mName, item.u.int64Value);
                break;
            case kTypeSize:
                tmp = sprintf(
                        "size_t %s = %d", item.mName, item.u.sizeValue);
                break;
            case kTypeFloat:
                tmp = sprintf(
                        "float %s = %f", item.mName, item.u.floatValue);
                break;
            case kTypeDouble:
                tmp = sprintf(
                        "double %s = %f", item.mName, item.u.doubleValue);
                break;
            case kTypePointer:
                tmp = sprintf(
                        "void *%s = %p", item.mName, item.u.ptrValue);
                break;
            case kTypeString:
                tmp = sprintf(
                        "string %s = \"%s\"",
                        item.mName,
                        item.u.stringValue->c_str());
                break;
            default:
                break;
        }

        appendIndent(&s, indent);
        s.append("  ");
        s.append(tmp);
        s.append("\n");
    }

    appendIndent(&s, indent);
    s.append("}");

    return s;
}*/


size_t AMessage::countEntries() const {
    return mNumItems;
}

const char *AMessage::getEntryNameAt(size_t index, Type *type) const {
    if (index >= mNumItems) {
        *type = kTypeInt32;

        return NULL;
    }

    *type = mItems[index].mType;

    return mItems[index].mName;
}

