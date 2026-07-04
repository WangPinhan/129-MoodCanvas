#ifndef DATASTORE_H
#define DATASTORE_H

#include "llmconfig.h"
#include "moodentry.h"
#include "cloudconfig.h"

#include <QDate>
#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

/**
 * DataStore 负责本地数据读写：心象日记、匿名树洞帖子和大模型配置。
 */
class DataStore : public QObject
{
    Q_OBJECT

public:
    explicit DataStore(QObject *parent = nullptr);

    QVector<MoodEntry> loadEntries() const;
    bool saveEntry(const MoodEntry &entry);
    bool deleteEntry(const MoodEntry &entry);
    QVector<MoodEntry> entriesForDate(const QDate &date) const;
    QVector<MoodEntry> entriesForMonth(int year, int month) const;
    QVector<MoodEntry> entriesForWeek(const QDate &anyDayInWeek) const;

    QJsonArray loadAnonymousPosts() const;
    bool saveAnonymousPost(const QJsonObject &post);

    QString dataFilePath() const;
    QString settingsFilePath() const;

    LlmConfig loadLlmConfig() const;
    /** 合并环境变量中的 API Key，有 Key 则默认启用大模型。 */
    LlmConfig resolveLlmConfig() const;
    bool saveLlmConfig(const LlmConfig &config);

    CloudConfig loadCloudConfig() const;
    bool saveCloudConfig(const CloudConfig &config);

private:
    QString anonymousPostsPath() const;

    QString m_dataFilePath;
    QString m_settingsFilePath;

    bool writeEntries(const QVector<MoodEntry> &entries) const;

    QJsonObject loadSettings() const;
    bool writeSettings(const QJsonObject &obj) const;
};

#endif // DATASTORE_H
