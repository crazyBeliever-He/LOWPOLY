#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>

namespace logger
{

enum Level {
    Error,   // 错误
    Warning, // 警告
    Info     // 信息
};

class LoggerConfig
{
public:
    Level logLevel;
    QString logFilePath;
    QFile* logFile;
    QTextStream* logStream;
    QMutex* writeMutex;         // QRecursiveMutex 和 QMutex 的选择
    qint64 maxFileSize;         // 最大日志文件大小限制 (字节), 默认10MB

    LoggerConfig(Level level = Info,
                 const QString& filePath = "LowPolyLog.txt",
                 qint64 maxSize = 10485760);
    ~LoggerConfig();

    bool initialize();
    void close();
};

class Logger
{
public:
    Logger(const char* className, const char* functionName, int line, Level level);
    ~Logger();

    QTextStream& stream();

    static void init(Level level = Info, const QString& filePath = "LowPolyLog.txt");
    static void destroy();
    static bool isEnabled(Level level);
    static QString levelToString(Level level);

private:
    QTextStream myStream;
    QString myBuffer;
    bool isLoggingEnabled;

    static LoggerConfig* sConfig;
    static QMutex sInitMutex;
    static bool sLoggerInitialized;
};

// 日志宏定义
#define LOG_INFO    if (logger::Logger::isEnabled(logger::Info)) \
    logger::Logger(__FILE__, __func__, __LINE__, logger::Info).stream()
#define LOG_WARNING if (logger::Logger::isEnabled(logger::Warning)) \
    logger::Logger(__FILE__, __func__, __LINE__, logger::Warning).stream()
#define LOG_ERROR   if (logger::Logger::isEnabled(logger::Error)) \
    logger::Logger(__FILE__, __func__, __LINE__, logger::Error).stream()

} // namespace logger

#endif // LOGGER_H
