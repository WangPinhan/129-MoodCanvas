#include "datastore.h"

#include "cloudconfig.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

DataStore::DataStore(QObject *parent)
    : QObject(parent)
{
    const QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDir);

    m_dataFilePath = appDir + "/mood_entries.json";
    m_settingsFilePath = appDir + "/mood_settings.json";
}

QString DataStore::dataFilePath() const
{
    return m_dataFilePath;
}

QString DataStore::settingsFilePath() const
{
    return m_settingsFilePath;
}

QVector<MoodEntry> DataStore::loadEntries() const
{
    QVector<MoodEntry> entries;

    QFile file(m_dataFilePath);
    if (!file.exists()) {
        return entries;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return entries;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return entries;
    }

    for (const QJsonValue &value : doc.array()) {
        if (value.isObject()) {
            entries.append(MoodEntry::fromJson(value.toObject()));
        }
    }

    std::sort(entries.begin(), entries.end(), [](const MoodEntry &a, const MoodEntry &b) {
        return a.createdAt < b.createdAt;
    });

    return entries;
}

bool DataStore::saveEntry(const MoodEntry &entry)
{
    QVector<MoodEntry> entries = loadEntries();

    // 新需求：同一天允许保存多条记录，因此不再按日期覆盖旧记录。
    entries.append(entry);

    return writeEntries(entries);
}

bool DataStore::deleteEntry(const MoodEntry &entry)
{
    QVector<MoodEntry> entries = loadEntries();
    bool removed = false;

    for (int i = entries.size() - 1; i >= 0; --i) {
        const MoodEntry &candidate = entries.at(i);
        const bool matched = !entry.id.isEmpty()
                                 ? candidate.id == entry.id
                                 : candidate.date == entry.date && candidate.createdAt == entry.createdAt;

        if (matched) {
            entries.removeAt(i);
            removed = true;
            break;
        }
    }

    if (!removed) {
        return false;
    }

    return writeEntries(entries);
}

QVector<MoodEntry> DataStore::entriesForDate(const QDate &date) const
{
    QVector<MoodEntry> result;
    for (const MoodEntry &entry : loadEntries()) {
        if (entry.date == date) {
            result.append(entry);
        }
    }
    return result;
}

QVector<MoodEntry> DataStore::entriesForMonth(int year, int month) const
{
    QVector<MoodEntry> result;
    for (const MoodEntry &entry : loadEntries()) {
        if (entry.date.year() == year && entry.date.month() == month) {
            result.append(entry);
        }
    }
    return result;
}

QVector<MoodEntry> DataStore::entriesForWeek(const QDate &anyDayInWeek) const
{
    QVector<MoodEntry> result;

    const int dayOfWeek = anyDayInWeek.dayOfWeek();
    const QDate monday = anyDayInWeek.addDays(1 - dayOfWeek);
    const QDate sunday = monday.addDays(6);

    for (const MoodEntry &entry : loadEntries()) {
        if (entry.date >= monday && entry.date <= sunday) {
            result.append(entry);
        }
    }

    return result;
}

QString DataStore::anonymousPostsPath() const
{
    return QFileInfo(m_settingsFilePath).absolutePath() + QStringLiteral("/anonymous_treehole_posts.json");
}

QJsonArray DataStore::loadAnonymousPosts() const
{
    QFile file(anonymousPostsPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QJsonArray();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isArray() ? doc.array() : QJsonArray();
}

bool DataStore::saveAnonymousPost(const QJsonObject &post)
{
    QJsonArray posts = loadAnonymousPosts();
    posts.insert(0, post);
    while (posts.size() > 80) {
        posts.removeAt(posts.size() - 1);
    }

    QFile file(anonymousPostsPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(posts).toJson(QJsonDocument::Indented));
    return true;
}

bool DataStore::writeEntries(const QVector<MoodEntry> &entries) const
{
    QJsonArray arr;
    for (const MoodEntry &entry : entries) {
        arr.append(entry.toJson());
    }

    QFile file(m_dataFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject DataStore::loadSettings() const
{
    QFile file(m_settingsFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

bool DataStore::writeSettings(const QJsonObject &obj) const
{
    QFile file(m_settingsFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

LlmConfig DataStore::loadLlmConfig() const
{
    const QJsonObject settings = loadSettings();
    const QJsonObject llm = settings.value(QStringLiteral("llm")).toObject();

    LlmConfig config;
    config.apiKey = llm.value(QStringLiteral("apiKey")).toString();
    config.baseUrl = llm.value(QStringLiteral("baseUrl")).toString(config.baseUrl);
    config.model = llm.value(QStringLiteral("model")).toString(config.model);
    config.timeoutMs = llm.value(QStringLiteral("timeoutMs")).toInt(config.timeoutMs);

    if (!LlmConfig::isValidApiKey(config.apiKey)) {
        config.apiKey.clear();
        config.enabled = false;
    } else {
        config.enabled = llm.value(QStringLiteral("enabled")).toBool(true);
    }
    return config;
}

LlmConfig DataStore::resolveLlmConfig() const
{
    LlmConfig config = loadLlmConfig();

    if (!LlmConfig::isValidApiKey(config.apiKey)) {
        const QByteArray envKey = qgetenv("DEEPSEEK_API_KEY");
        const QByteArray altKey = qgetenv("MOODCANVAS_API_KEY");
        const QString fromEnv = envKey.isEmpty() ? QString::fromUtf8(altKey) : QString::fromUtf8(envKey);
        if (LlmConfig::isValidApiKey(fromEnv)) {
            config.apiKey = fromEnv.trimmed();
            config.enabled = true;
        }
    }

    if (LlmConfig::isValidApiKey(config.apiKey)) {
        config.enabled = true;
    }

    return config;
}

bool DataStore::saveLlmConfig(const LlmConfig &config)
{
    QJsonObject settings = loadSettings();
    QJsonObject llm;
    llm.insert(QStringLiteral("enabled"), config.enabled);
    llm.insert(QStringLiteral("apiKey"), config.apiKey.trimmed());
    llm.insert(QStringLiteral("baseUrl"), config.baseUrl.trimmed());
    llm.insert(QStringLiteral("model"), config.model.trimmed());
    llm.insert(QStringLiteral("timeoutMs"), config.timeoutMs);
    settings.insert(QStringLiteral("llm"), llm);
    return writeSettings(settings);
}

CloudConfig DataStore::loadCloudConfig() const
{
    const QJsonObject settings = loadSettings();
    const QJsonObject cloud = settings.value(QStringLiteral("cloud")).toObject();

    CloudConfig config;
    config.serverUrl = cloud.value(QStringLiteral("serverUrl")).toString(config.serverUrl);
    config.token = cloud.value(QStringLiteral("token")).toString();
    config.username = cloud.value(QStringLiteral("username")).toString();
    config.nickname = cloud.value(QStringLiteral("nickname")).toString();
    return config;
}

bool DataStore::saveCloudConfig(const CloudConfig &config)
{
    QJsonObject settings = loadSettings();
    QJsonObject cloud;
    cloud.insert(QStringLiteral("serverUrl"), config.serverUrl.trimmed());
    cloud.insert(QStringLiteral("token"), config.token);
    cloud.insert(QStringLiteral("username"), config.username);
    cloud.insert(QStringLiteral("nickname"), config.nickname);
    settings.insert(QStringLiteral("cloud"), cloud);
    return writeSettings(settings);
}
