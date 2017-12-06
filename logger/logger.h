//
// Created by xingzhao on 2017/12/1.
//

#ifndef PROJECT_LOGGER_H
#define PROJECT_LOGGER_H

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#ifndef LOG_COLOR
#define LOG_COLOR NULL
#endif

#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <vector>


#define LOG_COLOR_TAG "\033[0m"
#define LOG_COLOR_RED "\033[0;31m"
#define LOG_COLOR_YELLOW "\033[0;33m"
#define LOG_COLOR_GREEN "\033[0;32m"
#define LOG_COLOR_BLUE "\033[0;34m"
#define LOG_COLOR_MAGENTA "\033[0;35m"
#define LOG_COLOR_CYAN "\033[0;36m"

#if 1

#define VLOGD(...)   g_vLog->log('D',LOG_COLOR_TAG,LOG_TAG,__LINE__,__VA_ARGS__)
#define VLOGI(...)   g_vLog->log('I',LOG_COLOR_GREEN,LOG_TAG,__LINE__,__VA_ARGS__)
#define VLOGW(...)   g_vLog->log('W',LOG_COLOR_YELLOW,LOG_TAG,__LINE__,__VA_ARGS__)
#define VLOGE(...)   g_vLog->log('E',LOG_COLOR_RED,LOG_TAG,__LINE__,__VA_ARGS__)

#else

#define VLOGD(...)
#define VLOGI(...)
#define VLOGW(...)
#define VLOGE(...)
#define VLOGA(...)
#endif

#include <stdarg.h>
#include <mutex>

#define LOG_BUF_SIZE 4096

class VGTP_API VLog : public AHandler {
    public:
    enum {
        kWhatStart,
        kWhatStop,
        kWhatLog
    };

    VLog();

    status_t start();

    status_t stop();

    void log(const char prio, const char *color, const char *tag, unsigned int line, const char *fmt, ...);

    virtual ~VLog();

    protected:
    virtual void onMessageReceived(const std::shared_ptr<AMessage> &msg);

    private:
    FILE *log_file_ptr_;
    std::mutex lock_;
    DISALLOW_EVIL_CONSTRUCTORS(VLog);
};

extern std::shared_ptr<VLog> g_vLog;

#endif //PROJECT_LOGGER_H
