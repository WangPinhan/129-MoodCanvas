#include "moodcanvaswidget.h"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>


static bool intersectsAny(const QRectF &rect, const QVector<QRectF> &usedRects, qreal gap = 6.0)
{
    const QRectF padded = rect.adjusted(-gap, -gap, gap, gap);

    for (const QRectF &used : usedRects) {
        if (padded.intersects(used)) {
            return true;
        }
    }

    return false;
}

static QRectF bubbleRectForText(QPainter &p,
                                const QString &text,
                                const QPointF &center,
                                QString *shownText = nullptr)
{
    const int maxTextWidth = 92;
    QFontMetricsF fm(p.font());

    const QString shown = fm.elidedText(text, Qt::ElideRight, maxTextWidth);
    if (shownText) {
        *shownText = shown;
    }

    const qreal padX = 12;
    const qreal padY = 7;
    const qreal textW = fm.horizontalAdvance(shown);
    const qreal textH = fm.height();

    return QRectF(center.x() - textW / 2.0 - padX,
                  center.y() - textH / 2.0 - padY,
                  textW + padX * 2.0,
                  textH + padY * 2.0);
}

static bool findFreeBubbleRect(QPainter &p,
                               const QRectF &safeArea,
                               const QString &text,
                               const QPointF &baseCenter,
                               QVector<QRectF> &usedRects,
                               QRectF &outRect,
                               QString &outText)
{
    const QVector<QPointF> offsets = {
        QPointF(0, 0),
        QPointF(-46, -28),
        QPointF(46, -28),
        QPointF(-58, 30),
        QPointF(58, 30),
        QPointF(0, -52),
        QPointF(0, 52),
        QPointF(-88, 0),
        QPointF(88, 0)
    };

    for (const QPointF &offset : offsets) {
        QString shown;
        const QRectF rect = bubbleRectForText(p, text, baseCenter + offset, &shown);

        if (!safeArea.contains(rect)) {
            continue;
        }

        if (intersectsAny(rect, usedRects)) {
            continue;
        }

        outRect = rect;
        outText = shown;
        usedRects.append(rect);
        return true;
    }

    return false;
}

namespace {

QColor blendColor(const QColor &a, const QColor &b, qreal t)
{
    return QColor(
        int(a.red() * (1 - t) + b.red() * t),
        int(a.green() * (1 - t) + b.green() * t),
        int(a.blue() * (1 - t) + b.blue() * t),
        int(a.alpha() * (1 - t) + b.alpha() * t));
}

} // namespace

MoodCanvasWidget::MoodCanvasWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(440, 280);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(&m_timer, &QTimer::timeout, this, [this]() {
        m_phase += 0.018 + m_entry.arousal * 0.038;
        update();
    });
    m_timer.start(42);
}

void MoodCanvasWidget::setEntry(const MoodEntry &entry)
{
    m_entry = entry;
    rebuildVisualContext();
    update();
}

QPixmap MoodCanvasWidget::grabCanvas()
{
    const QSize targetSize = size().expandedTo(QSize(760, 360));

    MoodCanvasWidget offscreen;
    offscreen.m_timer.stop();
    offscreen.setEntry(m_entry);
    offscreen.m_phase = m_phase;
    offscreen.resize(targetSize);

    QPixmap pixmap(targetSize);
    pixmap.fill(Qt::transparent);

    offscreen.render(&pixmap);

    return pixmap;
}

void MoodCanvasWidget::rebuildVisualContext()
{
    m_textSeed = hashText(m_entry.rawText);
    for (const QString &kw : m_entry.keywords) {
        m_textSeed ^= hashText(kw);
    }

    const QString corpus = m_entry.rawText + m_entry.keywords.join(QString())
                           + m_entry.triggerTags.join(QString());

    m_theme = VisualTheme{};
    if (corpus.contains(QStringLiteral("雨")) || corpus.contains(QStringLiteral("云"))
        || corpus.contains(QStringLiteral("雾")) || corpus.contains(QStringLiteral("阴"))) {
        m_theme.mist = true;
    }
    if (corpus.contains(QStringLiteral("星")) || corpus.contains(QStringLiteral("光"))
        || corpus.contains(QStringLiteral("月")) || corpus.contains(QStringLiteral("晴"))) {
        m_theme.starlight = true;
    }
    if (corpus.contains(QStringLiteral("火")) || corpus.contains(QStringLiteral("怒"))
        || corpus.contains(QStringLiteral("燥")) || m_entry.emotion == QStringLiteral("愤怒")) {
        m_theme.ember = true;
    }
    if (corpus.contains(QStringLiteral("风")) || corpus.contains(QStringLiteral("叶"))
        || corpus.contains(QStringLiteral("自然"))) {
        m_theme.breeze = true;
    }
}


