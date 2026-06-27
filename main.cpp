#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QWebEngineView>
#include <QMessageBox>
#include "BrowserWindow.h"
#include "BrowserEngine.h"
#include "SettingsManager.h"
#include "HistoryManager.h"
#include "BookmarkManager.h"
#include "SessionManager.h"
#include "CacheManager.h"
#include "DownloadManager.h"
#include "SafeBrowsing.h"
#include "ProxyManager.h"
#include "Logger.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("NovaBrowser");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Nova");
    app.setOrganizationDomain("novabrowser.local");

    const QString dataRoot    = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataRoot);

    Nova::Logger::instance().setLogFile(dataRoot + "/nova.log");
    Nova::Logger::instance().setMinLevel(Nova::LogLevel::Info);

    Nova::SettingsManager::instance().initialize(dataRoot + "/settings.ini");
    Nova::CacheManager::instance().initialize(dataRoot + "/cache", Nova::SettingsManager::instance().maxCacheSizeMB());

    if (!Nova::HistoryManager::instance().initialize(dataRoot + "/nova.db")) {
        QMessageBox::critical(nullptr, "Error", "Failed to open history database.");
        return 1;
    }
    if (!Nova::BookmarkManager::instance().initialize(dataRoot + "/bookmarks.db")) {
        QMessageBox::critical(nullptr, "Error", "Failed to open bookmark database.");
        return 1;
    }

    Nova::DownloadManager::instance().initialize(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    Nova::SessionManager::instance().initialize(dataRoot + "/session.json");
    Nova::SafeBrowsing::instance().initialize(dataRoot + "/blocklist.txt");
    Nova::ProxyManager::instance().resetToSystem();
    Nova::BrowserEngine::instance().initialize(dataRoot);

    Nova::BrowserWindow window;
    window.show();

    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QUrl url = QUrl::fromUserInput(args[i]);
        if (url.isValid()) window.openUrl(url);
    }

    const int ret = app.exec();
    Nova::SettingsManager::instance().sync();
    return ret;
}
