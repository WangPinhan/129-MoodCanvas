#include "toolboxdialog.h"

#include "moodcanvaswidget.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScreen>
#include <QSizePolicy>
#include <QUuid>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>

ToolboxDialog::ToolboxDialog(DataStore *store, const UserProfile &user, QWidget *parent)
    : QDialog(parent),
      m_store(store),
      m_user(user),
      m_cloud(&m_api)
{
    setWindowTitle(QStringLiteral("匿名树洞"));

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect geo = screen->availableGeometry();
        const int width = qMin(880, geo.width() - 48);
        const int height = qMin(640, geo.height() - 64);
        resize(width, height);
        setMaximumSize(geo.width() - 16, geo.height() - 16);
    } else {
        resize(880, 640);
    }

    if (m_store) {
        m_api.setServerUrl(m_store->loadCloudConfig().serverUrl);
        m_emotionAnalyzer.setLlmConfig(m_store->resolveLlmConfig());
    }
    m_api.setToken(m_user.authToken);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("匿名树洞"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setObjectName(QStringLiteral("toolboxTitle"));
    layout->addWidget(title);

    auto *subtitle = new QLabel(QStringLiteral("每句话会调用大模型分析心理，并用本地色彩算法渲染情感颜色（需配置 API Key）。"), this);
    subtitle->setWordWrap(true);
    subtitle->setObjectName(QStringLiteral("toolboxSubtitle"));
    layout->addWidget(subtitle);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(createTreeHolePage());
    layout->addWidget(scrollArea, 1);

    connect(&m_cloud, &CloudTreehole::statusChanged, this, [this](const QString &status) {
        if (m_networkStatusLabel) {
            m_networkStatusLabel->setText(status);
        }
        updateNetworkUi();
    });
    connect(&m_cloud, &CloudTreehole::postsChanged, this, &ToolboxDialog::refreshTreeList);
    connect(&m_emotionAnalyzer, &TreeholeEmotionAnalyzer::sentenceReady, this, [this](const QString &, const SentenceEmotion &) {
        refreshTreeList();
    });

    m_cloud.startSync();
}

ToolboxDialog::~ToolboxDialog()
{
    m_cloud.stopSync();
}

