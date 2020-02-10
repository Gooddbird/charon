#ifndef __CHARON_LOG_H__
#define __CHARON_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"
#include "mutex.h"


/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 */
#define CHARON_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        charon::LogEventWrap(charon::LogEvent::ptr(new charon::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, charon::GetThreadId(),\
                charon::GetFiberId(), time(0), charon::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define CHARON_LOG_DEBUG(logger) CHARON_LOG_LEVEL(logger, charon::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define CHARON_LOG_INFO(logger) CHARON_LOG_LEVEL(logger, charon::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define CHARON_LOG_WARN(logger) CHARON_LOG_LEVEL(logger, charon::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define CHARON_LOG_ERROR(logger) CHARON_LOG_LEVEL(logger, charon::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define CHARON_LOG_FATAL(logger) CHARON_LOG_LEVEL(logger, charon::LogLevel::FATAL)

/**
 * @brief 获取主日志器
 */
#define CHARON_LOG_ROOT() charon::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 */
#define CHARON_LOG_NAME(name) charon::LoggerMgr::GetInstance()->getLogger(name)

namespace charon {

class LogEvent;
class LogLevel;
class LogAppender;
class LogFormatter;
class LogEventWrap;
class Logger;
//日志级别
class LogLevel {
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };
    static const char *ToString(LogLevel::Level level);
};
//日志事件
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name);
    ~LogEvent();
    const char *getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; } 
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    const std::string& getThreadName() const { return m_threadName; }
    std::string getContent() { return m_ss.str(); }
    std::stringstream &getSS() { return m_ss; }
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }
    void format(const char *fmt, ...);
    void format(const char *ftm, va_list al);
private:
    const char *m_file = nullptr;   //文件名
    int32_t m_line = 0;             //行号
    uint32_t m_elapse = 0;          //程序启动后到现在的毫秒数
    uint32_t m_threadId = 0;        //线程id
    uint32_t m_fiberId = 0;         //协程id
    uint64_t m_time = 0;            //时间戳
    std::string m_threadName;
    std::stringstream m_ss;
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    std::stringstream& getSS();
    LogEvent::ptr getEvent() const { return m_event; }
private:
    LogEvent::ptr m_event;
};

//日志格式器
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string &pattern);
    //%t    %thread_id %m%n
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };
    void init();
private:
    std::string m_pattern; 
    std::vector<FormatItem::ptr> m_items;
};


//日志输出地
class LogAppender {
friend class Logger;
public:
    typedef charon::Spinlock MutexType;
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {};
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
    void setLevel(LogLevel::Level level) { m_level = level; }
    LogLevel::Level getLevel() const { return m_level; }
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    bool m_hasFormatter = false;
    MutexType m_mutex;
    LogFormatter::ptr m_formatter;
};

//日志器
class Logger : public std::enable_shared_from_this<Logger> {
friend class LogManager;
public:
    typedef charon::Spinlock MutexType;
    typedef std::shared_ptr<Logger> ptr;
    Logger(const std::string &name = "root");

    void log(LogLevel::Level level, LogEvent::ptr event);
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppender();
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }
    const std::string& getName() const { return m_name; }
    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
private:
    std::string m_name;
    std::list<LogAppender::ptr> m_appenders;
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
    MutexType m_mutex;
    Logger::ptr m_root;
};


class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    
};

class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string &filename);    
    virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
};

class LogManager {
public:
    typedef charon::Spinlock MutexType;
    LogManager();
    void init();
    Logger::ptr getLogger(const std::string &name);
    Logger::ptr getRoot() const { return m_root; } 
private:
    MutexType m_mutex;
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef charon::Singleton<LogManager> LoggerMgr;

}

#endif