#include "Logger.h"
#include <QStandardPaths>
#include <iostream>

namespace Nova {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger(QObject* parent) : QObject(parent) {}

Logger::~Logger() {
    if (m_stream) {
        m_stream->flush();
    }
}

void Logger::setLogFile(const QString& path) {
    QMutexLocker locker(&m_mutex);
    m_logFile = std::make_unique<QFile>(path);
    if (m_logFile->open(QIODevice::Append | QIODevice::Text)) {
        m_stream = std::make_unique<QTextStream>(m_logFile.get());
    } else {
        m_logFile.reset();
    }
}

void Logger::setMinLevel(LogLevel level) {
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

void Logger::log(LogLevel level, const QString& category, const QString& message) {
    if (!shouldLog(level)) return;

    QMutexLocker locker(&m_mutex);
    const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    const QString formatted = QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(levelToString(level))
        .arg(category)
        .arg(message);

    if (m_stream) {
        *m_stream << formatted << "\n";
        m_stream->flush();
    }

    // Also output to stderr for debug builds
#ifdef DEBUG
    std::cerr << formatted.toStdString() << std::endl;
#else
    if (level >= LogLevel::Warning) {
        std::cerr << formatted.toStdString() << std::endl;
    }
#endif
}

void Logger::debug(const QString& category, const QString& message) {
    log(LogLevel::Debug, category, message);
}

void Logger::info(const QString& category, const QString& message) {
    log(LogLevel::Info, category, message);
}

void Logger::warning(const QString& category, const QString& message) {
    log(LogLevel::Warning, category, message);
}

void Logger::error(const QString& category, const QString& message) {
    log(LogLevel::Error, category, message);
}

void Logger::critical(const QString& category, const QString& message) {
    log(LogLevel::Critical, category, message);
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

bool Logger::shouldLog(LogLevel level) const {
    return level >= m_minLevel;
}

} // namespace Nova
