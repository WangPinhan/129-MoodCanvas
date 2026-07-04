#include "arttitlelabel.h"
#include "thememanager.h"
#include "reportartutils.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

ArtTitleLabel::ArtTitleLabel(QWidget *parent)
    : QLabel(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}
ArtTitleLabel::ArtTitleLabel(const QString &text, QWidget *parent)
    : ArtTitleLabel(parent)
{
    setText(text);
}

QSize ArtTitleLabel::sizeHint() const
{
    const QFont titleFont = ReportArtUtils::scriptFont(30);
    const QFontMetricsF metrics(titleFont);
    const int width = qCeil(metrics.horizontalAdvance(text())) + 4;
    const int height = qCeil(metrics.height()) + 6;
    return QSize(width, height);
}

QSize ArtTitleLabel::minimumSizeHint() const
{
    return sizeHint();
}

void ArtTitleLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    const QFont titleFont = ReportArtUtils::scriptFont(30);
    painter.setFont(titleFont);

    const QFontMetricsF metrics(titleFont);
    const QRectF textRect = QRectF(rect()).adjusted(0, 2, 0, 0);
    const QString content = text();
    const bool dark = ThemeManager::instance().isDarkMode();
    QLinearGradient gradient(textRect.topLeft(), textRect.topRight());
    if (dark) {
        gradient.setColorAt(0.0, QColor("#e8ecf1"));
        gradient.setColorAt(0.45, QColor("#f5f7fa"));
        gradient.setColorAt(1.0, QColor("#d0d7e0"));
    } else {
        gradient.setColorAt(0.0, QColor("#31404d"));
        gradient.setColorAt(0.45, QColor("#566878"));
        gradient.setColorAt(1.0, QColor("#31404d"));
    }
    QPainterPath shadowPath;
    shadowPath.addText(textRect.left() + 1.5, textRect.top() + metrics.ascent() + 1.5, titleFont, content);
    if (dark) {
        painter.fillPath(shadowPath, QColor(255, 255, 255, 60));
    } else {
        painter.fillPath(shadowPath, QColor(255, 255, 255, 90));
    }
    QPainterPath textPath;
    textPath.addText(textRect.left(), textRect.top() + metrics.ascent(), titleFont, content);
    painter.fillPath(textPath, gradient);
}