QWidget *ToolboxDialog::createTreeHolePage()
{
    auto *page = new QWidget(this);
    page->setObjectName(QStringLiteral("page"));
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(18, 18, 18, 24);
    root->setSpacing(12);

    auto *networkBar = new QFrame(page);
    networkBar->setObjectName(QStringLiteral("treeHoleNetworkBar"));
    auto *networkLayout = new QHBoxLayout(networkBar);
    networkLayout->setSpacing(10);

    auto *refreshButton = new QPushButton(QStringLiteral("刷新云端"), networkBar);
    networkLayout->addWidget(refreshButton);
    networkLayout->addStretch();

    auto *serverHint = new QLabel(
        QStringLiteral("云端：%1").arg(m_store ? m_store->loadCloudConfig().serverUrl : QString()), networkBar);
    serverHint->setObjectName(QStringLiteral("previewInfoLabel"));
    networkLayout->addWidget(serverHint);
    root->addWidget(networkBar);

    m_networkStatusLabel = new QLabel(QStringLiteral("正在连接云端树洞…"), page);
    m_networkStatusLabel->setWordWrap(true);
    m_networkStatusLabel->setObjectName(QStringLiteral("previewInfoLabel"));
    root->addWidget(m_networkStatusLabel);

    auto *content = new QHBoxLayout();
    content->setSpacing(18);

    auto *left = new QVBoxLayout();
    left->setSpacing(12);

    m_previewInfoLabel = new QLabel(page);
    m_previewInfoLabel->setWordWrap(true);
    m_previewInfoLabel->setObjectName(QStringLiteral("previewInfoLabel"));
    left->addWidget(m_previewInfoLabel);

    m_previewCanvas = new MoodCanvasWidget(page);
    m_previewCanvas->setMinimumHeight(150);
    m_previewCanvas->setMaximumHeight(190);
    m_previewCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    left->addWidget(m_previewCanvas);

    const MoodEntry latest = latestEntry();
    if (!latest.rawText.isEmpty()) {
        m_previewCanvas->setEntry(latest);
        m_previewInfoLabel->setText(QStringLiteral("将分享最近一次心象：%1 · %2。树洞只展示昵称与模糊心情。")
                                    .arg(latest.createdAt.toString(QStringLiteral("MM-dd hh:mm")))
                                    .arg(latest.emotion));
    } else {
        m_previewInfoLabel->setText(QStringLiteral("还没有可分享的心象。请先在首页保存一条心情记录。"));
    }

    auto *nickLabel = new QLabel(QStringLiteral("树洞昵称：%1").arg(m_user.nickname), page);
    left->addWidget(nickLabel);

    auto *moodEdit = new QPlainTextEdit(page);
    moodEdit->setPlaceholderText(QStringLiteral("一句模糊心情，例如：今天的云有点重，但风还在。"));
    moodEdit->setMinimumHeight(68);
    moodEdit->setMaximumHeight(88);
    left->addWidget(moodEdit);

    auto *button = new QPushButton(QStringLiteral("投进匿名树洞 🌳"), page);
    button->setObjectName(QStringLiteral("treeHolePostButton"));
    button->setMinimumHeight(40);
    left->addWidget(button, 0, Qt::AlignLeft);
    connect(button, &QPushButton::clicked, this, [this, moodEdit]() {
        submitTreeHolePost(moodEdit->toPlainText().trimmed());
        moodEdit->clear();
    });
    content->addLayout(left, 1);

    auto *right = new QVBoxLayout();
    auto *listTitle = new QLabel(QStringLiteral("云端树洞里的实时心象"), page);
    QFont listFont = listTitle->font();
    listFont.setPointSize(15);
    listFont.setBold(true);
    listTitle->setFont(listFont);
    listTitle->setObjectName(QStringLiteral("treeHoleListTitle"));
    right->addWidget(listTitle);

    m_treeList = new QListWidget(page);
    m_treeList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeList->setMinimumHeight(180);
    right->addWidget(m_treeList, 1);

    m_replyEdit = new QPlainTextEdit(page);
    m_replyEdit->setPlaceholderText(QStringLiteral("选中一条心象后，在这里写下你的回应…"));
    m_replyEdit->setMinimumHeight(56);
    m_replyEdit->setMaximumHeight(72);
    right->addWidget(m_replyEdit);

    m_replyButton = new QPushButton(QStringLiteral("发送回应 💬"), page);
    m_replyButton->setMinimumHeight(38);
    m_replyButton->setEnabled(false);
    right->addWidget(m_replyButton, 0, Qt::AlignLeft);

    content->addLayout(right, 1);
    root->addLayout(content);

    connect(refreshButton, &QPushButton::clicked, this, [this]() {
        m_cloud.refreshNow();
    });

    connect(m_treeList, &QListWidget::itemSelectionChanged, this, [this]() {
        updateNetworkUi();
    });

    connect(m_replyButton, &QPushButton::clicked, this, &ToolboxDialog::submitReply);

    refreshTreeList();
    updateNetworkUi();
    return page;
}

MoodEntry ToolboxDialog::latestEntry() const
{
    MoodEntry latest;
    if (!m_store) {
        return latest;
    }
    const QVector<MoodEntry> entries = m_store->loadEntries();
    for (const MoodEntry &entry : entries) {
        if (latest.rawText.isEmpty() || entry.createdAt > latest.createdAt) {
            latest = entry;
        }
    }
    return latest;
}

void ToolboxDialog::requestEmotionsForPosts(const QVector<QJsonObject> &posts)
{
    for (const QJsonObject &post : posts) {
        const QString moodLine = post.value(QStringLiteral("moodLine")).toString();
        if (!moodLine.trimmed().isEmpty()) {
            m_emotionAnalyzer.requestAnalysis(moodLine);
        }

        const QJsonArray replies = post.value(QStringLiteral("replies")).toArray();
        for (const QJsonValue &value : replies) {
            const QString replyText = value.toObject().value(QStringLiteral("text")).toString();
            if (!replyText.trimmed().isEmpty()) {
                m_emotionAnalyzer.requestAnalysis(replyText);
            }
        }
    }
}

