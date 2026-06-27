#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QUrl>

namespace Nova {

SettingsManager& SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager(QObject* parent) : QObject(parent) {
    initSearchEngines();
}

void SettingsManager::initialize(const QString& configPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_settings = std::make_unique<QSettings>(configPath, QSettings::IniFormat);
}

void SettingsManager::initSearchEngines() {
    m_searchEngines["Google"]     = "https://www.google.com/search?q=%1";
    m_searchEngines["DuckDuckGo"] = "https://duckduckgo.com/?q=%1";
    m_searchEngines["Bing"]       = "https://www.bing.com/search?q=%1";
    m_searchEngines["Wikipedia"]  = "https://en.wikipedia.org/wiki/Special:Search?search=%1";
    m_searchEngines["GitHub"]     = "https://github.com/search?q=%1";
    m_searchEngines["YouTube"]    = "https://www.youtube.com/results?search_query=%1";
}

QString SettingsManager::homepage() const {
    return value("browser/homepage", "https://www.google.com").toString();
}

void SettingsManager::setHomepage(const QString& url) {
    setValue("browser/homepage", url);
}

QString SettingsManager::defaultSearchEngine() const {
    return value("browser/search_engine", "Google").toString();
}

void SettingsManager::setDefaultSearchEngine(const QString& name) {
    setValue("browser/search_engine", name);
}

QString SettingsManager::searchUrl(const QString& query) const {
    const QString engine = defaultSearchEngine();
    const QString urlTemplate = m_searchEngines.value(engine, m_searchEngines["Google"]);
    return urlTemplate.arg(QString::fromUtf8(QUrl::toPercentEncoding(query)));
}

QStringList SettingsManager::availableSearchEngines() const {
    return m_searchEngines.keys();
}

QString SettingsManager::downloadPath() const {
    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return value("browser/download_path", defaultPath).toString();
}

void SettingsManager::setDownloadPath(const QString& path) {
    setValue("browser/download_path", path);
}

bool SettingsManager::saveCookies() const {
    return value("privacy/save_cookies", true).toBool();
}

void SettingsManager::setSaveCookies(bool save) {
    setValue("privacy/save_cookies", save);
}

bool SettingsManager::saveHistory() const {
    return value("privacy/save_history", true).toBool();
}

void SettingsManager::setSaveHistory(bool save) {
    setValue("privacy/save_history", save);
}

bool SettingsManager::blockThirdPartyCookies() const {
    return value("privacy/block_third_party_cookies", false).toBool();
}

void SettingsManager::setBlockThirdPartyCookies(bool block) {
    setValue("privacy/block_third_party_cookies", block);
}

bool SettingsManager::enableDoNotTrack() const {
    return value("privacy/do_not_track", false).toBool();
}

void SettingsManager::setEnableDoNotTrack(bool enable) {
    setValue("privacy/do_not_track", enable);
}

int SettingsManager::maxCacheSizeMB() const {
    return value("cache/max_size_mb", 512).toInt();
}

void SettingsManager::setMaxCacheSizeMB(int mb) {
    setValue("cache/max_size_mb", mb);
}

bool SettingsManager::restoreSession() const {
    return value("session/restore", true).toBool();
}

void SettingsManager::setRestoreSession(bool restore) {
    setValue("session/restore", restore);
}

bool SettingsManager::darkMode() const {
    return value("ui/dark_mode", false).toBool();
}

void SettingsManager::setDarkMode(bool dark) {
    setValue("ui/dark_mode", dark);
}

bool SettingsManager::javascriptEnabled() const {
    return value("browser/javascript_enabled", true).toBool();
}

void SettingsManager::setJavascriptEnabled(bool enabled) {
    setValue("browser/javascript_enabled", enabled);
}

bool SettingsManager::blockPopups() const {
    return value("browser/block_popups", true).toBool();
}

void SettingsManager::setBlockPopups(bool block) {
    setValue("browser/block_popups", block);
}

QVariant SettingsManager::value(const QString& key, const QVariant& defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_settings) return defaultValue;
    return m_settings->value(key, defaultValue);
}

void SettingsManager::setValue(const QString& key, const QVariant& val) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_settings) return;
        m_settings->setValue(key, val);
    }
    emit settingChanged(key, val);
}

void SettingsManager::sync() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_settings) m_settings->sync();
}

} // namespace Nova
