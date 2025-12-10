#include "logger.h"
#include <iostream>
#include <stdexcept>

namespace logger
{

// 静态成员初始化
LoggerConfig* Logger::sConfig = nullptr;
QMutex Logger::sInitMutex;
bool Logger::sLoggerInitialized = false;

// LoggerConfig类
LoggerConfig::LoggerConfig(Level level, const QString& filePath, qint64 maxSize)
    : logLevel(level)
    , logFilePath(filePath)
    , logFile(nullptr)
    , logStream(nullptr)
    , writeMutex(new QMutex())
    , maxFileSize(maxSize)
{
}

bool LoggerConfig::initialize()
{
    QMutexLocker locker(writeMutex);

    if (logFile && logFile->isOpen()) {
        return true;
    }

    logFile = new QFile(logFilePath);
    // 检查日志文件是否存在且是否超出大小限制
    if (logFile->exists() && logFile->size() > maxFileSize) {
        if (!logFile->remove()) {
            //std::cerr << "Failed to delete old log file: " << logFilePath.toStdString() << std::endl;
            delete logFile;
            logFile = nullptr;
            return false;
        }
    }

    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        //std::cerr << "Failed to open log file: " << logFilePath.toStdString() << std::endl;
        delete logFile;
        logFile = nullptr;
        return false;
    }

    logStream = new QTextStream(logFile);
    return true;
}

void LoggerConfig::close()
{
    QMutexLocker locker(writeMutex);

    if (logStream) {
        logStream->flush();
        delete logStream;
        logStream = nullptr;
    }

    if (logFile) {
        if (logFile->isOpen()) {
            logFile->close();
        }
        delete logFile;
        logFile = nullptr;
    }
}

LoggerConfig::~LoggerConfig()
{
    close();
    delete writeMutex;
}

// Logger类
Logger::Logger(const char* className, const char* functionName, int line, Level level)
{
    if (!sLoggerInitialized || !sConfig) {
        isLoggingEnabled = false;
        return;
    }

    // 检查级别是否应该记录
    isLoggingEnabled = (level <= sConfig->logLevel);

    if (isLoggingEnabled) {
        // 构建日志头
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString levelStr = levelToString(level);

        // 优化: 使用单次 arg() 调用，避免链式调用的临时对象
        myBuffer = QString("[%1] [%2] [%3::%4:%5] ")
                       .arg(timestamp,
                            levelStr,
                            className,
                            functionName,
                            QString::number(line));  // 明确转换整数为 QString

        myStream.setString(&myBuffer, QIODevice::WriteOnly | QIODevice::Append);
    }
}

void Logger::init(Level level, const QString& filePath)
{
    QMutexLocker locker(&sInitMutex);

    if (sLoggerInitialized) {
        return;
    }

    sConfig = new LoggerConfig(level, filePath);
    if (!sConfig->initialize()) {
        delete sConfig;
        sConfig = nullptr;
        throw std::runtime_error("Failed to initialize(open or delete) logger configuration");
    }
    sLoggerInitialized = true;
}

void Logger::destroy()
{
    QMutexLocker locker(&sInitMutex);

    if (sConfig) {
        sConfig->close();
        delete sConfig;
        sConfig = nullptr;
    }
    sLoggerInitialized = false;
}

bool Logger::isEnabled(Level level)
{
    if (!sLoggerInitialized || !sConfig) {
        return false;
    }

    // 只记录小于等于配置级别的日志
    return level <= sConfig->logLevel;
}

QString Logger::levelToString(Level level)
{
    switch (level) {
    case Error:   return "ERROR";
    case Warning: return "WARNING";
    case Info:    return "INFO";
    default:      return "UNKNOWN";
    }
}

Logger::~Logger()
{
    if (isLoggingEnabled && sConfig && sConfig->logStream) {
        QMutexLocker locker(sConfig->writeMutex);

        *sConfig->logStream << myBuffer << "\n";
        sConfig->logStream->flush();
    }
}

QTextStream& Logger::stream()
{
    return myStream;
}

} // namespace logger
