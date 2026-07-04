#ifndef TREEHOLENETWORK_H
#define TREEHOLENETWORK_H

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

class TreeholeNetwork : public QObject
{
    Q_OBJECT

public:
    static constexpr quint16 DefaultPort = 47521;

    explicit TreeholeNetwork(QObject *parent = nullptr);
    ~TreeholeNetwork() override;

    bool isHosting() const;
    bool isConnected() const;
    quint16 hostPort() const;
    QString statusText() const;

    QVector<QJsonObject> posts() const;

    bool startHost(quint16 port = DefaultPort);
    void stopHost();
    bool connectToServer(const QString &host, quint16 port = DefaultPort);
    void disconnectFromServer();

    void setIdentity(const QString &username, const QString &nickname);
    void publishPost(const QJsonObject &post);
    void publishReply(const QString &postId, const QJsonObject &reply);

signals:
    void connectionChanged(bool active);
    void statusChanged(const QString &status);
    void postsChanged();
    void userJoined(const QString &nickname);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
    void onUpstreamReadyRead();
    void onUpstreamDisconnected();
    void onUpstreamConnected();

private:
    QTcpServer *m_server = nullptr;
    QTcpSocket *m_upstream = nullptr;
    QList<QTcpSocket *> m_clients;
    QHash<QTcpSocket *, QByteArray> m_buffers;

    bool m_isHost = false;
    quint16 m_hostPort = DefaultPort;
    QString m_statusText = QStringLiteral("未连接");
    QString m_username;
    QString m_nickname;

    QVector<QJsonObject> m_posts;

    void setStatus(const QString &status);
    void sendJson(QTcpSocket *socket, const QJsonObject &message);
    void broadcastJson(const QJsonObject &message, QTcpSocket *except = nullptr);
    void processBuffer(QTcpSocket *socket, QByteArray &buffer);
    void handleMessage(QTcpSocket *source, const QJsonObject &message);
    void sendSync(QTcpSocket *socket);
    int findPostIndex(const QString &postId) const;
    void appendReplyToPost(const QString &postId, const QJsonObject &reply);
    QJsonObject makeJoinMessage() const;
};

#endif // TREEHOLENETWORK_H
