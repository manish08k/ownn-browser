#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>

namespace Nova {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void onApply();
    void onBrowseDownloadPath();

private:
    void setupUi();
    void loadValues();
    void saveValues();

    // General tab
    QLineEdit* m_homepage{nullptr};
    QComboBox* m_searchEngine{nullptr};
    QLineEdit* m_downloadPath{nullptr};
    QPushButton* m_browseBtn{nullptr};
    QCheckBox* m_restoreSession{nullptr};
    QCheckBox* m_darkMode{nullptr};

    // Privacy tab
    QCheckBox* m_saveHistory{nullptr};
    QCheckBox* m_saveCookies{nullptr};
    QCheckBox* m_blockThirdParty{nullptr};
    QCheckBox* m_doNotTrack{nullptr};

    // Browser tab
    QCheckBox* m_javascript{nullptr};
    QCheckBox* m_blockPopups{nullptr};
    QSpinBox*  m_cacheSize{nullptr};
};

} // namespace Nova
