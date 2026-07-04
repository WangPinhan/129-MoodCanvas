#include "treeholeemotion.h"

#include "llmclient.h"
#include "moodentry.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QTimer>
#include <QtConcurrent>
#include <QtMath>

namespace {

QString detectEmotionLocal(const QString &text)
{
    const QString t = text;

    struct Rule {
        QStringList keywords;
        QString emotion;
    };

    const QVector<Rule> rules = {
        {{QStringLiteral("开心"), QStringLiteral("高兴"), QStringLiteral("快乐"), QStringLiteral("喜欢"),
          QStringLiteral("幸福"), QStringLiteral("棒"), QStringLiteral("好"), QStringLiteral("暖"), QStringLiteral("笑")},
         QStringLiteral("开心")},
        {{QStringLiteral("怒"), QStringLiteral("气"), QStringLiteral("烦"), QStringLiteral("讨厌"), QStringLiteral("恨"),
          QStringLiteral("火大"), QStringLiteral("滚")},
         QStringLiteral("愤怒")},
        {{QStringLiteral("难过"), QStringLiteral("伤心"), QStringLiteral("哭"), QStringLiteral("悲"), QStringLiteral("失去"),
          QStringLiteral("孤独"), QStringLiteral("空"), QStringLiteral("沉"), QStringLiteral("重")},
         QStringLiteral("难过")},
        {{QStringLiteral("焦虑"), QStringLiteral("紧张"), QStringLiteral("担心"), QStringLiteral("害怕"), QStringLiteral("怕"),
          QStringLiteral("慌"), QStringLiteral("压力")},
         QStringLiteral("焦虑")},
        {{QStringLiteral("累"), QStringLiteral("疲"), QStringLiteral("困"), QStringLiteral("乏"), QStringLiteral("睡")},
         QStringLiteral("疲惫")},
        {{QStringLiteral("平静"), QStringLiteral("安静"), QStringLiteral("慢慢"), QStringLiteral("风"), QStringLiteral("云"),
          QStringLiteral("还好"), QStringLiteral("一般")},
         QStringLiteral("平静")},
    };

    int bestScore = 0;
    QString bestEmotion = QStringLiteral("心象");
    for (const Rule &rule : rules) {
        int score = 0;
        for (const QString &kw : rule.keywords) {
            if (t.contains(kw)) {
                ++score;
            }
        }
        if (score > bestScore) {
            bestScore = score;
            bestEmotion = rule.emotion;
        }
    }
    return bestEmotion;
}

double localValence(const QString &emotion)
{
    if (emotion.contains(QStringLiteral("开心")) || emotion.contains(QStringLiteral("乐"))) {
        return 0.65;
    }
    if (emotion.contains(QStringLiteral("平静"))) {
        return 0.15;
    }
    if (emotion.contains(QStringLiteral("怒"))) {
        return -0.55;
    }
    if (emotion.contains(QStringLiteral("难过")) || emotion.contains(QStringLiteral("哀"))) {
        return -0.6;
    }
    if (emotion.contains(QStringLiteral("焦虑"))) {
        return -0.35;
    }
    if (emotion.contains(QStringLiteral("疲惫"))) {
        return -0.25;
    }
    return 0.0;
}

double localArousal(const QString &emotion)
{
    if (emotion.contains(QStringLiteral("怒")) || emotion.contains(QStringLiteral("焦虑"))) {
        return 0.78;
    }
    if (emotion.contains(QStringLiteral("开心"))) {
        return 0.62;
    }
    if (emotion.contains(QStringLiteral("疲惫")) || emotion.contains(QStringLiteral("平静"))) {
        return 0.28;
    }
    if (emotion.contains(QStringLiteral("难过"))) {
        return 0.42;
    }
    return 0.45;
}

} // namespace

TreeholeEmotionAnalyzer::TreeholeEmotionAnalyzer(QObject *parent)
    : QObject(parent)
{
}

QString TreeholeEmotionAnalyzer::cacheKey(const QString &text)
{
    return text.trimmed();
}

void TreeholeEmotionAnalyzer::setLlmConfig(const LlmConfig &config)
{
    m_config = config;
}

SentenceEmotion TreeholeEmotionAnalyzer::cached(const QString &text) const
{
    return m_cache.value(cacheKey(text));
}

bool TreeholeEmotionAnalyzer::hasPending(const QString &text) const
{
    return m_pending.value(cacheKey(text), false);
}

