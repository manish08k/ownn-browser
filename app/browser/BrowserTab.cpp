#include "BrowserTab.h"
#include "Logger.h"
#include "HistoryManager.h"
#include "SettingsManager.h"
#include "PermissionManager.h"
#include "ReaderMode.h"
#include <QWebEngineSettings>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineNavigationRequest>
#include <QWebEngineHistory>
#include <QWebEnginePermission>

namespace Nova {

qint64 BrowserTab::s_nextId = 1;

BrowserTab::BrowserTab(QWebEngineProfile* profile, QObject* parent)
    : QObject(parent), m_id(s_nextId++), m_profile(profile)
{
    m_view = std::make_unique<QWebEngineView>();
    m_view->setPage(new QWebEnginePage(m_profile, m_view.get()));
    setupPage();
    setupConnections();
}

BrowserTab::~BrowserTab() = default;

void BrowserTab::setupPage() {
    auto* s = page()->settings();
    s->setAttribute(QWebEngineSettings::JavascriptEnabled, SettingsManager::instance().javascriptEnabled());
    s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, !SettingsManager::instance().blockPopups());
    s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    s->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
    s->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
}

void BrowserTab::setupConnections() {
    auto* p = page();
    connect(p, &QWebEnginePage::titleChanged, this, &BrowserTab::onTitleChanged);
    connect(p, &QWebEnginePage::urlChanged,   this, &BrowserTab::onUrlChanged);
    connect(p, &QWebEnginePage::iconChanged,  this, &BrowserTab::onFaviconChanged);
    connect(m_view.get(), &QWebEngineView::loadStarted,  this, &BrowserTab::onLoadStarted);
    connect(m_view.get(), &QWebEngineView::loadFinished, this, &BrowserTab::onLoadFinished);
    connect(m_view.get(), &QWebEngineView::loadProgress, this, &BrowserTab::onLoadProgress);

    connect(p, &QWebEnginePage::permissionRequested, this,
        [](QWebEnginePermission perm) {
            PermissionType type;
            switch (perm.permissionType()) {
                case QWebEnginePermission::PermissionType::Geolocation:          type = PermissionType::Geolocation;   break;
                case QWebEnginePermission::PermissionType::MediaAudioCapture:    type = PermissionType::Microphone;    break;
                case QWebEnginePermission::PermissionType::MediaVideoCapture:    type = PermissionType::Camera;        break;
                case QWebEnginePermission::PermissionType::MediaAudioVideoCapture: type = PermissionType::MediaCapture; break;
                case QWebEnginePermission::PermissionType::Notifications:        type = PermissionType::Notifications; break;
                default: type = PermissionType::Notifications; break;
            }
            auto state = PermissionManager::instance().getPermission(perm.origin(), type);
            if (state == PermissionState::Granted) perm.grant();
            else perm.deny();
        }
    );

    connect(p, &QWebEnginePage::navigationRequested, this,
        [](QWebEngineNavigationRequest& req) { req.accept(); });

    connect(p, &QWebEnginePage::newWindowRequested, this,
        [this](QWebEngineNewWindowRequest& req) {
            emit newTabRequested(req.requestedUrl());
            req.openIn(nullptr);
        });

    connect(p, &QWebEnginePage::windowCloseRequested, this, &BrowserTab::closeRequested);
}

QWebEngineView* BrowserTab::view() const { return m_view.get(); }
QWebEnginePage* BrowserTab::page() const { return m_view->page(); }
QString BrowserTab::title() const { const QString t = page()->title(); return t.isEmpty() ? url().toString() : t; }
QUrl BrowserTab::url() const { return m_suspended ? m_suspendedUrl : page()->url(); }
QIcon BrowserTab::favicon() const { return page()->icon(); }
bool BrowserTab::isPinned() const { return m_pinned; }
void BrowserTab::setPinned(bool p) { if (m_pinned==p) return; m_pinned=p; emit pinnedChanged(p); }
bool BrowserTab::isSuspended() const { return m_suspended; }

