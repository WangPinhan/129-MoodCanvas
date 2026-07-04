#include "weeklyreportwindow.h"

#include "arttitlelabel.h"
#include "datastore.h"
#include "llmclient.h"
#include "reportartutils.h"
#include "weeklyscenerywidget.h"

#include "thememanager.h"

#include <QWheelEvent>
#include <QScreen>
#include <algorithm>
#include <QImage>
#include <QApplication>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

static constexpr int kWeeklyChartMinHeight = 540;
static constexpr qreal kTopCardHeight = 190.0;
static constexpr qreal kMoodStripMinHeight = 228.0;

static QVector<QPair<QString, int>> sortCountMap(const QMap<QString, int> &map)
{
    QVector<QPair<QString, int>> items;
    for (auto it = map.begin(); it != map.end(); ++it) {
        items.append(qMakePair(it.key(), it.value()));
    }

    std::sort(items.begin(), items.end(), [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
        if (a.second == b.second) {
            return a.first < b.first;
        }
        return a.second > b.second;
    });

    return items;
}

struct DayMoodCell
{
    bool hasRecord = false;
    QColor primary;
    QColor secondary;
    QString emotion;
    QDate date;
};

static QColor blendColors(const QColor &a, const QColor &b, qreal t)
{
    return QColor(
        int(a.red() * (1 - t) + b.red() * t),
        int(a.green() * (1 - t) + b.green() * t),
        int(a.blue() * (1 - t) + b.blue() * t),
        int(a.alpha() * (1 - t) + b.alpha() * t));
}

static QColor fadeToColorless(const QColor &color, qreal strength)
{
    const QColor blank = ThemeManager::instance().isDarkMode() ? QColor("#2a3037") : QColor("#eef1f4");
    const qreal t = qBound(0.0, strength, 1.0);
    QColor faded = blendColors(color, blank, t);
    faded.setAlpha(int(255 * (1.0 - t * 0.82)));
    return faded;
}

static QColor colorlessFill()
{
    return ThemeManager::instance().isDarkMode() ? QColor("#2a3037") : QColor("#f3f5f7");
}

static DayMoodCell latestEntryForDate(const QVector<MoodEntry> &entries, const QDate &date)
{
    DayMoodCell cell;
    cell.date = date;

    MoodEntry latest;
    for (const MoodEntry &entry : entries) {
        if (entry.date != date) {
            continue;
        }
        if (latest.id.isEmpty() || entry.createdAt > latest.createdAt) {
            latest = entry;
        }
    }

    if (latest.id.isEmpty()) {
        return cell;
    }
    cell.hasRecord = true;
    cell.emotion = latest.emotion;
    cell.primary = latest.mainColor.isValid() ? latest.mainColor : QColor("#9eb7ff");
    cell.secondary = cell.primary;
    if (latest.colorPalette.size() > 1) {
        cell.secondary = latest.colorPalette.at(1);
    } else {
        cell.secondary = cell.primary.lighter(118);
    }
    return cell;
}

static QVector<DayMoodCell> buildWeekCells(const QVector<MoodEntry> &entries, const QDate &monday)
{
    QVector<DayMoodCell> cells(7);
    for (int i = 0; i < 7; ++i) {
        cells[i] = latestEntryForDate(entries, monday.addDays(i));
    }

    auto resolveEmptyColor = [&cells](int index) {
        int prev = -1;
        int next = -1;
        for (int i = index - 1; i >= 0; --i) {
            if (cells[i].hasRecord) {
                prev = i;
                break;
            }
        }
        for (int i = index + 1; i < 7; ++i) {
            if (cells[i].hasRecord) {
                next = i;
                break;
            }
        }

        if (prev >= 0 && next >= 0) {
            const int span = next - prev;
            const qreal t = (index - prev) / static_cast<qreal>(span);
            const QColor left = cells[prev].primary;
            const QColor right = cells[next].primary;
            const qreal fadeStrength = 0.35 + 0.45 * (1.0 - qAbs(0.5 - t) * 2.0);
            return fadeToColorless(blendColors(left, right, t), fadeStrength);
        }
        if (prev >= 0) {
            const int distance = index - prev;
            return fadeToColorless(cells[prev].primary, qMin(0.25 + distance * 0.18, 0.92));
        }
        if (next >= 0) {
            const int distance = next - index;
            return fadeToColorless(cells[next].primary, qMin(0.25 + distance * 0.18, 0.92));
        }
        return colorlessFill();
    };

    for (int i = 0; i < 7; ++i) {
        if (cells[i].hasRecord) {
            continue;
        }
        const QColor faded = resolveEmptyColor(i);
        cells[i].primary = faded;
        cells[i].secondary = colorlessFill();
    }

    return cells;
}

static QLabel *createSectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("sectionTitle"));
    label->setFont(ReportArtUtils::scriptFont(14));
    return label;
}

static QLabel *createBodyLabel(const QString &objectName, QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setObjectName(objectName);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

WeeklyChartWidget::WeeklyChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(kWeeklyChartMinHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this]() { update(); });
}



void WeeklyChartWidget::setEntries(const QVector<MoodEntry> &entries, const QDate &weekDate)
{
    m_entries = entries;
    m_weekDate = weekDate;
    update();
}

QDate WeeklyChartWidget::weekMonday() const
{
    const QDate base = m_weekDate.isValid() ? m_weekDate : QDate::currentDate();
    return base.addDays(1 - base.dayOfWeek());
}


void WeeklyChartWidget::drawCard(QPainter &p, const QRectF &rect, const QString &title,
                                 const QColor &topTint, const QColor &bottomTint) const
{
    QLinearGradient cardBg(rect.topLeft(), rect.bottomRight());
    cardBg.setColorAt(0.0, topTint);
    cardBg.setColorAt(1.0, bottomTint);

    const bool dark = ThemeManager::instance().isDarkMode();

    p.setPen(QPen(dark ? QColor(255,255,255, 40) : QColor(255,255,255,140), 1));
    p.setBrush(cardBg);
    p.drawRoundedRect(rect, 20, 20);


    QFont titleFont = font();
    titleFont.setPointSize(10);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    p.setFont(titleFont);
    QColor TextColor = dark ? QColor("#8a929c") : QColor("#7d8b95");
    p.setPen(TextColor);
    p.drawText(rect.adjusted(24, 20, -24, -12), Qt::AlignTop | Qt::AlignLeft, title.toUpper());
}

void WeeklyChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    

    const bool dark = ThemeManager::instance().isDarkMode();

    // 背景渐变
    QLinearGradient bg(rect().topLeft(), rect().bottomRight());
    if (dark) {
        bg.setColorAt(0.0, QColor("#1c252b"));
        bg.setColorAt(0.45, QColor("#1e2625"));
        bg.setColorAt(1.0, QColor("#22241f"));
    } else {
        bg.setColorAt(0.0, QColor("#e8f4f8"));
        bg.setColorAt(0.45, QColor("#edf8f2"));
        bg.setColorAt(1.0, QColor("#fdf3ea"));
    }
    p.fillRect(rect(), bg);

    QColor emptyTextColor = dark ? QColor("#8a929c") : QColor("#7d8b95");
    if (m_entries.isEmpty()) {
        p.setPen(emptyTextColor);
        QFont f = font();
        f.setPointSize(15);
        f.setWeight(QFont::Light);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("本周暂无记录\n保存几条心象后即可生成完整周报"));
        return;
    }

    const qreal margin = 28;
    const qreal gap = 22;
    const qreal usableW = width() - margin * 2;
    const qreal usableH = height() - margin * 2;

    const qreal row1H = qMax(kTopCardHeight, usableH * 0.38);
    const qreal row2H = qMax(kMoodStripMinHeight, usableH - row1H - gap);


    const qreal emotionW = usableW * 0.54;
    const qreal keywordW = usableW - emotionW - gap;

    m_emotionCardRect = QRectF(margin, margin, emotionW, row1H);
    m_keywordCardRect = QRectF(margin + emotionW + gap, margin, keywordW, row1H);

    m_emotionScrollOffset = qBound(0.0,
                                   m_emotionScrollOffset,
                                   maxEmotionScroll(m_emotionCardRect));

    m_keywordScrollOffset = qBound(0.0,
                                   m_keywordScrollOffset,
                                   maxKeywordScroll(m_keywordCardRect));

    drawEmotionDistribution(p, m_emotionCardRect);
    drawKeywords(p, m_keywordCardRect);
    drawWeekMoodStrip(p, QRectF(margin, margin + row1H + gap, usableW, row2H));

}

void WeeklyChartWidget::drawEmotionDistribution(QPainter &p, const QRectF &rect) const
{
    const bool dark = ThemeManager::instance().isDarkMode();

    drawCard(p,
             rect,
             QStringLiteral("情绪分布"),
             QColor(dark ? "#26312f" : "#f0faf6"),
             QColor(dark ? "#272f3a" : "#e9f3ff"));

    QMap<QString, int> countMap;
    QMap<QString, QColor> colorMap;
    for (const MoodEntry &entry : m_entries) {
        if (!entry.emotion.trimmed().isEmpty()) {
            countMap[entry.emotion]++;
            colorMap[entry.emotion] = entry.mainColor;
        }
    }

    const auto items = sortCountMap(countMap);
    if (items.isEmpty()) {
        p.setPen(dark ? QColor("#8a929c") : QColor("#9aa8b0"));
        QFont f = font();
        f.setPointSize(11);
        p.setFont(f);
        p.drawText(rect.adjusted(24, 68, -24, -24),
                   Qt::AlignLeft | Qt::AlignTop,
                   QStringLiteral("暂无情绪数据"));
        return;
    }

    const int total = m_entries.size();

    const QRectF viewRect = rect.adjusted(24, 62, -24, -22);
    const qreal contentH = emotionContentHeight();

    QFont labelFont = font();
    labelFont.setPointSize(11);
    p.setFont(labelFont);

    p.save();
    p.setClipRect(viewRect);

    qreal y = viewRect.top() - m_emotionScrollOffset;

    for (int i = 0; i < items.size(); ++i) {
        const QString emotion = items.at(i).first;
        const int count = items.at(i).second;
        const qreal ratio = count / static_cast<qreal>(total);
        const QColor color = colorMap.value(emotion, QColor("#9eb7ff"));

        if (y + 48 >= viewRect.top() && y <= viewRect.bottom()) {
            p.setPen(dark ? QColor("#b0b8c0") : QColor("#56616a"));
            p.drawText(QRectF(viewRect.left(), y, viewRect.width() - 12, 20),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QStringLiteral("%1    %2%")
                           .arg(emotion)
                           .arg(static_cast<int>(ratio * 100)));

            const QRectF barBg(viewRect.left(), y + 28, viewRect.width() - 12, 9);
            p.setPen(Qt::NoPen);
            p.setBrush(dark ? QColor("#2a323a") : QColor("#eef3f5"));
            p.drawRoundedRect(barBg, 4.5, 4.5);

            p.setBrush(color);
            p.drawRoundedRect(QRectF(barBg.left(),
                                     barBg.top(),
                                     barBg.width() * ratio,
                                     barBg.height()),
                              4.5,
                              4.5);
        }

        y += 58;
    }

    p.restore();

    drawScrollIndicator(p, viewRect, contentH, m_emotionScrollOffset);
}


void WeeklyChartWidget::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF pos = event->position();
#else
    const QPointF pos = event->pos();
