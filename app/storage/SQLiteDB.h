#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QVector>
#include <functional>
#include <optional>

namespace Nova {

class SQLiteDB : public QObject {
    Q_OBJECT

public:
    explicit SQLiteDB(const QString& connectionName, QObject* parent = nullptr);
    ~SQLiteDB() override;

    bool open(const QString& databasePath);
    void close();
    bool isOpen() const;

    bool execute(const QString& sql);
    bool execute(const QString& sql, const QVariantList& bindings);

    QSqlQuery query(const QString& sql);
    QSqlQuery query(const QString& sql, const QVariantList& bindings);

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    bool transaction(std::function<bool()> fn);

    qint64 lastInsertId() const;
    QString lastError() const;

    bool tableExists(const QString& tableName);
    bool createTable(const QString& sql);

    static void enableWALMode(QSqlDatabase& db);

private:
    QString m_connectionName;
    mutable QMutex m_mutex;
    QString m_lastError;
    qint64 m_lastInsertId{-1};

    bool prepareAndExec(QSqlQuery& q, const QVariantList& bindings);
};

} // namespace Nova
