#pragma once

#include <QObject>
#include <QNetworkProxy>
#include <QString>
#include <mutex>

namespace Nova {

enum class ProxyType {
    None,
    System,
    HTTP,
    SOCKS5
};

struct ProxyConfig {
    ProxyType type{ProxyType::System};
    QString   host;
    quint16   port{0};
    QString   username;
    QString   password;
};

class ProxyManager : public QObject {
    Q_OBJECT

public:
    static ProxyManager& instance();

    void apply(const ProxyConfig& config);
    ProxyConfig currentConfig() const;
    void resetToSystem();

private:
    explicit ProxyManager(QObject* parent = nullptr);
    ~ProxyManager() override = default;

    ProxyManager(const ProxyManager&) = delete;
    ProxyManager& operator=(const ProxyManager&) = delete;

    ProxyConfig m_config;
    mutable std::mutex m_mutex;
};

} // namespace Nova
