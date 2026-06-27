#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QByteArray>
#include <QMap>
#include <functional>
#include <memory>

namespace Nova {

using HttpCallback = std::function<void(bool ok, int statusCode,
                                        const QByteArray& body,
                                        const QMap<QString,QString>& headers)>;

class HttpClient : public QObject {
    Q_OBJECT

public:
    static HttpClient& instance();

    void get(const QUrl& url, const QMap<QString,QString>& headers,
             HttpCallback callback);

    void post(const QUrl& url, const QByteArray& body,
              const QMap<QString,QString>& headers, HttpCallback callback);

    void setUserAgent(const QString& ua);
    void setTimeout(int ms);

private:
    explicit HttpClient(QObject* parent = nullptr);
    ~HttpClient() override = default;

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    void handleReply(QNetworkReply* reply, HttpCallback callback);
    void applyHeaders(QNetworkRequest& req, const QMap<QString,QString>& headers);

    std::unique_ptr<QNetworkAccessManager> m_nam;
    QString m_userAgent;
    int m_timeout{15000};
};

} // namespace Nova
