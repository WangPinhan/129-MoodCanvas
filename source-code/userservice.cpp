#include "userservice.h"

#include "datastore.h"

UserService::UserService(DataStore *store, QObject *parent)
    : QObject(parent),
      m_store(store)
{
    if (!m_store) {
        return;
    }

    const CloudConfig config = m_store->loadCloudConfig();
    m_api.setServerUrl(config.serverUrl);
    if (config.hasSession()) {
        m_api.setToken(config.token);
        m_currentUser.username = config.username;
        m_currentUser.nickname = config.nickname;
        m_currentUser.authToken = config.token;
    }
}

UserProfile UserService::currentUser() const
{
    return m_currentUser;
}

QString UserService::lastUsername() const
{
    if (!m_store) {
        return QString();
    }
    return m_store->loadCloudConfig().username;
}

CloudApiClient *UserService::apiClient()
{
    return &m_api;
}

void UserService::logout()
{
    m_currentUser = UserProfile();
    m_api.setToken(QString());
    clearSession();
}

bool UserService::isValidUsername(const QString &username, QString *errorOut)
{
    const QString trimmed = username.trimmed();
    if (trimmed.isEmpty()) {
        if (errorOut) {
            *errorOut = QStringLiteral("请输入用户名。");
        }
        return false;
    }

    if (trimmed.size() < 3 || trimmed.size() > 20) {
        if (errorOut) {
            *errorOut = QStringLiteral("用户名长度需在 3 到 20 个字符之间。");
        }
        return false;
    }

    for (const QChar &ch : trimmed) {
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
            continue;
        }
        if (errorOut) {
            *errorOut = QStringLiteral("用户名只能包含中文、字母、数字或下划线。");
        }
        return false;
    }

    return true;
}

bool UserService::registerUser(const QString &username,
                               const QString &password,
                               const QString &nickname,
                               QString *errorOut)
{
    QString usernameError;
    if (!isValidUsername(username, &usernameError)) {
        if (errorOut) {
            *errorOut = usernameError;
        }
        return false;
    }

    if (password.size() < 6) {
        if (errorOut) {
            *errorOut = QStringLiteral("密码至少 6 位。");
        }
        return false;
    }

    if (m_store) {
        m_api.setServerUrl(m_store->loadCloudConfig().serverUrl);
    }

    QString healthError;
    if (!m_api.checkHealth(&healthError)) {
        if (errorOut) {
            *errorOut = healthError;
        }
        return false;
    }

    UserProfile profile;
    if (!m_api.registerUser(username, password, nickname, &profile, errorOut)) {
        return false;
    }

    m_currentUser = profile;
    saveSession(profile);
    return true;
}

bool UserService::login(const QString &username,
                        const QString &password,
                        UserProfile *profileOut,
                        QString *errorOut)
{
    QString usernameError;
    if (!isValidUsername(username, &usernameError)) {
        if (errorOut) {
            *errorOut = usernameError;
        }
        return false;
    }

    if (m_store) {
        m_api.setServerUrl(m_store->loadCloudConfig().serverUrl);
    }

    QString healthError;
    if (!m_api.checkHealth(&healthError)) {
        if (errorOut) {
            *errorOut = healthError;
        }
        return false;
    }

    UserProfile profile;
    if (!m_api.loginUser(username, password, &profile, errorOut)) {
        return false;
    }

    m_currentUser = profile;
    saveSession(profile);

    if (profileOut) {
        *profileOut = profile;
    }
    return true;
}

void UserService::saveSession(const UserProfile &profile)
{
    if (!m_store) {
        return;
    }

    CloudConfig config = m_store->loadCloudConfig();
    config.token = profile.authToken;
    config.username = profile.username;
    config.nickname = profile.nickname;
    m_store->saveCloudConfig(config);
}

void UserService::clearSession()
{
    if (!m_store) {
        return;
    }

    CloudConfig config = m_store->loadCloudConfig();
    config.token.clear();
    config.username.clear();
    config.nickname.clear();
    m_store->saveCloudConfig(config);
}
