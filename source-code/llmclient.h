#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include "llmconfig.h"
#include "moodentry.h"

#include <QString>

struct WeeklyReportNarrative
{
    QString insight;
    QString advice;
    bool fromLlm = false;
};

struct TreeholeEcho
{
    QString text;
    bool fromLlm = false;
};

/**
 * 调用 OpenAI 兼容 Chat Completions，将用户日记解析为 MoodEntry。
 */
class LlmClient
{
public:
    static MoodEntry analyze(const QString &text, const LlmConfig &config, QString *errorMessage = nullptr);
    static WeeklyReportNarrative generateWeeklyReport(const QString &weekContext,
                                                      const LlmConfig &config,
                                                      QString *errorMessage = nullptr);
    static TreeholeEcho generateTreeholeEcho(const QString &moodLine,
                                             const QString &emotion,
                                             const LlmConfig &config,
                                             QString *errorMessage = nullptr);

private:
    static QString chatCompletionsUrl(const LlmConfig &config);
    static QString extractJsonPayload(const QString &raw);
    static MoodEntry moodEntryFromJson(const QJsonObject &obj, const QString &rawText);
    static WeeklyReportNarrative narrativeFromJson(const QJsonObject &obj);
    static QString requestChatCompletion(const LlmConfig &config,
                                         const QString &systemPrompt,
                                         const QString &userPrompt,
                                         double temperature,
                                         QString *errorMessage);
};

#endif // LLMCLIENT_H
