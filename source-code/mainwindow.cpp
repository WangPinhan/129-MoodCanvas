#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "llmconfig.h"
#include "llmclient.h"

#include "resultdialog.h"
#include "settingsdialog.h"
#include "timelinewindow.h"
#include "toolboxdialog.h"
#include "weeklyreportwindow.h"

#include "thememanager.h"

#include <QDate>
#include <QDateTime>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QRect>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSizePolicy>
#include <QApplication>
#include <QStatusBar>
#include <QUuid>
#include <QWidget>

MainWindow::MainWindow(DataStore *store, const UserProfile &user, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_store(store),
      m_user(user)
{
    ui->setupUi(this);

    setupLogic();
    fixResponsiveLayout();

// add by HYH for TM

    // 响应主题变化，重绘需要自定义绘制的控件
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](bool /*dark*/) {
                // 如果有自绘控件（如 ArtTitleLabel），可调用 update()
                update();
            });
// end

    const LlmConfig llm = m_store->resolveLlmConfig();
    if (llm.isReady()) {
        statusBar()->showMessage(QStringLiteral("API 模式已启用：情绪分析将会调用大模型。"));
    } else {
        statusBar()->showMessage(QStringLiteral("API 模式未启用：请在「设置 / 隐私」配置有效API Key后再生成心象。"));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupLogic()
{
    setupPlantCard();
    refreshPlantState();

    connect(ui->generateButton, &QPushButton::clicked, this, &MainWindow::generateMoodCanvas);
    connect(ui->timelineButton, &QPushButton::clicked, this, &MainWindow::openTimeline);
    connect(ui->weeklyReportButton, &QPushButton::clicked, this, &MainWindow::openWeeklyReport);
    connect(ui->toolboxButton, &QPushButton::clicked, this, &MainWindow::openToolbox);
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
}

/**
void MainWindow::setupStyle()
{
    setStyleSheet(
        "QMainWindow {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "        stop:0 #f8fbff, stop:0.45 #f2fff8, stop:1 #fff8f5);"
        "}"

        "QFrame#sideCard {"
        "    background: rgba(255,255,255,235);"
        "    border: 1px solid rgba(219,231,236,180);"
        "    border-radius: 28px;"
        "}"

        "QFrame#mainCard {"
        "    background: rgba(255,255,255,242);"
        "    border: 1px solid rgba(219,231,236,180);"
        "    border-radius: 32px;"
        "}"

        "QFrame#heroCard {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "        stop:0 #e7f7ef, stop:0.55 #f8f1ff, stop:1 #fff3e7);"
        "    border-radius: 26px;"
        "}"

        "QLabel#appIconLabel {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "        stop:0 #d8fff0, stop:1 #dfe8ff);"
        "    border-radius: 22px;"
        "}"

        "QLabel#titleLabel, QLabel#headlineLabel { color: #283044; }"
        "QLabel#headlineLabel {"
        "    padding-top: 8px;"
        "    padding-bottom: 8px;"
        "    min-height: 44px;"
        "}"
        "QLabel#subtitleLabel, QLabel#hintLabel { color: #6b7280; }"
        "QLabel#hintLabel {"
        "    padding-top: 2px;"
        "    padding-bottom: 4px;"
        "    min-height: 44px;"
        "}"

        "QFrame#plantCard {"
        "    background: rgba(255,255,255,236);"
        "    border: 1px solid rgba(198,229,215,190);"
        "    border-radius: 28px;"
        "}"

        "QLabel#plantTitleLabel {"
        "    color: #284139;"
        "    font-size: 17px;"
        "    font-weight: 800;"
        "}"

        "QLabel#plantHintLabel {"
        "    color: #60736b;"
        "}"

        "QLabel#sideIllustrationLabel {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "        stop:0 #f3fff8, stop:1 #edf3ff);"
        "    border-radius: 22px;"
        "    padding: 8px;"
        "}"

        "QLabel#privacyTipLabel, QLabel#tipsLabel {"
        "    color: #5f6c72;"
        "    background: rgba(245,250,252,235);"
        "    border-radius: 18px;"
        "    padding: 12px;"
        "}"


        "QTextEdit#inputEdit {"
        "    border: 1px solid #e1eceb;"
        "    border-radius: 22px;"
        "    padding: 14px;"
        "    background: #ffffff;"
        "    color: #2b2d42;"
        "}"

        "QLineEdit {"
        "    border: 1px solid #dfe9ec;"
        "    border-radius: 16px;"
        "    padding: 9px 12px;"
        "    background: #fbfdff;"
        "}"

        "QPushButton {"
        "    border: 1px solid #dce8ef;"
        "    border-radius: 18px;"
        "    padding: 9px 14px;"
        "    background: #ffffff;"
        "    color: #334155;"
        "}"

        "QPushButton:hover { background: #f1fbf7; border-color: #bfe4d2; }"

        "QPushButton:checked {"
        "    background: #b8eadb;"
        "    color: #263238;"
        "    border-color: #8ed9c0;"
        "    font-weight: 700;"
        "}"

        "QPushButton#generateButton {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "        stop:0 #85dcb8, stop:1 #9eb7ff);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 22px;"
        "    padding: 12px 26px;"
        "    font-weight: 800;"
        "}"

        "QPushButton#generateButton:hover {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "        stop:0 #70d1aa, stop:1 #8aa5f2);"
        "}"
    );
}
*/

/**
 * 最大化时：内容靠上紧凑排列，多余高度留给底部留白，避免输入框被拉得过长。
 */
void MainWindow::fixResponsiveLayout()
{
    QFontMetrics headlineFm(ui->headlineLabel->font());
    const int headlineH = headlineFm.height() + 14;
    ui->headlineLabel->setMinimumHeight(headlineH);
    ui->headlineLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->headlineLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QFontMetrics hintFm(ui->hintLabel->font());
    const int hintH = qMax(48, hintFm.boundingRect(
        QRect(0, 0, 640, 0), Qt::TextWordWrap, ui->hintLabel->text()).height() + 8);
    ui->hintLabel->setMinimumHeight(hintH);
    ui->hintLabel->setMaximumHeight(hintH + 12);
    ui->hintLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    const int heroH = headlineH + hintH + 52;
    ui->heroCard->setMinimumHeight(heroH);
    ui->heroCard->setMaximumHeight(heroH);
    ui->heroCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->inputEdit->setMinimumHeight(96);
    ui->inputEdit->setMaximumHeight(128);
    ui->inputEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ui->mainLayout->setAlignment(Qt::AlignTop);
    ui->sideLayout->setAlignment(Qt::AlignTop);

    for (int i = 0; i < ui->mainLayout->count(); ++i) {
        ui->mainLayout->setStretch(i, 0);
    }
    if (ui->mainLayout->count() > 0) {
        ui->mainLayout->setStretch(ui->mainLayout->count() - 1, 1);
    }

    int sideStretchIndex = -1;
    for (int i = 0; i < ui->sideLayout->count(); ++i) {
        ui->sideLayout->setStretch(i, 0);
        if (ui->sideLayout->itemAt(i)->spacerItem() != nullptr) {
            sideStretchIndex = i;
        }
    }
    if (sideStretchIndex >= 0) {
        ui->sideLayout->setStretch(sideStretchIndex, 1);
    }

    for (QPushButton *navButton : {
             ui->timelineButton,
             ui->weeklyReportButton,
             ui->toolboxButton,
             ui->settingsButton}) {
        navButton->setMinimumHeight(44);
        navButton->setMaximumHeight(44);
        navButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    if (QLayout *root = ui->centralWidget->layout()) {
        root->setAlignment(Qt::AlignTop);
    }

    ui->headlineLabel->setText(QStringLiteral("请写下今天的心情，我们会自动生成色彩心象"));
    ui->hintLabel->setText(QStringLiteral("回想自己一天的经历写一小段感受，下方的小植物按今天的心情改变状态哦。"));
    ui->inputEdit->setPlaceholderText(QStringLiteral("例如：今天有点紧张，但傍晚散步时觉得舒服了一些……"));
    ui->generateButton->setText(QStringLiteral("记录心情并生成心象 ✨"));
    ui->tipsLabel->setText(QStringLiteral("今日植物会以当天最后一次保存的记录为准；没有记录时植物只是休眠，不会枯萎。"));
    ui->privacyTipLabel->hide();

    ui->toolboxButton->setText(QStringLiteral("🌳 匿名树洞"));

/**
    statusBar()->setStyleSheet(
        "QStatusBar {"
        "    background: rgba(255,255,255,210);"
        "    color: #5f6c72;"
        "    border-top: 1px solid #e6eef2;"
        "    padding-left: 12px;"
        "}"
    );
*/
}


void MainWindow::setupPlantCard()
{
    if (m_plantCard) {
        return;
    }

    m_plantCard = new QFrame(this);
    m_plantCard->setObjectName(QStringLiteral("plantCard"));
    m_plantCard->setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(m_plantCard);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    m_plantTitleLabel = new QLabel(m_plantCard);
    m_plantTitleLabel->setObjectName(QStringLiteral("plantTitleLabel"));
    m_plantTitleLabel->setText(QStringLiteral("今日植物：休眠等待"));
    layout->addWidget(m_plantTitleLabel);

    m_plantWidget = new PlantWidget(m_plantCard);
    layout->addWidget(m_plantWidget);

    m_plantHintLabel = new QLabel(m_plantCard);
    m_plantHintLabel->setObjectName(QStringLiteral("plantHintLabel"));
    m_plantHintLabel->setWordWrap(true);
    layout->addWidget(m_plantHintLabel);

    const int insertAt = ui->mainLayout->indexOf(ui->inputEdit) + 1;
    ui->mainLayout->insertWidget(insertAt, m_plantCard);

    auto *illustration = new QLabel(this);
    illustration->setObjectName(QStringLiteral("sideIllustrationLabel"));
    illustration->setAlignment(Qt::AlignCenter);
    illustration->setMinimumHeight(72);
    illustration->setMaximumHeight(96);
    illustration->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    illustration->setWordWrap(true);
    const QPixmap gardenPixmap(QStringLiteral(":/images/fresh_garden.png"));
    if (!gardenPixmap.isNull()) {
        illustration->setPixmap(gardenPixmap.scaled(200, 88, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        illustration->setText(QStringLiteral("☁️  🌱  🌷\n把今天交给一座小花园"));
        QFont font = illustration->font();
        font.setPointSize(12);
        font.setBold(true);
        illustration->setFont(font);
    }

    // 导航按钮固定在标题下方；装饰图放在设置按钮之后，避免最大化时把「时间长廊」挤出可视区域。
    const int afterSettings = ui->sideLayout->indexOf(ui->settingsButton) + 1;
    ui->sideLayout->insertWidget(qMax(0, afterSettings), illustration);
}

PlantWidget::PlantState MainWindow::plantStateForEntry(const MoodEntry &entry) const
{
    const QString emotion = entry.emotion + entry.emotionSummary + entry.emoji;
    if (emotion.contains(QStringLiteral("开心")) || emotion.contains(QStringLiteral("喜"))
        || emotion.contains(QStringLiteral("乐")) || emotion.contains(QStringLiteral("期待"))) {
        return PlantWidget::PlantState::Bloom;
    }
    if (emotion.contains(QStringLiteral("焦虑")) || emotion.contains(QStringLiteral("紧张"))
        || emotion.contains(QStringLiteral("不安"))) {
        return PlantWidget::PlantState::Curled;
    }
    if (emotion.contains(QStringLiteral("难过")) || emotion.contains(QStringLiteral("哀"))
        || emotion.contains(QStringLiteral("悲")) || emotion.contains(QStringLiteral("委屈"))) {
        return PlantWidget::PlantState::Rain;
    }
    return PlantWidget::PlantState::Leaves;
}

void MainWindow::refreshPlantState()
{
    if (!m_plantWidget || !m_plantTitleLabel || !m_plantHintLabel) {
        return;
    }

    const QVector<MoodEntry> todayEntries = m_store->entriesForDate(QDate::currentDate());
    if (todayEntries.isEmpty()) {
        m_plantWidget->setPlantState(PlantWidget::PlantState::Dormant);
    } else {
        MoodEntry latest = todayEntries.first();
        for (const MoodEntry &entry : todayEntries) {
            if (entry.createdAt > latest.createdAt) {
                latest = entry;
            }
        }
        m_plantWidget->setPlantState(plantStateForEntry(latest));
    }

    m_plantTitleLabel->setText(m_plantWidget->stateTitle());
    m_plantHintLabel->setText(m_plantWidget->stateHint());
}

void MainWindow::generateMoodCanvas()
{
    const QString text = ui->inputEdit->toPlainText().trimmed();

    if (text.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("请先输入"), QStringLiteral("请写下一段话描述今日心情，系统将自动分析并生成色彩心象。"));
        return;
    }

    const LlmConfig llmConfig = m_store->resolveLlmConfig();
    if (!llmConfig.isReady()) {
        const int choice = QMessageBox::question(
            this,
            QStringLiteral("未配置 API"),
            QStringLiteral("当前版本已移除本地训练模型，只保留 API 分析。\n\n"
                           "请先在「设置 / 隐私」填写有效的 DeepSeek API Key，否则无法生成心象。\n\n"
                           "是否现在打开设置？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (choice == QMessageBox::Yes) {
            SettingsDialog dialog(m_store, this);
            dialog.exec();
        }
    }

    const LlmConfig activeConfig = m_store->resolveLlmConfig();
    if (!activeConfig.isReady()) {
        QMessageBox::warning(this,
                             QStringLiteral("无法分析"),
                             QStringLiteral("未配置有效 API。当前项目已删除本地训练模型，因此不会回退到离线分析。"));
        return;
    }

    QProgressDialog progress(this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setCancelButton(nullptr);
    progress.setMinimumDuration(0);
    progress.setLabelText(QStringLiteral("正在调用大模型 API 分析情绪，请稍候…"));
    progress.setRange(0, 0);
    progress.show();
    QApplication::processEvents();

    QString llmError;
    MoodEntry entry = LlmClient::analyze(text, activeConfig, &llmError);
    progress.close();

    if (entry.rawText.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("分析失败"), QStringLiteral("未能生成心象，请检查输入或网络与大模型设置。"));
        return;
    }

    if (entry.analysisSource == QStringLiteral("llm")) {
        statusBar()->showMessage(QStringLiteral("已由大模型 API 完成语义分析"), 5000);
    } else if (!llmError.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("API 分析失败：") + llmError, 8000);
    }

    // 新需求：同一天支持多条内容，因此每条记录都有独立 id 和创建时间。
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.createdAt = QDateTime::currentDateTime();
    entry.date = entry.createdAt.date();

    auto *dialog = new ResultDialog(entry, this);

    connect(dialog, &ResultDialog::saveRequested, this, [this](const MoodEntry &savedEntry) {
        if (!m_store->saveEntry(savedEntry)) {
            QMessageBox::warning(this, "保存失败", "本地数据文件写入失败，请检查权限。");
        } else {
            statusBar()->showMessage("今日心象已保存，小植物已按当天最后一次记录更新：" + savedEntry.createdAt.toString("yyyy-MM-dd hh:mm"), 3000);
            refreshPlantState();
        }
    });

    dialog->exec();
}

void MainWindow::openTimeline()
{
    TimelineWindow window(m_store, this);
    window.exec();
}

void MainWindow::openWeeklyReport()
{
    WeeklyReportWindow window(m_store, this);
    window.exec();
}

void MainWindow::openToolbox()
{
    ToolboxDialog dialog(m_store, m_user, this);
    dialog.exec();
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_store, this);
    dialog.exec();
}

// start by HYH
// end by HYH