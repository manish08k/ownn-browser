#include "SafeBrowsing.h"
#include "Logger.h"
#include <QFile>
#include <QTextStream>

namespace Nova {

SafeBrowsing& SafeBrowsing::instance() { static SafeBrowsing i; return i; }
SafeBrowsing::SafeBrowsing(QObject* parent) : QObject(parent) {}

void SafeBrowsing::initialize(const QString& dbPath) {
    m_dbPath = dbPath; loadBlocklist(dbPath);
}

void SafeBrowsing::enable(bool e) { std::lock_guard<std::mutex> lock(m_mutex); m_enabled = e; }
bool SafeBrowsing::isEnabled() const { std::lock_guard<std::mutex> lock(m_mutex); return m_enabled; }

void SafeBrowsing::checkUrl(const QUrl& url, std::function<void(const ThreatInfo&)> callback) {
    if (!isEnabled()) { callback({}); return; }
    const QString host = url.host().toLower();
    if (isInLocalBlocklist(host)) {
        ThreatInfo info; info.type=ThreatType::Malware; info.description="Site is on the local blocklist.";
        emit threatDetected(url, info); callback(info); return;
    }
    callback({});
}

void SafeBrowsing::addToBlocklist(const QString& host) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blocklist.insert(host.toLower());
    QFile f(m_dbPath);
    if (f.open(QIODevice::Append|QIODevice::Text)) { QTextStream s(&f); s << host.toLower() << "\n"; }
}

void SafeBrowsing::removeFromBlocklist(const QString& host) {
    std::lock_guard<std::mutex> lock(m_mutex); m_blocklist.remove(host.toLower());
}

bool SafeBrowsing::isInLocalBlocklist(const QString& host) const {
    std::lock_guard<std::mutex> lock(m_mutex); return m_blocklist.contains(host);
}

void SafeBrowsing::loadBlocklist(const QString& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    QFile f(path); if (!f.open(QIODevice::ReadOnly|QIODevice::Text)) return;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith('#')) m_blocklist.insert(line.toLower());
    }
}

} // namespace Nova