void BrowserTab::suspend() {
    if (m_suspended) return;
    m_suspendedUrl = page()->url();
    page()->setLifecycleState(QWebEnginePage::LifecycleState::Discarded);
    m_suspended = true;
    emit suspendedChanged(true);
}

void BrowserTab::unsuspend() {
    if (!m_suspended) return;
    m_suspended = false;
    navigate(m_suspendedUrl);
    emit suspendedChanged(false);
}

bool BrowserTab::isLoading() const { return m_isLoading; }
int  BrowserTab::loadProgress() const { return m_loadProgress; }

void BrowserTab::navigate(const QUrl& url) {
    if (!url.isValid()) return;
    if (m_suspended) { m_suspended = false; emit suspendedChanged(false); }
    m_view->load(url);
}

void BrowserTab::reload()  { m_view->reload(); }
void BrowserTab::stop()    { m_view->stop(); }
void BrowserTab::back()    { m_view->back(); }
void BrowserTab::forward() { m_view->forward(); }
bool BrowserTab::canGoBack()    const { return page()->history()->canGoBack(); }
bool BrowserTab::canGoForward() const { return page()->history()->canGoForward(); }

void BrowserTab::openDevTools() {
    auto* devPage = new QWebEnginePage(m_profile);
    page()->setDevToolsPage(devPage);
    auto* devView = new QWebEngineView();
    devView->setPage(devPage);
    devView->resize(900, 600);
    devView->show();
    devView->setAttribute(Qt::WA_DeleteOnClose);
}

void BrowserTab::inspectElement() { openDevTools(); }
void BrowserTab::viewPageSource() { emit newTabRequested(QUrl("view-source:" + page()->url().toString())); }

void BrowserTab::findText(const QString& text, bool cs, bool bwd) {
    QWebEnginePage::FindFlags f;
    if (cs)  f |= QWebEnginePage::FindCaseSensitively;
    if (bwd) f |= QWebEnginePage::FindBackward;
    page()->findText(text, f);
}

void BrowserTab::clearFindText() { page()->findText({}); }
void BrowserTab::setZoomFactor(double f) { m_view->setZoomFactor(f); }
double BrowserTab::zoomFactor() const { return m_view->zoomFactor(); }

void BrowserTab::enableReaderMode() {
    if (m_inReaderMode) return;
    m_originalUrl = page()->url();
    ReaderMode::instance().extractContent(page(), [this](const ReaderContent& c) {
        if (c.htmlContent.isEmpty()) return;
        m_inReaderMode = true;
        page()->setHtml(ReaderMode::instance().generateReaderHtml(c, SettingsManager::instance().darkMode()), m_originalUrl);
    });
}

void BrowserTab::disableReaderMode() {
    if (!m_inReaderMode) return;
    m_inReaderMode = false;
    navigate(m_originalUrl);
}

bool BrowserTab::isInReaderMode() const { return m_inReaderMode; }
qint64 BrowserTab::id() const { return m_id; }
QUrl BrowserTab::suspendedUrl() const { return m_suspendedUrl; }

void BrowserTab::onTitleChanged(const QString& t) { emit titleChanged(t); }

void BrowserTab::onUrlChanged(const QUrl& url) {
    if (m_inReaderMode) return;
    emit urlChanged(url);
    emit readerModeAvailable(ReaderMode::instance().isReaderModeAvailable(url.toString()));
}

void BrowserTab::onFaviconChanged(const QIcon& icon) { emit faviconChanged(icon); emit iconChanged(icon); }

void BrowserTab::onLoadStarted() {
    m_isLoading = true; m_loadProgress = 0; emit loadStarted();
}

void BrowserTab::onLoadFinished(bool ok) {
    m_isLoading = false; m_loadProgress = 100;
    if (ok && SettingsManager::instance().saveHistory()) {
        const QUrl u = page()->url();
        if (u.scheme() == "http" || u.scheme() == "https")
            HistoryManager::instance().addVisit(u.toString(), page()->title());
    }
    emit loadFinished(ok);
}

void BrowserTab::onLoadProgress(int p) { m_loadProgress = p; emit loadProgress(p); }

} // namespace Nova
