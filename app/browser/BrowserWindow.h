#pragma once

#include <QMainWindow>
#include <QTabBar>
#include <QLineEdit>
#include <QToolBar>
#include <QStatusBar>
#include <QStackedWidget>
#include <QProgressBar>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QShortcut>
#include <memory>
#include "TabManager.h"

namespace Nova {

class BrowserWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit BrowserWindow(QWidget* parent = nullptr);
    ~BrowserWindow() override;

    void openUrl(const QUrl& url);
    void openNewTab(const QUrl& url = QUrl());
    TabManager* tabManager() const;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    // Tab slots
    void onNewTab();
    void onCloseTab(int index);
    void onTabBarCurrentChanged(int index);
    void onTabMoved(int from, int to);
    void onTabCreated(BrowserTab* tab, int index);
    void onTabClosed(qint64 id, int index);
    void onActiveTabChanged(BrowserTab* tab);
    void onTabTitleChanged(qint64 id, const QString& title);
    void onTabIconChanged(qint64 id, const QIcon& icon);
    void onTabLoadProgress(qint64 id, int progress);
    void onTabUrlChanged(qint64 id, const QUrl& url);

    // Navigation
    void onNavigate();
    void onBack();
    void onForward();
    void onReload();
    void onStop();
    void onHome();
    void onUrlBarTextChanged(const QString& text);

    // Menu actions
    void onNewWindow();
    void onSavePageAs();
    void onPrint();
    void onOpenFile();
    void onQuit();

    void onFind();
    void onViewSource();
    void onDevTools();
    void onFullscreen();
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();

    void onHistory();
    void onBookmarks();
    void onDownloads();
    void onSettings();
    void onClearBrowsingData();

    void onBookmarkCurrentPage();
    void onReaderMode();
    void onRestoreTab();

    void onDuplicateTab();
    void onPinTab();

    // Tab context menu
    void showTabContextMenu(const QPoint& pos);

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupTabBar();
    void setupStatusBar();
    void setupShortcuts();
    void setupConnections();

    void updateNavigationButtons();
    void updateUrlBar(const QUrl& url);
    void updateWindowTitle(const QString& title);
    void saveSession();
    void restoreSession();

    void navigateToTextInput(const QString& text);
    bool isUrl(const QString& text) const;

    // UI components
    QTabBar* m_tabBar{nullptr};
    QStackedWidget* m_stack{nullptr};
    QLineEdit* m_urlBar{nullptr};
    QToolBar* m_navToolBar{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_statusLabel{nullptr};

    // Nav actions
    QAction* m_backAction{nullptr};
    QAction* m_forwardAction{nullptr};
    QAction* m_reloadAction{nullptr};
    QAction* m_stopAction{nullptr};
    QAction* m_homeAction{nullptr};

    // SSL indicator
    QLabel* m_sslIndicator{nullptr};
    QToolButton* m_readerModeBtn{nullptr};

    // Find bar
    QWidget* m_findBar{nullptr};
    QLineEdit* m_findEdit{nullptr};

    std::unique_ptr<TabManager> m_tabManager;
    BrowserTab* m_currentTab{nullptr};

    bool m_isFullscreen{false};
};

} // namespace Nova
