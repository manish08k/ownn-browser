#include "SettingsDialog.h"
#include "SettingsManager.h"
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QLabel>

namespace Nova {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Settings"); setMinimumSize(500, 400); setupUi(); loadValues();
}

void SettingsDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    auto* tabs   = new QTabWidget(this);

    auto* generalTab    = new QWidget();
    auto* generalLayout = new QFormLayout(generalTab);
    m_homepage     = new QLineEdit(generalTab);
    m_searchEngine = new QComboBox(generalTab);
    for (const auto& e : SettingsManager::instance().availableSearchEngines())
        m_searchEngine->addItem(e);
    auto* dlRow = new QHBoxLayout();
    m_downloadPath = new QLineEdit(generalTab);
    m_browseBtn    = new QPushButton("Browse...", generalTab);
    dlRow->addWidget(m_downloadPath); dlRow->addWidget(m_browseBtn);
    m_restoreSession = new QCheckBox("Restore previous session on startup", generalTab);
    m_darkMode       = new QCheckBox("Dark mode", generalTab);
    generalLayout->addRow("Homepage:", m_homepage);
    generalLayout->addRow("Default Search Engine:", m_searchEngine);
    generalLayout->addRow("Download Folder:", dlRow);
    generalLayout->addRow("", m_restoreSession);
    generalLayout->addRow("", m_darkMode);
    tabs->addTab(generalTab, "General");

    auto* privacyTab    = new QWidget();
    auto* privacyLayout = new QVBoxLayout(privacyTab);
    m_saveHistory     = new QCheckBox("Save browsing history",     privacyTab);
    m_saveCookies     = new QCheckBox("Accept cookies",            privacyTab);
    m_blockThirdParty = new QCheckBox("Block third-party cookies", privacyTab);
    m_doNotTrack      = new QCheckBox("Send Do Not Track request", privacyTab);
    privacyLayout->addWidget(m_saveHistory); privacyLayout->addWidget(m_saveCookies);
    privacyLayout->addWidget(m_blockThirdParty); privacyLayout->addWidget(m_doNotTrack);
    privacyLayout->addStretch();
    tabs->addTab(privacyTab, "Privacy");

    auto* browserTab    = new QWidget();
    auto* browserLayout = new QFormLayout(browserTab);
    m_javascript  = new QCheckBox("Enable JavaScript",    browserTab);
    m_blockPopups = new QCheckBox("Block pop-up windows", browserTab);
    m_cacheSize   = new QSpinBox(browserTab);
    m_cacheSize->setRange(50, 4096); m_cacheSize->setSuffix(" MB");
    browserLayout->addRow("", m_javascript);
    browserLayout->addRow("", m_blockPopups);
    browserLayout->addRow("Cache size:", m_cacheSize);
    tabs->addTab(browserTab, "Browser");

    layout->addWidget(tabs, 1);

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    layout->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, this, [this]() { onApply(); accept(); });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(btns->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseDownloadPath);
}

void SettingsDialog::loadValues() {
    auto& s = SettingsManager::instance();
    m_homepage->setText(s.homepage());
    m_searchEngine->setCurrentText(s.defaultSearchEngine());
    m_downloadPath->setText(s.downloadPath());
    m_restoreSession->setChecked(s.restoreSession());
    m_darkMode->setChecked(s.darkMode());
    m_saveHistory->setChecked(s.saveHistory());
    m_saveCookies->setChecked(s.saveCookies());
    m_blockThirdParty->setChecked(s.blockThirdPartyCookies());
    m_doNotTrack->setChecked(s.enableDoNotTrack());
    m_javascript->setChecked(s.javascriptEnabled());
    m_blockPopups->setChecked(s.blockPopups());
    m_cacheSize->setValue(s.maxCacheSizeMB());
}

void SettingsDialog::saveValues() {
    auto& s = SettingsManager::instance();
    s.setHomepage(m_homepage->text().trimmed());
    s.setDefaultSearchEngine(m_searchEngine->currentText());
    s.setDownloadPath(m_downloadPath->text().trimmed());
    s.setRestoreSession(m_restoreSession->isChecked());
    s.setDarkMode(m_darkMode->isChecked());
    s.setSaveHistory(m_saveHistory->isChecked());
    s.setSaveCookies(m_saveCookies->isChecked());
    s.setBlockThirdPartyCookies(m_blockThirdParty->isChecked());
    s.setEnableDoNotTrack(m_doNotTrack->isChecked());
    s.setJavascriptEnabled(m_javascript->isChecked());
    s.setBlockPopups(m_blockPopups->isChecked());
    s.setMaxCacheSizeMB(m_cacheSize->value());
    s.sync();
}

void SettingsDialog::onApply() { saveValues(); emit settingsApplied(); }

void SettingsDialog::onBrowseDownloadPath() {
    const QString path = QFileDialog::getExistingDirectory(this, "Select Download Folder", m_downloadPath->text());
    if (!path.isEmpty()) m_downloadPath->setText(path);
}

} // namespace Nova
