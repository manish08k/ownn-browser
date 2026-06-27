#pragma once

#include <QObject>
#include <QString>
#include <QSslCertificate>
#include <QSslError>
#include <QVector>
#include <QUrl>
#include <mutex>

namespace Nova {

enum class CertificateStatus {
    Valid,
    SelfSigned,
    Expired,
    HostnameMismatch,
    Untrusted,
    Revoked,
    Unknown
};

struct CertificateInfo {
    QString subjectCommonName;
    QString issuerCommonName;
    QDateTime effectiveDate;
    QDateTime expiryDate;
    QString serialNumber;
    QString fingerprint;
    CertificateStatus status{CertificateStatus::Unknown};
    QVector<QString> subjectAlternativeNames;
    bool isEV{false};
};

class CertificateManager : public QObject {
    Q_OBJECT

public:
    static CertificateManager& instance();

    CertificateInfo inspectCertificate(const QSslCertificate& cert) const;
    CertificateStatus evaluateErrors(const QVector<QSslError>& errors) const;

    // User-accepted exceptions
    void acceptCertificateException(const QString& host, const QSslCertificate& cert);
    bool hasAcceptedException(const QString& host) const;
    void clearExceptions();
    void removeException(const QString& host);

    bool isMixedContent(const QUrl& pageUrl, const QUrl& resourceUrl) const;

signals:
    void certificateError(const QString& host, const QVector<QSslError>& errors);

private:
    explicit CertificateManager(QObject* parent = nullptr);
    ~CertificateManager() override = default;

    CertificateManager(const CertificateManager&) = delete;
    CertificateManager& operator=(const CertificateManager&) = delete;

    mutable std::mutex m_mutex;
    QMap<QString, QSslCertificate> m_exceptions;
};

} // namespace Nova
