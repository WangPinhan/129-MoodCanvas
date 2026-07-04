#include "resultdialog.h"

#include <QApplication>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSizePolicy>
#include <QGridLayout>
#include <QVBoxLayout>

ResultDialog::ResultDialog(const MoodEntry &entry, QWidget *parent)
    : QDialog(parent),
      m_entry(entry)
{
    setWindowTitle(QStringLiteral("今日心象生成结果"));
    const QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    const int dlgW = qMin(780, avail.width() - 32);
    const int dlgH = qMin(620, avail.height() - 48);
    resize(dlgW, dlgH);
    setMinimumSize(520, 460);
    setMaximumSize(avail.width() - 16, avail.height() - 16);


    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("resultScrollContent"));
    auto *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(20, 20, 20, 12);
    contentLayout->setSpacing(12);
    m_canvas = new MoodCanvasWidget(scrollContent);
    m_canvas->setEntry(m_entry);
    m_canvas->setMinimumHeight(260);
    m_canvas->setMaximumHeight(320);
    m_canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    contentLayout->addWidget(m_canvas);
    auto *comfortCard = new QFrame(scrollContent);
    comfortCard->setObjectName(QStringLiteral("comfortCard"));
    auto *comfortLayout = new QVBoxLayout(comfortCard);
    comfortLayout->setContentsMargins(16, 12, 16, 14);
    comfortLayout->setSpacing(6);
    auto *comfortTitle = new QLabel(QStringLiteral("写给你的话"), comfortCard);
    comfortTitle->setObjectName(QStringLiteral("comfortTitleLabel"));
    auto *comfortBody = new QLabel(m_entry.comfortText, comfortCard);
    comfortBody->setObjectName(QStringLiteral("comfortBodyLabel"));
    comfortBody->setWordWrap(true);
    comfortBody->setTextInteractionFlags(Qt::TextSelectableByMouse);
    comfortLayout->addWidget(comfortTitle);
    comfortLayout->addWidget(comfortBody);
    contentLayout->addWidget(comfortCard);
    auto *infoCard = new QFrame(scrollContent);
    infoCard->setObjectName(QStringLiteral("infoCard"));
    auto *infoLayout = new QGridLayout(infoCard);
    infoLayout->setContentsMargins(16, 12, 16, 12);
    infoLayout->setHorizontalSpacing(12);
    infoLayout->setVerticalSpacing(6);
    const QString summary = m_entry.emotionSummary.isEmpty() ? m_entry.emotion : m_entry.emotionSummary;
    auto *timeLabel = new QLabel(QStringLiteral("记录时间：") + m_entry.createdAt.toString(QStringLiteral("yyyy-MM-dd hh:mm")), infoCard);
    auto *emotionLabel = new QLabel(QStringLiteral("主导情绪：") + m_entry.emotion, infoCard);
    auto *mixLabel = new QLabel(QStringLiteral("情绪配比：") + summary, infoCard);
    auto *keywordsLabel = new QLabel(QStringLiteral("关键词：") + m_entry.keywords.join(QStringLiteral("、")), infoCard);
    QFont boldFont = emotionLabel->font();
    boldFont.setBold(true);
    emotionLabel->setFont(boldFont);
    infoLayout->addWidget(timeLabel, 0, 0, 1, 2);
    infoLayout->addWidget(emotionLabel, 2, 0, 1, 2);
    infoLayout->addWidget(mixLabel, 3, 0, 1, 2);
    infoLayout->addWidget(keywordsLabel, 4, 0, 1, 2);
    if (!m_entry.cbtHint.isEmpty()) {
        auto *hintLabel = new QLabel(QStringLiteral("温柔小建议：") + m_entry.cbtHint, infoCard);
        hintLabel->setWordWrap(true);
        hintLabel->setObjectName(QStringLiteral("cbtHintLabel"));
        infoLayout->addWidget(hintLabel, 5, 0, 1, 2);
    }
    contentLayout->addWidget(infoCard);
    auto *rawText = new QLabel(m_entry.rawText, scrollContent);
    rawText->setObjectName(QStringLiteral("rawTextLabel"));
    rawText->setWordWrap(true);
    rawText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(rawText);
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
    auto *footer = new QFrame(this);
    footer->setObjectName(QStringLiteral("footerBar"));
    footer->setMinimumHeight(64);
    footer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 10, 20, 12);
    footerLayout->setSpacing(12);
    footerLayout->addStretch();
    auto *closeButton = new QPushButton(QStringLiteral("暂不保存"), footer);
    auto *saveButton = new QPushButton(QStringLiteral("保存到时间长廊"), footer);
    saveButton->setObjectName(QStringLiteral("saveButton"));
    saveButton->setDefault(true);
    saveButton->setAutoDefault(true);
    footerLayout->addWidget(closeButton);
    footerLayout->addWidget(saveButton);
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        emit saveRequested(m_entry);
        QMessageBox::information(this, QStringLiteral("已保存"), QStringLiteral("这条心象已保存。本版本支持同一天保存多条记录。"));
        accept();
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addWidget(footer, 0);
}