void ToolboxDialog::updatePostItemWidget(QListWidgetItem *item, const QJsonObject &post)
{
    if (!item || !m_treeList) {
        return;
    }

    auto *label = qobject_cast<QLabel *>(m_treeList->itemWidget(item));
    if (!label) {
        label = new QLabel(m_treeList);
        label->setWordWrap(true);
        label->setTextFormat(Qt::RichText);
        label->setStyleSheet(QStringLiteral("background: transparent;"));
        label->setContentsMargins(10, 8, 10, 8);
        m_treeList->setItemWidget(item, label);
    }

    const QString html = TreeholeEmotionHtml::buildPostCardHtml(post, m_emotionAnalyzer);
    label->setText(html);
    label->adjustSize();
    item->setSizeHint(label->sizeHint() + QSize(0, 12));
}

void ToolboxDialog::refreshTreeList()
{
    if (!m_treeList) {
        return;
    }

    const QString selectedId = selectedPostId();
    m_treeList->clear();

    const QVector<QJsonObject> posts = m_cloud.posts();
    requestEmotionsForPosts(posts);

    for (const QJsonObject &post : posts) {
        auto *item = new QListWidgetItem(m_treeList);
        item->setData(Qt::UserRole, post.value(QStringLiteral("id")).toString());
        updatePostItemWidget(item, post);
    }

    if (m_treeList->count() == 0) {
        m_treeList->addItem(QStringLiteral("这里还很安静。连接云端后，心象会在这里实时出现。"));
    } else if (!selectedId.isEmpty()) {
        for (int i = 0; i < m_treeList->count(); ++i) {
            if (m_treeList->item(i)->data(Qt::UserRole).toString() == selectedId) {
                m_treeList->setCurrentRow(i);
                break;
            }
        }
    }
}

QJsonObject ToolboxDialog::buildPostPayload(const QString &moodLine) const
{
    const MoodEntry latest = latestEntry();
    const QString emotion = latest.emotion.isEmpty() ? QStringLiteral("心象") : latest.emotion;

    QJsonObject post;
    post.insert(QStringLiteral("moodLine"), moodLine);
    post.insert(QStringLiteral("emotion"), emotion);
    post.insert(QStringLiteral("color"),
                latest.mainColor.isValid() ? latest.mainColor.name() : QStringLiteral("#bdebd9"));
    post.insert(QStringLiteral("entryId"), latest.id);
    return post;
}

void ToolboxDialog::submitTreeHolePost(const QString &moodLine)
{
    if (!m_cloud.isActive()) {
        QMessageBox::information(this,
                                 QStringLiteral("尚未连接云端"),
                                 QStringLiteral("无法连接云端树洞。请先启动 server\\start_server.bat，并在设置里确认云端地址。"));
        return;
    }

    const MoodEntry latest = latestEntry();
    if (latest.rawText.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("暂时无法投递"),
                                 QStringLiteral("请先在首页生成并保存一条心象记录。"));
        return;
    }

    if (moodLine.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("写一句心情吧"),
                                 QStringLiteral("请填写一句模糊心情，树洞里不会展示原始日记。"));
        return;
    }

    QString error;
    if (!m_cloud.publishPost(buildPostPayload(moodLine), &error)) {
        QMessageBox::warning(this, QStringLiteral("投递失败"), error);
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("已投进树洞"),
                             QStringLiteral("你的心象已发送到云端树洞，其他用户可以看到并回应。"));
}

void ToolboxDialog::submitReply()
{
    const QString postId = selectedPostId();
    const QString text = m_replyEdit->toPlainText().trimmed();
    if (postId.isEmpty() || text.isEmpty()) {
        return;
    }

    if (!m_cloud.isActive()) {
        QMessageBox::information(this, QStringLiteral("尚未连接云端"), QStringLiteral("连接云端树洞后才能回应。"));
        return;
    }

    QJsonObject reply;
    reply.insert(QStringLiteral("text"), text);

    QString error;
    if (!m_cloud.publishReply(postId, reply, &error)) {
        QMessageBox::warning(this, QStringLiteral("发送失败"), error);
        return;
    }

    m_replyEdit->clear();
}

QString ToolboxDialog::selectedPostId() const
{
    if (!m_treeList || !m_treeList->currentItem()) {
        return QString();
    }
    return m_treeList->currentItem()->data(Qt::UserRole).toString();
}

void ToolboxDialog::updateNetworkUi()
{
    if (m_networkStatusLabel && m_cloud.statusText().isEmpty()) {
        m_networkStatusLabel->setText(QStringLiteral("正在连接云端树洞…"));
    }
    if (m_replyButton) {
        m_replyButton->setEnabled(m_cloud.isActive() && !selectedPostId().isEmpty());
    }
}
