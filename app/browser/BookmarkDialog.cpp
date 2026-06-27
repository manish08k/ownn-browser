#include "BookmarkDialog.h"
#include "BookmarkManager.h"
#include <QHeaderView>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMenu>

namespace Nova {

BookmarkDialog::BookmarkDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Bookmarks"); setMinimumSize(800, 500); setupUi(); loadFolders(); loadBookmarks(0);
}

void BookmarkDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    auto* searchRow = new QHBoxLayout();
    searchRow->addWidget(new QLabel("Search:", this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search bookmarks...");
    m_searchEdit->setClearButtonEnabled(true);
    searchRow->addWidget(m_searchEdit);
    layout->addLayout(searchRow);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    m_folderTree = new QTreeWidget(splitter);
    m_folderTree->setHeaderLabel("Folders");
    m_folderTree->setRootIsDecorated(true);

    m_bookmarkList = new QTreeWidget(splitter);
    m_bookmarkList->setColumnCount(2);
    m_bookmarkList->setHeaderLabels({"Title","URL"});
    m_bookmarkList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_bookmarkList->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_bookmarkList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_bookmarkList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_bookmarkList->setRootIsDecorated(false);

    splitter->addWidget(m_folderTree);
    splitter->addWidget(m_bookmarkList);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    layout->addWidget(splitter, 1);

    auto* btnRow = new QHBoxLayout();
    m_addFolderBtn = new QPushButton("New Folder", this);
    m_deleteBtn    = new QPushButton("Delete Selected", this);
    m_importBtn    = new QPushButton("Import...", this);
    m_exportBtn    = new QPushButton("Export...", this);
    btnRow->addWidget(m_addFolderBtn); btnRow->addWidget(m_deleteBtn);
    btnRow->addStretch(); btnRow->addWidget(m_importBtn); btnRow->addWidget(m_exportBtn);
    layout->addLayout(btnRow);

    connect(m_searchEdit,   &QLineEdit::textChanged,            this, &BookmarkDialog::onSearch);
    connect(m_folderTree,   &QTreeWidget::itemClicked,          this, &BookmarkDialog::onFolderSelected);
    connect(m_folderTree,   &QTreeWidget::itemDoubleClicked,    this, &BookmarkDialog::onRenameFolder);
    connect(m_bookmarkList, &QTreeWidget::itemDoubleClicked,    this, &BookmarkDialog::onBookmarkDoubleClicked);
    connect(m_addFolderBtn, &QPushButton::clicked,              this, &BookmarkDialog::onAddFolder);
    connect(m_deleteBtn,    &QPushButton::clicked,              this, &BookmarkDialog::onDeleteSelected);
    connect(m_importBtn,    &QPushButton::clicked,              this, &BookmarkDialog::onImport);
    connect(m_exportBtn,    &QPushButton::clicked,              this, &BookmarkDialog::onExport);
    connect(m_bookmarkList, &QTreeWidget::customContextMenuRequested, this, &BookmarkDialog::onContextMenu);
}

void BookmarkDialog::loadFolders() {
    m_folderTree->clear();
    auto* root = new QTreeWidgetItem(m_folderTree);
    root->setText(0, "All Bookmarks");
    root->setData(0, Qt::UserRole, qint64(0));

    std::function<void(QTreeWidgetItem*, qint64)> addFolders = [&](QTreeWidgetItem* parent, qint64 parentId) {
        for (const auto& f : BookmarkManager::instance().folders(parentId)) {
            auto* item = new QTreeWidgetItem(parent);
            item->setText(0, f.name);
            item->setData(0, Qt::UserRole, f.id);
            addFolders(item, f.id);
        }
    };
    addFolders(root, 0);
    m_folderTree->expandAll();
}

void BookmarkDialog::loadBookmarks(qint64 folderId) {
    m_currentFolderId = folderId;
    m_bookmarkList->clear();
    for (const auto& bm : BookmarkManager::instance().bookmarksInFolder(folderId)) {
        auto* item = new QTreeWidgetItem(m_bookmarkList);
        item->setText(0, bm.title.isEmpty() ? bm.url : bm.title);
        item->setText(1, bm.url);
        item->setData(0, Qt::UserRole, bm.id);
        item->setData(1, Qt::UserRole, bm.url);
    }
}

void BookmarkDialog::onSearch(const QString& query) {
    if (query.isEmpty()) { loadBookmarks(m_currentFolderId); return; }
    m_bookmarkList->clear();
    for (const auto& bm : BookmarkManager::instance().search(query)) {
        auto* item = new QTreeWidgetItem(m_bookmarkList);
        item->setText(0, bm.title.isEmpty() ? bm.url : bm.title);
        item->setText(1, bm.url);
        item->setData(0, Qt::UserRole, bm.id);
        item->setData(1, Qt::UserRole, bm.url);
    }
}

void BookmarkDialog::onFolderSelected(QTreeWidgetItem* item, int) {
    if (item) loadBookmarks(item->data(0, Qt::UserRole).toLongLong());
}

void BookmarkDialog::onBookmarkDoubleClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    emit urlActivated(QUrl(item->data(1, Qt::UserRole).toString()));
    accept();
}

void BookmarkDialog::onAddFolder() {
    bool ok;
    const QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, {}, &ok);
    if (ok && !name.isEmpty()) { BookmarkManager::instance().createFolder(name, m_currentFolderId); loadFolders(); }
}

void BookmarkDialog::onDeleteSelected() {
    for (auto* item : m_bookmarkList->selectedItems()) {
        BookmarkManager::instance().removeBookmark(item->data(0, Qt::UserRole).toLongLong());
        delete item;
    }
}

void BookmarkDialog::onRenameFolder() {
    auto* item = m_folderTree->currentItem();
    if (!item) return;
    qint64 fid = item->data(0, Qt::UserRole).toLongLong();
    if (fid == 0) return;
    bool ok;
    const QString name = QInputDialog::getText(this, "Rename Folder", "New name:", QLineEdit::Normal, item->text(0), &ok);
    if (ok && !name.isEmpty()) { BookmarkManager::instance().renameFolder(fid, name); item->setText(0, name); }
}

void BookmarkDialog::onImport() {
    const QString path = QFileDialog::getOpenFileName(this, "Import Bookmarks", {}, "JSON Files (*.json);;All Files (*)");
    if (!path.isEmpty()) {
        if (BookmarkManager::instance().importFromJson(path)) { loadFolders(); loadBookmarks(m_currentFolderId); }
        else QMessageBox::warning(this, "Import Failed", "Could not import bookmarks.");
    }
}

void BookmarkDialog::onExport() {
    const QString path = QFileDialog::getSaveFileName(this, "Export Bookmarks", "bookmarks.json", "JSON Files (*.json)");
    if (!path.isEmpty() && !BookmarkManager::instance().exportToJson(path))
        QMessageBox::warning(this, "Export Failed", "Could not export bookmarks.");
}

void BookmarkDialog::onContextMenu(const QPoint& pos) {
    auto* item = m_bookmarkList->itemAt(pos);
    if (!item) return;
    QMenu menu(this);
    menu.addAction("Open in New Tab", this, [this, item]() {
        emit urlActivated(QUrl(item->data(1, Qt::UserRole).toString()));
    });
    menu.addAction("Copy URL", this, [item]() {
        QApplication::clipboard()->setText(item->data(1, Qt::UserRole).toString());
    });
    menu.addSeparator();
    menu.addAction("Delete", this, &BookmarkDialog::onDeleteSelected);
    menu.exec(m_bookmarkList->viewport()->mapToGlobal(pos));
}

} // namespace Nova
