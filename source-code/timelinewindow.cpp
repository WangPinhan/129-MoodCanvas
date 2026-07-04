#include "timelinewindow.h"
#include "thememanager.h"

#include <algorithm>

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

TimelineWindow::TimelineWindow(DataStore *store, QWidget *parent)
    : QDialog(parent),
      m_store(store),
      m_currentMonth(QDate::currentDate())
{
    setWindowTitle(QStringLiteral("时间长廊"));
    setObjectName(QStringLiteral("timelineDialog"));
    resize(940, 680);
/**
    setStyleSheet(
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f8fbff, stop:1 #fff8f2); }"
        "QToolButton { border: none; border-radius: 18px; padding: 8px; background: white; color: #334155; }"
        "QToolButton:hover { background: #eefaf5; }"
        "QPushButton { border-radius: 16px; padding: 8px 16px; background: white; border: 1px solid #dce8ef; }"
        "QPushButton:hover { background: #f1fbf7; }"
        "QPushButton#deleteEntryButton {"
        "    background: #fff5f5;"
        "    border: 1px solid #f0d4d4;"
        "    color: #b85c5c;"
        "    padding: 6px 14px;"
        "    font-size: 12px;"
        "}"
        "QPushButton#deleteEntryButton:hover { background: #ffecec; }"
        "QFrame#detailPanel { border: 1px solid #e6eef2; border-radius: 22px; background: white; }"
        "QFrame#entryCard { border: 1px solid #e8eef2; border-radius: 18px; background: rgba(255,255,255,235); }"
        "QLabel#sceneLabel { border-radius: 24px; padding: 14px; color: #34444c; font-weight: bold; }"
        "QLabel#detailHintLabel { color: #7c8b94; font-size: 13px; padding: 8px 4px; }"
        "QLabel#entryMetaLabel { color: #56616a; font-size: 12px; }"
        "QLabel#entryEmotionLabel { color: #2f3a44; font-size: 15px; font-weight: 600; }"
        "QLabel#entryBodyLabel { color: #52626b; font-size: 13px; }"
    );
*/

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto *top = new QHBoxLayout();
    auto *prevBtn = new QPushButton(QStringLiteral("← 上个月"), this);
    auto *nextBtn = new QPushButton(QStringLiteral("下个月 →"), this);
    m_monthLabel = new QLabel(this);
    m_monthLabel->setObjectName(QStringLiteral("monthTitleLabel"));
    QFont monthFont = m_monthLabel->font();
    monthFont.setPointSize(20);
    monthFont.setBold(true);
    m_monthLabel->setFont(monthFont);
    m_monthLabel->setAlignment(Qt::AlignCenter);

    top->addWidget(prevBtn);
    top->addStretch();
    top->addWidget(m_monthLabel);
    top->addStretch();
    top->addWidget(nextBtn);
    root->addLayout(top);

    m_sceneLabel = new QLabel(this);
    m_sceneLabel->setAlignment(Qt::AlignCenter);
    m_sceneLabel->setMinimumHeight(72);
    m_sceneLabel->setWordWrap(true);
    root->addWidget(m_sceneLabel);

    connect(prevBtn, &QPushButton::clicked, this, [this]() {
        m_currentMonth = m_currentMonth.addMonths(-1);
        refreshCalendar();
    });
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        m_currentMonth = m_currentMonth.addMonths(1);
        refreshCalendar();
    });

    auto *content = new QHBoxLayout();
    m_calendarGrid = new QGridLayout();
    m_calendarGrid->setSpacing(8);
    content->addLayout(m_calendarGrid, 2);

    auto *detailPanel = new QFrame(this);
    m_detailPanel = detailPanel;
    detailPanel->setObjectName(QStringLiteral("detailPanel"));
    detailPanel->setMinimumWidth(330);
    auto *detailPanelLayout = new QVBoxLayout(detailPanel);
    detailPanelLayout->setContentsMargins(14, 14, 14, 14);
    detailPanelLayout->setSpacing(10);

    auto *detailScroll = new QScrollArea(detailPanel);
    detailScroll->setWidgetResizable(true);
    detailScroll->setFrameShape(QFrame::NoFrame);
    detailScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_detailContainer = new QWidget(detailScroll);
    m_detailContainer->setStyleSheet("background: transparent;");
    m_detailLayout = new QVBoxLayout(m_detailContainer);
    m_detailLayout->setContentsMargins(0, 0, 0, 0);
    m_detailLayout->setSpacing(12);
    detailScroll->setWidget(m_detailContainer);
    detailPanelLayout->addWidget(detailScroll);

    content->addWidget(detailPanel, 1);
    root->addLayout(content, 1);

    clearDetailPanel();
    auto *hint = new QLabel(QStringLiteral("点击某一天查看当天全部心象记录，并可删除不需要的日记。"),
                            m_detailContainer);
    hint->setObjectName(QStringLiteral("detailHintLabel"));
    hint->setWordWrap(true);
    m_detailLayout->addWidget(hint);
    m_detailLayout->addStretch();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](bool) {refreshCalendar();});
    // 重新应用场景背景、刷新日历按钮（因为日历按钮内联样式可能没变，但无记录按钮需重绘）
    refreshCalendar();
}

