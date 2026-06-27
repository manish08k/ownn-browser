#pragma once

#include <QObject>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QString>
#include <memory>

namespace Nova {

class BrowserEngine : public QObject {
    Q_OBJECT

public:
    static BrowserEngine& instance();

    void initialize(const QString& dataPath);

    QWebEngineProfile* defaultProfile() const;
    QWebEngineProfile* privateProfile() const;

    void applySettings();
    void clearBrowsingData(bool history, bool cookies, bool cache, bool passwords);

private:
    explicit BrowserEngine(QObject* parent = nullptr);
    ~BrowserEngine() override = default;

    BrowserEngine(const BrowserEngine&) = delete;
    BrowserEngine& operator=(const BrowserEngine&) = delete;

    void setupProfile(QWebEngineProfile* profile, const QString& dataPath);
    void injectScripts(QWebEngineProfile* profile);

    std::unique_ptr<QWebEngineProfile> m_defaultProfile;
    std::unique_ptr<QWebEngineProfile> m_privateProfile;
    QString m_dataPath;
};

} // namespace Nova