#endif

    const qreal wheelSteps = event->angleDelta().y() / 120.0;
    const qreal scrollDelta = -wheelSteps * 42.0;

    if (m_emotionCardRect.contains(pos)) {
        const qreal maxScroll = maxEmotionScroll(m_emotionCardRect);
        const qreal oldOffset = m_emotionScrollOffset;

        m_emotionScrollOffset = qBound(0.0,
                                       m_emotionScrollOffset + scrollDelta,
                                       maxScroll);

        if (!qFuzzyCompare(oldOffset + 1.0, m_emotionScrollOffset + 1.0)) {
            update();
            event->accept();
            return;
        }
    }

    if (m_keywordCardRect.contains(pos)) {
        const qreal maxScroll = maxKeywordScroll(m_keywordCardRect);
        const qreal oldOffset = m_keywordScrollOffset;

        m_keywordScrollOffset = qBound(0.0,
                                       m_keywordScrollOffset + scrollDelta,
                                       maxScroll);

        if (!qFuzzyCompare(oldOffset + 1.0, m_keywordScrollOffset + 1.0)) {
            update();
            event->accept();
            return;
        }
    }

    QWidget::wheelEvent(event);
}

qreal WeeklyChartWidget::emotionContentHeight() const
{
    QMap<QString, int> countMap;
    for (const MoodEntry &entry : m_entries) {
        if (!entry.emotion.trimmed().isEmpty()) {
            countMap[entry.emotion]++;
        }
    }

    const auto items = sortCountMap(countMap);
    return items.size() * 58.0;
}

qreal WeeklyChartWidget::keywordContentHeight(const QRectF &contentRect) const
{
    QMap<QString, int> countMap;
    for (const MoodEntry &entry : m_entries) {
        for (const QString &word : entry.keywords) {
            if (!word.trimmed().isEmpty()) {
                countMap[word]++;
            }
        }
    }

    const auto items = sortCountMap(countMap);
    if (items.isEmpty()) {
        return 0.0;
    }

    qreal x = contentRect.left();
    qreal y = contentRect.top();
    const qreal maxRight = contentRect.right();
    const int tagH = 30;
    const int rowGap = 14;

    for (int i = 0; i < items.size(); ++i) {
        const QString word = items.at(i).first;
        const int count = items.at(i).second;

        QFont f = font();
        f.setPointSize(10);
        f.setWeight(i < 2 ? QFont::DemiBold : QFont::Normal);

        QFontMetrics fm(f);
        const QString text = QStringLiteral("%1  ×%2").arg(word).arg(count);
        int tagW = fm.horizontalAdvance(text) + 24;
        tagW = qMin(tagW, static_cast<int>(contentRect.width()));

        if (x + tagW > maxRight) {
            x = contentRect.left();
            y += tagH + rowGap;
        }

        x += tagW + 12;
    }

    return y - contentRect.top() + tagH;
}

qreal WeeklyChartWidget::maxEmotionScroll(const QRectF &cardRect) const
{
    const QRectF viewRect = cardRect.adjusted(24, 62, -24, -22);
    const qreal contentH = emotionContentHeight();
    return qMax(0.0, contentH - viewRect.height());
}

qreal WeeklyChartWidget::maxKeywordScroll(const QRectF &cardRect) const
{
    const QRectF viewRect = cardRect.adjusted(24, 68, -24, -22);
    const qreal contentH = keywordContentHeight(viewRect);
    return qMax(0.0, contentH - viewRect.height());
}

void WeeklyChartWidget::drawScrollIndicator(QPainter &p,
                                            const QRectF &viewRect,
                                            qreal contentHeight,
                                            qreal scrollOffset) const
{
    if (contentHeight <= viewRect.height() + 1.0) {
        return;
    }

    const bool dark = ThemeManager::instance().isDarkMode();

    const qreal trackW = 4.0;
    const QRectF track(viewRect.right() - trackW,
                       viewRect.top() + 4,
                       trackW,
                       viewRect.height() - 8);

    const qreal ratio = viewRect.height() / contentHeight;
    const qreal thumbH = qMax(24.0, track.height() * ratio);
    const qreal maxScroll = qMax(1.0, contentHeight - viewRect.height());
    const qreal thumbY = track.top()
                         + (track.height() - thumbH) * (scrollOffset / maxScroll);

    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(dark ? QColor(255, 255, 255, 32)
                    : QColor(90, 110, 120, 28));
    p.drawRoundedRect(track, 2, 2);

    p.setBrush(dark ? QColor(255, 255, 255, 100)
                    : QColor(90, 110, 120, 90));
    p.drawRoundedRect(QRectF(track.left(), thumbY, track.width(), thumbH), 2, 2);
    p.restore();
}


