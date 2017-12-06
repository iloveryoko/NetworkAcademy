//
// Created by xingzhao on 2017/12/4.
//

#ifndef PROJECT_TESTMAINLOOPER_H
#define PROJECT_TESTMAINLOOPER_H

#include "AHandler.h"
#include "AMessage.h"
#include "TestHandler.h"

class TestMainLooper: AHandler {
public:
    TestMainLooper();
    virtual ~TestMainLooper();


    enum {
        kWhatInit,
        kWhatStart,
        kWhatStop,
        kWhatTestHandlerNotify,
    };

    void init();
    void testSendInt();
    bool testSendString();
    bool testSendPtr();

protected:
    virtual void onMessageReceived(const std::shared_ptr<AMessage> &msg);

private:
    DISALLOW_EVIL_CONSTRUCTORS(TestMainLooper);

    std::shared_ptr<TestHandler> test_handler_;

    void initHandler();
    void startHandler();
    void stopHandler();
    void notifyHandler(const std::shared_ptr<AMessage> &msg);
};



#endif //PROJECT_TESTMAINLOOPER_H
