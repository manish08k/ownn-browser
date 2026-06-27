#include "BrowserWindow.h"
#include "BrowserEngine.h"
#include "SettingsManager.h"
#include "SessionManager.h"
#include "HistoryManager.h"
#include "BookmarkManager.h"
#include "DownloadManager.h"
#include "Logger.h"
#include "HistoryDialog.h"
#include "BookmarkDialog.h"
#include "DownloadDialog.h"
#include "SettingsDialog.h"
#include "ClearDataDialog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QShortcut>
#include <QKeySequence>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QWebEngineView>
#include <QInputDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QClipboard>
#include <QRegularExpression>
#include <QAction>

namespace Nova {

BrowserWindow::BrowserWindow(QWidget* parent) : QMainWindow(parent) {
    m_tabManager = std::make_unique<TabManager>(BrowserEngine::instance().defaultProfile(), this);
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupTabBar();
    setupStatusBar();
    setupShortcuts();
    setupConnections();
    restoreSession();
    if (m_tabManager->count() == 0)
        openNewTab(QUrl(SettingsManager::instance().homepage()));
}

BrowserWindow::~BrowserWindow() { saveSession(); }

void BrowserWindow::setupUi() {
    setWindowTitle("Nova Browser");
    setMinimumSize(800, 600);
    resize(1280, 800);

    auto* central = new QWidget(this);
    auto* layout  = new QVBoxLayout(central);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    m_tabBar = new QTabBar(central);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setContextMenuPolicy(Qt::CustomContextMenu);

    m_stack = new QStackedWidget(central);

    m_findBar  = new QWidget(central);
    auto* frow = new QHBoxLayout(m_findBar);
    frow->setContentsMargins(4,2,4,2);
    m_findEdit = new QLineEdit(m_findBar);
    m_findEdit->setPlaceholderText("Find in page...");
    auto* findNext  = new QPushButton("▼", m_findBar);
    auto* findPrev  = new QPushButton("▲", m_findBar);
    auto* findClose = new QPushButton("✕", m_findBar);
    frow->addWidget(new QLabel("Find:", m_findBar));
    frow->addWidget(m_findEdit);
    frow->addWidget(findPrev);
    frow->addWidget(findNext);
    frow->addWidget(findClose);
    m_findBar->hide();

    layout->addWidget(m_tabBar);
    layout->addWidget(m_stack, 1);
    layout->addWidget(m_findBar);
    setCentralWidget(central);

    connect(m_findEdit, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (m_currentTab) m_currentTab->findText(t);
    });
    connect(findNext, &QPushButton::clicked, this, [this]() {
        if (m_currentTab) m_currentTab->findText(m_findEdit->text());
    });
    connect(findPrev, &QPushButton::clicked, this, [this]() {
        if (m_currentTab) m_currentTab->findText(m_findEdit->text(), false, true);
    });
    connect(findClose, &QPushButton::clicked, this, [this]() {
        m_findBar->hide();
        if (m_currentTab) m_currentTab->clearFindText();
    });
}

// Helper: create a menu action with shortcut + slot
static QAction* makeAction(QMenu* menu, const QString& text, const QKeySequence& key,
                            QObject* receiver, std::function<void()> fn)
{
    auto* a = menu->addAction(text);
    a->setShortcut(key);
    QObject::connect(a, &QAction::triggered, receiver, fn);
    return a;
}

void BrowserWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("&File");
    makeAction(fileMenu, "New Tab",       QKeySequence::AddTab,  this, [this]{ onNewTab(); });
    makeAction(fileMenu, "New Window",    {},                     this, [this]{ onNewWindow(); });
    fileMenu->addSeparator();
    makeAction(fileMenu, "Open File...",  QKeySequence::Open,     this, [this]{ onOpenFile(); });
    makeAction(fileMenu, "Save Page As...",QKeySequence::Save,    this, [this]{ onSavePageAs(); });
    fileMenu->addSeparator();
    makeAction(fileMenu, "Print...",      QKeySequence::Print,    this, [this]{ onPrint(); });
    fileMenu->addSeparator();
    makeAction(fileMenu, "Quit",          QKeySequence::Quit,     this, [this]{ onQuit(); });

    auto* editMenu = menuBar()->addMenu("&Edit");
    makeAction(editMenu, "Find...",       QKeySequence::Find,     this, [this]{ onFind(); });
    editMenu->addSeparator();
    { auto* a = editMenu->addAction("Cut");   a->setShortcut(QKeySequence::Cut);
      connect(a, &QAction::triggered, this, [this]{ if(m_currentTab) m_currentTab->page()->triggerAction(QWebEnginePage::Cut); }); }
    { auto* a = editMenu->addAction("Copy");  a->setShortcut(QKeySequence::Copy);
      connect(a, &QAction::triggered, this, [this]{ if(m_currentTab) m_currentTab->page()->triggerAction(QWebEnginePage::Copy); }); }
    { auto* a = editMenu->addAction("Paste"); a->setShortcut(QKeySequence::Paste);
      connect(a, &QAction::triggered, this, [this]{ if(m_currentTab) m_currentTab->page()->triggerAction(QWebEnginePage::Paste); }); }

    auto* viewMenu = menuBar()->addMenu("&View");
    makeAction(viewMenu, "Zoom In",       QKeySequence::ZoomIn,   this, [this]{ onZoomIn(); });
    makeAction(viewMenu, "Zoom Out",      QKeySequence::ZoomOut,  this, [this]{ onZoomOut(); });
    makeAction(viewMenu, "Reset Zoom",    QKeySequence(Qt::CTRL|Qt::Key_0), this, [this]{ onZoomReset(); });
    viewMenu->addSeparator();
    makeAction(viewMenu, "Full Screen",   QKeySequence(Qt::Key_F11), this, [this]{ onFullscreen(); });
    viewMenu->addSeparator();
    makeAction(viewMenu, "Page Source",   QKeySequence(Qt::CTRL|Qt::Key_U), this, [this]{ onViewSource(); });
    makeAction(viewMenu, "Reader Mode",   {}, this, [this]{ onReaderMode(); });
    viewMenu->addSeparator();
    makeAction(viewMenu, "Developer Tools", QKeySequence(Qt::Key_F12), this, [this]{ onDevTools(); });

    auto* histMenu = menuBar()->addMenu("Hi&story");
    makeAction(histMenu, "Show History",      QKeySequence(Qt::CTRL|Qt::Key_H), this, [this]{ onHistory(); });
    makeAction(histMenu, "Restore Closed Tab",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_T), this, [this]{ onRestoreTab(); });

    auto* bmMenu = menuBar()->addMenu("&Bookmarks");
    makeAction(bmMenu, "Bookmark This Page", QKeySequence(Qt::CTRL|Qt::Key_D), this, [this]{ onBookmarkCurrentPage(); });
    makeAction(bmMenu, "Manage Bookmarks",   {}, this, [this]{ onBookmarks(); });

    auto* dlMenu = menuBar()->addMenu("&Downloads");
    makeAction(dlMenu, "Show Downloads", QKeySequence(Qt::CTRL|Qt::Key_J), this, [this]{ onDownloads(); });

    auto* toolsMenu = menuBar()->addMenu("&Tools");
    makeAction(toolsMenu, "Settings",            {}, this, [this]{ onSettings(); });
    makeAction(toolsMenu, "Clear Browsing Data...", {}, this, [this]{ onClearBrowsingData(); });
}

void BrowserWindow::setupToolBar() {
    m_navToolBar = addToolBar("Navigation");
    m_navToolBar->setMovable(false);
    m_navToolBar->setFloatable(false);

    m_backAction    = m_navToolBar->addAction("◀");
    m_forwardAction = m_navToolBar->addAction("▶");
    m_reloadAction  = m_navToolBar->addAction("⟳");
    m_stopAction    = m_navToolBar->addAction("✕");
    m_homeAction    = m_navToolBar->addAction("⌂");
    m_navToolBar->addSeparator();

    m_sslIndicator = new QLabel(this);
    m_sslIndicator->setFixedWidth(24);
    m_navToolBar->addWidget(m_sslIndicator);

    m_urlBar = new QLineEdit(this);
    m_urlBar->setPlaceholderText("Search or enter address");
    m_urlBar->setClearButtonEnabled(true);
    m_navToolBar->addWidget(m_urlBar);

    m_readerModeBtn = new QToolButton(this);
    m_readerModeBtn->setText("📖");
    m_readerModeBtn->setToolTip("Reader Mode");
    m_readerModeBtn->setCheckable(true);
    m_readerModeBtn->setEnabled(false);
    m_navToolBar->addWidget(m_readerModeBtn);

    m_stopAction->setVisible(false);

    connect(m_backAction,    &QAction::triggered, this, &BrowserWindow::onBack);
    connect(m_forwardAction, &QAction::triggered, this, &BrowserWindow::onForward);
    connect(m_reloadAction,  &QAction::triggered, this, &BrowserWindow::onReload);
    connect(m_stopAction,    &QAction::triggered, this, &BrowserWindow::onStop);
    connect(m_homeAction,    &QAction::triggered, this, &BrowserWindow::onHome);
    connect(m_urlBar, &QLineEdit::returnPressed,  this, &BrowserWindow::onNavigate);
    connect(m_readerModeBtn, &QToolButton::clicked, this, &BrowserWindow::onReaderMode);
}

