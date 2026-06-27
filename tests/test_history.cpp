#include <QtTest>
#include <QTemporaryDir>
#include "HistoryManager.h"

class TestHistory : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_dir.setAutoRemove(true);
        QVERIFY(Nova::HistoryManager::instance().initialize(m_dir.path() + "/history.db"));
    }

    void testAddAndRetrieve() {
        Nova::HistoryManager::instance().addVisit("https://example.com", "Example");
        const auto results = Nova::HistoryManager::instance().search("example");
        QVERIFY(!results.isEmpty());
        QCOMPARE(results[0].url, QString("https://example.com"));
        QCOMPARE(results[0].title, QString("Example"));
    }

    void testVisitCount() {
        Nova::HistoryManager::instance().addVisit("https://counted.com", "Counted");
        Nova::HistoryManager::instance().addVisit("https://counted.com", "Counted");
        QCOMPARE(Nova::HistoryManager::instance().visitCount("https://counted.com"), 2);
    }

    void testHasVisited() {
        Nova::HistoryManager::instance().addVisit("https://visited.com", "Visited");
        QVERIFY(Nova::HistoryManager::instance().hasVisited("https://visited.com"));
        QVERIFY(!Nova::HistoryManager::instance().hasVisited("https://notvisited.com"));
    }

    void testDeleteEntry() {
        Nova::HistoryManager::instance().addVisit("https://delete-me.com", "Delete Me");
        auto results = Nova::HistoryManager::instance().search("delete-me");
        QVERIFY(!results.isEmpty());
        Nova::HistoryManager::instance().deleteEntry(results[0].id);
        results = Nova::HistoryManager::instance().search("delete-me");
        QVERIFY(results.isEmpty());
    }

    void testClearAll() {
        Nova::HistoryManager::instance().addVisit("https://clear1.com", "Clear1");
        Nova::HistoryManager::instance().addVisit("https://clear2.com", "Clear2");
        Nova::HistoryManager::instance().clearAll();
        const auto results = Nova::HistoryManager::instance().recentHistory(100);
        QVERIFY(results.isEmpty());
    }

private:
    QTemporaryDir m_dir;
};

QTEST_MAIN(TestHistory)
#include "test_history.moc"
