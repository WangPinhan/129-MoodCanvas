#include "llmclient.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>

namespace {

QString systemPrompt()
{
    return QStringLiteral(
        "你是「心象日记 MoodCanvas」的情绪分析助手。用户会输入一段中文日记。"
        "你必须真正理解文字的具体情境、人物、事件和情绪转折，而不是套用空话。"
        "只输出一个 JSON 对象，不要 markdown，不要解释。字段如下：\n"
        "{\n"
        "  \"emotion\": \"主导情绪，只能是：开心、愤怒、难过、平静、焦虑、疲惫 之一\",\n"
        "  \"emotion_weights\": {\"喜\":0.0,\"怒\":0.0,\"哀\":0.0,\"乐\":0.0,\"焦虑\":0.0,\"疲惫\":0.0},\n"
        "  \"emotion_summary\": \"例如：焦虑 42% · 喜 28% · 疲惫 18%\",\n"
        "  \"valence\": -1到1之间的小数,\n"
        "  \"arousal\": 0到1之间的小数,\n"
        "  \"keywords\": [\"从原文提取的关键词，最多8个\"],\n"
        "  \"comfort_text\": \"2到4句中文，必须结合用户日记里的具体细节安慰和鼓励，不要用泛泛的鸡汤\",\n"
        "  \"cbt_hint\": \"一句温柔、可操作的小建议\",\n"
        "  \"colors\": [\"#RRGGBB\", \"至少4个十六进制颜色，体现情绪配比\"]\n"
        "}\n"
        "emotion_weights 六项之和应接近 1。");
}

QString weeklyReportSystemPrompt()
{
    return QStringLiteral(
        "你是「心象日记 MoodCanvas」的周报写作助手。用户会提供本周情绪记录的统计数据与若干日记摘要。"
        "请基于这些真实数据，写出温柔、具体、不过度诊断的中文周报文字。"
        "只输出一个 JSON 对象，不要 markdown，不要解释。字段如下：\n"
        "{\n"
        "  \"insight\": \"本周小结，3到5句。要提到主导情绪、变化趋势、关键词所反映的主题，语气像一位温和的朋友。\",\n"
        "  \"advice\": \"下周建议，2到3条。每条单独成句，具体可操作，不要空话，不要医疗诊断。\"\n"
        "}");
}

QString treeholeEchoSystemPrompt()
{
    return QStringLiteral(
        "你是「心象树洞」的回响精灵。有人刚在本机树洞投递一句模糊心情，你看不见原始日记全文。"
        "请用 1 到 2 句温柔、具体、不说教的中文回应，像深夜树洞里的一声轻应。"
        "要呼应对方写下的模糊心情和情绪色彩，避免空话和医疗诊断。"
        "不要称呼对方，不要列表，不要 emoji，不要 markdown。"
        "只输出一个 JSON 对象：{\"echo\":\"回响正文\"}");
}

QString offlineTreeholeEcho(const QString &emotion, const QString &moodLine)
{
    Q_UNUSED(moodLine)

    if (emotion.contains(QStringLiteral("开心")) || emotion.contains(QStringLiteral("喜"))
        || emotion.contains(QStringLiteral("乐"))) {
        return QStringLiteral("这一束亮色被树洞收下了，愿它在你心里多停一会儿。");
    }
    if (emotion.contains(QStringLiteral("焦虑")) || emotion.contains(QStringLiteral("紧张"))) {
        return QStringLiteral("树洞听见了那些未说尽的担心，先允许它们存在，你已经很努力了。");
    }
    if (emotion.contains(QStringLiteral("难过")) || emotion.contains(QStringLiteral("哀"))
        || emotion.contains(QStringLiteral("悲"))) {
        return QStringLiteral("那些下沉的心情被接住了，允许它们存在，风会慢慢把重量吹轻一点。");
    }
    if (emotion.contains(QStringLiteral("愤怒")) || emotion.contains(QStringLiteral("怒"))) {
        return QStringLiteral("那些灼热的感受被接住了，不必急着立刻原谅或立刻放下。");
    }
    if (emotion.contains(QStringLiteral("疲惫"))) {
        return QStringLiteral("树洞为你留了一处可以靠一靠的角落，今晚先对自己轻一点。");
    }
    return QStringLiteral("树洞收到了，你的心情会被温柔地留在这一页里。");
}

TreeholeEcho echoFromJson(const QJsonObject &obj)
{
    TreeholeEcho echo;
    echo.text = obj.value(QStringLiteral("echo")).toString().trimmed();
    echo.fromLlm = !echo.text.isEmpty();
    return echo;
}

} // namespace

