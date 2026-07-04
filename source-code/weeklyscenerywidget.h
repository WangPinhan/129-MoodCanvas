#ifndef WEEKLYSCENERYWIDGET_H
#define WEEKLYSCENERYWIDGET_H

#include <QPixmap>
#include <QTimer>
#include <QWidget>
class WeeklySceneryWidget : public QWidget
{
    Q_OBJECT

public:
    enum class SceneryTone {
        Positive,
        Negative
    };

    explicit WeeklySceneryWidget(QWidget *parent = nullptr);

    void setTone(SceneryTone tone);
    void setWeeklyValence(double avgValence);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SceneryTone m_tone = SceneryTone::Positive;
    QPixmap m_sourcePixmap;
    QTimer m_timer;
    qreal m_phase = 0.0;

    void reloadPixmap();
    static QPixmap loadSceneryAsset(SceneryTone tone);
    QRectF innerBounds() const;
    void drawCoverImage(QPainter &p, const QRectF &bounds) const;
    void drawAnimatedLight(QPainter &p, const QRectF &bounds) const;
    void drawPositiveEffects(QPainter &p, const QRectF &bounds) const;
    void drawNegativeEffects(QPainter &p, const QRectF &bounds) const;
    void drawCaption(QPainter &p, const QRectF &bounds) const;
};

#endif // WEEKLYSCENERYWIDGET_H