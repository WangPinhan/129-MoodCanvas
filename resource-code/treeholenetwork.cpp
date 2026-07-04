#include "treeholenetwork.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QUuid>

namespace {

QString messageType(const QJsonObject &message)
{
    return message.value(QStringLiteral("type")).toString();
}

QJsonObject cloneWithoutType(const QJsonObject &message)
{
    QJsonObject copy = message;
    copy.remove(QStringLiteral("type"));
    return copy;
}

} // namespace

TreeholeNetwork::TreeholeNetwork(QObject *parent)
    : QObject(parent)
{
}

TreeholeNetwork::~TreeholeNetwork()
{
    disconnectFromServer();
    stopHost();
}

bool TreeholeNetwork::isHosting() const
{
    return m_isHost && m_server && m_server->isListening();
}

bool TreeholeNetwork::isConnected() const
{
    return isHosting() || (m_upstream && m_upstream->state() == QAbstractSocket::ConnectedState);
}

quint16 TreeholeNetwork::hostPort() const
{
    return m_hostPort;
}

QString TreeholeNetwork::statusText() const
{
    return m_statusText;
}

QVector<QJsonObject> TreeholeNetwork::posts() const
{
    return m_posts;
}

void TreeholeNetwork::setIdentity(const QString &username, const QString &nickname)
{
    m_username = username.trimmed();
    m_nickname = nickname.trimmed().isEmpty() ? m_username : nickname.trimmed();

    if (m_upstream && m_upstream->state() == QAbstractSocket::ConnectedState) {
        sendJson(m_upstream, makeJoinMessage());
    }
}

bool TreeholeNetwork::startHost(quint16 port)
{
    if (isHosting()) {
        return true;
    }

    if (m_upstream) {
        m_upstream->disconnectFromHost();
    }

    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &TreeholeNetwork::onNewConnection);
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        setStatus(QStringLiteral("无法开启树洞房间：%1").arg(m_server->errorString()));
        return false;
    }

    m_isHost = true;
    m_hostPort = m_server->serverPort();
    setStatus(QStringLiteral("已开启树洞房间，端口 %1，等待其他用户加入…").arg(m_hostPort));
    emit connectionChanged(true);
    return true;
}

void TreeholeNetwork::stopHost()
{
    if (!m_server) {
        m_isHost = false;
        return;
    }

    for (QTcpSocket *client : std::as_const(m_clients)) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
    m_buffers.clear();

    m_server->close();
    m_isHost = false;

    if (!isConnected()) {
        setStatus(QStringLiteral("未连接"));
        emit connectionChanged(false);
    } else {
        setStatus(QStringLiteral("已关闭树洞房间，仍作为客户端在线"));
    }
}

bool TreeholeNetwork::connectToServer(const QString &host, quint16 port)
{
    if (host.trimmed().isEmpty()) {
        setStatus(QStringLiteral("请填写服务器地址。"));
        return false;
    }

    if (m_upstream) {
        m_upstream->disconnectFromHost();
        m_upstream->deleteLater();
        m_upstream = nullptr;
    }

    m_upstream = new QTcpSocket(this);
    connect(m_upstream, &QTcpSocket::readyRead, this, &TreeholeNetwork::onUpstreamReadyRead);
    connect(m_upstream, &QTcpSocket::disconnected, this, &TreeholeNetwork::onUpstreamDisconnected);
    connect(m_upstream, &QTcpSocket::connected, this, &TreeholeNetwork::onUpstreamConnected);

    setStatus(QStringLiteral("正在连接 %1:%2 …").arg(host.trimmed()).arg(port));
    m_upstream->connectToHost(host.trimmed(), port);
    return true;
}