void WeeklyChartWidget::drawWeekMoodStrip(QPainter &p, const QRectF &rect) const
{
    const bool dark = ThemeManager::instance().isDarkMode();
    QColor top = dark ? QColor("#2b2838") : QColor("#f6f0ff");
    QColor bottom = dark ? QColor("#28302d") : QColor("#eef8f3");
    drawCard(p, rect, QStringLiteral("本周每日心象色带"), top, bottom);

    const QDate monday = weekMonday();
    const QVector<DayMoodCell> cells = buildWeekCells(m_entries, monday);

    const qreal stripTop = rect.top() + 56;
    const qreal stripHeight = 72;
    const qreal labelTop = stripTop + stripHeight + 12;
    const qreal gap = 10;
    const qreal totalGap = gap * 6;
    const qreal cellWidth = (rect.width() - 48 - totalGap) / 7.0;
    const qreal cellLeft = rect.left() + 24;

    QFont dateFont = font();
    dateFont.setPointSize(9);
    QFont emotionFont = font();
    emotionFont.setPointSize(9);

    // 颜色定义
    QColor dateTextColor = dark ? QColor("#8a929c") : QColor("#8a97a3");
    QColor emotionTextColor = dark ? QColor("#b0b8c0") : QColor("#44515a");
    QColor emptyTextColor = dark ? QColor("#707880") : QColor("#b8c2c8");
    QColor emptyBorderColor = dark ? QColor("#3b434f") : QColor("#d8e0e5");
    QColor footerTextColor = dark ? QColor("#707880") : QColor("#93a0a8");
    // 空记录内部填充色（无色状态）
    QColor emptyFill = ThemeManager::instance().isDarkMode() ? QColor("#2a3037") : QColor("#f3f5f7");

    for (int i = 0; i < 7; ++i) {
        const QRectF cellRect(cellLeft + i * (cellWidth + gap), stripTop, cellWidth, stripHeight);

        QLinearGradient fill(cellRect.topLeft(), cellRect.bottomRight());
        if (cells[i].hasRecord) {
            fill.setColorAt(0.0, cells[i].primary.lighter(112));
            fill.setColorAt(0.55, cells[i].primary);
            fill.setColorAt(1.0, cells[i].secondary.isValid() ? cells[i].secondary : cells[i].primary.darker(108));
        } else {
            // 无记录：使用 fadeToColorless 的结果和 emptyFill 渐变
            fill.setColorAt(0.0, cells[i].primary);
            fill.setColorAt(0.65, blendColors(cells[i].primary, emptyFill, 0.72));
            fill.setColorAt(1.0, emptyFill);
        }

        // 边框
        QColor cellBorder = cells[i].hasRecord
                                ? QColor(255, 255, 255, dark ? 60 : 110)
                                : emptyBorderColor;
        p.setPen(QPen(cellBorder, 1.2));
        p.setBrush(fill);
        p.drawRoundedRect(cellRect, 16, 16);

        // 无记录时再画一层虚线边框？原代码是用实线再画一次，保持一致
        if (!cells[i].hasRecord) {
            p.setPen(QPen(emptyBorderColor, 1));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(cellRect.adjusted(0.6, 0.6, -0.6, -0.6), 15, 15);
        }

        p.setFont(dateFont);
        p.setPen(dateTextColor);
        p.drawText(QRectF(cellRect.left(), labelTop, cellRect.width(), 14),
                   Qt::AlignHCenter | Qt::AlignTop,
                   cells[i].date.toString(QStringLiteral("MM/dd")));

        p.setFont(emotionFont);
        if (cells[i].hasRecord) {
            p.setPen(emotionTextColor);
            p.drawText(QRectF(cellRect.left(), labelTop + 14, cellRect.width(), 16),
                       Qt::AlignHCenter | Qt::AlignTop,
                       cells[i].emotion);
        } else {
            p.setPen(emptyTextColor);
            p.drawText(QRectF(cellRect.left(), labelTop + 14, cellRect.width(), 16),
                       Qt::AlignHCenter | Qt::AlignTop,
                       QStringLiteral("未登记"));
        }
    }

    p.setFont(dateFont);
    p.setPen(footerTextColor);
    p.drawText(QRectF(rect.left() + 20, rect.bottom() - 28, rect.width() - 40, 22),
               Qt::AlignHCenter | Qt::AlignVCenter,
               QStringLiteral("有色方格为已保存心象，留白方格会随相邻心情淡出"));
}
/**void WeeklyChartWidget::drawWeekMoodStrip(QPainter &p, const QRectF &rect) const
{
    drawCard(p, rect, QStringLiteral("本周每日心象色带"),
             QColor("#f6f0ff"), QColor("#eef8f3"));

    const QDate monday = weekMonday();
    const QVector<DayMoodCell> cells = buildWeekCells(m_entries, monday);

    const qreal stripTop = rect.top() + 56;
    const qreal stripHeight = 72;
    const qreal labelTop = stripTop + stripHeight + 12;
    const qreal gap = 10;
    const qreal totalGap = gap * 6;
    const qreal cellWidth = (rect.width() - 48 - totalGap) / 7.0;
    const qreal cellLeft = rect.left() + 24;

    QFont dateFont = font();
    dateFont.setPointSize(9);
    QFont emotionFont = font();
    emotionFont.setPointSize(9);

    for (int i = 0; i < 7; ++i) {
        const QRectF cellRect(cellLeft + i * (cellWidth + gap), stripTop, cellWidth, stripHeight);

        QLinearGradient fill(cellRect.topLeft(), cellRect.bottomRight());
        if (cells[i].hasRecord) {
            fill.setColorAt(0.0, cells[i].primary.lighter(112));
            fill.setColorAt(0.55, cells[i].primary);
            fill.setColorAt(1.0, cells[i].secondary.isValid() ? cells[i].secondary : cells[i].primary.darker(108));
        } else {
            fill.setColorAt(0.0, cells[i].primary);
            fill.setColorAt(0.65, blendColors(cells[i].primary, colorlessFill(), 0.72));
            fill.setColorAt(1.0, colorlessFill());
        }

        p.setPen(QPen(QColor(255, 255, 255, cells[i].hasRecord ? 110 : 180), 1.2));
        p.setBrush(fill);
        p.drawRoundedRect(cellRect, 16, 16);

        if (!cells[i].hasRecord) {
            p.setPen(QPen(QColor("#d8e0e5"), 1));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(cellRect.adjusted(0.6, 0.6, -0.6, -0.6), 15, 15);
        }

        p.setFont(dateFont);
        p.setPen(QColor("#8a97a3"));
        p.drawText(QRectF(cellRect.left(), labelTop, cellRect.width(), 14),
                   Qt::AlignHCenter | Qt::AlignTop,
                   cells[i].date.toString(QStringLiteral("MM/dd")));

        p.setFont(emotionFont);
        if (cells[i].hasRecord) {
            p.setPen(QColor("#44515a"));
            p.drawText(QRectF(cellRect.left(), labelTop + 14, cellRect.width(), 16),
                       Qt::AlignHCenter | Qt::AlignTop,
                       cells[i].emotion);
        } else {
            p.setPen(QColor("#b8c2c8"));
            p.drawText(QRectF(cellRect.left(), labelTop + 14, cellRect.width(), 16),
                       Qt::AlignHCenter | Qt::AlignTop,
                       QStringLiteral("未登记"));
        }
    }

    p.setFont(dateFont);
    p.setPen(QColor("#93a0a8"));
    p.drawText(QRectF(rect.left() + 20, rect.bottom() - 28, rect.width() - 40, 22),
               Qt::AlignHCenter | Qt::AlignVCenter,
               QStringLiteral("有色方格为已保存心象，留白方格会随相邻心情淡出"));
}
*/
void WeeklyChartWidget::drawKeywords(QPainter &p, const QRectF &rect) const
{
    const bool dark = ThemeManager::instance().isDarkMode();

    drawCard(p,
             rect,
             QStringLiteral("高频关键词"),
             dark ? QColor("#2e2a31") : QColor("#fff7ef"),
             dark ? QColor("#2b2d2e") : QColor("#fffdf8"));

    QMap<QString, int> countMap;
    for (const MoodEntry &entry : m_entries) {
        for (const QString &word : entry.keywords) {
            if (!word.trimmed().isEmpty()) {
                countMap[word]++;
            }
        }
    }

    const auto items = sortCountMap(countMap);
    if (items.isEmpty()) {
        p.setPen(dark ? QColor("#8a929c") : QColor("#9aa8b0"));
        QFont f = font();
        f.setPointSize(11);
        p.setFont(f);
        p.drawText(rect.adjusted(24, 68, -24, -24),
                   Qt::AlignLeft | Qt::AlignTop,
                   QStringLiteral("暂无明显关键词"));
        return;
    }

    const QRectF viewRect = rect.adjusted(24, 68, -24, -22);
    const qreal contentH = keywordContentHeight(viewRect);

    p.save();
    p.setClipRect(viewRect);

    qreal x = viewRect.left();
    qreal y = viewRect.top() - m_keywordScrollOffset;
    const qreal maxRight = viewRect.right() - 10;

    const int tagH = 30;
    const int rowGap = 14;

    for (int i = 0; i < items.size(); ++i) {
        const QString word = items.at(i).first;
        const int count = items.at(i).second;

        QFont f = font();
        f.setPointSize(10);
        f.setWeight(i < 2 ? QFont::DemiBold : QFont::Normal);
        p.setFont(f);

        const QString fullText = QStringLiteral("%1  ×%2").arg(word).arg(count);
        QFontMetrics fm(f);

        int tagW = fm.horizontalAdvance(fullText) + 24;
        tagW = qMin(tagW, static_cast<int>(viewRect.width() - 12));

        if (x + tagW > maxRight) {
            x = viewRect.left();
            y += tagH + rowGap;
        }

        const QRectF tagRect(x, y, tagW, tagH);

        if (tagRect.bottom() >= viewRect.top() && tagRect.top() <= viewRect.bottom()) {
            p.setPen(Qt::NoPen);
            p.setBrush(i % 2 == 0
                           ? (dark ? QColor("#2a3330") : QColor("#f3faf5"))
                           : (dark ? QColor("#2b313a") : QColor("#f4f7ff")));
            p.drawRoundedRect(tagRect, 15, 15);

            p.setPen(dark ? QColor("#cbd0d8") : QColor("#44515a"));

            const QString shownText = fm.elidedText(fullText,
                                                    Qt::ElideRight,
                                                    tagW - 18);
            p.drawText(tagRect, Qt::AlignCenter, shownText);
        }

        x += tagW + 12;
    }

    p.restore();

    drawScrollIndicator(p, viewRect, contentH, m_keywordScrollOffset);
}


