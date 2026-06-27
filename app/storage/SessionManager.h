#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QTimer>
#include <QJsonObject>
#include <memory>
#include <mutex>

namespace Nova {

struct TabSessionData {
    QString url;
    QString title;
    bool pinned{false};
    int index{0};
};

struct SessionData {
    QVector<TabSessionData> tabs;
    int activeTabIndex{0};
    QDateTime savedAt;
};

class SessionManager : public QObject {
    Q_OBJECT

public:
    static SessionManager& instance();

    void initialize(const QString& sessionPath);
    void setAutoSaveInterval(int seconds);

    void saveSession(const SessionData& session);
    SessionData loadSession() const;
    bool hasSavedSession() const;
    void clearSession();

    // Crash recovery - saves session on every change
    void onTabChanged(const SessionData& session);

public slots:
    void autoSave();

signals:
    void sessionSaved();
    void sessionLoaded(const SessionData& session);

private:
    explicit SessionManager(QObject* parent = nullptr);
    ~SessionManager() override = default;

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    QJsonObject sessionToJson(const SessionData& s) const;
    SessionData sessionFromJson(const QJsonObject& obj) const;

    QString m_sessionPath;
    std::unique_ptr<QTimer> m_autoSaveTimer;
    SessionData m_pendingSession;
    mutable std::mutex m_mutex;
};

} // namespace Nova
