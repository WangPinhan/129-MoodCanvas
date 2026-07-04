#ifndef MOODCANVASWIDGET_H
#define MOODCANVASWIDGET_H

#include "moodentry.h"

#include <QColor>
#include <QTimer>
#include <QVector>
#include <QWidget>

/**
 * 心象画布：用多层渐变与色带表达喜怒哀乐等情绪，并与用户文本关联。
 */
class MoodCanvasWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MoodCanvasWidget(QWidget *parent = nullptr);

    void setEntry(const MoodEntry &entry);
    QPixmap grabCanvas();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct VisualTheme {
        bool mist = false;
        bool starlight = false;
        bool ember = false;
        bool breeze = false;
    };

    MoodEntry m_entry;
    VisualTheme m_theme;
    QTimer m_timer;
    qreal m_phase = 0.0;
    quint32 m_textSeed = 0;

    void rebuildVisualContext();
    quint32 hashText(const QString &text) const;
    qreal seeded(qreal min, qreal max, int salt) const;

    QVector<QColor> emotionPalette() const;
    QColor paletteColor(int index) const;
    QString excerptText() const;

    void drawBackground(QPainter &p, const QRectF &canvas);
    void drawColorRibbons(QPainter &p, const QRectF &canvas);
    void drawAuroraBands(QPainter &p, const QRectF &canvas);
    void drawKeywordConstellation(QPainter &p, const QRectF &canvas);
    void drawMistLayer(QPainter &p, const QRectF &canvas);
    void drawStarlight(QPainter &p, const QRectF &canvas);
    void drawTextCard(QPainter &p, const QRectF &canvas);
    void drawHeader(QPainter &p, const QRectF &canvas);
};

#endif // MOODCANVASWIDGET_H
