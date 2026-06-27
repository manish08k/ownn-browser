#include "HttpClient.h"
#include "Logger.h"
#include <QTimer>
#include <QSslConfiguration>

namespace Nova {

HttpClient& HttpClient::instance() { static HttpClient i; return i; }

HttpClient::HttpClient(QObject* parent)
    : QObject(parent), m_nam(std::make_unique<QNetworkAccessManager>(this)), m_userAgent("NovaBrowser/1.0")
{
    m_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

void HttpClient::get(const QUrl& url, const QMap<QString,QString>& headers, HttpCallback callback) {
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    applyHeaders(req, headers);
    auto ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyPeer);
    req.setSslConfiguration(ssl);
    auto* reply = m_nam->get(req);
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true); timer->start(m_timeout);
    connect(timer, &QTimer::timeout, reply, [reply]{ reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]{ handleReply(reply, callback); });
}

void HttpClient::post(const QUrl& url, const QByteArray& body, const QMap<QString,QString>& headers, HttpCallback callback) {
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    applyHeaders(req, headers);
    auto* reply = m_nam->post(req, body);
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true); timer->start(m_timeout);
    connect(timer, &QTimer::timeout, reply, [reply]{ reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]{ handleReply(reply, callback); });
}

void HttpClient::setUserAgent(const QString& ua) { m_userAgent = ua; }
void HttpClient::setTimeout(int ms) { m_timeout = ms; }

void HttpClient::handleReply(QNetworkReply* reply, HttpCallback callback) {
    reply->deleteLater();
    const bool ok = (reply->error() == QNetworkReply::NoError);
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = ok ? reply->readAll() : QByteArray();
    QMap<QString,QString> hdrs;
    for (const auto& p : reply->rawHeaderPairs()) hdrs[QString::fromUtf8(p.first)] = QString::fromUtf8(p.second);
    if (!ok) LOG_ERROR("HttpClient", "Request failed: " + reply->errorString());
    callback(ok, status, body, hdrs);
}

void HttpClient::applyHeaders(QNetworkRequest& req, const QMap<QString,QString>& headers) {
    for (auto it = headers.cbegin(); it != headers.cend(); ++it)
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
}

} // namespace Nova
