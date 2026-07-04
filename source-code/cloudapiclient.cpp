#include "cloudapiclient.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

CloudApiClient::CloudApiClient(QObject *parent)
    : QObject(parent),
      m_manager(new QNetworkAccessManager(this))
{
}

void CloudApiClient::setServerUrl(const QString &url)
{
    QString trimmed = url.trimmed();
    while (trimmed.endsWith(QLatin1Char('/'))) {
        trimmed.chop(1);
    }
    m_serverUrl = trimmed.isEmpty() ? QStringLiteral("http://127.0.0.1:8765") : trimmed;
}

QString CloudApiClient::serverUrl() const
{
    return m_serverUrl;
}

void CloudApiClient::setToken(const QString &token)
{
    m_token = token.trimmed();
}

QString CloudApiClient::token() const
{
    return m_token;
}

QString CloudApiClient::lastError() const
{
    return m_lastError;
}

QString CloudApiClient::normalizedBaseUrl() const
{
    return m_serverUrl;
}

QString CloudApiClient::extractErrorMessage(const QJsonObject &response, int statusCode)
{
    const QString detail = response.value(QStringLiteral("detail")).toString();
    if (!detail.isEmpty()) {
        return detail;
    }

    const QString error = response.value(QStringLiteral("error")).toString();
    if (!error.isEmpty()) {
        return error;
    }

    return QStringLiteral("云端请求失败（HTTP %1）。").arg(statusCode);
}

bool CloudApiClient::requestJson(const QString &method,
                                 const QString &path,
                                 const QJsonObject &body,
                                 QJsonObject *responseOut,
                                 int *statusCodeOut,
                                 bool auth)
{
    m_lastError.clear();

    const QUrl url(normalizedBaseUrl() + path);
    if (!url.isValid()) {
        m_lastError = QStringLiteral("云端地址无效，请在设置中检查。");
        return false;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setTransferTimeout(m_timeoutMs);
    if (auth && !m_token.isEmpty()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + m_token.toUtf8());
    }

    QNetworkReply *reply = nullptr;
    if (method == QStringLiteral("GET")) {
        reply = m_manager->get(request);
    } else if (method == QStringLiteral("POST")) {
        reply = m_manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    } else {
        m_lastError = QStringLiteral("不支持的 HTTP 方法。");
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(m_timeoutMs);
    loop.exec();

    const bool timedOut = !reply->isFinished();
    if (timedOut) {
        reply->abort();
        m_lastError = QStringLiteral("连接云端超时，请确认服务器已启动且地址正确。");
        reply->deleteLater();
        return false;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCodeOut) {
        *statusCodeOut = statusCode;
    }

    const QByteArray payload = reply->readAll();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError && statusCode == 0) {
        m_lastError = QStringLiteral("无法连接云端服务器。\n请先运行 server\\start_server.bat，或在设置里填写正确的云端地址。");
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    QJsonObject response = doc.isObject() ? doc.object() : QJsonObject();

    if (statusCode < 200 || statusCode >= 300) {
        m_lastError = extractErrorMessage(response, statusCode);
        return false;
    }

    if (responseOut) {
        *responseOut = response;
    }
    return true;
}

bool CloudApiClient::checkHealth(QString *errorOut)
{
    QJsonObject response;
    const bool ok = requestJson(QStringLiteral("GET"), QStringLiteral("/api/health"), QJsonObject(), &response);
    if (!ok && errorOut) {
        *errorOut = m_lastError;
    }
    return ok && response.value(QStringLiteral("ok")).toBool();
}

bool CloudApiClient::registerUser(const QString &username,
                                  const QString &password,
                                  const QString &nickname,
                                  UserProfile *profileOut,
                                  QString *errorOut)
{
    QJsonObject body;
    body.insert(QStringLiteral("username"), username.trimmed());
    body.insert(QStringLiteral("password"), password);
    body.insert(QStringLiteral("nickname"), nickname.trimmed());

    QJsonObject response;
    if (!requestJson(QStringLiteral("POST"), QStringLiteral("/api/auth/register"), body, &response)) {
        if (errorOut) {
            *errorOut = m_lastError;
        }
        return false;
    }

    m_token = response.value(QStringLiteral("token")).toString();
    if (profileOut) {
        profileOut->username = response.value(QStringLiteral("username")).toString(username.trimmed());
        profileOut->nickname = response.value(QStringLiteral("nickname")).toString(profileOut->username);
        profileOut->authToken = m_token;
    }
    return true;
}

bool CloudApiClient::loginUser(const QString &username,
                               const QString &password,
                               UserProfile *profileOut,
                               QString *errorOut)
{
    QJsonObject body;
    body.insert(QStringLiteral("username"), username.trimmed());
    body.insert(QStringLiteral("password"), password);

    QJsonObject response;
    if (!requestJson(QStringLiteral("POST"), QStringLiteral("/api/auth/login"), body, &response)) {
        if (errorOut) {
            *errorOut = m_lastError;
        }
        return false;
    }

    m_token = response.value(QStringLiteral("token")).toString();
    if (profileOut) {
        profileOut->username = response.value(QStringLiteral("username")).toString(username.trimmed());
        profileOut->nickname = response.value(QStringLiteral("nickname")).toString(profileOut->username);
        profileOut->authToken = m_token;
    }
    return true;
}

bool CloudApiClient::fetchTreeholePosts(QVector<QJsonObject> *postsOut, QString *errorOut)
{
    QJsonObject response;
    if (!requestJson(QStringLiteral("GET"), QStringLiteral("/api/treehole/posts"), QJsonObject(), &response, nullptr, true)) {
        if (errorOut) {
            *errorOut = m_lastError;
        }
        return false;
    }

    if (postsOut) {
        postsOut->clear();
        const QJsonArray posts = response.value(QStringLiteral("posts")).toArray();
        for (const QJsonValue &value : posts) {
            if (value.isObject()) {
                postsOut->append(value.toObject());
            }
        }
    }
    return true;
}

bool CloudApiClient::publishTreeholePost(const QJsonObject &post, QString *errorOut)
{
    QJsonObject body;
    body.insert(QStringLiteral("moodLine"), post.value(QStringLiteral("moodLine")).toString());
    body.insert(QStringLiteral("emotion"), post.value(QStringLiteral("emotion")).toString(QStringLiteral("心象")));
    body.insert(QStringLiteral("color"), post.value(QStringLiteral("color")).toString(QStringLiteral("#bdebd9")));
    body.insert(QStringLiteral("entryId"), post.value(QStringLiteral("entryId")).toString());

    QJsonObject response;
    if (!requestJson(QStringLiteral("POST"), QStringLiteral("/api/treehole/posts"), body, &response, nullptr, true)) {
        if (errorOut) {
            *errorOut = m_lastError;
        }
        return false;
    }
    Q_UNUSED(response);
    return true;
}

bool CloudApiClient::publishTreeholeReply(const QString &postId,
                                          const QJsonObject &reply,
                                          QString *errorOut)
{
    QJsonObject body;
    body.insert(QStringLiteral("text"), reply.value(QStringLiteral("text")).toString());

    QJsonObject response;
    const QString path = QStringLiteral("/api/treehole/posts/") + postId + QStringLiteral("/replies");
    if (!requestJson(QStringLiteral("POST"), path, body, &response, nullptr, true)) {
        if (errorOut) {
            *errorOut = m_lastError;
        }
        return false;
    }
    Q_UNUSED(response);
    return true;
}
