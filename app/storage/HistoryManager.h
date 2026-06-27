#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include "SQLiteDB.h"
#include <memory>
#include <mutex>

namespace Nova {

struct HistoryEntry {
    qint64 id{0};
    QString url;
    QString title;
    QDateTime visitTime;
    int visitCount{1};
    QString favicon;
};

class HistoryManager : public QObject {
    Q_OBJECT

public:
    static HistoryManager& instance();

    bool initialize(const QString& dbPath);

    void addVisit(const QString& url, const QString& title, const QString& favicon = {});
    void updateTitle(const QString& url, const QString& title);

    QVector<HistoryEntry> search(const QString& query, int limit = 100) const;
    QVector<HistoryEntry> recentHistory(int limit = 50) const;
    QVector<HistoryEntry> historyForDay(const QDate& date) const;

    void deleteEntry(qint64 id);
    void deleteUrl(const QString& url);
    void clearAll();
    void clearOlderThan(const QDateTime& dt);

    bool hasVisited(const QString& url) const;
    int visitCount(const QString& url) const;

signals:
    void historyChanged();

private:
    explicit HistoryManager(QObject* parent = nullptr);
    ~HistoryManager() override = default;

    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    bool createSchema();
    HistoryEntry entryFromQuery(QSqlQuery& q) const;

    std::unique_ptr<SQLiteDB> m_db;
    mutable std::mutex m_mutex;
};

} // namespace Nova
