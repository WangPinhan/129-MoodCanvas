#include "plantwidget.h"

#include <QPixmap>
#include <QPainterPath>
#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QSizePolicy>


static QString plantImagePath(PlantWidget::PlantState state)
{
    switch (state) {
    case PlantWidget::PlantState::Bloom:
        return QStringLiteral(":/images/plant_bloom.png");
    case PlantWidget::PlantState::Leaves:
        return QStringLiteral(":/images/plant_leaves.png");
    case PlantWidget::PlantState::Curled:
        return QStringLiteral(":/images/plant_curled.png");
    case PlantWidget::PlantState::Rain:
        return QStringLiteral(":/images/plant_rain.png");
    case PlantWidget::PlantState::Dormant:
    default:
        return QStringLiteral(":/images/plant_dormant.png");
    }
}

static void drawPlantImageOnly(QPainter &p,
                               const QRectF &rect,
                               PlantWidget::PlantState state)
{
    const QString path = plantImagePath(state);
    QPixmap pixmap(path);

    const QRectF imageRect = rect.adjusted(18, 18, -18, -18);

    if (pixmap.isNull()) {
        p.setPen(QColor("#8a97a3"));

        QFont hintFont = p.font();
        hintFont.setPointSize(10);
        p.setFont(hintFont);

        p.drawText(imageRect,
                   Qt::AlignCenter | Qt::TextWordWrap,
                   QStringLiteral("未找到图片：\n%1").arg(path));
        return;
    }

    QPixmap scaled = pixmap.scaled(imageRect.size().toSize(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);

    const QPointF topLeft(imageRect.center().x() - scaled.width() / 2.0,
                          imageRect.center().y() - scaled.height() / 2.0);

    p.drawPixmap(topLeft, scaled);
}
PlantWidget::PlantWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(170);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void PlantWidget::setPlantState(PlantState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    update();
}

PlantWidget::PlantState PlantWidget::plantState() const
{
    return m_state;
}

QString PlantWidget::stateTitle() const
{
    switch (m_state) {
    case PlantState::Bloom:
        return QStringLiteral("今日植物：开花中");
    case PlantState::Leaves:
        return QStringLiteral("今日植物：长出新叶");
    case PlantState::Curled:
        return QStringLiteral("今日植物：叶子有点卷曲");
    case PlantState::Rain:
        return QStringLiteral("今日植物：雨中继续生长");
    case PlantState::Dormant:
    default:
        return QStringLiteral("今日植物：休眠等待");
    }
}

QString PlantWidget::stateHint() const
{
    switch (m_state) {
    case PlantState::Bloom:
        return QStringLiteral("开心的记录像阳光，小花悄悄开放。");
    case PlantState::Leaves:
        return QStringLiteral("平静或疲惫也会被温柔接住，叶子慢慢伸展。");
    case PlantState::Curled:
        return QStringLiteral("焦虑时叶子会卷起来，但植物仍在被照顾。");
    case PlantState::Rain:
        return QStringLiteral("难过像一场雨，植物没有枯萎，只是在雨里继续长大。");
    case PlantState::Dormant:
    default:
        return QStringLiteral("今天还没记录也没关系，它只是安静休眠，不会枯萎。");
    }
}

void PlantWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF r = rect().adjusted(8, 8, -8, -8);

    QPainterPath cardPath;
    cardPath.addRoundedRect(r, 28, 28);

    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, QColor("#f7fff8"));
    bg.setColorAt(0.55, QColor("#edf7ff"));
    bg.setColorAt(1.0, QColor("#fff6ef"));

    p.fillPath(cardPath, bg);

    p.save();
    p.setClipPath(cardPath);

    drawPlantImageOnly(p, r, m_state);

    p.restore();
}
