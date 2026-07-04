#include "cloudtreehole.h"

CloudTreehole::CloudTreehole(CloudApiClient *api, QObject *parent)
    : QObject(parent),
      m_api(api)
{
    connect(&m_timer, &QTimer::timeout, this, &CloudTreehole::pollPosts);
}

bool CloudTreehole::isActive() const
{
    return m_active;
}

QString CloudTreehole::statusText() const
{
    return m_statusText;
}

QVector<QJsonObject> CloudTreehole::posts() const
{
    return m_posts;
}

void CloudTreehole::startSync(int intervalMs)
{
    m_timer.start(intervalMs);
    refreshNow();
}

void CloudTreehole::stopSync()
{
    m_timer.stop();
    m_active = false;
    setStatus(QStringLiteral("已断开云端树洞"));
}

void CloudTreehole::refreshNow()
{
    pollPosts();
}

bool CloudTreehole::publishPost(const QJsonObject &post, QString *errorOut)
{
    if (!m_api) {
        if (errorOut) {
            *errorOut = QStringLiteral("云端客户端未初始化。");
        }
        return false;
    }

    if (!m_api->publishTreeholePost(post, errorOut)) {
        return false;
    }

    refreshNow();
    return true;
}

bool CloudTreehole::publishReply(const QString &postId, const QJsonObject &reply, QString *errorOut)
{
    if (!m_api) {
        if (errorOut) {
            *errorOut = QStringLiteral("云端客户端未初始化。");
        }
        return false;
    }

    if (!m_api->publishTreeholeReply(postId, reply, errorOut)) {
        return false;
    }

    refreshNow();
    return true;
}

void CloudTreehole::setStatus(const QString &status)
{
    if (m_statusText == status) {
        return;
    }
    m_statusText = status;
    emit statusChanged(status);
}

void CloudTreehole::pollPosts()
{
    if (!m_api) {
        m_active = false;
        setStatus(QStringLiteral("云端客户端未初始化"));
        return;
    }

    QString error;
    QVector<QJsonObject> posts;
    if (!m_api->fetchTreeholePosts(&posts, &error)) {
        m_active = false;
        setStatus(error);
        return;
    }

    m_active = true;
    m_posts = posts;
    setStatus(QStringLiteral("已连接云端树洞，共 %1 条心象").arg(m_posts.size()));
    emit postsChanged();
}
