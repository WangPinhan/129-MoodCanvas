#ifndef LOGINBACKDROPWIDGET_H
#define LOGINBACKDROPWIDGET_H

#include <QWidget>

class LoginBackdropWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginBackdropWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    qreal m_phase = 0.0;
    int m_timerId = 0;
};

#endif // LOGINBACKDROPWIDGET_H
