#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateEdit>
#include <QMenu>
#include "HistoryManager.h"

namespace Nova {

class HistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit HistoryDialog(QWidget* parent = nullptr);

signals:
    void urlActivated(const QUrl& url);

private slots:
    void onSearch(const QString& query);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onDeleteSelected();
    void onClearAll();
    void onContextMenu(const QPoint& pos);

private:
    void setupUi();
    void loadHistory(const QString& filter = {});
    void populateTree(const QVector<HistoryEntry>& entries);

    QLineEdit*   m_searchEdit{nullptr};
    QTreeWidget* m_tree{nullptr};
    QPushButton* m_deleteBtn{nullptr};
    QPushButton* m_clearAllBtn{nullptr};
};

} // namespace Nova
