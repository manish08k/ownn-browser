#include "BookmarkManager.h"
#include "Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlQuery>

namespace Nova {

BookmarkManager& BookmarkManager::instance() {
    static BookmarkManager inst;
    return inst;
}

BookmarkManager::BookmarkManager(QObject* parent) : QObject(parent) {
    m_db = std::make_unique<SQLiteDB>("bookmark_db");
}

bool BookmarkManager::initialize(const QString& dbPath) {
    if (!m_db->open(dbPath)) {
        LOG_ERROR("BookmarkManager", "Cannot open database: " + dbPath);
        return false;
    }
    return createSchema();
}

bool BookmarkManager::createSchema() {
    bool ok = m_db->execute(
        "CREATE TABLE IF NOT EXISTS bookmark_folders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_id INTEGER NOT NULL DEFAULT 0,"
        "name TEXT NOT NULL,"
        "created_at INTEGER NOT NULL)"
    );
    ok &= m_db->execute(
        "CREATE TABLE IF NOT EXISTS bookmarks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "folder_id INTEGER NOT NULL DEFAULT 0,"
        "url TEXT NOT NULL,"
        "title TEXT,"
        "favicon TEXT,"
        "created_at INTEGER NOT NULL,"
        "FOREIGN KEY(folder_id) REFERENCES bookmark_folders(id) ON DELETE CASCADE)"
    );
    ok &= m_db->execute("CREATE INDEX IF NOT EXISTS idx_bm_url ON bookmarks(url)");
    ok &= m_db->execute("CREATE INDEX IF NOT EXISTS idx_bm_folder ON bookmarks(folder_id)");
    return ok;
}

qint64 BookmarkManager::addBookmark(const QString& url, const QString& title,
                                     qint64 folderId, const QString& favicon) {
    qint64 id = -1;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute(
            "INSERT INTO bookmarks (folder_id,url,title,favicon,created_at) VALUES (?,?,?,?,?)",
            {folderId, url, title, favicon, QDateTime::currentSecsSinceEpoch()}
        );
        id = m_db->lastInsertId();
    }
    emit bookmarksChanged();
    return id;
}

void BookmarkManager::removeBookmark(qint64 id) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("DELETE FROM bookmarks WHERE id=?", {id});
    }
    emit bookmarksChanged();
}

void BookmarkManager::updateBookmark(qint64 id, const QString& url, const QString& title) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("UPDATE bookmarks SET url=?,title=? WHERE id=?", {url, title, id});
    }
    emit bookmarksChanged();
}

bool BookmarkManager::isBookmarked(const QString& url) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query("SELECT COUNT(*) FROM bookmarks WHERE url=?", {url});
    return q.next() && q.value(0).toInt() > 0;
}

Bookmark BookmarkManager::bookmarkForUrl(const QString& url) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query(
        "SELECT id,folder_id,url,title,favicon,created_at FROM bookmarks WHERE url=? LIMIT 1",
        {url}
    );
    if (q.next()) return bookmarkFromQuery(q);
    return {};
}

QVector<Bookmark> BookmarkManager::bookmarksInFolder(qint64 folderId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query(
        "SELECT id,folder_id,url,title,favicon,created_at FROM bookmarks "
        "WHERE folder_id=? ORDER BY created_at DESC",
        {folderId}
    );
    QVector<Bookmark> results;
    while (q.next()) results.append(bookmarkFromQuery(q));
    return results;
}

QVector<Bookmark> BookmarkManager::search(const QString& query) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const QString pattern = "%" + query + "%";
    auto q = m_db->query(
        "SELECT id,folder_id,url,title,favicon,created_at FROM bookmarks "
        "WHERE url LIKE ? OR title LIKE ? ORDER BY created_at DESC",
        {pattern, pattern}
    );
    QVector<Bookmark> results;
    while (q.next()) results.append(bookmarkFromQuery(q));
    return results;
}

QVector<Bookmark> BookmarkManager::allBookmarks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query(
        "SELECT id,folder_id,url,title,favicon,created_at FROM bookmarks ORDER BY created_at DESC"
    );
    QVector<Bookmark> results;
    while (q.next()) results.append(bookmarkFromQuery(q));
    return results;
}