quint32 MoodCanvasWidget::hashText(const QString &text) const
{
    quint32 h = 2166136261u;
    for (const QChar ch : text) {
        h ^= static_cast<quint32>(ch.unicode());
        h *= 16777619u;
    }
    return h ? h : 1u;
}

qreal MoodCanvasWidget::seeded(qreal min, qreal max, int salt) const
{
    const quint32 v = m_textSeed ^ static_cast<quint32>(salt * 2654435761u);
    const qreal t = (v % 10000) / 10000.0;
    return min + (max - min) * t;
}

QVector<QColor> MoodCanvasWidget::emotionPalette() const
{
    if (!m_entry.colorPalette.isEmpty()) {
        return m_entry.colorPalette;
    }

    const QString &e = m_entry.emotion;

    // 喜怒哀乐：每种情绪一组高辨识度、低饱和渐变色（无分析色盘时的回退）
    if (e == QStringLiteral("开心")) {
        return {QColor("#FFB347"), QColor("#FFD166"), QColor("#FF9F68"), QColor("#FFF3C4")}; // 喜·暖金杏
    }
    if (e == QStringLiteral("愤怒")) {
        return {QColor("#E63946"), QColor("#FF6B6B"), QColor("#C1121F"), QColor("#FF9B85")}; // 怒·赤绯
    }
    if (e == QStringLiteral("难过")) {
        return {QColor("#5C6BC0"), QColor("#7986CB"), QColor("#9FA8DA"), QColor("#B39DDB")}; // 哀·靛紫
    }
    if (e == QStringLiteral("平静")) {
        return {QColor("#4ECDC4"), QColor("#7BDFF2"), QColor("#95E1D3"), QColor("#E8FFF8")}; // 乐·青碧
    }
    if (e == QStringLiteral("焦虑")) {
        return {QColor("#B388FF"), QColor("#9575CD"), QColor("#CE93D8"), QColor("#EDE7F6")};
    }
    if (e == QStringLiteral("疲惫")) {
        return {QColor("#90A4AE"), QColor("#B0BEC5"), QColor("#CFD8DC"), QColor("#ECEFF1")};
    }
    if (e == QStringLiteral("混合情绪")) {
        // 四色并存，体现喜怒哀乐的交织
        return {QColor("#FFB347"), QColor("#E63946"), QColor("#7986CB"), QColor("#4ECDC4")};
    }

    // 默认：按愉悦度在冷暖色带间插值
    if (m_entry.valence >= 0.25) {
        return {QColor("#FFD166"), QColor("#FF9F68"), QColor("#4ECDC4"), QColor("#FFF3C4")};
    }
    if (m_entry.valence <= -0.25) {
        return {QColor("#7986CB"), QColor("#9575CD"), QColor("#90A4AE"), QColor("#B39DDB")};
    }
    return {QColor("#B388FF"), QColor("#4ECDC4"), QColor("#FFD166"), QColor("#E8EAF6")};
}

QColor MoodCanvasWidget::paletteColor(int index) const
{
    const QVector<QColor> palette = emotionPalette();
    if (palette.isEmpty()) {
        return QColor("#a8b8c8");
    }
    return palette.at(index % palette.size());
}

QString MoodCanvasWidget::excerptText() const
{
    QString text = m_entry.rawText.trimmed();
    text.replace('\n', ' ');
    if (text.isEmpty()) {
        return QStringLiteral("今日尚未写下话语，但心绪已被轻轻记录。");
    }
    if (text.length() > 28) {
        return text.left(28) + QStringLiteral("…");
    }
    return text;
}

