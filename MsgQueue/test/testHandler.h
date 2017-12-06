//
// Created by xingzhao on 2017/12/4.
//

#ifndef PROJECT_TESTHANDLER_H
#define PROJECT_TESTHANDLER_H

#include "AHandler.h"
#include "AMessage.h"

class TestHandler: AHandler {
public:
    TestHandler();
    virtual ~TestHandler();

    enum {
        kWhatInit,
        kWhatStart,
        kWhatStop,
        kWhatTestHandlerNotify,
    };

    enum NotificationReason {
        kIntEcho,
        kStringEcho,
        kPointerEcho,
        kError
    };

    void init();
    void testSendInt();
    bool testSendString();
    bool testSendPtr();

protected:
    virtual void onMessageReceived(const std::shared_ptr<AMessage> &msg);

private:
    DISALLOW_EVIL_CONSTRUCTORS(TestHandler);

    std::shared_ptr<AMessage> notify_to_main_;

    void initHandler();
    void startHandler();
    void stopHandler();
    void notifyHandler(const std::shared_ptr<AMessage> &msg);
};



#endif //PROJECT_TESTMAINLOOPER_H


#endif //PROJECT_TESTHANDLER_H