QString LlmClient::chatCompletionsUrl(const LlmConfig &config)
{
    QString url = config.baseUrl.trimmed();
    while (url.endsWith(QLatin1Char('/'))) {
        url.chop(1);
    }

    if (url.contains(QStringLiteral("chat/completions"), Qt::CaseInsensitive)) {
        return url;
    }

    if (!url.endsWith(QStringLiteral("/v1"), Qt::CaseInsensitive)) {
        url += QStringLiteral("/v1");
    }
    url += QStringLiteral("/chat/completions");
    return url;
}

QString LlmClient::extractJsonPayload(const QString &raw)
{
    const int start = raw.indexOf(QLatin1Char('{'));
    const int end = raw.lastIndexOf(QLatin1Char('}'));
    if (start >= 0 && end > start) {
        return raw.mid(start, end - start + 1);
    }
    return raw.trimmed();
}

MoodEntry LlmClient::moodEntryFromJson(const QJsonObject &obj, const QString &rawText)
{
    MoodEntry entry;
    entry.rawText = rawText;
    entry.emoji = QStringLiteral("大模型分析");
    entry.analysisSource = QStringLiteral("llm");
    entry.emotion = obj.value(QStringLiteral("emotion")).toString().trimmed();
    entry.emotionSummary = obj.value(QStringLiteral("emotion_summary")).toString().trimmed();
    entry.valence = obj.value(QStringLiteral("valence")).toDouble(0.0);
    entry.arousal = obj.value(QStringLiteral("arousal")).toDouble(0.5);
    entry.comfortText = obj.value(QStringLiteral("comfort_text")).toString().trimmed();
    entry.cbtHint = obj.value(QStringLiteral("cbt_hint")).toString().trimmed();

    const QJsonObject weightsObj = obj.value(QStringLiteral("emotion_weights")).toObject();
    for (auto it = weightsObj.constBegin(); it != weightsObj.constEnd(); ++it) {
        entry.emotionWeights[it.key()] = it.value().toDouble(0.0);
    }

    for (const QJsonValue &v : obj.value(QStringLiteral("keywords")).toArray()) {
        const QString kw = v.toString().trimmed();
        if (!kw.isEmpty()) {
            entry.keywords << kw;
        }
    }

    for (const QJsonValue &v : obj.value(QStringLiteral("colors")).toArray()) {
        QColor c(v.toString().trimmed());
        if (c.isValid()) {
            entry.colorPalette.append(c);
        }
    }

    entry.valence = qBound(-1.0, entry.valence, 1.0);
    entry.arousal = qBound(0.0, entry.arousal, 1.0);

    if (!entry.colorPalette.isEmpty()) {
        entry.mainColor = entry.colorPalette.first();
    }

    return entry;
}

WeeklyReportNarrative LlmClient::narrativeFromJson(const QJsonObject &obj)
{
    WeeklyReportNarrative narrative;
    narrative.insight = obj.value(QStringLiteral("insight")).toString().trimmed();
    narrative.advice = obj.value(QStringLiteral("advice")).toString().trimmed();
    narrative.fromLlm = !narrative.insight.isEmpty() && !narrative.advice.isEmpty();
    return narrative;
}

QString LlmClient::requestChatCompletion(const LlmConfig &config,
                                         const QString &systemPromptText,
                                         const QString &userPrompt,
                                         double temperature,
                                         QString *errorMessage)
{
    if (!config.isReady()) {
        if (errorMessage) {
            if (!config.hasApiKey()) {
                *errorMessage = QStringLiteral("未配置 API Key。");
            } else if (!config.enabled) {
                *errorMessage = QStringLiteral("大模型未启用。");
            } else {
                *errorMessage = QStringLiteral("大模型配置不完整。");
            }
        }
        return {};
    }

    if (!QSslSocket::supportsSsl()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "HTTPS 不可用（缺少 OpenSSL）。请确保 Qt 的 TLS 插件在 PATH/QT_PLUGIN_PATH 中。");
        }
        return {};
    }

    QNetworkAccessManager manager;

    QNetworkRequest request(QUrl(chatCompletionsUrl(config)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + config.apiKey.trimmed().toUtf8());
    request.setTransferTimeout(config.timeoutMs);

    QJsonObject body;
    body.insert(QStringLiteral("model"), config.model.trimmed());
    body.insert(QStringLiteral("temperature"), temperature);

    QJsonArray messages;
    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("system")},
        {QStringLiteral("content"), systemPromptText},
    });
    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("user")},
        {QStringLiteral("content"), userPrompt},
    });
    body.insert(QStringLiteral("messages"), messages);

    QEventLoop loop;
    QNetworkReply *reply = manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(config.timeoutMs);
    loop.exec();

    if (!reply->isFinished() || reply->error() != QNetworkReply::NoError) {
        const QString err = reply->errorString();
        const QByteArray payload = reply->readAll();
        reply->deleteLater();
        if (errorMessage) {
            QString detail = err;
            if (!payload.isEmpty()) {
                const QJsonObject root = QJsonDocument::fromJson(payload).object();
                const QJsonObject apiErr = root.value(QStringLiteral("error")).toObject();
                const QString apiMsg = apiErr.value(QStringLiteral("message")).toString();
                if (!apiMsg.isEmpty()) {
                    detail = apiMsg;
                } else {
                    detail += QStringLiteral(" | ") + QString::fromUtf8(payload.left(160));
                }
            }
            *errorMessage = QStringLiteral("大模型请求失败：") + detail;
        }
        return {};
    }

    const QByteArray payload = reply->readAll();
    reply->deleteLater();

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    const QJsonObject root = doc.object();

    if (root.contains(QStringLiteral("error"))) {
        const QJsonObject errObj = root.value(QStringLiteral("error")).toObject();
        if (errorMessage) {
            *errorMessage = errObj.value(QStringLiteral("message")).toString(QStringLiteral("大模型返回错误"));
        }
        return {};
    }

    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("大模型未返回有效内容。");
        }
        return {};
    }

    const QJsonObject message = choices.first().toObject().value(QStringLiteral("message")).toObject();
    return message.value(QStringLiteral("content")).toString();
}

