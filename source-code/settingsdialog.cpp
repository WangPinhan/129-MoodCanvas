#include "settingsdialog.h"
#include "cloudapiclient.h"
#include "llmclient.h"
#include "llmconfig.h"
#include "thememanager.h"

#include <QSignalBlocker>
#include <QLayout>
#include <QScrollArea>
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QComboBox>

SettingsDialog::SettingsDialog(DataStore *store, QWidget *parent)
    : QDialog(parent),
      m_store(store)
{
    setWindowTitle("设置 / 隐私");
    resize(760, 560);
    setObjectName(QStringLiteral("settingsDialog"));
/**
    setStyleSheet(
        "QDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f8fbff, stop:1 #fff8f2); }"
        "QFrame#card { background: rgba(255,255,255,242); border: 1px solid #e6eef2; border-radius: 24px; }"
        "QTextEdit { background: white; border: 1px solid #e6eef2; border-radius: 20px; padding: 14px; }"
        "QLineEdit { border: 1px solid #dfe9ec; border-radius: 16px; padding: 9px 12px; background: #fbfdff; }"
        "QPushButton { border-radius: 18px; padding: 9px 18px; border: 1px solid #dce8ef; background: white; }"
        "QPushButton:hover { background: #f1fbf7; }"
    );
*/

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    outerLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    outerLayout->addWidget(scrollArea);
    auto *contentWidget = new QWidget(scrollArea);
    scrollArea->setWidget(contentWidget);
    contentWidget->setObjectName(QStringLiteral("settingsContentWidget"));
    auto *layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    auto *title = new QLabel("设置 / 隐私", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setObjectName(QStringLiteral("settingsTitleLabel"));
    layout->addWidget(title);

    //add by HYH for thememanager

    auto *themeRow = new QHBoxLayout();
    auto *themeLabel = new QLabel(QStringLiteral("外观设置"), this);
    themeLabel->setObjectName(QStringLiteral("themeLabel"));
    themeRow->addWidget(themeLabel);
    themeRow->addStretch();

    QComboBox *themeCombo = new QComboBox(this);
    themeCombo->addItem("跟随系统", static_cast<int>(ThemeManager::Auto));
    themeCombo->addItem("浅色",     static_cast<int>(ThemeManager::Light));
    themeCombo->addItem("深色",     static_cast<int>(ThemeManager::Dark));

    ThemeManager &tm = ThemeManager::instance();

    const int currentThemeIndex = themeCombo->findData(static_cast<int>(tm.themeMode()));
    themeCombo->setCurrentIndex(currentThemeIndex >= 0 ? currentThemeIndex : 0);

    connect(themeCombo, QOverload<int>::of(&QComboBox::activated),
            this, [themeCombo](int index) {
                const ThemeManager::ThemeMode mode =
                    static_cast<ThemeManager::ThemeMode>(themeCombo->itemData(index).toInt());

                ThemeManager::instance().setThemeMode(mode);
            });

    connect(&tm, &ThemeManager::themeChanged,
            this, [themeCombo]() {
                const QSignalBlocker blocker(themeCombo);

                const int index = themeCombo->findData(
                    static_cast<int>(ThemeManager::instance().themeMode()));

                if (index >= 0) {
                    themeCombo->setCurrentIndex(index);
                }
            });


    themeRow->addWidget(themeCombo);
    layout->addLayout(themeRow);
    //end

    QFont sectionFont = title->font();
    sectionFont.setBold(true);
    sectionFont.setPointSize(13);

    const CloudConfig cloud = m_store->loadCloudConfig();
    auto *cloudCard = new QFrame(this);
    cloudCard->setObjectName(QStringLiteral("card"));
    auto *cloudLayout = new QFormLayout(cloudCard);
    cloudLayout->setContentsMargins(22, 20, 22, 20);
    cloudLayout->setSpacing(12);
    auto *cloudTitle = new QLabel(QStringLiteral("云端账号与树洞"), cloudCard);
    cloudTitle->setFont(sectionFont);
    cloudLayout->addRow(cloudTitle);

    m_cloudServerEdit = new QLineEdit(cloudCard);
    m_cloudServerEdit->setPlaceholderText(QStringLiteral("http://127.0.0.1:8765"));
    m_cloudServerEdit->setText(cloud.serverUrl);
    cloudLayout->addRow(QStringLiteral("云端地址"), m_cloudServerEdit);

    auto *cloudHint = new QLabel(
        QStringLiteral("用户注册/登录与树洞互动都走云端数据库。\n"
                       "本机测试：先运行 server\\start_server.bat。\n"
                       "多人联机：填云服务器公网地址，如 http://你的IP:8765"),
        cloudCard);
    cloudHint->setWordWrap(true);
    cloudLayout->addRow(cloudHint);

    auto *saveCloudBtn = new QPushButton(QStringLiteral("保存云端设置"), cloudCard);
    cloudLayout->addRow(saveCloudBtn);
    connect(saveCloudBtn, &QPushButton::clicked, this, &SettingsDialog::saveCloudSettings);

    auto *testCloudBtn = new QPushButton(QStringLiteral("测试云端连接"), cloudCard);
    cloudLayout->addRow(testCloudBtn);
    connect(testCloudBtn, &QPushButton::clicked, this, &SettingsDialog::testCloudConnection);
    layout->addWidget(cloudCard);

    const LlmConfig llm = m_store->resolveLlmConfig();
    auto *llmCard = new QFrame(this);
    llmCard->setObjectName(QStringLiteral("card"));
    auto *llmLayout = new QFormLayout(llmCard);
    llmLayout->setContentsMargins(22, 20, 22, 20);
    llmLayout->setSpacing(12);
    auto *llmTitle = new QLabel(QStringLiteral("大模型语义分析"), llmCard);
    llmTitle->setFont(sectionFont);
    llmLayout->addRow(llmTitle);
    m_llmEnabledCheck = new QCheckBox(QStringLiteral("启用大模型分析日记（需联网）"), llmCard);
    m_llmEnabledCheck->setChecked(llm.enabled);
    llmLayout->addRow(m_llmEnabledCheck);

    m_llmApiKeyEdit = new QLineEdit(llmCard);
    m_llmApiKeyEdit->setPlaceholderText(QStringLiteral("DeepSeek 密钥，格式类似 sk-xxxxxxxx（不是 exe 路径）"));
    m_llmApiKeyEdit->setEchoMode(QLineEdit::Password);
    if (LlmConfig::isValidApiKey(llm.apiKey)) {
        m_llmApiKeyEdit->setText(llm.apiKey);
    } else if (llm.hasApiKey()) {
        m_llmApiKeyEdit->clear();
        m_llmApiKeyEdit->setPlaceholderText(QStringLiteral("当前保存的密钥无效，请重新填写 sk- 开头的 DeepSeek Key"));
    }
    m_llmBaseUrlEdit = new QLineEdit(llmCard);
    m_llmBaseUrlEdit->setPlaceholderText(QStringLiteral("https://api.deepseek.com"));
    m_llmBaseUrlEdit->setText(llm.baseUrl);

    m_llmModelEdit = new QLineEdit(llmCard);
    m_llmModelEdit->setPlaceholderText(QStringLiteral("deepseek-v4-flash"));
    m_llmModelEdit->setText(llm.model);
    llmLayout->addRow(QStringLiteral("API Key"), m_llmApiKeyEdit);
    llmLayout->addRow(QStringLiteral("接口地址"), m_llmBaseUrlEdit);
    llmLayout->addRow(QStringLiteral("模型名称"), m_llmModelEdit);
    auto *llmHint = new QLabel(
        QStringLiteral("请根据服务商填写 API Key、接口地址和模型名称。\n"
                       "当前版本只使用在线 API 分析；未配置或调用失败时不会进行离线分析。"),
        llmCard);
    llmHint->setWordWrap(true);
    llmHint->setObjectName(QStringLiteral("llmHintLabel"));
    llmLayout->addRow(llmHint);
    auto *saveLlmBtn = new QPushButton(QStringLiteral("保存大模型设置"), llmCard);
    saveLlmBtn->setObjectName(QStringLiteral("saveLlmBtn"));
    llmLayout->addRow(saveLlmBtn);
    connect(saveLlmBtn, &QPushButton::clicked, this, &SettingsDialog::saveLlmSettings);
    auto *testLlmBtn = new QPushButton(QStringLiteral("测试大模型连接"), llmCard);
    llmLayout->addRow(testLlmBtn);
    connect(testLlmBtn, &QPushButton::clicked, this, &SettingsDialog::testLlmConnection);

    if (llm.hasApiKey() && !LlmConfig::isValidApiKey(llm.apiKey)) {
        QMessageBox::warning(this,
                             QStringLiteral("API Key 无效"),
                             QStringLiteral("检测到设置里保存的不是 DeepSeek 密钥（可能误填了 exe 路径或命令）。\n"
                                              "请到 platform.deepseek.com 创建 API Key，格式类似 sk-xxxxxxxx，重新填写并保存。"));
    }
    layout->addWidget(llmCard);
    auto *privacyText = new QLabel(this);
    privacyText->setWordWrap(true);
    privacyText->setText(
        QStringLiteral("隐私与数据说明\n"
                       "1. 日记、情绪分析结果仍保存在本机；用户账号与树洞内容保存在云端数据库。\n"
                       "2. 若启用大模型，仅将您输入的日记文本发送至所配置的 API 服务商；API Key 保存在本机设置文件。\n"
                       "3. 本项目不是医疗诊断工具，只提供情绪记录、观察和轻量化调节建议。")
        );
    privacyText->setObjectName(QStringLiteral("privacyTextLabel"));
    layout->addWidget(privacyText);
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto *exportBtn = new QPushButton("导出本地 JSON 数据", this);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::exportData);
    buttonLayout->addWidget(exportBtn);
    layout->addLayout(buttonLayout);


}
void SettingsDialog::saveCloudSettings()
{
    CloudConfig config = m_store->loadCloudConfig();
    QString url = m_cloudServerEdit->text().trimmed();
    if (url.isEmpty()) {
        url = QStringLiteral("http://127.0.0.1:8765");
    }
    config.serverUrl = url;
    if (m_store->saveCloudConfig(config)) {
        QMessageBox::information(this,
                                 QStringLiteral("保存成功"),
                                 QStringLiteral("云端地址已保存。重新登录后生效。"));
    } else {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("无法写入本地设置文件。"));
    }
}
void SettingsDialog::testCloudConnection()
{
    CloudApiClient api;
    QString url = m_cloudServerEdit->text().trimmed();
    if (url.isEmpty()) {
        url = QStringLiteral("http://127.0.0.1:8765");
    }
    api.setServerUrl(url);

    QString error;
    if (api.checkHealth(&error)) {
        QMessageBox::information(this,
                                 QStringLiteral("连接成功"),
                                 QStringLiteral("云端服务可用：%1").arg(url));
    } else {
        QMessageBox::warning(this, QStringLiteral("连接失败"), error);
    }
}
void SettingsDialog::saveLlmSettings()
{
    LlmConfig config;
    config.apiKey = m_llmApiKeyEdit->text().trimmed();
    config.baseUrl = m_llmBaseUrlEdit->text().trimmed();
    config.model = m_llmModelEdit->text().trimmed();
    if (config.apiKey.isEmpty()) {
        config.enabled = false;
    } else if (!LlmConfig::isValidApiKey(config.apiKey)) {
        QMessageBox::warning(this,
                             QStringLiteral("API Key 格式不对"),
                             QStringLiteral("请填写 DeepSeek 平台生成的密钥（以 sk- 开头），\n"
                                              "不要填写 exe 路径、PowerShell 命令或网址。"));
        return;
    } else {
        config.enabled = true;
    }
    if (config.baseUrl.isEmpty()) {
        config.baseUrl = QStringLiteral("https://api.deepseek.com");
    }
    if (config.model.isEmpty()) {
        config.model = QStringLiteral("deepseek-chat");
    }
    m_llmEnabledCheck->setChecked(config.enabled);
    if (m_store->saveLlmConfig(config)) {
        QMessageBox::information(this,
                                 QStringLiteral("保存成功"),
                                 config.enabled
                                     ? QStringLiteral("大模型已启用。建议点击「测试大模型连接」确认可用。")
                                     : QStringLiteral("设置已保存。"));
    } else {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("无法写入本地设置文件。"));
    }
}
void SettingsDialog::testLlmConnection()
{
    LlmConfig config;
    config.apiKey = m_llmApiKeyEdit->text().trimmed();
    config.baseUrl = m_llmBaseUrlEdit->text().trimmed();
    config.model = m_llmModelEdit->text().trimmed();
    config.enabled = true;
    if (config.baseUrl.isEmpty()) {
        config.baseUrl = QStringLiteral("https://api.deepseek.com");
    }
    if (config.model.isEmpty()) {
        config.model = QStringLiteral("deepseek-chat");
    }
    if (!LlmConfig::isValidApiKey(config.apiKey)) {
        QMessageBox::warning(this, QStringLiteral("无法测试"), QStringLiteral("请先填写有效的 sk- 开头 API Key。"));
        return;
    }
    QString error;
    const MoodEntry probe = LlmClient::analyze(QStringLiteral("今天有点难过，测试连接。"), config, &error);
    if (!probe.emotion.isEmpty() && probe.analysisSource == QStringLiteral("llm")) {
        QMessageBox::information(this,
                                 QStringLiteral("连接成功"),
                                 QStringLiteral("大模型连接测试通过，可以正常使用。"));
        ;
    } else {
        QMessageBox::warning(this, QStringLiteral("连接失败"), error.isEmpty() ? QStringLiteral("未知错误") : error);
    }
}
void SettingsDialog::exportData()
{
    const QString savePath = QFileDialog::getSaveFileName(this, "导出 JSON 数据", "备份数据", "JSON 文件 (*.json)");
    if (savePath.isEmpty()) {
        return;
    }
    if (!QFile::exists(m_store->dataFilePath())) {
        QMessageBox::information(this, "暂无数据", "当前还没有保存过日记记录。");
        return;
    }
    QFile::remove(savePath);
    if (QFile::copy(m_store->dataFilePath(), savePath)) {
        QMessageBox::information(this, "导出成功", "本地日记数据已导出。");
    } else {
        QMessageBox::warning(this, "导出失败", "无法复制数据文件，请检查路径权限。");
    }
}