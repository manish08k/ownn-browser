#include "CertificateManager.h"
#include "Logger.h"
#include <QCryptographicHash>

namespace Nova {

CertificateManager& CertificateManager::instance() { static CertificateManager i; return i; }
CertificateManager::CertificateManager(QObject* parent) : QObject(parent) {}

CertificateInfo CertificateManager::inspectCertificate(const QSslCertificate& cert) const {
    CertificateInfo info;
    if (cert.isNull()) return info;
    info.subjectCommonName = cert.subjectInfo(QSslCertificate::CommonName).join(", ");
    info.issuerCommonName  = cert.issuerInfo(QSslCertificate::CommonName).join(", ");
    info.effectiveDate     = cert.effectiveDate();
    info.expiryDate        = cert.expiryDate();
    info.serialNumber      = cert.serialNumber().toHex();
    info.fingerprint       = cert.digest(QCryptographicHash::Sha256).toHex();
    for (const auto& name : cert.subjectAlternativeNames()) info.subjectAlternativeNames.append(name);
    if (cert.expiryDate() < QDateTime::currentDateTime()) info.status = CertificateStatus::Expired;
    else if (cert.issuerInfo(QSslCertificate::CommonName) == cert.subjectInfo(QSslCertificate::CommonName)) info.status = CertificateStatus::SelfSigned;
    else info.status = CertificateStatus::Valid;
    return info;
}

CertificateStatus CertificateManager::evaluateErrors(const QVector<QSslError>& errors) const {
    if (errors.isEmpty()) return CertificateStatus::Valid;
    for (const auto& e : errors) {
        switch (e.error()) {
            case QSslError::SelfSignedCertificate:
            case QSslError::SelfSignedCertificateInChain: return CertificateStatus::SelfSigned;
            case QSslError::CertificateExpired:           return CertificateStatus::Expired;
            case QSslError::HostNameMismatch:             return CertificateStatus::HostnameMismatch;
            case QSslError::CertificateRevoked:           return CertificateStatus::Revoked;
            default:                                       return CertificateStatus::Untrusted;
        }
    }
    return CertificateStatus::Unknown;
}

void CertificateManager::acceptCertificateException(const QString& host, const QSslCertificate& cert) {
    std::lock_guard<std::mutex> lock(m_mutex); m_exceptions[host] = cert;
}
bool CertificateManager::hasAcceptedException(const QString& host) const {
    std::lock_guard<std::mutex> lock(m_mutex); return m_exceptions.contains(host);
}
void CertificateManager::clearExceptions() { std::lock_guard<std::mutex> lock(m_mutex); m_exceptions.clear(); }
void CertificateManager::removeException(const QString& host) { std::lock_guard<std::mutex> lock(m_mutex); m_exceptions.remove(host); }
bool CertificateManager::isMixedContent(const QUrl& pageUrl, const QUrl& resourceUrl) const {
    return pageUrl.scheme().toLower()=="https" && resourceUrl.scheme().toLower()=="http";
}

} // namespace Nova
