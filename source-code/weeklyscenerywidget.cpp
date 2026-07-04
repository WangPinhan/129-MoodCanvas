#include "weeklyscenerywidget.h"
#include "reportartutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {

QColor withAlpha(const QColor &color, int alpha)
{
    QColor c = color;
    c.setAlpha(alpha);
    return c;
}
QString toneBaseName(WeeklySceneryWidget::SceneryTone tone)
{
    return tone == WeeklySceneryWidget::SceneryTone::Positive
               ? QStringLiteral("scenery_positive")
               : QStringLiteral("scenery_negative");
}

QStringList imageCandidates(const QString &baseName)
{
    return {
        baseName + QStringLiteral(".jpg"),
        baseName + QStringLiteral(".jpeg"),
        baseName + QStringLiteral(".png"),
        baseName + QStringLiteral(".webp"),
    };
}

} // namespace

WeeklySceneryWidget::WeeklySceneryWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(340);
    setMaximumHeight(380);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    reloadPixmap();

    connect(&m_timer, &QTimer::timeout, this, [this]() {
        m_phase += 0.016;
        update();
    });
    m_timer.start(42);
}

void WeeklySceneryWidget::setTone(SceneryTone tone)
{
    if (m_tone == tone && !m_sourcePixmap.isNull()) {
        return;
    }
    m_tone = tone;
    reloadPixmap();
    update();
}

void WeeklySceneryWidget::setWeeklyValence(double avgValence)
{
    setTone(avgValence >= 0.0 ? SceneryTone::Positive : SceneryTone::Negative);
}

void WeeklySceneryWidget::reloadPixmap()
{
    m_sourcePixmap = loadSceneryAsset(m_tone);
}

QPixmap WeeklySceneryWidget::loadSceneryAsset(SceneryTone tone)
{
    const QString baseName = toneBaseName(tone);
    const QStringList fileNames = imageCandidates(baseName);

    for (const QString &fileName : fileNames) {
        const QPixmap fromResource(QStringLiteral(":/images/") + fileName);
        if (!fromResource.isNull()) {
            return fromResource;
        }
    }

    QStringList searchDirs;
    searchDirs << QCoreApplication::applicationDirPath() + QStringLiteral("/images");
    searchDirs << QCoreApplication::applicationDirPath()
                      + QStringLiteral("/../../../images");
    searchDirs << QDir(QCoreApplication::applicationDirPath())
                      .absoluteFilePath(QStringLiteral("../../images"));

    const QDir appDir(QCoreApplication::applicationDirPath());
    searchDirs << appDir.absoluteFilePath(QStringLiteral("../images"));
    searchDirs << appDir.absoluteFilePath(QStringLiteral("../../MoodCanvasQt/images"));

    searchDirs.removeDuplicates();

    for (const QString &dirPath : searchDirs) {
        const QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }
        for (const QString &fileName : fileNames) {
            const QString fullPath = dir.filePath(fileName);
            if (!QFile::exists(fullPath)) {
                continue;
            }
            QPixmap pix(fullPath);
            if (!pix.isNull()) {
                return pix;
            }
        }
    }

    return QPixmap();
}

QRectF WeeklySceneryWidget::innerBounds() const
{
    return rect().adjusted(3, 3, -3, -3);
}

void WeeklySceneryWidget::drawCoverImage(QPainter &p, const QRectF &bounds) const
{
    if (m_sourcePixmap.isNull()) {
        QLinearGradient fallback(bounds.topLeft(), bounds.bottomRight());
        if (m_tone == SceneryTone::Positive) {
            fallback.setColorAt(0.0, QColor("#f7e8d8"));
            fallback.setColorAt(1.0, QColor("#d8ebf5"));
        } else {
            fallback.setColorAt(0.0, QColor("#ddd0f0"));
            fallback.setColorAt(1.0, QColor("#f0d8e8"));
        }
        p.fillRect(bounds, fallback);

        p.setPen(QColor("#8a97a3"));
        QFont hint = ReportArtUtils::displayFont(11);
        p.setFont(hint);
        p.drawText(bounds.adjusted(24, 0, -24, 0),
                   Qt::AlignCenter,
                   QStringLiteral("请将插画放入 images 文件夹：\n%1.jpg / .png")
                       .arg(toneBaseName(m_tone)));
        return;
    }

    const qreal breathe = 1.0 + 0.012 * qSin(m_phase * 0.55);
    const QSize targetSize(int(bounds.width() * breathe), int(bounds.height() * breathe));
    QPixmap scaled = m_sourcePixmap.scaled(targetSize,
                                           Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation);

    const QPointF topLeft(bounds.center().x() - scaled.width() / 2.0,
                          bounds.center().y() - scaled.height() / 2.0
                              + 4.0 * qSin(m_phase * 0.7));
    p.drawPixmap(topLeft, scaled);
}