QColor TreeholeEmotionAnalyzer::colorFromEmotionName(const QString &emotion)
{
    if (emotion.contains(QStringLiteral("开心")) || emotion.contains(QStringLiteral("乐"))) {
        return QColor(QStringLiteral("#FFB703"));
    }
    if (emotion.contains(QStringLiteral("怒"))) {
        return QColor(QStringLiteral("#E63946"));
    }
    if (emotion.contains(QStringLiteral("难过")) || emotion.contains(QStringLiteral("哀"))) {
        return QColor(QStringLiteral("#5C7AEA"));
    }
    if (emotion.contains(QStringLiteral("焦虑"))) {
        return QColor(QStringLiteral("#9B5DE5"));
    }
    if (emotion.contains(QStringLiteral("疲惫"))) {
        return QColor(QStringLiteral("#8D99AE"));
    }
    if (emotion.contains(QStringLiteral("平静"))) {
        return QColor(QStringLiteral("#52B788"));
    }
    return QColor(QStringLiteral("#7C8BA1"));
}

QColor TreeholeEmotionAnalyzer::colorFromValenceArousal(double valence, double arousal)
{
    const double hue = qBound(0.0, 18.0 + (valence + 1.0) * 0.5 * 300.0, 360.0);
    const double sat = qBound(0.48, 0.58 + arousal * 0.28, 0.92);
    const double val = qBound(0.34, 0.40 + valence * 0.06, 0.52);
    QColor color;
    color.setHsvF(hue / 360.0, sat, val);
    return color;
}

QString TreeholeEmotionAnalyzer::rgba(const QColor &color, int alpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha);
}

QColor TreeholeEmotionAnalyzer::readableTextColor(double valence, double arousal, const QColor &accent)
{
    QColor color = colorFromValenceArousal(valence, arousal);
    if (accent.isValid()) {
        float ah = 0.0f;
        float as = 0.0f;
        float av = 0.0f;
        accent.getHsvF(&ah, &as, &av);
        if (as > 0.08f) {
            float ch = 0.0f;
            float cs = 0.0f;
            float cv = 0.0f;
            color.getHsvF(&ch, &cs, &cv);
            color.setHsvF(ah, qMax(cs, 0.58f), qBound(0.34f, cv, 0.52f));
        }
    }
    return color;
}

SentenceEmotion TreeholeEmotionAnalyzer::fromMoodEntry(const MoodEntry &entry)
{
    SentenceEmotion result;
    result.emotion = entry.emotion.isEmpty() ? QStringLiteral("心象") : entry.emotion;
    result.summary = entry.emotionSummary.isEmpty() ? result.emotion : entry.emotionSummary;
    result.fromLlm = entry.analysisSource == QStringLiteral("llm");
    result.valence = entry.valence;
    result.arousal = entry.arousal;

    QColor accent;
    if (!entry.colorPalette.isEmpty() && entry.colorPalette.first().isValid()) {
        accent = entry.colorPalette.first();
    } else if (entry.mainColor.isValid()) {
        accent = entry.mainColor;
    }

    result.color = readableTextColor(entry.valence, entry.arousal, accent);
    result.tintColor = accent.isValid() ? accent : result.color;
    return result;
}

SentenceEmotion TreeholeEmotionAnalyzer::analyzeLocal(const QString &text)
{
    const QString trimmed = text.trimmed();
    SentenceEmotion result;
    result.emotion = detectEmotionLocal(trimmed);
    result.fromLlm = false;
    result.summary = result.emotion;
    result.valence = localValence(result.emotion);
    result.arousal = localArousal(result.emotion);
    result.color = readableTextColor(result.valence, result.arousal);
    result.tintColor = colorFromEmotionName(result.emotion);
    return result;
}

void TreeholeEmotionAnalyzer::requestAnalysis(const QString &text)
{
    const QString key = cacheKey(text);
    if (key.isEmpty() || m_cache.contains(key) || m_pending.value(key, false)) {
        return;
    }

    m_cache.insert(key, analyzeLocal(text));
    emit sentenceReady(text, m_cache.value(key));

    m_pending.insert(key, true);
    enqueue(key);
}

void TreeholeEmotionAnalyzer::enqueue(const QString &text)
{
    if (!m_queue.contains(text)) {
        m_queue.append(text);
    }
    if (!m_busy) {
        processNext();
    }
}

void TreeholeEmotionAnalyzer::processNext()
{
    if (m_queue.isEmpty()) {
        m_busy = false;
        return;
    }

    m_busy = true;
    const QString key = m_queue.takeFirst();
    const QString text = key;
    const LlmConfig config = m_config;
    TreeholeEmotionAnalyzer *self = this;

    QtConcurrent::run([self, text, key, config]() {
        SentenceEmotion result = analyzeLocal(text);

        if (config.isReady()) {
            QString error;
            const MoodEntry entry = LlmClient::analyze(text, config, &error);
            if (!entry.emotion.isEmpty()) {
                result = fromMoodEntry(entry);
            }
        }

        QTimer::singleShot(0, self, [self, text, result]() {
            self->finishAnalysis(text, result);
        });
    });
}