void BrowserWindow::setupTabBar() {
    connect(m_tabBar, &QTabBar::currentChanged,    this, &BrowserWindow::onTabBarCurrentChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &BrowserWindow::onCloseTab);
    connect(m_tabBar, &QTabBar::tabMoved,          this, &BrowserWindow::onTabMoved);
    connect(m_tabBar, &QTabBar::customContextMenuRequested, this, &BrowserWindow::showTabContextMenu);

    auto* newTabBtn = new QToolButton(this);
    newTabBtn->setText("+");
    newTabBtn->setToolTip("New Tab (Ctrl+T)");
    m_navToolBar->addWidget(newTabBtn);
    connect(newTabBtn, &QToolButton::clicked, this, &BrowserWindow::onNewTab);
}

void BrowserWindow::setupStatusBar() {
    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel, 1);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setRange(0, 100);
    m_progressBar->hide();
    statusBar()->addPermanentWidget(m_progressBar);
}

void BrowserWindow::setupShortcuts() {
    new QShortcut(QKeySequence(Qt::CTRL|Qt::Key_T), this, this, &BrowserWindow::onNewTab);
    new QShortcut(QKeySequence(Qt::CTRL|Qt::Key_W), this, [this]{ if(m_currentTab) onCloseTab(m_tabBar->currentIndex()); });
    new QShortcut(QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_T), this, this, &BrowserWindow::onRestoreTab);
    new QShortcut(QKeySequence(Qt::CTRL|Qt::Key_L), this, [this]{ m_urlBar->setFocus(); m_urlBar->selectAll(); });
    new QShortcut(QKeySequence(Qt::CTRL|Qt::Key_R), this, this, &BrowserWindow::onReload);
    new QShortcut(QKeySequence(Qt::CTRL|Qt::Key_F), this, this, &BrowserWindow::onFind);
    new QShortcut(QKeySequence(Qt::CTRL|Qt::Key_Tab), this, [this]{
        if (!m_tabManager->count()) return;
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex()+1) % m_tabManager->count());
    });
    new QShortcut(QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_Tab), this, [this]{
        if (!m_tabManager->count()) return;
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex()-1+m_tabManager->count()) % m_tabManager->count());
    });
    new QShortcut(QKeySequence(Qt::Key_F5),  this, this, &BrowserWindow::onReload);
    new QShortcut(QKeySequence(Qt::Key_F11), this, this, &BrowserWindow::onFullscreen);
    new QShortcut(QKeySequence(Qt::Key_F12), this, this, &BrowserWindow::onDevTools);
    new QShortcut(QKeySequence(Qt::Key_Escape), this, [this]{
        if (m_findBar->isVisible()) { m_findBar->hide(); if(m_currentTab) m_currentTab->clearFindText(); }
        else if (m_currentTab && m_currentTab->isLoading()) onStop();
    });
    new QShortcut(QKeySequence(Qt::ALT|Qt::Key_Left),  this, this, &BrowserWindow::onBack);
    new QShortcut(QKeySequence(Qt::ALT|Qt::Key_Right), this, this, &BrowserWindow::onForward);
}

