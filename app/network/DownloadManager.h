#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QWebEngineDownloadRequest>
#include <QMap>
#include <memory>
#include <mutex>

namespace Nova {

enum class DownloadStatus {
    Pending,
    InProgress,
    Paused,
    Completed,
    Cancelled,
    Failed
};

struct DownloadItem {
    qint64 id{0};
    QString url;
    QString filename;
    QString savePath;
    qint64 totalBytes{0};
    qint64 receivedBytes{0};
    DownloadStatus status{DownloadStatus::Pending};
    QDateTime startedAt;
    QDateTime finishedAt;
    QString mimeType;
    QWebEngineDownloadRequest* request{nullptr};
};

class DownloadManager : public QObject {
    Q_OBJECT

public:
    static DownloadManager& instance();

    void initialize(const QString& downloadPath);
    QString defaultDownloadPath() const;
    void setDefaultDownloadPath(const QString& path);

    void handleDownload(QWebEngineDownloadRequest* download);

    QVector<DownloadItem> downloads() const;
    DownloadItem* findDownload(qint64 id);

    void pauseDownload(qint64 id);
    void resumeDownload(qint64 id);
    void cancelDownload(qint64 id);
    void removeDownload(qint64 id);
    void clearCompleted();

signals:
    void downloadStarted(qint64 id);
    void downloadProgress(qint64 id, qint64 received, qint64 total);
    void downloadFinished(qint64 id);
    void downloadFailed(qint64 id, const QString& reason);
    void downloadPaused(qint64 id);
    void downloadResumed(qint64 id);
    void downloadCancelled(qint64 id);

private:
    explicit DownloadManager(QObject* parent = nullptr);
    ~DownloadManager() override = default;

    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;

    qint64 nextId();

    QString m_downloadPath;
    QVector<DownloadItem> m_downloads;
    qint64 m_nextId{1};
    mutable std::mutex m_mutex;
};

} // namespace Nova
