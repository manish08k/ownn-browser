#include <QtTest>
#include <QTemporaryDir>
#include "BookmarkManager.h"

class TestBookmarks : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_dir.setAutoRemove(true);
        QVERIFY(Nova::BookmarkManager::instance().initialize(m_dir.path() + "/bm.db"));
    }

    void testAddAndCheck() {
        Nova::BookmarkManager::instance().addBookmark("https://qt.io", "Qt", 0);
        QVERIFY(Nova::BookmarkManager::instance().isBookmarked("https://qt.io"));
        QVERIFY(!Nova::BookmarkManager::instance().isBookmarked("https://notbookmarked.io"));
    }

    void testRemove() {
        auto id = Nova::BookmarkManager::instance().addBookmark("https://remove.io", "Remove");
        QVERIFY(Nova::BookmarkManager::instance().isBookmarked("https://remove.io"));
        Nova::BookmarkManager::instance().removeBookmark(id);
        QVERIFY(!Nova::BookmarkManager::instance().isBookmarked("https://remove.io"));
    }

    void testFolders() {
        auto fid = Nova::BookmarkManager::instance().createFolder("Tech", 0);
        QVERIFY(fid > 0);
        Nova::BookmarkManager::instance().addBookmark("https://github.com", "GitHub", fid);
        const auto bms = Nova::BookmarkManager::instance().bookmarksInFolder(fid);
        QVERIFY(!bms.isEmpty());
        QCOMPARE(bms[0].url, QString("https://github.com"));
    }

    void testSearch() {
        Nova::BookmarkManager::instance().addBookmark("https://duckduckgo.com", "DuckDuckGo");
        const auto results = Nova::BookmarkManager::instance().search("duck");
        QVERIFY(!results.isEmpty());
    }

private:
    QTemporaryDir m_dir;
};

QTEST_MAIN(TestBookmarks)
#include "test_bookmarks.moc"
