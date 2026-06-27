#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include "SQLiteDB.h"
#include <memory>
#include <mutex>

namespace Nova {

struct BookmarkFolder {
    qint64 id{0};
    qint64 parentId{0};
    QString name;
    QDateTime createdAt;
};

struct Bookmark {
    qint64 id{0};
    qint64 folderId{0};
    QString url;
    QString title;
    QString favicon;
    QDateTime createdAt;
};

class BookmarkManager : public QObject {
    Q_OBJECT

public:
    static BookmarkManager& instance();

    bool initialize(const QString& dbPath);

    // Bookmarks
    qint64 addBookmark(const QString& url, const QString& title,
                       qint64 folderId = 0, const QString& favicon = {});
    void removeBookmark(qint64 id);
    void updateBookmark(qint64 id, const QString& url, const QString& title);
    bool isBookmarked(const QString& url) const;
    Bookmark bookmarkForUrl(const QString& url) const;

    QVector<Bookmark> bookmarksInFolder(qint64 folderId = 0) const;
    QVector<Bookmark> search(const QString& query) const;
    QVector<Bookmark> allBookmarks() const;

    // Folders
    qint64 createFolder(const QString& name, qint64 parentId = 0);
    void renameFolder(qint64 id, const QString& name);
    void deleteFolder(qint64 id);
    QVector<BookmarkFolder> folders(qint64 parentId = 0) const;

    // Import/Export
    bool exportToJson(const QString& filePath) const;
    bool importFromJson(const QString& filePath);

signals:
    void bookmarksChanged();

private:
    explicit BookmarkManager(QObject* parent = nullptr);
    ~BookmarkManager() override = default;

    BookmarkManager(const BookmarkManager&) = delete;
    BookmarkManager& operator=(const BookmarkManager&) = delete;

    bool createSchema();
    Bookmark bookmarkFromQuery(QSqlQuery& q) const;
    BookmarkFolder folderFromQuery(QSqlQuery& q) const;
    QJsonObject exportFolder(qint64 folderId) const;
    void importFolder(const QJsonObject& obj, qint64 parentId);

    std::unique_ptr<SQLiteDB> m_db;
    mutable std::mutex m_mutex;
};

} // namespace Nova
