#include "moodentry.h"

#include <QJsonArray>
#include <QUuid>

static QJsonArray stringListToArray(const QStringList &list)
{
    QJsonArray arr;
    for (const QString &item : list) {
        arr.append(item);
    }
    return arr;
}

static QStringList arrayToStringList(const QJsonArray &arr)
{
    QStringList list;
    for (const QJsonValue &value : arr) {
        list << value.toString();
    }
    return list;
}

QJsonObject MoodEntry::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["date"] = date.toString(Qt::ISODate);
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["rawText"] = rawText;
    obj["emoji"] = emoji;
    obj["emotion"] = emotion;
    obj["emotionSummary"] = emotionSummary;
    obj["valence"] = valence;
    obj["arousal"] = arousal;
    obj["keywords"] = stringListToArray(keywords);
    obj["triggerTags"] = stringListToArray(triggerTags);
    obj["cbtHint"] = cbtHint;
    obj["comfortText"] = comfortText;
    obj["analysisSource"] = analysisSource;
    obj["mainColor"] = mainColor.name(QColor::HexRgb);

    QJsonObject weightsObj;
    for (auto it = emotionWeights.constBegin(); it != emotionWeights.constEnd(); ++it) {
        weightsObj[it.key()] = it.value();
    }
    obj["emotionWeights"] = weightsObj;

    QJsonArray paletteArr;
    for (const QColor &c : colorPalette) {
        paletteArr.append(c.name(QColor::HexRgb));
    }
    obj["colorPalette"] = paletteArr;

    return obj;
}

MoodEntry MoodEntry::fromJson(const QJsonObject &obj)
{
    MoodEntry entry;
    entry.id = obj["id"].toString();
    entry.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
    entry.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
    entry.rawText = obj["rawText"].toString();
    entry.emoji = obj["emoji"].toString();
    entry.emotion = obj["emotion"].toString();
    entry.emotionSummary = obj["emotionSummary"].toString();
    entry.valence = obj["valence"].toDouble();
    entry.arousal = obj["arousal"].toDouble();
    entry.keywords = arrayToStringList(obj["keywords"].toArray());
    entry.triggerTags = arrayToStringList(obj["triggerTags"].toArray());
    entry.cbtHint = obj["cbtHint"].toString();
    entry.comfortText = obj["comfortText"].toString();
    entry.analysisSource = obj["analysisSource"].toString();
    entry.mainColor = QColor(obj["mainColor"].toString("#8ecae6"));

    const QJsonObject weightsObj = obj["emotionWeights"].toObject();
    for (auto it = weightsObj.constBegin(); it != weightsObj.constEnd(); ++it) {
        entry.emotionWeights[it.key()] = it.value().toDouble();
    }

    for (const QJsonValue &v : obj["colorPalette"].toArray()) {
        entry.colorPalette.append(QColor(v.toString()));
    }

    if (entry.id.isEmpty()) {
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (!entry.date.isValid()) {
        entry.date = QDate::currentDate();
    }
    if (!entry.createdAt.isValid()) {
        entry.createdAt = QDateTime(entry.date, QTime(12, 0));
    }
    if (!entry.mainColor.isValid()) {
        entry.mainColor = QColor("#8ecae6");
    }

    return entry;
}
