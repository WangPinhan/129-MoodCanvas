#ifndef TREEHOLEEMOTION_H
#define TREEHOLEEMOTION_H

#include "llmconfig.h"

#include <QColor>
#include <QObject>
#include <QHash>
#include <QString>

struct SentenceEmotion
{
    QString emotion = QStringLiteral("心象");
    QColor color = QColor(QStringLiteral("#475569"));
    QColor tintColor = QColor(QStringLiteral("#F1F5F9"));
    QString summary;
    bool fromLlm = false;
    double valence = 0.0;
    double arousal = 0.45;
};

class TreeholeEmotionAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit TreeholeEmotionAnalyzer(QObject *parent = nullptr);

    SentenceEmotion cached(const QString &text) const;
    bool hasPending(const QString &text) const;

    void setLlmConfig(const LlmConfig &config);
    void requestAnalysis(const QString &text);

    static SentenceEmotion analyzeLocal(const QString &text);

signals:
    void sentenceReady(const QString &text, const SentenceEmotion &emotion);

private:
    LlmConfig m_config;
    QHash<QString, SentenceEmotion> m_cache;
    QHash<QString, bool> m_pending;
    QStringList m_queue;
    bool m_busy = false;

    static QString cacheKey(const QString &text);
    static SentenceEmotion fromMoodEntry(const class MoodEntry &entry);
    static QColor colorFromEmotionName(const QString &emotion);
    static QColor colorFromValenceArousal(double valence, double arousal);
    static QColor readableTextColor(double valence, double arousal, const QColor &accent = QColor());
    static QString rgba(const QColor &color, int alpha);

    void enqueue(const QString &text);
    void processNext();

private slots:
    void finishAnalysis(const QString &text, const SentenceEmotion &result);
};

namespace TreeholeEmotionHtml {

QString escapeHtml(const QString &text);
QString coloredLine(const QString &prefix, const QString &sentence, const SentenceEmotion &emotion);
QString buildPostCardHtml(const QJsonObject &post,
                            const TreeholeEmotionAnalyzer &analyzer);

} // namespace TreeholeEmotionHtml

#endif // TREEHOLEEMOTION_H
