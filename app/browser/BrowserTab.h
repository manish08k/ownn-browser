#pragma once
#include <QObject>
#include <QString>
#include <QUrl>
#include <QIcon>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineHistory>
#include "PermissionManager.h"
#include "ReaderMode.h"
#include <memory>

namespace Nova {

class BrowserTab : public QObject {
    Q_OBJECT
public:
    explicit BrowserTab(QWebEngineProfile* profile, QObject* parent = nullptr);
    ~BrowserTab() override;

    QWebEngineView* view() const;
    QWebEnginePage* page() const;
    QString title() const;
    QUrl url() const;
    QIcon favicon() const;
    bool isPinned() const;
    void setPinned(bool pinned);
    bool isSuspended() const;
    void suspend();
    void unsuspend();
    bool isLoading() const;
    int loadProgress() const;
    void navigate(const QUrl& url);
    void reload();
    void stop();
    void back();
    void forward();
    bool canGoBack() const;
    bool canGoForward() const;
    void openDevTools();
    void inspectElement();
    void viewPageSource();
    void findText(const QString& text, bool caseSensitive = false, bool backward = false);
    void clearFindText();
    void setZoomFactor(double factor);
    double zoomFactor() const;
    void enableReaderMode();
    void disableReaderMode();
    bool isInReaderMode() const;
    qint64 id() const;
    QUrl suspendedUrl() const;

signals:
    void titleChanged(const QString& title);
    void urlChanged(const QUrl& url);
    void faviconChanged(const QIcon& icon);
    void loadStarted();
    void loadFinished(bool ok);
    void loadProgress(int progress);
    void readerModeAvailable(bool available);
    void pinnedChanged(bool pinned);
    void suspendedChanged(bool suspended);
    void iconChanged(const QIcon& icon);
    void newTabRequested(const QUrl& url);
    void closeRequested();

private slots:
    void onTitleChanged(const QString& title);
    void onUrlChanged(const QUrl& url);
    void onFaviconChanged(const QIcon& icon);
    void onLoadStarted();
    void onLoadFinished(bool ok);
    void onLoadProgress(int progress);

private:
    void setupPage();
    void setupConnections();
    static qint64 s_nextId;
    qint64 m_id;
    std::unique_ptr<QWebEngineView> m_view;
    QWebEngineProfile* m_profile;
    bool m_pinned{false};
    bool m_suspended{false};
    bool m_inReaderMode{false};
    QUrl m_suspendedUrl;
    QUrl m_originalUrl;
    int m_loadProgress{0};
    bool m_isLoading{false};
};

} // namespace Nova