void TreeholeEmotionAnalyzer::finishAnalysis(const QString &text, const SentenceEmotion &result)
{
    const QString key = cacheKey(text);
    m_cache.insert(key, result);
    m_pending.remove(key);
    emit sentenceReady(text, result);
    processNext();
}

namespace TreeholeEmotionHtml {

QString rgbaColor(const QColor &color, int alpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha);
}

QString formatCloudTime(const QString &isoString)
{
    const QString iso = isoString.trimmed();
    QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
    if (!dt.isValid()) {
        return iso.left(11);
    }

    if (iso.endsWith(QStringLiteral("Z"), Qt::CaseInsensitive) || iso.contains(QStringLiteral("+00:00"))) {
        dt.setTimeSpec(Qt::UTC);
    }

    if (dt.timeSpec() == Qt::UTC || dt.timeSpec() == Qt::OffsetFromUTC) {
        dt = dt.toLocalTime();
    }

    return dt.toString(QStringLiteral("MM-dd hh:mm"));
}

QString escapeHtml(const QString &text)
{
    QString out = text;
    out.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    out.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    out.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    return out;
}

QString coloredLine(const QString &prefix, const QString &sentence, const SentenceEmotion &emotion)
{
    const QString textColor = emotion.color.name(QColor::HexRgb);
    const QString bgColor = rgbaColor(emotion.tintColor.isValid() ? emotion.tintColor : emotion.color, 36);
    const QString borderColor = textColor;
    const QString emotionTag = escapeHtml(emotion.emotion);
    const QString body = escapeHtml(sentence);
    const QString prefixHtml = prefix.isEmpty() ? QString() : QStringLiteral("%1").arg(escapeHtml(prefix));
    const QString meta = emotion.fromLlm && !emotion.summary.isEmpty() && emotion.summary != emotion.emotion
                             ? escapeHtml(emotion.summary)
                             : emotionTag;

    QString inner;
    if (!prefixHtml.isEmpty()) {
        inner += QStringLiteral("<span style='color:#64748b; font-size:12px;'>%1</span> ").arg(prefixHtml);
    }
    inner += QStringLiteral("<span style='color:%1; font-weight:600; font-size:13px;'>%2</span>")
                 .arg(textColor, body);
    inner += QStringLiteral("<span style='color:%1; font-size:11px;'> · %2</span>").arg(textColor, meta);

    return QStringLiteral(
               "<div style='margin-top:6px; padding:8px 10px; border-left:4px solid %1;"
               " background:%2; border-radius:8px; line-height:1.55;'>%3</div>")
        .arg(borderColor, bgColor, inner);
}

QString buildPostCardHtml(const QJsonObject &post, const TreeholeEmotionAnalyzer &analyzer)
{
    const QString time = formatCloudTime(post.value(QStringLiteral("createdAt")).toString());
    const QString nickname = post.value(QStringLiteral("nickname")).toString(QStringLiteral("匿名访客"));
    const QString moodLine = post.value(QStringLiteral("moodLine")).toString();

    SentenceEmotion moodEmotion = analyzer.cached(moodLine);
    if (moodEmotion.emotion.isEmpty()) {
        moodEmotion = TreeholeEmotionAnalyzer::analyzeLocal(moodLine);
    }

    QString html = QStringLiteral(
                       "<div style='line-height:1.5;'>"
                       "<div style='color:#64748b; font-size:12px; margin-bottom:6px;'>%1 · %2</div>")
                       .arg(escapeHtml(nickname), escapeHtml(time));

    html += coloredLine(QString(), moodLine, moodEmotion);

    const QJsonArray replies = post.value(QStringLiteral("replies")).toArray();
    for (const QJsonValue &value : replies) {
        const QJsonObject reply = value.toObject();
        const QString replyNick = reply.value(QStringLiteral("nickname")).toString(QStringLiteral("访客"));
        const QString replyText = reply.value(QStringLiteral("text")).toString();
        if (replyText.isEmpty()) {
            continue;
        }

        SentenceEmotion replyEmotion = analyzer.cached(replyText);
        if (replyEmotion.emotion.isEmpty()) {
            replyEmotion = TreeholeEmotionAnalyzer::analyzeLocal(replyText);
        }

        html += coloredLine(QStringLiteral("💬 %1：").arg(replyNick), replyText, replyEmotion);
    }

    html += QStringLiteral("</div>");
    return html;
}

} // namespace TreeholeEmotionHtml