WeeklyReportNarrative LlmClient::generateWeeklyReport(const QString &weekContext,
                                                      const LlmConfig &config,
                                                      QString *errorMessage)
{
    const QString content = requestChatCompletion(config,
                                                  weeklyReportSystemPrompt(),
                                                  weekContext,
                                                  0.72,
                                                  errorMessage);
    if (content.isEmpty()) {
        return {};
    }

    const QString jsonText = extractJsonPayload(content);
    const QJsonDocument resultDoc = QJsonDocument::fromJson(jsonText.toUtf8());
    if (!resultDoc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法解析大模型 JSON：") + content.left(120);
        }
        return {};
    }

    WeeklyReportNarrative narrative = narrativeFromJson(resultDoc.object());
    if (!narrative.fromLlm) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("大模型返回字段不完整。");
        }
        return {};
    }

    return narrative;
}

TreeholeEcho LlmClient::generateTreeholeEcho(const QString &moodLine,
                                             const QString &emotion,
                                             const LlmConfig &config,
                                             QString *errorMessage)
{
    const QString userPrompt = QStringLiteral(
                                   "模糊心情：%1\n"
                                   "主导情绪：%2\n"
                                   "请给出树洞回响。")
                                   .arg(moodLine.trimmed(),
                                        emotion.trimmed().isEmpty() ? QStringLiteral("心象") : emotion.trimmed());

    const QString content = requestChatCompletion(config,
                                                  treeholeEchoSystemPrompt(),
                                                  userPrompt,
                                                  0.78,
                                                  errorMessage);
    if (content.isEmpty()) {
        TreeholeEcho fallback;
        fallback.text = offlineTreeholeEcho(emotion, moodLine);
        fallback.fromLlm = false;
        return fallback;
    }

    const QString jsonText = extractJsonPayload(content);
    const QJsonDocument resultDoc = QJsonDocument::fromJson(jsonText.toUtf8());
    if (resultDoc.isObject()) {
        TreeholeEcho echo = echoFromJson(resultDoc.object());
        if (!echo.text.isEmpty()) {
            return echo;
        }
    }

    QString plain = content.trimmed();
    plain.remove(QRegularExpression(QStringLiteral("^[\"'“”]+|[\"'“”]+$")));
    if (!plain.isEmpty()) {
        TreeholeEcho echo;
        echo.text = plain.left(120);
        echo.fromLlm = true;
        return echo;
    }

    TreeholeEcho fallback;
    fallback.text = offlineTreeholeEcho(emotion, moodLine);
    fallback.fromLlm = false;
    if (errorMessage && !errorMessage->isEmpty()) {
        // 保留 API 错误信息供调试，但仍返回离线回响
    }
    return fallback;
}

MoodEntry LlmClient::analyze(const QString &text, const LlmConfig &config, QString *errorMessage)
{
    const QString content = requestChatCompletion(config, systemPrompt(), text, 0.65, errorMessage);
    if (content.isEmpty()) {
        return {};
    }

    const QString jsonText = extractJsonPayload(content);
    const QJsonDocument resultDoc = QJsonDocument::fromJson(jsonText.toUtf8());
    if (!resultDoc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法解析大模型 JSON：") + content.left(120);
        }
        return {};
    }

    MoodEntry entry = moodEntryFromJson(resultDoc.object(), text);
    if (entry.emotion.isEmpty() || entry.comfortText.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("大模型返回字段不完整。");
        }
        return {};
    }

    return entry;
}