void MoodCanvasWidget::drawBackground(QPainter &p, const QRectF &canvas)
{
    const QVector<QColor> palette = emotionPalette();

    QLinearGradient base(canvas.topLeft(), canvas.bottomRight());
    base.setColorAt(0.0, blendColor(palette[0], QColor("#faf8f5"), 0.35));
    base.setColorAt(0.38, blendColor(palette[qMin(1, palette.size() - 1)], QColor("#ffffff"), 0.25));
    base.setColorAt(0.72, blendColor(palette[qMin(2, palette.size() - 1)], QColor("#f5f7fb"), 0.2));
    base.setColorAt(1.0, blendColor(palette.last(), QColor("#eef2f8"), 0.3));
    p.fillRect(canvas, base);

    // 仅用线性色带铺陈，不用径向圆斑（避免像「大圆球」）
    for (int i = 0; i < palette.size(); ++i) {
        const qreal bandH = canvas.height() * 0.22;
        const qreal y = canvas.top() + bandH * i * 0.85;
        QLinearGradient band(canvas.left(), y, canvas.right(), y + bandH);
        QColor c = palette[i];
        c.setAlpha(28 + i * 8);
        band.setColorAt(0.0, QColor(255, 255, 255, 0));
        band.setColorAt(0.5, c);
        band.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.fillRect(QRectF(canvas.left(), y, canvas.width(), bandH), band);
    }
}

void MoodCanvasWidget::drawColorRibbons(QPainter &p, const QRectF &canvas)
{
    const QVector<QColor> palette = emotionPalette();
    const int ribbons = qMax(4, palette.size());

    for (int i = 0; i < ribbons; ++i) {
        QPainterPath ribbon;
        const qreal yStart = canvas.height() * (0.12 + i * 0.11);
        ribbon.moveTo(canvas.left() - 20, yStart);
        for (int x = -20; x <= int(canvas.width()) + 20; x += 14) {
            const qreal drift = qSin(x * 0.012 + m_phase * (0.7 + i * 0.08) + i) * (12 + m_entry.arousal * 16);
            ribbon.lineTo(canvas.left() + x, yStart + drift);
        }
        ribbon.lineTo(canvas.right() + 20, canvas.bottom() + 20);
        ribbon.lineTo(canvas.left() - 20, canvas.bottom() + 20);
        ribbon.closeSubpath();

        QColor fill = paletteColor(i);
        fill.setAlpha(22 + i * 6);
        p.fillPath(ribbon, fill);
    }
}

