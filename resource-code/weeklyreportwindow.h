#ifndef WEEKLYREPORTWINDOW_H
#define WEEKLYREPORTWINDOW_H

#include "moodentry.h"

#include <QColor>
#include <QDate>
#include <QDialog>
#include <QImage>
#include <QMap>
#include <QPair>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QWidget>

class QWheelEvent;
class DataStore;
class QLabel;
class QPainter;
class QScrollArea;
class WeeklySceneryWidget;

class WeeklyChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WeeklyChartWidget(QWidget *parent = nullptr);
    void setEntries(const QVector<MoodEntry> &entries, const QDate &weekDate);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QVector<MoodEntry> m_entries;
    QDate m_weekDate;

    qreal m_emotionScrollOffset = 0.0;
    qreal m_keywordScrollOffset = 0.0;
    QRectF m_emotionCardRect;
    QRectF m_keywordCardRect;

    QDate weekMonday() const;

    qreal emotionContentHeight() const;
    qreal keywordContentHeight(const QRectF &contentRect) const;
    qreal maxEmotionScroll(const QRectF &cardRect) const;
    qreal maxKeywordScroll(const QRectF &cardRect) const;

    void drawScrollIndicator(QPainter &p,
                             const QRectF &viewRect,
                             qreal contentHeight,
                             qreal scrollOffset) const;

    void drawCard(QPainter &p, const QRectF &rect, const QString &title,
                  const QColor &topTint, const QColor &bottomTint) const;
    void drawEmotionDistribution(QPainter &p, const QRectF &rect) const;
    void drawWeekMoodStrip(QPainter &p, const QRectF &rect) const;
    void drawKeywords(QPainter &p, const QRectF &rect) const;

};

class WeeklyReportWindow : public QDialog
{
    Q_OBJECT

public:
    explicit WeeklyReportWindow(DataStore *store, QWidget *parent = nullptr);

private:
    DataStore *m_store = nullptr;
    QVector<MoodEntry> m_entries;
    WeeklyChartWidget *m_chart = nullptr;
    QLabel *m_periodLabel = nullptr;
    QLabel *m_sourceLabel = nullptr;
    QLabel *m_insightLabel = nullptr;
    QLabel *m_adviceLabel = nullptr;
    QLabel *m_epithetLabel = nullptr;
    WeeklySceneryWidget *m_sceneryWidget = nullptr;
    QScrollArea *m_reportScrollArea = nullptr;
    QWidget *m_reportScrollContent = nullptr;
    QDate m_weekDate;

    void buildReport();
    void updateReportPresentation();
    QString buildWeekContextForLlm() const;
    QVector<QPair<QString, int>> topKeywords(int limit = 6) const;
    QVector<QPair<QString, int>> topTriggers(int limit = 6) const;
    QMap<QString, int> emotionCounts() const;
    QString dominantEmotion() const;
    QString trendSentence() const;
    QString formatAdviceText(const QString &raw) const;
    double averageValence(double fallback = 0.0) const;
    double averageArousal(double fallback = 0.0) const;
    QImage renderFullReportImage();

    void exportImage();
    void exportPdf();
};

#endif // WEEKLYREPORTWINDOW_H