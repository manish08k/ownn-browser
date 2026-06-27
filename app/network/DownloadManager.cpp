#include "DownloadManager.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace Nova {

DownloadManager& DownloadManager::instance() {
    static DownloadManager inst;
    return inst;
}

DownloadManager::DownloadManager(QObject* parent) : QObject(parent) {
    m_downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

void DownloadManager::initialize(const QString& downloadPath) {
    m_downloadPath = downloadPath;
    QDir().mkpath(downloadPath);
}

QString DownloadManager::defaultDownloadPath() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_downloadPath;
}

void DownloadManager::setDefaultDownloadPath(const QString& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_downloadPath = path;
    QDir().mkpath(path);
}

void DownloadManager::handleDownload(QWebEngineDownloadRequest* download) {
    if (!download) return;

    DownloadItem item;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        item.id       = nextId();
        item.url      = download->url().toString();
        item.filename = QFileInfo(download->downloadFileName()).fileName();
        item.savePath = m_downloadPath + "/" + item.filename;
        item.status   = DownloadStatus::InProgress;
        item.startedAt = QDateTime::currentDateTime();
        item.mimeType  = download->mimeType();
        item.request   = download;
        item.totalBytes = download->totalBytes();

        download->setDownloadDirectory(m_downloadPath);
        download->setDownloadFileName(item.filename);

        m_downloads.append(item);
    }

    qint64 id = item.id;

    connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this,
        [this, id, download]() {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& d : m_downloads) {
                if (d.id == id) {
                    d.receivedBytes = download->receivedBytes();
                    d.totalBytes    = download->totalBytes();
                    emit downloadProgress(id, d.receivedBytes, d.totalBytes);
                    break;
                }
            }
        }
    );

    connect(download, &QWebEngineDownloadRequest::isFinishedChanged, this,
        [this, id, download]() {
            if (!download->isFinished()) return;
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& d : m_downloads) {
                if (d.id == id) {
                    d.finishedAt = QDateTime::currentDateTime();
                    if (download->state() == QWebEngineDownloadRequest::DownloadCompleted) {
                        d.status = DownloadStatus::Completed;
                        emit downloadFinished(id);
                        LOG_INFO("DownloadManager", "Download completed: " + d.filename);
                    } else if (download->state() == QWebEngineDownloadRequest::DownloadCancelled) {
                        d.status = DownloadStatus::Cancelled;
                        emit downloadCancelled(id);
                    } else {
                        d.status = DownloadStatus::Failed;
                        emit downloadFailed(id, "Download interrupted");
                        LOG_ERROR("DownloadManager", "Download failed: " + d.filename);
                    }
                    break;
                }
            }
        }
    );

    download->accept();
    emit downloadStarted(id);
    LOG_INFO("DownloadManager", "Download started: " + item.filename);
}

QVector<DownloadItem> DownloadManager::downloads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_downloads;
}

DownloadItem* DownloadManager::findDownload(qint64 id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& d : m_downloads) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

void DownloadManager::pauseDownload(qint64 id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& d : m_downloads) {
        if (d.id == id && d.request && d.status == DownloadStatus::InProgress) {
            d.request->pause();
            d.status = DownloadStatus::Paused;
            emit downloadPaused(id);
            break;
        }
    }
}

void DownloadManager::resumeDownload(qint64 id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& d : m_downloads) {
        if (d.id == id && d.request && d.status == DownloadStatus::Paused) {
            d.request->resume();
            d.status = DownloadStatus::InProgress;
            emit downloadResumed(id);
            break;
        }
    }
}

void DownloadManager::cancelDownload(qint64 id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& d : m_downloads) {
        if (d.id == id && d.request) {
            d.request->cancel();
            d.status = DownloadStatus::Cancelled;
            emit downloadCancelled(id);
            break;
        }
    }
}

void DownloadManager::removeDownload(qint64 id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_downloads.erase(
        std::remove_if(m_downloads.begin(), m_downloads.end(),
            [id](const DownloadItem& d) { return d.id == id; }),
        m_downloads.end()
    );
}

void DownloadManager::clearCompleted() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_downloads.erase(
        std::remove_if(m_downloads.begin(), m_downloads.end(),
            [](const DownloadItem& d) {
                return d.status == DownloadStatus::Completed ||
                       d.status == DownloadStatus::Cancelled;
            }),
        m_downloads.end()
    );
}

qint64 DownloadManager::nextId() {
    return m_nextId++;
}

} // namespace Nova
