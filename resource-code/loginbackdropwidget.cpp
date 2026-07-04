#include "loginbackdropwidget.h"

#include "thememanager.h"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QTimerEvent>
#include <QtMath>

LoginBackdropWidget::LoginBackdropWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_timerId = startTimer(48);
}

void LoginBackdropWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool dark = ThemeManager::instance().isDarkMode();
    const QRectF bounds = rect();

    QLinearGradient base(bounds.topLeft(), bounds.bottomRight());
    if (dark) {
        base.setColorAt(0.0, QColor("#1a1e24"));
        base.setColorAt(0.45, QColor("#1e2328"));
        base.setColorAt(1.0, QColor("#1f2327"));
    } else {
        base.setColorAt(0.0, QColor("#f8fbff"));
        base.setColorAt(0.45, QColor("#f2fff8"));
        base.setColorAt(1.0, QColor("#fff8f5"));
    }
    painter.fillRect(bounds, base);

    const auto blob = [&](qreal cx, qreal cy, qreal radius, const QColor &core, const QColor &edge, qreal alpha) {
        QRadialGradient gradient(QPointF(cx, cy), radius);
        QColor center = core;
        center.setAlphaF(alpha);
        QColor outer = edge;
        outer.setAlphaF(0.0);
        gradient.setColorAt(0.0, center);
        gradient.setColorAt(0.55, center);
        gradient.setColorAt(1.0, outer);

        QPainterPath path;
        path.addEllipse(QPointF(cx, cy), radius, radius * 0.88);
        painter.fillPath(path, gradient);
    };

    const qreal wave = qSin(m_phase);
    const qreal wave2 = qCos(m_phase * 0.82);

    if (dark) {
        blob(bounds.width() * (0.18 + wave * 0.02),
             bounds.height() * (0.22 + wave2 * 0.03),
             bounds.width() * 0.42,
             QColor("#2f6b58"), QColor("#1a1e24"), 0.42);
        blob(bounds.width() * (0.82 + wave2 * 0.02),
             bounds.height() * (0.18 + wave * 0.02),
             bounds.width() * 0.38,
             QColor("#4a5f96"), QColor("#1a1e24"), 0.34);
        blob(bounds.width() * (0.55 + wave * 0.03),
             bounds.height() * (0.78 + wave2 * 0.02),
             bounds.width() * 0.48,
             QColor("#7a5a42"), QColor("#1a1e24"), 0.28);
    } else {
        blob(bounds.width() * (0.16 + wave * 0.02),
             bounds.height() * (0.20 + wave2 * 0.03),
             bounds.width() * 0.44,
             QColor("#bdebd9"), QColor("#f8fbff"), 0.72);
        blob(bounds.width() * (0.84 + wave2 * 0.02),
             bounds.height() * (0.16 + wave * 0.02),
             bounds.width() * 0.40,
             QColor("#c8d8ff"), QColor("#f2fff8"), 0.66);
        blob(bounds.width() * (0.52 + wave * 0.03),
             bounds.height() * (0.82 + wave2 * 0.02),
             bounds.width() * 0.50,
             QColor("#ffd9bf"), QColor("#fff8f5"), 0.58);
    }

    QLinearGradient veil(bounds.topLeft(), bounds.bottomRight());
    if (dark) {
        veil.setColorAt(0.0, QColor(255, 255, 255, 8));
        veil.setColorAt(1.0, QColor(255, 255, 255, 0));
    } else {
        veil.setColorAt(0.0, QColor(255, 255, 255, 70));
        veil.setColorAt(1.0, QColor(255, 255, 255, 10));
    }
    painter.fillRect(bounds, veil);
}

void LoginBackdropWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_timerId) {
        QWidget::timerEvent(event);
        return;
    }

    m_phase += 0.018;
    update();
}
