#include "DownloadDialog.h"
#include "DownloadManager.h"
#include <QHeaderView>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QSet>

namespace Nova {

DownloadDialog::DownloadDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Downloads"); setMinimumSize(700, 400); setupUi();
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(500);
    connect(m_refreshTimer, &QTimer::timeout, this, &DownloadDialog::refresh);
    m_refreshTimer->start();
    refresh();
}

void DownloadDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    m_list = new QTreeWidget(this);
    m_list->setColumnCount(5);
    m_list->setHeaderLabels({"Filename","Size","Progress","Status","URL"});
    m_list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_list->header()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_list->setRootIsDecorated(false);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_list, 1);

    auto* btnRow = new QHBoxLayout();
    m_pauseBtn  = new QPushButton("Pause",  this);
    m_resumeBtn = new QPushButton("Resume", this);
    m_cancelBtn = new QPushButton("Cancel", this);
    m_clearBtn  = new QPushButton("Clear Completed", this);
    btnRow->addWidget(m_pauseBtn); btnRow->addWidget(m_resumeBtn);
    btnRow->addWidget(m_cancelBtn); btnRow->addStretch(); btnRow->addWidget(m_clearBtn);
    layout->addLayout(btnRow);

    connect(m_pauseBtn,  &QPushButton::clicked, this, &DownloadDialog::onPause);
    connect(m_resumeBtn, &QPushButton::clicked, this, &DownloadDialog::onResume);
    connect(m_cancelBtn, &QPushButton::clicked, this, &DownloadDialog::onCancel);
    connect(m_clearBtn,  &QPushButton::clicked, this, &DownloadDialog::onClearCompleted);
    connect(m_list, &QTreeWidget::customContextMenuRequested, this, &DownloadDialog::onContextMenu);
    connect(m_list, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*) { onOpenFile(); });
}

void DownloadDialog::refresh() {
    const auto downloads = DownloadManager::instance().downloads();
    QSet<qint64> seen;
    for (const auto& dl : downloads) {
        seen.insert(dl.id);
        QTreeWidgetItem* item = itemForDownload(dl.id);
        if (!item) { item = new QTreeWidgetItem(m_list); item->setData(0, Qt::UserRole, dl.id); }
        updateItem(item, dl);
    }
    for (int i = m_list->topLevelItemCount()-1; i >= 0; --i) {
        auto* item = m_list->topLevelItem(i);
        if (!seen.contains(item->data(0, Qt::UserRole).toLongLong())) delete item;
    }
}

void DownloadDialog::updateItem(QTreeWidgetItem* item, const DownloadItem& dl) {
    item->setText(0, QFileInfo(dl.filename).fileName());
    const qint64 total = dl.totalBytes, recv = dl.receivedBytes;
    item->setText(1, total > 0 ? QString("%1/%2 MB")
        .arg(recv/1048576.0, 0,'f',1).arg(total/1048576.0, 0,'f',1) : "—");
    item->setText(2, QString("%1%").arg(total>0 ? int(recv*100/total) : 0));
    QString st;
    switch (dl.status) {
        case DownloadStatus::Pending:    st="Pending";     break;
        case DownloadStatus::InProgress: st="Downloading"; break;
        case DownloadStatus::Paused:     st="Paused";      break;
        case DownloadStatus::Completed:  st="Complete";    break;
        case DownloadStatus::Cancelled:  st="Cancelled";   break;
        case DownloadStatus::Failed:     st="Failed";      break;
    }
    item->setText(3, st);
    item->setText(4, dl.url);
    item->setData(0, Qt::UserRole+1, dl.savePath);
}

QTreeWidgetItem* DownloadDialog::itemForDownload(qint64 id) {
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        auto* item = m_list->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toLongLong() == id) return item;
    }
    return nullptr;
}

void DownloadDialog::onPause() {
    auto* item = m_list->currentItem();
    if (item) DownloadManager::instance().pauseDownload(item->data(0, Qt::UserRole).toLongLong());
}
void DownloadDialog::onResume() {
    auto* item = m_list->currentItem();
    if (item) DownloadManager::instance().resumeDownload(item->data(0, Qt::UserRole).toLongLong());
}
void DownloadDialog::onCancel() {
    auto* item = m_list->currentItem();
    if (item) DownloadManager::instance().cancelDownload(item->data(0, Qt::UserRole).toLongLong());
}
void DownloadDialog::onClearCompleted() { DownloadManager::instance().clearCompleted(); refresh(); }

void DownloadDialog::onOpenFile() {
    auto* item = m_list->currentItem();
    if (item) QDesktopServices::openUrl(QUrl::fromLocalFile(item->data(0, Qt::UserRole+1).toString()));
}
void DownloadDialog::onOpenFolder() {
    auto* item = m_list->currentItem();
    if (item) QDesktopServices::openUrl(QUrl::fromLocalFile(
        QFileInfo(item->data(0, Qt::UserRole+1).toString()).absolutePath()));
}

void DownloadDialog::onContextMenu(const QPoint& pos) {
    auto* item = m_list->itemAt(pos);
    if (!item) return;
    QMenu menu(this);
    menu.addAction("Open File",   this, &DownloadDialog::onOpenFile);
    menu.addAction("Open Folder", this, &DownloadDialog::onOpenFolder);
    menu.addAction("Copy URL", this, [item]() {
        QApplication::clipboard()->setText(item->text(4));
    });
    menu.addSeparator();
    menu.addAction("Pause",  this, &DownloadDialog::onPause);
    menu.addAction("Resume", this, &DownloadDialog::onResume);
    menu.addAction("Cancel", this, &DownloadDialog::onCancel);
    menu.exec(m_list->viewport()->mapToGlobal(pos));
}

} // namespace Nova
