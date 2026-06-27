#include "ProxyManager.h"
#include "Logger.h"
#include <QNetworkProxyFactory>

namespace Nova {

ProxyManager& ProxyManager::instance() { static ProxyManager i; return i; }
ProxyManager::ProxyManager(QObject* parent) : QObject(parent) { m_config.type = ProxyType::System; }

void ProxyManager::apply(const ProxyConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex); m_config = config;
    if (config.type == ProxyType::System) { QNetworkProxyFactory::setUseSystemConfiguration(true); return; }
    QNetworkProxy proxy;
    switch (config.type) {
        case ProxyType::None:  proxy.setType(QNetworkProxy::NoProxy);   break;
        case ProxyType::HTTP:  proxy.setType(QNetworkProxy::HttpProxy); break;
        case ProxyType::SOCKS5:proxy.setType(QNetworkProxy::Socks5Proxy);break;
        default: break;
    }
    proxy.setHostName(config.host); proxy.setPort(config.port);
    if (!config.username.isEmpty()) { proxy.setUser(config.username); proxy.setPassword(config.password); }
    QNetworkProxy::setApplicationProxy(proxy);
}

ProxyConfig ProxyManager::currentConfig() const { std::lock_guard<std::mutex> lock(m_mutex); return m_config; }

void ProxyManager::resetToSystem() { ProxyConfig c; c.type=ProxyType::System; apply(c); }

} // namespace Nova
