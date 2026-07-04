#ifndef ARTTITLELABEL_H
#define ARTTITLELABEL_H

#include <QLabel>

class ArtTitleLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ArtTitleLabel(QWidget *parent = nullptr);
    explicit ArtTitleLabel(const QString &text, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // ARTTITLELABEL_H
