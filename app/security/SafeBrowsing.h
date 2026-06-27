#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QSet>
#include <QTimer>
#include <QNetworkAccessManager>
#include <mutex>
#include <functional>

namespace Nova {

enum class ThreatType {
    None,
    Malware,
    Phishing,
    UnwantedSoftware,
    HarmfulApplication
};

struct ThreatInfo {
    ThreatType type{ThreatType::None};
    QString    description;
};

class SafeBrowsing : public QObject {
    Q_OBJECT

public:
    static SafeBrowsing& instance();

    void initialize(const QString& dbPath);
    void enable(bool enabled);
    bool isEnabled() const;

    void checkUrl(const QUrl& url, std::function<void(const ThreatInfo&)> callback);

    void addToBlocklist(const QString& host);
    void removeFromBlocklist(const QString& host);

signals:
    void threatDetected(const QUrl& url, const ThreatInfo& info);

private:
    explicit SafeBrowsing(QObject* parent = nullptr);
    ~SafeBrowsing() override = default;

    SafeBrowsing(const SafeBrowsing&) = delete;
    SafeBrowsing& operator=(const SafeBrowsing&) = delete;

    bool isInLocalBlocklist(const QString& host) const;
    void loadBlocklist(const QString& path);

    bool m_enabled{true};
    QSet<QString> m_blocklist;
    mutable std::mutex m_mutex;
    QString m_dbPath;
};

} // namespace Nova
