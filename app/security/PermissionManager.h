#pragma once
#include <QObject>
#include <QUrl>
#include <QMap>
#include <mutex>

namespace Nova {

enum class PermissionType { Geolocation, Microphone, Camera, Notifications, MediaCapture, DesktopCapture };
enum class PermissionState { Ask, Granted, Denied };

class PermissionManager : public QObject {
    Q_OBJECT
public:
    static PermissionManager& instance();
    PermissionState getPermission(const QUrl& origin, PermissionType type) const;
    void setPermission(const QUrl& origin, PermissionType type, PermissionState state);
    void clearAll();
    void clearForOrigin(const QUrl& origin);
    QMap<QString, QMap<int, PermissionState>> allPermissions() const;
signals:
    void permissionRequested(const QUrl& url, PermissionType type);
    void permissionsChanged();
private:
    explicit PermissionManager(QObject* parent = nullptr);
    QString originKey(const QUrl& url) const;
    QMap<QString, QMap<int, PermissionState>> m_permissions;
    mutable std::mutex m_mutex;
};

} // namespace Nova