void TimelineWindow::clearDetailPanel()
{
    while (QLayoutItem *item = m_detailLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void TimelineWindow::refreshCalendar()
{
    while (QLayoutItem *item = m_calendarGrid->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    m_monthLabel->setText(m_currentMonth.toString(QStringLiteral("yyyy 年 MM 月")));
    if (m_sceneLabel) {
        m_sceneLabel->setText(monthSceneText());
    }
    const MonthSceneColors colors = monthSceneColors();
    setStyleSheet(QString("QDialog#timelineDialog { background: %1; } QLabel#sceneLabel { background: %2; }")
                      .arg(colors.dialogGradient, colors.sceneBackground));
/**
    setStyleSheet(monthSceneStyle() +
                  "QToolButton { border: none; border-radius: 18px; padding: 8px; background: white; color: #334155; }"
                  "QToolButton:hover { background: #eefaf5; }"
                  "QPushButton { border-radius: 16px; padding: 8px 16px; background: white; border: 1px solid #dce8ef; }"
                  "QPushButton:hover { background: #f1fbf7; }"
                  "QPushButton#deleteEntryButton {"
                  "    background: #fff5f5; border: 1px solid #f0d4d4; color: #b85c5c;"
                  "    padding: 6px 14px; font-size: 12px;"
                  "}"
                  "QPushButton#deleteEntryButton:hover { background: #ffecec; }"
                  "QFrame#detailPanel { border: 1px solid #e6eef2; border-radius: 22px; background: white; }"
                  "QFrame#entryCard { border: 1px solid #e8eef2; border-radius: 18px; background: rgba(255,255,255,235); }"
                  "QLabel#sceneLabel { border-radius: 24px; padding: 14px; color: #34444c; font-weight: bold; }"
                  "QLabel#detailHintLabel { color: #7c8b94; font-size: 13px; padding: 8px 4px; }"
                  "QLabel#entryMetaLabel { color: #56616a; font-size: 12px; }"
                  "QLabel#entryEmotionLabel { color: #2f3a44; font-size: 15px; font-weight: 600; }"
                  "QLabel#entryBodyLabel { color: #52626b; font-size: 13px; }");
*/

    const QStringList weekHeaders = {QStringLiteral("一"), QStringLiteral("二"), QStringLiteral("三"),
                                     QStringLiteral("四"), QStringLiteral("五"), QStringLiteral("六"),
                                     QStringLiteral("日")};
    for (int i = 0; i < weekHeaders.size(); ++i) {
        auto *label = new QLabel(weekHeaders.at(i), this);
        label->setAlignment(Qt::AlignCenter);
        /* label->setStyleSheet(QStringLiteral("color: #7c8b94; font-weight: bold;"));*/
        m_calendarGrid->addWidget(label, 0, i);
    }

    const QVector<MoodEntry> entries = m_store->entriesForMonth(m_currentMonth.year(), m_currentMonth.month());
    const QDate firstDay(m_currentMonth.year(), m_currentMonth.month(), 1);
    const int offset = firstDay.dayOfWeek() - 1;
    const int days = firstDay.daysInMonth();

    for (int day = 1; day <= days; ++day) {
        const QDate date(m_currentMonth.year(), m_currentMonth.month(), day);
        const int index = offset + day - 1;
        const int row = index / 7 + 1;
        const int col = index % 7;

        auto *btn = new QToolButton(this);
        const int count = countForDate(date, entries);
        btn->setText(count > 1 ? QStringLiteral("%1\n%2条").arg(day).arg(count) : QString::number(day));
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setMinimumSize(82, 58);

        const QColor color = colorForDate(date, entries);
        if (color.isValid()) {
            btn->setStyleSheet(QString(
                                   "QToolButton { background: %1; color: white; border-radius: 20px; font-weight: bold; }"
                                   "QToolButton:hover { border: 2px solid white; }")
                                   .arg(color.name()));
        } /*else {
            btn->setStyleSheet(QStringLiteral("QToolButton { background: white; color: #495057; border-radius: 20px; }"));
        }*/

        connect(btn, &QToolButton::clicked, this, [this, date]() {
            showEntryDetail(date);
        });

        m_calendarGrid->addWidget(btn, row, col);
    }

    if (m_selectedDate.isValid()
        && m_selectedDate.year() == m_currentMonth.year()
        && m_selectedDate.month() == m_currentMonth.month()) {
        showEntryDetail(m_selectedDate);
    }
}

QColor TimelineWindow::colorForDate(const QDate &date, const QVector<MoodEntry> &entries) const
{
    QColor result;
    QDateTime latest;

    for (const MoodEntry &entry : entries) {
        if (entry.date == date && (!latest.isValid() || entry.createdAt > latest)) {
            latest = entry.createdAt;
            result = entry.mainColor;
        }
    }

    return result;
}

int TimelineWindow::countForDate(const QDate &date, const QVector<MoodEntry> &entries) const
{
    int count = 0;
    for (const MoodEntry &entry : entries) {
        if (entry.date == date) {
            ++count;
        }
    }
    return count;
}

MoodEntry TimelineWindow::latestEntryForDate(const QDate &date) const
{
    MoodEntry latest;
    const QVector<MoodEntry> entries = m_store->entriesForDate(date);
    for (const MoodEntry &entry : entries) {
        if (latest.id.isEmpty() || entry.createdAt > latest.createdAt) {
            latest = entry;
        }
    }
    return latest;
}

QString TimelineWindow::monthSceneText() const
{
    static const QStringList scenes = {
        QStringLiteral("一月 · 雪地小屋 ❄️  温柔地开始新一年"),
        QStringLiteral("二月 · 粉色云朵 ☁️  慢慢靠近春天"),
        QStringLiteral("三月 · 小鹿花园 🦌  新芽正在冒头"),
        QStringLiteral("四月 · 樱花车站 🌸  每一天都可以轻轻停靠"),
        QStringLiteral("五月 · 森林野餐 🧺  让心情晒晒太阳"),
        QStringLiteral("六月 · 青柠雨季 🍋  雨也会滋养植物"),
        QStringLiteral("七月 · 海边气球 🫧  把情绪交给海风"),
        QStringLiteral("八月 · 夏夜星河 ✨  热闹之后也需要安静"),
        QStringLiteral("九月 · 奶油书桌 📖  适合重新整理自己"),
        QStringLiteral("十月 · 南瓜小路 🎃  温暖慢慢亮起来"),
        QStringLiteral("十一月 · 枫叶邮局 🍁  给过去的自己寄一封信"),
        QStringLiteral("十二月 · 冬夜壁炉 🕯  一年辛苦了")
    };
    return scenes.value(m_currentMonth.month() - 1);
}

TimelineWindow::MonthSceneColors TimelineWindow::monthSceneColors() const
{
    // 浅色配色（12个月）
    static const QStringList lightGradients = {
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #edf7ff, stop:1 #fffdf7)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff0f6, stop:1 #f1fbff)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f0fff4, stop:1 #fff7ed)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff1f5, stop:1 #f4fff8)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f8ffe5, stop:1 #effaf7)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #effaff, stop:1 #fffbea)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #e9f7ff, stop:1 #fff6ef)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #eef2ff, stop:1 #fff7d6)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff8e7, stop:1 #eef7ff)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff1df, stop:1 #f4fff0)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff5e6, stop:1 #f3f7ff)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #edf2ff, stop:1 #fff8f0)")
    };
    static const QStringList lightSceneBgs = {
        QStringLiteral("rgba(255,255,255,205)"),
        QStringLiteral("rgba(255,247,252,220)"),
        QStringLiteral("rgba(248,255,247,220)"),
        QStringLiteral("rgba(255,250,252,220)"),
        QStringLiteral("rgba(250,255,240,220)"),
        QStringLiteral("rgba(245,252,255,220)"),
        QStringLiteral("rgba(245,251,255,220)"),
        QStringLiteral("rgba(248,250,255,220)"),
        QStringLiteral("rgba(255,252,244,220)"),
        QStringLiteral("rgba(255,248,236,220)"),
        QStringLiteral("rgba(255,250,241,220)"),
        QStringLiteral("rgba(247,249,255,220)")
    };

    // 深色配色（保留氛围但降低亮度）
    static const QStringList darkGradients = {
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #1e2830, stop:1 #1f2325)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2a2329, stop:1 #1f2528)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #1f2a24, stop:1 #232520)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #292329, stop:1 #1f2622)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #242b1e, stop:1 #202722)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #1e282b, stop:1 #232320)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #1d262c, stop:1 #23241f)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #1f232c, stop:1 #22251d)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #232620, stop:1 #1e262c)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #25221c, stop:1 #1f2723)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #232520, stop:1 #1e242b)"),
        QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #1e232c, stop:1 #22251e)")
    };
    static const QStringList darkSceneBgs = {
        QStringLiteral("rgba(40,44,52,200)"),
        QStringLiteral("rgba(45,42,48,210)"),
        QStringLiteral("rgba(42,47,44,210)"),
        QStringLiteral("rgba(44,43,47,210)"),
        QStringLiteral("rgba(43,47,40,210)"),
        QStringLiteral("rgba(40,46,49,210)"),
        QStringLiteral("rgba(38,44,48,210)"),
        QStringLiteral("rgba(40,42,49,210)"),
        QStringLiteral("rgba(44,46,42,210)"),
        QStringLiteral("rgba(45,43,40,210)"),
        QStringLiteral("rgba(43,45,41,210)"),
        QStringLiteral("rgba(39,42,49,210)")
    };

    const int idx = m_currentMonth.month() - 1;
    if (ThemeManager::instance().isDarkMode()) {
        return { darkGradients.value(idx), darkSceneBgs.value(idx) };
    } else {
        return { lightGradients.value(idx), lightSceneBgs.value(idx) };
    }
}
/**
QString TimelineWindow::monthSceneStyle() const
{
    static const QStringList styles = {
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #edf7ff, stop:1 #fffdf7); } QLabel#sceneLabel { background: rgba(255,255,255,205); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff0f6, stop:1 #f1fbff); } QLabel#sceneLabel { background: rgba(255,247,252,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f0fff4, stop:1 #fff7ed); } QLabel#sceneLabel { background: rgba(248,255,247,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff1f5, stop:1 #f4fff8); } QLabel#sceneLabel { background: rgba(255,250,252,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f8ffe5, stop:1 #effaf7); } QLabel#sceneLabel { background: rgba(250,255,240,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #effaff, stop:1 #fffbea); } QLabel#sceneLabel { background: rgba(245,252,255,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #e9f7ff, stop:1 #fff6ef); } QLabel#sceneLabel { background: rgba(245,251,255,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #eef2ff, stop:1 #fff7d6); } QLabel#sceneLabel { background: rgba(248,250,255,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff8e7, stop:1 #eef7ff); } QLabel#sceneLabel { background: rgba(255,252,244,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff1df, stop:1 #f4fff0); } QLabel#sceneLabel { background: rgba(255,248,236,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fff5e6, stop:1 #f3f7ff); } QLabel#sceneLabel { background: rgba(255,250,241,220); }",
        "QDialog#timelineDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #edf2ff, stop:1 #fff8f0); } QLabel#sceneLabel { background: rgba(247,249,255,220); }"
    };
    return styles.value(m_currentMonth.month() - 1);
}
*/
QString TimelineWindow::detailPanelStyleForDate(const QDate &date) const
{
    const bool dark = ThemeManager::instance().isDarkMode();

    if (dark) {
        // 深色模式下使用统一暗色面板（保留微妙氛围差异）
        const MoodEntry latest = latestEntryForDate(date);
        const QString emotion = latest.emotion + latest.emotionSummary;
        const int variant = (date.day() + date.month() * 7) % 3;

        // 深色情绪背景色板（低饱和度，柔和暗调）
        QStringList backgrounds;
        if (emotion.contains(QStringLiteral("开心")) || emotion.contains(QStringLiteral("喜"))
            || emotion.contains(QStringLiteral("乐"))) {
            backgrounds = {QStringLiteral("#3d3a28"), QStringLiteral("#403e2b"), QStringLiteral("#3b3929")};
        } else if (emotion.contains(QStringLiteral("焦虑")) || emotion.contains(QStringLiteral("紧张"))) {
            backgrounds = {QStringLiteral("#2e2d3f"), QStringLiteral("#2a2f3d"), QStringLiteral("#2d2e3b")};
        } else if (emotion.contains(QStringLiteral("难过")) || emotion.contains(QStringLiteral("哀"))
                   || emotion.contains(QStringLiteral("悲"))) {
            backgrounds = {QStringLiteral("#2b3340"), QStringLiteral("#28303d"), QStringLiteral("#2a2f3c")};
        } else if (emotion.contains(QStringLiteral("愤怒")) || emotion.contains(QStringLiteral("怒"))) {
            backgrounds = {QStringLiteral("#3b2d2b"), QStringLiteral("#3d302d"), QStringLiteral("#3b2c2a")};
        } else {
            backgrounds = {QStringLiteral("#29332e"), QStringLiteral("#273032"), QStringLiteral("#2a302b")};
        }

        const QString bg = backgrounds.value(variant, QStringLiteral("#252b32"));
        return QStringLiteral(
                   "QFrame#detailPanel { border: 1px solid #3b434f; border-radius: 22px; background: %1; }"
                   ).arg(bg);
    }

    // 浅色模式：保留原有多彩情绪逻辑
    const MoodEntry latest = latestEntryForDate(date);
    const QString emotion = latest.emotion + latest.emotionSummary;
    const int variant = (date.day() + date.month() * 7) % 3;

    QStringList backgrounds;
    if (emotion.contains(QStringLiteral("开心")) || emotion.contains(QStringLiteral("喜"))
        || emotion.contains(QStringLiteral("乐"))) {
        backgrounds = {QStringLiteral("#fff7cc"), QStringLiteral("#fff0d6"), QStringLiteral("#fff9e8")};
    } else if (emotion.contains(QStringLiteral("焦虑")) || emotion.contains(QStringLiteral("紧张"))) {
        backgrounds = {QStringLiteral("#f2ecff"), QStringLiteral("#edf7ff"), QStringLiteral("#f6f0ff")};
    } else if (emotion.contains(QStringLiteral("难过")) || emotion.contains(QStringLiteral("哀"))
               || emotion.contains(QStringLiteral("悲"))) {
        backgrounds = {QStringLiteral("#edf3ff"), QStringLiteral("#eef8ff"), QStringLiteral("#f2f1ff")};
    } else if (emotion.contains(QStringLiteral("愤怒")) || emotion.contains(QStringLiteral("怒"))) {
        backgrounds = {QStringLiteral("#fff0ec"), QStringLiteral("#fff6ef"), QStringLiteral("#ffeceb")};
    } else {
        backgrounds = {QStringLiteral("#f4fff8"), QStringLiteral("#f0fbff"), QStringLiteral("#fffaf0")};
    }

    const QString bg = backgrounds.value(variant, QStringLiteral("#ffffff"));
    return QStringLiteral(
               "QFrame#detailPanel { border: 1px solid #e3edf0; border-radius: 22px; background: %1; }"
               ).arg(bg);
}
/*
QString TimelineWindow::detailPanelStyleForDate(const QDate &date) const
{
    const MoodEntry latest = latestEntryForDate(date);
    const QString emotion = latest.emotion + latest.emotionSummary;
    const int variant = (date.day() + date.month() * 7) % 3;

    QStringList backgrounds;
    if (emotion.contains(QStringLiteral("开心")) || emotion.contains(QStringLiteral("喜"))
        || emotion.contains(QStringLiteral("乐"))) {
        backgrounds = {QStringLiteral("#fff7cc"), QStringLiteral("#fff0d6"), QStringLiteral("#fff9e8")};
    } else if (emotion.contains(QStringLiteral("焦虑")) || emotion.contains(QStringLiteral("紧张"))) {
        backgrounds = {QStringLiteral("#f2ecff"), QStringLiteral("#edf7ff"), QStringLiteral("#f6f0ff")};
    } else if (emotion.contains(QStringLiteral("难过")) || emotion.contains(QStringLiteral("哀"))
               || emotion.contains(QStringLiteral("悲"))) {
        backgrounds = {QStringLiteral("#edf3ff"), QStringLiteral("#eef8ff"), QStringLiteral("#f2f1ff")};
    } else if (emotion.contains(QStringLiteral("愤怒")) || emotion.contains(QStringLiteral("怒"))) {
        backgrounds = {QStringLiteral("#fff0ec"), QStringLiteral("#fff6ef"), QStringLiteral("#ffeceb")};
    } else {
        backgrounds = {QStringLiteral("#f4fff8"), QStringLiteral("#f0fbff"), QStringLiteral("#fffaf0")};
    }

    const QString bg = backgrounds.value(variant, QStringLiteral("#ffffff"));
    return QStringLiteral("QFrame#detailPanel { border: 1px solid #e3edf0; border-radius: 22px; background: %1; }")
        .arg(bg);
}
*/
QString TimelineWindow::entryCardStyle(const MoodEntry &entry, const QDate &date) const
{
    Q_UNUSED(date)

    // 左边彩色强调线：优先使用情绪主色，无效时用中性灰
    const QString accent = entry.mainColor.isValid()
                               ? entry.mainColor.name()
                               : QStringLiteral("#7f8c8d");

    const bool dark = ThemeManager::instance().isDarkMode();

    // 深色卡片背景：低明度、低饱和，保留一定通透感
    const QString cardBg = dark
                               ? QStringLiteral("rgba(34, 39, 48, 220)")
                               : QStringLiteral("rgba(255, 255, 255, 228)");

    // 深色下边框稍微淡一点，浅色下保持原有白色半透明
    const QString borderColor = dark
                                    ? QStringLiteral("rgba(255, 255, 255, 60)")
                                    : QStringLiteral("rgba(255, 255, 255, 180)");

    return QStringLiteral(
               "QFrame#entryCard {"
               "   border: 1px solid %1;"
               "   border-left: 4px solid %2;"
               "   border-radius: 18px;"
               "   background: %3;"
               "}"
               ).arg(borderColor, accent, cardBg);
}
/*
QString TimelineWindow::entryCardStyle(const MoodEntry &entry, const QDate &date) const
{
    Q_UNUSED(date)
    const QString base = entry.mainColor.isValid() ? entry.mainColor.name() : QStringLiteral("#eef3f5");
    return QStringLiteral(
               "QFrame#entryCard {"
               " border: 1px solid rgba(255,255,255,180);"
               " border-left: 4px solid %1;"
               " border-radius: 18px;"
               " background: rgba(255,255,255,228);"
               "}")
        .arg(base);
}
*/
void TimelineWindow::appendEntryCard(const MoodEntry &entry, int index, const QDate &date)
{
    auto *card = new QFrame(m_detailContainer);
    card->setObjectName(QStringLiteral("entryCard"));
    card->setStyleSheet(entryCardStyle(entry, date));

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(8);

    auto *headerRow = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("【%1】%2  %3")
                                 .arg(index)
                                 .arg(entry.createdAt.toString(QStringLiteral("hh:mm")))
                                 .arg(entry.emotion),
                             card);
    title->setObjectName(QStringLiteral("entryEmotionLabel"));
    headerRow->addWidget(title, 1);

    auto *deleteBtn = new QPushButton(QStringLiteral("删除"), card);
    deleteBtn->setObjectName(QStringLiteral("deleteEntryButton"));
    headerRow->addWidget(deleteBtn);
    cardLayout->addLayout(headerRow);

    auto *meta = new QLabel(QStringLiteral("表情：%1    愉悦度：%2    激活度：%3")
                                .arg(entry.emoji)
                                .arg(entry.valence, 0, 'f', 2)
                                .arg(entry.arousal, 0, 'f', 2),
                            card);
    meta->setObjectName(QStringLiteral("entryMetaLabel"));
    meta->setWordWrap(true);
    cardLayout->addWidget(meta);

    const QString keywords = entry.keywords.isEmpty()
                                 ? QStringLiteral("暂无")
                                 : entry.keywords.join(QStringLiteral("、"));
    auto *keywordLabel = new QLabel(QStringLiteral("关键词：%1").arg(keywords), card);
    keywordLabel->setObjectName(QStringLiteral("entryMetaLabel"));
    keywordLabel->setWordWrap(true);
    cardLayout->addWidget(keywordLabel);

    QString rawPreview = entry.rawText.trimmed();
    if (rawPreview.length() > 120) {
        rawPreview = rawPreview.left(120) + QStringLiteral("…");
    }
    auto *body = new QLabel(QStringLiteral("原文：%1").arg(rawPreview), card);
    body->setObjectName(QStringLiteral("entryBodyLabel"));
    body->setWordWrap(true);
    cardLayout->addWidget(body);

    if (!entry.comfortText.trimmed().isEmpty()) {
        QString comfortPreview = entry.comfortText.trimmed();
        if (comfortPreview.length() > 100) {
            comfortPreview = comfortPreview.left(100) + QStringLiteral("…");
        }
        auto *comfort = new QLabel(QStringLiteral("安慰语：%1").arg(comfortPreview), card);
        comfort->setObjectName(QStringLiteral("entryBodyLabel"));
        comfort->setWordWrap(true);
        cardLayout->addWidget(comfort);
    }

    connect(deleteBtn, &QPushButton::clicked, this, [this, entry]() {
        confirmDeleteEntry(entry);
    });

    m_detailLayout->addWidget(card);
}

void TimelineWindow::confirmDeleteEntry(const MoodEntry &entry)
{
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("删除心象日记"),
        QStringLiteral("确定删除这条心象日记吗？\n删除后无法恢复。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!m_store->deleteEntry(entry)) {
        QMessageBox::warning(this,
                             QStringLiteral("删除失败"),
                             QStringLiteral("未能删除这条记录，请稍后重试。"));
        return;
    }

    refreshCalendar();
}

void TimelineWindow::showEntryDetail(const QDate &date)
{
    m_selectedDate = date;
    clearDetailPanel();

    if (m_detailPanel) {
        m_detailPanel->setStyleSheet(detailPanelStyleForDate(date));
    }

    const QVector<MoodEntry> entries = m_store->entriesForDate(date);
    if (entries.isEmpty()) {
        auto *emptyLabel = new QLabel(
            date.toString(QStringLiteral("yyyy-MM-dd"))
                + QStringLiteral("\n\n这一天还没有记录。背景保持柔和空白，等你回来。"),
            m_detailContainer);
        emptyLabel->setObjectName(QStringLiteral("detailHintLabel"));
        emptyLabel->setWordWrap(true);
        m_detailLayout->addWidget(emptyLabel);
        m_detailLayout->addStretch();
        return;
    }

    auto *header = new QLabel(date.toString(QStringLiteral("yyyy-MM-dd"))
                                  + QStringLiteral("\n共 %1 条记录").arg(entries.size()),
                              m_detailContainer);
    header->setObjectName(QStringLiteral("entryEmotionLabel"));
    m_detailLayout->addWidget(header);

    QVector<MoodEntry> sorted = entries;
    std::sort(sorted.begin(), sorted.end(), [](const MoodEntry &a, const MoodEntry &b) {
        return a.createdAt < b.createdAt;
    });

    for (int i = 0; i < sorted.size(); ++i) {
        appendEntryCard(sorted.at(i), i + 1, date);
    }

    m_detailLayout->addStretch();
}
