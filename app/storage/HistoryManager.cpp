#include "HistoryManager.h"
#include "Logger.h"
#include <QSqlQuery>

namespace Nova {

HistoryManager& HistoryManager::instance() {
    static HistoryManager inst;
    return inst;
}

HistoryManager::HistoryManager(QObject* parent) : QObject(parent) {
    m_db = std::make_unique<SQLiteDB>("history_db");
}

bool HistoryManager::initialize(const QString& dbPath) {
    if (!m_db->open(dbPath)) {
        LOG_ERROR("HistoryManager", "Cannot open history database: " + dbPath);
        return false;
    }
    return createSchema();
}

bool HistoryManager::createSchema() {
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS history (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            url         TEXT NOT NULL,
            title       TEXT,
            favicon     TEXT,
            visit_time  INTEGER NOT NULL,
            visit_count INTEGER NOT NULL DEFAULT 1
        );
        CREATE INDEX IF NOT EXISTS idx_history_url ON history(url);
        CREATE INDEX IF NOT EXISTS idx_history_visit_time ON history(visit_time);
    )";

    // Execute each statement separately
    bool ok = m_db->execute(
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "url TEXT NOT NULL,"
        "title TEXT,"
        "favicon TEXT,"
        "visit_time INTEGER NOT NULL,"
        "visit_count INTEGER NOT NULL DEFAULT 1)"
    );
    ok &= m_db->execute("CREATE INDEX IF NOT EXISTS idx_history_url ON history(url)");
    ok &= m_db->execute("CREATE INDEX IF NOT EXISTS idx_history_visit_time ON history(visit_time)");
    return ok;
}

void HistoryManager::addVisit(const QString& url, const QString& title, const QString& favicon) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if URL exists
    auto q = m_db->query(
        "SELECT id, visit_count FROM history WHERE url = ? ORDER BY visit_time DESC LIMIT 1",
        {url}
    );

    if (q.next()) {
        qint64 id = q.value(0).toLongLong();
        int count = q.value(1).toInt() + 1;
        m_db->execute(
            "UPDATE history SET title=?, favicon=?, visit_time=?, visit_count=? WHERE id=?",
            {title, favicon, QDateTime::currentSecsSinceEpoch(), count, id}
        );
    } else {
        m_db->execute(
            "INSERT INTO history (url, title, favicon, visit_time, visit_count) VALUES (?,?,?,?,1)",
            {url, title, favicon, QDateTime::currentSecsSinceEpoch()}
        );
    }

    emit historyChanged();
}

void HistoryManager::updateTitle(const QString& url, const QString& title) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_db->execute(
        "UPDATE history SET title=? WHERE url=?",
        {title, url}
    );
}

QVector<HistoryEntry> HistoryManager::search(const QString& query, int limit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const QString pattern = "%" + query + "%";
    auto q = m_db->query(
        "SELECT id,url,title,favicon,visit_time,visit_count FROM history "
        "WHERE url LIKE ? OR title LIKE ? "
        "ORDER BY visit_time DESC LIMIT ?",
        {pattern, pattern, limit}
    );

    QVector<HistoryEntry> results;
    while (q.next()) results.append(entryFromQuery(q));
    return results;
}

QVector<HistoryEntry> HistoryManager::recentHistory(int limit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query(
        "SELECT id,url,title,favicon,visit_time,visit_count FROM history "
        "ORDER BY visit_time DESC LIMIT ?",
        {limit}
    );

    QVector<HistoryEntry> results;
    while (q.next()) results.append(entryFromQuery(q));
    return results;
}

QVector<HistoryEntry> HistoryManager::historyForDay(const QDate& date) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    qint64 start = QDateTime(date, QTime(0,0,0)).toSecsSinceEpoch();
    qint64 end   = QDateTime(date, QTime(23,59,59)).toSecsSinceEpoch();

    auto q = m_db->query(
        "SELECT id,url,title,favicon,visit_time,visit_count FROM history "
        "WHERE visit_time BETWEEN ? AND ? ORDER BY visit_time DESC",
        {start, end}
    );

    QVector<HistoryEntry> results;
    while (q.next()) results.append(entryFromQuery(q));
    return results;
}

void HistoryManager::deleteEntry(qint64 id) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("DELETE FROM history WHERE id=?", {id});
    }
    emit historyChanged();
}

void HistoryManager::deleteUrl(const QString& url) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("DELETE FROM history WHERE url=?", {url});
    }
    emit historyChanged();
}

void HistoryManager::clearAll() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("DELETE FROM history");
    }
    emit historyChanged();
}

void HistoryManager::clearOlderThan(const QDateTime& dt) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("DELETE FROM history WHERE visit_time < ?", {dt.toSecsSinceEpoch()});
    }
    emit historyChanged();
}

bool HistoryManager::hasVisited(const QString& url) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query("SELECT COUNT(*) FROM history WHERE url=?", {url});
    return q.next() && q.value(0).toInt() > 0;
}

int HistoryManager::visitCount(const QString& url) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query(
        "SELECT COALESCE(SUM(visit_count),0) FROM history WHERE url=?", {url}
    );
    return q.next() ? q.value(0).toInt() : 0;
}

HistoryEntry HistoryManager::entryFromQuery(QSqlQuery& q) const {
    HistoryEntry e;
    e.id         = q.value(0).toLongLong();
    e.url        = q.value(1).toString();
    e.title      = q.value(2).toString();
    e.favicon    = q.value(3).toString();
    e.visitTime  = QDateTime::fromSecsSinceEpoch(q.value(4).toLongLong());
    e.visitCount = q.value(5).toInt();
    return e;
}

} // namespace Nova
