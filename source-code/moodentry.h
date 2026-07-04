#ifndef MOODENTRY_H
#define MOODENTRY_H

#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * MoodEntry 表示一条心象日记记录。
 * colorPalette / emotionWeights 由文本自动分析生成，用于色彩心象图。
 */
class MoodEntry
{
public:
    QString id;
    QDate date;
    QDateTime createdAt;
    QString rawText;
    QString emoji;
    QString emotion;
    QString emotionSummary;
    double valence = 0.0;
    double arousal = 0.5;
    QStringList keywords;
    QStringList triggerTags;
    QMap<QString, double> emotionWeights;
    QVector<QColor> colorPalette;
    QString cbtHint;
    QString comfortText;
    QString analysisSource;
    QColor mainColor;

    QJsonObject toJson() const;
    static MoodEntry fromJson(const QJsonObject &obj);
};

#endif // MOODENTRY_H