WeeklyReportWindow::WeeklyReportWindow(DataStore *store, QWidget *parent)
    : QDialog(parent),
    m_store(store),
    m_weekDate(QDate::currentDate())
{
    setWindowTitle(QStringLiteral("每周心象报告"));

    const QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    const int dlgW = qMin(920, avail.width() - 32);
    const int dlgH = qMin(780, avail.height() - 32);
    resize(dlgW, dlgH);
    setMaximumSize(avail.width() - 16, avail.height() - 16);

/*    setStyleSheet(
        "QDialog {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "        stop:0 #e7f3f8, stop:0.45 #edf8f2, stop:1 #fdf0e8);"
        "}"
        "QScrollArea { background: transparent; border: none; }"
        "QWidget#reportScrollContent { background: transparent; }"
        "QFrame#headerCard {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #e8faf3, stop:1 #efe9ff);"
        "    border: 1px solid rgba(205, 224, 218, 180);"
        "    border-radius: 22px;"
        "}"
        "QFrame#insightCard {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f8f0ff, stop:1 #fff6f0);"
        "    border: 1px solid rgba(220, 208, 232, 180);"
        "    border-radius: 22px;"
        "}"
        "QFrame#adviceCard {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #edf9f2, stop:1 #f4fff8);"
        "    border: 1px solid rgba(196, 224, 210, 180);"
        "    border-radius: 22px;"
        "}"
        "QFrame#sceneryCard {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #edf5fb, stop:1 #f7f2ea);"
        "    border: 1px solid rgba(205, 220, 232, 180);"
        "    border-radius: 22px;"
        "}"
        "QLabel#reportTitle { color: #24303a; font-size: 28px; font-weight: 600; }"
        "QLabel#reportEpithet { color: #8a7a92; font-size: 15px; padding-top: 2px; }"
        "QLabel#reportPeriod { color: #6f7d86; font-size: 13px; }"
        "QLabel#reportSource { color: #93a0a8; font-size: 12px; }"
        "QLabel#sectionTitle { color: #7a8790; font-size: 12px; letter-spacing: 1px; }"
        "QLabel#insightBody, QLabel#adviceBody {"
        "    color: #3f4b54;"
        "    font-size: 15px;"
        "    line-height: 1.85;"
        "}"
        "QPushButton {"
        "    border-radius: 16px;"
        "    padding: 10px 20px;"
        "    border: 1px solid #cfe0db;"
        "    background: rgba(255,255,255,210);"
        "}"
        "QPushButton:hover { background: #f0faf5; }"
        "QFrame#footerBar {"
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #eef7fb, stop:1 #f7fbf4);"
        "    border-top: 1px solid #dce8e4;"
        "}"
        );
*/

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);


    m_reportScrollArea = new QScrollArea(this);
    m_reportScrollArea->setWidgetResizable(true);
    m_reportScrollArea->setFrameShape(QFrame::NoFrame);
    m_reportScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_reportScrollContent = new QWidget(m_reportScrollArea);
    m_reportScrollContent->setObjectName(QStringLiteral("reportScrollContent"));

    auto *contentLayout = new QVBoxLayout(m_reportScrollContent);
    contentLayout->setContentsMargins(28, 28, 28, 24);
    contentLayout->setSpacing(24);

    auto *headerCard = new QFrame(m_reportScrollContent);
    headerCard->setObjectName(QStringLiteral("headerCard"));
    auto *headerLayout = new QVBoxLayout(headerCard);
    headerLayout->setContentsMargins(26, 24, 26, 22);
    headerLayout->setSpacing(10);

    auto *title = new ArtTitleLabel(QStringLiteral("本周心象报告"), headerCard);
    title->setObjectName(QStringLiteral("reportTitle"));
    title->setMinimumHeight(42);
    m_epithetLabel = new QLabel(headerCard);
    m_epithetLabel->setObjectName(QStringLiteral("reportEpithet"));
    m_epithetLabel->setFont(ReportArtUtils::scriptFont(15));
    m_periodLabel = new QLabel(headerCard);
    m_periodLabel->setObjectName(QStringLiteral("reportPeriod"));
    m_sourceLabel = new QLabel(headerCard);
    m_sourceLabel->setObjectName(QStringLiteral("reportSource"));

    headerLayout->addWidget(title);
    headerLayout->addWidget(m_epithetLabel);
    headerLayout->addWidget(m_periodLabel);
    headerLayout->addWidget(m_sourceLabel);
    contentLayout->addWidget(headerCard);

    m_chart = new WeeklyChartWidget(m_reportScrollContent);
    m_chart->setMinimumHeight(kWeeklyChartMinHeight);
    contentLayout->addWidget(m_chart);

    auto *insightCard = new QFrame(m_reportScrollContent);
    insightCard->setObjectName(QStringLiteral("insightCard"));
    auto *insightLayout = new QVBoxLayout(insightCard);
    insightLayout->setContentsMargins(26, 22, 26, 24);
    insightLayout->setSpacing(14);

    auto *insightTitle = createSectionTitle(QStringLiteral("本周小结"), insightCard);
    m_insightLabel = createBodyLabel(QStringLiteral("insightBody"), insightCard);

    insightLayout->addWidget(insightTitle);
    insightLayout->addWidget(m_insightLabel);
    contentLayout->addWidget(insightCard);

    auto *adviceCard = new QFrame(m_reportScrollContent);
    adviceCard->setObjectName(QStringLiteral("adviceCard"));
    auto *adviceLayout = new QVBoxLayout(adviceCard);
    adviceLayout->setContentsMargins(26, 22, 26, 24);
    adviceLayout->setSpacing(14);

    auto *adviceTitle = createSectionTitle(QStringLiteral("下周建议"), adviceCard);
    m_adviceLabel = createBodyLabel(QStringLiteral("adviceBody"), adviceCard);

    adviceLayout->addWidget(adviceTitle);
    adviceLayout->addWidget(m_adviceLabel);
    contentLayout->addWidget(adviceCard);

    auto *sceneryCard = new QFrame(m_reportScrollContent);
    sceneryCard->setObjectName(QStringLiteral("sceneryCard"));
    auto *sceneryLayout = new QVBoxLayout(sceneryCard);
    sceneryLayout->setContentsMargins(20, 18, 20, 18);
    sceneryLayout->setSpacing(12);

    auto *sceneryTitle = createSectionTitle(QStringLiteral("本周心境风景"), sceneryCard);
    m_sceneryWidget = new WeeklySceneryWidget(sceneryCard);

    sceneryLayout->addWidget(sceneryTitle);
    sceneryLayout->addWidget(m_sceneryWidget);
    contentLayout->addWidget(sceneryCard);

    m_reportScrollArea->setWidget(m_reportScrollContent);
    mainLayout->addWidget(m_reportScrollArea, 1);


    auto *footer = new QFrame(this);
    footer->setObjectName(QStringLiteral("footerBar"));
    footer->setMinimumHeight(64);
    footer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *buttons = new QHBoxLayout(footer);
    buttons->setContentsMargins(24, 10, 24, 12);
    buttons->addStretch();

    auto *exportPngBtn = new QPushButton(QStringLiteral("导出 PNG"), footer);
    auto *exportPdfBtn = new QPushButton(QStringLiteral("导出 PDF"), footer);
    buttons->addWidget(exportPngBtn);
    buttons->addWidget(exportPdfBtn);
    mainLayout->addWidget(footer);

    connect(exportPngBtn, &QPushButton::clicked, this, &WeeklyReportWindow::exportImage);
    connect(exportPdfBtn, &QPushButton::clicked, this, &WeeklyReportWindow::exportPdf);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            m_chart, [this]() { m_chart->update(); });

    buildReport();
}