void TreeholeNetwork::disconnectFromServer()
{
    if (!m_upstream) {
        if (!isHosting()) {
            setStatus(QStringLiteral("未连接"));
            emit connectionChanged(false);
        }
        return;
    }

    m_upstream->disconnectFromHost();
    m_upstream->deleteLater();
    m_upstream = nullptr;

    if (!isHosting()) {
        m_posts.clear();
        emit postsChanged();
        setStatus(QStringLiteral("未连接"));
        emit connectionChanged(false);
    } else {
        setStatus(QStringLiteral("已开启树洞房间，端口 %1").arg(m_hostPort));
    }
}

void TreeholeNetwork::publishPost(const QJsonObject &post)
{
    QJsonObject message = post;
    message.insert(QStringLiteral("type"), QStringLiteral("post"));
    if (!message.contains(QStringLiteral("id"))) {
        message.insert(QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (!message.contains(QStringLiteral("createdAt"))) {
        message.insert(QStringLiteral("createdAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    }
    if (!message.contains(QStringLiteral("replies"))) {
        message.insert(QStringLiteral("replies"), QJsonArray());
    }

    if (isHosting()) {
        m_posts.prepend(message);
        while (m_posts.size() > 100) {
            m_posts.removeLast();
        }
        broadcastJson(message);
        emit postsChanged();
        return;
    }

    if (m_upstream && m_upstream->state() == QAbstractSocket::ConnectedState) {
        sendJson(m_upstream, message);
        QJsonObject localPost = cloneWithoutType(message);
        m_posts.prepend(localPost);
        while (m_posts.size() > 100) {
            m_posts.removeLast();
        }
        emit postsChanged();
    }
}

void TreeholeNetwork::publishReply(const QString &postId, const QJsonObject &reply)
{
    QJsonObject message = reply;
    message.insert(QStringLiteral("type"), QStringLiteral("reply"));
    message.insert(QStringLiteral("postId"), postId);
    if (!message.contains(QStringLiteral("id"))) {
        message.insert(QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (!message.contains(QStringLiteral("createdAt"))) {
        message.insert(QStringLiteral("createdAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    }

    if (isHosting()) {
        appendReplyToPost(postId, message);
        broadcastJson(message);
        emit postsChanged();
        return;
    }

    if (m_upstream && m_upstream->state() == QAbstractSocket::ConnectedState) {
        sendJson(m_upstream, message);
        QJsonObject localReply = cloneWithoutType(message);
        localReply.remove(QStringLiteral("postId"));
        appendReplyToPost(postId, localReply);
        emit postsChanged();
    }
}

void TreeholeNetwork::setStatus(const QString &status)
{
    if (m_statusText == status) {
        return;
    }
    m_statusText = status;
    emit statusChanged(status);
}

void TreeholeNetwork::sendJson(QTcpSocket *socket, const QJsonObject &message)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QJsonDocument doc(message);
    socket->write(doc.toJson(QJsonDocument::Compact));
    socket->write("\n");
}

void TreeholeNetwork::broadcastJson(const QJsonObject &message, QTcpSocket *except)
{
    for (QTcpSocket *client : std::as_const(m_clients)) {
        if (client == except) {
            continue;
        }
        sendJson(client, message);
    }

    if (m_upstream && m_upstream != except && m_upstream->state() == QAbstractSocket::ConnectedState) {
        sendJson(m_upstream, message);
    }
}

void TreeholeNetwork::processBuffer(QTcpSocket *socket, QByteArray &buffer)
{
    while (true) {
        const int newlineIndex = buffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);
        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }

        handleMessage(socket, doc.object());
    }
}

void TreeholeNetwork::handleMessage(QTcpSocket *source, const QJsonObject &message)
{
    const QString type = messageType(message);

    if (type == QStringLiteral("join")) {
        const QString nickname = message.value(QStringLiteral("nickname")).toString();
        if (!nickname.isEmpty()) {
            emit userJoined(nickname);
        }
        if (isHosting() && source) {
            sendSync(source);
        }
        return;
    }

    if (type == QStringLiteral("sync")) {
        m_posts.clear();
        const QJsonArray items = message.value(QStringLiteral("posts")).toArray();
        for (const QJsonValue &value : items) {
            if (value.isObject()) {
                m_posts.append(value.toObject());
            }
        }
        emit postsChanged();
        return;
    }

    if (type == QStringLiteral("post")) {
        QJsonObject post = cloneWithoutType(message);
        if (findPostIndex(post.value(QStringLiteral("id")).toString()) >= 0) {
            return;
        }
        if (isHosting()) {
            m_posts.prepend(post);
            while (m_posts.size() > 100) {
                m_posts.removeLast();
            }
            broadcastJson(message, source);
        } else {
            m_posts.prepend(post);
            while (m_posts.size() > 100) {
                m_posts.removeLast();
            }
        }
        emit postsChanged();
        return;
    }

    if (type == QStringLiteral("reply")) {
        const QString postId = message.value(QStringLiteral("postId")).toString();
        QJsonObject reply = cloneWithoutType(message);
        reply.remove(QStringLiteral("postId"));

        if (isHosting()) {
            appendReplyToPost(postId, reply);
            broadcastJson(message, source);
        } else {
            appendReplyToPost(postId, reply);
        }
        emit postsChanged();
        return;
    }
}

void TreeholeNetwork::sendSync(QTcpSocket *socket)
{
    QJsonArray items;
    for (const QJsonObject &post : std::as_const(m_posts)) {
        items.append(post);
    }

    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("sync"));
    message.insert(QStringLiteral("posts"), items);
    sendJson(socket, message);
}

int TreeholeNetwork::findPostIndex(const QString &postId) const
{
    for (int i = 0; i < m_posts.size(); ++i) {
        if (m_posts.at(i).value(QStringLiteral("id")).toString() == postId) {
            return i;
        }
    }
    return -1;
}

void TreeholeNetwork::appendReplyToPost(const QString &postId, const QJsonObject &reply)
{
    const int index = findPostIndex(postId);
    if (index < 0) {
        return;
    }

    QJsonObject post = m_posts.at(index);
    QJsonArray replies = post.value(QStringLiteral("replies")).toArray();
    replies.append(reply);
    post.insert(QStringLiteral("replies"), replies);
    m_posts[index] = post;
}

QJsonObject TreeholeNetwork::makeJoinMessage() const
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("join"));
    message.insert(QStringLiteral("username"), m_username);
    message.insert(QStringLiteral("nickname"), m_nickname);
    return message;
}

void TreeholeNetwork::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        m_clients.append(client);
        m_buffers.insert(client, QByteArray());

        connect(client, &QTcpSocket::readyRead, this, &TreeholeNetwork::onClientReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &TreeholeNetwork::onClientDisconnected);

        sendSync(client);
        if (!m_username.isEmpty()) {
            sendJson(client, makeJoinMessage());
        }
    }
}

void TreeholeNetwork::onClientReadyRead()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

    m_buffers[socket].append(socket->readAll());
    processBuffer(socket, m_buffers[socket]);
}

void TreeholeNetwork::onClientDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

    m_clients.removeAll(socket);
    m_buffers.remove(socket);
    socket->deleteLater();
}

void TreeholeNetwork::onUpstreamReadyRead()
{
    if (!m_upstream) {
        return;
    }

    m_buffers[m_upstream].append(m_upstream->readAll());
    processBuffer(m_upstream, m_buffers[m_upstream]);
}

void TreeholeNetwork::onUpstreamDisconnected()
{
    if (!isHosting()) {
        m_posts.clear();
        emit postsChanged();
        setStatus(QStringLiteral("与树洞服务器断开连接"));
        emit connectionChanged(false);
    } else {
        setStatus(QStringLiteral("与服务器断开，本地房间仍在运行（端口 %1）").arg(m_hostPort));
    }
}

void TreeholeNetwork::onUpstreamConnected()
{
    sendJson(m_upstream, makeJoinMessage());
    setStatus(QStringLiteral("已加入树洞房间"));
    emit connectionChanged(true);
}
