#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <memory>

namespace Nova {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger : public QObject {
    Q_OBJECT

public:
    static Logger& instance();

    void setLogFile(const QString& path);
    void setMinLevel(LogLevel level);

    void log(LogLevel level, const QString& category, const QString& message);
    void debug(const QString& category, const QString& message);
    void info(const QString& category, const QString& message);
    void warning(const QString& category, const QString& message);
    void error(const QString& category, const QString& message);
    void critical(const QString& category, const QString& message);

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger() override;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QString levelToString(LogLevel level) const;
    bool shouldLog(LogLevel level) const;

    QMutex m_mutex;
    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_stream;
    LogLevel m_minLevel{LogLevel::Debug};
};

// Convenience macros
#define LOG_DEBUG(cat, msg)    Nova::Logger::instance().debug(cat, msg)
#define LOG_INFO(cat, msg)     Nova::Logger::instance().info(cat, msg)
#define LOG_WARN(cat, msg)     Nova::Logger::instance().warning(cat, msg)
#define LOG_ERROR(cat, msg)    Nova::Logger::instance().error(cat, msg)
#define LOG_CRITICAL(cat, msg) Nova::Logger::instance().critical(cat, msg)

} // namespace Nova
