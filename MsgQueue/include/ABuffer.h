//
// Created by ChrisChung on 2016/3/31.
//

#ifndef VPIXEL_ABUFFER_H
#define VPIXEL_ABUFFER_H

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

#include "ABase.h"
#include "AMessage.h"

struct ABuffer {
    ABuffer(size_t capacity);

    ABuffer(void *data, size_t capacity);

    void setFarewellMessage(const std::shared_ptr<AMessage> &msg);

    uint8_t *base() { return (uint8_t *) mData; }

    uint8_t *data() { return (uint8_t *) mData + mRangeOffset; }

    size_t capacity() const { return mCapacity; }

    size_t size() const { return mRangeLength; }

    size_t offset() const { return mRangeOffset; }

    void setRange(size_t offset, size_t size);

    void setInt32Data(int32_t data) { mInt32Data = data; }

    int32_t int32Data() const { return mInt32Data; }

    std::shared_ptr<AMessage> meta();

    virtual ~ABuffer();

private:
    std::shared_ptr<AMessage> mFarewell;
    std::shared_ptr<AMessage> mMeta;
    void *mData;
    size_t mCapacity;
    size_t mRangeOffset;
    size_t mRangeLength;
    int32_t mInt32Data;
    bool mOwnsData;
    DISALLOW_EVIL_CONSTRUCTORS(ABuffer);
};

#endif //VPIXEL_ABUFFER_H
