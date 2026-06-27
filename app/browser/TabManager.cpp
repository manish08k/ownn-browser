#include "TabManager.h"
#include "Logger.h"
#include <algorithm>

namespace Nova {

TabManager::TabManager(QWebEngineProfile* profile, QObject* parent)
    : QObject(parent), m_profile(profile) {}

BrowserTab* TabManager::createTab(const QUrl& url, bool background) {
    auto tab = std::make_unique<BrowserTab>(m_profile, this);
    BrowserTab* raw = tab.get();
    connectTab(raw);
    int index;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tabs.push_back(std::move(tab));
        index = static_cast<int>(m_tabs.size()) - 1;
    }
    emit tabCreated(raw, index);
    if (!background || !m_activeTab) setActiveTab(raw->id());
    if (url.isValid()) raw->navigate(url);
    return raw;
}

BrowserTab* TabManager::duplicateTab(qint64 tabId) {
    BrowserTab* src = tab(tabId);
    return src ? createTab(src->url(), false) : nullptr;
}

void TabManager::closeTab(qint64 tabId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (m_tabs[i]->id() == tabId) {
            m_closedTabs.push({m_tabs[i]->url(), m_tabs[i]->title(), m_tabs[i]->isPinned()});
            if (m_activeTab == m_tabs[i].get()) {
                m_activeTab = nullptr;
                if (i+1 < static_cast<int>(m_tabs.size())) m_activeTab = m_tabs[i+1].get();
                else if (i-1 >= 0) m_activeTab = m_tabs[i-1].get();
            }
            m_tabs.erase(m_tabs.begin() + i);
            emit tabClosed(tabId, i);
            if (m_activeTab) emit activeTabChanged(m_activeTab);
            return;
        }
    }
}

void TabManager::closeAllTabs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tabs.clear(); m_activeTab = nullptr;
}

BrowserTab* TabManager::activeTab() const { return m_activeTab; }

BrowserTab* TabManager::tab(qint64 id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& t : m_tabs) if (t->id()==id) return t.get();
    return nullptr;
}

BrowserTab* TabManager::tabAt(int index) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) return nullptr;
    return m_tabs[index].get();
}

int TabManager::tabIndex(qint64 id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i)
        if (m_tabs[i]->id()==id) return i;
    return -1;
}

int TabManager::count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_tabs.size());
}

void TabManager::setActiveTab(qint64 id) {
    BrowserTab* t = tab(id);
    if (!t || t==m_activeTab) return;
    if (t->isSuspended()) t->unsuspend();
    m_activeTab = t;
    emit activeTabChanged(t);
}

void TabManager::moveTab(int from, int to) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int sz = static_cast<int>(m_tabs.size());
    if (from<0||from>=sz||to<0||to>=sz||from==to) return;
    auto t = std::move(m_tabs[from]);
    m_tabs.erase(m_tabs.begin()+from);
    m_tabs.insert(m_tabs.begin()+to, std::move(t));
    emit tabMoved(from, to);
}

void TabManager::suspendInactiveTabs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& t : m_tabs)
        if (t.get()!=m_activeTab && !t->isPinned() && !t->isSuspended()) t->suspend();
}

void TabManager::unsuspendTab(qint64 id) { BrowserTab* t=tab(id); if(t) t->unsuspend(); }

QVector<BrowserTab*> TabManager::tabs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    QVector<BrowserTab*> r; for (const auto& t : m_tabs) r.append(t.get()); return r;
}

bool TabManager::canRestoreTab() const { return !m_closedTabs.isEmpty(); }

BrowserTab* TabManager::restoreLastClosedTab() {
    if (m_closedTabs.isEmpty()) return nullptr;
    const auto info = m_closedTabs.pop();
    BrowserTab* t = createTab(info.url, false);
    if (t) t->setPinned(info.pinned);
    return t;
}

void TabManager::connectTab(BrowserTab* tab) {
    connect(tab, &BrowserTab::titleChanged,    this, &TabManager::onTabTitleChanged);
    connect(tab, &BrowserTab::urlChanged,      this, &TabManager::onTabUrlChanged);
    connect(tab, &BrowserTab::iconChanged,     this, &TabManager::onTabIconChanged);
    connect(tab, &BrowserTab::newTabRequested, this, &TabManager::onNewTabRequested);
    connect(tab, &BrowserTab::closeRequested,  this, &TabManager::onTabCloseRequested);
    // loadProgress has signal/slot name collision with QWebEngineView — use lambda with explicit cast
    connect(tab, static_cast<void(BrowserTab::*)(int)>(&BrowserTab::loadProgress),
        this, [this, tab](int p) { emit tabLoadProgress(tab->id(), p); });
}

void TabManager::onTabTitleChanged(const QString& t) {
    auto* s = qobject_cast<BrowserTab*>(sender()); if(s) emit tabTitleChanged(s->id(), t);
}
void TabManager::onTabUrlChanged(const QUrl& u) {
    auto* s = qobject_cast<BrowserTab*>(sender()); if(s) emit tabUrlChanged(s->id(), u);
}
void TabManager::onTabIconChanged(const QIcon& i) {
    auto* s = qobject_cast<BrowserTab*>(sender()); if(s) emit tabIconChanged(s->id(), i);
}
void TabManager::onNewTabRequested(const QUrl& url) { createTab(url, false); }
void TabManager::onTabCloseRequested() {
    auto* s = qobject_cast<BrowserTab*>(sender()); if(s) closeTab(s->id());
}

} // namespace Nova
