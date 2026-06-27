#include "HistoryDialog.h"
#include "HistoryManager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QUrl>
#include <QApplication>
#include <QClipboard>
#include <QMenu>

namespace Nova {

HistoryDialog::HistoryDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("History"); setMinimumSize(700, 500); setupUi(); loadHistory();
}

void HistoryDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel("Search:", this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search history...");
    m_searchEdit->setClearButtonEnabled(true);
    row->addWidget(m_searchEdit);
    layout->addLayout(row);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({"Title","URL","Last Visit","Visits"});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setRootIsDecorated(false);
    layout->addWidget(m_tree);

    auto* btnRow = new QHBoxLayout();
    m_deleteBtn   = new QPushButton("Delete Selected", this);
    m_clearAllBtn = new QPushButton("Clear All History", this);
    btnRow->addWidget(m_deleteBtn); btnRow->addStretch(); btnRow->addWidget(m_clearAllBtn);
    layout->addLayout(btnRow);

    connect(m_searchEdit, &QLineEdit::textChanged,   this, &HistoryDialog::onSearch);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &HistoryDialog::onItemDoubleClicked);
    connect(m_deleteBtn,   &QPushButton::clicked,    this, &HistoryDialog::onDeleteSelected);
    connect(m_clearAllBtn, &QPushButton::clicked,    this, &HistoryDialog::onClearAll);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &HistoryDialog::onContextMenu);
}

void HistoryDialog::loadHistory(const QString& filter) {
    auto entries = filter.isEmpty()
        ? HistoryManager::instance().recentHistory(500)
        : HistoryManager::instance().search(filter, 500);
    populateTree(entries);
}

void HistoryDialog::populateTree(const QVector<HistoryEntry>& entries) {
    m_tree->clear();
    for (const auto& e : entries) {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, e.title.isEmpty() ? e.url : e.title);
        item->setText(1, e.url);
        item->setText(2, e.visitTime.toString("yyyy-MM-dd hh:mm"));
        item->setText(3, QString::number(e.visitCount));
        item->setData(0, Qt::UserRole, e.id);
        item->setData(1, Qt::UserRole, e.url);
    }
}

void HistoryDialog::onSearch(const QString& q) { loadHistory(q); }

void HistoryDialog::onItemDoubleClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    emit urlActivated(QUrl(item->data(1, Qt::UserRole).toString()));
    accept();
}

void HistoryDialog::onDeleteSelected() {
    for (auto* item : m_tree->selectedItems()) {
        HistoryManager::instance().deleteEntry(item->data(0, Qt::UserRole).toLongLong());
        delete item;
    }
}

void HistoryDialog::onClearAll() {
    if (QMessageBox::question(this, "Clear History", "Clear all browsing history?",
            QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        HistoryManager::instance().clearAll();
        m_tree->clear();
    }
}

void HistoryDialog::onContextMenu(const QPoint& pos) {
    auto* item = m_tree->itemAt(pos);
    if (!item) return;
    QMenu menu(this);
    menu.addAction("Open in New Tab", this, [this, item]() {
        emit urlActivated(QUrl(item->data(1, Qt::UserRole).toString()));
    });
    menu.addAction("Copy URL", this, [item]() {
        QApplication::clipboard()->setText(item->data(1, Qt::UserRole).toString());
    });
    menu.addSeparator();
    menu.addAction("Delete", this, &HistoryDialog::onDeleteSelected);
    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}

} // namespace Nova
