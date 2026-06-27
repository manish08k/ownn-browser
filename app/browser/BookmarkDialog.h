#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include "BookmarkManager.h"

namespace Nova {

class BookmarkDialog : public QDialog {
    Q_OBJECT

public:
    explicit BookmarkDialog(QWidget* parent = nullptr);

signals:
    void urlActivated(const QUrl& url);

private slots:
    void onSearch(const QString& query);
    void onFolderSelected(QTreeWidgetItem* item, int column);
    void onBookmarkDoubleClicked(QTreeWidgetItem* item, int column);
    void onAddFolder();
    void onDeleteSelected();
    void onRenameFolder();
    void onImport();
    void onExport();
    void onContextMenu(const QPoint& pos);

private:
    void setupUi();
    void loadFolders();
    void loadBookmarks(qint64 folderId = 0);

    QTreeWidget* m_folderTree{nullptr};
    QTreeWidget* m_bookmarkList{nullptr};
    QLineEdit*   m_searchEdit{nullptr};
    QPushButton* m_addFolderBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
    QPushButton* m_importBtn{nullptr};
    QPushButton* m_exportBtn{nullptr};

    qint64 m_currentFolderId{0};
};

} // namespace Nova
