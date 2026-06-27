#include "SQLiteDB.h"
#include "Logger.h"
#include <QSqlDriver>

namespace Nova {

SQLiteDB::SQLiteDB(const QString& connectionName, QObject* parent)
    : QObject(parent)
    , m_connectionName(connectionName)
{}

SQLiteDB::~SQLiteDB() {
    close();
}

bool SQLiteDB::open(const QString& databasePath) {
    QMutexLocker locker(&m_mutex);

    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(databasePath);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        LOG_ERROR("SQLiteDB", "Failed to open database: " + m_lastError);
        return false;
    }

    enableWALMode(db);

    // Enable foreign keys
    QSqlQuery q(db);
    q.exec("PRAGMA foreign_keys = ON");
    q.exec("PRAGMA journal_mode = WAL");
    q.exec("PRAGMA synchronous = NORMAL");
    q.exec("PRAGMA cache_size = 10000");
    q.exec("PRAGMA temp_store = MEMORY");

    LOG_INFO("SQLiteDB", "Opened database: " + databasePath);
    return true;
}

void SQLiteDB::close() {
    QMutexLocker locker(&m_mutex);
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool SQLiteDB::isOpen() const {
    QMutexLocker locker(&m_mutex);
    if (!QSqlDatabase::contains(m_connectionName)) return false;
    return QSqlDatabase::database(m_connectionName).isOpen();
}

bool SQLiteDB::execute(const QString& sql) {
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        LOG_ERROR("SQLiteDB", "Execute failed: " + m_lastError + " SQL: " + sql);
        return false;
    }
    m_lastInsertId = q.lastInsertId().toLongLong();
    return true;
}

bool SQLiteDB::execute(const QString& sql, const QVariantList& bindings) {
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(sql);
    if (!prepareAndExec(q, bindings)) return false;
    m_lastInsertId = q.lastInsertId().toLongLong();
    return true;
}

QSqlQuery SQLiteDB::query(const QString& sql) {
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        LOG_ERROR("SQLiteDB", "Query failed: " + m_lastError);
    }
    return q;
}

QSqlQuery SQLiteDB::query(const QString& sql, const QVariantList& bindings) {
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(sql);
    prepareAndExec(q, bindings);
    return q;
}

bool SQLiteDB::beginTransaction() {
    QMutexLocker locker(&m_mutex);
    return QSqlDatabase::database(m_connectionName).transaction();
}

bool SQLiteDB::commitTransaction() {
    QMutexLocker locker(&m_mutex);
    return QSqlDatabase::database(m_connectionName).commit();
}

bool SQLiteDB::rollbackTransaction() {
    QMutexLocker locker(&m_mutex);
    return QSqlDatabase::database(m_connectionName).rollback();
}

bool SQLiteDB::transaction(std::function<bool()> fn) {
    if (!beginTransaction()) return false;
    try {
        if (fn()) {
            return commitTransaction();
        } else {
            rollbackTransaction();
            return false;
        }
    } catch (const std::exception& e) {
        rollbackTransaction();
        LOG_ERROR("SQLiteDB", QString("Transaction exception: ") + e.what());
        return false;
    }
}

qint64 SQLiteDB::lastInsertId() const {
    QMutexLocker locker(&m_mutex);
    return m_lastInsertId;
}

QString SQLiteDB::lastError() const {
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

bool SQLiteDB::tableExists(const QString& tableName) {
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    return db.tables().contains(tableName, Qt::CaseInsensitive);
}

bool SQLiteDB::createTable(const QString& sql) {
    return execute(sql);
}

void SQLiteDB::enableWALMode(QSqlDatabase& db) {
    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode = WAL");
}

bool SQLiteDB::prepareAndExec(QSqlQuery& q, const QVariantList& bindings) {
    for (int i = 0; i < bindings.size(); ++i) {
        q.bindValue(i, bindings[i]);
    }
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        LOG_ERROR("SQLiteDB", "Prepared exec failed: " + m_lastError);
        return false;
    }
    return true;
}

} // namespace Nova