double WeeklyReportWindow::averageValence(double fallback) const
{
    if (m_entries.isEmpty()) {
        return fallback;
    }

    double total = 0.0;
    for (const MoodEntry &entry : m_entries) {
        total += entry.valence;
    }
    return total / m_entries.size();
}

double WeeklyReportWindow::averageArousal(double fallback) const
{
    if (m_entries.isEmpty()) {
        return fallback;
    }

    double total = 0.0;
    for (const MoodEntry &entry : m_entries) {
        total += entry.arousal;
    }
    return total / m_entries.size();
}

void WeeklyReportWindow::updateReportPresentation()
{
    const double avgValence = averageValence(0.08);

    if (m_epithetLabel) {
        m_epithetLabel->setText(ReportArtUtils::weekEpithet(avgValence));
    }
    if (m_sceneryWidget) {
        m_sceneryWidget->setWeeklyValence(avgValence);
    }
}

void WeeklyReportWindow::buildReport()
{
    m_entries = m_store->entriesForWeek(m_weekDate);
    m_chart->setEntries(m_entries, m_weekDate);

    const QDate monday = m_weekDate.addDays(1 - m_weekDate.dayOfWeek());
    const QDate sunday = monday.addDays(6);
    m_periodLabel->setText(QStringLiteral("%1  —  %2    ·    %3 条记录")
                               .arg(monday.toString(QStringLiteral("yyyy.MM.dd")))
                               .arg(sunday.toString(QStringLiteral("yyyy.MM.dd")))
                               .arg(m_entries.size()));

    if (m_entries.isEmpty()) {
        m_sourceLabel->setText(QStringLiteral("保存记录后，可在此生成周报。"));
        m_insightLabel->setText(QStringLiteral("本周暂无记录。"));
        m_adviceLabel->clear();
        updateReportPresentation();
        return;
    }

    const LlmConfig llmConfig = m_store->resolveLlmConfig();
    if (!llmConfig.isReady()) {
        m_sourceLabel->setText(QStringLiteral("未配置 API，暂不生成周报文字。"));
        m_insightLabel->setText(QStringLiteral("请先在设置中配置大模型 API。"));
        m_adviceLabel->clear();
        updateReportPresentation();
        return;
    }

    QProgressDialog progress(this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setCancelButton(nullptr);
    progress.setMinimumDuration(0);
    progress.setLabelText(QStringLiteral("正在调用大模型撰写周报文字…"));
    progress.setRange(0, 0);
    progress.show();
    QApplication::processEvents();

    QString llmError;
    const WeeklyReportNarrative narrative = LlmClient::generateWeeklyReport(buildWeekContextForLlm(),
                                                                            llmConfig,
                                                                            &llmError);
    progress.close();

    if (narrative.fromLlm) {
        m_sourceLabel->setText(QStringLiteral("文字部分由大模型 API 生成。"));
        m_insightLabel->setText(ReportArtUtils::formatInsightHtml(narrative.insight));
        m_adviceLabel->setText(ReportArtUtils::formatAdviceHtml(formatAdviceText(narrative.advice)));
        updateReportPresentation();
        return;
    }
    m_sourceLabel->setText(QStringLiteral("大模型暂不可用，未生成周报文字。"));
    if (!llmError.isEmpty()) {
        m_sourceLabel->setToolTip(llmError);
    }

    m_insightLabel->setText(QStringLiteral("大模型调用失败，请稍后重试。"));
    m_adviceLabel->clear();
    updateReportPresentation();
}

QString WeeklyReportWindow::buildWeekContextForLlm() const
{
    const QDate monday = m_weekDate.addDays(1 - m_weekDate.dayOfWeek());
    const QDate sunday = monday.addDays(6);

    const double avgValence = averageValence(0.0);
    const double avgArousal = averageArousal(0.0);

    QStringList emotionParts;
    const QMap<QString, int> emotionMap = emotionCounts();
    for (auto it = emotionMap.begin(); it != emotionMap.end(); ++it) {
        const int percent = static_cast<int>(it.value() * 100.0 / m_entries.size());
        emotionParts << QStringLiteral("%1 %2%").arg(it.key()).arg(percent);
    }

    QStringList keywordParts;
    for (const auto &item : topKeywords(8)) {
        keywordParts << QStringLiteral("%1×%2").arg(item.first).arg(item.second);
    }

    QStringList triggerParts;
    for (const auto &item : topTriggers(6)) {
        triggerParts << QStringLiteral("%1×%2").arg(item.first).arg(item.second);
    }

    QStringList diarySnippets;
    QVector<MoodEntry> sorted = m_entries;
    std::sort(sorted.begin(), sorted.end(), [](const MoodEntry &a, const MoodEntry &b) {
        return a.createdAt < b.createdAt;
    });
    for (int i = 0; i < sorted.size() && i < 8; ++i) {
        const MoodEntry &entry = sorted.at(i);
        const QString snippet = entry.rawText.left(80).replace(QLatin1Char('\n'), QLatin1Char(' '));
        diarySnippets << QStringLiteral("%1 | %2 | 关键词:%3")
                             .arg(entry.date.toString(QStringLiteral("MM-dd")))
                             .arg(snippet)
                             .arg(entry.keywords.join(QStringLiteral("、")));
    }

    return QStringLiteral(
               "请根据以下本周心象数据写周报。\n"
               "统计周期：%1 至 %2\n"
               "记录数量：%3\n"
               "主导情绪：%4\n"
               "情绪分布：%5\n"
               "平均愉悦度：%6\n"
               "平均激活度：%7\n"
               "趋势观察：%8\n"
               "高频关键词：%9\n"
               "触发因素：%10\n"
               "日记摘要：\n%11")
        .arg(monday.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(sunday.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(m_entries.size())
        .arg(dominantEmotion())
        .arg(emotionParts.join(QStringLiteral("、")))
        .arg(avgValence, 0, 'f', 2)
        .arg(avgArousal, 0, 'f', 2)
        .arg(trendSentence())
        .arg(keywordParts.isEmpty() ? QStringLiteral("无") : keywordParts.join(QStringLiteral("、")))
        .arg(triggerParts.isEmpty() ? QStringLiteral("未选择") : triggerParts.join(QStringLiteral("、")))
        .arg(diarySnippets.join(QStringLiteral("\n")));
}

QString WeeklyReportWindow::formatAdviceText(const QString &raw) const
{
    QString text = raw.trimmed();
    if (text.isEmpty()) {
        return text;
    }

    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    if (!text.contains(QLatin1Char('\n'))) {
        text.replace(QStringLiteral("。"), QStringLiteral("。\n\n"));
    }

    QStringList lines;
    for (const QString &line : text.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            lines << trimmed;
        }
    }
    return lines.join(QStringLiteral("\n\n"));
}

QMap<QString, int> WeeklyReportWindow::emotionCounts() const
{
    QMap<QString, int> countMap;
    for (const MoodEntry &entry : m_entries) {
        countMap[entry.emotion]++;
    }
    return countMap;
}

QVector<QPair<QString, int>> WeeklyReportWindow::topKeywords(int limit) const
{
    QMap<QString, int> countMap;
    for (const MoodEntry &entry : m_entries) {
        for (const QString &word : entry.keywords) {
            if (!word.trimmed().isEmpty()) {
                countMap[word]++;
            }
        }
    }

    auto items = sortCountMap(countMap);
    if (items.size() > limit) {
        items.resize(limit);
    }
    return items;
}

QVector<QPair<QString, int>> WeeklyReportWindow::topTriggers(int limit) const
{
    QMap<QString, int> countMap;
    for (const MoodEntry &entry : m_entries) {
        for (const QString &tag : entry.triggerTags) {
            if (!tag.trimmed().isEmpty()) {
                countMap[tag]++;
            }
        }
    }

    auto items = sortCountMap(countMap);
    if (items.size() > limit) {
        items.resize(limit);
    }
    return items;
}

QString WeeklyReportWindow::dominantEmotion() const
{
    const auto items = sortCountMap(emotionCounts());
    return items.isEmpty() ? QStringLiteral("暂无") : items.first().first;
}

QString WeeklyReportWindow::trendSentence() const
{
    if (m_entries.size() < 2) {
        return QStringLiteral("本周记录数量较少，趋势仍在形成中。");
    }

    QVector<MoodEntry> sorted = m_entries;
    std::sort(sorted.begin(), sorted.end(), [](const MoodEntry &a, const MoodEntry &b) {
        return a.createdAt < b.createdAt;
    });

    const double firstValence = sorted.first().valence;
    const double lastValence = sorted.last().valence;
    const double firstArousal = sorted.first().arousal;
    const double lastArousal = sorted.last().arousal;

    const QString valenceTrend = qAbs(lastValence - firstValence) < 0.15
                                     ? QStringLiteral("愉悦度整体较平稳")
                                     : (lastValence > firstValence ? QStringLiteral("愉悦度有回升") : QStringLiteral("愉悦度有下降"));

    const QString arousalTrend = qAbs(lastArousal - firstArousal) < 0.15
                                     ? QStringLiteral("激活度变化不大")
                                     : (lastArousal > firstArousal ? QStringLiteral("激活度有所升高") : QStringLiteral("激活度有所下降"));

    return valenceTrend + QStringLiteral("，") + arousalTrend + QStringLiteral("。");
}

QImage WeeklyReportWindow::renderFullReportImage()
{
    if (!m_reportScrollContent) {
        return QImage();
    }

    QApplication::processEvents();

    const QSize oldSize = m_reportScrollContent->size();

    int exportWidth = oldSize.width();
    if (m_reportScrollArea && m_reportScrollArea->viewport()) {
        exportWidth = m_reportScrollArea->viewport()->width();
    }

    int exportHeight = m_reportScrollContent->sizeHint().height();
    exportHeight = qMax(exportHeight, oldSize.height());

    const QSize exportSize(exportWidth, exportHeight);

    m_reportScrollContent->resize(exportSize);
    if (m_reportScrollContent->layout()) {
        m_reportScrollContent->layout()->activate();
    }

    QImage image(exportSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool dark = ThemeManager::instance().isDarkMode();
    QLinearGradient bg(QPointF(0, 0), QPointF(exportSize.width(), exportSize.height()));
    if (dark) {
        bg.setColorAt(0.0, QColor("#1b2226"));
        bg.setColorAt(0.45, QColor("#1e2627"));
        bg.setColorAt(1.0, QColor("#222322"));
    } else {
        bg.setColorAt(0.0, QColor("#e7f3f8"));
        bg.setColorAt(0.45, QColor("#edf8f2"));
        bg.setColorAt(1.0, QColor("#fdf0e8"));
    }
    painter.fillRect(image.rect(), bg);

    m_reportScrollContent->render(&painter);
    painter.end();
    /**QImage image(exportSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QLinearGradient bg(QPointF(0, 0), QPointF(exportSize.width(), exportSize.height()));
    bg.setColorAt(0.0, QColor("#e7f3f8"));
    bg.setColorAt(0.45, QColor("#edf8f2"));
    bg.setColorAt(1.0, QColor("#fdf0e8"));
    painter.fillRect(image.rect(), bg);

    m_reportScrollContent->render(&painter);
    painter.end();*/

    m_reportScrollContent->resize(oldSize);

    return image;
}

void WeeklyReportWindow::exportImage()
{
    const QString path = QFileDialog::getSaveFileName(this,
                                                      QStringLiteral("导出报告图片"),
                                                      QStringLiteral("mood_weekly_report.png"),
                                                      QStringLiteral("PNG 图片 (*.png)"));
    if (path.isEmpty()) {
        return;
    }

    QImage image = renderFullReportImage();

    if (image.isNull()) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("报告内容渲染失败。"));
        return;
    }

    if (image.save(path)) {
        QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("本周报告图片已保存。"));
    } else {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("图片保存失败，请检查路径权限。"));
    }
}


