#include "SessionManager.h"
#include "Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

namespace Nova {

SessionManager& SessionManager::instance() {
    static SessionManager inst;
    return inst;
}

SessionManager::SessionManager(QObject* parent) : QObject(parent) {
    m_autoSaveTimer = std::make_unique<QTimer>(this);
    connect(m_autoSaveTimer.get(), &QTimer::timeout, this, &SessionManager::autoSave);
}

void SessionManager::initialize(const QString& sessionPath) {
    m_sessionPath = sessionPath;
    m_autoSaveTimer->start(10000); // auto-save every 10 seconds
}

void SessionManager::setAutoSaveInterval(int seconds) {
    m_autoSaveTimer->setInterval(seconds * 1000);
}

void SessionManager::saveSession(const SessionData& session) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingSession = session;
    }
    autoSave();
}

void SessionManager::autoSave() {
    SessionData session;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        session = m_pendingSession;
    }
    if (session.tabs.isEmpty()) return;

    QFile f(m_sessionPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_ERROR("SessionManager", "Cannot write session file: " + m_sessionPath);
        return;
    }

    f.write(QJsonDocument(sessionToJson(session)).toJson());
    LOG_DEBUG("SessionManager", "Session auto-saved");
    emit sessionSaved();
}

SessionData SessionManager::loadSession() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    QFile f(m_sessionPath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        LOG_ERROR("SessionManager", "Session file parse error: " + err.errorString());
        return {};
    }

    return sessionFromJson(doc.object());
}

bool SessionManager::hasSavedSession() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return QFile::exists(m_sessionPath);
}

void SessionManager::clearSession() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        QFile::remove(m_sessionPath);
        m_pendingSession = {};
    }
}

void SessionManager::onTabChanged(const SessionData& session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingSession = session;
}

QJsonObject SessionManager::sessionToJson(const SessionData& s) const {
    QJsonObject obj;
    obj["activeTabIndex"] = s.activeTabIndex;
    obj["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray tabs;
    for (const auto& tab : s.tabs) {
        QJsonObject t;
        t["url"]    = tab.url;
        t["title"]  = tab.title;
        t["pinned"] = tab.pinned;
        t["index"]  = tab.index;
        tabs.append(t);
    }
    obj["tabs"] = tabs;
    return obj;
}

SessionData SessionManager::sessionFromJson(const QJsonObject& obj) const {
    SessionData s;
    s.activeTabIndex = obj["activeTabIndex"].toInt(0);
    s.savedAt = QDateTime::fromString(obj["savedAt"].toString(), Qt::ISODate);

    for (const auto& t : obj["tabs"].toArray()) {
        auto tab = t.toObject();
        TabSessionData td;
        td.url    = tab["url"].toString();
        td.title  = tab["title"].toString();
        td.pinned = tab["pinned"].toBool(false);
        td.index  = tab["index"].toInt();
        s.tabs.append(td);
    }
    return s;
}

} // namespace Nova