void WeeklySceneryWidget::drawPositiveEffects(QPainter &p, const QRectF &bounds) const
{
    const QPointF sunCenter(bounds.left() + bounds.width() * 0.58,
                            bounds.top() + bounds.height() * 0.34);
    const qreal pulse = 0.5 + 0.5 * qSin(m_phase * 1.1);

    QRadialGradient glow(sunCenter, bounds.width() * (0.16 + pulse * 0.03));
    glow.setColorAt(0.0, QColor(255, 244, 210, int(70 + pulse * 50)));
    glow.setColorAt(0.45, QColor(255, 220, 170, int(28 + pulse * 24)));
    glow.setColorAt(1.0, QColor(255, 220, 170, 0));
    p.fillRect(bounds, glow);

    p.setCompositionMode(QPainter::CompositionMode_Screen);
    for (int i = 0; i < 5; ++i) {
        const qreal y = bounds.top() + bounds.height() * (0.52 + i * 0.035);
        const qreal drift = qSin(m_phase * (1.0 + i * 0.12) + i) * bounds.width() * 0.02;
        QLinearGradient band(QPointF(bounds.left(), y), QPointF(bounds.right(), y + 8));
        band.setColorAt(0.0, QColor(255, 255, 255, 0));
        band.setColorAt(0.35, QColor(255, 248, 230, int(18 + i * 6)));
        band.setColorAt(0.65, QColor(255, 255, 255, int(28 + i * 5)));
        band.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.fillRect(QRectF(bounds.left() + drift, y - 2, bounds.width(), 10), band);
    }

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 10; ++i) {
        const qreal t = i / 9.0;
        const qreal x = bounds.left() + bounds.width() * (0.46 + t * 0.18)
                        + qSin(m_phase * 1.6 + i) * 8.0;
        const qreal y = bounds.top() + bounds.height() * 0.56 + qSin(m_phase + i * 0.8) * 3.0;
        p.setBrush(QColor(255, 255, 255, int(60 + (i % 3) * 25)));
        p.drawEllipse(QPointF(x, y), 2.0 + (i % 2), 1.2);
    }
}

