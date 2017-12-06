//
// Created by xingzhao on 2017/12/4.
//

#include "TestHandler.h"

TestHandler::TestHandler() {
}

TestHandler::~TestHandler() {
}

void TestHandler::init() {
    std::shared_ptr<AMessage> msg(new AMessage(kWhatInit, id()));
    msg->post();
}

void TestHandler::initHandler() {
    std::shared_ptr<AMessage> test_notify(new AMessage(kWhatTestHandlerNotify, id()));
    test_handler_ = std::shared_ptr<TestHandler>(new TestHandler(test_notify));
}

void TestHandler::startHandler() {
    test_handler_->start();
}

void TestHandler::stopHandler() {
    test_handler_->stop();
}

void TestHandler::notifyHandler(const std::shared_ptr<AMessage> &msg) {
    uint32_t replyID;
    msg->senderAwaitsResponse(&replyID);
    status_t err = OK;
    std::shared_ptr<AMessage> response(new AMessage());
    response->setInt32("err", err);
    response->postReply(replyID);
}


void TestHandler::onMessageReceived(const std::shared_ptr<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatInit: {
            initHandler();
            break;
        }
        case kWhatStart: {
            startHandler();
            break;
        }
        case kWhatStop: {
            stopHandler();
            break;
        }
        case kWhatTestHandlerNotify: {
            notifyHandler(msg);
            break;
        }
    }
}