void MoodCanvasWidget::drawAuroraBands(QPainter &p, const QRectF &canvas)
{
    const int bands = 4 + static_cast<int>(m_entry.arousal * 3);
    for (int b = 0; b < bands; ++b) {
        QPainterPath band;
        const qreal y0 = canvas.height() * (0.38 + b * 0.07);
        band.moveTo(canvas.left(), y0);
        for (int x = 0; x <= int(canvas.width()); x += 10) {
            const qreal amp = 5 + m_entry.arousal * 16 + seeded(0, 5, 200 + b * 17 + x);
            const qreal y = y0 + qSin(x * 0.02 + m_phase * (0.85 + b * 0.12) + b) * amp;
            band.lineTo(canvas.left() + x, y);
        }

        QColor stroke = paletteColor(b);
        stroke.setAlpha(55 + b * 12);
        p.setPen(QPen(stroke, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(band);
    }

    if (m_theme.breeze) {
        p.setPen(QPen(blendColor(paletteColor(0), paletteColor(2), 0.5), 1));
        for (int i = 0; i < 5; ++i) {
            QPainterPath leaf;
            const qreal x0 = canvas.width() * seeded(0.1, 0.7, 1000 + i);
            const qreal y0 = canvas.height() * seeded(0.2, 0.6, 1100 + i);
            leaf.moveTo(x0, y0);
            leaf.quadTo(x0 + 40, y0 - 18 + qSin(m_phase + i) * 8, x0 + 80, y0 + 6);
            p.drawPath(leaf);
        }
    }
}

void MoodCanvasWidget::drawKeywordConstellation(QPainter &p, const QRectF &canvas)
{
    QStringList labels;
    for (const QString &kw : m_entry.keywords) {
        const QString cleaned = kw.simplified();
        if (!cleaned.isEmpty()) {
            labels << cleaned;
        }
    }

    if (labels.isEmpty()) {
        labels << QStringLiteral("心绪");
    }

    const int maxLabelCount = canvas.width() < 560 || canvas.height() < 240 ? 3 : 5;
    labels = labels.mid(0, maxLabelCount);

    QFont kwFont = font();
    kwFont.setPointSize(canvas.height() < 240 ? 8 : 9);
    kwFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
    p.setFont(kwFont);

    QVector<QRectF> usedRects;

    // 顶部标题、色条、情绪说明区域，先预留出来
    const QRectF headerBlock(canvas.left() + 22,
                             canvas.top() + 16,
                             canvas.width() - 44,
                             72);
    usedRects.append(headerBlock);

    // 底部原文摘录卡片区域，也预留出来
    const QString quote = excerptText();
    QFont quoteFont = font();
    quoteFont.setPointSize(10);
    quoteFont.setItalic(true);
    quoteFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);

    QFontMetrics quoteFm(quoteFont);
    const int textW = qMin(int(canvas.width() * 0.62), quoteFm.horizontalAdvance(quote) + 36);
    const int textH = quoteFm.height() + 22;
    const QRectF textCard(canvas.left() + 24,
                          canvas.bottom() - textH - 22,
                          textW,
                          textH);
    usedRects.append(textCard.adjusted(-10, -10, 10, 10));

    const QRectF safeArea = canvas.adjusted(18, 76, -18, -18);

    QVector<QPointF> baseCenters = {
        QPointF(canvas.left() + canvas.width() * 0.18, canvas.top() + canvas.height() * 0.48),
        QPointF(canvas.left() + canvas.width() * 0.38, canvas.top() + canvas.height() * 0.40),
        QPointF(canvas.left() + canvas.width() * 0.62, canvas.top() + canvas.height() * 0.56),
        QPointF(canvas.left() + canvas.width() * 0.80, canvas.top() + canvas.height() * 0.43),
        QPointF(canvas.left() + canvas.width() * 0.48, canvas.top() + canvas.height() * 0.70)
    };

    struct PlacedBubble {
        QRectF rect;
        QString text;
        int colorIndex = 0;
        QPointF center;
    };

    QVector<PlacedBubble> placed;

    for (int i = 0; i < labels.size(); ++i) {
        QPointF base = baseCenters.at(i % baseCenters.size());
        base.setY(base.y() + qSin(m_phase * 0.6 + i) * 4.0);

        QRectF bubbleRect;
        QString shownText;

        if (!findFreeBubbleRect(p,
                                safeArea,
                                labels.at(i),
                                base,
                                usedRects,
                                bubbleRect,
                                shownText)) {
            continue;
        }

        PlacedBubble bubble;
        bubble.rect = bubbleRect;
        bubble.text = shownText;
        bubble.colorIndex = i;
        bubble.center = bubbleRect.center();
        placed.append(bubble);
    }

    // 先画连线，避免线压在文字上
    for (int i = 0; i < placed.size() - 1; ++i) {
        QColor lineColor = paletteColor(placed.at(i).colorIndex);
        lineColor.setAlpha(58);

        p.setPen(QPen(lineColor, 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(placed.at(i).center, placed.at(i + 1).center);
    }

    // 再画气泡
    for (const PlacedBubble &bubble : placed) {
        QColor chipFill = blendColor(paletteColor(bubble.colorIndex), QColor("#ffffff"), 0.62);
        chipFill.setAlpha(210);

        QColor chipBorder = paletteColor(bubble.colorIndex);
        chipBorder.setAlpha(150);

        QPainterPath chipPath;
        chipPath.addRoundedRect(bubble.rect,
                                bubble.rect.height() / 2.0,
                                bubble.rect.height() / 2.0);

        p.setPen(QPen(chipBorder, 1.2));
        p.setBrush(chipFill);
        p.drawPath(chipPath);

        p.setPen(chipBorder.darker(125));
        p.drawText(bubble.rect, Qt::AlignCenter, bubble.text);
    }
}


void MoodCanvasWidget::drawMistLayer(QPainter &p, const QRectF &canvas)
{
    if (!m_theme.mist) {
        return;
    }

    QColor mistTint = blendColor(paletteColor(2), QColor("#ffffff"), 0.55);
    for (int i = 0; i < 3; ++i) {
        QPainterPath fog;
        const qreal y = canvas.height() * (0.2 + i * 0.14) + qSin(m_phase * 0.3 + i) * 6;
        fog.moveTo(canvas.left(), y);
        for (int x = 0; x <= int(canvas.width()); x += 18) {
            fog.lineTo(canvas.left() + x,
                       y + qSin(x * 0.01 + m_phase + i) * 12);
        }
        fog.lineTo(canvas.right(), canvas.bottom());
        fog.lineTo(canvas.left(), canvas.bottom());
        fog.closeSubpath();

        mistTint.setAlpha(int(24 + seeded(0, 16, 620 + i)));
        p.fillPath(fog, mistTint);
    }
}

void MoodCanvasWidget::drawStarlight(QPainter &p, const QRectF &canvas)
{
    const int stars = (m_theme.starlight ? 16 : 6)
                      + static_cast<int>(m_entry.arousal * 18)
                      + qMin(10, m_entry.rawText.length() / 14);

    for (int i = 0; i < stars; ++i) {
        const qreal x = canvas.width() * seeded(0.05, 0.95, 700 + i * 3);
        const qreal y = canvas.height() * seeded(0.06, 0.58, 800 + i * 5)
                        + qSin(m_phase + i * 0.7) * 4;

        QColor star = paletteColor(i);
        star.setAlpha(int(90 + seeded(0, 100, 910 + i)));
        p.setPen(QPen(star, 1.2));
        p.drawPoint(QPointF(x, y));
        if (m_theme.starlight && i % 4 == 0) {
            p.drawLine(QPointF(x - 2, y), QPointF(x + 2, y));
            p.drawLine(QPointF(x, y - 2), QPointF(x, y + 2));
        }
    }
}

void MoodCanvasWidget::drawTextCard(QPainter &p, const QRectF &canvas)
{
    const QString quote = excerptText();
    QFont quoteFont = font();
    quoteFont.setPointSize(10);
    quoteFont.setItalic(true);
    quoteFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
    p.setFont(quoteFont);

    const QFontMetrics fm(quoteFont);
    const int textW = qMin(int(canvas.width() * 0.62), fm.horizontalAdvance(quote) + 36);
    const int textH = fm.height() + 22;
    QRectF card(canvas.left() + 24, canvas.bottom() - textH - 22, textW, textH);

    QLinearGradient cardBg(card.topLeft(), card.bottomRight());
    cardBg.setColorAt(0.0, QColor(255, 255, 255, 200));
    cardBg.setColorAt(1.0, blendColor(paletteColor(0), QColor("#ffffff"), 0.7));
    QPainterPath cardPath;
    cardPath.addRoundedRect(card, 14, 14);
    p.fillPath(cardPath, cardBg);
    p.setPen(QPen(paletteColor(0), 1));
    p.drawPath(cardPath);

    const QString shownQuote = fm.elidedText(quote, Qt::ElideRight, qMax(20, textW - 32));

    p.setPen(QColor("#3f4d5a"));
    p.drawText(card.adjusted(16, 8, -16, -8),
               Qt::AlignLeft | Qt::AlignVCenter,
               shownQuote);

}

void MoodCanvasWidget::drawHeader(QPainter &p, const QRectF &canvas)
{
    QFont titleFont = font();
    titleFont.setPointSize(canvas.height() < 240 ? 18 : 21);
    titleFont.setBold(true);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    p.setFont(titleFont);

    p.setPen(paletteColor(0).darker(135));

    const QString title = m_entry.emotion.isEmpty()
                              ? QStringLiteral("今日心象")
                              : m_entry.emotion;

    const QRectF titleRect(canvas.left() + 28,
                           canvas.top() + 18,
                           canvas.width() - 56,
                           30);

    p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

    const QVector<QColor> palette = emotionPalette();
    qreal x = canvas.left() + 28;
    const qreal y = canvas.top() + 52;

    for (int i = 0; i < palette.size(); ++i) {
        QRectF swatch(x + i * 22, y, 16, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(swatch, 3, 3);
    }

    QFont subFont = font();
    subFont.setPointSize(canvas.height() < 240 ? 8 : 9);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    p.setFont(subFont);
    p.setPen(QColor("#5f6f7a"));

    QString meta = m_entry.emotionSummary;
    if (meta.isEmpty()) {
        meta = QStringLiteral("情绪较为平和");
    }

    if (!m_entry.keywords.isEmpty()) {
        meta += QStringLiteral("  ·  ") + m_entry.keywords.mid(0, 4).join(QStringLiteral(" · "));
    }

    const QRectF metaRect(canvas.left() + 28,
                          canvas.top() + 64,
                          canvas.width() - 56,
                          22);

    const QFontMetrics metaFm(subFont);
    const QString shownMeta = metaFm.elidedText(meta, Qt::ElideRight, int(metaRect.width()));

    p.drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter, shownMeta);
}

void MoodCanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    QRectF canvas = rect().adjusted(3, 3, -3, -3);

    QPainterPath clipPath;
    clipPath.addRoundedRect(canvas, 32, 32);
    p.setClipPath(clipPath);

    drawBackground(p, canvas);
    drawColorRibbons(p, canvas);
    drawAuroraBands(p, canvas);
    drawMistLayer(p, canvas);
    drawStarlight(p, canvas);
    drawKeywordConstellation(p, canvas);

    p.setClipping(false);
    drawHeader(p, canvas);
    drawTextCard(p, canvas);
}
