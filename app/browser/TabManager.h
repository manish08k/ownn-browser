#pragma once
#include <QObject>
#include <QVector>
#include <QStack>
#include <vector>
#include <memory>
#include <mutex>
#include "BrowserTab.h"

namespace Nova {

class TabManager : public QObject {
    Q_OBJECT
public:
    explicit TabManager(QWebEngineProfile* profile, QObject* parent = nullptr);
    ~TabManager() override = default;

    BrowserTab* createTab(const QUrl& url = QUrl(), bool background = false);
    BrowserTab* duplicateTab(qint64 tabId);
    void closeTab(qint64 tabId);
    void closeAllTabs();
    BrowserTab* activeTab() const;
    BrowserTab* tab(qint64 id) const;
    BrowserTab* tabAt(int index) const;
    int tabIndex(qint64 id) const;
    int count() const;
    void setActiveTab(qint64 id);
    void moveTab(int fromIndex, int toIndex);
    void suspendInactiveTabs();
    void unsuspendTab(qint64 id);
    QVector<BrowserTab*> tabs() const;
    bool canRestoreTab() const;
    BrowserTab* restoreLastClosedTab();

signals:
    void tabCreated(BrowserTab* tab, int index);
    void tabClosed(qint64 tabId, int index);
    void tabMoved(int fromIndex, int toIndex);
    void activeTabChanged(BrowserTab* tab);
    void tabTitleChanged(qint64 tabId, const QString& title);
    void tabIconChanged(qint64 tabId, const QIcon& icon);
    void tabLoadProgress(qint64 tabId, int progress);
    void tabUrlChanged(qint64 tabId, const QUrl& url);

private slots:
    void onTabTitleChanged(const QString& title);
    void onTabUrlChanged(const QUrl& url);
    void onTabIconChanged(const QIcon& icon);
    void onNewTabRequested(const QUrl& url);
    void onTabCloseRequested();

private:
    void connectTab(BrowserTab* tab);
    QWebEngineProfile* m_profile;
    std::vector<std::unique_ptr<BrowserTab>> m_tabs;
    BrowserTab* m_activeTab{nullptr};
    struct ClosedTabInfo { QUrl url; QString title; bool pinned{false}; };
    QStack<ClosedTabInfo> m_closedTabs;
    mutable std::mutex m_mutex;
};

} // namespace Nova
