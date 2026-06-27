#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include "BrowserEngine.h"
#include "HistoryManager.h"
#include "CookieManager.h"
#include "CacheManager.h"
#include <QDateTime>

namespace Nova {

class ClearDataDialog : public QDialog {
    Q_OBJECT

public:
    explicit ClearDataDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Clear Browsing Data");
        setFixedSize(400, 300);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("<b>Clear the following items:</b>", this));

        m_timeRange = new QComboBox(this);
        m_timeRange->addItems({"Last hour", "Last 24 hours", "Last 7 days", "Last 4 weeks", "All time"});
        m_timeRange->setCurrentIndex(4);

        auto* form = new QFormLayout();
        form->addRow("Time range:", m_timeRange);
        layout->addLayout(form);

        m_history  = new QCheckBox("Browsing history",     this);
        m_cookies  = new QCheckBox("Cookies",              this);
        m_cache    = new QCheckBox("Cached images & files",this);
        m_history->setChecked(true);
        m_cookies->setChecked(true);
        m_cache->setChecked(true);

        layout->addWidget(m_history);
        layout->addWidget(m_cookies);
        layout->addWidget(m_cache);
        layout->addStretch();

        auto* btns = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(btns);

        connect(btns, &QDialogButtonBox::accepted, this, [this]() {
            QDateTime cutoff;
            switch (m_timeRange->currentIndex()) {
                case 0: cutoff = QDateTime::currentDateTime().addSecs(-3600);        break;
                case 1: cutoff = QDateTime::currentDateTime().addSecs(-86400);       break;
                case 2: cutoff = QDateTime::currentDateTime().addDays(-7);           break;
                case 3: cutoff = QDateTime::currentDateTime().addDays(-28);          break;
                default: cutoff = QDateTime(); break; // all time
            }

            if (m_history->isChecked()) {
                if (cutoff.isValid())
                    HistoryManager::instance().clearOlderThan(cutoff);
                else
                    HistoryManager::instance().clearAll();
            }
            if (m_cookies->isChecked()) CookieManager::instance().deleteAllCookies();
            if (m_cache->isChecked())   CacheManager::instance().clearAll();

            accept();
        });
        connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

private:
    QComboBox* m_timeRange{nullptr};
    QCheckBox* m_history{nullptr};
    QCheckBox* m_cookies{nullptr};
    QCheckBox* m_cache{nullptr};
};

} // namespace Nova
