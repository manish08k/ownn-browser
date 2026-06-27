#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <mutex>

namespace Nova {

struct CookieEntry {
    QString domain;
    QString name;
    QString value;
    QString path;
    QDateTime expiryDate;
    bool isSecure{false};
    bool isHttpOnly{false};
    bool isSessionCookie{false};
};

class CookieManager : public QObject {
    Q_OBJECT

public:
    static CookieManager& instance();

    void initialize(QWebEngineProfile* profile);

    QVector<CookieEntry> allCookies() const;
    QVector<CookieEntry> cookiesForDomain(const QString& domain) const;

    void deleteCookie(const QString& domain, const QString& name);
    void deleteCookiesForDomain(const QString& domain);
    void deleteAllCookies();

    void setThirdPartyBlocking(bool block);

signals:
    void cookiesChanged();

private:
    explicit CookieManager(QObject* parent = nullptr);
    ~CookieManager() override = default;

    CookieManager(const CookieManager&) = delete;
    CookieManager& operator=(const CookieManager&) = delete;

    CookieEntry fromNetworkCookie(const QNetworkCookie& c) const;

    QWebEngineCookieStore* m_store{nullptr};
    mutable std::mutex m_mutex;
    QVector<QNetworkCookie> m_cookies;
};

} // namespace Nova
