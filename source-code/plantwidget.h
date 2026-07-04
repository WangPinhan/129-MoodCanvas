#ifndef PLANTWIDGET_H
#define PLANTWIDGET_H

#include <QRectF>
#include <QString>
#include <QWidget>

class QPainter;
class QPaintEvent;

/**
 * PlantWidget 是首页的小植物成长区。
 * 当天没有记录时为休眠；当天保存心情后，按“当天最后一条记录”的情绪改变状态。
 */
class PlantWidget : public QWidget
{
public:
    enum class PlantState {
        Dormant,
        Bloom,
        Leaves,
        Curled,
        Rain
    };

    explicit PlantWidget(QWidget *parent = nullptr);

    void setPlantState(PlantState state);
    PlantState plantState() const;
    QString stateTitle() const;
    QString stateHint() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    PlantState m_state = PlantState::Dormant;

    void drawPot(QPainter &p, const QRectF &r);
    void drawStem(QPainter &p, const QRectF &r);
    void drawLeaves(QPainter &p, const QRectF &r, bool curled);
    void drawFlowers(QPainter &p, const QRectF &r);
    void drawRain(QPainter &p, const QRectF &r);
    void drawDormantBubble(QPainter &p, const QRectF &r);
};

#endif // PLANTWIDGET_H
