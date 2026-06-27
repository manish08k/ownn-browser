#include "BrowserEngine.h"
#include "Logger.h"
#include "SettingsManager.h"
#include "CacheManager.h"
#include "CookieManager.h"
#include "DownloadManager.h"
#include "HistoryManager.h"
#include <QWebEngineSettings>
#include <QDir>

namespace Nova {

BrowserEngine& BrowserEngine::instance() {
    static BrowserEngine inst;
    return inst;
}

BrowserEngine::BrowserEngine(QObject* parent) : QObject(parent) {}

void BrowserEngine::initialize(const QString& dataPath) {
    m_dataPath = dataPath;
    QDir().mkpath(dataPath);

    // Default (persistent) profile
    m_defaultProfile = std::make_unique<QWebEngineProfile>("NovaBrowser", this);
    setupProfile(m_defaultProfile.get(), dataPath);
    injectScripts(m_defaultProfile.get());

    // Private (off-the-record) profile
    m_privateProfile = std::make_unique<QWebEngineProfile>(this);
    setupProfile(m_privateProfile.get(), {});

    // Initialize subsystems
    CacheManager::instance().applyToProfile(m_defaultProfile.get());
    CookieManager::instance().initialize(m_defaultProfile.get());

    connect(m_defaultProfile.get(), &QWebEngineProfile::downloadRequested, this,
        [](QWebEngineDownloadRequest* dl) {
            DownloadManager::instance().handleDownload(dl);
        }
    );

    LOG_INFO("BrowserEngine", "Initialized with data path: " + dataPath);
}

QWebEngineProfile* BrowserEngine::defaultProfile() const {
    return m_defaultProfile.get();
}

QWebEngineProfile* BrowserEngine::privateProfile() const {
    return m_privateProfile.get();
}

void BrowserEngine::setupProfile(QWebEngineProfile* profile, const QString& dataPath) {
    if (!profile) return;

    profile->setHttpUserAgent(
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) NovaBrowser/1.0 Chrome/120.0.0.0 Safari/537.36"
    );

    if (!dataPath.isEmpty()) {
        profile->setPersistentStoragePath(dataPath + "/storage");
        profile->setCachePath(dataPath + "/cache");
        profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
        profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    } else {
        profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
        profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    }

    profile->setHttpCacheMaximumSize(
        SettingsManager::instance().maxCacheSizeMB() * 1024 * 1024
    );

    if (SettingsManager::instance().enableDoNotTrack()) {
        profile->setHttpAcceptLanguage("en-US,en;q=0.9");
    }

    auto* settings = profile->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled,
                           SettingsManager::instance().javascriptEnabled());
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, !dataPath.isEmpty());
    settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
    settings->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
}

void BrowserEngine::injectScripts(QWebEngineProfile* profile) {
    // Inject a minimal script to expose Nova browser API to pages if needed
    // Currently a placeholder for future extension scripting
}

void BrowserEngine::applySettings() {
    setupProfile(m_defaultProfile.get(), m_dataPath);
    CookieManager::instance().setThirdPartyBlocking(
        SettingsManager::instance().blockThirdPartyCookies()
    );
}

void BrowserEngine::clearBrowsingData(bool history, bool cookies, bool cache, bool) {
    if (history) HistoryManager::instance().clearAll();
    if (cookies) CookieManager::instance().deleteAllCookies();
    if (cache)   CacheManager::instance().clearAll();
    LOG_INFO("BrowserEngine", "Browsing data cleared");
}

} // namespace Nova