void WeeklySceneryWidget::drawNegativeEffects(QPainter &p, const QRectF &bounds) const
{
    const QPointF sunCenter(bounds.left() + bounds.width() * 0.56,
                            bounds.top() + bounds.height() * 0.42);
    const qreal pulse = 0.5 + 0.5 * qSin(m_phase * 0.85);

    p.setCompositionMode(QPainter::CompositionMode_Screen);
    for (int ray = 0; ray < 5; ++ray) {
        const qreal angle = -0.35 + ray * 0.18 + qSin(m_phase * 0.4 + ray) * 0.03;
        QLinearGradient rayGrad(sunCenter,
                                sunCenter + QPointF(qCos(angle), qSin(angle)) * bounds.height());
        rayGrad.setColorAt(0.0, QColor(255, 220, 180, int(55 + pulse * 35)));
        rayGrad.setColorAt(0.35, QColor(220, 180, 255, int(28 + pulse * 18)));
        rayGrad.setColorAt(1.0, QColor(220, 180, 255, 0));

        QPainterPath rayPath;
        rayPath.moveTo(sunCenter);
        rayPath.lineTo(sunCenter + QPointF(qCos(angle - 0.05), qSin(angle - 0.05)) * bounds.height());
        rayPath.lineTo(sunCenter + QPointF(qCos(angle + 0.05), qSin(angle + 0.05)) * bounds.height());
        rayPath.closeSubpath();
        p.fillPath(rayPath, rayGrad);
    }

    const qreal fogShift = qSin(m_phase * 0.55) * bounds.width() * 0.03;
    QLinearGradient fog(bounds.topLeft(), bounds.bottomLeft());
    fog.setColorAt(0.0, QColor(180, 160, 220, 0));
    fog.setColorAt(0.45, QColor(190, 170, 230, int(16 + pulse * 12)));
    fog.setColorAt(0.72, QColor(150, 180, 220, int(10 + pulse * 10)));
    fog.setColorAt(1.0, QColor(255, 255, 255, 0));
    p.fillRect(bounds.translated(fogShift, 0), fog);

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (int i = 0; i < 6; ++i) {
        const qreal y = bounds.top() + bounds.height() * (0.58 + i * 0.04);
        const qreal drift = qSin(m_phase * (0.8 + i * 0.1) + i) * 10.0;
        QPen pen(withAlpha(QColor("#fff6ea"), int(20 + i * 5)), 1.2);
        p.setPen(pen);
        QPainterPath ripple;
        ripple.moveTo(bounds.left(), y);
        for (qreal x = bounds.left(); x <= bounds.right(); x += 5) {
            ripple.lineTo(x + drift, y + 2.0 * qSin(x * 0.03 + m_phase + i));
        }
        p.drawPath(ripple);
    }
}

void WeeklySceneryWidget::drawAnimatedLight(QPainter &p, const QRectF &bounds) const
{
    const qreal pulse = 0.5 + 0.5 * qSin(m_phase * 0.9);
    QLinearGradient veil(bounds.topLeft(), bounds.bottomRight());
    if (m_tone == SceneryTone::Positive) {
        veil.setColorAt(0.0, QColor(255, 248, 236, int(8 + pulse * 10)));
        veil.setColorAt(1.0, QColor(220, 240, 255, int(6 + pulse * 8)));
        p.fillRect(bounds, veil);
        drawPositiveEffects(p, bounds);
    } else {
        veil.setColorAt(0.0, QColor(210, 190, 240, int(10 + pulse * 12)));
        veil.setColorAt(1.0, QColor(255, 230, 240, int(8 + pulse * 10)));
        p.fillRect(bounds, veil);
        drawNegativeEffects(p, bounds);
    }

    QLinearGradient vignette(bounds.topLeft(), bounds.bottomLeft());
    vignette.setColorAt(0.0, QColor(255, 255, 255, 0));
    vignette.setColorAt(0.82, QColor(255, 255, 255, 0));
    vignette.setColorAt(1.0, QColor(255, 255, 255, int(36 + pulse * 18)));
    p.fillRect(bounds, vignette);
}

void WeeklySceneryWidget::drawCaption(QPainter &p, const QRectF &bounds) const
{
    const QString caption = m_tone == SceneryTone::Positive
                                ? QStringLiteral("愿本周的心象，像花海日落一样温柔亮起")
                                : QStringLiteral("允许悲伤存在，就像允许雨水落下");

    const QRectF captionRect(bounds.left() + 16, bounds.bottom() - 42, bounds.width() - 32, 34);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 125));
    p.drawRoundedRect(captionRect, 16, 16);

    p.setPen(m_tone == SceneryTone::Positive ? QColor("#6d7f88") : QColor("#756880"));
    QFont captionFont = ReportArtUtils::scriptFont(11);
    p.setFont(captionFont);
    p.drawText(captionRect, Qt::AlignCenter, caption);
}


void WeeklySceneryWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF frame = rect().adjusted(0, 0, -1, -1);
    p.setPen(QPen(QColor(255, 255, 255, 180), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(frame, 22, 22);

    const QRectF bounds = innerBounds();
    QPainterPath clip;
    clip.addRoundedRect(bounds, 20, 20);
    p.setClipPath(clip);

    drawCoverImage(p, bounds);
    drawAnimatedLight(p, bounds);
    drawCaption(p, bounds);
}
