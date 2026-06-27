#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include "DownloadManager.h"

namespace Nova {

class DownloadDialog : public QDialog {
    Q_OBJECT

public:
    explicit DownloadDialog(QWidget* parent = nullptr);

private slots:
    void refresh();
    void onPause();
    void onResume();
    void onCancel();
    void onClearCompleted();
    void onOpenFile();
    void onOpenFolder();
    void onContextMenu(const QPoint& pos);

private:
    void setupUi();
    void updateItem(QTreeWidgetItem* item, const DownloadItem& dl);
    QTreeWidgetItem* itemForDownload(qint64 id);

    QTreeWidget*  m_list{nullptr};
    QPushButton*  m_pauseBtn{nullptr};
    QPushButton*  m_resumeBtn{nullptr};
    QPushButton*  m_cancelBtn{nullptr};
    QPushButton*  m_clearBtn{nullptr};
    QTimer*       m_refreshTimer{nullptr};
};

} // namespace Nova