void BrowserWindow::setupConnections() {
    connect(m_tabManager.get(), &TabManager::tabCreated,       this, &BrowserWindow::onTabCreated);
    connect(m_tabManager.get(), &TabManager::tabClosed,        this, &BrowserWindow::onTabClosed);
    connect(m_tabManager.get(), &TabManager::activeTabChanged, this, &BrowserWindow::onActiveTabChanged);
    connect(m_tabManager.get(), &TabManager::tabTitleChanged,  this, &BrowserWindow::onTabTitleChanged);
    connect(m_tabManager.get(), &TabManager::tabIconChanged,   this, &BrowserWindow::onTabIconChanged);
    connect(m_tabManager.get(), &TabManager::tabLoadProgress,  this, &BrowserWindow::onTabLoadProgress);
    connect(m_tabManager.get(), &TabManager::tabUrlChanged,    this, &BrowserWindow::onTabUrlChanged);
    connect(m_tabManager.get(), &TabManager::tabMoved,         this, &BrowserWindow::onTabMoved);
}

void BrowserWindow::openUrl(const QUrl& url) {
    if (m_currentTab) m_currentTab->navigate(url); else openNewTab(url);
}
void BrowserWindow::openNewTab(const QUrl& url) { m_tabManager->createTab(url, false); }
TabManager* BrowserWindow::tabManager() const { return m_tabManager.get(); }

void BrowserWindow::closeEvent(QCloseEvent* e) { saveSession(); e->accept(); }
void BrowserWindow::keyPressEvent(QKeyEvent* e) { QMainWindow::keyPressEvent(e); }

void BrowserWindow::onNewTab() { openNewTab(QUrl(SettingsManager::instance().homepage())); }

void BrowserWindow::onCloseTab(int index) {
    BrowserTab* t = m_tabManager->tabAt(index);
    if (t) m_tabManager->closeTab(t->id());
    if (m_tabManager->count() == 0) onNewTab();
}

void BrowserWindow::onTabBarCurrentChanged(int index) {
    BrowserTab* t = m_tabManager->tabAt(index);
    if (t && t != m_currentTab) m_tabManager->setActiveTab(t->id());
}

void BrowserWindow::onTabMoved(int from, int to) { m_tabManager->moveTab(from, to); }

void BrowserWindow::onTabCreated(BrowserTab* tab, int index) {
    m_stack->insertWidget(index, tab->view());
    m_tabBar->insertTab(index, "New Tab");
    QSignalBlocker b(m_tabBar);
    m_tabBar->setCurrentIndex(index);
}

void BrowserWindow::onTabClosed(qint64, int index) {
    m_stack->removeWidget(m_stack->widget(index));
    m_tabBar->removeTab(index);
}

void BrowserWindow::onActiveTabChanged(BrowserTab* tab) {
    m_currentTab = tab;
    if (!tab) return;
    int idx = m_tabManager->tabIndex(tab->id());
    m_stack->setCurrentIndex(idx);
    { QSignalBlocker b(m_tabBar); m_tabBar->setCurrentIndex(idx); }
    updateUrlBar(tab->url());
    updateNavigationButtons();
    updateWindowTitle(tab->title());
    m_readerModeBtn->setChecked(tab->isInReaderMode());
}

void BrowserWindow::onTabTitleChanged(qint64 id, const QString& title) {
    int idx = m_tabManager->tabIndex(id);
    if (idx >= 0 && idx < m_tabBar->count()) m_tabBar->setTabText(idx, title.left(30));
    if (m_currentTab && m_currentTab->id()==id) updateWindowTitle(title);
}

void BrowserWindow::onTabIconChanged(qint64 id, const QIcon& icon) {
    int idx = m_tabManager->tabIndex(id);
    if (idx >= 0 && idx < m_tabBar->count()) m_tabBar->setTabIcon(idx, icon);
}

void BrowserWindow::onTabLoadProgress(qint64 id, int progress) {
    if (!m_currentTab || m_currentTab->id()!=id) return;
    if (progress < 100) {
        m_progressBar->show(); m_progressBar->setValue(progress);
        m_reloadAction->setVisible(false); m_stopAction->setVisible(true);
    } else {
        m_progressBar->hide(); m_progressBar->setValue(0);
        m_reloadAction->setVisible(true); m_stopAction->setVisible(false);
    }
}

void BrowserWindow::onTabUrlChanged(qint64 id, const QUrl& url) {
    if (!m_currentTab || m_currentTab->id()!=id) return;
    updateUrlBar(url);
    updateNavigationButtons();
    if (url.scheme()=="https") { m_sslIndicator->setText("🔒"); m_sslIndicator->setToolTip("Secure"); }
    else if (url.scheme()=="http") { m_sslIndicator->setText("⚠"); m_sslIndicator->setToolTip("Not secure"); }
    else { m_sslIndicator->setText(""); m_sslIndicator->setToolTip(""); }
}

