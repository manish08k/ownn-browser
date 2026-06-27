#pragma once

#include <QObject>
#include <QString>
#include <QWebEnginePage>
#include <functional>

namespace Nova {

struct ReaderContent {
    QString title;
    QString byline;
    QString htmlContent;
    QString textContent;
    int estimatedReadingTimeMinutes{0};
};

class ReaderMode : public QObject {
    Q_OBJECT

public:
    static ReaderMode& instance();

    // Extract reader content from a loaded page
    void extractContent(QWebEnginePage* page,
                        std::function<void(const ReaderContent&)> callback);

    // Generate styled HTML for reader view
    QString generateReaderHtml(const ReaderContent& content, bool darkMode = false) const;

    bool isReaderModeAvailable(const QString& url) const;

private:
    explicit ReaderMode(QObject* parent = nullptr);
    ~ReaderMode() override = default;

    ReaderMode(const ReaderMode&) = delete;
    ReaderMode& operator=(const ReaderMode&) = delete;

    // JavaScript to extract readable content
    QString extractionScript() const;
    ReaderContent parseExtractionResult(const QVariant& result) const;

    QString m_readerCss;
    void initCss();
};

} // namespace Nova
