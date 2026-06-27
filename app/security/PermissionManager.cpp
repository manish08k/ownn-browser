#include "PermissionManager.h"
#include "Logger.h"

namespace Nova {

PermissionManager& PermissionManager::instance() { static PermissionManager i; return i; }
PermissionManager::PermissionManager(QObject* parent) : QObject(parent) {}

PermissionState PermissionManager::getPermission(const QUrl& origin, PermissionType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const QString key = originKey(origin);
    if (m_permissions.contains(key)) {
        const auto& map = m_permissions[key];
        if (map.contains(static_cast<int>(type))) return map[static_cast<int>(type)];
    }
    return PermissionState::Ask;
}

void PermissionManager::setPermission(const QUrl& origin, PermissionType type, PermissionState state) {
    { std::lock_guard<std::mutex> lock(m_mutex); m_permissions[originKey(origin)][static_cast<int>(type)] = state; }
    emit permissionsChanged();
}

void PermissionManager::clearAll() {
    { std::lock_guard<std::mutex> lock(m_mutex); m_permissions.clear(); } emit permissionsChanged();
}

void PermissionManager::clearForOrigin(const QUrl& origin) {
    { std::lock_guard<std::mutex> lock(m_mutex); m_permissions.remove(originKey(origin)); } emit permissionsChanged();
}

QMap<QString, QMap<int, PermissionState>> PermissionManager::allPermissions() const {
    std::lock_guard<std::mutex> lock(m_mutex); return m_permissions;
}

QString PermissionManager::originKey(const QUrl& url) const { return url.scheme()+"://"+url.host(); }

} // namespace Nova