void BrowserWindow::onNavigate() { navigateToTextInput(m_urlBar->text().trimmed()); }

void BrowserWindow::navigateToTextInput(const QString& text) {
    if (text.isEmpty()) return;
    if (!m_currentTab) openNewTab();
    QUrl url = isUrl(text) ? QUrl::fromUserInput(text) : QUrl(SettingsManager::instance().searchUrl(text));
    if (m_currentTab) m_currentTab->navigate(url);
}

bool BrowserWindow::isUrl(const QString& text) const {
    if (text.contains("://") || text.startsWith("localhost") || text.startsWith("file://") || text.startsWith("about:")) return true;
    static QRegularExpression rx(R"(^[a-zA-Z0-9\-\.]+\.[a-zA-Z]{2,}(/.*)?$)");
    return rx.match(text).hasMatch();
}

void BrowserWindow::onBack()    { if (m_currentTab) m_currentTab->back(); }
void BrowserWindow::onForward() { if (m_currentTab) m_currentTab->forward(); }
void BrowserWindow::onReload()  { if (m_currentTab) m_currentTab->reload(); }
void BrowserWindow::onStop()    { if (m_currentTab) m_currentTab->stop(); }
void BrowserWindow::onHome()    { if (m_currentTab) m_currentTab->navigate(QUrl(SettingsManager::instance().homepage())); }
void BrowserWindow::onUrlBarTextChanged(const QString&) {}

void BrowserWindow::onNewWindow() { (new BrowserWindow())->show(); }

void BrowserWindow::onSavePageAs() {
    if (!m_currentTab) return;
    const QString path = QFileDialog::getSaveFileName(this, "Save Page As",
        QDir::homePath()+"/"+m_currentTab->title()+".html", "Web Page (*.html);;All Files (*)");
    if (!path.isEmpty()) m_currentTab->page()->save(path);
}

void BrowserWindow::onPrint() {
    if (!m_currentTab) return;
    QPrinter printer;
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) m_currentTab->view()->print(&printer);
}

void BrowserWindow::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath(),
        "Web Files (*.html *.htm *.pdf);;All Files (*)");
    if (!path.isEmpty()) openNewTab(QUrl::fromLocalFile(path));
}

void BrowserWindow::onQuit()  { saveSession(); qApp->quit(); }
void BrowserWindow::onFind()  { m_findBar->show(); m_findEdit->setFocus(); m_findEdit->selectAll(); }
void BrowserWindow::onViewSource() { if (m_currentTab) m_currentTab->viewPageSource(); }
void BrowserWindow::onDevTools()   { if (m_currentTab) m_currentTab->openDevTools(); }

void BrowserWindow::onFullscreen() {
    if (m_isFullscreen) showNormal(); else showFullScreen();
    m_isFullscreen = !m_isFullscreen;
}

void BrowserWindow::onZoomIn()    { if (m_currentTab) m_currentTab->setZoomFactor(m_currentTab->zoomFactor()+0.1); }
void BrowserWindow::onZoomOut()   { if (m_currentTab) m_currentTab->setZoomFactor(m_currentTab->zoomFactor()-0.1); }
void BrowserWindow::onZoomReset() { if (m_currentTab) m_currentTab->setZoomFactor(1.0); }

void BrowserWindow::onHistory() {
    auto* dlg = new HistoryDialog(this);
    connect(dlg, &HistoryDialog::urlActivated, this, [this](const QUrl& u){ openNewTab(u); });
    dlg->exec();
}

void BrowserWindow::onBookmarks() {
    auto* dlg = new BookmarkDialog(this);
    connect(dlg, &BookmarkDialog::urlActivated, this, [this](const QUrl& u){ openNewTab(u); });
    dlg->exec();
}

void BrowserWindow::onDownloads() { (new DownloadDialog(this))->exec(); }

void BrowserWindow::onSettings() {
    auto* dlg = new SettingsDialog(this);
    connect(dlg, &SettingsDialog::settingsApplied, this, [this]{ BrowserEngine::instance().applySettings(); });
    dlg->exec();
}

void BrowserWindow::onClearBrowsingData() { (new ClearDataDialog(this))->exec(); }