void WeeklyReportWindow::exportPdf()
{
    const QString path = QFileDialog::getSaveFileName(this,
                                                      QStringLiteral("导出报告 PDF"),
                                                      QStringLiteral("mood_weekly_report.pdf"),
                                                      QStringLiteral("PDF 文件 (*.pdf)"));
    if (path.isEmpty()) {
        return;
    }

    QImage image = renderFullReportImage();

    if (image.isNull()) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("报告内容渲染失败。"));
        return;
    }

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(144);

    QPainter painter(&writer);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRect pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    const qreal scale = pageRect.width() / static_cast<qreal>(image.width());
    const int sourcePageHeight = qMax(1, static_cast<int>(pageRect.height() / scale));

    int sourceY = 0;
    bool firstPage = true;

    while (sourceY < image.height()) {
        if (!firstPage) {
            writer.newPage();
        }
        firstPage = false;

        const int currentSourceHeight = qMin(sourcePageHeight, image.height() - sourceY);

        const QRect sourceRect(0,
                               sourceY,
                               image.width(),
                               currentSourceHeight);

        const QRect targetRect(0,
                               0,
                               pageRect.width(),
                               static_cast<int>(currentSourceHeight * scale));

        painter.fillRect(pageRect, Qt::white);
        painter.drawImage(targetRect, image, sourceRect);

        sourceY += currentSourceHeight;
    }

    painter.end();

    QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("本周报告 PDF 已保存。"));
}
