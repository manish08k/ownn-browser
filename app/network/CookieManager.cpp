#include "CookieManager.h"
#include "Logger.h"

namespace Nova {

CookieManager& CookieManager::instance() {
    static CookieManager inst;
    return inst;
}

CookieManager::CookieManager(QObject* parent) : QObject(parent) {}

void CookieManager::initialize(QWebEngineProfile* profile) {
    if (!profile) return;
    m_store = profile->cookieStore();
    if (!m_store) return;

    m_store->loadAllCookies();

    connect(m_store, &QWebEngineCookieStore::cookieAdded, this,
        [this](const QNetworkCookie& cookie) {
            std::lock_guard<std::mutex> lock(m_mutex);
            // Remove old cookie with same domain+name first
            m_cookies.erase(
                std::remove_if(m_cookies.begin(), m_cookies.end(),
                    [&cookie](const QNetworkCookie& c) {
                        return c.domain() == cookie.domain() && c.name() == cookie.name();
                    }),
                m_cookies.end()
            );
            m_cookies.append(cookie);
            emit cookiesChanged();
        }
    );

    connect(m_store, &QWebEngineCookieStore::cookieRemoved, this,
        [this](const QNetworkCookie& cookie) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cookies.erase(
                std::remove_if(m_cookies.begin(), m_cookies.end(),
                    [&cookie](const QNetworkCookie& c) {
                        return c.domain() == cookie.domain() && c.name() == cookie.name();
                    }),
                m_cookies.end()
            );
            emit cookiesChanged();
        }
    );

    LOG_INFO("CookieManager", "Initialized with profile cookie store");
}

QVector<CookieEntry> CookieManager::allCookies() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    QVector<CookieEntry> result;
    for (const auto& c : m_cookies) {
        result.append(fromNetworkCookie(c));
    }
    return result;
}

QVector<CookieEntry> CookieManager::cookiesForDomain(const QString& domain) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    QVector<CookieEntry> result;
    for (const auto& c : m_cookies) {
        if (c.domain().contains(domain, Qt::CaseInsensitive)) {
            result.append(fromNetworkCookie(c));
        }
    }
    return result;
}

void CookieManager::deleteCookie(const QString& domain, const QString& name) {
    if (!m_store) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& c : m_cookies) {
        if (c.domain() == domain && c.name() == name.toUtf8()) {
            m_store->deleteCookie(c);
        }
    }
}

void CookieManager::deleteCookiesForDomain(const QString& domain) {
    if (!m_store) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& c : m_cookies) {
        if (c.domain().contains(domain, Qt::CaseInsensitive)) {
            m_store->deleteCookie(c);
        }
    }
}

void CookieManager::deleteAllCookies() {
    if (!m_store) return;
    m_store->deleteAllCookies();
    LOG_INFO("CookieManager", "All cookies deleted");
}

void CookieManager::setThirdPartyBlocking(bool block) {
    if (!m_store) return;
    QWebEngineCookieStore::FilterRequest filter;
    // Allow all or block third-party based on setting
    m_store->setCookieFilter([block](const QWebEngineCookieStore::FilterRequest& req) {
        if (block && !req.thirdParty) return true;
        if (block && req.thirdParty) return false;
        return true;
    });
    LOG_INFO("CookieManager", QString("Third-party cookie blocking: %1").arg(block ? "enabled" : "disabled"));
}

CookieEntry CookieManager::fromNetworkCookie(const QNetworkCookie& c) const {
    CookieEntry e;
    e.domain         = c.domain();
    e.name           = QString::fromUtf8(c.name());
    e.value          = QString::fromUtf8(c.value());
    e.path           = c.path();
    e.expiryDate     = c.expirationDate();
    e.isSecure       = c.isSecure();
    e.isHttpOnly     = c.isHttpOnly();
    e.isSessionCookie = c.isSessionCookie();
    return e;
}

} // namespace Nova