qint64 BookmarkManager::createFolder(const QString& name, qint64 parentId) {
    qint64 id = -1;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute(
            "INSERT INTO bookmark_folders (parent_id,name,created_at) VALUES (?,?,?)",
            {parentId, name, QDateTime::currentSecsSinceEpoch()}
        );
        id = m_db->lastInsertId();
    }
    emit bookmarksChanged();
    return id;
}

void BookmarkManager::renameFolder(qint64 id, const QString& name) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("UPDATE bookmark_folders SET name=? WHERE id=?", {name, id});
    }
    emit bookmarksChanged();
}

void BookmarkManager::deleteFolder(qint64 id) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_db->execute("DELETE FROM bookmark_folders WHERE id=?", {id});
        m_db->execute("DELETE FROM bookmarks WHERE folder_id=?", {id});
    }
    emit bookmarksChanged();
}

QVector<BookmarkFolder> BookmarkManager::folders(qint64 parentId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto q = m_db->query(
        "SELECT id,parent_id,name,created_at FROM bookmark_folders WHERE parent_id=?",
        {parentId}
    );
    QVector<BookmarkFolder> results;
    while (q.next()) results.append(folderFromQuery(q));
    return results;
}

bool BookmarkManager::exportToJson(const QString& filePath) const {
    QJsonObject root = exportFolder(0);
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) {
        LOG_ERROR("BookmarkManager", "Cannot open export file: " + filePath);
        return false;
    }
    f.write(QJsonDocument(root).toJson());
    return true;
}

bool BookmarkManager::importFromJson(const QString& filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        LOG_ERROR("BookmarkManager", "Cannot open import file: " + filePath);
        return false;
    }
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        LOG_ERROR("BookmarkManager", "JSON parse error: " + err.errorString());
        return false;
    }
    importFolder(doc.object(), 0);
    emit bookmarksChanged();
    return true;
}

QJsonObject BookmarkManager::exportFolder(qint64 folderId) const {
    QJsonObject obj;

    auto subFolders = m_db->query(
        "SELECT id,parent_id,name,created_at FROM bookmark_folders WHERE parent_id=?",
        {folderId}
    );
    QJsonArray foldersArr;
    while (subFolders.next()) {
        QJsonObject f;
        f["id"] = subFolders.value(0).toLongLong();
        f["name"] = subFolders.value(2).toString();
        f["children"] = exportFolder(subFolders.value(0).toLongLong());
        foldersArr.append(f);
    }
    obj["folders"] = foldersArr;

    auto bms = m_db->query(
        "SELECT url,title,favicon FROM bookmarks WHERE folder_id=?", {folderId}
    );
    QJsonArray bmsArr;
    while (bms.next()) {
        QJsonObject b;
        b["url"] = bms.value(0).toString();
        b["title"] = bms.value(1).toString();
        b["favicon"] = bms.value(2).toString();
        bmsArr.append(b);
    }
    obj["bookmarks"] = bmsArr;
    return obj;
}

void BookmarkManager::importFolder(const QJsonObject& obj, qint64 parentId) {
    for (const auto& bm : obj["bookmarks"].toArray()) {
        auto b = bm.toObject();
        addBookmark(b["url"].toString(), b["title"].toString(), parentId, b["favicon"].toString());
    }
    for (const auto& folder : obj["folders"].toArray()) {
        auto f = folder.toObject();
        qint64 fid = createFolder(f["name"].toString(), parentId);
        importFolder(f["children"].toObject(), fid);
    }
}

Bookmark BookmarkManager::bookmarkFromQuery(QSqlQuery& q) const {
    Bookmark b;
    b.id        = q.value(0).toLongLong();
    b.folderId  = q.value(1).toLongLong();
    b.url       = q.value(2).toString();
    b.title     = q.value(3).toString();
    b.favicon   = q.value(4).toString();
    b.createdAt = QDateTime::fromSecsSinceEpoch(q.value(5).toLongLong());
    return b;
}

BookmarkFolder BookmarkManager::folderFromQuery(QSqlQuery& q) const {
    BookmarkFolder f;
    f.id        = q.value(0).toLongLong();
    f.parentId  = q.value(1).toLongLong();
    f.name      = q.value(2).toString();
    f.createdAt = QDateTime::fromSecsSinceEpoch(q.value(3).toLongLong());
    return f;
}

} // namespace Nova
