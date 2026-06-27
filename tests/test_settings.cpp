#include <QtTest>
#include <QTemporaryDir>
#include "SettingsManager.h"

class TestSettings : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_dir.setAutoRemove(true);
        Nova::SettingsManager::instance().initialize(m_dir.path() + "/settings.ini");
    }

    void testHomepage() {
        Nova::SettingsManager::instance().setHomepage("https://news.ycombinator.com");
        QCOMPARE(Nova::SettingsManager::instance().homepage(),
                 QString("https://news.ycombinator.com"));
    }

    void testSearchEngine() {
        Nova::SettingsManager::instance().setDefaultSearchEngine("DuckDuckGo");
        QCOMPARE(Nova::SettingsManager::instance().defaultSearchEngine(),
                 QString("DuckDuckGo"));
    }

    void testSearchUrl() {
        Nova::SettingsManager::instance().setDefaultSearchEngine("Google");
        const QString url = Nova::SettingsManager::instance().searchUrl("hello world");
        QVERIFY(url.contains("google.com"));
        QVERIFY(url.contains("hello"));
    }

    void testPrivacyFlags() {
        Nova::SettingsManager::instance().setSaveHistory(false);
        QVERIFY(!Nova::SettingsManager::instance().saveHistory());
        Nova::SettingsManager::instance().setSaveHistory(true);
        QVERIFY(Nova::SettingsManager::instance().saveHistory());
    }

    void testCacheSize() {
        Nova::SettingsManager::instance().setMaxCacheSizeMB(256);
        QCOMPARE(Nova::SettingsManager::instance().maxCacheSizeMB(), 256);
    }

private:
    QTemporaryDir m_dir;
};

QTEST_MAIN(TestSettings)
#include "test_settings.moc"