void BrowserWindow::onBookmarkCurrentPage() {
    if (!m_currentTab) return;
    const QString url = m_currentTab->url().toString();
    if (BookmarkManager::instance().isBookmarked(url)) {
        BookmarkManager::instance().removeBookmark(BookmarkManager::instance().bookmarkForUrl(url).id);
        statusBar()->showMessage("Bookmark removed", 2000);
    } else {
        BookmarkManager::instance().addBookmark(url, m_currentTab->title());
        statusBar()->showMessage("Page bookmarked", 2000);
    }
}

void BrowserWindow::onReaderMode() {
    if (!m_currentTab) return;
    if (m_currentTab->isInReaderMode()) { m_currentTab->disableReaderMode(); m_readerModeBtn->setChecked(false); }
    else { m_currentTab->enableReaderMode(); m_readerModeBtn->setChecked(true); }
}

void BrowserWindow::onRestoreTab()   { m_tabManager->restoreLastClosedTab(); }
void BrowserWindow::onDuplicateTab() { if (m_currentTab) m_tabManager->duplicateTab(m_currentTab->id()); }
void BrowserWindow::onPinTab()       { if (m_currentTab) m_currentTab->setPinned(!m_currentTab->isPinned()); }

void BrowserWindow::showTabContextMenu(const QPoint& pos) {
    const int index = m_tabBar->tabAt(pos);
    if (index < 0) return;
    BrowserTab* tab = m_tabManager->tabAt(index);
    if (!tab) return;
    QMenu menu(this);
    menu.addAction("New Tab",        this, &BrowserWindow::onNewTab);
    menu.addSeparator();
    menu.addAction("Duplicate Tab",  this, &BrowserWindow::onDuplicateTab);
    menu.addAction(tab->isPinned() ? "Unpin Tab" : "Pin Tab", this, &BrowserWindow::onPinTab);
    menu.addSeparator();
    menu.addAction("Restore Closed Tab", this, &BrowserWindow::onRestoreTab);
    menu.addSeparator();
    menu.addAction("Close Tab", this, [this, index]{ onCloseTab(index); });
    menu.addAction("Close Other Tabs", this, [this, tab]{
        for (auto* t : m_tabManager->tabs()) if (t!=tab) m_tabManager->closeTab(t->id());
    });
    menu.exec(m_tabBar->mapToGlobal(pos));
}

void BrowserWindow::updateNavigationButtons() {
    if (!m_currentTab) { m_backAction->setEnabled(false); m_forwardAction->setEnabled(false); return; }
    m_backAction->setEnabled(m_currentTab->canGoBack());
    m_forwardAction->setEnabled(m_currentTab->canGoForward());
}

void BrowserWindow::updateUrlBar(const QUrl& url) {
    if (!m_urlBar->hasFocus()) m_urlBar->setText(url.toString());
}

void BrowserWindow::updateWindowTitle(const QString& title) {
    setWindowTitle(title.isEmpty() ? "Nova Browser" : title + " — Nova Browser");
}

void BrowserWindow::saveSession() {
    if (!SettingsManager::instance().restoreSession()) return;
    SessionData data;
    const auto tabs = m_tabManager->tabs();
    for (int i = 0; i < tabs.size(); ++i) {
        TabSessionData td;
        td.url=tabs[i]->url().toString(); td.title=tabs[i]->title();
        td.pinned=tabs[i]->isPinned(); td.index=i;
        data.tabs.append(td);
    }
    data.activeTabIndex = m_tabBar->currentIndex();
    SessionManager::instance().saveSession(data);
}

void BrowserWindow::restoreSession() {
    if (!SettingsManager::instance().restoreSession()) return;
    if (!SessionManager::instance().hasSavedSession()) return;
    const SessionData data = SessionManager::instance().loadSession();
    if (data.tabs.isEmpty()) return;
    for (const auto& td : data.tabs) {
        BrowserTab* t = m_tabManager->createTab(QUrl(td.url), true);
        if (t) t->setPinned(td.pinned);
    }
    if (data.activeTabIndex >= 0 && data.activeTabIndex < m_tabManager->count()) {
        BrowserTab* active = m_tabManager->tabAt(data.activeTabIndex);
        if (active) m_tabManager->setActiveTab(active->id());
    }
}

} // namespace Nova
