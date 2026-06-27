#include "CacheManager.h"
#include "Logger.h"
#include <QDir>
#include <QDirIterator>

namespace Nova {

CacheManager& CacheManager::instance() { static CacheManager i; return i; }
CacheManager::CacheManager(QObject* parent) : QObject(parent) {}

void CacheManager::initialize(const QString& cachePath, int maxSizeMB) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cachePath = cachePath; m_maxSizeMB = maxSizeMB;
    QDir().mkpath(cachePath);
}

void CacheManager::applyToProfile(QWebEngineProfile* profile) {
    if (!profile) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(m_maxSizeMB * 1024 * 1024);
    profile->setCachePath(m_cachePath);
}

void CacheManager::clearMemoryCache() {}

void CacheManager::clearDiskCache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    QDir dir(m_cachePath);
    if (dir.exists()) { dir.removeRecursively(); dir.mkpath("."); }
}

void CacheManager::clearAll() { clearMemoryCache(); clearDiskCache(); }
QString CacheManager::cachePath() const { std::lock_guard<std::mutex> lock(m_mutex); return m_cachePath; }

qint64 CacheManager::diskCacheSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    qint64 total = 0;
    QDirIterator it(m_cachePath, QDirIterator::Subdirectories);
    while (it.hasNext()) { it.next(); total += it.fileInfo().size(); }
    return total;
}

} // namespace Nova
