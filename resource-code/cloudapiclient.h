#ifndef CLOUDAPICLIENT_H
#define CLOUDAPICLIENT_H

#include "userprofile.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QVector>

class QNetworkAccessManager;

class CloudApiClient : public QObject
{
    Q_OBJECT

public:
    explicit CloudApiClient(QObject *parent = nullptr);

    void setServerUrl(const QString &url);
    QString serverUrl() const;
    void setToken(const QString &token);
    QString token() const;
    QString lastError() const;

    bool checkHealth(QString *errorOut = nullptr);

    bool registerUser(const QString &username,
                      const QString &password,
                      const QString &nickname,
                      UserProfile *profileOut = nullptr,
                      QString *errorOut = nullptr);

    bool loginUser(const QString &username,
                   const QString &password,
                   UserProfile *profileOut = nullptr,
                   QString *errorOut = nullptr);

    bool fetchTreeholePosts(QVector<QJsonObject> *postsOut, QString *errorOut = nullptr);

    bool publishTreeholePost(const QJsonObject &post, QString *errorOut = nullptr);

    bool publishTreeholeReply(const QString &postId,
                              const QJsonObject &reply,
                              QString *errorOut = nullptr);

private:
    QNetworkAccessManager *m_manager = nullptr;
    QString m_serverUrl = QStringLiteral("http://127.0.0.1:8765");
    QString m_token;
    QString m_lastError;
    int m_timeoutMs = 15000;

    QString normalizedBaseUrl() const;
    bool requestJson(const QString &method,
                     const QString &path,
                     const QJsonObject &body,
                     QJsonObject *responseOut,
                     int *statusCodeOut = nullptr,
                     bool auth = false);
    static QString extractErrorMessage(const QJsonObject &response, int statusCode);
};

#endif // CLOUDAPICLIENT_H
