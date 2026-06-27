#pragma once

#include <QObject>
#include <QString>
#include <QWebEngineProfile>
#include <mutex>

namespace Nova {

class CacheManager : public QObject {
    Q_OBJECT

public:
    static CacheManager& instance();

    void initialize(const QString& cachePath, int maxSizeMB = 512);

    void applyToProfile(QWebEngineProfile* profile);

    void clearMemoryCache();
    void clearDiskCache();
    void clearAll();

    QString cachePath() const;
    qint64 diskCacheSize() const;

private:
    explicit CacheManager(QObject* parent = nullptr);
    ~CacheManager() override = default;

    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    QString m_cachePath;
    int m_maxSizeMB{512};
    mutable std::mutex m_mutex;
};

} // namespace Nova
