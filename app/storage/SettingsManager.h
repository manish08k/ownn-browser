#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QStringList>
#include <memory>
#include <mutex>

namespace Nova {

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager& instance();

    void initialize(const QString& configPath);

    // Homepage
    QString homepage() const;
    void setHomepage(const QString& url);

    // Search engine
    QString defaultSearchEngine() const;
    void setDefaultSearchEngine(const QString& name);
    QString searchUrl(const QString& query) const;
    QStringList availableSearchEngines() const;

    // Downloads
    QString downloadPath() const;
    void setDownloadPath(const QString& path);

    // Privacy
    bool saveCookies() const;
    void setSaveCookies(bool save);

    bool saveHistory() const;
    void setSaveHistory(bool save);

    bool blockThirdPartyCookies() const;
    void setBlockThirdPartyCookies(bool block);

    bool enableDoNotTrack() const;
    void setEnableDoNotTrack(bool enable);

    // Cache
    int maxCacheSizeMB() const;
    void setMaxCacheSizeMB(int mb);

    // Sessions
    bool restoreSession() const;
    void setRestoreSession(bool restore);

    // Dark mode
    bool darkMode() const;
    void setDarkMode(bool dark);

    // JavaScript
    bool javascriptEnabled() const;
    void setJavascriptEnabled(bool enabled);

    // Popup blocking
    bool blockPopups() const;
    void setBlockPopups(bool block);

    // Generic
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);
    void sync();

signals:
    void settingChanged(const QString& key, const QVariant& value);

private:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override = default;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    std::unique_ptr<QSettings> m_settings;
    mutable std::mutex m_mutex;

    QMap<QString, QString> m_searchEngines;
    void initSearchEngines();
};

} // namespace Nova
