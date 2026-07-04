#ifndef CLOUDTREEHOLE_H
#define CLOUDTREEHOLE_H

#include "cloudapiclient.h"

#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVector>

class CloudTreehole : public QObject
{
    Q_OBJECT

public:
    explicit CloudTreehole(CloudApiClient *api, QObject *parent = nullptr);

    bool isActive() const;
    QString statusText() const;
    QVector<QJsonObject> posts() const;

    void startSync(int intervalMs = 3000);
    void stopSync();
    void refreshNow();

    bool publishPost(const QJsonObject &post, QString *errorOut = nullptr);
    bool publishReply(const QString &postId, const QJsonObject &reply, QString *errorOut = nullptr);

signals:
    void statusChanged(const QString &status);
    void postsChanged();

private:
    CloudApiClient *m_api = nullptr;
    QTimer m_timer;
    QVector<QJsonObject> m_posts;
    QString m_statusText = QStringLiteral("正在连接云端树洞…");
    bool m_active = false;

    void setStatus(const QString &status);
    void pollPosts();
};

#endif // CLOUDTREEHOLE_H
