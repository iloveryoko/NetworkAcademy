


#include "ABuffer.h"
#include "AMessage.h"

ABuffer::ABuffer(size_t capacity)
        : mData(malloc(capacity)),
          mCapacity(capacity),
          mRangeOffset(0),
          mRangeLength(capacity),
          mInt32Data(0),
          mOwnsData(true) {
}

ABuffer::ABuffer(void *data, size_t capacity)
        : mData(data),
          mCapacity(capacity),
          mRangeOffset(0),
          mRangeLength(capacity),
          mInt32Data(0),
          mOwnsData(false) {
}

ABuffer::~ABuffer() {
    if (mOwnsData) {
        if (mData != NULL) {
            free(mData);
            mData = NULL;
        }
    }
    if (mFarewell != NULL) {
        mFarewell->post();
    }
}

void ABuffer::setRange(size_t offset, size_t size) {
    mRangeOffset = offset;
    mRangeLength = size;
}

void ABuffer::setFarewellMessage(const std::shared_ptr<AMessage> &msg) {
    mFarewell = msg;
}

std::shared_ptr<AMessage> ABuffer::meta() {
    if (mMeta == NULL) {
        std::shared_ptr<AMessage> msg(new AMessage());
        mMeta = msg;
    }
    return mMeta;
}
